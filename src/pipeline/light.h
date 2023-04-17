#pragma once

#include "scene.h"

namespace SCN {

	enum eLightType : uint32 {
		NO_LIGHT = 0,
		POINT = 1,
		SPOT = 2,
		DIRECTIONAL = 3
	};

	//internal shader data (16 bits aligned)
	//#pragma pack(push, 16)
	struct sLightData {
		vec4 position; //pos,max_distance
		vec4 color; //(max_distance,attenuation)
		vec4 params; //type, SPOT=(,min_angle, max_angle)
		vec4 front;
		vec4 shadow_params; //cast_shadows, bias, ...
		vec4 shadow_area; //start, end inside atlas
		mat4 shadowmap_vp;
	};
	//#pragma pack(pop)

	class LightEntity : public BaseEntity
	{
	public:

		eLightType light_type;
		float intensity;
		vec3 color;
		float near_distance;
		float max_distance;
		bool cast_shadows;
		float shadow_bias;
		vec2 cone_info;
		float area; //for direct;

		sLightData light_data; //for internal 
		//Texture* cookie;

		ENTITY_METHODS(LightEntity, LIGHT, 14,4);

		LightEntity();

		void configure(cJSON* json);
		void serialize(cJSON* json);
	};

};
