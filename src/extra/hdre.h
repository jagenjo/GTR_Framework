#pragma once

#define N_LEVELS 6
#define N_FACES 6

typedef struct {

	char signature[4];
	float version;

	short width;
	short height;

	float maxFileSize;

	short numChannels;
	short bitsPerChannel;
	short headerSize;
	short endianEncoding;

	float maxLuminance;
	short type;

	short includesSH;
	float numCoeffs;
	float coeffs[27];

} sHDREHeader;

typedef struct {

	int width;
	int height;

	float* data;
	float** faces;

} sHDRELevel;

class HDRE {

private:

	float* data; // only f32 now
	float* pixels[N_LEVELS][N_FACES]; // Xpos, Xneg, Ypos, Yneg, Zpos, Zneg
	float* faces_array[N_LEVELS];

	bool clean();

public:

	sHDREHeader header;
	int width;
	int height;

	float version;

	HDRE();
	HDRE(const char* filename);
	~HDRE();

	bool load(const char* filename);

	// useful methods
	float getMaxLuminance() { return this->header.maxLuminance; };
	float* getSHCoeffs() { if (this->header.numCoeffs > 0) return this->header.coeffs; }

	float* getData(); // All pixel data
	float* getFace(int level, int face);	// Specific level and face
	float** getFaces(int level = 0);		// [[]]: Array per face with all level data

	sHDRELevel getLevel(int level = 0);
};
