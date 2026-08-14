#include "io/NiftiLoader.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace nnc {
namespace detail {

constexpr int kHeaderBytes = 348;

struct Nifti1Header {
  int32_t sizeof_hdr = 0;
  int16_t dim[8]{};
  int16_t datatype = 0;
  int16_t bitpix = 0;
  float pixdim[8]{};
  float vox_offset = 0.f;
  float scl_slope = 0.f;
  float scl_inter = 0.f;
  int16_t qform_code = 0;
  int16_t sform_code = 0;
  float quatern_b = 0.f;
  float quatern_c = 0.f;
  float quatern_d = 0.f;
  float qoffset_x = 0.f;
  float qoffset_y = 0.f;
  float qoffset_z = 0.f;
  float srow_x[4]{};
  float srow_y[4]{};
  float srow_z[4]{};
  char magic[4]{};
};

template <typename T>
T readLE(const uint8_t* p)
{
  T v{};
  std::memcpy(&v, p, sizeof(T));
  return v;
}

template <typename T>
T swapBytes(T v)
{
  auto* b = reinterpret_cast<uint8_t*>(&v);
  for (size_t i = 0; i < sizeof(T) / 2; ++i) {
    std::swap(b[i], b[sizeof(T) - 1 - i]);
  }
  return v;
}

// TODO : review
bool parseHeader(const uint8_t* raw, Nifti1Header& h, bool& littleEndian, std::string* error)
{
  int32_t sizeofHdr = detail::readLE<int32_t>(raw + 0);
  littleEndian = true;
  if (sizeofHdr != kHeaderBytes) {
    sizeofHdr = detail::swapBytes(sizeofHdr);
    littleEndian = false;
  }
  if (sizeofHdr != kHeaderBytes) {
    if (error) {
      *error = "invalid sizeof_hdr (not a NIfTI-1 header)";
    }
    return false;
  }

  auto rd16 = [&](int off) {
    auto v = detail::readLE<int16_t>(raw + off);
    return littleEndian ? v : detail::swapBytes(v);
  };
  auto rd32f = [&](int off) {
    auto v = detail::readLE<float>(raw + off);
    return littleEndian ? v : detail::swapBytes(v);
  };

  h.sizeof_hdr = sizeofHdr;
  for (int i = 0; i < 8; ++i) {
    h.dim[i] = rd16(40 + i * 2);
  }
  h.datatype = rd16(70);
  h.bitpix = rd16(72);
  for (int i = 0; i < 8; ++i) {
    h.pixdim[i] = rd32f(76 + i * 4);
  }
  h.vox_offset = rd32f(108);
  h.scl_slope = rd32f(112);
  h.scl_inter = rd32f(116);
  h.qform_code = rd16(252);
  h.sform_code = rd16(254);
  h.quatern_b = rd32f(256);
  h.quatern_c = rd32f(260);
  h.quatern_d = rd32f(264);
  h.qoffset_x = rd32f(268);
  h.qoffset_y = rd32f(272);
  h.qoffset_z = rd32f(276);
  for (int i = 0; i < 4; ++i) {
    h.srow_x[i] = rd32f(280 + i * 4);
    h.srow_y[i] = rd32f(296 + i * 4);
    h.srow_z[i] = rd32f(312 + i * 4);
  }
  std::memcpy(h.magic, raw + 344, 4);

  const bool magicOk = (std::memcmp(h.magic, "n+1", 3) == 0) || (std::memcmp(h.magic, "ni1", 3) == 0);
  if (!magicOk) {
    if (error) {
      *error = "bad NIfTI magic (expected n+1 or ni1)";
    }
    return false;
  }
  if (std::memcmp(h.magic, "ni1", 3) == 0) {
    if (error) {
      *error = "header-only .hdr/.img pairs are not supported; use a single-file .nii";
    }
    return false;
  }
  return true;
}

Mat4 matFromSform(const Nifti1Header& h)
{
  Mat4 M = Mat4::identity();
  for (int c = 0; c < 4; ++c) {
    M(0, c) = h.srow_x[c];
    M(1, c) = h.srow_y[c];
    M(2, c) = h.srow_z[c];
  }
  return M;
}

// NIfTI-1 quaternion → rotation (Method 2 in the NIfTI-1 spec).
Mat4 matFromQform(const Nifti1Header& h)
{
  float b = h.quatern_b;
  float c = h.quatern_c;
  float d = h.quatern_d;
  float a2 = 1.f - b * b - c * c - d * d;
  float a = (a2 > 0.f) ? std::sqrt(a2) : 0.f;

  float R[3][3];
  R[0][0] = a * a + b * b - c * c - d * d;
  R[0][1] = 2.f * b * c - 2.f * a * d;
  R[0][2] = 2.f * b * d + 2.f * a * c;
  R[1][0] = 2.f * b * c + 2.f * a * d;
  R[1][1] = a * a + c * c - b * b - d * d;
  R[1][2] = 2.f * c * d - 2.f * a * b;
  R[2][0] = 2.f * b * d - 2.f * a * c;
  R[2][1] = 2.f * c * d + 2.f * a * b;
  R[2][2] = a * a + d * d - b * b - c * c;

  float qfac = h.pixdim[0];
  if (qfac == 0.f) {
    qfac = 1.f;
  }
  const float dx = h.pixdim[1];
  const float dy = h.pixdim[2];
  const float dz = h.pixdim[3];

  Mat4 M = Mat4::identity();
  for (int r = 0; r < 3; ++r) {
    M(r, 0) = dx * R[r][0];
    M(r, 1) = dy * R[r][1];
    M(r, 2) = qfac * dz * R[r][2];
  }
  M(0, 3) = h.qoffset_x;
  M(1, 3) = h.qoffset_y;
  M(2, 3) = h.qoffset_z;
  return M;
}

// TODO : review
Mat4 voxelToImageFromHeader(const Nifti1Header& h)
{
  if (h.sform_code > 0) {
    return detail::matFromSform(h);
  }
  if (h.qform_code > 0) {
    return detail::matFromQform(h);
  }
  Mat4 M = Mat4::identity();
  M(0, 0) = (h.pixdim[1] != 0.f) ? h.pixdim[1] : 1.f;
  M(1, 1) = (h.pixdim[2] != 0.f) ? h.pixdim[2] : 1.f;
  M(2, 2) = (h.pixdim[3] != 0.f) ? h.pixdim[3] : 1.f;
  return M;
}

float scaleSample(float raw, float slope, float inter)
{
  if (slope == 0.f) {
    return raw;
  }
  return raw * slope + inter;
}

// TODO : review
bool loadVoxels(const uint8_t* data, size_t nbytes, bool littleEndian, const Nifti1Header& h,
                std::vector<float>& out, std::string* error)
{
  const int nx = h.dim[1];
  const int ny = h.dim[2];
  const int nz = h.dim[3] > 0 ? h.dim[3] : 1;
  if (nx <= 0 || ny <= 0 || nz <= 0) {
    if (error) {
      *error = "invalid volume dimensions";
    }
    return false;
  }
  const size_t count = static_cast<size_t>(nx) * static_cast<size_t>(ny) * static_cast<size_t>(nz);
  const int bytesPer = h.bitpix / 8;
  if (bytesPer <= 0 || nbytes < count * static_cast<size_t>(bytesPer)) {
    if (error) {
      *error = "voxel buffer shorter than dim product";
    }
    return false;
  }

  out.resize(count);
  const float slope = h.scl_slope;
  const float inter = h.scl_inter;

  switch (h.datatype) {
    case 2: {  // UINT8
      for (size_t i = 0; i < count; ++i) {
        out[i] = detail::scaleSample(static_cast<float>(data[i]), slope, inter);
      }
      break;
    }
    case 4: {  // INT16
      for (size_t i = 0; i < count; ++i) {
        auto v = detail::readLE<int16_t>(data + i * 2);
        if (!littleEndian) {
          v = detail::swapBytes(v);
        }
        out[i] = detail::scaleSample(static_cast<float>(v), slope, inter);
      }
      break;
    }
    case 8: {  // INT32
      for (size_t i = 0; i < count; ++i) {
        auto v = detail::readLE<int32_t>(data + i * 4);
        if (!littleEndian) {
          v = detail::swapBytes(v);
        }
        out[i] = detail::scaleSample(static_cast<float>(v), slope, inter);
      }
      break;
    }
    case 16: {  // FLOAT32
      for (size_t i = 0; i < count; ++i) {
        auto v = detail::readLE<float>(data + i * 4);
        if (!littleEndian) {
          v = detail::swapBytes(v);
        }
        out[i] = detail::scaleSample(v, slope, inter);
      }
      break;
    }
    default:
      if (error) {
        *error = "unsupported NIfTI datatype (need uint8/int16/int32/float32)";
      }
      return false;
  }
  return true;
}

}  // namespace detail

