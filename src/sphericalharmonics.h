#pragma once

#include "framework.h"
#include "texture.h"

extern Vector3 cubemapFaceNormals[6][3]; //(x,y,z)

//the struct that holds one probe coeffs
struct SphericalHarmonics {
	Vector3 coeffs[9];
};

SphericalHarmonics computeSH( FloatImage images[], bool degamma = false);
