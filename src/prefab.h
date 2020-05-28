#pragma once

#include "framework.h"
#include <cassert>
#include <map>
#include <string>

#include "material.h"

//forward declaration
class Mesh;
class Texture;
class Camera;

namespace GTR {

	//A node represents a part of a prefab, that has a mesh, a material, and a transform matrix
	class Node
	{
	public:
		std::string name;
		bool visible;
		int layers;

		Mesh* mesh;
		Material* material;
		Matrix44 model;	//the matrix that defines where is the object (in relation to its parent)
		Matrix44 global_model;	//the matrix that defines where is the object (in relation to the world)

		BoundingBox aabb; //node bounding box in world space

		//info to create the tree
		Node* parent;
		std::vector<Node*> children;

		//ctor
		Node();

		//dtor
		virtual ~Node();

		BoundingBox getBoundingBox();

		//render GUI info
		void renderInMenu();

		//add node to children list
		void addChild(Node* child) { assert(child->parent == NULL);  children.push_back(child); child->parent = this; }

		//compute the global matrix taking into account its parent
		Matrix44 getGlobalMatrix(bool fast = false) { 
			if (parent)
				global_model = model * (fast ? parent->global_model : parent->getGlobalMatrix());
			else
				global_model = model;
			return global_model;
		}
	};

	//a Prefab represent a set of objects in a tree structure
	//used to load info from GLTF files
	class Prefab
	{
	public:

		std::string name;
		std::map<std::string, Node*> nodes_by_name;
		//root node which contains the tree
		Node root;
		BoundingBox bounding;

		//dtor
		virtual ~Prefab();

		void updateBounding();
		void updateNodesByName();
		Node* getNodeByName(const char* name);

		//Manager to cache loaded prefabs
		static std::map<std::string, Prefab*> sPrefabsLoaded;
		static Prefab* Get(const char* filename);
		void registerPrefab(std::string name);
	};

};