Mat4 Mat4::identity()
{
  Mat4 M;
  M.m = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  return M;
}

Mat4 Mat4::operator*(const Mat4& other) const
{
  Mat4 R;
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      float sum = 0.f;
      for (int k = 0; k < 4; ++k) {
        sum += (*this)(row, k) * other(k, col);
      }
      R(row, col) = sum;
    }
  }
  return R;
}

void Mat4::transformPoint(float i, float j, float k, float& x, float& y, float& z) const
{
  x = (*this)(0, 0) * i + (*this)(0, 1) * j + (*this)(0, 2) * k + (*this)(0, 3);
  y = (*this)(1, 0) * i + (*this)(1, 1) * j + (*this)(1, 2) * k + (*this)(1, 3);
  z = (*this)(2, 0) * i + (*this)(2, 1) * j + (*this)(2, 2) * k + (*this)(2, 3);
}

float NiftiVolume::voxel(int i, int j, int k) const
{
  return this->voxels[static_cast<size_t>(i + this->nx * (j + this->ny * k))];
}

bool NiftiLoader::load(const std::string& path, NiftiVolume& out, std::string* error)
{
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (error) {
      *error = "failed to open file: " + path;
    }
    return false;
  }
  in.seekg(0, std::ios::end);
  const auto fileSize = static_cast<size_t>(in.tellg());
  in.seekg(0, std::ios::beg);
  if (fileSize < static_cast<size_t>(detail::kHeaderBytes)) {
    if (error) {
      *error = "file too small for NIfTI-1 header";
    }
    return false;
  }

  std::vector<uint8_t> file(fileSize);
  in.read(reinterpret_cast<char*>(file.data()), static_cast<std::streamsize>(fileSize));
  if (!in) {
    if (error) {
      *error = "failed reading file";
    }
    return false;
  }

  detail::Nifti1Header header{};
  bool littleEndian = true;
  if (!detail::parseHeader(file.data(), header, littleEndian, error)) {
    return false;
  }

  size_t dataOff = static_cast<size_t>(header.vox_offset);
  if (dataOff < static_cast<size_t>(detail::kHeaderBytes) || dataOff > fileSize) {
    if (error) {
      *error = "invalid vox_offset";
    }
    return false;
  }

  out = NiftiVolume{};
  out.nx = header.dim[1];
  out.ny = header.dim[2];
  out.nz = header.dim[3] > 0 ? header.dim[3] : 1;
  out.sformCode = header.sform_code;
  out.qformCode = header.qform_code;
  out.voxelToImage = detail::voxelToImageFromHeader(header);

  if (!detail::loadVoxels(file.data() + dataOff, fileSize - dataOff, littleEndian, header, out.voxels,
                          error)) {
    return false;
  }
  return true;
}

}  // namespace nnc
