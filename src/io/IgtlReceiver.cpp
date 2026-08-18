#include "io/IgtlReceiver.h"

namespace nnc
{

IgtlReceiver::IgtlReceiver(QObject *parent)
  : QThread(parent)
{
}

IgtlReceiver::~IgtlReceiver()
{
  this->stopReceiver();
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
  if (!this->isRunning())
  {
    return;
  }
  if (!this->wait(5000))
  {
    // Idle loop should exit quickly; force-join only as a last resort.
    this->terminate();
    this->wait();
  }
}

void IgtlReceiver::run()
{
  // 3b: keep the thread alive until stopReceiver(). Socket I/O in 3c+.
  while (!this->stopRequested_.load())
  {
    QThread::msleep(20);
  }
}

} // namespace nnc
