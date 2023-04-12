#include "gltf_loader.h"

//#include "../../engine/application.h"

#define CGLTF_IMPLEMENTATION
#include "../extra/cgltf.h"

#include "../gfx/mesh.h"
#include "../gfx/texture.h"
#include "../pipeline/material.h"
#include "../pipeline/prefab.h"
#include "../utils/utils.h"

#include <iostream>

//** PARSING GLTF IS UGLY
std::string base_folder;

#ifdef _DEBUG2
	bool load_textures = false; //must textures be loadead?
#else
	bool load_textures = true; //must textures be loadead?
#endif

void parseGLTFBufferVector4(std::vector<Vector4f>& container, cgltf_accessor* acc, cgltf_accessor* indices_acc = NULL)
{
	int i = 0;

	assert(acc->buffer_view->buffer->data);
	unsigned char* data = (unsigned char*)(acc->buffer_view->buffer->data) + acc->buffer_view->offset + acc->offset;
	if (acc->normalized)
	{
		//denormalize
		//assert(!"TO DO");
	}
	int num_elements = acc->count;
	std::vector<Vector4f> unindexed;
	unindexed.resize(num_elements);

	std::vector<float> values;

	//sometimes colors are in 16u or 8u format, instead of 32f
	if (acc->component_type != cgltf_component_type_r_32f)
	{
		if (acc->type != cgltf_type_vec4)
		{
			//not supported vec3 colors
			return;
		}
		//num elements per num components as values is vector of floats
		values.resize(num_elements * 4);

		if (acc->component_type == cgltf_component_type_r_16u)
		{
			int stride = acc->stride;
			uint8* current = data;
			for (size_t i = 0; i < num_elements; ++i)
			{
				for (size_t j = 0; j < 4; ++j)
				{
					values[i*4+j] = *(uint16*)(current+j*2);
					if (acc->normalized) values[i * 4 + j] /= (float)0xFFFF;
				}
				current += stride;
			}
		}
		else if (acc->component_type == cgltf_component_type_r_8u)
		{
			int stride = acc->stride;
			uint8* current = data;
			for (size_t i = 0; i < num_elements; ++i)
			{
				for (size_t j = 0; j < 4; ++j)
				{
					values[i * 4 + j] = *(current + j);
					if (acc->normalized) values[i * 4 + j] /= (float)0xFFFF;
				}
				current += stride;
			}
		}
		else
			return;

		acc->stride = sizeof(Vector4f);
		data = (unsigned char*)&values[0];
	}

	//assert(acc->component_type == cgltf_component_type_r_32f && acc->type == cgltf_type_vec4);
	if (acc->stride == sizeof(Vector4f))
		memcpy(&unindexed[0], data, num_elements * sizeof(Vector4f));
	else
	{
		//read every element one by one to jump the gap between them
		for (int i = 0; i < num_elements; ++i)
		{
			memcpy(&unindexed[i], data, sizeof(Vector4f));
			data += acc->stride;
		}
	}

	if (!indices_acc)
	{
		container = unindexed;
		return;
	}

	//deindex
	container.resize(indices_acc->count);

	unsigned char* indices = (unsigned char*)indices_acc->buffer_view->buffer->data + indices_acc->offset;
	int stride = indices_acc->stride;
	for (size_t i = 0; i < indices_acc->count; ++i)
	{
		size_t index = 0;
		unsigned char* pos = indices + i * stride;
		switch (indices_acc->component_type)
		{
		case cgltf_component_type_r_8u: index = (int)*pos; break;
		case cgltf_component_type_r_16u: index = (int)*(unsigned short*)pos; break;
		case cgltf_component_type_r_32u: index = (int)*(unsigned int*)pos; break;
		}
		if (index < unindexed.size()) //sometimes indices are out of bounds
			container[i] = unindexed[index];
		else
			std::cout << "index out of bounds:" << index << std::endl;
	}
}

