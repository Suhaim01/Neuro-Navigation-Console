#include "io/IgtlReceiver.h"

#include "app/SceneModel.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QString>
#include <QTextStream>

#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace nnc
{
namespace igtl_receiver_detail
{

constexpr const char *kDefaultHost = "127.0.0.1";
constexpr int kDefaultPort = 18944;

QString readEnvFileValue(const QString &key)
{
  const QString path = QCoreApplication::applicationDirPath() + QStringLiteral("/../nnc.env");
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    return {};
  }
  QTextStream in(&file);
  while (!in.atEnd())
  {
    QString line = in.readLine().trimmed();
    if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
    {
      continue;
    }
    const int eq = line.indexOf(QLatin1Char('='));
    if (eq <= 0)
    {
      continue;
    }
    if (line.left(eq).trimmed() == key)
    {
      return line.mid(eq + 1).trimmed();
    }
  }
  return {};
}

bool setNonBlocking(int fd, std::string *error)
{
  const int flags = ::fcntl(fd, F_GETFL, 0);
  if (flags < 0)
  {
    if (error)
    {
      *error = std::string("fcntl(F_GETFL) failed: ") + std::strerror(errno);
    }
    return false;
  }
  if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
  {
    if (error)
    {
      *error = std::string("fcntl(O_NONBLOCK) failed: ") + std::strerror(errno);
    }
    return false;
  }
  return true;
}

} // namespace igtl_receiver_detail

IgtlReceiver::IgtlReceiver(QObject *parent)
  : QThread(parent)
{
}

IgtlReceiver::~IgtlReceiver()
{
  this->stopReceiver();
}

void IgtlReceiver::setEndpoint(const std::string &host, int port)
{
  std::lock_guard<std::mutex> lock(this->endpointMutex_);
  if (!host.empty())
  {
    this->host_ = host;
  }
  if (port > 0 && port <= 65535)
  {
    this->port_ = port;
  }
  this->endpointExplicit_ = true;
}

std::string IgtlReceiver::host() const
{
  std::lock_guard<std::mutex> lock(this->endpointMutex_);
  return this->host_;
}

int IgtlReceiver::port() const
{
  std::lock_guard<std::mutex> lock(this->endpointMutex_);
  return this->port_;
}

void IgtlReceiver::setSceneModel(nnc::SceneModel *sceneModel)
{
  this->sceneModel_ = sceneModel;
}

void IgtlReceiver::startReceiver()
{
  if (this->isRunning())
  {
    return;
  }
  this->stopRequested_.store(false);
  this->start();
}

void IgtlReceiver::stopReceiver()
{
  this->stopRequested_.store(true);
  this->closeSocket();
  if (!this->isRunning())
  {
    return;
  }
  if (!this->wait(5000))
  {
    this->terminate();
    this->wait();
  }
}

bool IgtlReceiver::snapshotToolToTracker(nnc::Mat4 *out) const
{
  return this->poseBuffer_.snapshot(out);
}

bool IgtlReceiver::hasToolPose() const
{
  return this->poseBuffer_.hasPose();
}

nnc::PoseTripleBuffer &IgtlReceiver::poseBuffer()
{
  return this->poseBuffer_;
}

const nnc::PoseTripleBuffer &IgtlReceiver::poseBuffer() const
{
  return this->poseBuffer_;
}

void IgtlReceiver::closeSocket()
{
  const int fd = this->socketFd_.exchange(-1);
  if (fd >= 0)
  {
    ::shutdown(fd, SHUT_RDWR);
    ::close(fd);
  }
}

