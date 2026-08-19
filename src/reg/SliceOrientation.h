#pragma once

namespace nnc
{

// Patient-space MPR: axial=fixed z, coronal=fixed y, sagittal=fixed x.
enum class SliceOrientation
{
  Axial = 0,
  Coronal = 1,
  Sagittal = 2
};

} // namespace nnc
