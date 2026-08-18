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

} // namespace nnc
