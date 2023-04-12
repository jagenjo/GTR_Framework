#include "mesh.h"
#include "../extra/textparser.h"
#include "../utils/utils.h"
#include "shader.h"
#include "../core/includes.h"
#include "math.h"
#include "gfx.h"

#include <cassert>
#include <iostream>
#include <limits>
#include <sys/stat.h>

#include "../pipeline/camera.h" //??
#include "texture.h"
//#include "animation.h"
#include "../extra/coldet/coldet.h"

//#include "engine/application.h"

namespace GFX {

bool Mesh::use_binary = false;			//checks if there is .wbin, it there is one tries to read it instead of the other file
bool Mesh::auto_upload_to_vram = true;	//uploads the mesh to the GPU VRAM to speed up rendering
bool Mesh::interleave_meshes = true;	//places the geometry in an interleaved array

std::map<std::string, Mesh*> Mesh::sMeshesLoaded;
long Mesh::num_meshes_rendered = 0;
long Mesh::num_triangles_rendered = 0;
uint32 Mesh::s_last_index = 0;

#define FORMAT_ASE 1
#define FORMAT_OBJ 2
#define FORMAT_MBIN 3
#define FORMAT_MESH 4

Mesh::Mesh()
{
	index = s_last_index++;
	radius = 0;
	vertices_vbo_id = uvs_vbo_id = uvs1_vbo_id = normals_vbo_id = colors_vbo_id = interleaved_vbo_id = indices_vbo_id = bones_vbo_id = weights_vbo_id = 0;
	collision_model = NULL;

	clear();
}

Mesh::~Mesh()
{
	clear();
}


void Mesh::clear()
{
	//Free VBOs
	#ifdef USE_OPENGL_EXT
		if (vertices_vbo_id)
			glDeleteBuffersARB(1,&vertices_vbo_id);
		if (uvs_vbo_id)
			glDeleteBuffersARB(1,&uvs_vbo_id);
		if (normals_vbo_id)
			glDeleteBuffersARB(1,&normals_vbo_id);
		if (colors_vbo_id)
			glDeleteBuffersARB(1,&colors_vbo_id);
		if (interleaved_vbo_id)
			glDeleteBuffersARB(1, &interleaved_vbo_id);
		if (indices_vbo_id)
			glDeleteBuffersARB(1, &indices_vbo_id);
		if (bones_vbo_id)
			glDeleteBuffersARB(1, &bones_vbo_id);
		if (weights_vbo_id)
			glDeleteBuffersARB(1, &weights_vbo_id);
		if (uvs1_vbo_id)
			glDeleteBuffersARB(1, &uvs1_vbo_id);
    #else
	if (vertices_vbo_id)
		glDeleteBuffers(1,&vertices_vbo_id);
	if (uvs_vbo_id)
		glDeleteBuffers(1,&uvs_vbo_id);
	if (normals_vbo_id)
		glDeleteBuffers(1,&normals_vbo_id);
	if (colors_vbo_id)
		glDeleteBuffers(1,&colors_vbo_id);
	if (interleaved_vbo_id)
		glDeleteBuffers(1, &interleaved_vbo_id);
	if (indices_vbo_id)
		glDeleteBuffers(1, &indices_vbo_id);
	if (bones_vbo_id)
		glDeleteBuffers(1, &bones_vbo_id);
	if (weights_vbo_id)
		glDeleteBuffers(1, &weights_vbo_id);
	if (uvs1_vbo_id)
		glDeleteBuffers(1, &uvs1_vbo_id);
    #endif


	//VBOs ids
	vertices_vbo_id = uvs_vbo_id = normals_vbo_id = colors_vbo_id = interleaved_vbo_id = indices_vbo_id = weights_vbo_id = bones_vbo_id = uvs1_vbo_id = 0;

	//buffers
	vertices.clear();
	normals.clear();
	uvs.clear();
	colors.clear();
	interleaved.clear();
	m_indices.clear();
	bones.clear();
	weights.clear();
	m_uvs1.clear();

	if (collision_model)
		delete (CollisionModel3D*)collision_model;
}

int vertex_location = -1;
int normal_location = -1;
int uv_location = -1;
int uv1_location = -1;
int color_location = -1;
int bones_location = -1;
int weights_location = -1;

void Mesh::enableBuffers(Shader* sh)
{
	vertex_location = sh->getAttribLocation("a_vertex");
	/*
	assert(vertex_location != -1 && "No a_vertex found in shader");
	if (vertex_location == -1)
		return;
	*/

	int spacing = 0;
	int offset_normal = 0;
	int offset_uv = 0;

	if (interleaved.size())
	{
		spacing = sizeof(tInterleaved);
		offset_normal = sizeof(Vector3f);
		offset_uv = sizeof(Vector3f) + sizeof(Vector3f);
	}

	if (vertex_location != -1)
	{
		glEnableVertexAttribArray(vertex_location);
		if (vertices_vbo_id || interleaved_vbo_id)
		{
			glBindBuffer(GL_ARRAY_BUFFER, interleaved_vbo_id ? interleaved_vbo_id : vertices_vbo_id);
			glVertexAttribPointer(vertex_location, 3, GL_FLOAT, GL_FALSE, spacing, 0);
		}
		else
			glVertexAttribPointer(vertex_location, 3, GL_FLOAT, GL_FALSE, spacing, interleaved.size() ? &interleaved[0].vertex : &vertices[0]);
		checkGLErrors();
	}

	normal_location = -1;
	if (normals.size() || spacing)
	{
		normal_location = sh->getAttribLocation("a_normal");
		if (normal_location != -1)
		{
			glEnableVertexAttribArray(normal_location);
			if (normals_vbo_id || interleaved_vbo_id)
			{
				glBindBuffer(GL_ARRAY_BUFFER, interleaved_vbo_id ? interleaved_vbo_id : normals_vbo_id);
				glVertexAttribPointer(normal_location, 3, GL_FLOAT, GL_FALSE, spacing, (void*)offset_normal);
			}
			else
				glVertexAttribPointer(normal_location, 3, GL_FLOAT, GL_FALSE, spacing, interleaved.size() ? &interleaved[0].normal : &normals[0]);
		}
		checkGLErrors();
	}

	uv_location = -1;
	if (uvs.size() || spacing)
	{
		uv_location = sh->getAttribLocation("a_coord");
		if (uv_location != -1)
		{
			glEnableVertexAttribArray(uv_location);
			if (uvs_vbo_id || interleaved_vbo_id)
			{
				glBindBuffer(GL_ARRAY_BUFFER, interleaved_vbo_id ? interleaved_vbo_id : uvs_vbo_id);
				glVertexAttribPointer(uv_location, 2, GL_FLOAT, GL_FALSE, spacing, (void*)offset_uv);
			}
			else
				glVertexAttribPointer(uv_location, 2, GL_FLOAT, GL_FALSE, spacing, interleaved.size() ? &interleaved[0].uv : &uvs[0]);
		}
		checkGLErrors();
	}

	uv1_location = -1;
	if (m_uvs1.size())
	{
		uv1_location = sh->getAttribLocation("a_coord1");
		if (uv1_location != -1)
		{
			glEnableVertexAttribArray(uv1_location);
			if (uvs1_vbo_id)
			{
				glBindBuffer(GL_ARRAY_BUFFER, uvs1_vbo_id);
				glVertexAttribPointer(uv1_location, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
			}
			else
				glVertexAttribPointer(uv1_location, 2, GL_FLOAT, GL_FALSE, 0, &m_uvs1[0]);
		}
		checkGLErrors();
	}

	color_location = -1;
	if (colors.size())
	{
		color_location = sh->getAttribLocation("a_color");
		if (color_location != -1)
		{
			glEnableVertexAttribArray(color_location);
			if (colors_vbo_id)
			{
				glBindBuffer(GL_ARRAY_BUFFER, colors_vbo_id);
				glVertexAttribPointer(color_location, 4, GL_FLOAT, GL_FALSE, 0, NULL);
			}
			else
				glVertexAttribPointer(color_location, 4, GL_FLOAT, GL_FALSE, 0, &colors[0]);
		}
		checkGLErrors();
	}

	bones_location = -1;
	if (bones.size())
	{
		bones_location = sh->getAttribLocation("a_bones");
		if (bones_location != -1)
		{
			glEnableVertexAttribArray(bones_location);
			if (bones_vbo_id)
			{
				glBindBuffer(GL_ARRAY_BUFFER, bones_vbo_id);
				glVertexAttribPointer(bones_location, 4, GL_UNSIGNED_BYTE, GL_FALSE, 0, NULL);
			}
			else
				glVertexAttribPointer(bones_location, 4, GL_UNSIGNED_BYTE, GL_FALSE, 0, &bones[0]);
		}
	}
	weights_location = -1;
	if (weights.size())
	{
		weights_location = sh->getAttribLocation("a_weights");
		if (weights_location != -1)
		{
			glEnableVertexAttribArray(weights_location);
			if (weights_vbo_id)
			{
				glBindBuffer(GL_ARRAY_BUFFER, weights_vbo_id);
				glVertexAttribPointer(weights_location, 4, GL_FLOAT, GL_FALSE, 0, NULL);
			}
			else
				glVertexAttribPointer(weights_location, 4, GL_FLOAT, GL_FALSE, 0, &weights[0]);
		}
	}

}

void Mesh::render(unsigned int primitive, int submesh_id, int num_instances)
{
    //return;

	Shader* shader = Shader::current;
	if (!shader || !shader->compiled)
	{
		assert(0 && "no shader or shader not compiled or enabled");
		return;
	}
	assert((interleaved.size() || vertices.size()) && "No vertices in this mesh");

	//bind buffers to attribute locations
	enableBuffers(shader);
	checkGLErrors();

	//draw call
	drawCall(primitive, submesh_id, num_instances);
	checkGLErrors();

	//unbind them
	disableBuffers(shader);
	checkGLErrors();
}

void Mesh::drawCall(unsigned int primitive, int submesh_id, int num_instances)
{
	int start = 0; //in primitives
	int size = (int)vertices.size();
	if (m_indices.size())
		size = (int)m_indices.size();
	else
	if (interleaved.size())
		size = (int)interleaved.size();

	if (submesh_id > -1)
	{
		assert(submesh_id < submeshes.size() && "this mesh doesnt have as many submeshes");
		sSubmeshInfo& submesh = submeshes[submesh_id];
		start = submesh.start;
		size = submesh.start + submesh.length;
	}

	//DRAW
	if (m_indices.size())
	{
		if (num_instances > 0)
		{
			assert(indices_vbo_id && "indices must be uploaded to the GPU");
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_vbo_id);
			#ifdef OPENGL_ES3
				glDrawElementsInstanced(primitive, size, GL_UNSIGNED_INT, (void*)(start + sizeof(Vector3u)), num_instances);
            #else
				assert(0 && "not supported in OpenGL ES2");
            #endif
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
		else
		{
			if (indices_vbo_id)
			{
				/*if (size != 90)*/ {
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_vbo_id);
					glDrawElements(primitive, size, GL_UNSIGNED_INT,(void *) (start * sizeof(Vector3u)));
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
				}
				checkGLErrors();
			}
			else
				glDrawElements(primitive, size, GL_UNSIGNED_INT, (void*)(&m_indices[0] + start)); //no multiply, its a vector3u pointer)
		}
	}
	else
	{
		if (num_instances > 0)
		{
			#ifdef OPENGL_ES3
				glDrawArraysInstanced(primitive, start, size, num_instances);
            #else
				assert(0 && "not supported in OpenGL ES2");
            #endif
		}
		else
			glDrawArrays(primitive, start, size);
	}

	num_triangles_rendered += (size / 3) * (num_instances ? num_instances : 1);
	num_meshes_rendered++;
}

void Mesh::disableBuffers(Shader* shader)
{
	if (vertex_location != -1) glDisableVertexAttribArray(vertex_location);
	if (normal_location != -1) glDisableVertexAttribArray(normal_location);
	if (uv_location != -1) glDisableVertexAttribArray(uv_location);
	if (uv1_location != -1) glDisableVertexAttribArray(uv1_location);
	if (color_location != -1) glDisableVertexAttribArray(color_location);
	if (bones_location != -1) glDisableVertexAttribArray(bones_location);
	if (weights_location != -1) glDisableVertexAttribArray(weights_location);
	glBindBuffer(GL_ARRAY_BUFFER, 0);    //if it crashes here, COMMENT THIS LINE ****************************
	checkGLErrors();
}

GLuint instances_buffer_id = 0;

//should be faster but in some system it is slower
void Mesh::renderInstanced(unsigned int primitive, const Matrix44* instanced_models, int num_instances)
{
	if (!num_instances)
		return;

	#ifdef OPENGL_ES3
		Shader* shader = Shader::current;
		assert(shader && "shader must be enabled");

		if (instances_buffer_id == 0)
			glGenBuffersARB(1, &instances_buffer_id);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, instances_buffer_id);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, num_instances * sizeof(Matrix44), instanced_models, GL_STREAM_DRAW_ARB);

		int attribLocation = shader->getAttribLocation("u_model");
		assert(attribLocation != -1 && "shader must have attribute mat4 u_model (not a uniform)");
		if (attribLocation == -1)
			return; //this shader doesnt support instanced model

		//mat4 count as 4 different attributes of vec4... (thanks opengl...)
		for (int k = 0; k < 4; ++k)
		{
			glEnableVertexAttribArray(attribLocation + k );
			int offset = sizeof(float) * 4 * k;
			const Uint8* addr = (Uint8*) offset;
			glVertexAttribPointer(attribLocation + k, 4, GL_FLOAT, false, sizeof(Matrix44), addr);
			glVertexAttribDivisor(attribLocation + k, 1); // This makes it instanced!
		}

		//regular render
		render(primitive, 0, num_instances);

		//disable instanced attribs
		for (int k = 0; k < 4; ++k)
		{
			glDisableVertexAttribArray(attribLocation + k);
			glVertexAttribDivisor(attribLocation + k, 0);
		}
    #else
		assert(0 && "not supported");
    #endif
}

