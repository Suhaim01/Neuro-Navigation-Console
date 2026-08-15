#pragma once

#include "render/VolumeTexture.h"

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>

class QLabel;

namespace nnc {

// Visible GL surface: uploads volume (Task 7) and draws one mid-slice (Task 8 §1–3).
class VolumeUploadSurface : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
  explicit VolumeUploadSurface(QLabel* status, QWidget* parent = nullptr);
  ~VolumeUploadSurface() override;

protected:
  void initializeGL() override;
  void paintGL() override;
  void resizeGL(int w, int h) override;

private:
  bool buildSlicePipeline(QString* error);
  void destroyGlResources();

  VolumeTexture texture_;
  QLabel* status_ = nullptr;

  QOpenGLShaderProgram program_;
  QOpenGLVertexArrayObject vao_;
  QOpenGLBuffer vbo_{QOpenGLBuffer::VertexBuffer};
  bool pipelineReady_ = false;
};

}  // namespace nnc
