//example of some shaders compiled
flat basic.vs flat.fs
texture basic.vs texture.fs
light basic.vs light.fs
skybox basic.vs skybox.fs
depth quad.vs depth.fs
multi basic.vs multi.fs
gbuffers basic.vs gbuffers.fs
deferred_global quad.vs deferred_global.fs
deferred_light quad.vs deferred_light.fs




\basic.vs

#version 330 core

in vec3 a_vertex;
in vec3 a_normal;
in vec2 a_coord;
in vec4 a_color;

uniform vec3 u_camera_pos;

uniform mat4 u_model;
uniform mat4 u_viewprojection;

//this will store the color for the pixel shader
out vec3 v_position;
out vec3 v_world_position;
out vec3 v_normal;
out vec2 v_uv;
out vec4 v_color;

uniform float u_time;

void main()
{	
	//calcule the normal in camera space (the NormalMatrix is like ViewMatrix but without traslation)
	v_normal = (u_model * vec4( a_normal, 0.0) ).xyz;
	
	//calcule the vertex in object space
	v_position = a_vertex;
	v_world_position = (u_model * vec4( v_position, 1.0) ).xyz;
	
	//store the color in the varying var to use it from the pixel shader
	v_color = a_color;

	//store the texture coordinates
	v_uv = a_coord;

	//calcule the position of the vertex using the matrices
	gl_Position = u_viewprojection * vec4( v_world_position, 1.0 );
}





\quad.vs

#version 330 core

in vec3 a_vertex;
in vec2 a_coord;
out vec2 v_uv;

void main()
{	
	v_uv = a_coord;
	gl_Position = vec4( a_vertex, 1.0 );
}






\deferred_global.fs

#version 330 core

in vec2 v_uv;

uniform sampler2D u_albedo_texture;
uniform sampler2D u_normal_texture;
uniform sampler2D u_emissive_texture;
uniform sampler2D u_depth_texture;

uniform vec3 u_ambient_light;


out vec4 FragColor;

void main()
{	

	float depth = texture(u_depth_texture, v_uv).x;
	if (depth == 1.0) discard;

	vec4 albedo = texture(u_albedo_texture, v_uv);	
	vec4 emissive = texture(u_emissive_texture, v_uv);

	//vec4 normal = texture(u_normal_texture, v_uv);
	//vec3 N = normalize (normal.xyz * 2.0 - vec3(1.0));

	vec4 color = vec4(0.0);

	color.xyz += emissive.xyz + u_ambient_light * albedo.xyz;

	FragColor = color;
	gl_FragDepth = depth;
}






\flat.fs

#version 330 core

uniform vec4 u_color;

out vec4 FragColor;

void main()
{
	FragColor = u_color;
}






\texture.fs

#version 330 core

in vec3 v_position;
in vec3 v_world_position;
in vec3 v_normal;
in vec2 v_uv;
in vec4 v_color;

uniform vec4 u_albedo_factor;
uniform sampler2D u_albedo_texture;
uniform float u_time;
uniform float u_alpha_cutoff;


out vec4 FragColor;

void main()
{
	vec2 uv = v_uv;
	vec4 color = u_albedo_factor;
	color *= texture( u_albedo_texture, v_uv );

	if(color.a < u_alpha_cutoff)
		discard;

	FragColor = color;
}






\gbuffers.fs

#version 330 core

in vec3 v_position;
in vec3 v_world_position;
in vec3 v_normal;
in vec2 v_uv;
in vec4 v_color;

uniform vec4 u_albedo_factor;
uniform vec3 u_emissive_factor;
uniform sampler2D u_albedo_texture;
uniform sampler2D u_emissive_texture;
uniform float u_time;
uniform float u_alpha_cutoff;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 NormalColor;
layout(location = 2) out vec4 ExtraColor; //for now only emissive


void main()
{
	vec2 uv = v_uv;
	vec4 color = u_albedo_factor;
	color *= texture( u_albedo_texture, v_uv );

	vec3 N = normalize(v_normal);

	if(color.a < u_alpha_cutoff)
		discard;

	vec3 emissive = u_emissive_factor * texture(u_emissive_texture, v_uv).xyz;

	FragColor = vec4(color.xyz, 1.0);
	NormalColor = vec4(N*0.5 + vec3(0.5), 1.0);
	ExtraColor = vec4(emissive, 1.0);
}






