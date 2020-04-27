#ifndef FBO_H
#define FBO_H

#include "includes.h"
#include "texture.h"

//FrameBufferObject
//helps rendering the scene inside a texture

class FBO {
public:
	GLuint fbo_id; 
	Texture* color_textures[4];
	Texture* depth_texture;
	int num_color_textures;
	GLenum bufs[4];
	int width;
	int height;
	bool owns_textures;

	GLuint renderbuffer_color;
	GLuint renderbuffer_depth;//not used

	FBO();
	~FBO();

	bool create(int width, int height, int num_textures = 1, int format = GL_RGB, int type = GL_UNSIGNED_BYTE, bool use_depth_texture = true );
	bool setTexture(Texture* texture, int cubemap_face = -1);
	bool setTextures(std::vector<Texture*> textures, Texture* depth = NULL, int cubemap_face = -1);
	bool setDepthOnly(int width, int height); //use this for shadowmaps
	
	void bind();
	void unbind();

	//to render momentarily to a single buffer
	void enableSingleBuffer(int num);
	void enableAllBuffers(); //back to all

	void freeTextures();
};

#endif