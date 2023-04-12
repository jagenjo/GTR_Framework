#pragma once

#include "../core/math.h"
#include "texture.h"

extern Vector3f cubemapFaceNormals[6][3]; //(x,y,z)

struct SphericalHarmonics {
	Vector3f coeffs[9];
};

SphericalHarmonics computeSH( FloatImage images[], bool degamma = false);
