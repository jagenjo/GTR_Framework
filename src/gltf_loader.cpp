#include "gltf_loader.h"

#define CGLTF_IMPLEMENTATION
#include "extra/cgltf.h"

#include "mesh.h"
#include "texture.h"
#include "material.h"
#include "prefab.h"

#include <iostream>

//** PARSING GLTF IS UGLY
std::string base_folder;

#ifdef _DEBUG
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

void parseGLTFBufferIndices(std::vector<Vector3u>& container, cgltf_accessor* acc)
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

std::vector<Mesh*> parseGLTFMesh(cgltf_mesh* meshdata)
{
	std::vector<Mesh*> result;

	if(meshdata->name)
		std::cout << "MESH: " << meshdata->name << std::endl;

	//submeshes
	for (int i = 0; i < meshdata->primitives_count; ++i)
	{
		cgltf_primitive* primitive = &meshdata->primitives[i];
		Mesh* mesh = NULL;

		std::string submesh_name;
		if (meshdata->name)
		{
			submesh_name = std::string(meshdata->name) + std::string("::") + std::to_string(i);
			mesh = Mesh::Get(submesh_name.c_str(),true);
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
			else if (attr->type == cgltf_attribute_type_normal)
				parseGLTFBufferVector3(mesh->normals, attr->data);
			else if (attr->type == cgltf_attribute_type_texcoord)
			{
				if ( strcmp( attr->name,"TEXCOORD_1") == 0 ) //secondary UV set
					parseGLTFBufferVector2(mesh->uvs1, attr->data);
				else
					parseGLTFBufferVector2(mesh->uvs, attr->data);
			}

			if (primitive->indices && primitive->indices->count)
				parseGLTFBufferIndices(mesh->indices, primitive->indices);
		}

		mesh->uploadToVRAM();
		if (meshdata->name)
			mesh->registerMesh(submesh_name);
		result.push_back(mesh);
	}

	return result;
}

GTR::Material* parseGLTFMaterial(cgltf_material* matdata)
{
	GTR::Material* material = matdata->name ? GTR::Material::Get(matdata->name) : NULL;
	if (material)
		return material;
	material = new GTR::Material();
	if(matdata->name)
		material->registerMaterial(matdata->name);
	material->alpha_mode = (GTR::AlphaMode)matdata->alpha_mode;
	material->alpha_cutoff = matdata->alpha_cutoff;
	material->two_sided = matdata->double_sided;

	//normalmap
	if (matdata->normal_texture.texture)
	{
		const char* filename = matdata->normal_texture.texture->image->uri;
		if (load_textures)
			material->normal_texture = Texture::Get(std::string(base_folder + "/" + filename).c_str());
	}

	//emissive
	material->emissive_factor = matdata->emissive_factor;
	if (matdata->emissive_texture.texture)
	{
		const char* filename = matdata->emissive_texture.texture->image->uri;
		if (load_textures)
			material->emissive_texture = Texture::Get(std::string(base_folder + "/" + filename).c_str());
	}

	//pbr
	if (matdata->has_pbr_specular_glossiness)
	{
		if (matdata->pbr_specular_glossiness.diffuse_texture.texture)
		{
			const char* filename = matdata->pbr_specular_glossiness.diffuse_texture.texture->image->uri;
			//std::cout << base_folder + "/" + filename << std::endl;
			if (load_textures)
				material->color_texture = Texture::Get(std::string(base_folder + "/" + filename).c_str());
		}
	}
	if (matdata->has_pbr_metallic_roughness)
	{
		material->color = matdata->pbr_metallic_roughness.base_color_factor;
		material->metallic_factor = matdata->pbr_metallic_roughness.metallic_factor;
		material->roughness_factor = matdata->pbr_metallic_roughness.roughness_factor;

		if (matdata->pbr_metallic_roughness.base_color_texture.texture)
		{
			const char* filename = matdata->pbr_metallic_roughness.base_color_texture.texture->image->uri;
			if (load_textures)
				material->color_texture = Texture::Get(std::string(base_folder + "/" + filename).c_str());
		}
		if (matdata->pbr_metallic_roughness.metallic_roughness_texture.texture)
		{
			const char* filename = matdata->pbr_metallic_roughness.metallic_roughness_texture.texture->image->uri;
			if (load_textures)
				material->metallic_roughness_texture = Texture::Get(std::string(base_folder + "/" + filename).c_str());
		}
	}

	if (matdata->occlusion_texture.texture)
	{
		const char* filename = matdata->occlusion_texture.texture->image->uri;
		if (load_textures)
			material->occlusion_texture = Texture::Get(std::string(base_folder + "/" + filename).c_str());
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
			R.transpose();
			model = model * R;
		}
		if (node->has_scale)
			model.scale(node->scale[0], node->scale[1], node->scale[2]);
	}
	//model.transpose(); //dont know why
}