//super obsolete rendering method, do not use
/*
void Mesh::renderFixedPipeline(int primitive)
{
	assert((vertices.size() || interleaved.size()) && "No vertices in this mesh");

	int interleave_offset = interleaved.size() ? sizeof(tInterleaved) : 0;
	int offset_normal = sizeof(Vector3);
	int offset_uv = sizeof(Vector3) + sizeof(Vector3);

	glEnableClientState(GL_VERTEX_ARRAY);

	if (vertices_vbo_id || interleaved_vbo_id)
	{
		glBindBuffer(GL_ARRAY_BUFFER, interleave_offset ? interleaved_vbo_id : vertices_vbo_id);
		glVertexPointer(3, GL_FLOAT, interleave_offset, 0);
	}
	else
		glVertexPointer(3, GL_FLOAT, interleave_offset, interleave_offset ? &interleaved[0].vertex : &vertices[0]);

	if (normals.size() || interleave_offset)
	{
		glEnableClientState(GL_NORMAL_ARRAY);
		if (normals_vbo_id || interleaved_vbo_id)
		{
			glBindBuffer(GL_ARRAY_BUFFER, interleaved_vbo_id ? interleaved_vbo_id : normals_vbo_id);
			glNormalPointer(GL_FLOAT, interleave_offset, (void*)offset_normal);
		}
		else
			glNormalPointer(GL_FLOAT, interleave_offset, interleave_offset ? &interleaved[0].normal : &normals[0]);
	}

	if (uvs.size() || interleave_offset)
	{
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		if (uvs_vbo_id || interleaved_vbo_id)
		{
			glBindBuffer(GL_ARRAY_BUFFER, interleaved_vbo_id ? interleaved_vbo_id : uvs_vbo_id);
			glTexCoordPointer(2, GL_FLOAT, interleave_offset, (void*)offset_uv);
		}
		else
			glTexCoordPointer(2, GL_FLOAT, interleave_offset, interleave_offset ? &interleaved[0].uv : &uvs[0]);
	}

	if (colors.size())
	{
		glEnableClientState(GL_COLOR_ARRAY);
		if (colors_vbo_id)
		{
			glBindBuffer(GL_ARRAY_BUFFER, colors_vbo_id);
			glColorPointer(4, GL_FLOAT, 0, NULL);
		}
		else
			glColorPointer(4, GL_FLOAT, 0, &colors[0]);
	}

	int size = (int)vertices.size();
	if (!size)
		size = (int)interleaved.size();

	glDrawArrays(primitive, 0, (GLsizei)size);
	glDisableClientState(GL_VERTEX_ARRAY);
	if (normals.size())
		glDisableClientState(GL_NORMAL_ARRAY);
	if (uvs.size())
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if (colors.size())
		glDisableClientState(GL_COLOR_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, 0); //if it crashes, comment this line
}
*/

