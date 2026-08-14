#include "app/MainWindow.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
{
  this->setWindowTitle(QStringLiteral("Neuro-Navigation-Console"));
  this->resize(960, 640);

  auto* central = new QWidget(this);
  auto* layout = new QVBoxLayout(central);

  auto* title = new QLabel(QStringLiteral("Neuro-Navigation-Console"), central);
  title->setAlignment(Qt::AlignCenter);

  auto* stub = new QLabel(
      QStringLiteral(
          "Stub console — Day 1 skeleton.\n"
          "MPR views and tracking land in later tasks.\n\n"
          "Research software. Not a medical device. Not for clinical use."),
      central);
  stub->setAlignment(Qt::AlignCenter);
  stub->setWordWrap(true);

  layout->addStretch();
  layout->addWidget(title);
  layout->addWidget(stub);
  layout->addStretch();

  this->setCentralWidget(central);
}
