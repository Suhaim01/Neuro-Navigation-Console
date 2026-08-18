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
constexpr float kSvdEps = 1e-12f;

struct Mat3
{
  float m[9]{};

  static nnc::paired_point_detail::Mat3 identity()
  {
    nnc::paired_point_detail::Mat3 out{};
    out(0, 0) = 1.f;
    out(1, 1) = 1.f;
    out(2, 2) = 1.f;
    return out;
  }

  float operator()(int row, int col) const
  {
    return this->m[static_cast<std::size_t>(row * 3 + col)];
  }

  float &operator()(int row, int col)
  {
    return this->m[static_cast<std::size_t>(row * 3 + col)];
  }
};

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

nnc::Vec3 vec3Add(const nnc::Vec3 &a, const nnc::Vec3 &b)
{
  nnc::Vec3 out{};
  out.x = a.x + b.x;
  out.y = a.y + b.y;
  out.z = a.z + b.z;
  return out;
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

nnc::Vec3 vec3Scale(const nnc::Vec3 &v, float s)
{
  nnc::Vec3 out{};
  out.x = v.x * s;
  out.y = v.y * s;
  out.z = v.z * s;
  return out;
}

bool vec3Near(const nnc::Vec3 &a, const nnc::Vec3 &b, float eps)
{
  return nnc::paired_point_detail::vec3Norm(nnc::paired_point_detail::vec3Sub(a, b)) <= eps;
}

nnc::Vec3 mat3MulVec3(const nnc::paired_point_detail::Mat3 &m, const nnc::Vec3 &v)
{
  nnc::Vec3 out{};
  out.x = m(0, 0) * v.x + m(0, 1) * v.y + m(0, 2) * v.z;
  out.y = m(1, 0) * v.x + m(1, 1) * v.y + m(1, 2) * v.z;
  out.z = m(2, 0) * v.x + m(2, 1) * v.y + m(2, 2) * v.z;
  return out;
}

float mat3Det(const nnc::paired_point_detail::Mat3 &m)
{
  return m(0, 0) * (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1)) -
         m(0, 1) * (m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0)) +
         m(0, 2) * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0));
}

nnc::Vec3 computeCentroid(const std::vector<nnc::Vec3> &points)
{
  nnc::Vec3 sum{};
  for (const nnc::Vec3 &point : points)
  {
    sum = nnc::paired_point_detail::vec3Add(sum, point);
  }
  const float invN = 1.f / static_cast<float>(points.size());
  return nnc::paired_point_detail::vec3Scale(sum, invN);
}

// H = Σ_i a_i b_iᵀ (tracker ⊗ image), centered offsets.
nnc::paired_point_detail::Mat3 computeCrossCovariance(const std::vector<nnc::Vec3> &trackerCentered,
                                                       const std::vector<nnc::Vec3> &imageCentered)
{
  nnc::paired_point_detail::Mat3 H{};
  for (std::size_t i = 0; i < trackerCentered.size(); ++i)
  {
    const nnc::Vec3 &a = trackerCentered[i];
    const nnc::Vec3 &b = imageCentered[i];
    H(0, 0) += a.x * b.x;
    H(0, 1) += a.x * b.y;
    H(0, 2) += a.x * b.z;
    H(1, 0) += a.y * b.x;
    H(1, 1) += a.y * b.y;
    H(1, 2) += a.y * b.z;
    H(2, 0) += a.z * b.x;
    H(2, 1) += a.z * b.y;
    H(2, 2) += a.z * b.z;
  }
  return H;
}

struct SymMat4
{
  float m[4][4]{};

