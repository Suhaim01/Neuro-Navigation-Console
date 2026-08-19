#include "app/VolumeUploadSurface.h"

#include "app/SceneModel.h"
#include "io/IgtlReceiver.h"
#include "io/NiftiLoader.h"
#include "reg/ToolComposition.h"

#include <QCoreApplication>
#include <QFile>
#include <QMatrix4x4>
#include <QTextStream>
#include <QVector2D>

#include <algorithm>
#include <cmath>
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

namespace detail {

// QMatrix4x4(const float*) reads row-major, which is how Mat4 already stores.
QMatrix4x4 toQMatrix(const nnc::Mat4& M)
{
  return QMatrix4x4(M.m.data());
}

// Axis aligned bounding box
void patientAabb(const nnc::NiftiVolume& volume, QVector3D& outMin, QVector3D& outMax)
{
  const float iCorners[2] = {0.f, static_cast<float>(volume.nx - 1)};
  const float jCorners[2] = {0.f, static_cast<float>(volume.ny - 1)};
  const float kCorners[2] = {0.f, static_cast<float>(volume.nz - 1)};

  bool first = true;
  for (float i : iCorners) {
    for (float j : jCorners) {
      for (float k : kCorners) {
        float x = 0.f;
        float y = 0.f;
        float z = 0.f;
        volume.voxelToImage.transformPoint(i, j, k, x, y, z);
        if (first) {
          outMin = QVector3D(x, y, z);
          outMax = outMin;
          first = false;
        } else {
          outMin.setX(std::min(outMin.x(), x));
          outMin.setY(std::min(outMin.y(), y));
          outMin.setZ(std::min(outMin.z(), z));
          outMax.setX(std::max(outMax.x(), x));
          outMax.setY(std::max(outMax.y(), y));
          outMax.setZ(std::max(outMax.z(), z));
        }
      }
    }
  }
}

void intensityRange(const nnc::NiftiVolume& volume, float& outMin, float& outMax)
{
  if (volume.voxels.empty()) {
    outMin = 0.f;
    outMax = 1.f;
    return;
  }
  outMin = volume.voxels.front();
  outMax = volume.voxels.front();
  for (float v : volume.voxels) {
    outMin = std::min(outMin, v);
    outMax = std::max(outMax, v);
  }
  if (outMax <= outMin) {
    outMax = outMin + 1.f;
  }
}

float normalizePatientAxis(float valueMm, float minMm, float maxMm)
{
  const float span = maxMm - minMm;
  if (std::fabs(span) < 1e-6f) {
    return 0.5f;
  }
  return std::clamp((valueMm - minMm) / span, 0.f, 1.f);
}

QVector3D imageMmToFocusNorm(const nnc::Vec3& tipImageMm,
                             const QVector3D& patientMin,
                             const QVector3D& patientMax)
{
  return QVector3D(
      nnc::detail::normalizePatientAxis(tipImageMm.x, patientMin.x(), patientMax.x()),
      nnc::detail::normalizePatientAxis(tipImageMm.y, patientMin.y(), patientMax.y()),
      nnc::detail::normalizePatientAxis(tipImageMm.z, patientMin.z(), patientMax.z()));
}

}  // namespace detail

VolumeUploadSurface::VolumeUploadSurface(SliceOrientation orientation, QWidget* parent)
  : QOpenGLWidget(parent)
  , orientation_(orientation)
{
  this->setMinimumSize(280, 280);
  this->setMouseTracking(false);
}

VolumeUploadSurface::~VolumeUploadSurface()
{
  this->makeCurrent();
  this->destroyGlResources();
  this->texture_.destroy();
  this->doneCurrent();
}

void VolumeUploadSurface::setFocusNorm(const QVector3D& focusNorm)
{
  const QVector3D clamped(
      std::clamp(focusNorm.x(), 0.f, 1.f),
      std::clamp(focusNorm.y(), 0.f, 1.f),
      std::clamp(focusNorm.z(), 0.f, 1.f));
  if (this->focusNorm_ == clamped) {
    return;
  }
  this->focusNorm_ = clamped;
  this->update();
}