/*
void Mesh::renderAnimated( unsigned int primitive, Skeleton* skeleton )
{
	Shader* shader = Shader::current;
	std::vector<Matrix44> bone_matrices;
	assert(bones.size());
	int bones_loc = shader->getUniformLocation("u_bones");
	if (bones_loc != -1)
	{
		skeleton->computeFinalBoneMatrices(bone_matrices, this);
		shader->setUniform("u_bones", bone_matrices );
	}

	render(primitive);
}
*/

#define glGenBuffersARB glGenBuffers
#define glBindBufferARB glBindBuffer
#define glBufferDataARB glBufferData
#define GL_ARRAY_BUFFER_ARB GL_ARRAY_BUFFER
#define GL_STATIC_DRAW_ARB GL_STATIC_DRAW

void Mesh::uploadToVRAM()
{
	assert(vertices.size() || interleaved.size());

	if (glGenBuffersARB == nullptr)
	{
		std::cout << "Error: your graphics cards dont support VBOs. Sorry." << std::endl;
		exit(0);
	}

	if (interleaved.size())
	{
		// Vertex,Normal,UV
		if (interleaved_vbo_id == 0)
			glGenBuffersARB(1, &interleaved_vbo_id);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, interleaved_vbo_id);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, interleaved.size() * sizeof(tInterleaved), &interleaved[0], GL_STATIC_DRAW_ARB);
	}
	else
	{
		// Vertices
		if (vertices_vbo_id == 0)
			glGenBuffersARB(1, &vertices_vbo_id);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, vertices_vbo_id);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, vertices.size() * sizeof(Vector3f), &vertices[0], GL_STATIC_DRAW_ARB);

		// UVs
		if (uvs.size())
		{
			if (uvs_vbo_id == 0)
				glGenBuffersARB(1, &uvs_vbo_id);
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, uvs_vbo_id);
			glBufferDataARB(GL_ARRAY_BUFFER_ARB, uvs.size() * sizeof(Vector2f), &uvs[0], GL_STATIC_DRAW_ARB);
		}

		// Normals
		if (normals.size())
		{
			if (normals_vbo_id == 0)
				glGenBuffersARB(1, &normals_vbo_id);
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, normals_vbo_id);
			glBufferDataARB(GL_ARRAY_BUFFER_ARB, normals.size() * sizeof(Vector3f), &normals[0], GL_STATIC_DRAW_ARB);
		}
	}

	// UVs
	if (m_uvs1.size())
	{
		if (uvs1_vbo_id == 0)
			glGenBuffersARB(1, &uvs1_vbo_id);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, uvs1_vbo_id);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, m_uvs1.size() * sizeof(Vector2f), &m_uvs1[0], GL_STATIC_DRAW_ARB);
	}

	// Colors
	if (colors.size())
	{
		if (colors_vbo_id == 0)
			glGenBuffersARB(1, &colors_vbo_id);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, colors_vbo_id);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, colors.size() * sizeof(Vector4f), &colors[0], GL_STATIC_DRAW_ARB);
	}

	if (bones.size())
	{
		if (bones_vbo_id == 0)
			glGenBuffersARB(1, &bones_vbo_id);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, bones_vbo_id);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, bones.size() * sizeof(Vector4ub), &bones[0], GL_STATIC_DRAW_ARB);
	}
	if (weights.size())
	{
		if (weights_vbo_id == 0)
			glGenBuffersARB(1, &weights_vbo_id);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, weights_vbo_id);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, weights.size() * sizeof(Vector4f), &weights[0], GL_STATIC_DRAW_ARB);
	}

	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

	// Indices
	if (m_indices.size())
	{
		if (indices_vbo_id == 0)
			glGenBuffersARB(1, &indices_vbo_id);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, indices_vbo_id);
		glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), &m_indices[0], GL_STATIC_DRAW_ARB);
	}
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);

	checkGLErrors();
	//clear buffers to save memory
}

bool Mesh::createCollisionModel(bool is_static)
{
	if (collision_model)
		return true;

	double time = getTime();
	std::cout << "Creating collision model for: " << this->name << " (" << (interleaved.size() ? interleaved.size() : vertices.size()) / 3 << ") ...";

	CollisionModel3D* collision_model = newCollisionModel3D(is_static);

	if (m_indices.size()) //indexed
	{
		collision_model->setTriangleNumber((int)m_indices.size() / 3);

		if (interleaved.size())
			for (unsigned int i = 0; i < m_indices.size(); i+=3)
			{
				auto v1 = interleaved[m_indices[i+0]];
				auto v2 = interleaved[m_indices[i+1]];
				auto v3 = interleaved[m_indices[i+2]];
				collision_model->addTriangle(v1.vertex.v, v2.vertex.v, v3.vertex.v);
			}
		else
		for (unsigned int i = 0; i < m_indices.size(); i+=3)
		{
			auto v1 = vertices[m_indices[i+0]];
			auto v2 = vertices[m_indices[i+1]];
			auto v3 = vertices[m_indices[i+2]];
			collision_model->addTriangle(v1.v, v2.v, v3.v);
		}
	}
	else if (interleaved.size()) //is interleaved
	{
		collision_model->setTriangleNumber(interleaved.size() / 3);
		for (unsigned int i = 0; i < interleaved.size(); i+=3)
		{
			auto v1 = interleaved[i];
			auto v2 = interleaved[i+1];
			auto v3 = interleaved[i+2];
			collision_model->addTriangle(v1.vertex.v, v2.vertex.v, v3.vertex.v);
		}
	}
	else if (vertices.size()) //non interleaved
	{
		collision_model->setTriangleNumber((int)vertices.size() / 3);
		for (unsigned int i = 0; i < (int)vertices.size(); i+=3)
		{
			auto v1 = vertices[i];
			auto v2 = vertices[i + 1];
			auto v3 = vertices[i + 2];
			collision_model->addTriangle(v1.v, v2.v, v3.v);
		}
	}
	else
	{
		assert(0 && "mesh without vertices, cannot create collision model");
		std::cout << "[ERROR]" << std::endl;
		return false;
	}
	collision_model->finalize();
	this->collision_model = collision_model;

	std::cout << "[OK] Time: " << (getTime() - time) * 0.001 << "sec" << std::endl;

	return true;
}

