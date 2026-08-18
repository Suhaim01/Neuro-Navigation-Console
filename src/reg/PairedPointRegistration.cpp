#include "reg/PairedPointRegistration.h"

#include <cmath>
#include <string>
#include <vector>

namespace nnc
{
namespace paired_point_detail
{

constexpr float kCoincidentEps = 1e-4f;
constexpr float kCollinearEps = 1e-4f;
constexpr std::size_t kMinPairs = 3;

void setError(std::string *error, const std::string &message)
{
  if (error != nullptr)
  {
    *error = message;
  }
}

bool vec3Finite(const nnc::Vec3 &v)
{
  return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

nnc::Vec3 vec3Sub(const nnc::Vec3 &a, const nnc::Vec3 &b)
{
  nnc::Vec3 out{};
  out.x = a.x - b.x;
  out.y = a.y - b.y;
  out.z = a.z - b.z;
  return out;
}

nnc::Vec3 vec3Cross(const nnc::Vec3 &a, const nnc::Vec3 &b)
{
  nnc::Vec3 out{};
  out.x = a.y * b.z - a.z * b.y;
  out.y = a.z * b.x - a.x * b.z;
  out.z = a.x * b.y - a.y * b.x;
  return out;
}

float vec3Norm(const nnc::Vec3 &v)
{
  return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

bool vec3Near(const nnc::Vec3 &a, const nnc::Vec3 &b, float eps)
{
  return vec3Norm(nnc::paired_point_detail::vec3Sub(a, b)) <= eps;
}

bool hasDuplicatePoints(const std::vector<nnc::Vec3> &points,
                        const char *label,
                        std::string *error)
{
  for (std::size_t i = 0; i < points.size(); ++i)
  {
    for (std::size_t j = i + 1; j < points.size(); ++j)
    {
      if (nnc::paired_point_detail::vec3Near(points[i], points[j], kCoincidentEps))
      {
        nnc::paired_point_detail::setError(
          error,
          std::string("coincident ") + label + " landmarks at indices " + std::to_string(i) +
            " and " + std::to_string(j));
        return true;
      }
    }
  }
  return false;
}

bool arePointsCollinear(const std::vector<nnc::Vec3> &points, const char *label, std::string *error)
{
  if (points.size() < 3)
  {
    return true;
  }

  nnc::Vec3 axis{};
  bool hasAxis = false;
  for (std::size_t i = 0; i < points.size() && !hasAxis; ++i)
  {
    for (std::size_t j = i + 1; j < points.size(); ++j)
    {
      const nnc::Vec3 edge = nnc::paired_point_detail::vec3Sub(points[j], points[i]);
      const float edgeLen = nnc::paired_point_detail::vec3Norm(edge);
      if (edgeLen > nnc::paired_point_detail::kCollinearEps)
      {
        axis.x = edge.x / edgeLen;
        axis.y = edge.y / edgeLen;
        axis.z = edge.z / edgeLen;
        hasAxis = true;
        break;
      }
    }
  }

  if (!hasAxis)
  {
    nnc::paired_point_detail::setError(error, std::string("degenerate ") + label +
                                                  " landmarks: all coincident");
    return true;
  }

  const nnc::Vec3 anchor = points[0];
  for (const nnc::Vec3 &point : points)
  {
    const nnc::Vec3 offset = nnc::paired_point_detail::vec3Sub(point, anchor);
    const nnc::Vec3 perp = nnc::paired_point_detail::vec3Cross(axis, offset);
    if (nnc::paired_point_detail::vec3Norm(perp) > nnc::paired_point_detail::kCollinearEps)
    {
      return false;
    }
  }

  nnc::paired_point_detail::setError(error, std::string("collinear ") + label + " landmarks");
  return true;
}

bool validatePairs(const std::vector<nnc::FiducialPair> &pairs, std::string *error)
{
  if (pairs.size() < nnc::paired_point_detail::kMinPairs)
  {
    nnc::paired_point_detail::setError(
      error,
      "at least " + std::to_string(nnc::paired_point_detail::kMinPairs) +
        " fiducial pairs required, got " + std::to_string(pairs.size()));
    return false;
  }

  std::vector<nnc::Vec3> trackerPoints;
  std::vector<nnc::Vec3> imagePoints;
  trackerPoints.reserve(pairs.size());
  imagePoints.reserve(pairs.size());

  for (std::size_t i = 0; i < pairs.size(); ++i)
  {
    const nnc::FiducialPair &pair = pairs[i];
    if (!nnc::paired_point_detail::vec3Finite(pair.tracker) ||
        !nnc::paired_point_detail::vec3Finite(pair.image))
    {
      nnc::paired_point_detail::setError(
        error, "non-finite coordinates at pair index " + std::to_string(i));
      return false;
    }
    trackerPoints.push_back(pair.tracker);
    imagePoints.push_back(pair.image);
  }

  if (nnc::paired_point_detail::hasDuplicatePoints(trackerPoints, "tracker", error))
  {
    return false;
  }
  if (nnc::paired_point_detail::hasDuplicatePoints(imagePoints, "image", error))
  {
    return false;
  }
  if (nnc::paired_point_detail::arePointsCollinear(trackerPoints, "tracker", error))
  {
    return false;
  }
  if (nnc::paired_point_detail::arePointsCollinear(imagePoints, "image", error))
  {
    return false;
  }

  return true;
}

} // namespace paired_point_detail

bool PairedPointRegistration::solve(const std::vector<nnc::FiducialPair> &pairs,
                                    nnc::RegistrationResult *out,
                                    std::string *error)
{
  if (out == nullptr)
  {
    nnc::paired_point_detail::setError(error, "output RegistrationResult is null");
    return false;
  }

  *out = nnc::RegistrationResult{};

  if (!nnc::paired_point_detail::validatePairs(pairs, error))
  {
    return false;
  }

  nnc::paired_point_detail::setError(error, "paired-point solver not implemented");
  return false;
}

} // namespace nnc
