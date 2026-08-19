#include "app/SceneModel.h"

namespace nnc
{

bool SceneModel::hasPlan() const
{
  std::lock_guard<std::mutex> lock(this->mutex_);
  return this->hasPlan_;
}

nnc::Vec3 SceneModel::planEntry() const
{
  std::lock_guard<std::mutex> lock(this->mutex_);
  return this->planEntry_;
}

nnc::Vec3 SceneModel::planTarget() const
{
  std::lock_guard<std::mutex> lock(this->mutex_);
  return this->planTarget_;
}

void SceneModel::setPlan(const nnc::Vec3 &entry, const nnc::Vec3 &target)
{
  std::lock_guard<std::mutex> lock(this->mutex_);
  this->planEntry_ = entry;
  this->planTarget_ = target;
  this->hasPlan_ = true;
}

void SceneModel::clearPlan()
{
  std::lock_guard<std::mutex> lock(this->mutex_);
  this->planEntry_ = nnc::Vec3{};
  this->planTarget_ = nnc::Vec3{};
  this->hasPlan_ = false;
}

bool SceneModel::hasToolPose() const
{
  std::lock_guard<std::mutex> lock(this->mutex_);
  return this->hasToolPose_;
}

nnc::Mat4 SceneModel::toolToTracker() const
{
  std::lock_guard<std::mutex> lock(this->mutex_);
  return this->toolToTracker_;
}

void SceneModel::setToolToTracker(const nnc::Mat4 &toolToTracker)
{
  std::lock_guard<std::mutex> lock(this->mutex_);
  this->toolToTracker_ = toolToTracker;
  this->hasToolPose_ = true;
}

void SceneModel::clearToolPose()
{
  std::lock_guard<std::mutex> lock(this->mutex_);
  this->toolToTracker_ = nnc::Mat4::identity();
  this->hasToolPose_ = false;
}

bool SceneModel::hasRegistration() const
{
  std::lock_guard<std::mutex> lock(this->mutex_);
  return this->hasRegistration_;
}

nnc::Mat4 SceneModel::trackerToImage() const
{
  std::lock_guard<std::mutex> lock(this->mutex_);
  return this->trackerToImage_;
}

float SceneModel::freMm() const
{
  std::lock_guard<std::mutex> lock(this->mutex_);
  return this->freMm_;
}

std::vector<float> SceneModel::residualMm() const
{
  std::lock_guard<std::mutex> lock(this->mutex_);
  return this->residualMm_;
}

void SceneModel::setRegistration(const nnc::RegistrationResult &result)
{
  std::lock_guard<std::mutex> lock(this->mutex_);
  if (!result.ok)
  {
    this->hasRegistration_ = false;
    this->trackerToImage_ = nnc::Mat4::identity();
    this->freMm_ = 0.f;
    this->residualMm_.clear();
    return;
  }

  this->hasRegistration_ = true;
  this->trackerToImage_ = result.trackerToImage;
  this->freMm_ = result.freMm;
  this->residualMm_ = result.residualMm;
}

void SceneModel::clearRegistration()
{
  std::lock_guard<std::mutex> lock(this->mutex_);
  this->hasRegistration_ = false;
  this->trackerToImage_ = nnc::Mat4::identity();
  this->freMm_ = 0.f;
  this->residualMm_.clear();
}

} // namespace nnc
