#pragma once

#include "framework.h"
#include <cassert>
#include <map>
#include <string>

//forward declaration
class Mesh;
class Texture;

namespace GTR {

	enum AlphaMode {
		NO_ALPHA,
		MASK,
		BLEND
	};

	//this class contains all info relevant of how something must be rendered
	class Material {
	public:
		//static manager to reuse materials
		static std::map<std::string, Material*> sMaterials;
		static Material* Get(const char* name);
		std::string name;
		void registerMaterial(const char* name);

		//parameters to control transparency
		AlphaMode alpha_mode;	//could be NO_ALPHA, MASK (alpha cut) or BLEND (alpha blend)
		float alpha_cutoff;		//pixels with alpha than this value shouldnt be rendered
		bool two_sided;			//render both faces of the triangles

								//material properties
		Vector4 color;			//color and opacity
		float roughness_factor;	//how smooth or rough is the surface
		float metallic_factor;	//how metallic is the surface
		Vector3 emissive_factor;//does this object emit light?

								//textures
		Texture* color_texture;	//base texture for color (must be modulated by the color factor)
		Texture* emissive_texture;//emissive texture (must be modulated by the emissive factor)
		Texture* metallic_roughness_texture;//occlusion, metallic and roughtness (in R, G and B)
		
		Texture* occlusion_texture;	//which areas receive ambient light
		Texture* normal_texture;//normalmap

								//ctors
		Material() : alpha_mode(NO_ALPHA), alpha_cutoff(0.5), color(1, 1, 1, 1), two_sided(false), roughness_factor(1), metallic_factor(0) {
			color_texture = emissive_texture = metallic_roughness_texture = occlusion_texture = normal_texture = NULL;
		}
		Material(Texture* texture) : Material() { color_texture = texture; }
		virtual ~Material();

		//render gui info inside the panel
		void renderInMenu();
	};
};