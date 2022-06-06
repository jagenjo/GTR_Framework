#include "gltf_loader.h"

//#include "../../engine/application.h"

#define CGLTF_IMPLEMENTATION
#include "extra/cgltf.h"

#include "mesh.h"
#include "texture.h"
#include "material.h"
#include "prefab.h"
#include "utils.h"

#include <iostream>

//** PARSING GLTF IS UGLY
std::string base_folder;

#ifdef _DEBUG2
	bool load_textures = false; //must textures be loadead?
#else
	bool load_textures = true; //must textures be loadead?
#endif

void parseGLTFBufferVector3(std::vector<Vector3>& container, cgltf_accessor* acc, cgltf_accessor* indices_acc = NULL)
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
	std::vector<Vector3> unindexed;
	unindexed.resize(num_elements);
	assert(acc->component_type == cgltf_component_type_r_32f && acc->type == cgltf_type_vec3);
	if (acc->stride == sizeof(Vector3))
		memcpy(&unindexed[0], data, num_elements * sizeof(Vector3));
	else
	{
		for (int i = 0; i < num_elements; ++i)
		{
			memcpy(&unindexed[i], data, sizeof(Vector3));
			data += acc->stride;
		}
	}

	if (!indices_acc)
	{
		container = unindexed;
		return;
	}

	container.resize(indices_acc->count);

	unsigned char* indices = (unsigned char*)indices_acc->buffer_view->buffer->data + indices_acc->offset;
	int stride = indices_acc->stride;
	for (int i = 0; i < indices_acc->count; ++i)
	{
		int index = 0;
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

void parseGLTFBufferVector2(std::vector<Vector2>& container, cgltf_accessor* acc, cgltf_accessor* indices_acc = NULL)
{
	assert(acc->buffer_view->buffer->data);
	unsigned char* data = (unsigned char*)(acc->buffer_view->buffer->data) + acc->offset + acc->buffer_view->offset;
	if (acc->normalized)
	{
		//denormalize
		assert(!"TO DO");
	}
	int num_elements = acc->count;
	std::vector<Vector2> unindexed;
	unindexed.resize(num_elements);
	assert(acc->component_type == cgltf_component_type_r_32f && acc->type == cgltf_type_vec2);
	if (acc->stride == sizeof(Vector2))
		memcpy(&unindexed[0], data, num_elements * sizeof(Vector2));
	else
	{
		for (int i = 0; i < num_elements; ++i)
		{
			memcpy(&unindexed[i], data, sizeof(Vector2));
			data += acc->stride;
		}
	}

	if (!indices_acc)
	{
		container = unindexed;
		return;
	}

	container.resize(indices_acc->count);

	assert(indices_acc->sparse.count == 0); //sparse not supported yet

	unsigned char* indices = (unsigned char*)indices_acc->buffer_view->buffer->data + indices_acc->offset;
	int stride = indices_acc->stride;
	for (int i = 0; i < indices_acc->count; ++i)
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

std::vector<Mesh*> parseGLTFMesh(cgltf_mesh* meshdata, const char* basename)
{
	std::vector<Mesh*> result;

	if (meshdata->name)
		stdlog( std::string("\t<- MESH: ") + meshdata->name);

    //submeshes
	for (int i = 0; i < meshdata->primitives_count; ++i)
	{
		cgltf_primitive* primitive = &meshdata->primitives[i];
		Mesh* mesh = NULL;

		std::string submesh_name;
		if (meshdata->name)
		{
			submesh_name = std::string(basename) + std::string("::") + std::string(meshdata->name) + std::string("::") + std::to_string(i);
			mesh = Mesh::Get(submesh_name.c_str(), true);
			if (mesh)
			{
				result.push_back(mesh);
				continue;
			}
		}

		mesh = new Mesh();

        //streams
		for (int j = 0; j < primitive->attributes_count; ++j)
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

Texture* parseGLTFTexture(cgltf_image* image, const char* filename)
{
	if (!load_textures || !image )
		return NULL;

	std::string fullpath = filename ? filename : "";

	if (image->uri)
		return Texture::GetAsync((std::string(base_folder) + "/" + image->uri).c_str());
	else
	if (filename)
	{
		fullpath = std::string(base_folder) + "/" + filename;
		Texture* tex = Texture::Find(fullpath.c_str());
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
		Texture* tex = new Texture();
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

GTR::Material* parseGLTFMaterial(cgltf_material* matdata, const char* basename)
{
	GTR::Material* material = NULL;
	std::string name;
	if (matdata->name)
	{
		name = std::string(basename) + std::string("::") + std::string(matdata->name);
		material = GTR::Material::Get(name.c_str());
	}
	
	if (material)
		return material;

	material = new GTR::Material();
	if (matdata->name)
		material->registerMaterial(name.c_str());

	material->alpha_mode = (GTR::eAlphaMode)matdata->alpha_mode;
	material->alpha_cutoff = matdata->alpha_cutoff;
	material->two_sided = matdata->double_sided;

	//normalmap
	if (matdata->normal_texture.texture)
	{
		material->normal_texture.texture = parseGLTFTexture( matdata->normal_texture.texture->image, matdata->normal_texture.texture->name);
		material->normal_texture.uv_channel = matdata->normal_texture.texcoord;
	}

	//emissive
	material->emissive_factor = matdata->emissive_factor;
	if (matdata->emissive_texture.texture)
	{
		material->emissive_texture.texture = parseGLTFTexture(matdata->emissive_texture.texture->image, matdata->emissive_texture.texture->name);
		material->emissive_texture.uv_channel = matdata->emissive_texture.texcoord;
	}


	//pbr
	if (matdata->has_pbr_specular_glossiness)
	{
		if (matdata->pbr_specular_glossiness.diffuse_texture.texture)
			material->color_texture.texture = parseGLTFTexture(matdata->pbr_specular_glossiness.diffuse_texture.texture->image, matdata->pbr_specular_glossiness.diffuse_texture.texture->name);
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
				material->color_texture.texture = parseGLTFTexture(matdata->pbr_metallic_roughness.base_color_texture.texture->image, matdata->pbr_metallic_roughness.base_color_texture.texture->name);
				material->color_texture.uv_channel = matdata->pbr_metallic_roughness.base_color_texture.texcoord;
			}
			if (matdata->pbr_metallic_roughness.metallic_roughness_texture.texture)
			{
				material->metallic_roughness_texture.texture = parseGLTFTexture(matdata->pbr_metallic_roughness.metallic_roughness_texture.texture->image, matdata->pbr_metallic_roughness.metallic_roughness_texture.texture->name);
				material->metallic_roughness_texture.uv_channel = matdata->pbr_metallic_roughness.metallic_roughness_texture.texcoord;
			}
		}
	}

	if (matdata->occlusion_texture.texture)
	{
		material->occlusion_texture.texture = parseGLTFTexture(matdata->occlusion_texture.texture->image, matdata->occlusion_texture.texture->name);
		material->occlusion_texture.uv_channel = matdata->occlusion_texture.texcoord;
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
GTR::Node* parseGLTFNode(cgltf_node* node, GTR::Node* scenenode = NULL, const char* basename = NULL)
{
	if (scenenode == NULL)
		scenenode = new GTR::Node();

	//std::cout << node->name << std::endl;
	if (node->name)
		scenenode->name = node->name;

    stdlog("\t\t* prefab node: " + scenenode->name );

	parseGLTFTransform(node, scenenode->model);

    if (node->mesh)
	{
        //split in subnodes
		if (node->mesh->primitives_count > 1)
		{
			std::vector<Mesh*> meshes;
			meshes = parseGLTFMesh(node->mesh, basename);

			for (int i = 0; i < node->mesh->primitives_count; ++i)
			{
				GTR::Node* subnode = new GTR::Node();
				subnode->mesh = meshes[i];
				if (node->mesh->primitives[i].material)
					subnode->material = parseGLTFMaterial(node->mesh->primitives[i].material, basename );
				scenenode->addChild(subnode);
			}
		}
		else //single primitive
		{
			if (node->mesh->name)
				scenenode->mesh = Mesh::Get(node->mesh->name, true);

			if (!scenenode->mesh)
			{
				std::vector<Mesh*> meshes;
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

	for (int i = 0; i < node->children_count; ++i)
		scenenode->addChild(parseGLTFNode(node->children[i],NULL, basename));

	return scenenode;
}

cgltf_result internalOpenFile(const struct cgltf_memory_options* memory_options, const struct cgltf_file_options* file_options, const char* path, cgltf_size* size, void** data)
{
	stdlog(std::string(" <- ") + path);
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
	stdlog(std::string(" <- ") + path);
	*size = g_buffer.size();
	char* file_data = new char[*size];
	memcpy(file_data, &g_buffer[0], *size);
	*data = file_data;

	g_buffer.clear();

	return cgltf_result_success;
}

GTR::Prefab* loadGLTF(const char *filename, cgltf_data *data, cgltf_options& options)
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

	GTR::Prefab* prefab = new GTR::Prefab();

	{
		if (scene->nodes_count > 1)
		{
			cgltf_node *root = nullptr;
			float fiTotal = 1.0f / (float) scene->nodes_count;
			for (int i = 0; i < scene->nodes_count; ++i)
			{
				float fProgress = ((float) i * fiTotal) * 100.0f;
				GTR::Node *node = parseGLTFNode(scene->nodes[i], NULL, filename);
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

GTR::Prefab* loadGLTF(const std::vector<unsigned char>& dat, const std::string& path)
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

GTR::Prefab* loadGLTF(const char* filename)
{
	stdlog(std::string("loading gltf... ") + filename);
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