//help: model is the transform of the mesh, ray origin and direction, a Vector3 where to store the collision if found, a Vector3 where to store the normal if there was a collision, max ray distance in case the ray should go to infintiy, and in_object_space to get the collision point in object space or world space
bool Mesh::testRayCollision(Matrix44 model, Vector3f start, Vector3f front, Vector3f& collision, Vector3f& normal, float max_ray_dist, bool in_object_space )
{
	if (!this->collision_model)
	{
		//test first against bounding before creating collision model
		BoundingBox aabb = transformBoundingBox(model, box);
		if (!RayBoundingBoxCollision(aabb, start, front, collision))
			return false;

		if (!createCollisionModel())
			return false;
	}

	CollisionModel3D* collision_model = (CollisionModel3D*)this->collision_model;
	assert(collision_model && "CollisionModel3D must be created before using it, call createCollisionModel");

	collision_model->setTransform( model.m );
	if (collision_model->rayCollision( start.v , front.v, true,0.0, max_ray_dist) == false)
		return false;

	collision_model->getCollisionPoint( collision.v, in_object_space);

	float t1[9],t2[9];
	collision_model->getCollidingTriangles(t1,t2, in_object_space);

	Vector3f v1;
	Vector3f v2;
	v1=Vector3f(t1[3]-t1[0],t1[4]-t1[1],t1[5]-t1[2]);
	v2=Vector3f(t1[6]-t1[0],t1[7]-t1[1],t1[8]-t1[2]);
	v1.normalize();
	v2.normalize();
	normal = v1.cross(v2);

	return true;
}

bool Mesh::testSphereCollision(Matrix44 model, Vector3f center, float radius, Vector3f& collision, Vector3f& normal)
{
	if (!this->collision_model)
		if (!createCollisionModel())
			return false;

	CollisionModel3D* collision_model = (CollisionModel3D*)this->collision_model;
	assert(collision_model && "CollisionModel3D must be created before using it, call createCollisionModel");

	collision_model->setTransform(model.m);
	if (collision_model->sphereCollision(center.v, radius) == false)
		return false;

	collision_model->getCollisionPoint(collision.v, false);

	float t1[9], t2[9];
	collision_model->getCollidingTriangles(t1, t2, false);

	Vector3f v1;
	Vector3f v2;
	v1 = Vector3f(t1[3] - t1[0], t1[4] - t1[1], t1[5] - t1[2]);
	v2 = Vector3f(t1[6] - t1[0], t1[7] - t1[1], t1[8] - t1[2]);
	v1.normalize();
	v2.normalize();
	normal = v1.cross(v2);

	return true;
}

bool Mesh::interleaveBuffers()
{
	if (!vertices.size() || !normals.size() || !uvs.size())
		return false;

	assert(vertices.size() == normals.size() && normals.size() == uvs.size());

	interleaved.resize(vertices.size());

	for (unsigned int i = 0; i < vertices.size(); ++i)
	{
		interleaved[i].vertex = vertices[i];
		interleaved[i].normal = normals[i];
		interleaved[i].uv = uvs[i];
	}

	vertices.resize(0);
	normals.resize(0);
	uvs.resize(0);

	return true;
}

typedef struct 
{
	int version;
	int header_bytes;
	int size;
	int num_indices;
	Vector3f aabb_min;
	Vector3f	aabb_max;
	Vector3f	center;
	Vector3f	halfsize;
	float radius;
	int num_bones;
	int num_submeshes;
	Matrix44 bind_matrix;
	char streams[8]; //Vertex/Interlaved|Normal|Uvs|Color|Indices|Bones|Weights|Extra|Uvs1
	char extra[32]; //unused
} sMeshInfo;

bool Mesh::readBin(const char* filename)
{
	FILE *f;
	assert(filename);

	struct stat stbuffer;

	stat(filename,&stbuffer);
	f = fopen(filename,"rb");
	if (f == NULL)
		return false;

	unsigned int size = (unsigned int)stbuffer.st_size;
	char* data = new char[size];
	fread(data,size,1,f);
	fclose(f);

	//watermark
	if ( memcmp(data,"MBIN",4) != 0 )
	{
		std::cout << "[ERROR] loading BIN: invalid content: " << filename << std::endl;
		return false;
	}

	char* pos = data + 4;
	sMeshInfo info;
	memcpy(&info,pos,sizeof(sMeshInfo));
	pos += sizeof(sMeshInfo);

	if(info.version != MESH_BIN_VERSION || info.header_bytes != sizeof(sMeshInfo) )
	{
		std::cout << "[WARN] loading BIN: old version: " << filename << std::endl;
		return false;
	}

	if (info.streams[0] == 'I')
	{
		interleaved.resize(info.size);
		memcpy((void*)&interleaved[0], pos, sizeof(tInterleaved) * info.size);
		pos += sizeof(tInterleaved) * info.size;
	}
	else if (info.streams[0] == 'V')
	{
		vertices.resize(info.size);
		memcpy((void*)&vertices[0], pos, sizeof(Vector3f) * info.size);
		pos += sizeof(Vector3f) * info.size;
	}

	if (info.streams[1] == 'N')
	{
		normals.resize(info.size);
		memcpy((void*)&normals[0],pos,sizeof(Vector3f) * info.size);
		pos += sizeof(Vector3f) * info.size;
	}

	if (info.streams[2] == 'U')
	{
		uvs.resize(info.size);
		memcpy((void*)&uvs[0],pos,sizeof(Vector2f) * info.size);
		pos += sizeof(Vector2f) * info.size;
	}

	if (info.streams[3] == 'C')
	{
		colors.resize(info.size);
		memcpy((void*)&colors[0],pos,sizeof(Vector4f) * info.size);
		pos += sizeof(Vector4f) * info.size;
	}

	if (info.streams[4] == 'I')
	{
		m_indices.resize(info.num_indices);
		memcpy((void*)&m_indices[0], pos, sizeof(unsigned int) * info.num_indices);
		pos += sizeof(Vector3u) * info.num_indices;
	}

	if (info.streams[5] == 'B')
	{
		bones.resize(info.size);
		memcpy((void*)&bones[0], pos, sizeof(Vector4ub) * info.size);
		pos += sizeof(Vector4ub) * info.size;
	}

	if (info.streams[6] == 'W')
	{
		weights.resize(info.size);
		memcpy((void*)&weights[0], pos, sizeof(Vector4f) * info.size);
		pos += sizeof(Vector4f) * info.size;
	}

	if (info.streams[7] == 'u')
	{
		m_uvs1.resize(info.size);
		memcpy((void*)&m_uvs1[0], pos, sizeof(Vector2f) * info.size);
		pos += sizeof(Vector2f) * info.size;
	}

	if (info.num_bones)
	{
		bones_info.resize(info.num_bones);
		memcpy((void*)&bones_info[0], pos, sizeof(BoneInfo) * info.num_bones);
		pos += sizeof(BoneInfo) * info.num_bones;
	}

	aabb_max = info.aabb_max;
	aabb_min = info.aabb_min;
	box.center = info.center;
	box.halfsize = info.halfsize;
	radius = info.radius;
	bind_matrix = info.bind_matrix;

	submeshes.resize(info.num_submeshes);
	memcpy(&submeshes[0], pos, sizeof(sSubmeshInfo) * info.num_submeshes);
	pos += sizeof(sSubmeshInfo) * info.num_submeshes;

	createCollisionModel();
	return true;
}

