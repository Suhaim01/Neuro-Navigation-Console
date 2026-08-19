#include "reg/ToolComposition.h"

#include <cmath>

namespace nnc
{
namespace tool_composition_detail
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

} // namespace tool_composition_detail

bool ToolComposition::composeToolInImage(const nnc::Mat4 &trackerToImage,
                                         const nnc::Mat4 &toolToTracker,
                                         nnc::Mat4 *toolInImage)
{
  if (toolInImage == nullptr)
  {
    return false;
  }
  if (!nnc::tool_composition_detail::mat4Finite(trackerToImage) ||
      !nnc::tool_composition_detail::mat4Finite(toolToTracker))
  {
    return false;
  }

  *toolInImage = trackerToImage * toolToTracker;
  return true;
}

bool ToolComposition::toolTipInImage(const nnc::Mat4 &toolInImage, nnc::Vec3 *tipImageMm)
{
  if (tipImageMm == nullptr)
  {
    return false;
  }
  if (!nnc::tool_composition_detail::mat4Finite(toolInImage))
  {
    return false;
  }

  float x = 0.f;
  float y = 0.f;
  float z = 0.f;
  toolInImage.transformPoint(0.f, 0.f, 0.f, x, y, z);
  if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
  {
    return false;
  }

  tipImageMm->x = x;
  tipImageMm->y = y;
  tipImageMm->z = z;
  return true;
}

bool ToolComposition::composeToolTipInImage(const nnc::Mat4 &trackerToImage,
                                            const nnc::Mat4 &toolToTracker,
                                            nnc::Mat4 *toolInImage,
                                            nnc::Vec3 *tipImageMm)
{
  nnc::Mat4 composed = nnc::Mat4::identity();
  if (!nnc::ToolComposition::composeToolInImage(trackerToImage, toolToTracker, &composed))
  {
    return false;
  }

  if (toolInImage != nullptr)
  {
    *toolInImage = composed;
  }

  if (tipImageMm == nullptr)
  {
    return true;
  }

  return nnc::ToolComposition::toolTipInImage(composed, tipImageMm);
}

} // namespace nnc
