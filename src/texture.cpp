
#include "texture.h"
#include "fbo.h"
#include "utils.h"

#include <iostream> //to output
#include <cmath>

#include "mesh.h"
#include "shader.h"
#include "extra/picopng.h"
#include "extra/jpgd.h"
#include <cassert>

#define STB_IMAGE_IMPLEMENTATION
//#include "extra/stb_image.h"
//#include "engine/application.h"

#include "extra/stb_image.h"

#ifdef USE_SKIA

#define SK_CPU_LENDIAN 1
#include "skia/include/core/SkData.h"
#include "skia/include/core/SkImage.h"
#include "skia/include/codec/SkCodec.h"

#endif

//bilinear interpolation
Color Image::getPixelInterpolated(float x, float y, bool repeat) {
	int ix = repeat ? fmod(x,width) : clamp(x,0,width-1);
	int iy = repeat ? fmod(y,height) : clamp(y, 0, height - 1);
	if (ix < 0) ix += width;
	if (iy < 0) iy += height;
	float fx = (x - (int)x);
	float fy = (y - (int)y);
	int ix2 = ix < width - 1 ? ix + 1 : 0;
	int iy2 = iy < height - 1 ? iy + 1 : 0;
	Color top = lerp( getPixel(ix, iy), getPixel(ix2, iy), fx );
	Color bottom = lerp( getPixel(ix, iy2), getPixel(ix2, iy2), fx);
	return lerp(top, bottom, fy);
};

Vector4 Image::getPixelInterpolatedHigh(float x, float y, bool repeat) {
	int ix = repeat ? fmod(x, width) : clamp(x, 0, width - 1);
	int iy = repeat ? fmod(y, height) : clamp(y, 0, height - 1);
	if (ix < 0) ix += width;
	if (iy < 0) iy += height;
	float fx = (x - (int)x);
	float fy = (y - (int)y);
	int ix2 = ix < width - 1 ? ix + 1 : 0;
	int iy2 = iy < height - 1 ? iy + 1 : 0;
	Vector4 top = lerp( getPixel(ix, iy).toVector4(), getPixel(ix2, iy).toVector4(), fx);
	Vector4 bottom = lerp(getPixel(ix, iy2).toVector4(), getPixel(ix2, iy2).toVector4(), fx);
	return lerp(top, bottom, fy);
};


std::map<std::string, Texture*> Texture::sTexturesLoaded;

int Texture::default_mag_filter = GL_LINEAR;
int Texture::default_min_filter = GL_LINEAR_MIPMAP_LINEAR;
FBO* Texture::global_fbo = NULL;

Texture::Texture()
{
	width = 0;
	height = 0;
	depth = 0;
	texture_id = 0;
	mipmaps = false;
	format = 0;
	type = 0;
	texture_type = GL_TEXTURE_2D;
	loading = false;
}

Texture::Texture(unsigned int width, unsigned int height, unsigned int format, unsigned int type, bool mipmaps, Uint8* data, unsigned int internal_format)
{
	loading = false;
	texture_id = 0;
	create(width, height, format, type, mipmaps, data, internal_format);
}

Texture::Texture(Image* img)
{
	loading = false;
	texture_id = 0;
	create(img->width, img->height, img->num_channels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, true, img->data);
}

Texture::~Texture()
{
	clear();
}

void Texture::clear()
{
	glBindTexture(this->texture_type, 0);

	//external textures are handled by an outside system (like Android OS)
	if( texture_type != GL_TEXTURE_EXTERNAL_OES)
		glDeleteTextures(1, &texture_id);

	if(!loading) //when loading the texture of 1x1 is replaced with the new one
		stdlog("Destroy texture: " + filename );
	texture_id = 0;

	if (filename.size())
	{
		auto it = sTexturesLoaded.find(filename);
		if (it != sTexturesLoaded.end())
			sTexturesLoaded.erase(it);
	}
}

void Texture::Release()
{
	std::vector<Texture *> texs;

	for (auto mp : sTexturesLoaded)
	{
		Texture *m = mp.second;
		texs.push_back(m);
	}

	for (Texture *m : texs)
	{
		delete m;
	}
	sTexturesLoaded.clear();
}

void Texture::debugInMenu()
{
	#ifdef IMGUI
	if (this == NULL)
		return;
	this->bind();
		ImGui::Image((void*)(intptr_t)texture_id, ImVec2(50, 50));
	#endif
}

