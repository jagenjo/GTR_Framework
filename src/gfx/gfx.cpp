#include "gfx.h"

#include "../pipeline/camera.h"
#include "../gfx/shader.h"
#include "../gfx/mesh.h"
#include "../gfx/texture.h"
#include "../extra/stb_easy_font.h"

namespace GFX {

	long gpu_frame_microseconds = 0;
	long gpu_frame_microseconds_history[GPU_FRAME_HISTORY_SIZE];

	void startGPULabel(const char* text)
	{
		//glPushDebugGroup(GL_DEBUG_SOURCE_THIRD_PARTY, 1, -1, text);
	}

	void endGPULabel()
	{
		//glPopDebugGroup();
	}


#define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX 0x9048
#define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX 0x9049

	std::string getGPUStats()
	{
		GLint nTotalMemoryInKB = 0;
		glGetIntegerv(GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX, &nTotalMemoryInKB);
		GLint nCurAvailMemoryInKB = 0;
		glGetIntegerv(GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX, &nCurAvailMemoryInKB);
		if (glGetError() != GL_NO_ERROR) //unsupported feature by driver
		{
			nTotalMemoryInKB = 0;
			nCurAvailMemoryInKB = 0;
		}

		std::string str = "FPS: " + std::to_string(CORE::BaseApplication::instance->fps) + " Time: " + std::to_string(gpu_frame_microseconds) + "us DCS: " + std::to_string(Mesh::num_meshes_rendered) + " Tris: " + std::to_string(long(Mesh::num_triangles_rendered * 0.001)) + "Ks  VRAM: " + std::to_string(int((nTotalMemoryInKB - nCurAvailMemoryInKB) * 0.001)) + "MBs / " + std::to_string(int(nTotalMemoryInKB * 0.001)) + "MBs";
		Mesh::num_meshes_rendered = 0;
		Mesh::num_triangles_rendered = 0;
		return str;
	}

	bool checkGLErrors()
	{
#ifndef _DEBUG
		return true;
#endif

		GLenum errCode;
		const GLubyte* errString;

		if ((errCode = glGetError()) != GL_NO_ERROR) {
#ifndef GCC
			errString = gluErrorString(errCode);
			std::cerr << "OpenGL Error: " << (errString ? (const char*)errString : "NO ERROR STRING") << std::endl;
#endif
			assert(0);
			return false;
		}

		return true;
	}


	Mesh* grid = NULL;

	void drawGrid()
	{
		if (!grid)
		{
			grid = new Mesh();
			grid->createGrid(10);
		}

		glLineWidth(1);
		glEnable(GL_BLEND);
		glDepthMask(false);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		Shader* grid_shader = Shader::getDefaultShader("grid");
		grid_shader->enable();
		Matrix44 m;
		m.translate(floor(Camera::current->eye.x / 100.0f) * 100.0f, 0.0f, floor(Camera::current->eye.z / 100.0f) * 100.0f);
		grid_shader->setUniform("u_color", Vector4f(0.7f, 0.7f, 0.7f, 0.7f));
		grid_shader->setUniform("u_model", m);
		grid_shader->setUniform("u_camera_position", Camera::current->eye);
		grid_shader->setUniform("u_viewprojection", Camera::current->viewprojection_matrix);
		grid->render(GL_LINES); //background grid
		glDisable(GL_BLEND);
		glDepthMask(true);
		grid_shader->disable();
	}

	bool drawText3D(Vector3f pos, std::string text, Vector4f c, float scale)
	{
		Vector2f size = CORE::getWindowSize();
		Vector3f p = Camera::current->project(pos, size.x, size.y);
		if (p.z > 1)
			return true;
		drawText(p.x, size.y - p.y, text, c, scale);
		return true;
	}

