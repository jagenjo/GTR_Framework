#pragma once

#define N_MAX_LEVELS 10
#define N_LEVELS 6
#define N_FACES 6

#include <string>
#include <map>

typedef unsigned char byte;

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

	float* data_f;
	float** faces_f;
	short* data_h;
	short** faces_h;

} sHDRELevel;

class HDRE {

private:

    std::string filename;
	float* data; // only f32 now

    float* pixels_f[N_MAX_LEVELS][N_FACES]; // Xpos, Xneg, Ypos, Yneg, Zpos, Zneg
    short* pixels_h[N_MAX_LEVELS][N_FACES]; // Xpos, Xneg, Ypos, Yneg, Zpos, Zneg
    byte* pixels_b[N_MAX_LEVELS][N_FACES]; // Xpos, Xneg, Ypos, Yneg, Zpos, Zneg

	bool clean();
	void init();

public:
	static std::map<std::string, HDRE*> s_loaded_hdres;

	sHDREHeader header;
	int width;
	int height;
    int levels = N_MAX_LEVELS;

	HDRE();
	HDRE(const char* filename);
	~HDRE();

	bool load(const char* filename);
	//bool load(void* data, int size);

	// useful methods
	float getMaxLuminance() { return this->header.maxLuminance; };
	float* getSHCoeffs()
	{
		if (this->header.numCoeffs > 0)
			return this->header.coeffs;
		return nullptr;
	}

	float* getData(); // All pixel data

	float* getFacef(int level, int face);	// Specific level and face
	float** getFacesf(int level = 0);		// [[]]: Array per face with all level data

    byte* getFaceb(int level, int face);	// Specific level and face
    byte** getFacesb(int level = 0);		// [[]]: Array per face with all level data

    short* getFaceh(int level, int face);	// Specific level and face
	short** getFacesh(int level = 0);		// [[]]: Array per face with all level data

	//sHDRELevel getLevel(int level = 0);

	static HDRE* Get(const char* filename);
};
