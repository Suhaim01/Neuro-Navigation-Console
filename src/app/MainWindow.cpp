#include "app/MainWindow.h"

#include "app/VolumeUploadSurface.h"

#include <QLabel>
#include <QSurfaceFormat>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
{
  this->setWindowTitle(QStringLiteral("Neuro-Navigation-Console"));
  this->resize(960, 640);

  QSurfaceFormat format;
  format.setDepthBufferSize(24);
  format.setVersion(3, 3);
  format.setProfile(QSurfaceFormat::CoreProfile);
  QSurfaceFormat::setDefaultFormat(format);

  QWidget* central = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(central);

  QLabel* title = new QLabel(QStringLiteral("Neuro-Navigation-Console"), central);
  title->setAlignment(Qt::AlignCenter);

  QLabel* statusLabel =
      new QLabel(QStringLiteral("Creating OpenGL context and uploading volume…"), central);
  statusLabel->setAlignment(Qt::AlignCenter);
  statusLabel->setWordWrap(true);

  nnc::VolumeUploadSurface* uploadSurface =
      new nnc::VolumeUploadSurface(statusLabel, central);

  layout->addStretch();
  layout->addWidget(title);
  layout->addWidget(statusLabel);
  layout->addWidget(uploadSurface, 0, Qt::AlignCenter);
  layout->addStretch();

  this->setCentralWidget(central);
}
