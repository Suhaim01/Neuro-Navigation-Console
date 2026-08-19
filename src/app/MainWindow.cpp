#include "app/MainWindow.h"

#include "app/VolumeUploadSurface.h"
#include "app/RegistrationBootstrap.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QObject>
#include <QSurfaceFormat>
#include <QTimer>
#include <QVBoxLayout>
#include <QVector3D>
#include <QWidget>

#include <cstdlib>
#include <iostream>

namespace nnc
{
namespace detail
{

QWidget *makeOrientationPane(nnc::VolumeUploadSurface *surface,
                             const QString &title,
                             QWidget *parent)
{
  QWidget *pane = new QWidget(parent);
  QVBoxLayout *paneLayout = new QVBoxLayout(pane);
  paneLayout->setContentsMargins(0, 0, 0, 0);

  QLabel *heading = new QLabel(title, pane);
  heading->setAlignment(Qt::AlignCenter);

  surface->setParent(pane);
  paneLayout->addWidget(heading);
  paneLayout->addWidget(surface, 1);
  return pane;
}

} // namespace detail
} // namespace nnc

MainWindow::MainWindow(const std::string &igtlHost, int igtlPort, QWidget *parent)
    : QMainWindow(parent)
{
  if (!nnc::bootstrapRegistration(&this->sceneModel_))
  {
    std::exit(1);
  }

  this->setWindowTitle(QStringLiteral("Neuro-Navigation-Console"));
  this->resize(1200, 520);

  QSurfaceFormat format;
  format.setDepthBufferSize(24);
  format.setVersion(3, 3);
  format.setProfile(QSurfaceFormat::CoreProfile);
  QSurfaceFormat::setDefaultFormat(format);

  QWidget *central = new QWidget(this);
  QVBoxLayout *layout = new QVBoxLayout(central);

  QLabel *statusLabel =
    new QLabel(QStringLiteral("Creating OpenGL context and uploading volume…"), central);
  statusLabel->setAlignment(Qt::AlignCenter);
  statusLabel->setWordWrap(true);

  nnc::VolumeUploadSurface *axial =
    new nnc::VolumeUploadSurface(nnc::SliceOrientation::Axial);
  nnc::VolumeUploadSurface *coronal =
    new nnc::VolumeUploadSurface(nnc::SliceOrientation::Coronal);
  nnc::VolumeUploadSurface *sagittal =
    new nnc::VolumeUploadSurface(nnc::SliceOrientation::Sagittal);

  QObject::connect(axial, &nnc::VolumeUploadSurface::statusChanged, statusLabel, &QLabel::setText);
  QObject::connect(coronal, &nnc::VolumeUploadSurface::statusChanged, statusLabel, &QLabel::setText);
  QObject::connect(sagittal, &nnc::VolumeUploadSurface::statusChanged, statusLabel, &QLabel::setText);

  const auto syncFocus = [axial, coronal, sagittal](const QVector3D &focusNorm)
  {
    axial->setFocusNorm(focusNorm);
    coronal->setFocusNorm(focusNorm);
    sagittal->setFocusNorm(focusNorm);
  };
  QObject::connect(axial, &nnc::VolumeUploadSurface::focusChanged, central, syncFocus);
  QObject::connect(coronal, &nnc::VolumeUploadSurface::focusChanged, central, syncFocus);
  QObject::connect(sagittal, &nnc::VolumeUploadSurface::focusChanged, central, syncFocus);

  QWidget *mprRow = new QWidget(central);
  QHBoxLayout *mprLayout = new QHBoxLayout(mprRow);
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

  if (!igtlHost.empty() || igtlPort > 0)
  {
    this->igtlReceiver_.setEndpoint(igtlHost, igtlPort);
  }
  this->igtlReceiver_.setSceneModel(&this->sceneModel_);
  this->igtlReceiver_.startReceiver();

  QTimer *navTimer = new QTimer(this);
  navTimer->setInterval(33);
  QObject::connect(navTimer, &QTimer::timeout, this, [this, axial, coronal, sagittal, statusLabel]()
  {
    QString navStatus;
    const bool tracking =
      axial->applyNavigationFocus(&this->sceneModel_, &this->igtlReceiver_, &navStatus);
    if (tracking)
    {
      statusLabel->setText(navStatus);
    }
    if (tracking || this->igtlReceiver_.hasToolPose())
    {
      axial->update();
      coronal->update();
      sagittal->update();
    }
  });
  navTimer->start();
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

nnc::IgtlReceiver &MainWindow::igtlReceiver()
{
  return this->igtlReceiver_;
}

const nnc::IgtlReceiver &MainWindow::igtlReceiver() const
{
  return this->igtlReceiver_;
}
