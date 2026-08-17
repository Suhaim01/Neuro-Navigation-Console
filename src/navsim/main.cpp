#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

namespace nnc
{
namespace navsim_detail
{

struct Options
{
  int port = 18944;
  int rateHz = 60;
  bool showHelp = false;
};

void printUsage(const char *argv0)
{
  std::cerr
    << "Usage: " << argv0
    << " [--port N] [--rate Hz] [--help]\n"
    << "\n"
    << "  --port N   TCP listen port (default 18944)\n"
    << "  --rate Hz  Nominal pose stream rate (default 60; demo range ~30-60)\n"
    << "  --help     Show this help\n";
}

bool parseArgs(int argc, char **argv, Options &out, std::string *error)
{
  for (int i = 1; i < argc; ++i)
  {
    const char *arg = argv[i];
    if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0)
    {
      out.showHelp = true;
      return true;
    }
    if (std::strcmp(arg, "--port") == 0)
    {
      if (i + 1 >= argc)
      {
        if (error)
        {
          *error = "--port requires a value";
        }
        return false;
      }
      ++i;
      out.port = std::atoi(argv[i]);
      if (out.port <= 0 || out.port > 65535)
      {
        if (error)
        {
          *error = "--port must be in 1..65535";
        }
        return false;
      }
      continue;
    }
    if (std::strcmp(arg, "--rate") == 0)
    {
      if (i + 1 >= argc)
      {
        if (error)
        {
          *error = "--rate requires a value";
        }
        return false;
      }
      ++i;
      out.rateHz = std::atoi(argv[i]);
      if (out.rateHz <= 0)
      {
        if (error)
        {
          *error = "--rate must be a positive integer";
        }
        return false;
      }
      continue;
    }
    if (error)
    {
      *error = std::string("unknown argument: ") + arg;
    }
    return false;
  }
  return true;
}

} // namespace navsim_detail
} // namespace nnc

int main(int argc, char **argv)
{
  nnc::navsim_detail::Options opts;
  std::string err;
  if (!nnc::navsim_detail::parseArgs(argc, argv, opts, &err))
  {
    std::cerr << "navsim: " << err << '\n';
    nnc::navsim_detail::printUsage(argv[0]);
    return 1;
  }
  if (opts.showHelp)
  {
    nnc::navsim_detail::printUsage(argv[0]);
    return 0;
  }

  std::cout << "navsim: port=" << opts.port << " rate=" << opts.rateHz
            << " Hz (CLI skeleton; TCP listen lands in Task 2b)\n";
  return 0;
}
