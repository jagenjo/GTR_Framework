#pragma once

#include "framework.h"
#include <cassert>
#include <map>
#include <string>

//forward declaration
class Mesh;
class Texture;

namespace GTR {

	enum eAlphaMode {
		NO_ALPHA,
		MASK,
		BLEND
	};

	
	enum eChannels {
		ALBEDO,
		NORMAL,
		EMISSIVE,
		DEPTH,
		OCCLUSION,
		METALLICROUGHNESS,
		OPACITY,
		DISPLACEMENT,
        PROBE,
		DEPTH_SHADOW
	};

	struct Sampler {
		Texture* texture;
		int uv_channel;

		Sampler() { texture = NULL; uv_channel = 0; }
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
		eAlphaMode alpha_mode;	//could be NO_ALPHA, MASK (alpha cut) or BLEND (alpha blend)
		float alpha_cutoff;		//pixels with alpha than this value shouldnt be rendered

		
		bool two_sided;			//render both faces of the triangles

		float _zMin, _zMax;       // Z-Range

								//material properties
		Vector4 color;			//color and opacity
		float roughness_factor;	//how smooth or rough is the surface
		float metallic_factor;	//how metallic is the surface
		Vector3 emissive_factor;//does this object emit light?

								//textures
		Sampler color_texture;	//base texture for color (must be modulated by the color factor)
		Sampler emissive_texture;//emissive texture (must be modulated by the emissive factor)
		Sampler opacity_texture;
		Sampler metallic_roughness_texture;//occlusion, metallic and roughtness (in R, G and B)
		Sampler occlusion_texture;	//which areas receive ambient light
		Sampler normal_texture;	//normalmap

		//ctors
		Material() : alpha_mode(NO_ALPHA), alpha_cutoff(0.5), color(1, 1, 1, 1), _zMin(0.0f), _zMax(1.0f), two_sided(false), roughness_factor(1), metallic_factor(0) {
			//color_texture = emissive_texture = metallic_roughness_texture = occlusion_texture = normal_texture = NULL;
		}
		Material(Texture* texture) : Material() { 
			color_texture.texture = texture; }

		virtual ~Material();

		static void Release();

		void renderInMenu();


	};

};
