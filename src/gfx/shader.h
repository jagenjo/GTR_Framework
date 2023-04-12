/*  by Javi Agenjo 2013 UPF  javi.agenjo@gmail.com
	This allows to use compile and use shaders when rendering. Used for advanced lighting.
*/

#pragma once

#include <string>
#include <map>
#include <cassert>

#include "../core/includes.h"
#include "../core/math.h"
#include "gfx.h"


#ifdef _DEBUG
	#define CHECK_SHADER_VAR(a,b) if (a == -1) return
	//#define CHECK_SHADER_VAR(a,b) if (a == -1) { std::cout << "Shader error: Var not found in shader: " << b << std::endl; return; } 
#else
	#define CHECK_SHADER_VAR(a,b) if (a == -1) return
#endif

namespace GFX {

	class Texture;
	class UBO;

	class Shader
	{
		int last_slot;

		static bool s_ready; //used to initialize shader vars

	public:
		static Shader* current;

		Shader();
		~Shader();

		void setFilenames(const std::string& vsf, const std::string& psf); //set but not compile
		bool compile();
		bool recompile();

		bool load(const std::string& vsf, const std::string& psf, const char* macros);

		//internal functions
		bool compileFromMemory(const std::string& vsm, const std::string& psm);
		void release();
		void enable();
		void disable();

		static void init();
		static void disableShaders();

		//check
		bool IsUniform(const char* varname) { return (getUniformLocation(varname) != -1); } //uniform exist
		bool IsAttribute(const char* varname) { return (getAttribLocation(varname) != -1); } //attribute exist

		//upload
		void setUniform(const char* varname, bool input) { assert(current == this); setUniform1(varname, input); }
		void setUniform(const char* varname, int input) { assert(current == this); setUniform1(varname, input); }
		void setUniform(const char* varname, float input) { assert(current == this); setUniform1(varname, input); }
		void setUniform(const char* varname, const Vector2f& input) { assert(current == this); setUniform2(varname, input.x, input.y); }
		void setUniform(const char* varname, const Vector3f& input) { assert(current == this); setUniform3(varname, input.x, input.y, input.z); }
		void setUniform(const char* varname, const Vector4f& input) { assert(current == this); setUniform4(varname, input.x, input.y, input.z, input.w); }
		void setUniform(const char* varname, const Matrix44& input) { assert(current == this); setMatrix44(varname, input); }
		void setUniform(const char* varname, std::vector<Matrix44>& m_vector) { assert(current == this && m_vector.size()); setMatrix44Array(varname, &m_vector[0], m_vector.size()); }

		//for textures you must specify an slot (a number from 0 to 16) where this texture is stored in the shader
		void setUniform(const char* varname, Texture* texture, int slot) { assert(current == this); setTexture(varname, texture, slot); }


		void setInt(const char* varname, const int& input) { setUniform1(varname, input); }
		void setFloat(const char* varname, const float& input) { setUniform1(varname, input); }
		void setVector3(const char* varname, const Vector3f& input) { setUniform3(varname, input.x, input.y, input.z); }
		void setMatrix44(const char* varname, const float* m);
		void setMatrix44(const char* varname, const Matrix44& m);
		void setMatrix44Array(const char* varname, Matrix44* m_array, int num);

		void setUniform1Array(const char* varname, const float* input, const int count);
		void setUniform2Array(const char* varname, const float* input, const int count);
		void setUniform3Array(const char* varname, const float* input, const int count);
		void setUniform4Array(const char* varname, const float* input, const int count);

		void setUniform1Array(const char* varname, const int* input, const int count);
		void setUniform2Array(const char* varname, const int* input, const int count);
		void setUniform3Array(const char* varname, const int* input, const int count);
		void setUniform4Array(const char* varname, const int* input, const int count);

		void setUniform1(const char* varname, const bool input1);

		void setUniform1(const char* varname, const int input1);
		void setUniform2(const char* varname, const int input1, const int input2);
		void setUniform3(const char* varname, const int input1, const int input2, const int input3);
		void setUniform3(const char* varname, const Vector3f& input) { setUniform3(varname, input.x, input.y, input.z); }
		void setUniform4(const char* varname, const int input1, const int input2, const int input3, const int input4);

		void setUniform1(const char* varname, const float input);
		void setUniform2(const char* varname, const float input1, const float input2);
		void setUniform3(const char* varname, const float input1, const float input2, const float input3);
		void setUniform4(const char* varname, const Vector4f& input) { setUniform4(varname, input.x, input.y, input.z, input.w); }
		void setUniform4(const char* varname, const float input1, const float input2, const float input3, const float input4);

