#include "app/MainWindow.h"

#include "app/VolumeUploadSurface.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QObject>
#include <QSurfaceFormat>
#include <QVBoxLayout>
#include <QVector3D>
#include <QWidget>

namespace nnc {
namespace detail {

QWidget* makeOrientationPane(nnc::VolumeUploadSurface* surface,
                              const QString& title,
                              QWidget* parent)
{
  QWidget* pane = new QWidget(parent);
  QVBoxLayout* paneLayout = new QVBoxLayout(pane);
  paneLayout->setContentsMargins(0, 0, 0, 0);

  QLabel* heading = new QLabel(title, pane);
  heading->setAlignment(Qt::AlignCenter);

  surface->setParent(pane);
  paneLayout->addWidget(heading);
  paneLayout->addWidget(surface, 1);
  return pane;
}

}  // namespace detail
}  // namespace nnc

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
{
  this->setWindowTitle(QStringLiteral("Neuro-Navigation-Console"));
  this->resize(1200, 520);

  QSurfaceFormat format;
  format.setDepthBufferSize(24);
  format.setVersion(3, 3);
  format.setProfile(QSurfaceFormat::CoreProfile);
  QSurfaceFormat::setDefaultFormat(format);

  QWidget* central = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(central);

  QLabel* statusLabel =
      new QLabel(QStringLiteral("Creating OpenGL context and uploading volume…"), central);
  statusLabel->setAlignment(Qt::AlignCenter);
  statusLabel->setWordWrap(true);

  nnc::VolumeUploadSurface* axial =
      new nnc::VolumeUploadSurface(nnc::SliceOrientation::Axial);
  nnc::VolumeUploadSurface* coronal =
      new nnc::VolumeUploadSurface(nnc::SliceOrientation::Coronal);
  nnc::VolumeUploadSurface* sagittal =
      new nnc::VolumeUploadSurface(nnc::SliceOrientation::Sagittal);

  QObject::connect(axial, &nnc::VolumeUploadSurface::statusChanged, statusLabel, &QLabel::setText);
  QObject::connect(coronal, &nnc::VolumeUploadSurface::statusChanged, statusLabel, &QLabel::setText);
  QObject::connect(sagittal, &nnc::VolumeUploadSurface::statusChanged, statusLabel, &QLabel::setText);

  const auto syncFocus = [axial, coronal, sagittal](const QVector3D& focusNorm) {
    axial->setFocusNorm(focusNorm);
    coronal->setFocusNorm(focusNorm);
    sagittal->setFocusNorm(focusNorm);
  };
  QObject::connect(axial, &nnc::VolumeUploadSurface::focusChanged, central, syncFocus);
  QObject::connect(coronal, &nnc::VolumeUploadSurface::focusChanged, central, syncFocus);
  QObject::connect(sagittal, &nnc::VolumeUploadSurface::focusChanged, central, syncFocus);

  QWidget* mprRow = new QWidget(central);
  QHBoxLayout* mprLayout = new QHBoxLayout(mprRow);
  mprLayout->setContentsMargins(0, 0, 0, 0);
  mprLayout->addWidget(
      nnc::detail::makeOrientationPane(axial, QStringLiteral("Axial"), mprRow), 1);
  mprLayout->addWidget(
      nnc::detail::makeOrientationPane(coronal, QStringLiteral("Coronal"), mprRow), 1);
  mprLayout->addWidget(
      nnc::detail::makeOrientationPane(sagittal, QStringLiteral("Sagittal"), mprRow), 1);

  layout->addWidget(statusLabel);
  layout->addWidget(mprRow, 1);

  this->setCentralWidget(central);

  this->igtlReceiver_.startReceiver();
}

MainWindow::~MainWindow()
{
  this->igtlReceiver_.stopReceiver();
}

nnc::SceneModel &MainWindow::sceneModel()
{
  return this->sceneModel_;
}

const nnc::SceneModel &MainWindow::sceneModel() const
{
  return this->sceneModel_;
}
