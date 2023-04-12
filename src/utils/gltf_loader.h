#pragma once

#include "../pipeline/prefab.h"

SCN::Prefab* loadGLTF(const char* filename);
//GTR::Prefab* loadGLTF(const char* filename, cgltf_data* data, cgltf_options& options);
SCN::Prefab* loadGLTF(const std::vector<unsigned char>& data, const std::string& path);
