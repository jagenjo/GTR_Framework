#pragma once

#include "../core/math.h"
#include <cassert>
#include <map>
#include <string>

//forward declaration
namespace GFX
{
	class Mesh;
	class Texture;
}

namespace SCN {

	enum eAlphaMode {
		NO_ALPHA,
		MASK,
		BLEND
	};

	struct Sampler {
		GFX::Texture* texture;
		int uv_channel;

		Sampler() { texture = NULL; uv_channel = 0; }
	};

	enum eTextureChannel {
		ALBEDO,
		EMISSIVE,
		OPACITY,
		METALLIC_ROUGHNESS,
		OCCLUSION,
		NORMALMAP,
		ALL
	};

	extern const char* texture_channel_str[];

	//this class contains all info relevant of how something must be rendered
	class Material {
	public:

		//static manager to reuse materials
		static std::map<std::string, Material*> sMaterials;
		static Material* Get(const char* name);
		static uint32 s_last_index;
		static Material default_material;
		std::string name;
		uint32 index;
		void registerMaterial(const char* name);

		//parameters to control transparency
		eAlphaMode alpha_mode;	//could be NO_ALPHA, MASK (alpha cut) or BLEND (alpha blend)
		float alpha_cutoff;		//pixels with alpha than this value shouldnt be rendered
		bool two_sided;			//render both faces of the triangles

								//material properties
		Vector4f color;			//color and opacity
		float roughness_factor;	//how smooth or rough is the surface
		float metallic_factor;	//how metallic is the surface
		Vector3f emissive_factor;//does this object emit light?

		//textures
		Sampler textures[eTextureChannel::ALL];

		//ctors
		Material() : alpha_mode(NO_ALPHA), alpha_cutoff(0.5), color(1, 1, 1, 1), two_sided(false), roughness_factor(1), metallic_factor(0) {
			//color_texture = emissive_texture = metallic_roughness_texture = occlusion_texture = normal_texture = NULL;
			index = s_last_index++;
		}
		virtual ~Material();

		static void Release();
	};
};