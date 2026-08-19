#pragma once

#include "app/SceneModel.h"

#include <string>

namespace nnc
{

// Load canned fiducials, solve trackerToImage + FRE, store on SceneModel.
bool bootstrapRegistration(nnc::SceneModel *sceneModel, std::string *error = nullptr);

} // namespace nnc
