#pragma once

#include "io/IgtlParser.h"
#include "io/NiftiLoader.h"

#include <mutex>

namespace nnc
{

// Single source of truth for plan + live tool pose (minimal stub for Task 3).
// Plan/pose setters will be called from IgtlReceiver's worker thread; getters
// from the GUI/render path. Pose path will move to a triple buffer in 3e.
class SceneModel
{
public:
  bool hasPlan() const;
  nnc::Vec3 planEntry() const;
  nnc::Vec3 planTarget() const;
  void setPlan(const nnc::Vec3 &entry, const nnc::Vec3 &target);
  void clearPlan();

  bool hasToolPose() const;
  nnc::Mat4 toolToTracker() const;
  void setToolToTracker(const nnc::Mat4 &toolToTracker);
  void clearToolPose();

private:
  mutable std::mutex mutex_;
  bool hasPlan_ = false;
  nnc::Vec3 planEntry_{};
  nnc::Vec3 planTarget_{};
  bool hasToolPose_ = false;
  nnc::Mat4 toolToTracker_ = nnc::Mat4::identity();
};

} // namespace nnc