void parseGLTFBufferVector3(std::vector<Vector3f>& container, cgltf_accessor* acc, cgltf_accessor* indices_acc = NULL)
{
	int i = 0;

	assert(acc->buffer_view->buffer->data);
	unsigned char* data = (unsigned char*)(acc->buffer_view->buffer->data) + acc->buffer_view->offset + acc->offset;
	if (acc->normalized)
	{
		//denormalize
		assert(!"TO DO");
	}
	int num_elements = acc->count;
	std::vector<Vector3f> unindexed;
	unindexed.resize(num_elements);
	assert(acc->component_type == cgltf_component_type_r_32f && acc->type == cgltf_type_vec3);
	if (acc->stride == sizeof(Vector3f))
		memcpy(&unindexed[0], data, num_elements * sizeof(Vector3f));
	else
	{
		for (int i = 0; i < num_elements; ++i)
		{
			memcpy(&unindexed[i], data, sizeof(Vector3f));
			data += acc->stride;
		}
	}

	if (!indices_acc)
	{
		container = unindexed;
		return;
	}

	//deindex
	container.resize(indices_acc->count);

	unsigned char* indices = (unsigned char*)indices_acc->buffer_view->buffer->data + indices_acc->offset;
	int stride = indices_acc->stride;
	for (size_t i = 0; i < indices_acc->count; ++i)
	{
		size_t index = 0;
		unsigned char* pos = indices + i * stride;
		switch (indices_acc->component_type)
		{
		case cgltf_component_type_r_8u: index = (int)*pos; break;
		case cgltf_component_type_r_16u: index = (int)*(unsigned short*)pos; break;
		case cgltf_component_type_r_32u: index = (int)*(unsigned int*)pos; break;
		}
		if (index < unindexed.size()) //sometimes indices are out of bounds
			container[i] = unindexed[index];
		else
			std::cout << "index out of bounds:" << index << std::endl;
	}
}

void parseGLTFBufferVector2(std::vector<Vector2f>& container, cgltf_accessor* acc, cgltf_accessor* indices_acc = NULL)
{
	assert(acc->buffer_view->buffer->data);
	unsigned char* data = (unsigned char*)(acc->buffer_view->buffer->data) + acc->offset + acc->buffer_view->offset;
	if (acc->normalized)
	{
		//denormalize
		assert(!"TO DO");
	}
	int num_elements = acc->count;
	std::vector<Vector2f> unindexed;
	unindexed.resize(num_elements);
	assert(acc->component_type == cgltf_component_type_r_32f && acc->type == cgltf_type_vec2);
	if (acc->stride == sizeof(Vector2f))
		memcpy(&unindexed[0], data, num_elements * sizeof(Vector2f));
	else
	{
		for (int i = 0; i < num_elements; ++i)
		{
			memcpy(&unindexed[i], data, sizeof(Vector2f));
			data += acc->stride;
		}
	}

	if (!indices_acc)
	{
		container = unindexed;
		return;
	}

	//deindex
	container.resize(indices_acc->count);

	assert(indices_acc->sparse.count == 0); //sparse not supported yet

	unsigned char* indices = (unsigned char*)indices_acc->buffer_view->buffer->data + indices_acc->offset;
	int stride = indices_acc->stride;
	for (size_t i = 0; i < indices_acc->count; ++i)
	{
		unsigned int index = 0;
		unsigned char* pos = indices + i * stride;
		switch (indices_acc->component_type)
		{
		case cgltf_component_type_r_8u: index = static_cast<unsigned int>(*pos); break;
		case cgltf_component_type_r_16u: index = static_cast<unsigned int>(*(unsigned short*)pos); break;
		case cgltf_component_type_r_32u: index = static_cast<unsigned int>(*(unsigned int*)pos); break;
		}
		if (index < unindexed.size())  //sometimes indices are out of bounds
			container[i] = unindexed[index];
		else
			std::cout << "index out of bounds:" << index << std::endl;
	}

}

