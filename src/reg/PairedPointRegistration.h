#pragma once

#include "io/IgtlParser.h"
#include "io/NiftiLoader.h"

#include <string>
#include <vector>

namespace nnc
{

struct FiducialPair
{
  nnc::Vec3 tracker; // mm, tracker frame
  nnc::Vec3 image;   // mm, image world
};

struct RegistrationResult
{
  nnc::Mat4 trackerToImage = nnc::Mat4::identity();
  float freMm = 0.f;
  std::vector<float> residualMm;
  bool ok = false;
};

class PairedPointRegistration
{
public:
  // Returns false when input is invalid or the solver cannot run.
  // On success, out->ok is true and trackerToImage / FRE are filled (Tasks 4c–4f).
  static bool solve(const std::vector<nnc::FiducialPair> &pairs,
                    nnc::RegistrationResult *out,
                    std::string *error = nullptr);
};

} // namespace nnc
