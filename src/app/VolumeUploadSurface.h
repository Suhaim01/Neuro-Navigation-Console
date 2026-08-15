#pragma once

#include "render/VolumeTexture.h"

#include <QOpenGLWidget>

class QLabel;

namespace nnc {

// Owns a GL context and uploads the configured NIfTI as a 3D texture (Task 7).
class VolumeUploadSurface : public QOpenGLWidget
{
public:
  explicit VolumeUploadSurface(QLabel* status, QWidget* parent = nullptr);
  ~VolumeUploadSurface() override;

protected:
  void initializeGL() override;
  void paintGL() override;

private:
  VolumeTexture texture_;
  QLabel* status_ = nullptr;
};

}  // namespace nnc
