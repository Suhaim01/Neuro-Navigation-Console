#include "render/VolumeTexture.h"

#include <QOpenGLContext>
#include <QOpenGLTexture>

namespace nnc {

VolumeTexture::~VolumeTexture()
{
  this->destroy();
}

bool VolumeTexture::upload(const NiftiVolume& volume, QString* error)
{
  if (!QOpenGLContext::currentContext()) {
    if (error) {
      *error = QStringLiteral("VolumeTexture::upload requires a current OpenGL context");
    }
    return false;
  }
  if (volume.nx <= 0 || volume.ny <= 0 || volume.nz <= 0) {
    if (error) {
      *error = QStringLiteral("VolumeTexture::upload: invalid volume dimensions");
    }
    return false;
  }
  const size_t expected =
      static_cast<size_t>(volume.nx) * static_cast<size_t>(volume.ny) * static_cast<size_t>(volume.nz);
  if (volume.voxels.size() != expected) {
    if (error) {
      *error = QStringLiteral("VolumeTexture::upload: voxel buffer size mismatch");
    }
    return false;
  }

  this->destroy();

  this->texture_ = new QOpenGLTexture(QOpenGLTexture::Target3D);
  this->texture_->setFormat(QOpenGLTexture::R32F);
  this->texture_->setSize(volume.nx, volume.ny, volume.nz);
  this->texture_->setAutoMipMapGenerationEnabled(false);
  this->texture_->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::Float32);
  this->texture_->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, volume.voxels.data());
  this->texture_->setMinificationFilter(QOpenGLTexture::Linear);
  this->texture_->setMagnificationFilter(QOpenGLTexture::Linear);
  this->texture_->setWrapMode(QOpenGLTexture::ClampToEdge);

  if (!this->texture_->isCreated()) {
    this->destroy();
    if (error) {
      *error = QStringLiteral("VolumeTexture::upload: failed to create GL 3D texture");
    }
    return false;
  }

  return true;
}

void VolumeTexture::destroy()
{
  if (this->texture_ != nullptr) {
    delete this->texture_;
    this->texture_ = nullptr;
  }
}

int VolumeTexture::width() const
{
  return this->texture_ != nullptr ? this->texture_->width() : 0;
}

int VolumeTexture::height() const
{
  return this->texture_ != nullptr ? this->texture_->height() : 0;
}

int VolumeTexture::depth() const
{
  return this->texture_ != nullptr ? this->texture_->depth() : 0;
}

unsigned int VolumeTexture::textureId() const
{
  if (this->texture_ == nullptr) {
    return 0;
  }
  return this->texture_->textureId();
}

}  // namespace nnc