bool Mesh::writeBin(const char* filename)
{
	assert( vertices.size() || interleaved.size() );
	std::string s_filename = filename;
	s_filename += ".mbin";

	FILE* f = fopen(s_filename.c_str(),"wb");
	if (f == NULL)
	{
		std::cout << "[ERROR] cannot write mesh BIN: " << s_filename.c_str() << std::endl;
		return false;
	}

	//watermark
	fwrite("MBIN",sizeof(char),4,f);

	sMeshInfo info;
	memset(&info, 0, sizeof(info));
	info.version = MESH_BIN_VERSION;
	info.header_bytes = sizeof(sMeshInfo);
	info.size = interleaved.size() ? interleaved.size() : vertices.size();
	info.num_indices = m_indices.size();
	info.aabb_max = aabb_max;
	info.aabb_min = aabb_min;
	info.center = box.center;
	info.halfsize = box.halfsize;
	info.radius = radius;
	info.num_bones = bones_info.size();
	info.bind_matrix = bind_matrix;
	info.num_submeshes = submeshes.size();

	info.streams[0] = interleaved.size() ? 'I' : 'V';
	info.streams[1] = normals.size() ? 'N' : ' ';
	info.streams[2] = uvs.size() ? 'U' : ' ';
	info.streams[3] = colors.size() ? 'C' : ' ';
	info.streams[4] = m_indices.size() ? 'I' : ' ';
	info.streams[5] = bones.size() ? 'B' : ' ';
	info.streams[6] = weights.size() ? 'W' : ' ';
	info.streams[7] = m_uvs1.size() ? 'u' : ' '; //uv second set

	//write info
	fwrite((void*)&info, sizeof(sMeshInfo),1, f);

	//write streams
	if (interleaved.size())
		fwrite((void*)&interleaved[0], interleaved.size() * sizeof(tInterleaved), 1, f);
	else
	{
		fwrite((void*)&vertices[0], vertices.size() * sizeof(Vector3f), 1, f);
		if (normals.size())
			fwrite((void*)&normals[0], normals.size() * sizeof(Vector3f), 1, f);
		if (uvs.size())
			fwrite((void*)&uvs[0], uvs.size() * sizeof(Vector2f), 1, f);
	}

	if (colors.size())
		fwrite((void*)&colors[0], colors.size() * sizeof(Vector4f), 1, f);

	if (m_indices.size())
		fwrite((void*)&m_indices[0], m_indices.size() * sizeof(unsigned int), 1, f);

	if (bones.size())
		fwrite((void*)&bones[0], bones.size() * sizeof(Vector4ub), 1, f);
	if (weights.size())
		fwrite((void*)&weights[0], weights.size() * sizeof(Vector4f), 1, f);
	if (bones_info.size())
		fwrite((void*)&bones_info[0], bones_info.size() * sizeof(BoneInfo), 1, f);
	if (m_uvs1.size())
		fwrite((void*)&m_uvs1[0], m_uvs1.size() * sizeof(Vector2f), 1, f);

	fwrite((void*)&submeshes[0], submeshes.size() * sizeof(sSubmeshInfo), 1, f);

	fclose(f);
	return true;
}

bool Mesh::loadASE(const char* filename)
{
	int nVtx,nFcs;
	int count;
	int vId,aId,bId,cId;
	float vtxX,vtxY,vtxZ;
	float nX,nY,nZ;
	TextParser t;
	if (t.create(filename) == false)
		return false;

	t.seek("*MESH_NUMVERTEX");
	nVtx = t.getint();
	t.seek("*MESH_NUMFACES");
	nFcs = t.getint();

	normals.resize(nFcs*3);
	vertices.resize(nFcs*3);
	uvs.resize(nFcs*3);

	std::vector<Vector3f> unique_vertices;
	unique_vertices.resize(nVtx);

	const float max_float = 10000000;
	const float min_float = -10000000;
	aabb_min.set(max_float,max_float,max_float);
	aabb_max.set(min_float,min_float,min_float);

	//load unique vertices
	for(count=0;count<nVtx;count++)
	{
		t.seek("*MESH_VERTEX");
		vId = t.getint();
		vtxX=(float)t.getfloat();
		vtxY= (float)t.getfloat();
		vtxZ= (float)t.getfloat();
		Vector3f v(-vtxX,vtxZ,vtxY);
		unique_vertices[count] = v;
		aabb_min.setMin( v );
		aabb_max.setMax( v );
	}
	box.center = (aabb_max + aabb_min) * 0.5f;
	box.halfsize = (aabb_max - box.center);
	radius = (float)fmax( aabb_max.length(), aabb_min.length() );
	
	int prev_mat = 0;

	sSubmeshInfo submesh;
	memset(&submesh, 0, sizeof(submesh));

	//load faces
	for(count=0;count<nFcs;count++)
	{
		t.seek("*MESH_FACE");
		t.seek("A:");
		aId = t.getint();
		t.seek("B:");
		bId = t.getint();
		t.seek("C:");
		cId = t.getint();
		vertices[count*3 + 0] = unique_vertices[aId];
		vertices[count*3 + 1] = unique_vertices[bId];
		vertices[count*3 + 2] = unique_vertices[cId];

		t.seek("*MESH_MTLID");
		int current_mat = t.getint();
		if (current_mat != prev_mat)
		{
			submesh.length = count * 3 - submesh.start;
			submeshes.push_back( submesh );
			memset(&submesh, 0, sizeof(submesh));
			submesh.start = count * 3;
			prev_mat = current_mat;
		}
	}

	submesh.length = count * 3 - submesh.start;
	submeshes.push_back(submesh);

	t.seek("*MESH_NUMTVERTEX");
	nVtx = t.getint();
	std::vector<Vector2f> unique_uvs;
	unique_uvs.resize(nVtx);

	for(count=0;count<nVtx;count++)
	{
		t.seek("*MESH_TVERT");
		vId = t.getint();
		vtxX= (float)t.getfloat();
		vtxY= (float)t.getfloat();
		unique_uvs[count]=Vector2f(vtxX,vtxY);
	}

	t.seek("*MESH_NUMTVFACES");
	nFcs = t.getint();
	for(count=0;count<nFcs;count++)
	{
		t.seek("*MESH_TFACE");
		t.getint(); //num face
		uvs[count*3] = unique_uvs[ t.getint() ];
		uvs[count*3+1] = unique_uvs[ t.getint() ];
		uvs[count*3+2] = unique_uvs[ t.getint() ];
	}

	//normals
	for(count=0;count<nFcs;count++)
	{
		t.seek("*MESH_VERTEXNORMAL");
		aId = t.getint();
		nX = (float)t.getfloat();
		nY = (float)t.getfloat();
		nZ = (float)t.getfloat();
		normals[count*3]=Vector3f(-nX,nZ,nY);
		t.seek("*MESH_VERTEXNORMAL");
		aId = t.getint();
		nX = (float)t.getfloat();
		nY = (float)t.getfloat();
		nZ = (float)t.getfloat();
		normals[count*3+1]=Vector3f(-nX,nZ,nY);
		t.seek("*MESH_VERTEXNORMAL");
		aId = t.getint();
		nX = (float)t.getfloat();
		nY = (float)t.getfloat();
		nZ = (float)t.getfloat();
		normals[count*3+2]=Vector3f(-nX,nZ,nY);
	}

	return true;
}