\light.fs

#version 330 core

in vec3 v_position;
in vec3 v_world_position;
in vec3 v_normal;
in vec2 v_uv;
in vec4 v_color;

uniform vec4 u_albedo_factor;
uniform vec3 u_emissive_factor;
uniform vec3 u_metallic_factor;
uniform vec3 u_roughness_factor;

uniform sampler2D u_albedo_texture;
uniform sampler2D u_emissive_texture;
uniform sampler2D u_normal_texture;
uniform sampler2D u_metallic_texture;

uniform vec3 u_ambient_light;

uniform float u_time;
uniform float u_alpha_cutoff;

#include "all_lights"

out vec4 FragColor;

mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv)
{
	// get edge vectors of the pixel triangle
	vec3 dp1 = dFdx( p );
	vec3 dp2 = dFdy( p );
	vec2 duv1 = dFdx( uv );
	vec2 duv2 = dFdy( uv );
	
	// solve the linear system
	vec3 dp2perp = cross( dp2, N );
	vec3 dp1perp = cross( N, dp1 );
	vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
	vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
 
	// construct a scale-invariant frame 
	float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
	return mat3( T * invmax, B * invmax, N );
}

void main()
{
	vec4 albedo = u_albedo_factor * texture( u_albedo_texture, v_uv );

	if(albedo.a < u_alpha_cutoff)
		discard;

	vec3 metallicness = texture(u_metallic_texture, v_uv).rgb * u_metallic_factor;
	vec3 roughness = texture(u_metallic_texture, v_uv).rgb * u_roughness_factor;

	vec3 normal_map = texture(u_normal_texture, v_uv).rgb;
	normal_map = normalize(normal_map * 2.0 - 1.0);

	vec3 N = normalize(v_normal);
	vec3 WP = v_world_position;

	mat3 TBN = cotangent_frame(N, WP, v_uv);
	vec3 normal = normalize(TBN * normal_map);
	
	vec3 light = vec3(0.0);
	light += u_ambient_light;

	float shadow_factor = 1.0;
	if(u_shadow_param.x != 0.0) shadow_factor = testShadow(v_world_position);

	if (int(u_light_info.x) == DIRECTIONAL_LIGHT)
	{
		float NdotL = dot(normal, u_light_front);
		light += max(NdotL, 0.0)  * u_light_color;
	}
	else if (int(u_light_info.x) == POINT_LIGHT || int(u_light_info.x) == SPOT_LIGHT)
	{
		vec3 L = u_light_position - v_world_position;
		float dist = length(L);
		L /= dist; //to normalize L

		float NdotL = dot(normal, L);
		float attenuation = max(0.0, (u_light_info.z - dist) / u_light_info.z);

		if (int(u_light_info.x) == SPOT_LIGHT)
		{
			float cos_angle = dot(u_light_front, L);
			if (cos_angle < u_light_cone.y) attenuation = 0.0;
			else if (cos_angle < u_light_cone.x) attenuation *= 1.0 - (cos_angle - u_light_cone.x) / (u_light_cone.y - u_light_cone.x);
		}

		light += max(NdotL, 0.0)  * u_light_color * attenuation;
	}

	vec3 color = albedo.rgb * light * shadow_factor; //*specular
	color += u_emissive_factor * texture( u_emissive_texture, v_uv ).xyz; 
	
	FragColor = vec4(color, albedo.a);
}





\deferred_light.fs

#version 330 core

in vec2 v_uv;

uniform sampler2D u_albedo_texture;
uniform sampler2D u_normal_texture;
uniform sampler2D u_emissive_texture;
uniform sampler2D u_depth_texture;

uniform vec3 u_ambient_light;

#include "all_lights"

uniform mat4 u_ivp;
uniform vec2 u_iRes;

out vec4 FragColor;

