#pragma once

#include <cassert>
#include <map>
#include <string>

#include "../core/math.h"
#include "material.h"

//forward declaration
namespace GFX {
	class Mesh;
	class Texture;
};

class Camera;

namespace SCN {

	class Primitive {
	public:
		Material* material;
		int index; //submesh index
		int prim; //triangles, points, lines
	};

	//A node represents a part of a prefab, that has a mesh, a material, and a transform matrix
	class Node
	{
	public:
		static int s_NodeID;
		static Node* s_selected;
		int m_Id;

	public:

	public:
		std::string name;
		bool visible;

		GFX::Mesh* mesh;
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
		void clear();

		BoundingBox getBoundingBox();

		Node* findNode(const char* name);

		//add node to children list
		void addChild(Node* child)
		{
			assert(child->parent == NULL);
			children.push_back(child);
			child->parent = this;
		}
		void removeChild(Node* child);

		//compute the global matrix taking into account its parent
		Matrix44 getGlobalMatrix(bool fast = false) { 
			if (parent)
				global_model = model * (fast ? parent->global_model : parent->getGlobalMatrix());
			else
				global_model = model;
			return global_model;
		}

		bool testRay(const Ray& ray, Vector3f& result, int layers = 0xFF, float max_dist = 3.4e+38F);
		Vector3f localToGlobal(Vector3f v) { return global_model * v; }

		void operator = (const Node& node);
	};

	//a Prefab represent a set of objects in a tree structure
	//used to load info from GLTF files
	class Prefab
	{
	public:

		std::string name;
		std::map<std::string, Node*> nodes_by_name;
		std::string url;

		//root node which contains the tree
		Node root;
		BoundingBox bounding;

		//ctor and dtor
		Prefab();
		~Prefab();

		void updateBounding();
		void updateNodesByName();
		Node* getNodeByName(const char* name);

		//Manager to cache loaded prefabs
		static std::map<std::string, Prefab*> sPrefabsLoaded;
		static Prefab* Get(const char* filename);
		void registerPrefab(std::string name);
	};

};