bool Mesh::loadOBJ(const char* filename)
{
	std::string data;
	if(!readFile(filename,data))
		return false;
	char* pos = &data[0];
	char line[255];
	int i = 0;

	std::vector<Vector3f> indexed_positions;
	std::vector<Vector3f> indexed_normals;
	std::vector<Vector2f> indexed_uvs;

	const float max_float = 10000000;
	const float min_float = -10000000;
	aabb_min.set(max_float,max_float,max_float);
	aabb_max.set(min_float,min_float,min_float);

	unsigned int vertex_i = 0;

	sSubmeshInfo submesh_info;
	int last_submesh_vertex = 0;
	memset(&submesh_info, 0, sizeof(submesh_info));

	//parse file
	while(*pos != 0)
	{
		if (*pos == '\n') pos++;
		if (*pos == '\r') pos++;

		//read one line
		i = 0;
		while(i < 255 && pos[i] != '\n' && pos[i] != '\r' && pos[i] != 0) i++;
		memcpy(line,pos,i);
		line[i] = 0;
		pos = pos + i;

		//std::cout << "Line: \"" << line << "\"" << std::endl;
		if (*line == '#' || *line == 0) continue; //comment

		//tokenize line
		std::vector<std::string> tokens = tokenize(line," ");

		if (tokens.empty()) continue;

		if (tokens[0] == "v" && tokens.size() == 4)
		{
			Vector3f v((float)atof(tokens[1].c_str()), (float)atof(tokens[2].c_str()), (float)atof(tokens[3].c_str()) );
			indexed_positions.push_back(v);

			aabb_min.setMin( v );
			aabb_max.setMax( v );
		}
		else if (tokens[0] == "vt" && tokens.size() >= 3)
		{
			Vector2f v((float)atof(tokens[1].c_str()), 1.0 - (float)atof(tokens[2].c_str()) );
			indexed_uvs.push_back(v);
		}
		else if (tokens[0] == "vn" && tokens.size() == 4)
		{
			Vector3f v((float)atof(tokens[1].c_str()), (float)atof(tokens[2].c_str()), (float)atof(tokens[3].c_str()) );
			indexed_normals.push_back(v);
		}
		else if (tokens[0] == "s") //surface? it appears one time before the faces
		{
			//process mesh: ????
			//if (uvs.size() == 0 && indexed_uvs.size() )
			//	uvs.resize(1);
		}
		else if (tokens[0] == "usemtl") //surface? it appears one time before the faces
		{
			if (last_submesh_vertex != vertices.size())
			{
				submesh_info.length = vertices.size() - submesh_info.start;
				last_submesh_vertex = vertices.size();
				submeshes.push_back(submesh_info);
				memset(&submesh_info, 0, sizeof(submesh_info));
				strcpy(submesh_info.name, tokens[1].c_str());
				submesh_info.start = last_submesh_vertex;
			}
			else
				strcpy(submesh_info.material, tokens[1].c_str());
		}
		else if (tokens[0] == "g") //surface? it appears one time before the faces
		{
			if (last_submesh_vertex != vertices.size())
			{
				submesh_info.length = vertices.size() - submesh_info.start;
				last_submesh_vertex = vertices.size();
				submeshes.push_back(submesh_info);
				memset(&submesh_info, 0, sizeof(submesh_info));
				strcpy( submesh_info.name, tokens[1].c_str());
				submesh_info.start = last_submesh_vertex;
			}
		}
		else if (tokens[0] == "f" && tokens.size() >= 4)
		{
			Vector3f v1,v2,v3;
			v1.parseFromText( tokens[1].c_str(), '/' );

			for (unsigned int iPoly = 2; iPoly < tokens.size() - 1; iPoly++)
			{
				v2.parseFromText( tokens[iPoly].c_str(), '/' );
				v3.parseFromText( tokens[iPoly+1].c_str(), '/' );

				vertices.push_back( indexed_positions[ (unsigned int)(v1.x) -1 ] );
				vertices.push_back( indexed_positions[ (unsigned int)(v2.x) -1] );
				vertices.push_back( indexed_positions[ (unsigned int)(v3.x) -1] );
				//triangles.push_back( VECTOR_INDICES_TYPE(vertex_i, vertex_i+1, vertex_i+2) ); //not needed
				vertex_i += 3;

				if (indexed_uvs.size() > 0)
				{
					uvs.push_back( indexed_uvs[(unsigned int)(v1.y) -1] );
					uvs.push_back( indexed_uvs[(unsigned int)(v2.y) -1] );
					uvs.push_back( indexed_uvs[(unsigned int)(v3.y) -1] );
				}

				if (indexed_normals.size() > 0)
				{
					normals.push_back( indexed_normals[(unsigned int)(v1.z) -1] );
					normals.push_back( indexed_normals[(unsigned int)(v2.z) -1] );
					normals.push_back( indexed_normals[(unsigned int)(v3.z) -1] );
				}
			}
		}
	}

	box.center = (aabb_max + aabb_min) * 0.5f;
	box.halfsize = (aabb_max - box.center);
	radius = (float)fmax( aabb_max.length(), aabb_min.length() );

	submesh_info.length = vertices.size() - last_submesh_vertex;
	submeshes.push_back(submesh_info);
	return true;
}

/*

#define TINYOBJLOADER_IMPLEMENTATION
#include "extra/tiny_obj_loader.h"

bool Mesh::loadOBJTiny(const char* filename)
{
	std::string data;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn;
	std::string err;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, NULL, true);
	if (!ret)
		return false;

	std::vector<Vector3> individual_vertices;
	std::vector<Vector3> individual_normals;
	std::vector<Vector2> individual_uvs;

	individual_vertices.resize( attrib.vertices.size() / 3 );
	memcpy(&individual_vertices[0], &attrib.vertices[0], sizeof(float) * attrib.vertices.size() );

	individual_normals.resize(attrib.normals.size() / 3);
	memcpy(&individual_normals[0], &attrib.normals[0], sizeof(float) * attrib.normals.size());

	individual_uvs.resize(attrib.texcoords.size() / 2);
	memcpy( &individual_uvs[0], &attrib.texcoords[0], sizeof(float) * attrib.texcoords.size());

	for (int i = 0; i < shapes.size(); ++i)
	{
		auto shape = shapes[i];
		auto mesh = shape.mesh;
		int index = 0;
		for (int j = 0; j < mesh.indices.size(); j+=3)
		{
			//I need to pack shared V/N/UV I guess, so it will be complex
		}
	}

	return true;
}
*/

bool Mesh::loadMESH(const char* filename)
{
	struct stat stbuffer;

	FILE* f = fopen(filename, "rb");
	if (f == NULL)
	{
		std::cerr << "File not found: " << filename << std::endl;
		return false;
	}
	stat(filename, &stbuffer);

	unsigned int size = stbuffer.st_size;
	char* data = new char[size + 1];
	fread(data, size, 1, f);
	fclose(f);
	data[size] = 0;
	char* pos = data;
	char word[255];

	while (*pos)
	{
		char type = *pos;
		pos++;
		if (type == '-') //buffer
		{
			pos = fetchWord(pos, word);
			std::string str(word);
			if (str == "vertices")
				pos = fetchBufferVec3(pos, vertices);
			else if (str == "normals")
				pos = fetchBufferVec3(pos, normals);
			else if (str == "coords")
				pos = fetchBufferVec2(pos, uvs);
			else if (str == "colors")
				pos = fetchBufferVec4(pos, colors);
			else if (str == "bone_indices")
				pos = fetchBufferVec4ub(pos, bones);
			else if (str == "weights")
				pos = fetchBufferVec4(pos, weights);
			else
				pos = fetchEndLine(pos);
		}
		else if (type == '*') //buffer
		{
			pos = fetchWord(pos, word);
			pos = fetchBufferVec3u(pos, m_indices);
		}
		else if (type == '@') //info
		{
			pos = fetchWord(pos, word);
			std::string str(word);
			if (str == "bones")
			{
				pos = fetchWord(pos, word);
				bones_info.resize(atof(word));
				for (int j = 0; j < bones_info.size(); ++j)
				{
					pos = fetchWord(pos, word);
                    #if defined(GCC) || defined(__APPLE__) || defined( ANDROID ) || _PLATFORM_WASM
                        strcpy(bones_info[j].name, word);
                    #else
                        strcpy_s(bones_info[j].name, 32, word);
                    #endif
                    pos = fetchMatrix44(pos, bones_info[j].bind_pose);
				}
			}
			else if (str == "bind_matrix")
				pos = fetchMatrix44( pos, bind_matrix);
			else
				pos = fetchEndLine(pos);
		}
		else
			pos = fetchEndLine(pos);
	}

	delete[] data;

	return true;
}

void Mesh::createCube(Vector3f size)
{
	const float _verts[] = { -1, 1, -1, -1, -1, +1, -1, 1, 1,    -1, 1, -1, -1, -1, -1, -1, -1, +1,     1, 1, -1,  1, 1, 1,  1, -1, +1,     1, 1, -1,   1, -1, +1,   1, -1, -1,    -1, 1, 1,  1, -1, 1,  1, 1, 1,    -1, 1, 1, -1,-1,1,  1, -1, 1,    -1,1,-1, 1,1,-1,  1,-1,-1,   -1,1,-1, 1,-1,-1, -1,-1,-1,   -1,1,-1, 1,1,1, 1,1,-1,    -1,1,-1, -1,1,1, 1,1,1,    -1,-1,-1, 1,-1,-1, 1,-1,1,   -1,-1,-1, 1,-1,1, -1,-1,1 };
	const float _uvs[] = {       0,  1, 1, 0, 1, 1,			 	     0, 1,       0,  0,      1,  0,        0, 1,      1, 1,      1, 0,         0, 1,        1, 0,        0, 0,          0, 1, 1, 0, 1, 1,               0, 1,  0, 0,  1,  0,              0,1,  1,1, 1,0,              0,1,    1,0,    0,0,           0,0, 1,1, 1,0,           0,0,    0,1,   1,1,        0,0, 1,0, 1,1,              0,0, 1,1, 0,1 };

	vertices.resize( 6 * 2 * 3);
	uvs.resize(6 * 2 * 3);
	memcpy(&vertices[0], _verts, sizeof(Vector3f) * vertices.size());
	memcpy(&uvs[0], _uvs, sizeof(Vector2f) * uvs.size());
	for (int i = 0; i < vertices.size(); ++i)
		vertices[i] = vertices[i] * (size * 0.5f);

	box.center.set(0, 0, 0);
	box.halfsize = size * 0.5f;
	radius = (float)box.halfsize.length();
}