	bool drawText(float x, float y, std::string text, Vector4f c, float scale)
	{
		static char buffer[99999]; // ~500 chars
		int num_quads;

		if (scale == 0)
			return true;

		x /= scale;
		y /= scale;

		if (Shader::current)
			Shader::current->disable();

		num_quads = stb_easy_font_print(x, y, (char*)(text.c_str()), NULL, buffer, sizeof(buffer));

		Matrix44 projection_matrix;
		Vector2f size = CORE::getWindowSize();
		projection_matrix.ortho(0, size.x / scale, size.y / scale, 0, -1, 1);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		Shader* shader = Shader::getDefaultShader("flat2D");
		shader->enable();
		shader->setUniform("u_viewprojection", projection_matrix);
		shader->setUniform("u_color", c);

		int loc = shader->getAttribLocation("a_vertex");
		glEnableVertexAttribArray(loc);
		glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 16, buffer);
		glDrawArrays(GL_QUADS, 0, num_quads * 4);
		glEnableVertexAttribArray(0);

		return true;
	}

	void drawTexture2D(Texture* tex, vec4 pos)
	{
		glDisable(GL_DEPTH_TEST);
		glPushAttrib(GL_VIEWPORT_BIT);
		glViewport(pos.x, pos.y, pos.z, pos.w);

		GFX::Shader* sh = nullptr;
		if (tex->format == GL_DEPTH_COMPONENT)
		{
			sh = Shader::getDefaultShader("linear_depth");
			sh->enable();
			sh->setUniform("u_camera_nearfar",tex->near_far);
		}

		tex->toViewport();
		glPopAttrib();
	}

	void drawPoints(std::vector<Vector3f> points, Vector4f color, int size)
	{
		if (!points.size())
			return;
		Camera* camera = Camera::current;
		assert(camera);
		GFX::Mesh m;
		m.vertices = points;
		GFX::Shader* sh = GFX::Shader::getDefaultShader("flat");
		sh->enable();
		sh->setUniform("u_color", color);
		sh->setUniform("u_model", Matrix44());
		glPointSize(size);
		sh->setUniform("u_viewprojection", camera->viewprojection_matrix );
		sh->setUniform("u_camera_position", camera->eye );
		m.render(GL_POINTS);
	}

	void displaceMesh(Mesh* mesh, ::Image* heightmap, float altitude)
	{
		assert(heightmap && heightmap->data && "image without data");
		assert(mesh->uvs.size() && "cannot displace without uvs");

		bool is_interleaved = mesh->interleaved.size() != 0;
		int num = is_interleaved ? mesh->interleaved.size() : mesh->vertices.size();
		assert(num && "no vertices found");

		for (int i = 0; i < num; ++i)
		{
			Vector2f& uv = mesh->uvs[i];
			Color c = heightmap->getPixelInterpolated(uv.x * heightmap->width, uv.y * heightmap->height);
			if (is_interleaved)
				mesh->interleaved[i].vertex.y = (c.x / 255.0f) * altitude;
			else
				mesh->vertices[i].y = (c.x / 255.0f) * altitude;
		}
		mesh->box.center.y += altitude * 0.5f;
		mesh->box.halfsize.y += altitude * 0.5f;
		mesh->radius = mesh->box.halfsize.length();
	}

	//GPU QUERYs 

	GPUQuery::GPUQuery(GLuint t) { type = t; handler = 0; value = 0; waiting = false; }
	GPUQuery::~GPUQuery() { if (handler) glDeleteQueries(1, &handler); }

	void GPUQuery::start()
	{
		GFX::checkGLErrors();
		if (!handler)
			glGenQueries(1, &handler);
		glBeginQuery(GL_TIME_ELAPSED, handler);
		waiting = true;
		GFX::checkGLErrors();
	}

	void GPUQuery::finish()
	{
		if (!handler)
			return;
		glEndQuery(GL_TIME_ELAPSED);
	}

	bool GPUQuery::isReady()
	{
		if (!handler)
			return false;
		int available = 0;
		GFX::checkGLErrors();
		glGetQueryObjectiv(handler, GL_QUERY_RESULT_AVAILABLE, &available);
		GFX::checkGLErrors();
		if(available)
			glGetQueryObjectui64v(handler, GL_QUERY_RESULT, &value);
		GFX::checkGLErrors();
		waiting = available == 0;
		return available != 0;
	}
};

/*
uint64 gpu_current_state = GFX_STATE_DEFAULT;

void setGPUState(uint64 state)
{
	//TODO
}
*/