void Texture::create(unsigned int width, unsigned int height, unsigned int format, unsigned int type, bool mipmaps, Uint8* data, unsigned int internal_format)
{
	assert(width && height && "texture must have a size");

	this->width = (float)width;
	this->height = (float)height;
	this->depth = 0;
	this->format = format;
	this->internal_format = internal_format;
	this->type = type;
	this->mipmaps = mipmaps && isPowerOfTwo(width) && isPowerOfTwo(height) && format != GL_DEPTH_COMPONENT;

	//Delete previous texture and ensure that previous bounded texture_id is not of another texture type
	if (this->texture_id != 0)
		clear();

	this->texture_type = GL_TEXTURE_2D;

	if(texture_id == 0)
		glGenTextures(1, &texture_id); //we need to create an unique ID for the texture

	assert(checkGLErrors() && "Error creating texture");
	upload(format, type, mipmaps, data, internal_format);
}

/*
void Texture::create3D(unsigned int width, unsigned int height, unsigned int depth, unsigned int format, unsigned int type, bool mipmaps, Uint8* data, unsigned int internal_format)
{
	assert(width && height && depth && "texture must have a size");

	this->width = (float)width;
	this->height = (float)height;
	this->depth = (float)depth;
	this->format = format;
	this->internal_format = internal_format;
	this->type = type;
	this->mipmaps = mipmaps && isPowerOfTwo(width) && isPowerOfTwo(height) && format != GL_DEPTH_COMPONENT && isPowerOfTwo(this->depth);

	//Delete previous texture and ensure that previous bounded texture_id is not of another texture type
	if (this->texture_id != 0)
		clear();

	this->texture_type = GL_TEXTURE_3D;

	if (texture_id == 0)
		glGenTextures(1, &texture_id); //we need to create an unique ID for the texture

	assert(checkGLErrors() && "Error creating texture");

	upload3D(format, type, mipmaps, data, internal_format);
}
*/

void Texture::createCubemap(unsigned int width, unsigned int height, Uint8** data, unsigned int format, unsigned int type, bool mipmaps, unsigned int internal_format)
{
	assert(width && height && "texture must have a size");

	this->width = (float)width;
	this->height = (float)height;
	this->depth = 0;
	this->format = format;
	this->internal_format = internal_format;
	this->type = type;
	this->texture_type = GL_TEXTURE_CUBE_MAP;
	this->mipmaps = mipmaps && isPowerOfTwo(width) && isPowerOfTwo(height) && format != GL_DEPTH_COMPONENT;

	this->wrapS = GL_CLAMP_TO_EDGE;
	this->wrapT = GL_CLAMP_TO_EDGE;

	if (texture_id == 0)
		glGenTextures(1, &texture_id); //we need to create an unique ID for the texture

	glBindTexture(this->texture_type, texture_id);	//we activate this id to tell opengl we are going to use this texture
	uploadCubemap(format, type, mipmaps, data, internal_format);
}

Texture* Texture::Find(const char* filename)
{
	assert(filename);
	auto it = sTexturesLoaded.find(filename);
	if (it != sTexturesLoaded.end())
		return it->second;
	return NULL;
}

Texture* Texture::Get(const char* filename, bool mipmaps, bool wrap)
{
	//load it
	Texture* texture = Find(filename);
	if (texture)
		return texture;

	texture = new Texture();
	if (!texture->load(filename, mipmaps, wrap))
	{
		delete texture;
		return NULL;
	}

	return texture;
}

Texture* Texture::GetAsync(const char* filename, bool mipmaps, bool wrap)
{
	//check if exists
	Texture* texture = Find(filename);
	if (texture)
		return texture;

	//create temp texture
	Texture* temp = new Texture();
	temp->create(1, 1);
	//register
	temp->setName(filename);
	temp->loading = true;

	//add action to BG Thread 
	LoadTextureTask* task = new LoadTextureTask(filename);
	TaskManager::background.addTask(task);

	return temp;
}

bool Texture::load(const char* filename, bool mipmaps, bool wrap, unsigned int type)
{
	Image* image = new Image();
	if (!image->load(filename))
	{
		delete image;
		return false;
	}

	loadFromImage(image,mipmaps,wrap,type);
	setName(filename);

	this->image.clear(); //remove from RAM after loading. ???
	return true;
}