void Mesh::createSphere(float radius, float slices, float arcs)
{
	for (int i = 0; i < slices; ++i)
	{
		float u1 = (i / slices);
		float r_angle1 = u1 * M_PI*2;
		float u2 = ((i + 1) / slices);
		float r_angle2 = u2 * M_PI * 2;
		quat r1;
		r1.setAxisAngle(vec3(0, 1, 0), r_angle1);
		quat r2;
		r2.setAxisAngle(vec3(0, 1, 0), r_angle2);

		for (int j = -arcs/2; j < arcs/2; ++j)
		{
			float v1 = (j / arcs);
			float ang1 = v1 * M_PI;
			float v2 = ((j + 1) / arcs);
			float ang2 = v2 * M_PI;
			vec3 A(cos(ang1) * radius, sin(ang1) * radius,0.0);
			vec3 B(cos(ang2) * radius, sin(ang2) * radius, 0.0);
			vec3 P1 = r1.rotate(A);
			vec3 P2 = r2.rotate(A);
			vec3 P3 = r1.rotate(B);
			vec3 P4 = r2.rotate(B);
			vec3 N1 = normalize(P1);
			vec3 N2 = normalize(P2);
			vec3 N3 = normalize(P3);
			vec3 N4 = normalize(P4);
			vec2 UV1 = vec2(u1, v1 * 0.5 + 0.5);
			vec2 UV2 = vec2(u2, v1 * 0.5 + 0.5);
			vec2 UV3 = vec2(u1, v2 * 0.5 + 0.5);
			vec2 UV4 = vec2(u2, v2 * 0.5 + 0.5);

			vertices.push_back(P1);
			vertices.push_back(P3);
			vertices.push_back(P2);
			vertices.push_back(P2);
			vertices.push_back(P3);
			vertices.push_back(P4);

			normals.push_back(N1);
			normals.push_back(N3);
			normals.push_back(N2);
			normals.push_back(N2);
			normals.push_back(N3);
			normals.push_back(N4);

			uvs.push_back(UV1);
			uvs.push_back(UV3);
			uvs.push_back(UV2);
			uvs.push_back(UV2);
			uvs.push_back(UV3);
			uvs.push_back(UV4);
		}
	}

	box.center.set(0, 0, 0);
	box.halfsize = vec3(radius * sqrt(3.0)); //not sure
	this->radius = radius;
}


void Mesh::createWireBox()
{
	const float _verts[] = { -1,-1,-1,  1,-1,-1,  -1,1,-1,  1,1,-1, -1,-1,1,  1,-1,1, -1,1,1,  1,1,1,    -1,-1,-1, -1,1,-1, 1,-1,-1, 1,1,-1, -1,-1,1, -1,1,1, 1,-1,1, 1,1,1,   -1,-1,-1, -1,-1,1, 1,-1,-1, 1,-1,1, -1,1,-1, -1,1,1, 1,1,-1, 1,1,1 };
	vertices.resize(24);
	memcpy(&vertices[0], _verts, sizeof(Vector3f) * vertices.size());

	box.center.set(0, 0, 0);
	box.halfsize.set(1,1,1);
	radius = (float)box.halfsize.length();
}

void Mesh::createQuad(float center_x, float center_y, float w, float h, bool flip_uvs)
{
	vertices.clear();
	normals.clear();
	uvs.clear();
	colors.clear();

	//create six vertices (3 for upperleft triangle and 3 for lowerright)

	vertices.push_back(Vector3f(center_x + w*0.5f, center_y + h*0.5f, 0.0f));
	vertices.push_back(Vector3f(center_x - w*0.5f, center_y - h*0.5f, 0.0f));
	vertices.push_back(Vector3f(center_x + w*0.5f, center_y - h*0.5f, 0.0f));
	vertices.push_back(Vector3f(center_x - w*0.5f, center_y + h*0.5f, 0.0f));
	vertices.push_back(Vector3f(center_x - w*0.5f, center_y - h*0.5f, 0.0f));
	vertices.push_back(Vector3f(center_x + w*0.5f, center_y + h*0.5f, 0.0f));

	//texture coordinates
	uvs.push_back(Vector2f(1.0f, flip_uvs ? 0.0f : 1.0f));
	uvs.push_back(Vector2f(0.0f, flip_uvs ? 1.0f : 0.0f));
	uvs.push_back(Vector2f(1.0f, flip_uvs ? 1.0f : 0.0f));
	uvs.push_back(Vector2f(0.0f, flip_uvs ? 0.0f : 1.0f));
	uvs.push_back(Vector2f(0.0f, flip_uvs ? 1.0f : 0.0f));
	uvs.push_back(Vector2f(1.0f, flip_uvs ? 0.0f : 1.0f));

	//all of them have the same normal
	normals.push_back(Vector3f(0.0f, 0.0f, 1.0f));
	normals.push_back(Vector3f(0.0f, 0.0f, 1.0f));
	normals.push_back(Vector3f(0.0f, 0.0f, 1.0f));
	normals.push_back(Vector3f(0.0f, 0.0f, 1.0f));
	normals.push_back(Vector3f(0.0f, 0.0f, 1.0f));
	normals.push_back(Vector3f(0.0f, 0.0f, 1.0f));

	updateBoundingBox();
}


void Mesh::createPlane(float size)
{
	vertices.clear();
	normals.clear();
	uvs.clear();
	colors.clear();

	//create six vertices (3 for upperleft triangle and 3 for lowerright)

	vertices.push_back(Vector3f(size, 0, size));
	vertices.push_back(Vector3f(size, 0, -size));
	vertices.push_back(Vector3f(-size, 0, -size));
	vertices.push_back(Vector3f(-size, 0, size));
	vertices.push_back(Vector3f(size, 0, size));
	vertices.push_back(Vector3f(-size, 0, -size));

	//all of them have the same normal
	normals.push_back(Vector3f(0, 1, 0));
	normals.push_back(Vector3f(0, 1, 0));
	normals.push_back(Vector3f(0, 1, 0));
	normals.push_back(Vector3f(0, 1, 0));
	normals.push_back(Vector3f(0, 1, 0));
	normals.push_back(Vector3f(0, 1, 0));

	//texture coordinates
	uvs.push_back(Vector2f(1, 1));
	uvs.push_back(Vector2f(1, 0));
	uvs.push_back(Vector2f(0, 0));
	uvs.push_back(Vector2f(0, 1));
	uvs.push_back(Vector2f(1, 1));
	uvs.push_back(Vector2f(0, 0));

	box.center.set(0, 0, 0);
	box.halfsize.set(size, 0, size);
	radius = (float)box.halfsize.length();
}

