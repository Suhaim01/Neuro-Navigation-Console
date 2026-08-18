#pragma once

#include "reg/PairedPointRegistration.h"

#include <string>
#include <vector>

namespace nnc
{

class FiducialLoader
{
public:
  // Parse whitespace-separated tracker/image mm pairs from a text file.
  static bool load(const std::string &path,
                   std::vector<nnc::FiducialPair> *out,
                   std::string *error = nullptr);

  // NNC_FIDUCIALS process env, then nnc.env, then default fixture path.
  static std::string resolvePath();
};

} // namespace nnc
