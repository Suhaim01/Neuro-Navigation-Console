#pragma once

#include "io/IgtlParser.h"

namespace nnc
{

// Cosmetic shaft segment length for MPR overlay (does not model physical instrument).
constexpr float kShaftDisplayLengthMm = 60.f;

struct ToolOverlayImage
{
  nnc::Vec3 tipMm{};
  nnc::Vec3 shaftDirMm{}; // unit vector in image mm (tool +Z axis)
  nnc::Vec3 shaftEndMm{}; // tipMm + shaftDirMm * kShaftDisplayLengthMm
};

class ToolOverlay
{
public:
  // toolInImage → tip position, unit shaft direction, and fixed-length shaft endpoint.
  static bool toolGeometryInImage(const nnc::Mat4 &toolInImage, nnc::ToolOverlayImage *out);
};

} // namespace nnc
