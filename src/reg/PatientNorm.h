#pragma once

#include "io/IgtlParser.h"
#include "reg/SliceOrientation.h"

#include <algorithm>
#include <cmath>

namespace nnc
{

struct PatientBounds
{
  nnc::Vec3 minMm{};
  nnc::Vec3 maxMm{};
};

struct SliceUv
{
  float u = 0.f;
  float v = 0.f;
};

inline float normalizePatientAxis(float valueMm, float minMm, float maxMm)
{
  const float span = maxMm - minMm;
  if (std::fabs(span) < 1e-6f)
  {
    return 0.5f;
  }
  return std::clamp((valueMm - minMm) / span, 0.f, 1.f);
}

inline nnc::Vec3 imageMmToFocusNorm(const nnc::Vec3 &tipImageMm, const nnc::PatientBounds &bounds)
{
  return nnc::Vec3{
    nnc::normalizePatientAxis(tipImageMm.x, bounds.minMm.x, bounds.maxMm.x),
    nnc::normalizePatientAxis(tipImageMm.y, bounds.minMm.y, bounds.maxMm.y),
    nnc::normalizePatientAxis(tipImageMm.z, bounds.minMm.z, bounds.maxMm.z)};
}

inline nnc::SliceUv imageMmToSliceUv(nnc::SliceOrientation orientation,
                                           const nnc::PatientBounds &bounds,
                                           const nnc::Vec3 &imageMm)
{
  if (orientation == nnc::SliceOrientation::Axial)
  {
    return nnc::SliceUv{
      nnc::normalizePatientAxis(imageMm.x, bounds.minMm.x, bounds.maxMm.x),
      nnc::normalizePatientAxis(imageMm.y, bounds.minMm.y, bounds.maxMm.y)};
  }
  if (orientation == nnc::SliceOrientation::Coronal)
  {
    return nnc::SliceUv{
      nnc::normalizePatientAxis(imageMm.x, bounds.minMm.x, bounds.maxMm.x),
      nnc::normalizePatientAxis(imageMm.z, bounds.minMm.z, bounds.maxMm.z)};
  }
  return nnc::SliceUv{
    nnc::normalizePatientAxis(imageMm.y, bounds.minMm.y, bounds.maxMm.y),
    nnc::normalizePatientAxis(imageMm.z, bounds.minMm.z, bounds.maxMm.z)};
}

} // namespace nnc
