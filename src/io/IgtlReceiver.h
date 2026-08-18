#pragma once

#include <QThread>

#include <atomic>

namespace nnc
{

// OpenIGTLink client worker. Owns its QThread (this object *is* the thread).
// 3b: start/stop idle loop only — socket + parse land in 3c/3d.
class IgtlReceiver : public QThread
{
  Q_OBJECT

public:
  explicit IgtlReceiver(QObject *parent = nullptr);
  ~IgtlReceiver() override;

  // Clear stop flag and start the worker thread (no-op if already running).
  void startReceiver();
  // Request stop and join the worker thread.
  void stopReceiver();

protected:
  void run() override;

private:
  std::atomic<bool> stopRequested_{false};
};

} // namespace nnc
