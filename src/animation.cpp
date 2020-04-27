#include "animation.h"
#include "framework.h"
#include "utils.h"
#include <cassert>

#include "camera.h"
#include "shader.h"
#include "mesh.h"

#include <sys/stat.h>

Skeleton::Skeleton()
{
	num_bones = 0;
}

Skeleton::Bone* Skeleton::getBone(const char* name)
{
	auto it = bones_by_name.find(name);
	if (it == bones_by_name.end())
		return NULL;
	return &bones[it->second];
}

Matrix44& Skeleton::getBoneMatrix(const char* name, bool local )
{
	static Matrix44 none;
	auto it = bones_by_name.find(name);
	if (it == bones_by_name.end())
		return none;
	if (local)
		return bones[ it->second ].model;
	return global_bone_matrices[ it->second ];
}

void Skeleton::computeFinalBoneMatrices( std::vector<Matrix44>& bone_matrices, Mesh* mesh )
{
	assert(mesh);

	updateGlobalMatrices();

	bone_matrices.resize(mesh->bones_info.size());
	#pragma omp for  
	for (int i = 0; i < mesh->bones_info.size(); ++i)
	{
		BoneInfo& bone_info = mesh->bones_info[i];
		bone_matrices[i] = mesh->bind_matrix * bone_info.bind_pose * getBoneMatrix( bone_info.name, false ); //use globals
	}
}

void blendSkeleton(Skeleton* a, Skeleton* b, float w, Skeleton* result, uint8 layer)
{
	assert(a && b && result && "skeleton cannot be NULL");
	assert(a->num_bones == b->num_bones && "skeleton must contain the same number of bones");

	w = clamp(w, 0.0f, 1.0f);//safety

	if (layer == 0xFF)
	{
		if (w == 0.0f)
		{
			if(result == a) //nothing to do
				return;
			*result = *a; //copy A in Result
			return;
		}
		if (w == 1.0f) //copy B in result
		{
			*result = *b;
			return;
		}
	}

	if (result != a) //copy bone names
	{
		memcpy(result->bones, a->bones, sizeof(result->bones)); //copy skeleton structure
		result->bones_by_name = a->bones_by_name;
		result->num_bones = a->num_bones;
	}

	//blend bones locally
	#pragma omp for  
	for (int i = 0; i < result->num_bones; ++i)
	{
		Skeleton::Bone& bone = result->bones[i];
		Skeleton::Bone& boneA = a->bones[i];
		Skeleton::Bone& boneB = b->bones[i];
		if ( layer != 0xFF && !(bone.layer & layer) ) //not in the same layer
			continue;
		#pragma omp for  
		for (int j = 0; j < 16; ++j)
			bone.model.m[j] = lerp( boneA.model.m[j], boneB.model.m[j], w);
	}
}

void Skeleton::renderSkeleton(Camera* camera, Matrix44 model, Vector4 color, bool render_points)
{
	Mesh m;

	for (int i = 1; i < num_bones; ++i)
	{
		Bone& bone = bones[i];
		Vector3 v1;
		Vector3 v2;
		Matrix44 parent_global_matrix = global_bone_matrices[ bone.parent ];
		Matrix44 global_matrix = global_bone_matrices[i];
		v1 = global_matrix * v1;
		v2 = parent_global_matrix * v2;
		m.vertices.push_back(v1);
		m.vertices.push_back(v2);
	}

	Shader* shader = Shader::getDefaultShader("flat");
	shader->enable();
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
	shader->setUniform("u_model", model);
	shader->setUniform("u_color", color);
	m.render(GL_LINES);
	if (render_points)
	{
		shader->setUniform("u_color", color * 2);
		glPointSize(10);
		m.render(GL_POINTS);
		glPointSize(1);
	}
	shader->disable();
}

void Skeleton::applyTransformToBones(const char* root, Matrix44 transform)
{
	Bone* bone = getBone(root);
	if (!bone)
		return;
	bone->model = bone->model * transform;
}

void Skeleton::updateGlobalMatrices()
{
	//compute global matrices
	global_bone_matrices[0] = bones[0].model;
	//order dependant
	for (int i = 1; i < num_bones; ++i)
	{
		Skeleton::Bone& bone = bones[i];
		global_bone_matrices[i] = bone.model * global_bone_matrices[ bone.parent ];
	}
}

void Skeleton::assignLayer( Bone* bone, uint8 layer )
{
	if (!bone)
		return;
	if(layer)
		bone->layer |= layer;
	else
		bone->layer = 0;
	for (int i = 0; i < bone->num_children; ++i)
	{
		Bone* child = &bones[bone->children[i]];
		assignLayer(child, layer);
	}
}

Animation::Animation()
{
	duration = 0.0f;
	keyframes = NULL;
	num_keyframes = 0;
	num_animated_bones = 0;
}

Animation::~Animation()
{
	if (keyframes)
		delete[] keyframes;
}

