#pragma once

#include "io/IgtlParser.h"
#include "io/NiftiLoader.h"

namespace nnc
{

class ToolComposition
{
public:
  // toolInImage = trackerToImage × toolToTracker (row-major, p' = M × p).
  static bool composeToolInImage(const nnc::Mat4 &trackerToImage,
                                 const nnc::Mat4 &toolToTracker,
                                 nnc::Mat4 *toolInImage);

  // Tool tip at origin in tool frame → image world millimetres.
  static bool toolTipInImage(const nnc::Mat4 &toolInImage, nnc::Vec3 *tipImageMm);

  // Convenience: full chain from registration + live tracker pose.
  static bool composeToolTipInImage(const nnc::Mat4 &trackerToImage,
                                    const nnc::Mat4 &toolToTracker,
                                    nnc::Mat4 *toolInImage,
                                    nnc::Vec3 *tipImageMm);
};

} // namespace nnc
