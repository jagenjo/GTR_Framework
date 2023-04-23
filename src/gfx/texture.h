/*  by Javi Agenjo 2013 UPF  javi.agenjo@gmail.com
	This contains a texture wrapper to use textures. It allow to load TGAs.
*/
#ifndef TEXTURE_H
#define TEXTURE_H

#include "../core/includes.h"
#include "../core/math.h"
#include "../core/task.h"
#include <map>
#include <set>
#include <string>
#include <cassert>

//forward declaration
namespace GFX {
	class Shader;
	class FBO;
	class Texture;
};

#ifndef OPENGL_ES3
#define GL_RGBA32F 0x8814
#define GL_RGB32F 0x8815
#define GL_HALF_FLOAT 0x140B
#define GL_RGB16F 0x881B
#define GL_RGBA16F 0x881A
#endif

#ifndef GL_TEXTURE_EXTERNAL_OES
	#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

//Simple class to handle images
template <typename T> class tImage
{
public:
	unsigned int width;
	unsigned int height;
	unsigned int num_channels; //bits per pixel
	bool origin_topleft;
	T* data; //bytes with the pixel information

	tImage() { width = height = 0; data = NULL; num_channels = 3; origin_topleft = true; }
	tImage(int w, int h, int num_channels = 3) { data = NULL; origin_topleft = true; resize(w, h, num_channels); }
	~tImage() { if (data) delete[]data; data = NULL; }

	void resize(int w, int h, int num_channels = 3) { if (data) delete[] data; width = w; height = h; this->num_channels = num_channels; data = new T[w * h * num_channels]; memset(data, 0, w * h * sizeof(T) * num_channels); }
	void clear() { if (data) delete[]data; data = NULL; width = height = 0; }
	void flipY();
};

class Image : public tImage<uint8>
{
public:
	Color getPixel(int x, int y) {
		assert(x >= 0 && x < (int)width && y >= 0 && y < (int)height && "reading of memory");
		int pos = y*width* num_channels + x* num_channels;
		return Color(data[pos], data[pos + 1], data[pos + 2], num_channels == 4 ? 255 : data[pos + 3]);
	};
	void setPixel(int x, int y, Color v) {
		assert(x >= 0 && x < (int)width && y >= 0 && y < (int)height && "writing of memory");
		int pos = y*width*num_channels + x* num_channels;
		data[pos] = v.x; data[pos + 1] = v.y; data[pos + 2] = v.z; if (num_channels == 4) data[pos + 3] = v.w;
	};

	Color getPixelInterpolated(float x, float y, bool repeat = false);
	Vector4f getPixelInterpolatedHigh(float x, float y, bool repeat = false); //returns a Vector4 (floats)

	void fromTexture(GFX::Texture* texture);
	void fromScreen(int width, int height);

	bool load(const char* filename);

	bool loadTGA(const char* filename);
	bool loadPNG(const char* filename, bool flip_y = true);
	bool loadPNG(std::vector<unsigned char>& buffer, bool flip_y = false);
	bool loadJPG(const char* filename, bool flip_y = false);
	bool loadJPG(std::vector<unsigned char>& buffer, bool flip_y = false);
	bool saveTGA(const char* filename, bool flip_y = false);
};

class FloatImage : public tImage<float>
{
public:
	//~FloatImage(); //no need, the tImage dtor is valid

	Vector4f getPixel(int x, int y) {
		assert(x >= 0 && x < (int)width&& y >= 0 && y < (int)height && "reading of memory");
		int pos = y * width * num_channels + x * num_channels;
		return Vector4f(data[pos], data[pos + 1], data[pos + 2], num_channels == 3 ? 1 : data[pos + 3]);
	};
	void setPixel(int x, int y, Vector4f v) {
		assert(x >= 0 && x < (int)width&& y >= 0 && y < (int)height && "writing of memory");
		int pos = y * width * num_channels + x * num_channels;
		data[pos] = v.x; data[pos + 1] = v.y; data[pos + 2] = v.z;
		if(num_channels == 4)
			data[pos + 3] = v.w;
	};
	void fromTexture(GFX::Texture* texture);
	void fromScreen(int width, int height);
	bool loadIBIN(const char* filename);
	bool saveIBIN(const char* filename);
};

namespace GFX {

	// TEXTURE CLASS
	class Texture
	{
	public:
		static int default_mag_filter;
		static int default_min_filter;
		static FBO* global_fbo;

		//a general struct to store all the information about a TGA file

		//textures manager
		static std::map<std::string, Texture*> sTexturesLoaded;
		static std::map<unsigned int, Texture*> sTextures;
		static unsigned int s_last_index;