//GLTF PARSING: you can pass the node or it will create it
GTR::Node* parseGLTFNode(cgltf_node* node, GTR::Node* scenenode = NULL)
{
	if (scenenode == NULL)
		scenenode = new GTR::Node();

	//std::cout << node->name << std::endl;
	if(node->name)
		scenenode->name = node->name;

	parseGLTFTransform(node, scenenode->model);

	if (node->mesh)
	{
		/*
		if (node->mesh->name)
		{
			//meshname = node->mesh->name;
			scenenode->mesh = Mesh::Get(node->mesh->name);
		}
		*/

		if (node->mesh->primitives_count > 1 && 1)
		{
			std::vector<Mesh*> meshes;
			meshes = parseGLTFMesh(node->mesh);

			for (int i = 0; i < node->mesh->primitives_count; ++i)
			{
				GTR::Node* subnode = new GTR::Node();
				subnode->mesh = meshes[i];
				if (node->mesh->primitives[i].material)
					subnode->material = parseGLTFMaterial(node->mesh->primitives[i].material);
				scenenode->addChild(subnode);
			}
		}
		else //single primitive
		{
			if (node->mesh->name)
				scenenode->mesh = Mesh::Get(node->mesh->name,true);

			if (!scenenode->mesh)
			{
				std::vector<Mesh*> meshes;
				meshes = parseGLTFMesh(node->mesh);
				if(meshes.size())
					scenenode->mesh = meshes[0];
			}

			if (node->mesh->primitives->material)
				scenenode->material = parseGLTFMaterial(node->mesh->primitives->material);
		}
	}

	for (int i = 0; i < node->children_count; ++i)
		scenenode->addChild(parseGLTFNode(node->children[i]));

	return scenenode;
}

GTR::Prefab* loadGLTF(const char* filename)
{
	std::cout << "loading gltf... " << filename << std::endl;
	cgltf_options options;
	memset(&options, 0, sizeof(cgltf_options));
	cgltf_data* data = NULL;
	cgltf_result result = cgltf_parse_file(&options, filename, &data);
	if (result != cgltf_result_success)
	{
		std::cout << "[NOT FOUND]" << std::endl;
		return NULL;
	}

	if (data->scenes_count > 1)
		std::cout << "[WARN] more than one scene, skipping the rest" << std::endl;

	//get nodes
	cgltf_scene* scene = &data->scenes[0];

	char folder[1024];
	strcpy(folder, filename);
	char* name_start = strrchr(folder, '/');
	*name_start = '\0';
	base_folder = folder; //global

	result = cgltf_load_buffers(&options, data, filename);
	if (result != cgltf_result_success)
	{
		std::cout << "[BIN NOT FOUND]:" << filename << std::endl;
		return NULL;
	}

	if (scene->nodes_count > 1)
		std::cout << "[WARN] more than one root node, skipping the rest" << std::endl;

	cgltf_node* node = scene->nodes[0];

	//fetch first valid node (glTF sometime have lots of nested empty nodes 
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

	GTR::Prefab* prefab = new GTR::Prefab();

	parseGLTFNode(node, &prefab->root);
	prefab->root.model = model;
	prefab->updateNodesByName();
	prefab->updateBounding();

	//frees all data, including bin
	cgltf_free(data);

	return prefab;
}