#include "fbo.h"
#include <cassert>
#include "utils.h"

FBO::FBO()
{
	fbo_id = 0;
	color_textures[0] = color_textures[1] = color_textures[2] = color_textures[3] = NULL;
	depth_texture = NULL;

	renderbuffer_color = 0;
	renderbuffer_depth = 0;
	num_color_textures = 0;
	owns_textures = false;
	width = 0;
	height = 0;
}

FBO::~FBO()
{
	freeTextures();
	if (fbo_id)
		glDeleteFramebuffers(1, &fbo_id);
	if (renderbuffer_color)
		glDeleteRenderbuffersEXT(1, &renderbuffer_color);
	if (renderbuffer_depth)
		glDeleteRenderbuffersEXT(1, &renderbuffer_depth);
}

void FBO::freeTextures()
{
	for (int i = 0; i < 4; ++i)
	{
		Texture* t = color_textures[i];
		if (t && owns_textures)
			delete t;
		color_textures[i] = NULL;
	}

	if (depth_texture && owns_textures)
		delete depth_texture;
	depth_texture = NULL;

	if (renderbuffer_color)
		glDeleteRenderbuffers(1, &renderbuffer_color);
	if (renderbuffer_depth)
		glDeleteRenderbuffers(1, &renderbuffer_depth);

	renderbuffer_color = renderbuffer_depth = 0;
	width = height = 0;
	owns_textures = false;
}

bool FBO::create( int width, int height, int num_textures, int format, int type, bool use_depth_texture)
{
	assert(glGetError() == GL_NO_ERROR);
	assert(width && height);
	assert(num_textures < 5); //too many
	freeTextures();

	num_color_textures = num_textures;

	std::vector<Texture*> textures(4);
	for (int i = 0; i < num_textures; ++i)
	{
		Texture* colortex = textures[i] = new Texture(width, height, format, type, false); //,NULL, format == GL_RGBA ? GL_RGBA8 : GL_RGB8 
		glBindTexture(colortex->texture_type, colortex->texture_id);	//we activate this id to tell opengl we are going to use this texture
		glTexParameteri(colortex->texture_type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);	//set the min filter
		glTexParameteri(colortex->texture_type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);   //set the mag filter
		glTexParameteri(colortex->texture_type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(colortex->texture_type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	//is using a depth_texture slower than using a renderbuffer?
	//https://stackoverflow.com/questions/45320836/why-is-depth-buffers-faster-than-depth-textures
	Texture* depth_texture = NULL;
	if(use_depth_texture)
		depth_texture = new Texture(width, height, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, false);
	owns_textures = true;
	return setTextures(textures, depth_texture);
}

bool FBO::setTexture(Texture* texture, int cubemap_face )
{
	std::vector<Texture*> textures;
	if(texture->format == GL_DEPTH_COMPONENT)
		setTextures(textures, texture, cubemap_face);
	else
	{
		textures.push_back(texture);
		setTextures(textures, NULL, cubemap_face);
	}
	return true;
}

bool FBO::setTextures(std::vector<Texture*> textures, Texture* depth_texture, int cubemap_face)
{
	assert(textures.size() >= 0 && textures.size() <= 4);
	assert(glGetError() == GL_NO_ERROR);
	assert(textures.size() || depth_texture ); //at least one texture
	int format = 0; //RGB,RGBA
	int type = 0;//UNSIGNED_BYTE
	if (textures.size())
	{
		width = (int)textures[0]->width;
		height = (int)textures[0]->height;
		format = (int)textures[0]->format;
		type = (int)textures[0]->type;
	}
	else
	{
		width = depth_texture->width;
		height = depth_texture->height;
	}

	//create and bind FBO
	if(fbo_id == 0)
		glGenFramebuffersEXT(1, &fbo_id);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_id);
	checkGLErrors();

	if (depth_texture)
	{
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture->texture_id, 0);
		this->depth_texture = depth_texture;
	}
	else
	{
		if (!renderbuffer_depth)
			glGenRenderbuffers(1, &renderbuffer_depth);
		glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer_depth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer_depth);
	}
	checkGLErrors();

	//reset the buffer to store color attachments
	memset(bufs, 0, sizeof(bufs));

	num_color_textures = 0;
	for (int i = 0; i < 4; ++i)
	{
		Texture* texture = i < textures.size() ? textures[i] : NULL;
		assert(!texture || (texture->width == width && texture->height == height)); //incorrect size, textures must have same size
		assert(!texture || (texture->type == type && texture->format == format)); //incorrect texture format

		if (texture && texture->texture_type == GL_TEXTURE_CUBE_MAP)
		{
			assert(cubemap_face != -1); //MUST SPECIFY CUBEMAP FACE
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + i, GL_TEXTURE_CUBE_MAP_POSITIVE_X + cubemap_face, texture ? texture->texture_id : NULL, 0);
		}
		else
        {
            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + i, GL_TEXTURE_2D, texture ? texture->texture_id : NULL, 0);
        }
        if(texture)
            bufs[i] = GL_COLOR_ATTACHMENT0_EXT + i;
        else
            bufs[i] = GL_NONE;
		color_textures[i] = texture;
		if (texture)
			num_color_textures++;
	}

	//add a render buffer for the color
	if (num_color_textures == 0)
	{
		if(!renderbuffer_color)
			glGenRenderbuffersEXT(1, &renderbuffer_color);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderbuffer_color);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGB, width, height);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER_EXT, renderbuffer_color);
		bufs[0] = GL_COLOR_ATTACHMENT0_EXT;
	}
    
    glDrawBuffers(4, bufs);

	checkGLErrors();

	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		std::cout << "Error: Framebuffer object is not completed: " << status << std::endl;
		assert(0);
		return false;
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	checkGLErrors();
	return true;
}

