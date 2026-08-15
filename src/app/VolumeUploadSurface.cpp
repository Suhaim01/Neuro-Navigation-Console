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

// Process NNC_VOLUME, then committed nnc.env. Empty if neither is set.
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
  this->setMinimumSize(1, 1);
  this->setMaximumSize(1, 1);
}

VolumeUploadSurface::~VolumeUploadSurface()
{
  this->makeCurrent();
  this->texture_.destroy();
  this->doneCurrent();
}

void VolumeUploadSurface::initializeGL()
{
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

  if (this->status_ != nullptr) {
    this->status_->setText(
        QStringLiteral(
            "Volume uploaded as GL 3D texture (R32F sampler3D).\n"
            "Size: %1 × %2 × %3\n"
            "Texture id: %4\n"
            "Source: %5\n\n"
            "MPR views land in the next task.\n"
            "Research software. Not a medical device. Not for clinical use.")
            .arg(this->texture_.width())
            .arg(this->texture_.height())
            .arg(this->texture_.depth())
            .arg(this->texture_.textureId())
            .arg(path));
  }
}

void VolumeUploadSurface::paintGL()
{
  // Context keep-alive only; sampling comes with MPR.
}

}  // namespace nnc
