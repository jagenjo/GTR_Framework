//example of some shaders compiled
flat basic.vs flat.fs
texture basic.vs texture.fs
skybox basic.vs skybox.fs
depth quad.vs depth.fs
multi basic.vs multi.fs
light basic.vs light.fs
lightSinglePass basic.vs lightSinglePass.fs

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

uniform vec4 u_color;
uniform sampler2D u_texture;
uniform float u_time;
uniform float u_alpha_cutoff;

out vec4 FragColor;

void main()
{
	vec2 uv = v_uv;
	vec4 color = u_color;
	color *= texture( u_texture, v_uv );

	if(color.a < u_alpha_cutoff)
		discard;

	FragColor = color;
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



\light.fs

#version 330 core

// light defines
#define NO_LIGHT 0
#define POINT_LIGHT 1
#define SPOT_LIGHT 2
#define DIRECTIONAL_LIGHT 3

in vec3 v_position;
in vec3 v_world_position;
in vec3 v_normal;
in vec2 v_uv;
in vec4 v_color;

uniform vec3 u_camera_position;

// material properties
uniform vec4 u_color;

// REMEMBER -> although we call it factor, it is a vector3
uniform vec3 u_emissive_factor;
uniform vec3 u_metal_rough_factor; // x = metal, y = roughness, z = add specular?

// textures
uniform sampler2D u_albedo_texture;
uniform sampler2D u_emissive_texture;
uniform sampler2D u_occl_metal_rough_texture;
uniform sampler2D u_normal_map;

// global properties
uniform float u_time;
uniform float u_alpha_cutoff;

uniform vec3 u_ambient_light;

out vec4 FragColor;

// light variables
uniform vec4 u_light_info; // (type, near_dist, far_dist, use_normal_map?)
uniform vec3 u_light_position;
uniform vec3 u_light_front;
uniform vec3 u_light_color;

uniform vec2 u_light_cone; // cos(min_angle), cos(max_angle)

// shadowmap related
uniform mat4 u_shadow_viewproj;
uniform vec2 u_shadow_params;
uniform sampler2D u_shadowmap;

float testShadow( vec3 world_pos )
{
	// projection of 3d point to shadowmap
	vec4 proj_pos = u_shadow_viewproj * vec4(world_pos, 1.0);

	// from homogenous space to clip space
	vec2 shadow_uv = proj_pos.xy / proj_pos.w;

	//clip space -> uv space
	shadow_uv = shadow_uv * 0.5 + vec2(0.5);

	// check if the point is inside the shadowmap
	if( (shadow_uv.x < 0.0) || (shadow_uv.x > 1.0) 
		|| (shadow_uv.y < 0.0) || (shadow_uv.y > 1.0) )
		return 0.0;


	// get point depth [-1 ... +1] in non-linear space
	float real_depth = (proj_pos.z - u_shadow_params.y) / proj_pos.w;

	// normalize [-1 .. +1] -> [0..1]
	real_depth = real_depth * 0.5 + 0.5;

	float shadow_depth = texture(u_shadowmap, shadow_uv).x;

	float shadow_factor = 1.0;

	if (shadow_depth < real_depth)
		shadow_factor = 0.0;

	return shadow_factor;
}

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

 
vec3 perturbNormal(vec3 N, vec3 WP, vec2 uv, vec3 norm_pixel)
{
	norm_pixel = norm_pixel * 255./127. - 128./127.;
	mat3 TBN = cotangent_frame(N, WP, uv);
	return normalize(TBN * norm_pixel);
}

void main()
{
	vec2 uv = v_uv;

	vec3 N = normalize( v_normal );

	// w coordinate will either be 0 (no nmap) or 1
	if(int(u_light_info.w) > 0.0){
		vec3 n_pixel =  texture2D( u_normal_map, v_uv ).xyz; 
		N = perturbNormal( v_normal , v_world_position, v_uv, n_pixel);
	}

	vec4 albedo = u_color;
	albedo *= texture( u_albedo_texture, v_uv );

	if(albedo.a < u_alpha_cutoff)
		discard;

	// compute light
	vec3 light = vec3(0.0);

	float att = 1.0;

	// add ambient light considering occlusion, taking into account that occlusion texture is in the red channel (pos x)
	light +=  texture( u_occl_metal_rough_texture , v_uv).x * u_ambient_light;

	if(int(u_light_info.x) == NO_LIGHT){}

	else{
		vec3 L;

		if(int(u_light_info.x) == DIRECTIONAL_LIGHT)
		{
			float NdotL = dot(N, u_light_front);
			light += max(NdotL, 0.0) * u_light_color;

			att = max(NdotL, 0.0);
			L = normalize(u_light_front);
		}
		else
		{
			L = u_light_position - v_world_position;
			float dist = length(L);
			L /= dist;

			float NdotL = dot(N, L);
			att = max(0.0, (u_light_info.z - dist) / u_light_info.z);

			if(int(u_light_info.x) == SPOT_LIGHT){
				float cos_angle = dot( u_light_front , L);

				// check if inside max angle
				if( cos_angle < u_light_cone.y )
					att = 0.0;

				// inside min angle
				else if( cos_angle < u_light_cone.x )
					att *= 1.0 - (cos_angle - u_light_cone.x) / (u_light_cone.y - u_light_cone.x);
			}

			// quadratic attenuation
			att = att * att;

			// we can simply do max since both N and L are normal vectors
			light += max(NdotL, 0.0) * u_light_color *att;

		}
		
		// z coord of this factor indicates whether to add specular light (1) or not (0)
		if(u_metal_rough_factor.z > 0.0){
			vec3 v = normalize(u_camera_position - v_world_position);
			vec3 r = reflect(-L, N);
			float prod_s = max(0.0, dot(r,v));

			int shininess = int(1/(u_metal_rough_factor.y * texture( u_occl_metal_rough_texture , v_uv).z));
			prod_s = pow(prod_s, shininess);

			light += u_light_color * att * prod_s * u_metal_rough_factor.x * texture( u_occl_metal_rough_texture , v_uv).y;

		}
	}

	// apply light to the color
	vec3 color = light * albedo.xyz;
	color += u_emissive_factor * texture( u_emissive_texture, v_uv ).xyz;

	//color = vec3(u_metal_rough_factor.x);

	FragColor = vec4(color, albedo.a);
}

\lightSinglePass.fs

#version 330 core

#define MAX_LIGHTS 4

// light defines
#define NO_LIGHT 0
#define POINT_LIGHT 1
#define SPOT_LIGHT 2
#define DIRECTIONAL_LIGHT 3

in vec3 v_position;
in vec3 v_world_position;
in vec3 v_normal;
in vec2 v_uv;
in vec4 v_color;

uniform vec3 u_camera_position;

// material properties
uniform vec4 u_color;
// REMEMBER -> although we call it factor, it is a vector3
uniform vec3 u_emissive_factor;
uniform vec3 u_metal_rough_factor; // x = metal, y = roughness, z = add specular?

// textures
uniform sampler2D u_albedo_texture;
uniform sampler2D u_emissive_texture;
uniform sampler2D u_occl_metal_rough_texture;
uniform sampler2D u_normal_map;

// global properties
uniform float u_time;
uniform float u_alpha_cutoff;

uniform vec3 u_ambient_light;

out vec4 FragColor;

// light variables
uniform int u_num_lights;

uniform vec4 u_light_info[MAX_LIGHTS]; // (type, near_dist, far_dist, use_normal_map?)
uniform vec3 u_light_position[MAX_LIGHTS];
uniform vec3 u_light_front[MAX_LIGHTS];
uniform vec3 u_light_color[MAX_LIGHTS];

uniform vec2 u_light_cone[MAX_LIGHTS]; // cos(min_angle), cos(max_angle)

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

 
vec3 perturbNormal(vec3 N, vec3 WP, vec2 uv, vec3 norm_pixel)
{
	norm_pixel = norm_pixel * 255./127. - 128./127.;
	mat3 TBN = cotangent_frame(N, WP, uv);
	return normalize(TBN * norm_pixel);
}

void main()
{
	vec2 uv = v_uv;
	vec3 N = normalize( v_normal );

	// w coordinate will either be 0 (no nmap) or 1
	if(int(u_light_info[0].w) > 0.0){
		vec3 n_pixel =  texture2D( u_normal_map, v_uv ).xyz; 
		N = perturbNormal( v_normal , v_world_position, v_uv, n_pixel);
	}

	vec4 albedo = u_color;
	albedo *= texture( u_albedo_texture, v_uv );

	if(albedo.a < u_alpha_cutoff)
		discard;

	// compute light
	vec3 light = vec3(0.0);

	float att = 1.0;

	// add ambient light considering occlusion, taking into account that occlusion texture is in the red channel (pos x)
	light +=  texture( u_occl_metal_rough_texture , v_uv).x * u_ambient_light;

	for( int i = 0; i < MAX_LIGHTS; i++){
		if((i >= u_num_lights) || (int(u_light_info[i].x) == NO_LIGHT)){}
		
		else{
			vec3 L;
			if(int(u_light_info[i].x) == DIRECTIONAL_LIGHT)
			{
				float NdotL = dot(N, u_light_front[i]);
				light += max(NdotL, 0.0) * u_light_color[i];
				att = max(NdotL, 0.0);
				L = normalize(u_light_front[i]);
			}
			else
			{
				L = u_light_position[i] - v_world_position;
				float dist = length(L);
				L /= dist;

				float NdotL = dot(N, L);
				att = max(0.0, (u_light_info[i].z - dist) / u_light_info[i].z);

				if(int(u_light_info[i].x) == SPOT_LIGHT){
					float cos_angle = dot( u_light_front[i] , L);

					// check if inside max angle
					if( cos_angle < u_light_cone[i].y )
						att = 0.0;

					// inside min angle
					else if( cos_angle < u_light_cone[i].x )
						att *= 1.0 - (cos_angle - u_light_cone[i].x) / (u_light_cone[i].y - u_light_cone[i].x);
				}

				// quadratic attenuation
				att = att * att;

				// we can simply do max since both N and L are normal vectors
				light += max(NdotL, 0.0) * u_light_color[i] *att;

			}

			// z coord of this factor indicates whether to add specular light (1) or not (0)
			if(u_metal_rough_factor.z > 0.0){
				vec3 v = normalize(u_camera_position - v_world_position);
				vec3 r = reflect(-L, N);
				float prod_s = max(0.0, dot(r,v));

				int shininess = int(1/(u_metal_rough_factor.y * texture( u_occl_metal_rough_texture , v_uv).z));
				prod_s = pow(prod_s, shininess);

				light += u_light_color[i] * att * prod_s * u_metal_rough_factor.x * texture( u_occl_metal_rough_texture , v_uv).y;

			}

		}

	}

	// apply light to the color
	vec3 color = light * albedo.xyz;
	color += u_emissive_factor * texture( u_emissive_texture, v_uv ).xyz;

	//color = vec3(u_metal_rough_factor.x);

	FragColor = vec4(color, albedo.a);
}