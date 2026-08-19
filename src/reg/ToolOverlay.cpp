#include "reg/ToolOverlay.h"

#include "reg/ToolComposition.h"

#include <cmath>

namespace nnc
{
namespace tool_overlay_detail
{

bool mat4Finite(const nnc::Mat4 &m)
{
  for (float value : m.m)
  {
    if (!std::isfinite(value))
    {
      return false;
    }
  }
  return true;
}

bool normalizeVec3(const nnc::Vec3 &v, nnc::Vec3 *unitOut)
{
  if (unitOut == nullptr)
  {
    return false;
  }

  const float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
  if (!std::isfinite(lenSq) || lenSq <= 0.f)
  {
    return false;
  }

  const float invLen = 1.f / std::sqrt(lenSq);
  unitOut->x = v.x * invLen;
  unitOut->y = v.y * invLen;
  unitOut->z = v.z * invLen;
  return std::isfinite(unitOut->x) && std::isfinite(unitOut->y) && std::isfinite(unitOut->z);
}

nnc::Vec3 shaftDirectionFromToolInImage(const nnc::Mat4 &toolInImage)
{
  return nnc::Vec3{toolInImage(0, 2), toolInImage(1, 2), toolInImage(2, 2)};
}

bool vec3Finite(const nnc::Vec3 &v)
{
  return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

bool patientBoundsFinite(const nnc::PatientBounds &bounds)
{
  return nnc::tool_overlay_detail::vec3Finite(bounds.minMm) &&
         nnc::tool_overlay_detail::vec3Finite(bounds.maxMm);
}

} // namespace tool_overlay_detail

bool ToolOverlay::toolGeometryInImage(const nnc::Mat4 &toolInImage, nnc::ToolOverlayImage *out)
{
  if (out == nullptr)
  {
    return false;
  }
  if (!nnc::tool_overlay_detail::mat4Finite(toolInImage))
  {
    return false;
  }

  if (!nnc::ToolComposition::toolTipInImage(toolInImage, &out->tipMm))
  {
    return false;
  }

  const nnc::Vec3 shaftRaw = nnc::tool_overlay_detail::shaftDirectionFromToolInImage(toolInImage);
  if (!nnc::tool_overlay_detail::normalizeVec3(shaftRaw, &out->shaftDirMm))
  {
    return false;
  }

  out->shaftEndMm.x = out->tipMm.x + out->shaftDirMm.x * nnc::kShaftDisplayLengthMm;
  out->shaftEndMm.y = out->tipMm.y + out->shaftDirMm.y * nnc::kShaftDisplayLengthMm;
  out->shaftEndMm.z = out->tipMm.z + out->shaftDirMm.z * nnc::kShaftDisplayLengthMm;
  return true;
}

bool ToolOverlay::projectToolOverlaySlice(nnc::SliceOrientation orientation,
                                            const nnc::PatientBounds &patientBounds,
                                            const nnc::ToolOverlayImage &imageGeom,
                                            nnc::ToolOverlaySlice *out)
{
  if (out == nullptr)
  {
    return false;
  }
  if (!nnc::tool_overlay_detail::patientBoundsFinite(patientBounds))
  {
    return false;
  }
  if (!nnc::tool_overlay_detail::vec3Finite(imageGeom.tipMm) ||
      !nnc::tool_overlay_detail::vec3Finite(imageGeom.shaftEndMm))
  {
    return false;
  }

  out->tipUv = nnc::imageMmToSliceUv(orientation, patientBounds, imageGeom.tipMm);
  out->shaftUv = nnc::imageMmToSliceUv(orientation, patientBounds, imageGeom.shaftEndMm);
  out->visible = true;
  return true;
}

} // namespace nnc
