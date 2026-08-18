#pragma once

#include "io/IgtlParser.h"
#include "io/NiftiLoader.h"

#include <mutex>

namespace nnc
{

// Single source of truth for plan (+ optional pose mirror). Live toolToTracker
// for the render path comes from IgtlReceiver's PoseTripleBuffer (3e); SceneModel
// pose setters remain for 3f plan handoff / optional mirror.
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
