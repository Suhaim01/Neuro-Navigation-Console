#pragma once

#include "app/SceneModel.h"

#include <QMainWindow>

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);

  nnc::SceneModel &sceneModel();
  const nnc::SceneModel &sceneModel() const;

private:
  nnc::SceneModel sceneModel_;
};
