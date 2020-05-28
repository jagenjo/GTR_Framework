
#include "includes.h"
#include "prefab.h"

#include "mesh.h"
#include "texture.h"
#include "material.h"
#include "camera.h"

#include "gltf_loader.h"
#include "utils.h"
#include "framework.h"

#include <iostream>

using namespace GTR;

Node::Node() : parent(NULL), mesh(NULL), material(NULL), visible(true), layers(0xFF)
{

}

Node::~Node()
{
	assert(parent == NULL); //cannot delete a node that has a parent

	//delete children
	for (int i = 0; i < children.size(); ++i)
	{
		children[i]->parent = NULL;
		delete children[i];
	}
}

BoundingBox Node::getBoundingBox()
{
	aabb.center.set(0, 0, 0);
	aabb.halfsize.set(0, 0, 0);
	if (mesh)
		aabb = mesh->box;
	for (int i = 0; i < children.size(); ++i)
		aabb = mergeBoundingBoxes( children[i]->getBoundingBox(), aabb );
	return transformBoundingBox(model, aabb);
}

void Node::renderInMenu()
{
	ImGui::Text("Name: %s", name.c_str()); // Edit 3 floats representing a color

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.75f, 0.75f, 1.0f));

	//Model edit
	ImGuiMatrix44(model, "Model");

	//Material
	if (material && ImGui::TreeNode(material, "Material"))
	{
		material->renderInMenu();
		ImGui::TreePop();
	}

	ImGui::PopStyleColor();

	if (children.size() > 0)
	{
		if (ImGui::TreeNode(&children[0],"Children"))
		{
			for (int i = 0; i < children.size(); ++i)
			{
				children[i]->renderInMenu();
			}
			ImGui::TreePop();
		}
	}
}

Prefab::~Prefab()
{
	if (name.size())
	{
		auto it = sPrefabsLoaded.find(name);
		if (it != sPrefabsLoaded.end());
		sPrefabsLoaded.erase(it);
	}
}

void Prefab::updateBounding()
{
	bounding = root.getBoundingBox();
}

std::map<std::string, Prefab*> Prefab::sPrefabsLoaded;

Prefab* Prefab::Get(const char* filename)
{
	assert(filename);
	std::map<std::string, Prefab*>::iterator it = sPrefabsLoaded.find(filename);
	if (it != sPrefabsLoaded.end())
		return it->second;

	Prefab* prefab = loadGLTF(filename);
	if (!prefab)
	{
		std::cout << "[ERROR]: Prefab not found" << std::endl;
		return NULL;
	}

	std::string name = filename;
	prefab->registerPrefab(name);
	prefab->updateBounding();
	return prefab;
}

void Prefab::registerPrefab(std::string name)
{
	this->name = name;
	sPrefabsLoaded[name] = this;
}

Node* Prefab::getNodeByName(const char* name)
{
	auto it = nodes_by_name.find(name);
	if (it != nodes_by_name.end())
		return it->second;
	return NULL;
}

void updateInDepth(std::map<std::string, GTR::Node*>& container, GTR::Node* node)
{
	if (node->name.size())
		container[node->name] = node;
	for (int i = 0; i < node->children.size(); ++i)
		updateInDepth(container, node->children[i]);
}

void Prefab::updateNodesByName()
{
	nodes_by_name.clear();
	updateInDepth(nodes_by_name, &root);
}