void parseGLTFBufferIndices(std::vector<unsigned int>& container, cgltf_accessor* acc)
{
	container.resize(acc->count);
	unsigned int *final_indices = (unsigned int*)&container[0];

	assert(acc->sparse.count == 0); //sparse not supported yet

	unsigned char* indices = (unsigned char*)acc->buffer_view->buffer->data + acc->buffer_view->offset + acc->offset;
	int stride = acc->stride;
	for (int i = 0; i < acc->count; ++i)
	{
		unsigned int index = 0;
		unsigned char* pos = indices + i * stride;
		switch (acc->component_type)
		{
		case cgltf_component_type_r_8u: index = static_cast<unsigned int>(*pos); break;
		case cgltf_component_type_r_16u: index = static_cast<unsigned int>(*(unsigned short*)pos); break;
		case cgltf_component_type_r_32u: index = static_cast<unsigned int>(*(unsigned int*)pos); break;
		}
		final_indices[i] = index;
	}
}

std::vector<GFX::Mesh*> parseGLTFMesh(cgltf_mesh* meshdata, const char* basename)
{
	std::vector<GFX::Mesh*> result;

	//if (meshdata->name)
	//	stdlog( std::string("\t<- MESH: ") + meshdata->name);

    //submeshes
	for (size_t i = 0; i < meshdata->primitives_count; ++i)
	{
		cgltf_primitive* primitive = &meshdata->primitives[i];
		GFX::Mesh* mesh = NULL;

		std::string submesh_name;
		if (meshdata->name)
		{
			submesh_name = std::string(basename) + std::string("::") + std::string(meshdata->name) + std::string("::") + std::to_string(i);
			mesh = GFX::Mesh::Get(submesh_name.c_str(), true);
			if (mesh)
			{
				result.push_back(mesh);
				continue;
			}
		}

		mesh = new GFX::Mesh();

        //streams
		for (size_t j = 0; j < primitive->attributes_count; ++j)
		{
			cgltf_attribute* attr = &primitive->attributes[j];

            //std::string attrname = attr->name;
			if (attr->type == cgltf_attribute_type_position)
			{
				parseGLTFBufferVector3(mesh->vertices, attr->data);
				if (attr->data->has_min && attr->data->has_max)
				{
					mesh->aabb_min = attr->data->min;
					mesh->aabb_max = attr->data->max;
					mesh->box.center = (mesh->aabb_max + mesh->aabb_min) * 0.5f;
					mesh->box.halfsize = mesh->aabb_max - mesh->box.center;
				}
				else
					mesh->updateBoundingBox();
			}
			else
			if (attr->type == cgltf_attribute_type_normal)
				parseGLTFBufferVector3(mesh->normals, attr->data);
			else
			if (attr->type == cgltf_attribute_type_texcoord)
			{
				if (strcmp(attr->name,"TEXCOORD_1") == 0) //secondary UV set
					parseGLTFBufferVector2(mesh->m_uvs1, attr->data);
				else
					parseGLTFBufferVector2(mesh->uvs, attr->data);
			}
			else
			if (attr->type == cgltf_attribute_type_color)
			{
				parseGLTFBufferVector4(mesh->colors, attr->data);
			}
			else
			if (attr->type == cgltf_attribute_type_weights)
			{
				parseGLTFBufferVector4(mesh->weights, attr->data);
			}
			else
			if (attr->type == cgltf_attribute_type_joints)
			{
				//parseGLTFBufferVector4(mesh->bones, attr->data);
			}

			if (primitive->indices && primitive->indices->count)
				parseGLTFBufferIndices(mesh->m_indices, primitive->indices);
		}
		mesh->uploadToVRAM();
		if (meshdata->name)
			mesh->registerMesh(submesh_name);
		result.push_back(mesh);
	}

	return result;
}

int GLTF_TEXTURE_LAST_ID = 1;

