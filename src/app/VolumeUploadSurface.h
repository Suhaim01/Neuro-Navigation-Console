#pragma once

#include "io/NiftiLoader.h"
#include "render/VolumeTexture.h"

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QString>
#include <QVector3D>

namespace nnc {

// Patient-space MPR (Slicer): axial=fixed z, coronal=fixed y, sagittal=fixed x.
enum class SliceOrientation {
  Axial = 0,
  Coronal = 1,
  Sagittal = 2
};

// Visible GL surface: uploads volume and draws one mid-slice orientation.
class VolumeUploadSurface : public QOpenGLWidget, protected QOpenGLFunctions
{
  Q_OBJECT

public:
  explicit VolumeUploadSurface(SliceOrientation orientation, QWidget* parent = nullptr);
  ~VolumeUploadSurface() override;

signals:
  void statusChanged(const QString& text);

protected:
  void initializeGL() override;
  void paintGL() override;
  void resizeGL(int w, int h) override;

private:
  bool buildSlicePipeline(QString* error);
  void destroyGlResources();
  void uploadSliceUniforms();

  SliceOrientation orientation_;
  VolumeTexture texture_;

  Mat4 voxelFromImage_ = Mat4::identity();
  QVector3D volSize_;
  QVector3D patientMin_;
  QVector3D patientMax_;

  QOpenGLShaderProgram program_;
  QOpenGLVertexArrayObject vao_;
  QOpenGLBuffer vbo_{QOpenGLBuffer::VertexBuffer};
  bool pipelineReady_ = false;
};

}  // namespace nnc