bool VolumeUploadSurface::applyNavigationFocus(const nnc::SceneModel* sceneModel,
                                               const nnc::IgtlReceiver* igtlReceiver,
                                               QString* statusOut)
{
  if (!this->pipelineReady_ || sceneModel == nullptr || igtlReceiver == nullptr) {
    return false;
  }
  if (!sceneModel->hasRegistration() || !igtlReceiver->hasToolPose()) {
    return false;
  }

  nnc::Mat4 toolToTracker = nnc::Mat4::identity();
  if (!igtlReceiver->snapshotToolToTracker(&toolToTracker)) {
    return false;
  }

  nnc::Mat4 toolInImage = nnc::Mat4::identity();
  nnc::Vec3 tipImageMm{};
  if (!nnc::ToolComposition::composeToolTipInImage(
          sceneModel->trackerToImage(), toolToTracker, &toolInImage, &tipImageMm)) {
    return false;
  }

  const QVector3D focusNorm =
      nnc::detail::imageMmToFocusNorm(tipImageMm, this->patientMin_, this->patientMax_);
  const QVector3D clamped(
      std::clamp(focusNorm.x(), 0.f, 1.f),
      std::clamp(focusNorm.y(), 0.f, 1.f),
      std::clamp(focusNorm.z(), 0.f, 1.f));
  if (this->focusNorm_ != clamped)
  {
    emit this->focusChanged(clamped);
  }

  if (statusOut != nullptr) {
    float vx = 0.f;
    float vy = 0.f;
    float vz = 0.f;
    this->voxelFromImage_.transformPoint(
        tipImageMm.x, tipImageMm.y, tipImageMm.z, vx, vy, vz);
    *statusOut =
        this->volumeStatusText_ +
        QStringLiteral("\n\nLive navigation (Task 5):\n"
                       "  tool tip image mm: (%1, %2, %3)\n"
                       "  tool tip voxel: (%4, %5, %6)\n"
                       "  focusNorm: (%7, %8, %9)\n"
                       "  registration FRE: %10 mm")
            .arg(static_cast<double>(tipImageMm.x), 0, 'f', 2)
            .arg(static_cast<double>(tipImageMm.y), 0, 'f', 2)
            .arg(static_cast<double>(tipImageMm.z), 0, 'f', 2)
            .arg(static_cast<double>(vx), 0, 'f', 2)
            .arg(static_cast<double>(vy), 0, 'f', 2)
            .arg(static_cast<double>(vz), 0, 'f', 2)
            .arg(static_cast<double>(clamped.x()), 0, 'f', 3)
            .arg(static_cast<double>(clamped.y()), 0, 'f', 3)
            .arg(static_cast<double>(clamped.z()), 0, 'f', 3)
            .arg(static_cast<double>(sceneModel->freMm()), 0, 'g', 4);
  }

  return true;
}

float VolumeUploadSurface::sliceNorm() const
{
  if (this->orientation_ == nnc::SliceOrientation::Axial) {
    return this->focusNorm_.z();
  }
  if (this->orientation_ == nnc::SliceOrientation::Coronal) {
    return this->focusNorm_.y();
  }
  return this->focusNorm_.x();
}

QVector2D VolumeUploadSurface::crossUv() const
{
  if (this->orientation_ == nnc::SliceOrientation::Axial) {
    return QVector2D(this->focusNorm_.x(), this->focusNorm_.y());
  }
  if (this->orientation_ == nnc::SliceOrientation::Coronal) {
    return QVector2D(this->focusNorm_.x(), this->focusNorm_.z());
  }
  return QVector2D(this->focusNorm_.y(), this->focusNorm_.z());
}

void VolumeUploadSurface::applyPointer(const QPoint& pos)
{
  if (!this->pipelineReady_ || this->width() <= 0 || this->height() <= 0) {
    return;
  }

  float screenU = static_cast<float>(pos.x()) / static_cast<float>(this->width());
  float screenV = 1.f - static_cast<float>(pos.y()) / static_cast<float>(this->height());
  screenU = std::clamp(screenU, 0.f, 1.f);
  screenV = std::clamp(screenV, 0.f, 1.f);

  // Inverse of shader: uvSample = pan + (screen - 0.5) / zoom
  const float u = std::clamp(this->viewCenterUv_.x() + (screenU - 0.5f) / this->zoom_, 0.f, 1.f);
  const float v = std::clamp(this->viewCenterUv_.y() + (screenV - 0.5f) / this->zoom_, 0.f, 1.f);

  QVector3D next = this->focusNorm_;
  if (this->orientation_ == nnc::SliceOrientation::Axial) {
    next.setX(u);
    next.setY(v);
  } else if (this->orientation_ == nnc::SliceOrientation::Coronal) {
    next.setX(u);
    next.setZ(v);
  } else {
    next.setY(u);
    next.setZ(v);
  }
  emit this->focusChanged(next);
}

void VolumeUploadSurface::applyPanDrag(const QPoint& pos)
{
  if (!this->pipelineReady_ || this->width() <= 0 || this->height() <= 0) {
    return;
  }

  const float dxScreen =
      static_cast<float>(pos.x() - this->lastViewCenterPos_.x()) /
      static_cast<float>(this->width());
  // Screen y is top-down; patient v is bottom-up.
  const float dyScreen =
      -static_cast<float>(pos.y() - this->lastViewCenterPos_.y()) /
      static_cast<float>(this->height());
  this->lastViewCenterPos_ = pos;

  // Grab-pan: content follows the cursor.
  this->viewCenterUv_.setX(std::clamp(this->viewCenterUv_.x() - dxScreen / this->zoom_, 0.f, 1.f));
  this->viewCenterUv_.setY(std::clamp(this->viewCenterUv_.y() - dyScreen / this->zoom_, 0.f, 1.f));
  this->update();
}

