#pragma once

#include <array>
#include <string>
#include <vector>

namespace nnc {

// Row-major 4x4 homogeneous transform. p' = M * p (column vector).
struct Mat4 {
  std::array<float, 16> m{};

  static Mat4 identity();
  float operator()(int row, int col) const { return this->m[static_cast<size_t>(row * 4 + col)]; }
  float& operator()(int row, int col) { return this->m[static_cast<size_t>(row * 4 + col)]; }

  Mat4 operator*(const Mat4& other) const;
  void transformPoint(float i, float j, float k, float& x, float& y, float& z) const;
};

struct NiftiVolume {
  int nx = 0;
  int ny = 0;
  int nz = 0;
  std::vector<float> voxels; // size nx*ny*nz, scaled by scl_slope/inter
  Mat4 voxelToImage = Mat4::identity();
  int sformCode = 0;
  int qformCode = 0;

  // Voxel index (i,j,k) → linear index (i + nx*(j + ny*k)), NIfTI fastest-x.
  float voxel(int i, int j, int k) const;
};

class NiftiLoader {
public:
  // Loads a NIfTI-1 .nii (single-file). Prefer sform when sform_code > 0, else qform.
  static bool load(const std::string& path, NiftiVolume& out, std::string* error = nullptr);
};

}  // namespace nnc
