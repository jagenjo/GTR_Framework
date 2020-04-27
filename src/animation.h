#pragma once

#include "mesh.h"

class Camera;

#define ANIM_BIN_VERSION 3

//defined layers for every body
enum BODY_LAYERS {
	BODY = 1,
	UPPER_BODY = 2,
	LOWER_BODY = 4,
	LEFT_ARM = 8,
	RIGHT_ARM = 16,
	LEFT_LEG = 32,
	RIGHT_LEG = 64,
	HIPS = 128,
};

//used to compare bone names in the map
struct cmp_str { bool operator()(char const *a, char const *b) const { return std::strcmp(a, b) < 0; } };


//This class contains the bone structure hierarchy
class Skeleton {
public:
	//fixed size to help serializing
	struct Bone {
		int8 parent;	//id of the parent bone
		char name[32];	//fixed size bone name
		Matrix44 model; //local transformation (according to its parent bone)
		uint8 layer;	//which layers are assigned to this bone (UPPER_BODY, RIGHT_ARM, etc)
		uint8 num_children;	//how many child bones
		int8 children[16]; //list of child bone ids (max 16 children )
	};
	Bone bones[128]; //max 128 bones
	int num_bones;	//number of bones

	Matrix44 global_bone_matrices[128]; //transform of every bone in global coordinates (according to the 0,0,0 and not the parent)
	std::map<const char*, int, cmp_str> bones_by_name;	//map to get the bone index from its name, required to extract the final bones array

	Skeleton();

	Bone* getBone(const char* name); //returns the bone pointer
	Matrix44& getBoneMatrix(const char* name, bool local = true); //returns the local matrix of a bone
	void applyTransformToBones(const char* root, Matrix44 transform); //given a bone name and matrix, it multiplies the matrix to the bone
	void updateGlobalMatrices(); //updates the list of global matrices according to the local matrices

	void renderSkeleton(Camera* camera, Matrix44 model, Vector4 color = Vector4(0.5, 0, 0.5, 1), bool render_points = false); //renders the skeleton with lines
	void computeFinalBoneMatrices(std::vector<Matrix44>& bones, Mesh* mesh); //fills the std::vector with the bones ready for the shader
	void assignLayer(Bone* bone, uint8 layer); //assigns a layer to a node and all its children
};

//this function takes skeleton A and blends it with skeleton B and stores the result in result
void blendSkeleton(Skeleton* a, Skeleton* b, float w, Skeleton* result, uint8 layer = 0xFF);

//This class contains one animation loaded from a file (it also uses a skeleton to store the current snapshot)
class Animation {
public:

	Skeleton skeleton;

	float duration;
	float samples_per_second;
	int num_animated_bones;
	int num_keyframes;
	int8 bones_map[128]; //maps from keyframe data index to bone

	Matrix44* keyframes;

	Animation();
	~Animation();	//we need the dtor to remove the keyframes memory

	//change the skeleton to the given pose according to time
	void assignTime(float time, bool loop = true, bool interpolate = true, uint8 layers = 0xFF);

	//storage
	bool load(const char* filename);
	bool loadSKANIM(const char* filename);
	bool loadABIN(const char* filename);
	bool writeABIN(const char* filename);

	static std::map<std::string, Animation*> sAnimationsLoaded;
	static Animation* Get(const char* filename);

	//copy operator to copy the keyframes
	void operator = (Animation* anim);
};

