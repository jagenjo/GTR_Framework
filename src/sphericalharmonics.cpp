#include "sphericalharmonics.h"

//system axis
Vector3 cubemapFaceNormals[6][3] = {
    {{0, 0, -1} ,{0, -1, 0},{1, 0, 0} },  // posx
    {{0, 0, 1},{0, -1, 0},{-1, 0, 0} },  // negx

    {{1, 0, 0},{0, 0, 1},{0, 1, 0}},    // posy
    {{1, 0, 0},{0, 0, -1},{0, -1, 0}},  // negy

    {{1, 0, 0},{0, -1, 0},{0, 0, 1}},   // posz
    {{-1, 0, 0},{0, -1, 0},{0, 0, -1}}  // negz
};

const int sh_length = 9;
std::vector< std::vector<Vector3> > cubeMapVecs;
int cubeMapVecs_size = 0;

float areaElement(float x, float y) {
    return atan2(x * y, sqrtf(x * x + y * y + 1.0f));
}

float texelSolidAngle(float aU, float aV, float width, float height) {
    // transform from [0..res - 1] to [- (1 - 1 / res) .. (1 - 1 / res)]
    // ( 0.5 is for texel center addressing)
    float  U = (2.0 * (aU + 0.5) / width) - 1.0;
    float  V = (2.0 * (aV + 0.5) / height) - 1.0;

    // shift from a demi texel, mean 1.0 / size  with U and V in [-1..1]
    float  invResolutionW = 1.0 / width;
    float  invResolutionH = 1.0 / height;

    // U and V are the -1..1 texture coordinate on the current face.
    // get projected area for this texel
    float  x0 = U - invResolutionW;
    float  y0 = V - invResolutionH;
    float  x1 = U + invResolutionW;
    float  y1 = V + invResolutionH;
    float  angle = areaElement(x0, y0) - areaElement(x0, y1) - areaElement(x1, y0) + areaElement(x1, y1);
    return angle;
}

// give me a cubemap, its size and number of channels
// and i'll give you spherical harmonics
SphericalHarmonics computeSH( FloatImage images[], bool degamma ) {
    int size = images[0].width;
    int channels = 3;
    SphericalHarmonics sh;

    // generate cube map vectors
    if (cubeMapVecs_size != size)
    {
        cubeMapVecs_size = size;
        cubeMapVecs.clear();
        for (int index = 0; index < 6; ++index)
        {
            std::vector<Vector3> faceVecs;
            for (int v = 0; v < size; v++) {
                for (int u = 0; u < size; u++)
                {
                    float fU = (2.0 * u / (size - 1.0)) - 1.0;
                    float fV = (2.0 * v / (size - 1.0)) - 1.0;

                    Vector3 vecX = cubemapFaceNormals[index][0] * fU;
                    Vector3 vecY = cubemapFaceNormals[index][1] * fV;
                    Vector3 vecZ = cubemapFaceNormals[index][2];

                    Vector3 res = normalize(vecX + vecY + vecZ);
                    faceVecs.push_back(res);
                }
            }
            cubeMapVecs.push_back(faceVecs);
        }
    }

    // generate spherical harmonics
    float weightAccum = 0;

    for (int index = 0; index < 6; ++index)
    {
        FloatImage& face = images[index];
        float* pixels = face.data;
        bool gammaCorrect = degamma;
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                Vector3 texelVect = cubeMapVecs[index][y * size + x];
                float weight = texelSolidAngle(x, y, size, size);
                    // forsyths weights
                float weight1 = weight * 4 / 17;
                float weight2 = weight * 8 / 17;
                float weight3 = weight * 15 / 17;
                float weight4 = weight * 5 / 68;
                float weight5 = weight * 15 / 68;

                float dx = texelVect[0];
                float dy = texelVect[1];
                float dz = texelVect[2];

                Vector3 value = face.getPixel(x,y).xyz();
                if (gammaCorrect)
                    value = Vector3(pow(value.x, 2.2), pow(value.y, 2.2), pow(value.z, 2.2));
                    
                sh.coeffs[0] += value * weight1;
                sh.coeffs[1] += value * weight2 * dy;
                sh.coeffs[2] += value * weight2 * dz;
                sh.coeffs[3] += value * weight2 * dx;

                sh.coeffs[4] += value * weight3 * dx * dy;
                sh.coeffs[5] += value * weight3 * dy * dz;
                sh.coeffs[6] += value * weight4 * (3.0f * dz * dz - 1.0f);

                sh.coeffs[7] += value * weight3 * dx * dz;
                sh.coeffs[8] += value * weight5 * (dx * dx - dy * dy);

                weightAccum += weight * 3.0f;
            }
        }
    };

    SphericalHarmonics linear_sh;
    for (int i = 0; i < sh_length; i++)
        linear_sh.coeffs[i] = sh.coeffs[i] * (4 * PI / weightAccum);
    return linear_sh;
}
