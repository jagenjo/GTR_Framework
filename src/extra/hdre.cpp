#include <iostream>
#include <fstream>
#include <cmath>
#include <cassert>
#include <algorithm>

#include "hdre.h"

HDRE::HDRE(const char* filename)
{
	load(filename);
}

HDRE::~HDRE()
{
	
}

sHDRELevel HDRE::getLevel(int n)
{
	sHDRELevel level;

	level.width = std::max(8, (int)(this->width / pow(2.0, n)));
	level.height = std::max(8, (int)(this->height / pow(2.0, n)));
	level.data = this->faces_array[n];
	level.faces = this->getFaces(n);

	return level;
}

float* HDRE::getData()
{
	return this->data;
}

float* HDRE::getFace(int level, int face)
{
	return this->pixels[level][face];
}

float** HDRE::getFaces(int level)
{
	return this->pixels[level];
}

bool HDRE::load(const char* filename)
{
	FILE *f;
	assert(filename);

	//fopen_s(&f, filename, "rb");
	f = fopen(filename, "rb");
	if (f == NULL)
		return false;

	sHDREHeader HDREHeader;

	fread(&HDREHeader, sizeof(sHDREHeader), 1, f);

	if (HDREHeader.type != 3)
		throw("ArrayType not supported. Please export in Float32Array.");

	this->header = HDREHeader;

	int width = HDREHeader.width;
	int height = HDREHeader.height;

	this->width = width;
	this->height = height;

	int dataSize = 0;
	int w = width;
	int h = height;

	// Get number of floats inside the HDRE
	// Per channel & Per face
	for (int i = 0; i < N_LEVELS; i++)
	{
		int mip_level = i + 1;
		dataSize += w * w * N_FACES * HDREHeader.numChannels;
		w = std::max(8, (int)(width / pow(2.0, mip_level)));
	}

	this->data = new float[dataSize];

	fseek(f, HDREHeader.headerSize, SEEK_SET);

	fread(this->data, sizeof(float) * dataSize, 1, f);
	fclose(f);

	// get separated levels

	w = width;
	int mapOffset = 0;
	
	for (int i = 0; i < N_LEVELS; i++)
	{
		int mip_level = i + 1;
		int faceSize = w * w * HDREHeader.numChannels;
		int mapSize = faceSize * N_FACES;
		
		int faceOffset = 0;
		int facePixel = 0;

		this->faces_array[i] = new float[mapSize];

		for (int j = 0; j < N_FACES; j++)
		{
			// allocate memory
			this->pixels[i][j] = new float[faceSize];

			// set data
			for (int k = 0; k < faceSize; k++) {

				float value = this->data[mapOffset + faceOffset + k];

				this->pixels[i][j][k] = value;
				this->faces_array[i][facePixel] = value;
				facePixel++;
			}

			// update face offset
			faceOffset += faceSize;
		}	

		// update level offset
		mapOffset += mapSize;
		// reassign width for next level
		w = std::max(8, (int)(width / pow(2.0, mip_level)));
	}

	std::cout << std::endl << " + '" << filename << "' loaded successfully" << std::endl;
	return true;
}