		//void setTexture(const char* varname, const unsigned int tex) ;
		void setTexture(const char* varname, Texture* texture, int slot);

		int getAttribLocation(const char* varname);
		int getUniformLocation(const char* varname);
		int getUniformBlockLocation(const char* varname);

		std::string getInfoLog() const;
		bool hasInfoLog() const;

		void setMacros(const char* macros);

		static Shader* Get(const char* vsf, const char* psf = NULL, const char* macros = NULL);
		static void ReloadAll();
		static std::map<std::string, Shader*> s_Shaders;

		std::string vs_filename;
		std::string fs_filename;
		std::string macros;
		bool compiled;
		bool from_atlas;

		GLuint vs;
		GLuint fs;
		GLuint cs; //compute
		GLuint program;
		std::string info_log;
		std::string log;

		static std::vector<char> lines_with_error;//used for debug

		bool createVertexShaderObject(const std::string& shader);
		bool createFragmentShaderObject(const std::string& shader);
		bool createComputeShaderObject(const std::string& shader); //not used yet
		bool createShaderObject(unsigned int type, GLuint& handle, const std::string& shader);
		void saveShaderInfoLog(GLuint obj);
		void saveProgramInfoLog(GLuint obj);

		bool validate();

		//This is to speed up shader usage (save locations locally)
		//HACK: uses original const char* value instead of string because uniform names will always come from const char* inside the code (are we sure??)
		//const string comparator, allowing const char loctables
		struct ltstr { bool operator()(const char* s1, const char* s2) const { return strcmp(s1, s2) < 0; } };
		typedef std::map<const char*, int, ltstr> loctable;
		GLint getLocation(const char* varname, bool is_block = false);
		loctable locations;

		//Shader Atlas stuff ************************
		//to know more about the file format, it is based in this https://github.com/jagenjo/rendeer.js/tree/master/guides#the-shaders but with tiny differences
		//this is a way to load a single file that contains all the shaders 
		static std::string s_shader_atlas_filename;
		static std::map<std::string, std::string> s_shader_files; //stores strings with shadercode

		//compiles and stores shader, if exist it will recompile it!
		static Shader* CompileShader(const char* name, const char* vs_code, const char* fs_code, const char* macros);
		static std::string ExpandIncludes(std::string name, std::string content, std::map<std::string, std::string>& subfiles, const std::string& base_path);
		static bool LoadAtlas(const char* filename, const char* base_path = nullptr);
		static bool GetShaderFile(const char* filename, std::string& content);

		//UberShaders allow permutations, use @ as the first char in the name to specify it
		class UberShader {
		public:
			std::string name;
			std::string vs_name;
			std::string fs_name;
			std::vector<std::string> macros;
			std::map<std::string,int> macros_index;
			std::map<uint64,Shader*> compiled_shaders;
			UberShader(std::string name, std::string vs_name, std::string fs_name, std::vector<std::string> macros) {
				this->name = name, this->vs_name = vs_name, this->fs_name = fs_name, this->macros = macros;
				for (size_t i = 0; i < macros.size(); ++i) 
					macros_index[ macros[i] ] = i;
			}
			Shader* get(uint64 macros);
			void clear();
			int getMacroIndex(const char* name) { auto it = macros_index.find(name); return it == macros_index.end() ? -1 : it->second; }
		};
		static std::map<std::string, UberShader*> s_ubershaders;
		static UberShader* GetUberShader(const char* name);

		static Shader* getDefaultShader(std::string name);
	};

	//Frontend for Uniform Buffer Objects or Shared Storage Buffer Objects
	//UBOs: from here https://paroj.github.io/gltut/Positioning/Tut07%20Shared%20Uniforms.html
	//SSBOs: from here https://www.khronos.org/opengl/wiki/Shader_Storage_Buffer_Object
	class BufferObject {
	public:
		GLuint type;
		GLuint id;
		size_t size;
		std::string name;
		BufferObject();
		BufferObject(const char* name);
		~BufferObject();
		void allocate(int size);
		void deallocate();
		template <typename T>
		void update(const T& obj) { updateFromPointer(&obj, sizeof(T)); }
		void updateFromPointer(const void* data, int size);
		//the global index behaves similar to slots in textures, you bind a UBO to an index, and a block to the same index
		void bind(Shader* shader, int global_index, int start = 0, int length = -1);
	};

};