void VolumeUploadSurface::mousePressEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton) {
    this->applyPointer(event->pos());
  } else if (event->button() == Qt::MiddleButton) {
    this->panning_ = true;
    this->lastViewCenterPos_ = event->pos();
  }
  QOpenGLWidget::mousePressEvent(event);
}

void VolumeUploadSurface::mouseMoveEvent(QMouseEvent* event)
{
  if (event->buttons() & Qt::LeftButton) {
    this->applyPointer(event->pos());
  } else if (this->panning_ && (event->buttons() & Qt::MiddleButton)) {
    this->applyPanDrag(event->pos());
  }
  QOpenGLWidget::mouseMoveEvent(event);
}

void VolumeUploadSurface::mouseReleaseEvent(QMouseEvent* event)
{
  if (event->button() == Qt::MiddleButton) {
    this->panning_ = false;
  }
  QOpenGLWidget::mouseReleaseEvent(event);
}

void VolumeUploadSurface::wheelEvent(QWheelEvent* event)
{
  const float steps = static_cast<float>(event->angleDelta().y()) / 120.f;
  if (steps == 0.f) {
    QOpenGLWidget::wheelEvent(event);
    return;
  }

  const float factor = std::pow(1.1f, steps);
  this->zoom_ = std::clamp(this->zoom_ * factor, 0.25f, 32.f);
  this->update();
  event->accept();
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

void VolumeUploadSurface::uploadSliceUniforms()
{
  this->program_.setUniformValue("uVolume", 0);
  this->program_.setUniformValue("uVoxelFromImage", nnc::detail::toQMatrix(this->voxelFromImage_));
  this->program_.setUniformValue("uVolSize", this->volSize_);
  this->program_.setUniformValue("uPatientMin", this->patientMin_);
  this->program_.setUniformValue("uPatientMax", this->patientMax_);
  this->program_.setUniformValue("uOrientation", static_cast<int>(this->orientation_));
  this->program_.setUniformValue("uSlice", this->sliceNorm());
  this->program_.setUniformValue("uWindowLevel", this->windowLevel_);
  this->program_.setUniformValue("uWindowWidth", this->windowWidth_);
  this->program_.setUniformValue("uCrossUV", this->crossUv());
  this->program_.setUniformValue("uZoom", this->zoom_);
  this->program_.setUniformValue("uViewCenterUv", this->viewCenterUv_);
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
  this->uploadSliceUniforms();
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
    emit this->statusChanged(
        QStringLiteral(
            "NNC_VOLUME is not set.\n"
            "Set it in nnc.env or as a process environment variable."));
    return;
  }

  nnc::NiftiVolume volume;
  std::string err;
  if (!nnc::NiftiLoader::load(path.toStdString(), volume, &err)) {
    emit this->statusChanged(
        QStringLiteral("NIfTI load failed (%1):\n%2").arg(path, QString::fromStdString(err)));
    return;
  }

  if (!volume.voxelToImage.inverted(this->voxelFromImage_)) {
    emit this->statusChanged(QStringLiteral("voxelToImage is singular; cannot invert"));
    return;
  }
  
  this->volSize_ = QVector3D(static_cast<float>(volume.nx),
                               static_cast<float>(volume.ny),
                               static_cast<float>(volume.nz));
                               
  nnc::detail::patientAabb(volume, this->patientMin_, this->patientMax_);

  float intensityMin = 0.f;
  float intensityMax = 1.f;
  nnc::detail::intensityRange(volume, intensityMin, intensityMax);
  this->windowWidth_ = intensityMax - intensityMin;
  this->windowLevel_ = 0.5f * (intensityMin + intensityMax);

  QString glErr;
  if (!this->texture_.upload(volume, &glErr)) {
    emit this->statusChanged(QStringLiteral("GL 3D texture upload failed:\n%1").arg(glErr));
    return;
  }

  if (!this->buildSlicePipeline(&glErr)) {
    emit this->statusChanged(QStringLiteral("Slice shader pipeline failed:\n%1").arg(glErr));
    return;
  }

  this->volumeStatusText_ =
      QStringLiteral(
          "Task 8 §7c: middle-drag to pan; wheel zoom; left-drag crosshair.\n"
          "Size: %1 × %2 × %3\n"
          "WL: %4  WW: %5\n"
          "Texture id: %6\n"
          "Source: %7\n\n"
          "Research software. Not a medical device. Not for clinical use.")
          .arg(this->texture_.width())
          .arg(this->texture_.height())
          .arg(this->texture_.depth())
          .arg(this->windowLevel_)
          .arg(this->windowWidth_)
          .arg(this->texture_.textureId())
          .arg(path);
  emit this->statusChanged(this->volumeStatusText_);
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
  this->uploadSliceUniforms();
  // every openGL context has texture unit slots
  this->texture_.bind(0);
  this->vao_.bind();
  this->glDrawArrays(GL_TRIANGLES, 0, 6);
  this->vao_.release();
  this->program_.release();
}

}  // namespace nnc