bool FBO::setDepthOnly(int width, int height)
{
	owns_textures = true;
	memset(bufs, 0, sizeof(bufs));
	num_color_textures = 0;

	glGenFramebuffersEXT(1, &fbo_id);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_id);

	glGenRenderbuffersEXT(1, &renderbuffer_color);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderbuffer_color);

	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA, width, height);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER_EXT, renderbuffer_color);

	//create texture
	depth_texture = new Texture(width, height, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, false);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture->texture_id, 0);

	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		std::cout << "Error: Framebuffer object is not completed" << std::endl;
		return false;
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	return true;
}

void FBO::bind()
{
	assert(glGetError() == GL_NO_ERROR);
	Texture* tex = color_textures[0] ? color_textures[0] : depth_texture;
	assert(tex && "framebuffer without texture");
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_id);
	checkGLErrors();
	glPushAttrib(GL_VIEWPORT_BIT);
	glDrawBuffers(4, bufs);
	glViewport(0, 0, (int)tex->width, (int)tex->height);
	assert(glGetError() == GL_NO_ERROR);
}

GLenum one_buffer = GL_BACK;

void FBO::unbind()
{
	// output goes to the FBO and it’s attached buffers
	glPopAttrib();
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	//glDrawBuffers(1, &one_buffer);
	assert(glGetError() == GL_NO_ERROR);
}

void FBO::enableSingleBuffer(int num)
{
	assert(num < this->num_color_textures);
    GLenum DrawBuffers[1] = {static_cast<GLenum>( (int)GL_COLOR_ATTACHMENT0) + num };
	glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
}

void FBO::enableAllBuffers()
{
	glDrawBuffers(4, bufs);
}


/*
 glGenFramebuffers(1, &FramebufferName);
 glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
 GLuint renderedTexture;
 glGenTextures(1, &renderedTexture);
 glBindTexture(GL_TEXTURE_2D, renderedTexture);
 glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, 1024, 768, 0,GL_RGB, GL_UNSIGNED_BYTE, 0);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 // The depth buffer
 GLuint depthrenderbuffer;
 glGenRenderbuffers(1, &depthrenderbuffer);
 glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
 glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 1024, 768);
 glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);
 glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);
 GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
 glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
 if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
 exit(1);
 */
