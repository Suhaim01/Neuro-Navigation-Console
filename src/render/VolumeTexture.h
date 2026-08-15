#pragma once

#include "io/NiftiLoader.h"

#include <QString>

class QOpenGLTexture;

namespace nnc {

// GPU-resident volume as a single-channel 3D texture (sampler3D / R32F).
// Requires a current QOpenGLContext when upload()/destroy()/bind() are called.
class VolumeTexture {
public:
  VolumeTexture() = default;
  ~VolumeTexture();

  VolumeTexture(const VolumeTexture&) = delete;
  VolumeTexture& operator=(const VolumeTexture&) = delete;

  bool upload(const NiftiVolume& volume, QString* error = nullptr);
  void destroy();
  void bind(int textureUnit = 0) const;

  int width() const;
  int height() const;
  int depth() const;
  unsigned int textureId() const;

private:
  // A texture is a positional lookup of intensity
  QOpenGLTexture* texture_ = nullptr;
};

}  // namespace nnc
