#include "app/VolumeUploadSurface.h"

#include "io/NiftiLoader.h"

#include <QCoreApplication>
#include <QFile>
#include <QLabel>
#include <QTextStream>

#include <cstdlib>
#include <string>

namespace nnc {
namespace volume_path {

QString readEnvFileValue(const QString& key)
{
  const QStringList candidates = {
      QStringLiteral("nnc.env"),
      QStringLiteral("../nnc.env"),
      QCoreApplication::applicationDirPath() + QStringLiteral("/../nnc.env"),
  };

  for (const QString& path : candidates) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      continue;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
      QString line = in.readLine().trimmed();
      if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
        continue;
      }
      const int eq = line.indexOf(QLatin1Char('='));
      if (eq <= 0) {
        continue;
      }
      if (line.left(eq).trimmed() == key) {
        return line.mid(eq + 1).trimmed();
      }
    }
  }
  return {};
}

QString resolve()
{
  if (const char* fromProcess = std::getenv("NNC_VOLUME")) {
    if (fromProcess[0] != '\0') {
      return QString::fromLocal8Bit(fromProcess);
    }
  }

  return nnc::volume_path::readEnvFileValue(QStringLiteral("NNC_VOLUME"));
}

}  // namespace volume_path

VolumeUploadSurface::VolumeUploadSurface(QLabel* status, QWidget* parent)
  : QOpenGLWidget(parent)
  , status_(status)
{
  this->setMinimumSize(400, 400);
}

VolumeUploadSurface::~VolumeUploadSurface()
{
  this->makeCurrent();
  this->destroyGlResources();
  this->texture_.destroy();
  this->doneCurrent();
}

void VolumeUploadSurface::destroyGlResources()
{
  this->pipelineReady_ = false;
  if (this->vbo_.isCreated()) {
    this->vbo_.destroy();
  }
  if (this->vao_.isCreated()) {
    this->vao_.destroy();
  }
  this->program_.removeAllShaders();
}

bool VolumeUploadSurface::buildSlicePipeline(QString* error)
{
  if (!QFile::exists(QStringLiteral(":/nnc/shaders/slice.vert"))) {
    if (error) {
      *error = QStringLiteral("shader resources not registered (:/nnc/shaders missing)");
    }
    return false;
  }

  if (!this->program_.addShaderFromSourceFile(QOpenGLShader::Vertex,
                                              QStringLiteral(":/nnc/shaders/slice.vert"))) {
    if (error) {
      *error = QStringLiteral("vertex shader: %1").arg(this->program_.log());
    }
    return false;
  }
  if (!this->program_.addShaderFromSourceFile(QOpenGLShader::Fragment,
                                              QStringLiteral(":/nnc/shaders/slice.frag"))) {
    if (error) {
      *error = QStringLiteral("fragment shader: %1").arg(this->program_.log());
    }
    return false;
  }
  if (!this->program_.link()) {
    if (error) {
      *error = QStringLiteral("program link: %1").arg(this->program_.log());
    }
    return false;
  }

  // Fullscreen quad in NDC: position.xy + UV.xy
  const float verts[] = {
      // x, y, u, v
      -1.f, -1.f, 0.f, 0.f,
       1.f, -1.f, 1.f, 0.f,
       1.f,  1.f, 1.f, 1.f,
      -1.f, -1.f, 0.f, 0.f,
       1.f,  1.f, 1.f, 1.f,
      -1.f,  1.f, 0.f, 1.f,
  };

  this->vao_.create();
  this->vao_.bind();

  this->vbo_.create();
  this->vbo_.bind();
  this->vbo_.setUsagePattern(QOpenGLBuffer::StaticDraw);
  this->vbo_.allocate(verts, static_cast<int>(sizeof(verts)));

  this->program_.bind();
  this->program_.enableAttributeArray(0);
  this->program_.setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(float));
  this->program_.enableAttributeArray(1);
  this->program_.setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(float), 2, 4 * sizeof(float));
  this->program_.setUniformValue("uVolume", 0);
  this->program_.release();

  this->vbo_.release();
  this->vao_.release();

  this->pipelineReady_ = true;
  return true;
}

void VolumeUploadSurface::initializeGL()
{
  // this widget issues raw GL calls, so needed inheritance from QOpenGLFunctions
  this->initializeOpenGLFunctions();

  const QString path = nnc::volume_path::resolve();
  if (path.isEmpty()) {
    if (this->status_ != nullptr) {
      this->status_->setText(
          QStringLiteral(
              "NNC_VOLUME is not set.\n"
              "Set it in nnc.env or as a process environment variable."));
    }
    return;
  }

  nnc::NiftiVolume volume;
  std::string err;
  if (!nnc::NiftiLoader::load(path.toStdString(), volume, &err)) {
    if (this->status_ != nullptr) {
      this->status_->setText(
          QStringLiteral("NIfTI load failed (%1):\n%2").arg(path, QString::fromStdString(err)));
    }
    return;
  }

  QString glErr;
  if (!this->texture_.upload(volume, &glErr)) {
    if (this->status_ != nullptr) {
      this->status_->setText(QStringLiteral("GL 3D texture upload failed:\n%1").arg(glErr));
    }
    return;
  }

  if (!this->buildSlicePipeline(&glErr)) {
    if (this->status_ != nullptr) {
      this->status_->setText(QStringLiteral("Slice shader pipeline failed:\n%1").arg(glErr));
    }
    return;
  }

  if (this->status_ != nullptr) {
    this->status_->setText(
        QStringLiteral(
            "Task 8 §1–3: mid-slice sample of GL 3D texture.\n"
            "Size: %1 × %2 × %3\n"
            "Texture id: %4\n"
            "Source: %5\n\n"
            "Research software. Not a medical device. Not for clinical use.")
            .arg(this->texture_.width())
            .arg(this->texture_.height())
            .arg(this->texture_.depth())
            .arg(this->texture_.textureId())
            .arg(path));
  }
}

void VolumeUploadSurface::resizeGL(int w, int h)
{
  this->glViewport(0, 0, w, h);
}

// Called by Qt when resized for example
void VolumeUploadSurface::paintGL()
{
  this->glClearColor(0.05f, 0.05f, 0.07f, 1.f);
  this->glClear(GL_COLOR_BUFFER_BIT);

  if (!this->pipelineReady_) {
    return;
  }

  this->program_.bind();
  // every openGL context has texture unit slots
  this->texture_.bind(0);
  this->vao_.bind();
  this->glDrawArrays(GL_TRIANGLES, 0, 6);
  this->vao_.release();
  this->program_.release();
}

}  // namespace nnc
