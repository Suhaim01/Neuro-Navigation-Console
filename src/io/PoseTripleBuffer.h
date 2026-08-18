#pragma once

#include "io/NiftiLoader.h"

#include <atomic>

namespace nnc
{

// Lock-free latest-wins pose handoff (REQ-IGTL-004).
// Writer (IgtlReceiver): publish(). Reader (render / SceneModel): snapshot().
class PoseTripleBuffer
{
public:
  void clear()
  {
    this->published_.store(-1, std::memory_order_relaxed);
    this->reading_.store(-1, std::memory_order_relaxed);
  }

  void publish(const nnc::Mat4 &pose)
  {
    const int published = this->published_.load(std::memory_order_relaxed);
    const int reading = this->reading_.load(std::memory_order_relaxed);
    int write = 0;
    for (; write < 3; ++write)
    {
      if (write != published && write != reading)
      {
        break;
      }
    }
    this->slots_[write] = pose;
    this->published_.store(write, std::memory_order_release);
  }

  // Copy the latest published pose. Returns false if nothing has been published yet.
  bool snapshot(nnc::Mat4 *out) const
  {
    if (out == nullptr)
    {
      return false;
    }
    const int index = this->published_.load(std::memory_order_acquire);
    if (index < 0)
    {
      return false;
    }
    this->reading_.store(index, std::memory_order_relaxed);
    *out = this->slots_[index];
    return true;
  }

  bool hasPose() const
  {
    return this->published_.load(std::memory_order_acquire) >= 0;
  }

private:
  nnc::Mat4 slots_[3]{};
  std::atomic<int> published_{-1};
  mutable std::atomic<int> reading_{-1};
};

} // namespace nnc
