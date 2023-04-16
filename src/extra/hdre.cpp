#include <iostream>
#include <fstream>
#include <cmath>
#include <cassert>
#include <algorithm>

#include "../utils/utils.h"
#include "hdre.h"

std::map<std::string, HDRE*> HDRE::s_loaded_hdres;

HDRE::HDRE()
{
    init();
}

HDRE::HDRE(const char* filename)
{
    init();

	load(filename);
}

void HDRE::init()
{
    data = nullptr;
    width = height = 0;
    levels = N_MAX_LEVELS;

    for (int j = 0; j < N_FACES; j++)
    {
        for (int i = 0; i < N_MAX_LEVELS; i++)
        {
            pixels_h[i][j] = nullptr;
            pixels_f[i][j] = nullptr;
            pixels_b[i][j] = nullptr;
        }
    }
}

HDRE::~HDRE()
{
	clean();

	auto it = s_loaded_hdres.find(filename);
	if (it != s_loaded_hdres.end())
	    s_loaded_hdres.erase(it);
}

/*sHDRELevel HDRE::getLevel(int n)
{
	sHDRELevel level;

	//float size = std::max(8, (int)(this->width / pow(2.0, n)));
	float size = fmax(8, (int)(this->width / pow(2.0, n)));

	if (this->version > 2.0)
		size = (int)(this->width / pow(2.0, n));

	level.width = size;
	level.height = size; // cubemap sizes!
	level.data_f = this->faces_array_f[n];
	level.faces_f = this->getFacesf(n);

	level.data_h = this->faces_array_h[n];
	level.faces_h = this->getFacesh(n);

	return level;
}*/

float* HDRE::getData()
{
	return this->data;
}

float** HDRE::getFacesf(int level)
{
    return this->pixels_f[level];
}
float* HDRE::getFacef(int level, int face)
{
    return this->pixels_f[level][face];
}

byte** HDRE::getFacesb(int level)
{
	return this->pixels_b[level];
}
byte* HDRE::getFaceb(int level, int face)
{
    return this->pixels_b[level][face];
}

short** HDRE::getFacesh(int level)
{
    return this->pixels_h[level];
}
short* HDRE::getFaceh(int level, int face)
{
    return this->pixels_h[level][face];
}


bool HDRE::load(const char* filename)
{
	assert(filename);

	FILE *f = fopen(filename,"rb");
	if (f == nullptr)
		return false;

	sHDREHeader HDREHeader;

	fread(&HDREHeader, sizeof(sHDREHeader), 1, f);

	if (HDREHeader.type != 3) {
        std::cout << "HDRE Header has wrong type: " << HDREHeader.type << std::endl;
        throw ("ArrayType not supported. Please export in Float32Array.");
    }

    this->header = HDREHeader;

	int width = HDREHeader.width;
	int height = HDREHeader.height;

	this->width = width;
	this->height = height;

	int dataSize = 0;
	int w = width;

	// Get number of floats inside the HDRE
	// Per channel & Per face
	for (int i = 0; i < N_LEVELS; i++)
	{
		int mip_level = i + 1;
		dataSize += w * w * N_FACES * HDREHeader.numChannels;

		//w = std::max(8, (int)(width / pow(2.0, mip_level)));
		w = fmax(8, (int)(width / pow(2.0, mip_level)));

		if (this->header.version > 2.0)
			w = (int)(width / pow(2.0, mip_level));
	}

	this->data = new float[dataSize];

	fseek(f, HDREHeader.headerSize, SEEK_SET);

	fread(this->data, sizeof(float) * dataSize, 1, f);
	fclose(f);

	// get separated levels

	w = width;
	int mapOffset = 0;
    int mapPrevOffset = 0;

	int nFullMips = 0;
	while (w)
    {
	    nFullMips++;
	    w >>= 1;
    }
	assert(nFullMips <= N_MAX_LEVELS);
	levels = nFullMips;
	w = width;

    int i;
    for (i = 0; i < N_LEVELS; i++)
	{
		int mip_level = i + 1;
		int faceSize = w * w * HDREHeader.numChannels;
		int mapSize = faceSize * N_FACES;

		int faceOffset = 0;

		for (int j = 0; j < N_FACES; j++)
		{
			// allocate memory
			this->pixels_f[i][j] = new float[faceSize];

			// set data
			for (int k = 0; k < faceSize; k++) {

				float value = this->data[mapOffset + faceOffset + k];

				this->pixels_f[i][j][k] = value;
			}

			// update face offset
			faceOffset += faceSize;
		}

		// update level offset
		mapPrevOffset = mapOffset;
		mapOffset += mapSize;
		// reassign width for next level
		//w = std::max(8, (int)(width / pow(2.0, mip_level)));
		w = fmax(8, (int)(width / pow(2.0, mip_level)));

		if (this->header.version > 2.0)
			w = (int)(width / pow(2.0, mip_level));
	}
	std::cout << " + '" << filename << "' (v" << this->header.version << ") loaded successfully" << std::endl;
	return true;
}

bool HDRE::clean()
{
	try
	{
		if (data)
			delete data;

        for (int j = 0; j < N_FACES; j++)
        {
            for (int i = 0; i < N_LEVELS; i++)
            {
				delete pixels_h[i][j];
				pixels_h[i][j] = nullptr;
			}
            for (int i = 0; i < N_LEVELS; i++)
            {
                delete pixels_f[i][j];
                pixels_f[i][j] = nullptr;
            }
		}

		return true;
	}
	catch (const std::exception&)
	{
		std::cout << std::endl << "Error cleaning" << std::endl;
		return false;
	}

	return false;
}

HDRE* HDRE::Get(const char* filename)
{
	auto it = s_loaded_hdres.find(filename);
	if (it != s_loaded_hdres.end())
		return it->second;

	HDRE* hdre = new HDRE();
	if (!hdre->load(filename))
	{
		delete hdre;
		return nullptr;
	}
	hdre->filename = filename;

	s_loaded_hdres[filename] = hdre;
	return hdre;
}