#ifndef MESH_H
#define MESH_H

#include <vector>
#include "framework.h"

#include <map>
#include <string>

class Shader; //for binding
class Image; //for displace
class Skeleton; //for skinned meshes

//version from 11/5/2020
#define MESH_BIN_VERSION 11 //this is used to regenerate bins if the format changes

struct BoneInfo {
	char name[32]; //max 32 chars per bone name
	Matrix44 bind_pose;
};

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
	static bool auto_upload_to_vram; //loaded meshes will be stored in the VRAM
	static long num_meshes_rendered;
	static long num_triangles_rendered;

	std::string name;

	std::vector<sSubmeshInfo> submeshes; //contains info about every submesh

	std::vector< Vector3 > vertices; //here we store the vertices
	std::vector< Vector3 > normals;	 //here we store the normals
	std::vector< Vector2 > uvs;	 //here we store the texture coordinates
	std::vector< Vector2 > m_uvs1; //secondary sets of uvs
	std::vector< Vector4 > colors; //here we store the colors
	
	struct tInterleaved {
		Vector3 vertex;
		Vector3 normal;
		Vector2 uv;
	};

	std::vector< tInterleaved > interleaved; //to render interleaved

	std::vector<unsigned int> m_indices; //for indexed meshes

	//for animated meshes
	std::vector< Vector4ub > bones; //tells which bones afect the vertex (4 max)
	std::vector< Vector4 > weights; //tells how much affect every bone
	std::vector< BoneInfo > bones_info; //tells 
	Matrix44 bind_matrix;

	Vector3 aabb_min;
	Vector3	aabb_max;
	BoundingBox box;

	float radius;

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

	void render( unsigned int primitive, int submesh_id = -1, int num_instances = 0 );
	void renderInstanced(unsigned int primitive, const Matrix44* instanced_models, int number);
	void renderBounding( const Matrix44& model, bool world_bounding = true );
	void renderFixedPipeline(int primitive); //sloooooooow
	//void renderAnimated(unsigned int primitive, Skeleton *sk);

	void enableBuffers(Shader* shader);
	void drawCall(unsigned int primitive, int submesh_id, int num_instances);
	void disableBuffers(Shader* shader);

	bool readBin(const char* filename);
	bool writeBin(const char* filename);

	unsigned int getNumSubmeshes() { return (unsigned int)submeshes.size(); }
	unsigned int getNumVertices() { return (unsigned int)interleaved.size() ? (unsigned int)interleaved.size() : (unsigned int)vertices.size(); }

	//collision testing
	void* collision_model;
	bool createCollisionModel(bool is_static = false); //is_static sets if the inv matrix should be computed after setTransform (true) or before rayCollision (false)
	//help: model is the transform of the mesh, ray origin and direction, a Vector3 where to store the collision if found, a Vector3 where to store the normal if there was a collision, max ray distance in case the ray should go to infintiy, and in_object_space to get the collision point in object space or world space
	bool testRayCollision( Matrix44 model, Vector3 ray_origin, Vector3 ray_direction, Vector3& collision, Vector3& normal, float max_ray_dist = 3.4e+38F, bool in_object_space = false );
	bool testSphereCollision(Matrix44 model, Vector3 center, float radius, Vector3& collision, Vector3& normal);

	//loader
	static Mesh* Get(const char* filename, bool skip_load = false);
	static void Release();
	void registerMesh(std::string name);

	//create help meshes
	void createQuad(float center_x, float center_y, float w, float h, bool flip_uvs);
	void createPlane(float size);
	void createSubdividedPlane(float size = 1, int subdivisions = 256, bool centered = false);
	void createCube();
	void createWireBox();
	void createGrid(float dist);
	void displace(Image* heightmap, float altitude);
	static Mesh* getQuad(); //get global quad

	void updateBoundingBox();

	//optimize meshes
	void uploadToVRAM();
	bool interleaveBuffers();

private:
	bool loadASE(const char* filename);
	bool loadOBJ(const char* filename);
	bool loadMESH(const char* filename); //personal format used for animations
};

#endif
