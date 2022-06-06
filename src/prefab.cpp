
#include "includes.h"
#include "prefab.h"

#include "mesh.h"
#include "texture.h"
#include "material.h"
#include "camera.h"

#include "gltf_loader.h"
#include "utils.h"
#include "framework.h"
#include "application.h"

#include <iostream>

using namespace GTR;

int Node::s_NodeID = 0;

Node::Node() : parent(NULL), mesh(NULL), material(NULL), visible(true), layers(0xFF)
{
	m_Id = s_NodeID++;
}

Node::~Node()
{
	assert(parent == NULL); //cannot delete a node that has a parent
	clear();

	//delete mesh;
	//delete material;

	mesh = nullptr;
	material = nullptr;
}

void Node::clear()
{
	//delete children
	for (int i = 0; i < children.size(); ++i)
	{
		children[i]->parent = NULL;
		delete children[i];
	}
	children.resize(0);
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

void Node::removeChild(Node* child)
{
	assert(child->parent == this);
	for (int i = 0; i < children.size(); ++i)
	{
		Node* node = children[i];
		if (node != child)
			continue;
		child->parent = NULL;
		children.erase(children.begin() + i);
		return;
	}
}

Node* Node::findNode(const char* name)
{
	if (this->name == name)
		return this;

	for (int i = 0; i < children.size(); ++i)
	{
		Node* node = children[i];
		if (node->name == name)
			return node;

		if (!node->children.size())
			continue;

		Node* found = node->findNode(name);
		if (found)
			return found;
	}

	return nullptr;
}

bool Node::testRay(const Ray& ray, Vector3& result, int layers, float max_dist)
{
	Vector3 collision;
	Vector3 normal;
	bool collided = false;
	if (mesh)
	{
		collided = mesh->testRayCollision( getGlobalMatrix(), ray.origin, ray.direction, collision, normal, max_dist );
		if (collided)
			max_dist = ray.origin.distance(collision);
	}

	for (int i = 0; i < children.size(); ++i)
	{
		Vector3 child_collision;
		if (!children[i]->testRay(ray, child_collision, layers, max_dist))
			continue;
		collided = true;
		collision = child_collision;
		max_dist = ray.origin.distance(collision);
	}

	if (collided)
		result = collision;

	return collided;
}

void Node::operator = (const Node& node)
{
	Node* old_parent = parent;
	clear(); //remove any children

	mesh = node.mesh;
	material = node.material;
	name = node.name;
	visible = node.visible;
	layers = node.layers;
	model = node.model;
	aabb = node.aabb;

	//clone children
	for (int i = 0; i < node.children.size(); ++i)
	{
		Node* child = node.children[i];
		Node* new_child = new Node();
		*new_child = *child;
		addChild(new_child);
	}
}

void Node::renderInMenu()
{
#ifndef SKIP_IMGUI
	ImGui::Text("Name: %s", name.c_str()); // Edit 3 floats representing a color
	if(mesh)
		ImGui::Text("Mesh: %s", mesh->name.c_str()); // Edit 3 floats representing a color

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
#endif
}

Prefab::Prefab()
{
}

Prefab::~Prefab()
{
	if (name.size())
	{
		auto it = sPrefabsLoaded.find(name);
		if (it != sPrefabsLoaded.end())
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

	Prefab* prefab = nullptr;
	{
		if (!prefab)
			prefab = loadGLTF(filename);
		if (!prefab) {
			std::cout << "[ERROR]: Prefab not found" << std::endl;
			return NULL;
		}
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
