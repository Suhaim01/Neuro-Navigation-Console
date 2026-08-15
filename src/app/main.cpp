#include "app/MainWindow.h"

#include <QApplication>
#include <QMessageBox>

int main(int argc, char* argv[])
{
  // Shaders live in the nnc_render static library; without this the linker drops
  // the generated resource object and ":/nnc/shaders/*" resolves to nothing.
  Q_INIT_RESOURCE(shaders);

  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("Neuro-Navigation-Console"));
  QApplication::setApplicationVersion(QStringLiteral(NNC_VERSION_FULL));

  MainWindow window;
  window.show();

  QMessageBox::warning(
      &window,
      QStringLiteral("Not a medical device"),
      QStringLiteral(
          "Research software. Not a medical device. Not for clinical use.\n\n"
          "This prototype is for demonstration and engineering review only. "
          "It must not be used to diagnose, treat, or guide care of any patient."));

  return app.exec();
}
