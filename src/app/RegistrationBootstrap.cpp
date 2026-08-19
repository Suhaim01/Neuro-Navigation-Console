#include "app/RegistrationBootstrap.h"

#include "reg/FiducialLoader.h"
#include "reg/PairedPointRegistration.h"

#include <iostream>
#include <string>
#include <vector>

namespace nnc
{

bool bootstrapRegistration(nnc::SceneModel *sceneModel, std::string *error)
{
  if (sceneModel == nullptr)
  {
    if (error != nullptr)
    {
      *error = "null SceneModel";
    }
    return false;
  }

  const std::string path = nnc::FiducialLoader::resolvePath();
  std::vector<nnc::FiducialPair> pairs;
  std::string err;
  if (!nnc::FiducialLoader::load(path, &pairs, &err))
  {
    if (error != nullptr)
    {
      *error = "failed to load fiducials from " + path + ": " + err;
    }
    return false;
  }

  nnc::RegistrationResult result{};
  if (!nnc::PairedPointRegistration::solve(pairs, &result, &err))
  {
    if (error != nullptr)
    {
      *error = "registration solve failed: " + err;
    }
    return false;
  }
  if (!result.ok)
  {
    if (error != nullptr)
    {
      *error = "registration solve returned ok=false";
    }
    return false;
  }

  sceneModel->setRegistration(result);
  std::cerr << "nnc_console: registration ok path=" << path << " FRE=" << result.freMm
            << " mm landmarks=" << pairs.size() << '\n';
  return true;
}

} // namespace nnc