void IgtlReceiver::resolveEndpoint(std::string *host, int *port) const
{
  std::string resolvedHost = nnc::igtl_receiver_detail::kDefaultHost;
  int resolvedPort = nnc::igtl_receiver_detail::kDefaultPort;
  bool explicitEndpoint = false;

  {
    std::lock_guard<std::mutex> lock(this->endpointMutex_);
    explicitEndpoint = this->endpointExplicit_;
    if (explicitEndpoint)
    {
      resolvedHost = this->host_;
      resolvedPort = this->port_;
    }
  }

  if (!explicitEndpoint)
  {
    const QString fileHost =
        nnc::igtl_receiver_detail::readEnvFileValue(QStringLiteral("NNC_NAVSIM_HOST"));
    if (!fileHost.isEmpty())
    {
      resolvedHost = fileHost.toStdString();
    }
    const QString filePort =
        nnc::igtl_receiver_detail::readEnvFileValue(QStringLiteral("NNC_NAVSIM_PORT"));
    if (!filePort.isEmpty())
    {
      const int p = filePort.toInt();
      if (p > 0 && p <= 65535)
      {
        resolvedPort = p;
      }
    }

    if (const char *envHost = std::getenv("NNC_NAVSIM_HOST"))
    {
      if (envHost[0] != '\0')
      {
        resolvedHost = envHost;
      }
    }
    if (const char *envPort = std::getenv("NNC_NAVSIM_PORT"))
    {
      if (envPort[0] != '\0')
      {
        const int p = std::atoi(envPort);
        if (p > 0 && p <= 65535)
        {
          resolvedPort = p;
        }
      }
    }
  }

  if (host)
  {
    *host = resolvedHost;
  }
  if (port)
  {
    *port = resolvedPort;
  }
}

int IgtlReceiver::connectToServer(const std::string &host, int port)
{
  while (!this->stopRequested_.load())
  {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo *result = nullptr;
    const std::string portText = std::to_string(port);
    const int ga = ::getaddrinfo(host.c_str(), portText.c_str(), &hints, &result);
    if (ga != 0)
    {
      qWarning().nospace() << "IgtlReceiver: getaddrinfo(" << host.c_str() << ":" << port
                           << ") failed: " << gai_strerror(ga);
      QThread::msleep(500);
      continue;
    }

    int connectedFd = -1;
    for (addrinfo *rp = result; rp != nullptr; rp = rp->ai_next)
    {
      if (this->stopRequested_.load())
      {
        break;
      }

      const int fd = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (fd < 0)
      {
        continue;
      }

      std::string nbErr;
      if (!nnc::igtl_receiver_detail::setNonBlocking(fd, &nbErr))
      {
        qWarning().noquote() << "IgtlReceiver:" << QString::fromStdString(nbErr);
        ::close(fd);
        continue;
      }

      const int cr = ::connect(fd, rp->ai_addr, rp->ai_addrlen);
      if (cr == 0)
      {
        connectedFd = fd;
        break;
      }
      if (errno != EINPROGRESS)
      {
        ::close(fd);
        continue;
      }

      // Wait until writable (connected/failed) or stop, in 200 ms slices.
      while (!this->stopRequested_.load())
      {
        pollfd pfd{};
        pfd.fd = fd;
        pfd.events = POLLOUT;
        const int pr = ::poll(&pfd, 1, 200);
        if (pr < 0)
        {
          if (errno == EINTR)
          {
            continue;
          }
          break;
        }
        if (pr == 0)
        {
          continue;
        }

        int soError = 0;
        socklen_t len = sizeof(soError);
        if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &soError, &len) < 0 || soError != 0)
        {
          break;
        }
        connectedFd = fd;
        break;
      }

      if (connectedFd == fd)
      {
        break;
      }
      ::close(fd);
    }

    ::freeaddrinfo(result);

    if (connectedFd >= 0)
    {
      this->socketFd_.store(connectedFd);
      return connectedFd;
    }

    if (!this->stopRequested_.load())
    {
      qWarning().nospace() << "IgtlReceiver: connect " << host.c_str() << ":" << port
                           << " failed; retrying…";
      QThread::msleep(500);
    }
  }

  return -1;
}

