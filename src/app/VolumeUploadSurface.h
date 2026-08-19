#pragma once

#include "io/NiftiLoader.h"
#include "reg/SliceOrientation.h"
#include "render/VolumeTexture.h"

#include <QMouseEvent>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QString>
#include <QVector2D>
#include <QVector3D>
#include <QWheelEvent>

namespace nnc {

class IgtlReceiver;
class SceneModel;

// Patient-space MPR (Slicer): axial=fixed z, coronal=fixed y, sagittal=fixed x.
// SliceOrientation lives in reg/SliceOrientation.h.

// Visible GL surface: uploads volume and draws one orientation with shared focus.
class VolumeUploadSurface : public QOpenGLWidget, protected QOpenGLFunctions
{
  Q_OBJECT

public:
  explicit VolumeUploadSurface(SliceOrientation orientation, QWidget* parent = nullptr);
  ~VolumeUploadSurface() override;

  // Shared focus in normalized patient AABB coords [0,1]^3 (x,y,z).
  void setFocusNorm(const QVector3D& focusNorm);

  // Snapshot pose, compose registration, map tool tip → focusNorm for MPR crosshair.
  // Returns true when navigation state was applied (registered + live pose).
  bool applyNavigationFocus(const nnc::SceneModel* sceneModel,
                            const nnc::IgtlReceiver* igtlReceiver,
                            QString* statusOut = nullptr);

  bool toolOverlayVisible() const { return this->toolOverlayVisible_; }
  QVector2D toolTipUv() const { return this->toolTipUv_; }
  QVector2D toolShaftUv() const { return this->toolShaftUv_; }

signals:
  void statusChanged(const QString& text);
  void focusChanged(const QVector3D& focusNorm);

protected:
  void initializeGL() override;
  void paintGL() override;
  void resizeGL(int w, int h) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

private:
  bool buildSlicePipeline(QString* error);
  void destroyGlResources();
  void uploadSliceUniforms();
  void applyPointer(const QPoint& pos);
  void applyPanDrag(const QPoint& pos);
  float sliceNorm() const;
  QVector2D crossUv() const;
  void clearLiveToolOverlay();
  void updateLiveToolOverlay(const nnc::Mat4& toolInImage);

  SliceOrientation orientation_;
  VolumeTexture texture_;

  Mat4 voxelFromImage_ = Mat4::identity();
  QVector3D volSize_;
  QVector3D patientMin_;
  QVector3D patientMax_;
  float windowLevel_ = 0.f;
  float windowWidth_ = 1.f;
  QVector3D focusNorm_{0.5f, 0.5f, 0.5f};
  float zoom_ = 1.f;
  // Patient-normalized UV at screen center (default mid-volume).
  QVector2D viewCenterUv_{0.5f, 0.5f};
  QPoint lastViewCenterPos_;
  bool panning_ = false;
  QString volumeStatusText_;
  bool toolOverlayVisible_ = false;
  QVector2D toolTipUv_{0.5f, 0.5f};
  QVector2D toolShaftUv_{0.5f, 0.5f};

  QOpenGLShaderProgram program_;
  QOpenGLVertexArrayObject vao_;
  QOpenGLBuffer vbo_{QOpenGLBuffer::VertexBuffer};
  bool pipelineReady_ = false;
};

}  // namespace nnc