void main()
{	

	//vec2 uv = v_uv;
	vec2 uv = gl_FragCoord.xy * u_iRes.xy;

	float depth = texture(u_depth_texture, v_uv).x;
	if (depth == 1.0) discard;

	vec4 screen_pos = vec4(uv.x * 2.0 - 1.0, uv.y * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
	vec4 world_pos_proj = u_ivp * screen_pos;
	vec3 world_pos = world_pos_proj.xyz / world_pos_proj.w;

	vec4 albedo = texture(u_albedo_texture, uv);	
	vec4 emissive = texture(u_emissive_texture, uv);

	//vec4 normal = texture(u_normal_texture, uv);
	//vec3 N = normalize (normal.xyz * 2.0 - vec3(1.0));

	vec4 color = vec4(0.0, 0.0, 0.0, 1.0);

	//color.xyz += emissive.xyz + u_ambient_light * albedo.xyz;
	color.xyz = mod(abs(world_pos * 0.01), vec3(1.0)); //to show world pos as a color

	FragColor = color;
	gl_FragDepth = depth;
}




\skybox.fs

#version 330 core

in vec3 v_position;
in vec3 v_world_position;

uniform samplerCube u_texture;
uniform vec3 u_camera_position;
out vec4 FragColor;

void main()
{
	vec3 E = v_world_position - u_camera_position;
	vec4 color = texture( u_texture, E );
	FragColor = color;
}







\multi.fs

#version 330 core

in vec3 v_position;
in vec3 v_world_position;
in vec3 v_normal;
in vec2 v_uv;

uniform vec4 u_color;
uniform sampler2D u_texture;
uniform float u_time;
uniform float u_alpha_cutoff;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 NormalColor;

void main()
{
	vec2 uv = v_uv;
	vec4 color = u_color;
	color *= texture( u_texture, uv );

	if(color.a < u_alpha_cutoff)
		discard;

	vec3 N = normalize(v_normal);

	FragColor = color;
	NormalColor = vec4(N,1.0);
}







\depth.fs

#version 330 core

uniform vec2 u_camera_nearfar;
uniform sampler2D u_texture; //depth map
in vec2 v_uv;
out vec4 FragColor;

void main()
{
	float n = u_camera_nearfar.x;
	float f = u_camera_nearfar.y;
	float z = texture2D(u_texture,v_uv).x;
	if( n == 0.0 && f == 1.0 )
		FragColor = vec4(z);
	else
		FragColor = vec4( n * (z + 1.0) / (f + n - z * (f - n)) );
}







\instanced.vs

#version 330 core

in vec3 a_vertex;
in vec3 a_normal;
in vec2 a_coord;

in mat4 u_model;

uniform vec3 u_camera_pos;

uniform mat4 u_viewprojection;

//this will store the color for the pixel shader
out vec3 v_position;
out vec3 v_world_position;
out vec3 v_normal;
out vec2 v_uv;

void main()
{	
	//calcule the normal in camera space (the NormalMatrix is like ViewMatrix but without traslation)
	v_normal = (u_model * vec4( a_normal, 0.0) ).xyz;
	
	//calcule the vertex in object space
	v_position = a_vertex;
	v_world_position = (u_model * vec4( a_vertex, 1.0) ).xyz;
	
	//store the texture coordinates
	v_uv = a_coord;

	//calcule the position of the vertex using the matrices
	gl_Position = u_viewprojection * vec4( v_world_position, 1.0 );
}


\all_lights

//light_type
#define NO_LIGHT 0
#define POINT_LIGHT 1
#define SPOT_LIGHT 2
#define DIRECTIONAL_LIGHT 3

uniform vec4 u_light_info; //light_type, near_distance, max_distance, 0
uniform vec3 u_light_position;
uniform vec3 u_light_front;
uniform vec3 u_light_color;
uniform vec2 u_light_cone; //cos(min_angle), cos(max_angle)

uniform vec2 u_shadow_param; //(0 or 1 in there is shadowmap, bias)
uniform mat4 u_shadow_viewproj;
uniform sampler2D u_shadowmap;

float testShadow(vec3 pos)
{
	vec4 proj_pos = u_shadow_viewproj * vec4(pos, 1.0);
	vec2 shadow_uv = (proj_pos.xy / proj_pos.w) * 0.5 + vec2(0.5);

	if( shadow_uv.x < 0.0 || shadow_uv.x > 1.0 || shadow_uv.y < 0.0 || shadow_uv.y > 1.0 ) return 1.0;	

	float real_depth = (proj_pos.z - u_shadow_param.y) / proj_pos.w;
	real_depth = real_depth * 0.5 + 0.5;

	if( real_depth < 0.0 || real_depth > 1.0) return 1.0;

	float shadow_depth = texture( u_shadowmap, shadow_uv).x;

	float shadow_factor = 1.0;
	if( shadow_depth < real_depth )	shadow_factor = 0.0;

	return shadow_factor;
}