void Texture::loadFromImage(Image* image, bool mipmaps, bool wrap, unsigned int type)
{
	unsigned int internal_format = 0;
	if (type == GL_FLOAT)
		internal_format = (image->num_channels == 3 ? GL_RGB32F : GL_RGBA32F);

	//upload to VRAM
	// We have to synchronously upload for now because Image class is not ref-counted
	create(image->width, image->height, (image->num_channels == 3 ? GL_RGB : GL_RGBA), type,  mipmaps, image->data, 0);

	glBindTexture(this->texture_type, texture_id);	//we activate this id to tell opengl we are going to use this texture
	glTexParameteri(this->texture_type, GL_TEXTURE_WRAP_S, (this->mipmaps && wrap) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri(this->texture_type, GL_TEXTURE_WRAP_T, (this->mipmaps && wrap) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	//glTexParameteri(this->texture_type, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(this->texture_type, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//if (mipmaps)
	//	generateMipmaps();
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::upload(Image* img)
{
	create(img->width, img->height, img->num_channels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, true, img->data);
}

void Texture::upload(FloatImage* img)
{
	create(img->width, img->height, img->num_channels == 3 ? GL_RGB : GL_RGBA, GL_FLOAT, true);
	upload(this->format, this->type, false, (Uint8*)img->data);
}


//uploads the bytes of a texture to the VRAM
void Texture::upload( unsigned int format, unsigned int type, bool mipmaps, Uint8* data, unsigned int internal_format)
{
	assert(texture_id && "Must create texture before uploading data.");
	assert(texture_type == GL_TEXTURE_2D && "Texture type does not match.");

	glBindTexture(this->texture_type, texture_id);	//we activate this id to tell opengl we are going to use this texture

	if (internal_format == 0)
	{
		if (type == GL_FLOAT)
			internal_format = format == GL_RGB ? GL_RGB32F : GL_RGBA32F;
		else if (type == GL_HALF_FLOAT)
			internal_format = format == GL_RGB ? GL_RGB16F : GL_RGBA16F;
	}

	glTexImage2D(this->texture_type, 0, internal_format == 0 ? format : internal_format, width, height, 0, format, type, data);

	glTexParameteri(this->texture_type, GL_TEXTURE_MAG_FILTER, Texture::default_mag_filter);	//set the min filter
	glTexParameteri(this->texture_type, GL_TEXTURE_MIN_FILTER, this->mipmaps ? Texture::default_min_filter : GL_LINEAR);   //set the mag filter
	glTexParameteri(this->texture_type, GL_TEXTURE_WRAP_S, this->mipmaps ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri(this->texture_type, GL_TEXTURE_WRAP_T, this->mipmaps ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4); //better quality but takes more resources

	if (data && this->mipmaps)
		generateMipmaps(); //glGenerateMipmapEXT(GL_TEXTURE_2D); 

	glBindTexture(this->texture_type, 0);
	assert(checkGLErrors() && "Error uploading texture");
}

/*
void Texture::upload3D(unsigned int format, unsigned int type, bool mipmaps, Uint8* data, unsigned int internal_format) {
	assert(texture_id && "Must create texture before uploading data.");
	assert(texture_type == GL_TEXTURE_3D && "Texture type does not match.");

	glBindTexture(this->texture_type, texture_id);	//we activate this id to tell opengl we are going to use this texture

	glTexImage3D(this->texture_type, 0, internal_format == 0 ? format : internal_format, width, height, depth, 0, format, type, data);

	glTexParameteri(this->texture_type, GL_TEXTURE_MAG_FILTER, Texture::default_mag_filter);	//set the min filter
	glTexParameteri(this->texture_type, GL_TEXTURE_MIN_FILTER, this->mipmaps ? Texture::default_min_filter : GL_LINEAR);   //set the mag filter
	glTexParameteri(this->texture_type, GL_TEXTURE_WRAP_S, this->mipmaps ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri(this->texture_type, GL_TEXTURE_WRAP_T, this->mipmaps ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri(this->texture_type, GL_TEXTURE_WRAP_R, this->mipmaps ? GL_REPEAT : GL_CLAMP_TO_EDGE);

	if (data && this->mipmaps)
		generateMipmaps(); //glGenerateMipmapEXT(GL_TEXTURE_2D); 

	glBindTexture(this->texture_type, 0);
	assert(checkGLErrors() && "Error uploading texture");
}
*/

void Texture::uploadCubemap(unsigned int format, unsigned int t, bool mips, Uint8** data, unsigned int intFormat, int level) {
	
	assert(texture_id && "Must create texture before uploading data.");
	assert(texture_type == GL_TEXTURE_CUBE_MAP && "Texture type does not match.");
	//assert(glGetError() == GL_NO_ERROR);

	glBindTexture(this->texture_type, texture_id);	//we activate this id to tell opengl we are going to use this texture

	int w = ((int)this->width) >> level;
	int h = ((int)this->height) >> level;

	if (intFormat == 0)
	{
		if (type == GL_FLOAT)
			intFormat = format == GL_RGB ? GL_RGB32F : GL_RGBA32F;
		else
		if (type == GL_HALF_FLOAT)
			intFormat = format == GL_RGB ? GL_RGB16F : GL_RGBA16F;
        else
        if (type == GL_UNSIGNED_BYTE)
            intFormat = format == GL_RGB ? GL_RGB : GL_RGBA;
		this->internal_format = intFormat;
	}

	for (int i = 0; i < 6; i++)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, level, internal_format == 0 ? format : internal_format, w, h, 0, format, t, data ? data[i] : NULL);

	glTexParameteri(this->texture_type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(this->texture_type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	bool bAllowMips = true;

    /*if (t == GL_HALF_FLOAT)
    {
        short *s = (short *)data[0];
        printf("Env Data %d, Mip: %d: [%.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f\n", texture_id, level, r3dHalfToFloat(s[0]), r3dHalfToFloat(s[1]), r3dHalfToFloat(s[2]), r3dHalfToFloat(s[3]), r3dHalfToFloat(s[4]), r3dHalfToFloat(s[5]), r3dHalfToFloat(s[6]), r3dHalfToFloat(s[7]));
    }*/

	if (level == 0)
	{
		glTexParameteri(this->texture_type, GL_TEXTURE_MAG_FILTER, Texture::default_mag_filter);	//set the min filter
		glTexParameteri(this->texture_type, GL_TEXTURE_MIN_FILTER, this->mipmaps ? Texture::default_min_filter : GL_LINEAR);   //set the mag filter
		//if (data && this->mipmaps && level == 0 && bAllowMips)
		//	generateMipmaps();
	}

	glBindTexture(this->texture_type, 0);
	assert(glGetError() == GL_NO_ERROR && "Error creating texture");
}

//special function to upload texture arrays, a special type of texture that has layers
void Texture::uploadAsArray(unsigned int texture_size, bool mipmaps)
{
#ifndef OPENGL_ES3
	assert(0 && "texture arrays not supported");
#else
	assert((image.height % texture_size) == 0); //size doesnt match
	assert(image.data);//no image in memory
	int num_columns = image.width / texture_size;
	int num_rows = image.height / texture_size;
	int num_textures = num_columns * num_rows;
	int width = texture_size;
	int height = texture_size;

	int max_layers;
	glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_layers);
	if (max_layers < num_textures)
	{
		std::cout << "GPU does not support " << std::endl;
		return;
	}

	texture_type = GL_TEXTURE_2D_ARRAY;
	type = GL_UNSIGNED_BYTE;
	int dataFormat = (image.num_channels == 3 ? GL_RGB : GL_RGBA);
	format = (image.num_channels == 3 ? GL_RGB8 : GL_RGBA8);
	this->width = (float)width;
	this->height = (float)height;
	int bytes_per_pixel = image.num_channels;
	this->mipmaps = mipmaps && isPowerOfTwo((int)width) && isPowerOfTwo((int)height);
	uint8* data = NULL;

	//if texture is a grid, linearize the data so it can be uploaded in a single call
	if (num_columns > 1)
	{
		data = new uint8[num_textures * width * height * bytes_per_pixel];
		int offset = width * bytes_per_pixel;
		int offset_row = width * bytes_per_pixel * num_columns;
		int offset_image = width * height * bytes_per_pixel * num_columns;
		int offset_image_linear = width * height * bytes_per_pixel;
		for (int i = 0; i < num_rows; ++i)
			for (int j = 0; j < num_columns; ++j)
			{
				int start = (i * offset_image) + (j * offset);
				int start_linear = (i * num_columns + j) * offset_image_linear;
				for (int k = 0; k < height; ++k)
					memcpy(data + start_linear + (height - k - 1) * offset, image.data + start + k * offset_row, offset);
			}
	}
	else
		data = image.data;

	//How to store a texture in VRAM
	assert(glGetError() == GL_NO_ERROR);
	if (texture_id == 0)
		glGenTextures(1, &texture_id); //we need to create an unique ID for the texture
	glBindTexture( this->texture_type, texture_id);	//we activate this id to tell opengl we are going to use this texture
	glTexImage3D( this->texture_type, 0, format, width, height, num_textures, 0, dataFormat, type, data);
	assert(glGetError() == GL_NO_ERROR);

	glTexParameteri(this->texture_type, GL_TEXTURE_MAG_FILTER, Texture::default_mag_filter);	//set the min filter
	glTexParameteri(this->texture_type, GL_TEXTURE_MIN_FILTER, this->mipmaps ? Texture::default_min_filter : GL_LINEAR); //set the mag filter
	glTexParameteri(this->texture_type, GL_TEXTURE_WRAP_S, this->mipmaps ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri(this->texture_type, GL_TEXTURE_WRAP_T, this->mipmaps ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameterf(this->texture_type, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4); //better quality but takes more resources
	assert(glGetError() == GL_NO_ERROR);
	if (mipmaps)
		generateMipmaps();
	assert(glGetError() == GL_NO_ERROR);

	if (num_columns > 1)
		delete[] data;
	#endif
}


void Texture::bind()
{
	//glEnable(this->texture_type); //enable the textures 
	glBindTexture(this->texture_type, texture_id );	//enable the id of the texture we are going to use
}

void Texture::unbind()
{
	//glDisable(this->texture_type); //disable the textures 
	glBindTexture(this->texture_type, 0 );	//disable the id of the texture we are going to use
}

void Texture::UnbindAll()
{
	glDisable( GL_TEXTURE_CUBE_MAP );
	glDisable( GL_TEXTURE_2D );
	glDisable(GL_TEXTURE_3D);
	glBindTexture( GL_TEXTURE_2D, 0 );
	glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );
	glBindTexture(GL_TEXTURE_3D, 0);
}

void Texture::generateMipmaps()
{
#ifdef OPENGL_ES3
		if(!glGenerateMipmapEXT)
			return;

		glBindTexture(this->texture_type, texture_id );	//enable the id of the texture we are going to use
		glTexParameteri(this->texture_type, GL_TEXTURE_MIN_FILTER, Texture::default_min_filter ); //set the mag filter
		if (this->texture_type == GL_TEXTURE_CUBE_MAP)
		{
			glTexParameteri(this->texture_type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); //set the mag filter
			glTexParameteri(this->texture_type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); //set the mag filter
		}
		glGenerateMipmapEXT(this->texture_type);
#else
	glBindTexture(this->texture_type, texture_id);	//enable the id of the texture we are going to use
	glTexParameteri(this->texture_type, GL_TEXTURE_MIN_FILTER, Texture::default_min_filter);
	glGenerateMipmap(this->texture_type);
    #endif
}


void Texture::toViewport(Shader* shader)
{
	Mesh* quad = Mesh::getQuad();
	if (!shader)
		shader = Shader::getDefaultShader("screen");
	shader->enable();
	if(shader->getUniformLocation("u_texture") != -1)
		shader->setUniform("u_texture", this, 0);
	assert(glGetError() == GL_NO_ERROR);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	quad->render(GL_TRIANGLES);
	assert(glGetError() == GL_NO_ERROR);
	shader->disable();
}

FBO* Texture::getGlobalFBO(Texture* texture)
{
	if (!global_fbo)
		global_fbo = new FBO();
	global_fbo->setTexture(texture);
	return global_fbo;
}

Texture* Texture::getBlackTexture()
{
	static Texture* black = NULL;
	if (black)
		return black;
	const Uint8 data[3] = { 0,0,0 };
	black = new Texture(1, 1, GL_RGB, GL_UNSIGNED_BYTE, true, (Uint8*)data);
	return black;
}

Texture* Texture::getWhiteTexture()
{
	static Texture* white = NULL;
	if (white)
		return white;
	const Uint8 data[3] = { 255,255,255 };
	white = new Texture(1, 1, GL_RGB, GL_UNSIGNED_BYTE, true, (Uint8*)data);
	return white;
}

void Texture::copyTo(Texture* destination, Shader* shader)
{
	if (!destination) //to current viewport
	{
		if (format == GL_DEPTH_COMPONENT) //to clone depth buffer
		{
			glEnable(GL_DEPTH_TEST); //we need to use the depth buffer
			glDepthFunc(GL_ALWAYS); //but ignore the test, every fragment should update the depth
			glColorMask(false, false, false, false); //block drawing to colors
			if(!shader)
				shader = Shader::getDefaultShader("screen_depth");
		}
		else if (!shader)
			shader = Shader::getDefaultShader("texture");
		Mesh* quad = Mesh::getQuad();
		shader->enable();
		shader->setUniform("u_texture", this, 0);
		shader->setUniform("u_color", Vector4(1,1,1,1) );
		glDisable(GL_CULL_FACE);
		quad->render(GL_TRIANGLES);
		glColorMask(true, true, true, true);
		glDisable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		return;
	}

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	FBO* fbo = getGlobalFBO(destination);
	fbo->bind();
	if (!shader && format == GL_DEPTH_COMPONENT)
	{
		shader = Shader::getDefaultShader("screen_depth");
		glDepthFunc(GL_ALWAYS);
		glEnable(GL_DEPTH_TEST);
	}
	toViewport(shader);
	fbo->unbind();
	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
}

void Image::fromScreen(int width, int height)
{
	if (data && (width != this->width || height != this->height))
		clear();

	if (!data)
	{
		this->width = width;
		this->height = height;
		data = new uint8[width * height * 4];
	}

	glReadPixels(0,0,width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

void Image::fromTexture(Texture* texture)
{
	assert(texture);

	if(data && (width != texture->width || height != texture->height ))
		clear();

	if (!data)
	{
		width = texture->width;
		height = texture->height;
		data = new uint8[width * height * 4];
	}
	
	texture->bind();
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

bool Image::load(const char* filename)
{
	std::string str = filename;
	std::string ext = str.substr(str.size() - 4, 4);
	double time = getTime();
	std::cout << " + Image loading: " << filename << " ... ";

	bool found = false;

	if (ext == ".tga" || ext == ".TGA")
		found = loadTGA(filename);
	else if (ext == ".png" || ext == ".PNG")
		found = loadPNG(filename);
	else if (ext == ".jpg" || ext == ".JPG" || ext == "JPEG" || ext == "jpeg")
		found = loadJPG(filename);
	else
	{
		std::cout << "[ERROR]: unsupported format" << std::endl;
		return false; //unsupported file type
	}

	if (!found) //file not found
	{
		std::cout << " [ERROR]: Texture not found " << std::endl;
		return false;
	}

	std::cout << "[OK] Size: " << width << "x" << height << " Time: " << (getTime() - time) * 0.001 << "sec" << std::endl;

	return true;
}

//TGA format from: http://www.paulbourke.net/dataformats/tga/
//also on https://gshaw.ca/closecombat/formats/tga.html
bool Image::loadTGA(const char* filename)
{
	GLubyte TGAheader[12] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    GLubyte TGAcompare[12];
    GLubyte header[6];
    GLuint bytesPerPixel;
    GLuint imageSize;
    //GLuint type = GL_RGBA;

    FILE * file = fopen(filename, "rb");
    
    if ( file == NULL || fread(TGAcompare, 1, sizeof(TGAcompare), file) != sizeof(TGAcompare) ||
        memcmp(TGAheader, TGAcompare, sizeof(TGAheader)) != 0 ||
        fread(header, 1, sizeof(header), file) != sizeof(header))
    {
        if (file == NULL)
            return NULL;
        else
        {
            fclose(file);
            return NULL;
        }
    }

    width = header[1] * 256 + header[0];
    height = header[3] * 256 + header[2];
	num_channels = header[4] / 8;

	bool error = false;

	if (num_channels != 3 && num_channels != 4)
	{
		error = true;
		std::cerr << "File format not supported: " << num_channels << " bytes per pixel" << std::endl;
	}
    
    if (width <= 0 || height <= 0)
	{
		error = true;
		std::cerr << "Wrong texture size: " << width << "x" << height << " pixels" << std::endl;
	}

	if (error)
    {
        fclose(file);
        return false;
    }

    imageSize = width * height * num_channels;
    
    data = new GLubyte[imageSize];
    if (data == NULL || fread(data, 1, imageSize, file) != imageSize)
    {
        if (data != NULL)
            delete []data;
        fclose(file);
        return NULL;
    }

	if (header[5] & (1 << 5)) //flip
		origin_topleft = true;
    
	//flip BGR to RGB pixels
	#pragma omp simd
    for (GLuint i = 0; i < int(imageSize); i += num_channels)
    {
        uint8 temp = data[i];
        data[i] = data[i + 2];
        data[i + 2] = temp;
    }
    
    fclose(file);
	return true;
}

#include <iostream>
#include <fstream>

bool Image::loadPNG(const char* filename, bool flip_y)
{
	std::vector<unsigned char> buffer;
	if (!readFileBin(filename, buffer))
		return false;
	return loadPNG(buffer);
}

bool Image::loadPNG(std::vector<unsigned char>& buffer, bool flip_y)
{
#ifdef USE_SKIA
    sk_sp<SkData> skData = SkData::MakeWithoutCopy(&buffer[0], buffer.size());
    std::unique_ptr<SkCodec> codec(SkCodec::MakeFromData(skData));
    SkBitmap bitmap;
    const SkImageInfo skInfo = codec->getInfo();
    bitmap.allocPixels(skInfo);
    if (bitmap.getPixels())
    {
        codec->getPixels(skInfo, bitmap.getPixels(), skInfo.minRowBytes());
        width = bitmap.width();
        height = bitmap.height();
        const uint8_t *pSrc = (uint8_t *)bitmap.getPixels();
        size_t nSize = bitmap.computeByteSize();

        this->width = (unsigned int)width;
        this->height = (unsigned int)height;
        this->num_channels = skInfo.bytesPerPixel(); // (unsigned int)channels;

        data = new unsigned char[nSize];
        memcpy(data, pSrc, nSize);
    }
#else
    std::vector<unsigned char> out_image;

	if (decodePNG(out_image, width, height, buffer.empty() ? 0 : &buffer[0], (unsigned long)buffer.size(), true) != 0)
		return false;

	data = new Uint8[out_image.size()];
	memcpy(data, &out_image[0], out_image.size());
	num_channels = 4;
#endif

	//flip pixels in Y
	if (flip_y)
		flipY();

	return true;
}

bool Image::loadJPG(const char* filename, bool flip_y)
{
	std::vector<unsigned char> buffer;
	if (!readFileBin(filename, buffer))
		return false;
	return loadJPG(buffer);
}

bool Image::loadJPG(std::vector<unsigned char>& buffer, bool flip_y)
{
	std::vector<unsigned char> out_image;

	int width;
	int height;
	int actual_comps;
	int channels;

	/*
	int req_comps = 3;
	assert(data == NULL); //image must be empty
	data = jpgd::decompress_jpeg_image_from_memory(&buffer[0], (unsigned long)buffer.size(), &width, &height, &actual_comps, req_comps);
	if(!data)
		return false;

	this->width = (unsigned int)width;
	this->height = (unsigned int)height;
	*/

#ifdef USE_SKIA
    sk_sp<SkData> skData = SkData::MakeWithoutCopy(&buffer[0], buffer.size());
    std::unique_ptr<SkCodec> codec(SkCodec::MakeFromData(skData));
    SkBitmap bitmap;
    const SkImageInfo skInfo = codec->getInfo();
    bitmap.allocPixels(skInfo);
    if (bitmap.getPixels())
    {
        codec->getPixels(skInfo, bitmap.getPixels(), skInfo.minRowBytes());
        width = bitmap.width();
        height = bitmap.height();
        const uint8_t *pSrc = (uint8_t *)bitmap.getPixels();
        size_t nSize = bitmap.computeByteSize();

        this->width = (unsigned int)width;
        this->height = (unsigned int)height;
        this->num_channels = skInfo.bytesPerPixel(); // (unsigned int)channels;

        data = new unsigned char[nSize];
        memcpy(data, pSrc, nSize);
    }
#else
	//stb_image
	unsigned char* image_data = stbi_load_from_memory( (stbi_uc*) &buffer[0], (unsigned long)buffer.size(), &width, &height, &channels, STBI_rgb);
	if (!image_data)
		return false;
	this->width = (unsigned int)width;
	this->height = (unsigned int)height;
	this->num_channels = 3;// (unsigned int)channels;

	//clone
	data = new unsigned char[width * height * this->num_channels];
	memcpy(data, image_data, width * height * this->num_channels);

	stbi_image_free(image_data);
#endif

	//flip pixels in Y
	if (flip_y)
		flipY();

	return true;
}

// Saves the image to a TGA file
bool Image::saveTGA(const char* filename, bool flip_y)
{
	unsigned char TGAheader[12] = { 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#ifdef ANDROID
	//assert(!"not supported");
	return false;
#endif
	FILE *file = fopen(filename, "wb");
	if (file == NULL)
	{
		fclose(file);
		return false;
	}

	unsigned short header_short[3];
	header_short[0] = width;
	header_short[1] = height;
	unsigned char* header = (unsigned char*)header_short;
	header[4] = 32; //bitsperpixel
	header[5] = 0;

	fwrite(TGAheader, 1, sizeof(TGAheader), file);
	fwrite(header, 1, 6, file);

	//convert pixels to unsigned char
	unsigned char* bytes = new unsigned char[width*height * 4];
	for (unsigned int y = 0; y < height; ++y)
		for (unsigned int x = 0; x < width; ++x)
		{
			Uint8* p = data + (height - y - 1)*width*4 + x*4;
			unsigned int pos = (y*width + x) * 4;
			if(flip_y)
				pos = ((height - y - 1)*width + x) * 4;
			bytes[pos + 2] = *p;
			bytes[pos + 1] = *(p+1);
			bytes[pos] = *(p+2);
			bytes[pos + 3] = *(p+3);
		}

	fwrite(bytes, 1, width*height * 4, file);
	fclose(file);
	return true;
}

template<typename T>
void tImage<T>::flipY()
{
	assert(data);
	int row_size = num_channels * width;
	T* temp_row = new T[row_size];
#pragma omp simd
	for (int y = 0; y < height*0.5; y += 1)
	{
		uint8* pos = data + y*row_size;
		memcpy(temp_row, pos, row_size);
		uint8* pos2 = data + (height - y - 1)*row_size;
		memcpy(pos, pos2, row_size);
		memcpy(pos2, temp_row, row_size);
	}
	delete[] temp_row;
}

struct tImageHeader {
	int width;
	int height;
	int layers;
	uint8 channels;
	uint8 bytesperchannel;
	uint8 flags[17]; //32 bytes header
};

bool FloatImage::saveIBIN(const char* filename)
{
	tImageHeader header;
	header.width = width;
	header.height = height;
	header.layers = 1;
	header.bytesperchannel = 4;
	header.channels = 3;
	FILE* file = fopen(filename, "wb");
	if (file == NULL)
	{
		fclose(file);
		return false;
	}
	fwrite(&header, 1, sizeof(header), file);
	fwrite(data, 1, sizeof(float) * width * height * num_channels * header.layers, file);
	fclose(file);
	return true;
}

bool FloatImage::loadIBIN(const char* filename)
{
	FILE* file = fopen(filename, "rb");
	if (file == NULL)
		return false;
	tImageHeader header;
	fread(&header, 1, sizeof(header), file);
	resize(header.width, header.height, header.channels);
	fread(data, 1, sizeof(float) * width * height * num_channels * header.layers, file);
	fclose(file);
	return true;
}

void FloatImage::fromTexture(Texture* texture)
{
	assert(texture);
	assert(texture->type == GL_FLOAT);
	if (data && (width != texture->width || height != texture->height))
		clear();
	if (!data)
	{
		width = texture->width;
		height = texture->height;
		data = new float[width * height * num_channels];
	}

	texture->bind();
	glGetTexImage(GL_TEXTURE_2D, 0, num_channels == 3 ? GL_RGB : GL_RGBA, GL_FLOAT, data);
}


bool isPowerOfTwo( int n )
{
	return (n & (n - 1)) == 0;
}

//*********************

LoadTextureTask::LoadTextureTask(const char* str)
{
	filename = str;
	image = NULL;
}

void LoadTextureTask::onExecute()
{
	image = new Image();
	if (!image->load(filename.c_str()))
	{
		delete image;
		image = NULL;
	}

	//image loaded, ready to go back to main thread
	UploadTextureTask* upload_task = new UploadTextureTask(filename.c_str(), image);
	TaskManager::foreground.addTask(upload_task);
}

UploadTextureTask::UploadTextureTask(const char* filename, Image* image)
{
	this->filename = filename;
	this->image = image;
}

void UploadTextureTask::onExecute()
{
	Texture* texture = NULL;

	//in case somehow it got loaded while I was loading it in the background
	auto it = Texture::sTexturesLoaded.find(filename);
	if (it == Texture::sTexturesLoaded.end())
	{
		/*
		//create texture
		if (!texture)
			texture = new Texture();
		*/
		delete image;
		std::cout << "Warning: image loaded in background not found foreground thread" << std::endl;
		return;
	}

	texture = it->second;

	//upload to GPU
	texture->loadFromImage(image);
	texture->loading = false;

	//delete image
	delete image;
}
