#pragma once

#include "io/IgtlParser.h"
#include "io/NiftiLoader.h"
#include "reg/PairedPointRegistration.h"

#include <mutex>
#include <vector>

namespace nnc
{

// Single source of truth for plan and registration. Live toolToTracker for the
// render path comes from IgtlReceiver's PoseTripleBuffer; not mirrored here
// (REQ-IGTL-004).
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

  bool hasRegistration() const;
  nnc::Mat4 trackerToImage() const;
  float freMm() const;
  std::vector<float> residualMm() const;
  void setRegistration(const nnc::RegistrationResult &result);
  void clearRegistration();

private:
  mutable std::mutex mutex_;
  bool hasPlan_ = false;
  nnc::Vec3 planEntry_{};
  nnc::Vec3 planTarget_{};
  bool hasToolPose_ = false;
  nnc::Mat4 toolToTracker_ = nnc::Mat4::identity();
  bool hasRegistration_ = false;
  nnc::Mat4 trackerToImage_ = nnc::Mat4::identity();
  float freMm_ = 0.f;
  std::vector<float> residualMm_;
};

} // namespace nnc