void Animation::assignTime(float t, bool loop, bool interpolate, uint8 layers)
{
	assert(keyframes && skeleton.num_bones);

	if (loop)
	{
		t = fmod(t, duration);
		if (t < 0)
			t = duration + t;
	}
	else
		t = clamp( t, 0.0f, duration - (1.0/samples_per_second) );
	float v = samples_per_second * t;
	int index = clamp(floor(v), 0, num_keyframes - 1);
	int index2 = index + 1;
	if (index2 >= num_keyframes)
		index2 = 0;
	float f = v - floor(v);

	Matrix44* k = keyframes + index * num_animated_bones;
	Matrix44* k2 = keyframes + index2 * num_animated_bones;

	//compute local bones
	#pragma omp for  
	for (int i = 0; i < num_animated_bones; ++i)
	{
		int bone_index = bones_map[i];
		Skeleton::Bone& bone = skeleton.bones[bone_index];
		if (layers != 0xFF && !(bone.layer & layers))
			continue;
		for (int j = 0; j < 16; ++j)
			bone.model.m[j] = lerp(k[i].m[j], k2[i].m[j], f);
	}

	skeleton.updateGlobalMatrices();
}


void Animation::operator = (Animation* anim)
{
	memcpy(this, anim, sizeof(Animation));
	this->keyframes = NULL;
}

bool Animation::load(const char* filename)
{
	//struct stat stbuffer;

	std::cout << " + Animation loading: " << filename << " ... ";
	long time = getTime();

	//char file_format = 0;
	std::string name = filename;
	std::string ext = name.substr(name.find_last_of(".") + 1);
	if (ext == "abin" || ext == "ABIN")
	{
		if( !loadABIN(filename) )
			return false;
	}
	else //not a bin
	{
		std::string binfilename = name + ".abin";
		if (!loadABIN(binfilename.c_str())) //not found
		{
			//try to load in ASCII
			if (!loadSKANIM(filename))
			{
				std::cout << " [ERROR]: File not found" << std::endl;
				return false;
			}

			std::cout << "[Writing .ABIN] ... ";
			writeABIN( filename );
		}
	}

	std::cout << "[OK] Num. Bones: " << skeleton.num_bones << " Time: " << (getTime() - time) * 0.001 << "sec" << std::endl;
	return true;
}

struct sAnimHeader {
	int version;
	int header_bytes;
	float duration;
	float samples_per_second;
	int num_animated_bones;
	int num_keyframes;
	int num_bones;
	int8 bones_map[128];
	char extra[16];
};

bool Animation::writeABIN(const char* filename)
{
	std::string s_filename = filename;
	s_filename += ".abin";

	FILE* f = fopen(s_filename.c_str(), "wb");
	if (f == NULL)
	{
		std::cout << "[ERROR] cannot write BIN: " << s_filename.c_str() << std::endl;
		return false;
	}

	//watermark
	fwrite("ABIN", sizeof(char), 4, f);

	sAnimHeader header;
	header.version = ANIM_BIN_VERSION;
	header.header_bytes = sizeof(header);
	header.duration = duration;
	header.samples_per_second = samples_per_second;
	header.num_animated_bones = num_animated_bones;
	header.num_keyframes = num_keyframes;
	header.num_bones = skeleton.num_bones;
	memcpy( header.bones_map, bones_map, sizeof(bones_map)  );

	//write header
	fwrite((void*)&header, sizeof(sAnimHeader), 1, f);

	//write skeleton
	fwrite((void*)skeleton.bones, sizeof(skeleton.bones), 1, f);

	//write keyframes
	fwrite((void*)keyframes, sizeof(Matrix44) * num_keyframes * num_animated_bones, 1, f);

	fclose(f);
	return true;
}

bool Animation::loadABIN(const char* filename)
{
	FILE *f;
	assert(filename);

	struct stat stbuffer;

	stat(filename, &stbuffer);
	f = fopen(filename, "rb");
	if (f == NULL)
		return false;

	unsigned int size = (unsigned int)stbuffer.st_size;
	char* data = new char[size];
	fread(data, size, 1, f);
	fclose(f);

	//watermark
	if (memcmp(data, "ABIN", 4) != 0)
	{
		std::cout << "[ERROR] loading BIN: invalid content: " << filename << std::endl;
		return false;
	}

	char* pos = data + 4;
	sAnimHeader header;
	memcpy(&header, pos, sizeof(sAnimHeader));
	pos += sizeof(sAnimHeader);

	if (header.version != ANIM_BIN_VERSION || header.header_bytes != sizeof(sAnimHeader))
	{
		std::cout << "[WARN] loading BIN: old version: " << filename << std::endl;
		return false;
	}

	//extract header
	duration = header.duration;
	samples_per_second = header.samples_per_second;
	num_animated_bones = header.num_animated_bones;
	num_keyframes = header.num_keyframes;
	skeleton.num_bones = header.num_bones;
	memcpy(bones_map, header.bones_map, sizeof(bones_map));

	//extract skeleton
	memcpy( skeleton.bones, pos, sizeof(skeleton.bones) );
	pos += sizeof(skeleton.bones);

	//extract keyframes
	assert(keyframes == NULL);
	keyframes = new Matrix44[num_keyframes * num_animated_bones];
	memcpy( keyframes, pos, sizeof(Matrix44)*num_keyframes * num_animated_bones );
	pos += sizeof(Matrix44) * num_keyframes * num_animated_bones;

	//compute bone names map
	for (int i = 0; i < skeleton.num_bones; ++i)
		skeleton.bones_by_name[ skeleton.bones[i].name ] = i;

	delete[] data;
	return true;
}