		GLuint texture_id; // GL id to identify the texture in opengl, every texture must have its own id
		float width;
		float height;
		float depth;	//Optional for 3dTexture or 2dTexture array
		std::string filename;
		bool loading;
		vec2 near_far; //used for depth textures
		unsigned int index;

		unsigned int format; //GL_RGB, GL_RGBA, GL_DEPTH_COMPONENT
		unsigned int type; //GL_UNSIGNED_INT, GL_FLOAT
		unsigned int internal_format;
		unsigned int texture_type; //GL_TEXTURE_2D, GL_TEXTURE_CUBE, GL_TEXTURE_2D_ARRAY
		bool mipmaps;

		unsigned int wrapS;
		unsigned int wrapT;

		//original data info
		::Image image;

		Texture();
		Texture(unsigned int width, unsigned int height, unsigned int format = GL_RGB, unsigned int type = GL_UNSIGNED_BYTE, bool mipmaps = true, Uint8* data = NULL, unsigned int internal_format = 0);
		Texture(::Image* img);
		~Texture();

		static void Release();


		void clear();

		void create(unsigned int width, unsigned int height, unsigned int format = GL_RGB, unsigned int type = GL_UNSIGNED_BYTE, bool mipmaps = true, Uint8* data = NULL, unsigned int internal_format = 0);
		//void create3D(unsigned int width, unsigned int height, unsigned int depth, unsigned int format = GL_RED, unsigned int type = GL_UNSIGNED_BYTE, bool mipmaps = true, Uint8* data = NULL, unsigned int internal_format = 0);
		void createCubemap(unsigned int width, unsigned int height, Uint8** data = NULL, unsigned int format = GL_RGBA, unsigned int type = GL_UNSIGNED_BYTE, bool mipmaps = true, unsigned int internal_format = 0);

		void upload(::Image* img);
		void upload(::FloatImage* img);
		void upload(unsigned int format = GL_RGB, unsigned int type = GL_UNSIGNED_BYTE, bool mipmaps = true, const Uint8* data = NULL, unsigned int internal_format = 0);
		//void upload3D(unsigned int format = GL_RED, unsigned int type = GL_UNSIGNED_BYTE, bool mipmaps = true, Uint8* data = NULL, unsigned int internal_format = 0);
		void uploadCubemap(unsigned int format = GL_RGB, unsigned int type = GL_UNSIGNED_BYTE, bool mipmaps = true, Uint8** data = NULL, unsigned int internal_format = 0, int level = 0);
		void uploadAsArray(unsigned int texture_size, bool mipmaps = true);

		bool loadKTX(const char* filename);
		bool loadKTX(std::vector<unsigned char>& buffer);

		void bind();
		void unbind();

		void debugInMenu();

		static void UnbindAll();

		void operator = (const Texture& tex) { assert("textures cannot be cloned like this!"); }

		//load without using the manager
		bool load(const char* filename, bool mipmaps = true, bool wrap = true, unsigned int type = GL_UNSIGNED_BYTE);
		void loadFromImage(::Image* image, bool mipmaps = true, bool wrap = true, unsigned int type = GL_UNSIGNED_BYTE);

		//load using the manager (caching loaded ones to avoid reloading them)
		static Texture* Get(const char* filename, bool mipmaps = true, bool wrap = true);
		static Texture* GetAsync(const char* filename, bool mipmaps = true, bool wrap = true);
		static Texture* Find(const char* filename);
		void setName(const char* name) {
			filename = name;
			sTexturesLoaded[filename] = this;
		}

		void generateMipmaps();

		//show the texture on the current viewport
		void toViewport(Shader* shader = NULL);
		//copy to another texture
		void copyTo(Texture* destination, Shader* shader = NULL);

		static FBO* getGlobalFBO(Texture* texture);
		static Texture* getBlackTexture();
		static Texture* getWhiteTexture();
	};

};

GFX::Texture* CubemapFromHDRE(const char* filename, GFX::Texture* output = nullptr);


bool isPowerOfTwo(int n);

//When loading textures asyncrhonously, first we load them from the hard drive in a background thread
//afterwards we pass the data to the main thread as bg threads cannot access opengl, and main thread
//uploads to GPU. While loading a fake 1x1 texture is created

class LoadTextureTask : public Task {
public:
	std::string filename;
	Image* image;

	LoadTextureTask(const char* filename);
	void onExecute();
};

class UploadTextureTask : public Task {
public:
	std::string filename;
	Image* image;

	UploadTextureTask(const char* filename, Image* image);
	void onExecute();
};

#endif