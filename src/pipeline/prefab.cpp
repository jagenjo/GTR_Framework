
#include "../core/includes.h"
#include "prefab.h"

#include "../gfx/mesh.h"
#include "../gfx/texture.h"
#include "material.h"
#include "camera.h"

#include "../utils/gltf_loader.h"
#include "../utils/utils.h"
#include "../core/math.h"

#include <iostream>

using namespace SCN;

int Node::s_NodeID = 0;
Node* Node::s_selected = nullptr;

Node::Node() : parent(nullptr), mesh(nullptr), material(nullptr), visible(true)
{
	m_Id = s_NodeID++;
}

Node::~Node()
{
	assert(parent == nullptr); //cannot delete a node that has a parent
	clear();

	//cant delete mesh, material or skeleton as it is a shared resource
	//delete mesh;
	//delete material;

	mesh = nullptr;
	material = nullptr;

	if (s_selected == this)
		s_selected = nullptr;
}

void Node::clear()
{
	//delete children
	for (int i = 0; i < children.size(); ++i)
	{
		children[i]->parent = NULL;
		delete children[i]; //triggers clear
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

bool Node::testRay(const Ray& ray, Vector3f& result, int layers, float max_dist)
{
	Vector3f collision;
	Vector3f normal;
	bool collided = false;
	if (mesh && material && material->alpha_mode != SCN::eAlphaMode::BLEND)
	{

		collided = mesh->testRayCollision( getGlobalMatrix(), ray.origin, ray.direction, collision, normal, max_dist );
		if (collided)
			max_dist = ray.origin.distance(collision);
	}

	for (int i = 0; i < children.size(); ++i)
	{
		Vector3f child_collision;
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
			std::cout << "[ERROR]: Prefab not found: " << filename << std::endl;
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


void updateInDepth(std::map<std::string, SCN::Node*>& container, SCN::Node* node)
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