GFX::Texture* parseGLTFTexture(cgltf_image* image, const char* filename)
{
	if (!load_textures || !image )
		return NULL;

	std::string fullpath = filename ? filename : "";

	if (image->uri)
		return GFX::Texture::GetAsync((std::string(base_folder) + "/" + image->uri).c_str());
	else
	if (filename)
	{
		fullpath = std::string(base_folder) + "/" + filename;
		GFX::Texture* tex = GFX::Texture::Find(fullpath.c_str());
		if (tex)
			return tex;
	}
	else
	{
		std::stringstream ss;
		ss << GLTF_TEXTURE_LAST_ID++;
		fullpath = std::string(base_folder) + "/image" + ss.str();
	}

	if (image->buffer_view)
	{
		Image img;
		std::vector<unsigned char> buffer;
		buffer.resize(image->buffer_view->size);
		memcpy(&buffer[0], (char*)image->buffer_view->buffer->data + image->buffer_view->offset, image->buffer_view->size);

		if (!strcmp(image->mime_type, "image/png"))
			img.loadPNG(buffer);
		else if (!strcmp(image->mime_type, "image/jpeg"))
			img.loadJPG(buffer);
		else
		{
			stdlog(std::string("image format not supported: ") + image->mime_type);
			return NULL;
		}
		if (!img.width)
		{
			stdlog(std::string("image encoding has error: ") + image->mime_type);
			return NULL;
		}
		GFX::Texture* tex = new GFX::Texture();
		tex->loadFromImage(&img);
		if (filename)
		{
			tex->setName(fullpath.c_str());
			stdlog(std::string("\t<- TEXTURE: ") + fullpath);
		}
		else
			stdlog(std::string(" TEXTURE: UNNAMED ") + image->mime_type );

		return tex;
	}
	else
		stdlog(std::string(" No texture data") + image->mime_type);
	return NULL;
}

SCN::Material* parseGLTFMaterial(cgltf_material* matdata, const char* basename)
{
	SCN::Material* material = NULL;
	std::string name;
	if (matdata->name)
	{
		name = std::string(basename) + std::string("::") + std::string(matdata->name);
		material = SCN::Material::Get(name.c_str());
	}
	
	if (material)
		return material;

	material = new SCN::Material();
	if (matdata->name)
		material->registerMaterial(name.c_str());

	material->alpha_mode = (SCN::eAlphaMode)matdata->alpha_mode;
	material->alpha_cutoff = matdata->alpha_cutoff;
	material->two_sided = matdata->double_sided;

	//normalmap
	if (matdata->normal_texture.texture)
	{
		material->textures[SCN::eTextureChannel::NORMALMAP].texture = parseGLTFTexture( matdata->normal_texture.texture->image, matdata->normal_texture.texture->name);
		material->textures[SCN::eTextureChannel::NORMALMAP].uv_channel = matdata->normal_texture.texcoord;
	}

	//emissive
	material->emissive_factor = matdata->emissive_factor;
	if (matdata->emissive_texture.texture)
	{
		material->textures[SCN::eTextureChannel::EMISSIVE].texture = parseGLTFTexture(matdata->emissive_texture.texture->image, matdata->emissive_texture.texture->name);
		material->textures[SCN::eTextureChannel::EMISSIVE].uv_channel = matdata->emissive_texture.texcoord;
	}


	//pbr
	if (matdata->has_pbr_specular_glossiness)
	{
		if (matdata->pbr_specular_glossiness.diffuse_texture.texture)
			material->textures[SCN::eTextureChannel::ALBEDO].texture = parseGLTFTexture(matdata->pbr_specular_glossiness.diffuse_texture.texture->image, matdata->pbr_specular_glossiness.diffuse_texture.texture->name);
	}
	if (matdata->has_pbr_metallic_roughness)
	{
		material->color = matdata->pbr_metallic_roughness.base_color_factor;
		material->metallic_factor = matdata->pbr_metallic_roughness.metallic_factor;
		material->roughness_factor = matdata->pbr_metallic_roughness.roughness_factor;

		if (load_textures)
		{
			if (matdata->pbr_metallic_roughness.base_color_texture.texture)
			{
				material->textures[SCN::eTextureChannel::ALBEDO].texture = parseGLTFTexture(matdata->pbr_metallic_roughness.base_color_texture.texture->image, matdata->pbr_metallic_roughness.base_color_texture.texture->name);
				material->textures[SCN::eTextureChannel::ALBEDO].uv_channel = matdata->pbr_metallic_roughness.base_color_texture.texcoord;
			}
			if (matdata->pbr_metallic_roughness.metallic_roughness_texture.texture)
			{
				material->textures[SCN::eTextureChannel::METALLIC_ROUGHNESS].texture = parseGLTFTexture(matdata->pbr_metallic_roughness.metallic_roughness_texture.texture->image, matdata->pbr_metallic_roughness.metallic_roughness_texture.texture->name);
				material->textures[SCN::eTextureChannel::METALLIC_ROUGHNESS].uv_channel = matdata->pbr_metallic_roughness.metallic_roughness_texture.texcoord;
			}
		}
	}

	if (matdata->occlusion_texture.texture)
	{
		material->textures[SCN::eTextureChannel::OCCLUSION].texture = parseGLTFTexture(matdata->occlusion_texture.texture->image, matdata->occlusion_texture.texture->name);
		material->textures[SCN::eTextureChannel::OCCLUSION].uv_channel = matdata->occlusion_texture.texcoord;
	}

	return material;
}

