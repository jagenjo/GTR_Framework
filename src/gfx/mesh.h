#ifndef MESH_H
#define MESH_H

#include <vector>
#include "../core/math.h"

#include <map>
#include <string>

struct BoneInfo {
	char name[32]; //max 32 chars per bone name
	Matrix44 bind_pose;
};

namespace GFX {

	class Shader; //for binding
	class Skeleton; //for skinned meshes

	//version from 11/5/2020
#define MESH_BIN_VERSION 11 //this is used to regenerate bins if the format changes

	struct sSubmeshInfo
	{
		char name[64];
		char material[64];
		int start;//in primitive
		int length;//in primitive
	};

	class Mesh
	{
	public:
		static std::map<std::string, Mesh*> sMeshesLoaded;
		static bool use_binary; //always load the binary version of a mesh when possible
		static bool interleave_meshes; //loaded meshes will me automatically interleaved
		static bool use_vao; //use vertex array object
		static bool auto_upload_to_vram; //loaded meshes will be stored in the VRAM
		static long num_meshes_rendered;
		static long num_triangles_rendered;
		static uint32 s_last_index;

		std::string name;
		uint32 index; //used internally

		std::vector<sSubmeshInfo> submeshes; //contains info about every submesh

		std::vector< Vector3f > vertices; //here we store the vertices
		std::vector< Vector3f > normals;	 //here we store the normals
		std::vector< Vector2f > uvs;	 //here we store the texture coordinates
		std::vector< Vector2f > m_uvs1; //secondary sets of uvs
		std::vector< Vector4f > colors; //here we store the colors

		struct tInterleaved {
			Vector3f vertex;
			Vector3f normal;
			Vector2f uv;
		};

		std::vector< tInterleaved > interleaved; //to render interleaved

		std::vector<unsigned int> m_indices; //for indexed meshes

		//for animated meshes
		std::vector< Vector4ub > bones; //tells which bones afect the vertex (4 max)
		std::vector< Vector4f > weights; //tells how much affect every bone
		std::vector< BoneInfo > bones_info; //tells 
		Matrix44 bind_matrix;

		Vector3f aabb_min;
		Vector3f	aabb_max;
		BoundingBox box;

		float radius;

		unsigned int vao_id; //Vertex Array Object

		unsigned int vertices_vbo_id;
		unsigned int uvs_vbo_id;
		unsigned int normals_vbo_id;
		unsigned int colors_vbo_id;

		unsigned int indices_vbo_id;
		unsigned int interleaved_vbo_id;
		unsigned int bones_vbo_id;
		unsigned int weights_vbo_id;
		unsigned int uvs1_vbo_id;

		Mesh();
		~Mesh();

		void clear();

		void render(unsigned int primitive, int submesh_id = -1, int num_instances = 0);
		void renderInstanced(unsigned int primitive, const Matrix44* instanced_models, int number);
		void renderBounding(const Matrix44& model, bool world_bounding = true);
		void renderFixedPipeline(int primitive); //sloooooooow
		//void renderAnimated(unsigned int primitive, Skeleton *sk);

		void enableBuffers(Shader* shader); //if shader is null the attrib locations must be POS=0, NORM=1, COORD=2, COORD1=3, COLOR=4, BONES=5, WEIGHTS=6
		void drawCall(unsigned int primitive, int submesh_id = -1, int num_instances = 0);
		void disableBuffers(Shader* shader);

		void getSubmeshStartAndSize(int submesh_id, unsigned int& start, unsigned int& size);

		bool readBin(const char* filename);
		bool writeBin(const char* filename);

		unsigned int getNumSubmeshes() { return (unsigned int)submeshes.size(); }
		unsigned int getNumVertices() { return (unsigned int)interleaved.size() ? (unsigned int)interleaved.size() : (unsigned int)vertices.size(); }

		//collision testing
		void* collision_model;
		bool createCollisionModel(bool is_static = false); //is_static sets if the inv matrix should be computed after setTransform (true) or before rayCollision (false)
		//help: model is the transform of the mesh, ray origin and direction, a Vector3 where to store the collision if found, a Vector3 where to store the normal if there was a collision, max ray distance in case the ray should go to infintiy, and in_object_space to get the collision point in object space or world space
		bool testRayCollision(Matrix44 model, Vector3f ray_origin, Vector3f ray_direction, Vector3f& collision, Vector3f& normal, float max_ray_dist = 3.4e+38F, bool in_object_space = false);
		bool testSphereCollision(Matrix44 model, Vector3f center, float radius, Vector3f& collision, Vector3f& normal);

		//loader
		static Mesh* Get(const char* filename, bool skip_load = false);
		static void Release();
		void registerMesh(std::string name);

		//create help meshes
		void createQuad(float center_x, float center_y, float w, float h, bool flip_uvs);
		void createPlane(float size);
		void createSubdividedPlane(float size = 1, int subdivisions = 256, bool centered = false);
		void createCube(Vector3f size);
		void createSphere(float radius, float slices = 24,float arcs = 16);
		void createWireBox();
		void createGrid(float dist);

		static Mesh* getQuad(); //get global quad

		void updateBoundingBox();

		//optimize meshes
		void uploadToVRAM();
		void drawUsingVAO(unsigned int primitive, int submesh_id = -1);
		bool interleaveBuffers();

	private:
		bool loadASE(const char* filename);
		bool loadOBJ(const char* filename);
		bool loadMESH(const char* filename); //personal format used for animations
		//bool loadOBJTiny(const char* filename);
	};

};

#endif
