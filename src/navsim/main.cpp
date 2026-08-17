#include "io/IgtlParser.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace nnc
{
namespace navsim_detail
{

// Fixed plan in image-ish millimetres for Task 5 fiducial alignment later.
constexpr nnc::Vec3 kPlanEntry{0.f, 0.f, 0.f};
constexpr nnc::Vec3 kPlanTarget{0.f, 0.f, 80.f};

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

// Block until peer closes or an error; then close clientFd.
void waitUntilClientDisconnects(int clientFd)
{
  char discard[256];
  while (true)
  {
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
    // Ignore inbound bytes for now (OpenIGTLink queries come later if ever).
  }
  ::close(clientFd);
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

// Accept one client, send TRAJ, hold until disconnect. Returns false on hard failure.
bool acceptOneClientSession(int listenFd, std::string *error)
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

  nnc::navsim_detail::waitUntilClientDisconnects(clientFd);
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

  std::cout << "navsim: listening on port " << opts.port << " (rate=" << opts.rateHz
            << " Hz reserved for pose stream in 2d)" << std::endl;

  while (true)
  {
    if (!nnc::navsim_detail::acceptOneClientSession(listenFd, &err))
    {
      std::cerr << "navsim: " << err << std::endl;
      nnc::navsim_detail::closeFd(&listenFd);
      return 1;
    }
  }
}
