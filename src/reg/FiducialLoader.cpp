#include "reg/FiducialLoader.h"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace nnc
{
namespace fiducial_loader_detail
{

constexpr const char *kDefaultFiducialsPath = "tests/fixtures/fiducials_navsim.txt";
constexpr const char *kEnvKey = "NNC_FIDUCIALS";

void setError(std::string *error, const std::string &message)
{
  if (error != nullptr)
  {
    *error = message;
  }
}

std::string trim(const std::string &text)
{
  std::size_t start = 0;
  while (start < text.size() &&
         (text[start] == ' ' || text[start] == '\t' || text[start] == '\r'))
  {
    ++start;
  }
  std::size_t end = text.size();
  while (end > start &&
         (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\r'))
  {
    --end;
  }
  return text.substr(start, end - start);
}

bool readEnvFileValue(const std::string &key, std::string *out)
{
  const char *candidates[] = {
    "nnc.env",
    "../nnc.env",
  };

  for (const char *candidate : candidates)
  {
    std::ifstream file(candidate);
    if (!file.is_open())
    {
      continue;
    }

    std::string line;
    while (std::getline(file, line))
    {
      const std::string trimmed = nnc::fiducial_loader_detail::trim(line);
      if (trimmed.empty() || trimmed[0] == '#')
      {
        continue;
      }
      const std::size_t eq = trimmed.find('=');
      if (eq == std::string::npos || eq == 0)
      {
        continue;
      }
      if (nnc::fiducial_loader_detail::trim(trimmed.substr(0, eq)) == key)
      {
        *out = nnc::fiducial_loader_detail::trim(trimmed.substr(eq + 1));
        return !out->empty();
      }
    }
  }

  return false;
}

bool parseFloatToken(const std::string &token, float *out)
{
  if (out == nullptr)
  {
    return false;
  }

  char *end = nullptr;
  const float value = std::strtof(token.c_str(), &end);
  if (end == token.c_str() || (end != nullptr && *end != '\0'))
  {
    return false;
  }
  if (!std::isfinite(value))
  {
    return false;
  }
  *out = value;
  return true;
}

bool parsePairLine(const std::string &line,
                   int lineNumber,
                   nnc::FiducialPair *out,
                   std::string *error)
{
  std::istringstream stream(line);
  std::vector<float> values;
  std::string token;
  while (stream >> token)
  {
    float value = 0.f;
    if (!nnc::fiducial_loader_detail::parseFloatToken(token, &value))
    {
      nnc::fiducial_loader_detail::setError(
        error,
        "invalid float on line " + std::to_string(lineNumber) + ": \"" + token + "\"");
      return false;
    }
    values.push_back(value);
  }

  if (values.size() != 6)
  {
    nnc::fiducial_loader_detail::setError(
      error,
      "expected 6 floats on line " + std::to_string(lineNumber) + ", got " +
        std::to_string(values.size()));
    return false;
  }

  out->tracker.x = values[0];
  out->tracker.y = values[1];
  out->tracker.z = values[2];
  out->image.x = values[3];
  out->image.y = values[4];
  out->image.z = values[5];
  return true;
}

} // namespace fiducial_loader_detail

bool FiducialLoader::load(const std::string &path,
                          std::vector<nnc::FiducialPair> *out,
                          std::string *error)
{
  if (out == nullptr)
  {
    nnc::fiducial_loader_detail::setError(error, "output vector is null");
    return false;
  }

  out->clear();

  if (path.empty())
  {
    nnc::fiducial_loader_detail::setError(error, "fiducial path is empty");
    return false;
  }

  std::ifstream file(path);
  if (!file.is_open())
  {
    nnc::fiducial_loader_detail::setError(error, "failed to open fiducial file: " + path);
    return false;
  }

  std::string line;
  int lineNumber = 0;
  while (std::getline(file, line))
  {
    ++lineNumber;
    const std::string trimmed = nnc::fiducial_loader_detail::trim(line);
    if (trimmed.empty() || trimmed[0] == '#')
    {
      continue;
    }

    nnc::FiducialPair pair{};
    if (!nnc::fiducial_loader_detail::parsePairLine(trimmed, lineNumber, &pair, error))
    {
      return false;
    }
    out->push_back(pair);
  }

  if (out->empty())
  {
    nnc::fiducial_loader_detail::setError(error, "fiducial file contains no pairs: " + path);
    return false;
  }

  return true;
}

std::string FiducialLoader::resolvePath()
{
  if (const char *fromProcess = std::getenv(nnc::fiducial_loader_detail::kEnvKey))
  {
    if (fromProcess[0] != '\0')
    {
      return std::string(fromProcess);
    }
  }

  std::string fromFile;
  if (nnc::fiducial_loader_detail::readEnvFileValue(nnc::fiducial_loader_detail::kEnvKey,
                                                    &fromFile))
  {
    return fromFile;
  }

  return std::string(nnc::fiducial_loader_detail::kDefaultFiducialsPath);
}

} // namespace nnc
