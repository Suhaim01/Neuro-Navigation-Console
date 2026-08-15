#include "app/MainWindow.h"

#include "app/VolumeUploadSurface.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QObject>
#include <QSurfaceFormat>
#include <QVBoxLayout>
#include <QWidget>

namespace nnc {
namespace detail {

QWidget* makeOrientationPane(nnc::SliceOrientation orientation,
                              const QString& title,
                              QLabel* statusLabel,
                              QWidget* parent)
{
  QWidget* pane = new QWidget(parent);
  QVBoxLayout* paneLayout = new QVBoxLayout(pane);
  paneLayout->setContentsMargins(0, 0, 0, 0);

  QLabel* heading = new QLabel(title, pane);
  heading->setAlignment(Qt::AlignCenter);

  nnc::VolumeUploadSurface* surface =
      new nnc::VolumeUploadSurface(orientation, pane);
      
  QObject::connect(
      surface,
      &nnc::VolumeUploadSurface::statusChanged,
      statusLabel,
      &QLabel::setText);

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

  QWidget* mprRow = new QWidget(central);
  QHBoxLayout* mprLayout = new QHBoxLayout(mprRow);
  mprLayout->setContentsMargins(0, 0, 0, 0);
  mprLayout->addWidget(
      nnc::detail::makeOrientationPane(
          nnc::SliceOrientation::Axial, QStringLiteral("Axial"), statusLabel, mprRow),
      1);
  mprLayout->addWidget(
      nnc::detail::makeOrientationPane(
          nnc::SliceOrientation::Coronal, QStringLiteral("Coronal"), statusLabel, mprRow),
      1);
  mprLayout->addWidget(
      nnc::detail::makeOrientationPane(
          nnc::SliceOrientation::Sagittal, QStringLiteral("Sagittal"), statusLabel, mprRow),
      1);

  layout->addWidget(statusLabel);
  layout->addWidget(mprRow, 1);

  this->setCentralWidget(central);
}