  float operator()(int row, int col) const
  {
    return this->m[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
  }

  float &operator()(int row, int col)
  {
    return this->m[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
  }
};

nnc::paired_point_detail::SymMat4 buildKabschMatrix(const nnc::paired_point_detail::Mat3 &c)
{
  const float sxx = c(0, 0);
  const float sxy = c(0, 1);
  const float sxz = c(0, 2);
  const float syx = c(1, 0);
  const float syy = c(1, 1);
  const float syz = c(1, 2);
  const float szx = c(2, 0);
  const float szy = c(2, 1);
  const float szz = c(2, 2);

  nnc::paired_point_detail::SymMat4 n{};
  n(0, 0) = sxx + syy + szz;
  n(0, 1) = syz - szy;
  n(0, 2) = szx - sxz;
  n(0, 3) = sxy - syx;
  n(1, 0) = syz - szy;
  n(1, 1) = sxx - syy - szz;
  n(1, 2) = sxy + syx;
  n(1, 3) = szx + sxz;
  n(2, 0) = szx - sxz;
  n(2, 1) = sxy + syx;
  n(2, 2) = -sxx + syy - szz;
  n(2, 3) = syz + szy;
  n(3, 0) = sxy - syx;
  n(3, 1) = szx + sxz;
  n(3, 2) = syz + szy;
  n(3, 3) = -sxx - syy + szz;
  return n;
}

void jacobiRotate4(nnc::paired_point_detail::SymMat4 &a,
                   nnc::paired_point_detail::SymMat4 &v,
                   int p,
                   int q)
{
  if (std::abs(a(p, q)) < nnc::paired_point_detail::kSvdEps)
  {
    return;
  }

  const float app = a(p, p);
  const float aqq = a(q, q);
  const float apq = a(p, q);
  const float phi = 0.5f * std::atan2(2.f * apq, aqq - app);
  const float c = std::cos(phi);
  const float s = std::sin(phi);

  for (int row = 0; row < 4; ++row)
  {
    if (row == p || row == q)
    {
      continue;
    }
    const float aRp = a(row, p);
    const float aRq = a(row, q);
    a(row, p) = c * aRp - s * aRq;
    a(p, row) = a(row, p);
    a(row, q) = s * aRp + c * aRq;
    a(q, row) = a(row, q);
  }

  const float newApp = c * c * app - 2.f * s * c * apq + s * s * aqq;
  const float newAqq = s * s * app + 2.f * s * c * apq + c * c * aqq;
  a(p, p) = newApp;
  a(q, q) = newAqq;
  a(p, q) = 0.f;
  a(q, p) = 0.f;

  for (int row = 0; row < 4; ++row)
  {
    const float vRp = v(row, p);
    const float vRq = v(row, q);
    v(row, p) = c * vRp - s * vRq;
    v(row, q) = s * vRp + c * vRq;
  }
}

// All eigenpairs of symmetric 4×4; returns index of largest eigenvalue in outLargestIndex.
bool symmetricEigen4(const nnc::paired_point_detail::SymMat4 &input,
                     nnc::paired_point_detail::SymMat4 *vectorsOut,
                     float eigenvaluesOut[4],
                     int *largestIndexOut,
                     std::string *error)
{
  if (vectorsOut == nullptr || eigenvaluesOut == nullptr || largestIndexOut == nullptr)
  {
    nnc::paired_point_detail::setError(error, "symmetricEigen4 output is null");
    return false;
  }

  nnc::paired_point_detail::SymMat4 a = input;
  nnc::paired_point_detail::SymMat4 v{};
  for (int row = 0; row < 4; ++row)
  {
    v(row, row) = 1.f;
  }

  for (int sweep = 0; sweep < 64; ++sweep)
  {
    int p = 0;
    int q = 1;
    float maxOff = std::abs(a(p, q));
    for (int i = 0; i < 4; ++i)
    {
      for (int j = i + 1; j < 4; ++j)
      {
        const float off = std::abs(a(i, j));
        if (off > maxOff)
        {
          maxOff = off;
          p = i;
          q = j;
        }
      }
    }
    if (maxOff < nnc::paired_point_detail::kSvdEps)
    {
      break;
    }
    nnc::paired_point_detail::jacobiRotate4(a, v, p, q);
  }

  for (int i = 0; i < 4; ++i)
  {
    eigenvaluesOut[i] = a(i, i);
  }

  int largestIndex = 0;
  for (int i = 1; i < 4; ++i)
  {
    if (eigenvaluesOut[i] > eigenvaluesOut[largestIndex])
    {
      largestIndex = i;
    }
  }

  *vectorsOut = v;
  *largestIndexOut = largestIndex;
  return true;
}

float vec4Norm(const float v[4])
{
  return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3]);
}

// Dominant eigenvector of symmetric 4×4 (Kabsch quaternion).
bool dominantEigenvector4(const nnc::paired_point_detail::SymMat4 &matrix, float out[4])
{
  nnc::paired_point_detail::SymMat4 vectors{};
  float eigenvalues[4]{};
  int largestIndex = 0;
  std::string error;
  if (!nnc::paired_point_detail::symmetricEigen4(
        matrix, &vectors, eigenvalues, &largestIndex, &error))
  {
    return false;
  }

  for (int row = 0; row < 4; ++row)
  {
    out[row] = vectors(row, largestIndex);
  }

  const float norm = nnc::paired_point_detail::vec4Norm(out);
  if (norm <= nnc::paired_point_detail::kSvdEps)
  {
    return false;
  }
  for (int i = 0; i < 4; ++i)
  {
    out[i] /= norm;
  }
  return true;
}

