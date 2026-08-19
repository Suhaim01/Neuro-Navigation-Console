#pragma once

#include "io/IgtlParser.h"
#include "reg/PatientNorm.h"
#include "reg/SliceOrientation.h"

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

struct ToolOverlaySlice
{
  nnc::SliceUv tipUv{};
  nnc::SliceUv shaftUv{};
  bool visible = false;
};

class ToolOverlay
{
public:
  // toolInImage → tip position, unit shaft direction, and fixed-length shaft endpoint.
  static bool toolGeometryInImage(const nnc::Mat4 &toolInImage, nnc::ToolOverlayImage *out);

  // Image mm geometry → patient-normalized in-plane UVs for one MPR orientation.
  static bool projectToolOverlaySlice(nnc::SliceOrientation orientation,
                                      const nnc::PatientBounds &patientBounds,
                                      const nnc::ToolOverlayImage &imageGeom,
                                      nnc::ToolOverlaySlice *out);
};

} // namespace nnc