void parseGLTFTransform(cgltf_node* node, Matrix44 &model)
{
	if (node->has_matrix)
		memcpy(model.m, node->matrix, sizeof(node->matrix)); //transform
	else {
		if (node->has_translation)
			model.translate(node->translation[0], node->translation[1], node->translation[2]);
		if (node->has_rotation)
		{
			Quaternion q(node->rotation[0], node->rotation[1], node->rotation[2], node->rotation[3]);
			Matrix44 R;
			q.toMatrix(R);
			//R.transpose();
			model = R * model;
		}
		if (node->has_scale)
			model.scale(node->scale[0], node->scale[1], node->scale[2]);
	}
}

//GLTF PARSING: you can pass the node or it will create it
SCN::Node* parseGLTFNode(cgltf_node* node, SCN::Node* scenenode = NULL, const char* basename = NULL)
{
	if (scenenode == NULL)
		scenenode = new SCN::Node();

	//std::cout << node->name << std::endl;
	if (node->name)
		scenenode->name = node->name;

    //stdlog("\t\t* prefab node: " + scenenode->name );

	parseGLTFTransform(node, scenenode->model);

    if (node->mesh)
	{
        //split in subnodes
		if (node->mesh->primitives_count > 1)
		{
			std::vector<GFX::Mesh*> meshes;
			meshes = parseGLTFMesh(node->mesh, basename);

			for (size_t i = 0; i < node->mesh->primitives_count; ++i)
			{
				SCN::Node* subnode = new SCN::Node();
				subnode->mesh = meshes[i];
				if (node->mesh->primitives[i].material)
					subnode->material = parseGLTFMaterial(node->mesh->primitives[i].material, basename );
				scenenode->addChild(subnode);
			}
		}
		else //single primitive
		{
			if (node->mesh->name)
				scenenode->mesh = GFX::Mesh::Get(node->mesh->name, true);

			if (!scenenode->mesh)
			{
				std::vector<GFX::Mesh*> meshes;
				meshes = parseGLTFMesh(node->mesh, basename);
				//printf("Parsed GLTF mesh %s (success)\n", node->name);
				//return nullptr;
				if(meshes.size())
					scenenode->mesh = meshes[0];
			}

			if (node->mesh->primitives->material)
				scenenode->material = parseGLTFMaterial(node->mesh->primitives->material, basename );
		}
	}

	for (size_t i = 0; i < node->children_count; ++i)
		scenenode->addChild(parseGLTFNode(node->children[i],NULL, basename));

	return scenenode;
}

