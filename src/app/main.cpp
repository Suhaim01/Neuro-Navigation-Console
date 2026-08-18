#include "app/MainWindow.h"

#include <QApplication>
#include <QMessageBox>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

namespace nnc
{
namespace console_main_detail
{

struct IgtlCli
{
  std::string host;
  int port = 0;
  bool showHelp = false;
  bool ok = true;
  std::string error;
};

void printUsage(const char *argv0)
{
  std::cerr << "Usage: " << argv0 << " [--igtl-host HOST] [--igtl-port N] [--help]\n"
            << "\n"
            << "  --igtl-host HOST  OpenIGTLink server host (default 127.0.0.1 / env)\n"
            << "  --igtl-port N     OpenIGTLink server port (default 18944 / env)\n"
            << "  --help            Show this help\n";
}

IgtlCli parseArgs(int argc, char **argv)
{
  IgtlCli out;
  for (int i = 1; i < argc; ++i)
  {
    const char *arg = argv[i];
    if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0)
    {
      out.showHelp = true;
      return out;
    }
    if (std::strcmp(arg, "--igtl-host") == 0)
    {
      if (i + 1 >= argc)
      {
        out.ok = false;
        out.error = "--igtl-host requires a value";
        return out;
      }
      ++i;
      out.host = argv[i];
      continue;
    }
    if (std::strcmp(arg, "--igtl-port") == 0)
    {
      if (i + 1 >= argc)
      {
        out.ok = false;
        out.error = "--igtl-port requires a value";
        return out;
      }
      ++i;
      out.port = std::atoi(argv[i]);
      if (out.port <= 0 || out.port > 65535)
      {
        out.ok = false;
        out.error = "--igtl-port must be in 1..65535";
        return out;
      }
      continue;
    }
    out.ok = false;
    out.error = std::string("unknown argument: ") + arg;
    return out;
  }
  return out;
}

} // namespace console_main_detail
} // namespace nnc

int main(int argc, char *argv[])
{
  // Shaders live in the nnc_render static library; without this the linker drops
  // the generated resource object and ":/nnc/shaders/*" resolves to nothing.
  Q_INIT_RESOURCE(shaders);

  const nnc::console_main_detail::IgtlCli cli = nnc::console_main_detail::parseArgs(argc, argv);
  if (!cli.ok)
  {
    std::cerr << "nnc_console: " << cli.error << '\n';
    nnc::console_main_detail::printUsage(argv[0]);
    return 1;
  }
  if (cli.showHelp)
  {
    nnc::console_main_detail::printUsage(argv[0]);
    return 0;
  }

  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("Neuro-Navigation-Console"));
  QApplication::setApplicationVersion(QStringLiteral(NNC_VERSION_FULL));

  MainWindow window(cli.host, cli.port);
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
