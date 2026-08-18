#include "io/IgtlParser.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace nnc
{
namespace navsim_detail
{

// Fixed plan in image millimetres (TRAJ on the wire). Task 5 fiducials align to this.
constexpr nnc::Vec3 kPlanEntry{0.f, 0.f, 0.f};
constexpr nnc::Vec3 kPlanTarget{0.f, 0.f, 80.f};

// Ground-truth image→tracker so registration is load-bearing (REQ-FRAME-003).
// toolToTracker = imageToTracker × toolInImage. Task 5 pairs must match the inverse.
constexpr float kImageToTrackerPitchRad = 30.f * 0.017453292519943295f; // 30 deg about +Y
constexpr float kImageToTrackerTx = 40.f;
constexpr float kImageToTrackerTy = -25.f;
constexpr float kImageToTrackerTz = 15.f;

nnc::Mat4 makeImageToTracker()
{
  const float c = std::cos(nnc::navsim_detail::kImageToTrackerPitchRad);
  const float s = std::sin(nnc::navsim_detail::kImageToTrackerPitchRad);
  nnc::Mat4 M = nnc::Mat4::identity();
  // Rotation about +Y, then translation (mm).
  M(0, 0) = c;
  M(0, 2) = s;
  M(2, 0) = -s;
  M(2, 2) = c;
  M(0, 3) = nnc::navsim_detail::kImageToTrackerTx;
  M(1, 3) = nnc::navsim_detail::kImageToTrackerTy;
  M(2, 3) = nnc::navsim_detail::kImageToTrackerTz;
  return M;
}

struct Options
{
  int port = 18944;
  int rateHz = 60;
  bool showHelp = false;
};

void printUsage(const char *argv0)
{
  std::cerr << "Usage: " << argv0 << " [--port N] [--rate Hz] [--help]\n"
            << "\n"
            << "  --port N   TCP listen port (default 18944)\n"
            << "  --rate Hz  Nominal pose stream rate (default 60; demo range ~30-60)\n"
            << "  --help     Show this help\n";
}

bool parseArgs(int argc, char **argv, Options &out, std::string *error)
{
  for (int i = 1; i < argc; ++i)
  {
    const char *arg = argv[i];
    if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0)
    {
      out.showHelp = true;
      return true;
    }
    if (std::strcmp(arg, "--port") == 0)
    {
      if (i + 1 >= argc)
      {
        if (error)
        {
          *error = "--port requires a value";
        }
        return false;
      }
      ++i;
      out.port = std::atoi(argv[i]);
      if (out.port <= 0 || out.port > 65535)
      {
        if (error)
        {
          *error = "--port must be in 1..65535";
        }
        return false;
      }
      continue;
    }
    if (std::strcmp(arg, "--rate") == 0)
    {
      if (i + 1 >= argc)
      {
        if (error)
        {
          *error = "--rate requires a value";
        }
        return false;
      }
      ++i;
      out.rateHz = std::atoi(argv[i]);
      if (out.rateHz <= 0)
      {
        if (error)
        {
          *error = "--rate must be a positive integer";
        }
        return false;
      }
      continue;
    }
    if (error)
    {
      *error = std::string("unknown argument: ") + arg;
    }
    return false;
  }
  return true;
}

void closeFd(int *fd)
{
  if (fd == nullptr || *fd < 0)
  {
    return;
  }
  ::close(*fd);
  *fd = -1;
}

// Returns listening socket fd, or -1 on failure (error set).
int createListeningSocket(int port, std::string *error)
{
  const int listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (listenFd < 0)
  {
    if (error)
    {
      *error = std::string("socket() failed: ") + std::strerror(errno);
    }
    return -1;
  }

  const int yes = 1;
  if (::setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
  {
    if (error)
    {
      *error = std::string("setsockopt(SO_REUSEADDR) failed: ") + std::strerror(errno);
    }
    ::close(listenFd);
    return -1;
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(static_cast<std::uint16_t>(port));

  if (::bind(listenFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
  {
    if (error)
    {
      *error = std::string("bind() failed: ") + std::strerror(errno);
    }
    ::close(listenFd);
    return -1;
  }

  if (::listen(listenFd, 1) < 0)
  {
    if (error)
    {
      *error = std::string("listen() failed: ") + std::strerror(errno);
    }
    ::close(listenFd);
    return -1;
  }

  return listenFd;
}

bool sendAll(int fd, const std::uint8_t *data, std::size_t size, std::string *error)
{
  std::size_t sent = 0;
  while (sent < size)
  {
    const ssize_t n = ::send(fd, data + sent, size - sent, MSG_NOSIGNAL);
    if (n < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }
      if (error)
      {
        *error = std::string("send() failed: ") + std::strerror(errno);
      }
      return false;
    }
    if (n == 0)
    {
      if (error)
      {
        *error = "send() returned 0";
      }
      return false;
    }
    sent += static_cast<std::size_t>(n);
  }
  return true;
}

// toolInImage: tip-at-origin, shaft +Z, tip slides entry→target in image mm.
nnc::Mat4 poseAlongPlan(float phase01)
{
  nnc::Mat4 pose = nnc::Mat4::identity();
  const float t = phase01 - static_cast<float>(static_cast<int>(phase01)); // wrap [0,1)
  pose(0, 3) = nnc::navsim_detail::kPlanEntry.x +
               t * (nnc::navsim_detail::kPlanTarget.x - nnc::navsim_detail::kPlanEntry.x);
  pose(1, 3) = nnc::navsim_detail::kPlanEntry.y +
               t * (nnc::navsim_detail::kPlanTarget.y - nnc::navsim_detail::kPlanEntry.y);
  pose(2, 3) = nnc::navsim_detail::kPlanEntry.z +
               t * (nnc::navsim_detail::kPlanTarget.z - nnc::navsim_detail::kPlanEntry.z);
  return pose;
}

nnc::Mat4 toolToTrackerAlongPlan(float phase01, const nnc::Mat4 &imageToTracker)
{
  return imageToTracker * nnc::navsim_detail::poseAlongPlan(phase01);
}

// Stream TRANSFORM poses at rateHz until the peer disconnects; then close clientFd.
bool streamPosesUntilDisconnect(int clientFd, int rateHz, std::string *error)
{
  if (rateHz <= 0)
  {
    if (error)
    {
      *error = "rateHz must be positive";
    }
    ::close(clientFd);
    return false;
  }

  const int periodMs = 1000 / rateHz;
  std::uint64_t frame = 0;
  const nnc::Mat4 imageToTracker = nnc::navsim_detail::makeImageToTracker();
  std::cout << "navsim: streaming TRANSFORM at " << rateHz
            << " Hz (image→tracker pitch=30deg t=(" << nnc::navsim_detail::kImageToTrackerTx
            << ',' << nnc::navsim_detail::kImageToTrackerTy << ','
            << nnc::navsim_detail::kImageToTrackerTz << "))" << std::endl;

  while (true)
  {
    pollfd pfd{};
    pfd.fd = clientFd;
    pfd.events = POLLIN;
    const int pr = ::poll(&pfd, 1, periodMs);
    if (pr < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }
      if (error)
      {
        *error = std::string("poll() failed: ") + std::strerror(errno);
      }
      ::close(clientFd);
      return false;
    }

    if (pr > 0 && (pfd.revents & (POLLIN | POLLHUP | POLLERR)) != 0)
    {
      char discard[256];
      const ssize_t n = ::recv(clientFd, discard, sizeof(discard), 0);
      if (n == 0)
      {
        break; // peer closed
      }
      if (n < 0)
      {
        if (errno == EINTR)
        {
          continue;
        }
        break;
      }
      // Ignore inbound bytes for now.
      continue;
    }

    // Timeout: send next pose. One full entry→target cycle about every 2 seconds.
    const float phase01 = static_cast<float>(frame % static_cast<std::uint64_t>(rateHz * 2)) /
                          static_cast<float>(rateHz * 2);
    const nnc::Mat4 toolToTracker =
      nnc::navsim_detail::toolToTrackerAlongPlan(phase01, imageToTracker);

    std::vector<std::uint8_t> bytes;
    if (!nnc::IgtlParser::packTransformMessage(toolToTracker, "Tool", frame, bytes, error))
    {
      ::close(clientFd);
      return false;
    }
    if (!nnc::navsim_detail::sendAll(clientFd, bytes.data(), bytes.size(), error))
    {
      // Peer gone mid-stream — end session without killing the server.
      if (error)
      {
        error->clear();
      }
      ::close(clientFd);
      return true;
    }
    ++frame;
  }

  ::close(clientFd);
  return true;
}

bool sendTrajectoryOnConnect(int clientFd, std::string *error)
{
  std::vector<std::uint8_t> bytes;
  if (!nnc::IgtlParser::packTrajectoryMessage(nnc::navsim_detail::kPlanEntry,
                                              nnc::navsim_detail::kPlanTarget,
                                              "Plan",
                                              0,
                                              bytes,
                                              error))
  {
    return false;
  }
  if (!nnc::navsim_detail::sendAll(clientFd, bytes.data(), bytes.size(), error))
  {
    return false;
  }
  std::cout << "navsim: sent TRAJ (" << bytes.size() << " bytes) entry=("
            << nnc::navsim_detail::kPlanEntry.x << ',' << nnc::navsim_detail::kPlanEntry.y << ','
            << nnc::navsim_detail::kPlanEntry.z << ") target=(" << nnc::navsim_detail::kPlanTarget.x
            << ',' << nnc::navsim_detail::kPlanTarget.y << ',' << nnc::navsim_detail::kPlanTarget.z
            << ')' << std::endl;
  return true;
}

// Accept one client, send TRAJ, then stream TRANSFORM until disconnect.
bool acceptOneClientSession(int listenFd, int rateHz, std::string *error)
{
  sockaddr_in peer{};
  socklen_t peerLen = sizeof(peer);
  const int clientFd = ::accept(listenFd, reinterpret_cast<sockaddr *>(&peer), &peerLen);
  if (clientFd < 0)
  {
    if (errno == EINTR)
    {
      return true; // interrupted; caller may retry
    }
    if (error)
    {
      *error = std::string("accept() failed: ") + std::strerror(errno);
    }
    return false;
  }

  char peerText[INET_ADDRSTRLEN] = {};
  ::inet_ntop(AF_INET, &peer.sin_addr, peerText, sizeof(peerText));
  std::cout << "navsim: client connected from " << peerText << ':' << ntohs(peer.sin_port)
            << std::endl;

  if (!nnc::navsim_detail::sendTrajectoryOnConnect(clientFd, error))
  {
    ::close(clientFd);
    return false;
  }

  if (!nnc::navsim_detail::streamPosesUntilDisconnect(clientFd, rateHz, error))
  {
    return false;
  }
  std::cout << "navsim: client disconnected" << std::endl;
  return true;
}

} // namespace navsim_detail
} // namespace nnc

int main(int argc, char **argv)
{
  nnc::navsim_detail::Options opts;
  std::string err;
  if (!nnc::navsim_detail::parseArgs(argc, argv, opts, &err))
  {
    std::cerr << "navsim: " << err << '\n';
    nnc::navsim_detail::printUsage(argv[0]);
    return 1;
  }
  if (opts.showHelp)
  {
    nnc::navsim_detail::printUsage(argv[0]);
    return 0;
  }

  int listenFd = nnc::navsim_detail::createListeningSocket(opts.port, &err);
  if (listenFd < 0)
  {
    std::cerr << "navsim: " << err << '\n';
    return 1;
  }

  std::cout << "navsim: listening on port " << opts.port << " (pose rate=" << opts.rateHz
            << " Hz)" << std::endl;

  while (true)
  {
    if (!nnc::navsim_detail::acceptOneClientSession(listenFd, opts.rateHz, &err))
    {
      std::cerr << "navsim: " << err << std::endl;
      nnc::navsim_detail::closeFd(&listenFd);
      return 1;
    }
  }
}