cgltf_result internalOpenFile(const struct cgltf_memory_options* memory_options, const struct cgltf_file_options* file_options, const char* path, cgltf_size* size, void** data)
{
	//stdlog(std::string(" <- ") + path);
    std::vector<unsigned char> buffer;
    if (!readFileBin(path, buffer))
        return cgltf_result_file_not_found;
    *size = buffer.size();
    char* file_data = new char[*size];
    memcpy(file_data, &buffer[0], *size);
    *data = file_data;
    return cgltf_result_success;
}

std::vector<unsigned char> g_buffer;

cgltf_result internalOpenMemory(const struct cgltf_memory_options* memory_options, const struct cgltf_file_options* file_options, const char* path, cgltf_size* size, void** data)
{
	//stdlog(std::string(" <- ") + path);
	*size = g_buffer.size();
	char* file_data = new char[*size];
	memcpy(file_data, &g_buffer[0], *size);
	*data = file_data;

	g_buffer.clear();

	return cgltf_result_success;
}

SCN::Prefab* loadGLTF(const char *filename, cgltf_data *data, cgltf_options& options)
{
	cgltf_result result;

	if (data->scenes_count > 1)
		std::cout << "[WARN] more than one scene, skipping the rest" << std::endl;

	//get nodes
	cgltf_scene* scene = &data->scenes[0];

	char folder[1024];
	char basename[1024];
	strcpy(folder, filename);
	char* name_start = strrchr(folder, '/');
	*name_start = '\0';
	base_folder = folder; //global
	const char* basename_start = strrchr(filename, '/');
	strcpy(basename, basename_start+1);

	{
		result = cgltf_load_buffers(&options, data, filename);
		if (result != cgltf_result_success) {
			stdlog(std::string("[BIN NOT FOUND]:") + filename);
			return NULL;
		}
	}

	SCN::Prefab* prefab = new SCN::Prefab();

	{
		if (scene->nodes_count > 1)
		{
			cgltf_node *root = nullptr;
			float fiTotal = 1.0f / (float) scene->nodes_count;
			for (size_t i = 0; i < scene->nodes_count; ++i)
			{
				float fProgress = ((float) i * fiTotal) * 100.0f;
				SCN::Node *node = parseGLTFNode(scene->nodes[i], NULL, filename);
				prefab->root.addChild(node);
			}
		}
		else
		{
			parseGLTFNode(scene->nodes[0], &prefab->root, filename);
		}
	}


	//fetch first valid node (glTF sometime have lots of nested empty nodes 
	/*
	Matrix44 model;
	if (1)
	{
		while (node->children_count == 1 && !node->mesh)
		{
			Matrix44 temp;
			parseGLTFTransform(node, temp);
			model = temp * model;
			node = node->children[0];
		}
	}
	*/
	//prefab->root.model = model;

	prefab->updateNodesByName();
	prefab->updateBounding();

	//frees all data, including bin
	cgltf_free(data);

    stdlog( std::string(" - Loaded ") + filename );

    return prefab;
}

SCN::Prefab* loadGLTF(const std::vector<unsigned char>& dat, const std::string& path)
{
	cgltf_options options;
	memset(&options, 0, sizeof(cgltf_options));
	cgltf_data *data = NULL;

	g_buffer = dat;
	options.file.read = internalOpenMemory;
	cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data);

	if (result != cgltf_result_success) {
		std::cout << "[NOT FOUND]" << std::endl;
		return NULL;
	}
	return loadGLTF(path.c_str(), data, options);
}

SCN::Prefab* loadGLTF(const char* filename)
{
	std::cout << "loading gltf " << TermColor::YELLOW << filename << TermColor::DEFAULT << " ..." << std::endl;
	cgltf_options options;
	memset(&options, 0, sizeof(cgltf_options));
	cgltf_data *data = NULL;

	{
		options.file.read = internalOpenFile;
		cgltf_result result = cgltf_parse_file(&options, filename, &data);

		if (result != cgltf_result_success) {
			std::cout << "[NOT FOUND]" << std::endl;
			return NULL;
		}
	}

	return loadGLTF(filename, data, options);
}

