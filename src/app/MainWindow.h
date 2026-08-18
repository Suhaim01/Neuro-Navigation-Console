#pragma once

#include "app/SceneModel.h"
#include "io/IgtlReceiver.h"

#include <QMainWindow>

#include <string>

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(const std::string &igtlHost = {},
                      int igtlPort = 0,
                      QWidget *parent = nullptr);
  ~MainWindow() override;

  nnc::SceneModel &sceneModel();
  const nnc::SceneModel &sceneModel() const;

private:
  nnc::SceneModel sceneModel_;
  nnc::IgtlReceiver igtlReceiver_;
};