bool Animation::loadSKANIM(const char* filename)
{
	struct stat stbuffer;

	//duration in seconds, samples per second, num. samples, number of bones in the skeleton, number of animated bones
	FILE* f = fopen(filename, "rb");
	if (f == NULL)
		return false;
	stat(filename, &stbuffer);

	unsigned int size = (unsigned int)stbuffer.st_size;
	char* data = new char[size + 1];
	fread(data, size, 1, f);
	fclose(f);
	data[size] = 0;
	char* pos = data;
	char word[255];
	memset(&skeleton.bones, 0, sizeof(skeleton.bones)); //clear

	//duration in seconds, samples per second, num. samples, number of bones in the skeleton, number of animated bones
	std::vector<float> header;
	pos = fetchBufferFloat(pos, header, 5);
	duration = header[0];
	samples_per_second = header[1];
	num_keyframes = header[2];
	skeleton.num_bones = header[3];
	assert(skeleton.num_bones < 128); //MAX_BONES
	num_animated_bones = 0;

	int current_keyframe = 0;

	while (*pos)
	{
		char type = *pos;

		//memcpy(word, pos, 32 );
		//word[31] = 0;
		//std::cout << word << std::endl << std::flush;

		pos++;
		if (type == 'B') //bone
		{
			pos = fetchWord(pos, word);
			int index = atof(word);
			Skeleton::Bone& bone = skeleton.bones[index];
			pos = fetchWord(pos, bone.name);
			//std::cout << bone.name << std::endl;
			pos = fetchWord(pos, word);
			int parent_index = atof(word);
			bone.parent = parent_index;
			if (bone.parent != -1)
			{
				Skeleton::Bone& parent_bone = skeleton.bones[bone.parent];
				assert(parent_bone.num_children < 16);
				parent_bone.children[parent_bone.num_children++] = index;
			}

			pos = fetchMatrix44(pos, bone.model);
		}
		else if (type == '@')
		{
			std::vector<float> bones_map_info;
			pos = fetchBufferFloat(pos, bones_map_info);
			for (int j = 0; j < (int)bones_map_info.size(); ++j)
				bones_map[j] = bones_map_info[j];
			num_animated_bones = (int)bones_map_info.size();
			assert(keyframes == NULL);
			keyframes = new Matrix44[num_animated_bones * num_keyframes];
		}
		else if (type == 'K')
		{
			pos = fetchWord(pos, word);
			//float time = atof(word);
			Matrix44* k = keyframes + current_keyframe * num_animated_bones;
			current_keyframe++;
			for (int j = 0; j < num_animated_bones; ++j)
				pos = fetchMatrix44(pos, *(k + j));
		}
		else
			break; //end of file probably
	}

	for (int i = 0; i < skeleton.num_bones; ++i)
	{
		Skeleton::Bone& bone = skeleton.bones[i];
		bone.layer = BODY;
		skeleton.bones_by_name[bone.name] = i;
	}

	//assign layers
	Skeleton::Bone* hips = skeleton.getBone("mixamorig_Hips");
	if (hips)
	{
		hips->layer |= HIPS;
		skeleton.assignLayer(hips, BODY);//force every bone to have a bit
		skeleton.assignLayer(skeleton.getBone("mixamorig_Spine"), UPPER_BODY);
		skeleton.assignLayer(skeleton.getBone("mixamorig_RightUpLeg"), LOWER_BODY | RIGHT_LEG);
		skeleton.assignLayer(skeleton.getBone("mixamorig_LeftUpLeg"), LOWER_BODY | LEFT_LEG);
		skeleton.assignLayer(skeleton.getBone("mixamorig_RightShoulder"), RIGHT_ARM);
		skeleton.assignLayer(skeleton.getBone("mixamorig_LeftShoulder"), LEFT_ARM);
	}

	assignTime(0); //reset pose

	delete[] data;
	return true;
}


std::map<std::string, Animation*> Animation::sAnimationsLoaded;
Animation* Animation::Get(const char* filename)
{
	assert(filename);

	//check if loaded
	auto it = sAnimationsLoaded.find(filename);
	if (it != sAnimationsLoaded.end())
		return it->second;

	//load it
	Animation* anim = new Animation();
	if (!anim->load(filename))
	{
		delete anim;
		return NULL;
	}

	sAnimationsLoaded[filename] = anim;
	return anim;
}
