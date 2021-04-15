#pragma once

#include "prefab.h"

GTR::Prefab* loadGLTF(const char* filename);
//GTR::Prefab* loadGLTF(const char* filename, cgltf_data* data, cgltf_options& options);
GTR::Prefab* loadGLTF(const std::vector<unsigned char>& data, const std::string& path);
