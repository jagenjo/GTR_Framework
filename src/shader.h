/*  by Javi Agenjo 2013 UPF  javi.agenjo@gmail.com
	This allows to use compile and use shaders when rendering. Used for advanced lighting.
*/

#ifndef SHADER_H
#define SHADER_H

#include "includes.h"
#include <string>
#include <map>
#include "framework.h"
#include <cassert>

#ifdef _DEBUG
	#define CHECK_SHADER_VAR(a,b) if (a == -1) return
	//#define CHECK_SHADER_VAR(a,b) if (a == -1) { std::cout << "Shader error: Var not found in shader: " << b << std::endl; return; } 
#else
	#define CHECK_SHADER_VAR(a,b) if (a == -1) return
#endif

class Texture;

class Shader
{
	int last_slot;

	static bool s_ready; //used to initialize shader vars

public:
	static Shader* current;

	Shader();
	virtual ~Shader();

	virtual void setFilenames(const std::string& vsf, const std::string& psf); //set but not compile
	virtual bool compile();
	virtual bool recompile();

	virtual bool load(const std::string& vsf, const std::string& psf, const char* macros);

	//internal functions
	virtual bool compileFromMemory(const std::string& vsm, const std::string& psm);
	virtual void release();
	virtual void enable();
	virtual void disable();

	static void init();
	static void disableShaders();

	//check
	virtual bool IsUniform(const char* varname) { return (getUniformLocation(varname) != -1); } //uniform exist
	virtual bool IsAttribute(const char* varname) { return (getAttribLocation(varname) != -1); } //attribute exist

	//upload
	void setUniform(const char* varname, bool input) { assert(current == this); setUniform1(varname, input); }
	void setUniform(const char* varname, int input) { assert(current == this); setUniform1(varname, input); }
	void setUniform(const char* varname, float input) { assert(current == this); setUniform1(varname, input); }
	void setUniform(const char* varname, const Vector2& input) { assert(current == this); setUniform2(varname, input.x, input.y ); }
	void setUniform(const char* varname, const Vector3& input) { assert(current == this); setUniform3(varname, input.x, input.y, input.z); }
	void setUniform(const char* varname, const Vector4& input) { assert(current == this); setUniform4(varname, input.x, input.y, input.z, input.w); }
	void setUniform(const char* varname, const Matrix44& input) { assert(current == this); setMatrix44(varname, input); }
	void setUniform(const char* varname, std::vector<Matrix44>& m_vector) { assert(current == this && m_vector.size()); setMatrix44Array(varname, &m_vector[0], m_vector.size()); }
	
	//for textures you must specify an slot (a number from 0 to 16) where this texture is stored in the shader
	void setUniform(const char* varname, Texture* texture, int slot) { assert(current == this); setTexture(varname, texture, slot); }


	virtual void setInt(const char* varname, const int& input) { setUniform1(varname, input); }
	virtual void setFloat(const char* varname, const float& input) { setUniform1(varname, input); }
	virtual void setVector3(const char* varname, const Vector3& input) { setUniform3(varname, input.x, input.y, input.z); }
	virtual void setMatrix44(const char* varname, const float* m);
	virtual void setMatrix44(const char* varname, const Matrix44 &m);
	virtual void setMatrix44Array(const char* varname, Matrix44* m_array, int num);

	virtual void setUniform1Array(const char* varname, const float* input, const int count) ;
	virtual void setUniform2Array(const char* varname, const float* input, const int count) ;
	virtual void setUniform3Array(const char* varname, const float* input, const int count) ;
	virtual void setUniform4Array(const char* varname, const float* input, const int count) ;

	virtual void setUniform1Array(const char* varname, const int* input, const int count) ;
	virtual void setUniform2Array(const char* varname, const int* input, const int count) ;
	virtual void setUniform3Array(const char* varname, const int* input, const int count) ;
	virtual void setUniform4Array(const char* varname, const int* input, const int count) ;

	virtual void setUniform1(const char* varname, const bool input1);

	virtual void setUniform1(const char* varname, const int input1) ;
	virtual void setUniform2(const char* varname, const int input1, const int input2) ;
	virtual void setUniform3(const char* varname, const int input1, const int input2, const int input3) ;
	virtual void setUniform3(const char* varname, const Vector3& input) { setUniform3(varname, input.x, input.y, input.z); }
	virtual void setUniform4(const char* varname, const int input1, const int input2, const int input3, const int input4) ;

	virtual void setUniform1(const char* varname, const float input) ;
	virtual void setUniform2(const char* varname, const float input1, const float input2) ;
	virtual void setUniform3(const char* varname, const float input1, const float input2, const float input3) ;
	virtual void setUniform4(const char* varname, const Vector4& input) { setUniform4(varname, input.x, input.y, input.z, input.w); }
	virtual void setUniform4(const char* varname, const float input1, const float input2, const float input3, const float input4) ;

	//virtual void setTexture(const char* varname, const unsigned int tex) ;
	virtual void setTexture(const char* varname, Texture* texture, int slot);

	virtual int getAttribLocation(const char* varname);
	virtual int getUniformLocation(const char* varname);

	std::string getInfoLog() const;
	bool hasInfoLog() const;
	bool compiled;

	void setMacros(const char * macros);

	static Shader* Get(const char* vsf, const char* psf = NULL, const char* macros = NULL);
	static void ReloadAll();
	static std::map<std::string,Shader*> s_Shaders;

	//this is a way to load a single file that contains all the shaders 
	//to know more about the file format, it is based in this https://github.com/jagenjo/rendeer.js/tree/master/guides#the-shaders but with tiny differences
	static bool LoadAtlas(const char* filename);
	static std::string s_shader_atlas_filename;
	static std::map<std::string, std::string> s_shaders_atlas; //stores strings, no shaders

	static Shader* getDefaultShader(std::string name);

protected:

	std::string info_log;
	std::string vs_filename;
	std::string ps_filename;
	std::string macros;
	bool from_atlas;

	bool createVertexShaderObject(const std::string& shader);
	bool createFragmentShaderObject(const std::string& shader);
	bool createShaderObject(unsigned int type, GLuint& handle, const std::string& shader);
	void saveShaderInfoLog(GLuint obj);
	void saveProgramInfoLog(GLuint obj);

	bool validate();

	GLuint vs;
	GLuint fs;
	GLuint program;
	std::string log;

//this is a hack to speed up shader usage (save info locally)
private: 

	struct ltstr
	{
		bool operator()(const char* s1, const char* s2) const
		{
			return strcmp(s1, s2) < 0;
		}
	};	
	typedef std::map<const char*, int, ltstr> loctable;

public:
	GLint getLocation( const char* varname, loctable* table );
	loctable locations;	
};

#endif