bool IgtlReceiver::readAndDispatch(int fd)
{
  std::uint8_t bytes[64 * 1024];
  const ssize_t received = ::recv(fd, bytes, sizeof(bytes), 0);
  if (received == 0)
  {
    return false;
  }
  if (received < 0)
  {
    if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
    {
      return true;
    }
    qWarning().nospace() << "IgtlReceiver: recv failed: " << std::strerror(errno);
    return false;
  }

  this->reassembler_.append(bytes, static_cast<std::size_t>(received));

  while (true)
  {
    std::vector<std::uint8_t> message;
    std::string framingError;
    if (!this->reassembler_.tryExtractMessage(message, &framingError))
    {
      if (!framingError.empty())
      {
        qWarning().noquote()
            << "IgtlReceiver: framing error:" << QString::fromStdString(framingError);
      }
      break;
    }

    nnc::IgtlHeader header;
    std::string parseError;
    if (!nnc::IgtlParser::parseHeader(
            message.data(), message.size(), header, &parseError))
    {
      qWarning().noquote()
          << "IgtlReceiver: header parse failed:" << QString::fromStdString(parseError);
      continue;
    }

    if (std::strcmp(header.type, "TRAJ") == 0)
    {
      nnc::Vec3 entry;
      nnc::Vec3 target;
      if (!nnc::IgtlParser::parseTrajectoryMessage(
              message.data(), message.size(), entry, target, &header, &parseError))
      {
        qWarning().noquote()
            << "IgtlReceiver: TRAJ parse failed:" << QString::fromStdString(parseError);
        continue;
      }
      if (this->sceneModel_ != nullptr)
      {
        this->sceneModel_->setPlan(entry, target);
      }
      qInfo().nospace() << "IgtlReceiver: stored TRAJ on SceneModel entry=(" << entry.x << ','
                        << entry.y << ',' << entry.z << ") target=(" << target.x << ','
                        << target.y << ',' << target.z << ')';
      continue;
    }

    if (std::strcmp(header.type, "TRANSFORM") == 0)
    {
      nnc::Mat4 toolToTracker;
      if (!nnc::IgtlParser::parseTransformMessage(
              message.data(), message.size(), toolToTracker, &header, &parseError))
      {
        qWarning().noquote()
            << "IgtlReceiver: TRANSFORM parse failed:" << QString::fromStdString(parseError);
        continue;
      }
      this->poseBuffer_.publish(toolToTracker);
      if (!this->loggedTransform_)
      {
        qInfo().nospace() << "IgtlReceiver: published TRANSFORM device=" << header.deviceName
                          << " tip=(" << toolToTracker(0, 3) << ',' << toolToTracker(1, 3) << ','
                          << toolToTracker(2, 3) << ')';
        this->loggedTransform_ = true;
      }
      continue;
    }

    qWarning().nospace() << "IgtlReceiver: ignoring unsupported type " << header.type;
  }

  return true;
}

void IgtlReceiver::run()
{
  std::string host;
  int port = 0;
  this->resolveEndpoint(&host, &port);
  this->reassembler_.clear();
  this->poseBuffer_.clear();
  this->loggedTransform_ = false;
  if (this->sceneModel_ != nullptr)
  {
    this->sceneModel_->clearPlan();
  }

  qInfo().nospace() << "IgtlReceiver: connecting to " << host.c_str() << ":" << port;

  const int fd = this->connectToServer(host, port);
  if (fd < 0)
  {
    qWarning() << "IgtlReceiver: stopped before connect";
    return;
  }

  qInfo().nospace() << "IgtlReceiver: connected to " << host.c_str() << ":" << port;

  while (!this->stopRequested_.load())
  {
    pollfd pfd{};
    pfd.fd = fd;
    pfd.events = POLLIN;
    const int pr = ::poll(&pfd, 1, 200);
    if (pr < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }
      qWarning().nospace() << "IgtlReceiver: poll failed: " << std::strerror(errno);
      break;
    }
    if (pr > 0 && (pfd.revents & POLLIN) != 0)
    {
      if (!this->readAndDispatch(fd))
      {
        qWarning() << "IgtlReceiver: peer closed or recv failed";
        break;
      }
    }
    if (pr > 0 && (pfd.revents & (POLLHUP | POLLERR | POLLNVAL)) != 0)
    {
      qWarning() << "IgtlReceiver: peer closed or socket error";
      break;
    }
  }

  this->closeSocket();
  qInfo() << "IgtlReceiver: disconnected";
}

} // namespace nnc
