#pragma once

#include <QThread>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

namespace nnc
{

// OpenIGTLink client worker. Owns its QThread (this object *is* the thread).
// 3c: TCP connect to navsim; read/parse in 3d+.
class IgtlReceiver : public QThread
{
  Q_OBJECT

public:
  explicit IgtlReceiver(QObject *parent = nullptr);
  ~IgtlReceiver() override;

  // Override endpoint before startReceiver(). Empty host / port<=0 keeps prior values.
  void setEndpoint(const std::string &host, int port);
  std::string host() const;
  int port() const;

  // Clear stop flag and start the worker thread (no-op if already running).
  void startReceiver();
  // Request stop and join the worker thread.
  void stopReceiver();

protected:
  void run() override;

private:
  // Resolve host/port from setEndpoint, process env, nnc.env, then defaults.
  void resolveEndpoint(std::string *host, int *port) const;
  // Connect with retries until success or stopRequested_. Returns fd or -1.
  int connectToServer(const std::string &host, int port);
  void closeSocket();

  std::atomic<bool> stopRequested_{false};
  std::atomic<int> socketFd_{-1};

  mutable std::mutex endpointMutex_;
  std::string host_ = "127.0.0.1";
  int port_ = 18944;
  bool endpointExplicit_ = false;
};

} // namespace nnc