void Mesh::createSubdividedPlane(float size, int subdivisions, bool centered )
{
	double isize = size / (double)(subdivisions);
	//float hsize = centered ? size * -0.5f : 0.0f;
	double iuv = 1 / (double)(subdivisions * size);
	float sub_size = 1.0f / subdivisions;
	vertices.clear();

	for (int x = 0; x < subdivisions; ++x)
	{
		for (int z = 0; z < subdivisions; ++z)
		{

			Vector2f offset(sub_size*z, sub_size*x);
			Vector3f offset2(isize*x, 0.0f, isize*z);

			vertices.push_back(Vector3f(isize, 0.0f, isize) + offset2);
			vertices.push_back(Vector3f(isize, 0.0f, 0.0f) + offset2);
			vertices.push_back(Vector3f(0.0f, 0.0f, 0.0f) + offset2);

			uvs.push_back(Vector2f(sub_size, sub_size) + offset);
			uvs.push_back(Vector2f(0.0f, sub_size) + offset);
			uvs.push_back(Vector2f(0.0f, 0.0f) + offset);

			vertices.push_back(Vector3f(isize, 0.0f, isize) + offset2);
			vertices.push_back(Vector3f(0.0f, 0.0f, 0.0f) + offset2);
			vertices.push_back(Vector3f(0.0f, 0.0f, isize) + offset2);

			uvs.push_back(Vector2f(sub_size, sub_size) + offset);
			uvs.push_back(Vector2f(0.0f, 0.0f) + offset);
			uvs.push_back(Vector2f(sub_size, 0.0f) + offset);
		}
	}
	if (centered)
		box.center.set(0.0f, 0.0f, 0.0f);
	else
		box.center.set(size*0.5f, 0.0f, size*0.5f);

	box.halfsize.set(size*0.5f, 0.0f, size*0.5f);
	radius = box.halfsize.length();
}

void Mesh::createGrid(float dist)
{
	int num_lines = 2000;
	Vector4f color(0.5f, 0.5f, 0.5f, 1.0f);

	for (float i = num_lines * -0.5f; i <= num_lines * 0.5f; ++i)
	{
		vertices.push_back(Vector3f(i*dist, 0.0f, dist * num_lines * -0.5f ));
		vertices.push_back(Vector3f(i*dist, 0.0f, dist * num_lines * +0.5f));
		vertices.push_back(Vector3f(dist * num_lines * 0.5f, 0.0f, i*dist));
		vertices.push_back(Vector3f(dist * num_lines * -0.5f, 0.0f, i*dist));

		Vector4f color = int(i) % 10 == 0 ? Vector4f(1.0f, 1.0f, 1.0f, 1.0f) : Vector4f(0.75f, 0.75f, 0.75f, 0.5f);
		colors.push_back(color);
		colors.push_back(color);
		colors.push_back(color);
		colors.push_back(color);
	}
}

void Mesh::updateBoundingBox()
{
	if (vertices.size())
	{
		aabb_max = aabb_min = vertices[0];
		for (int i = 1; i < vertices.size(); ++i)
		{
			aabb_min.setMin(vertices[i]);
			aabb_max.setMax(vertices[i]);
		}
	}
	else if (interleaved.size())
	{
		aabb_max = aabb_min = interleaved[0].vertex;
		for (int i = 1; i < vertices.size(); ++i)
		{
			aabb_min.setMin(interleaved[i].vertex);
			aabb_max.setMax(interleaved[i].vertex);
		}
	}
	box.center = (aabb_max + aabb_min) * 0.5f;
	box.halfsize = aabb_max - box.center;
}

Mesh* wire_box = NULL;

void Mesh::renderBounding( const Matrix44& model, bool world_bounding )
{
	if (!wire_box)
	{
		wire_box = new Mesh();
		wire_box->createWireBox();
		wire_box->uploadToVRAM();
	}

	Shader* sh = Shader::getDefaultShader("flat");
	sh->enable();
	sh->setUniform("u_viewprojection", Camera::current->viewprojection_matrix);

	Matrix44 matrix;
	matrix.translate(box.center.x, box.center.y, box.center.z);
	matrix.scale(box.halfsize.x, box.halfsize.y, box.halfsize.z);

	sh->setUniform("u_color", Vector4f(1, 1, 0, 1));
	sh->setUniform("u_model", matrix * model);
	wire_box->render(GL_LINES);

	if (world_bounding)
	{
		BoundingBox AABB = transformBoundingBox(model, box);
		matrix.setIdentity();
		matrix.translate(AABB.center.x, AABB.center.y, AABB.center.z);
		matrix.scale(AABB.halfsize.x, AABB.halfsize.y, AABB.halfsize.z);
		sh->setUniform("u_model", matrix);
		sh->setUniform("u_color", Vector4f(0, 1, 1, 1));
		wire_box->render(GL_LINES);
	}

	sh->disable();
}



Mesh* Mesh::getQuad()
{
	static Mesh* quad = NULL;
	if (!quad)
	{
		quad = new Mesh();
		quad->createQuad(0, 0, 2, 2, false);
		quad->uploadToVRAM();
	}
	return quad;
}

Mesh* Mesh::Get(const char* filename, bool skip_load)
{
	assert(filename);
	std::map<std::string, Mesh*>::iterator it = sMeshesLoaded.find(filename);
	if (it != sMeshesLoaded.end())
		return it->second;

	if (skip_load)
		return NULL;

	Mesh* m = new Mesh();
	std::string name = filename;

	//detect format
	char file_format = 0;
	std::string ext = name.substr(name.find_last_of(".")+1);
	if (ext == "ase" || ext == "ASE")
		file_format = FORMAT_ASE;
	else if (ext == "obj" || ext == "OBJ")
		file_format = FORMAT_OBJ;
	else if (ext == "mbin" || ext == "MBIN")
		file_format = FORMAT_MBIN;
	else if (ext == "mesh" || ext == "MESH")
		file_format = FORMAT_MESH;
	else 
	{
		//if (ext.size()) std::cerr << "Unknown mesh format: " << filename << std::endl;
		return NULL;
	}

	//stats
	double time = getTime();
	std::cout << " + Mesh loading: " << TermColor::YELLOW << filename << TermColor::DEFAULT << " ... ";
	std::string binfilename = filename;

	if (file_format != FORMAT_MBIN)
		binfilename = binfilename + ".mbin";

	//try loading the binary version
	if (use_binary && m->readBin(binfilename.c_str()) )
	{
		if (interleave_meshes && m->interleaved.size() == 0)
		{
			std::cout << "[INTERL] ";
			m->interleaveBuffers();
		}

		if (auto_upload_to_vram)
		{
			std::cout << "[VRAM] ";
			m->uploadToVRAM();
		}

		std::cout << "[OK BIN]  Faces: " << (m->interleaved.size() ? m->interleaved.size() : m->vertices.size()) / 3 << " Time: " << (getTime() - time) * 0.001 << "sec" << std::endl;
		sMeshesLoaded[filename] = m;
		return m;
	}

	//load the ascii version
	bool loaded = false;
	if (file_format == FORMAT_OBJ)
		loaded = m->loadOBJ(filename);
	else if (file_format == FORMAT_ASE)
		loaded = m->loadASE(filename);
	else if (file_format == FORMAT_MESH)
		loaded = m->loadMESH(filename);

	if (!loaded)
	{
		delete m;
		std::cout << "[ERROR]: Mesh not found" << std::endl;
		return NULL;
	}

	//to optimize, interleave the meshes
	if (interleave_meshes)
	{
		std::cout << "[INTERL] ";
		m->interleaveBuffers();
	}

	//and upload them to VRAM
	if (auto_upload_to_vram)
	{
		std::cout << "[VRAM] ";
		m->uploadToVRAM();
	}

	std::cout << "[OK]  Faces: " << m->vertices.size() / 3 << " Time: " << (getTime() - time) * 0.001 << "sec" << std::endl;
	if (use_binary)
	{
		std::cout << "\t\t Writing .BIN ... ";
		m->writeBin(filename);
		std::cout << "[OK]" << std::endl;
	}

	m->registerMesh(name);
	return m;
}

void Mesh::registerMesh( std::string name )
{
	this->name = name;
	sMeshesLoaded[name] = this;
}

void Mesh::Release()
{
	for (auto m : sMeshesLoaded)
	{
        stdlog("Destroy mesh: " + m.first );
		delete m.second;
	}
	sMeshesLoaded.clear();
}

};