nnc::paired_point_detail::Mat3 rotationFromQuaternion(const float q[4])
{
  const float w = q[0];
  const float x = q[1];
  const float y = q[2];
  const float z = q[3];

  nnc::paired_point_detail::Mat3 r{};
  r(0, 0) = 1.f - 2.f * (y * y + z * z);
  r(0, 1) = 2.f * (x * y - w * z);
  r(0, 2) = 2.f * (x * z + w * y);
  r(1, 0) = 2.f * (x * y + w * z);
  r(1, 1) = 1.f - 2.f * (x * x + z * z);
  r(1, 2) = 2.f * (y * z - w * x);
  r(2, 0) = 2.f * (x * z - w * y);
  r(2, 1) = 2.f * (y * z + w * x);
  r(2, 2) = 1.f - 2.f * (x * x + y * y);
  return r;
}

// Kabsch rotation from cross-covariance H (tracker ⊗ image).
bool rotationFromCrossCovariance(const nnc::paired_point_detail::Mat3 &h,
                                 nnc::paired_point_detail::Mat3 *rotationOut,
                                 std::string *error)
{
  if (rotationOut == nullptr)
  {
    nnc::paired_point_detail::setError(error, "rotation output is null");
    return false;
  }

  const nnc::paired_point_detail::SymMat4 kabschMatrix =
    nnc::paired_point_detail::buildKabschMatrix(h);
  float quaternion[4]{};
  if (!nnc::paired_point_detail::dominantEigenvector4(kabschMatrix, quaternion))
  {
    nnc::paired_point_detail::setError(error, "failed to extract registration quaternion");
    return false;
  }

  nnc::paired_point_detail::Mat3 rotation =
    nnc::paired_point_detail::rotationFromQuaternion(quaternion);
  if (nnc::paired_point_detail::mat3Det(rotation) < 0.f)
  {
    quaternion[0] = -quaternion[0];
    quaternion[1] = -quaternion[1];
    quaternion[2] = -quaternion[2];
    quaternion[3] = -quaternion[3];
    rotation = nnc::paired_point_detail::rotationFromQuaternion(quaternion);
  }

  *rotationOut = rotation;
  return true;
}

nnc::Mat4 mat4FromRotationTranslation(const nnc::paired_point_detail::Mat3 &rotation,
                                      const nnc::Vec3 &translation)
{
  nnc::Mat4 out = nnc::Mat4::identity();
  for (int row = 0; row < 3; ++row)
  {
    for (int col = 0; col < 3; ++col)
    {
      out(row, col) = rotation(row, col);
    }
  }
  out(0, 3) = translation.x;
  out(1, 3) = translation.y;
  out(2, 3) = translation.z;
  return out;
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

  std::vector<nnc::Vec3> trackerPoints;
  std::vector<nnc::Vec3> imagePoints;
  trackerPoints.reserve(pairs.size());
  imagePoints.reserve(pairs.size());
  for (const nnc::FiducialPair &pair : pairs)
  {
    trackerPoints.push_back(pair.tracker);
    imagePoints.push_back(pair.image);
  }

  const nnc::Vec3 centroidTracker = nnc::paired_point_detail::computeCentroid(trackerPoints);
  const nnc::Vec3 centroidImage = nnc::paired_point_detail::computeCentroid(imagePoints);

  std::vector<nnc::Vec3> trackerCentered;
  std::vector<nnc::Vec3> imageCentered;
  trackerCentered.reserve(pairs.size());
  imageCentered.reserve(pairs.size());
  for (std::size_t i = 0; i < pairs.size(); ++i)
  {
    trackerCentered.push_back(
      nnc::paired_point_detail::vec3Sub(trackerPoints[i], centroidTracker));
    imageCentered.push_back(nnc::paired_point_detail::vec3Sub(imagePoints[i], centroidImage));
  }

  const nnc::paired_point_detail::Mat3 h =
    nnc::paired_point_detail::computeCrossCovariance(trackerCentered, imageCentered);

  nnc::paired_point_detail::Mat3 rotation{};
  if (!nnc::paired_point_detail::rotationFromCrossCovariance(h, &rotation, error))
  {
    return false;
  }

  const nnc::Vec3 rotatedCentroidTracker =
    nnc::paired_point_detail::mat3MulVec3(rotation, centroidTracker);
  const nnc::Vec3 translation =
    nnc::paired_point_detail::vec3Sub(centroidImage, rotatedCentroidTracker);

  out->trackerToImage =
    nnc::paired_point_detail::mat4FromRotationTranslation(rotation, translation);
  out->ok = true;
  return true;
}

} // namespace nnc
