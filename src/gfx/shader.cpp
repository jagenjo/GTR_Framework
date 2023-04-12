#include "shader.h"
#include <cassert>
#include <iostream>
#include "../utils/utils.h"
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

#include "texture.h"

//typedef unsigned int GLhandle;

#ifdef LOAD_EXTENSIONS_MANUALLY

	REGISTER_GLEXT( GLhandle, glCreateProgramObject, void )
	REGISTER_GLEXT( void, glLinkProgram, GLhandle programObj )
	REGISTER_GLEXT( void, glGetObjectParameteriv, GLhandle obj, GLenum pname, GLint *params )
	REGISTER_GLEXT( void, glValidateProgram, GLhandle obj )
	REGISTER_GLEXT( GLhandle, glCreateShaderObject, GLenum shaderType )
	REGISTER_GLEXT( void, glShaderSource, GLhandle shaderObj, GLsizei count, const GLchar* *string, const GLint *length)
	REGISTER_GLEXT( void, glCompileShader, GLhandle shaderObj )
	REGISTER_GLEXT( void, glAttachObject, GLhandle containerObj, GLhandle obj )
	REGISTER_GLEXT( void, glDetachObject, GLhandle containerObj, GLhandle attachedObj )
	REGISTER_GLEXT( void, glDeleteObject, GLhandle obj )
	REGISTER_GLEXT( void, glUseProgramObject, GLhandle obj )

	REGISTER_GLEXT( void, glActiveTexture, GLenum texture )
	REGISTER_GLEXT( void, glGetInfoLog, GLhandle obj, GLsizei maxLength, GLsizei *length, GLchar *infoLog )
	REGISTER_GLEXT( GLint, glGetUniformLocation, GLhandle programObj, const GLchar *name)
	REGISTER_GLEXT( GLint, glGetAttribLocation, GLhandle programObj, const GLchar *name)
	REGISTER_GLEXT( void, glUniform1i, GLint location, GLint v0 )
	REGISTER_GLEXT( void, glUniform2i, GLint location, GLint v0, GLint v1 )
	REGISTER_GLEXT( void, glUniform3i, GLint location, GLint v0, GLint v1, GLint v2 )
	REGISTER_GLEXT( void, glUniform4i, GLint location, GLint v0, GLint v1, GLint v2, GLint v3 )
	REGISTER_GLEXT( void, glUniform1iv, GLint location, GLsizei count, const GLint *value )
	REGISTER_GLEXT( void, glUniform2iv, GLint location, GLsizei count, const GLint *value )
	REGISTER_GLEXT( void, glUniform3iv, GLint location, GLsizei count, const GLint *value )
	REGISTER_GLEXT( void, glUniform4iv, GLint location, GLsizei count, const GLint *value )
	REGISTER_GLEXT( void, glUniform1f, GLint location, GLfloat v0 )
	REGISTER_GLEXT( void, glUniform2f, GLint location, GLfloat v0, GLfloat v1)
	REGISTER_GLEXT( void, glUniform3f, GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
	REGISTER_GLEXT( void, glUniform4f, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
	REGISTER_GLEXT( void, glUniform1fv, GLint location, GLsizei count, const GLfloat *value)
	REGISTER_GLEXT( void, glUniform2fv, GLint location, GLsizei count, const GLfloat *value)
	REGISTER_GLEXT( void, glUniform3fv, GLint location, GLsizei count, const GLfloat *value)
	REGISTER_GLEXT( void, glUniform4fv, GLint location, GLsizei count, const GLfloat *value)
	REGISTER_GLEXT( void, glUniformMatrix4fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value )

#endif

namespace GFX {

std::string Shader::s_shader_atlas_filename;
std::map<std::string, std::string> Shader::s_shader_files;
std::map<std::string, Shader::UberShader*> Shader::s_ubershaders;

std::map<std::string,Shader*> Shader::s_Shaders;
bool Shader::s_ready = false;
Shader* Shader::current = NULL;
std::vector<char> Shader::lines_with_error;

Shader::Shader()
{
	if(!Shader::s_ready)
		Shader::init();
	program = vs = fs = cs = 0;
	compiled = false;
	from_atlas = false;

}

Shader::~Shader()
{
	release();
}

void Shader::setFilenames(const std::string& vsf, const std::string& psf)
{
	vs_filename = vsf;
	fs_filename = psf;
}

bool Shader::load(const std::string& vsf, const std::string& psf, const char* macros)
{
	assert(	compiled == false );
	assert (glGetError() == GL_NO_ERROR);

	vs_filename = vsf;
	fs_filename = psf;
	from_atlas = false;

	bool printMacros = false;

    std::cout << " + Shader: Vertex: " << vsf << "  Pixel: " << psf << "  " << (macros && printMacros ? macros : "") << std::endl;
	std::string vsm,psm;
	if (!readFile(vsf,vsm) || !readFile(psf,psm))
		return false;

	//printf("Vertex shader from memory:\n%s\n", vsm.c_str());
	//printf("Fragment shader from memory:\n%s\n", psm.c_str());
	if (macros)
	{
		vsm = macros + vsm;
		psm = macros + psm;
		this->macros = macros;
	}

	if (!compileFromMemory(vsm,psm))
		return false;

	assert (glGetError() == GL_NO_ERROR);

	return true;
}

Shader* Shader::Get(const char* vsf, const char* psf, const char* macros)
{
	std::string name;
	
	if (psf)
		name = std::string(vsf) + "," + std::string(psf ? psf : "") + (macros ? macros : "");
	else
		name = vsf;
	std::map<std::string,Shader*>::iterator it = s_Shaders.find(name);
	if (it != s_Shaders.end())
		return it->second;

	if (!psf)
		return NULL;

	Shader* sh = new Shader();
	if (!sh->load( vsf,psf, macros ))
		return NULL;
	s_Shaders[name] = sh;
	return sh;
}

void Shader::ReloadAll()
{
	for( std::map<std::string,Shader*>::iterator it = s_Shaders.begin(); it!=s_Shaders.end();it++)
		it->second->recompile();
	if(!s_shader_atlas_filename.empty())
		LoadAtlas(s_shader_atlas_filename.c_str());
	std::cout << "Shaders recompiled" << std::endl;
}

//functions to trim strings
static inline std::string trim(std::string str) {
	size_t startpos = str.find_first_not_of(" \t\r\n");
	if( std::string::npos != startpos && startpos > 0)
	    str = str.substr( startpos );
	size_t endpos = str.find_last_not_of(" \t\r\n");
	if( std::string::npos != endpos )
	    str = str.substr( 0, endpos+1 );
	if( std::string::npos == startpos && std::string::npos == endpos)
		return "";
	return str;
}

void Shader::setMacros(const char* macros)
{
	this->macros = macros;
	this->recompile();
}

bool Shader::compile()
{
	assert(!compiled && "Shader already compiled" );
    return load( vs_filename, fs_filename, macros.size() ? macros.c_str() : NULL);
}

bool Shader::recompile()
{ 
	if (from_atlas || !vs_filename.size() || !fs_filename.size() ) //shaders compiled from memory cannot be recompiled
		return false;
	release(); //remove old shader
    return load( vs_filename, fs_filename, macros.size() ? macros.c_str() : NULL );
}

std::string Shader::getInfoLog() const
{
	return info_log;
}

bool Shader::hasInfoLog() const
{
	return info_log.size() > 0; 
}

// ******************************************

bool Shader::compileFromMemory(const std::string& vsm, const std::string& psm)
{
	if (glCreateProgram == 0)
	{
		std::cout << "Error: your graphics cards dont support shaders. Sorry." << std::endl;
		exit(0);
	}

	if (program != 0)
		glDeleteProgram(program);
	program = glCreateProgram();
	assert (glGetError() == GL_NO_ERROR);

	if (!createVertexShaderObject(vsm))
	{
		printf("Vertex shader compilation failed\n");
		return false;
	}

	if (!createFragmentShaderObject(psm))
	{
		printf("Fragment shader compilation failed\n");
		return false;
	}

	glLinkProgram(program);
	assert (glGetError() == GL_NO_ERROR);

	GLint linked=0;
    
	glGetProgramiv(program,GL_LINK_STATUS,&linked);
	assert(glGetError() == GL_NO_ERROR);

	if (!linked)
	{
		saveProgramInfoLog(program);
		release();
		return false;
	}

#ifdef _DEBUG
	validate();
#endif

	compiled = true;
	locations.clear(); //regenerate table

	return true;
}

bool Shader::validate()
{
	glValidateProgram(program);
	assert ( glGetError() == GL_NO_ERROR );

	GLint validated = 0;
	glGetProgramiv(program,GL_LINK_STATUS,&validated);
	assert(glGetError() == GL_NO_ERROR);
	
	if (!validated)
	{
		printf("Shader validation failed\n");
		saveProgramInfoLog(program);
		return false;
	}

	return true;
}

bool Shader::createVertexShaderObject(const std::string& shader)
{
	return createShaderObject(GL_VERTEX_SHADER,vs,shader);
}

bool Shader::createFragmentShaderObject(const std::string& shader)
{
	return createShaderObject(GL_FRAGMENT_SHADER,fs,shader);
}

bool Shader::createComputeShaderObject(const std::string& shader)
{
	return createShaderObject(GL_COMPUTE_SHADER, cs, shader);
}


bool Shader::createShaderObject(unsigned int type, GLuint& handle, const std::string& code)
{
	if (handle != 0)
		glDeleteShader(handle);

	handle = glCreateShader(type);
	assert( glGetError() == GL_NO_ERROR );
    
	std::string prefix = "";//"#define DESKTOP\n";

    std::string fullcode = prefix + code;
	const char* ptr = fullcode.c_str();
	glShaderSource(handle, 1, &ptr, NULL);
	assert( glGetError() == GL_NO_ERROR );
	
	glCompileShader(handle);
	assert( glGetError() == GL_NO_ERROR );

	GLint compile=0;
	glGetShaderiv(handle,GL_COMPILE_STATUS,&compile);
	assert( glGetError() == GL_NO_ERROR );

	//we want to see the compile log if we are in debug (to check warnings)
	if (!compile)
	{
		//prints errors
		saveShaderInfoLog(handle);
        std::cout << "Shader code:\n " << std::endl;
		std::vector<std::string> lines = split( fullcode, '\n' );
		std::vector<char> lines_errors_mask(lines.size());
		memset(&lines_errors_mask[0], 0, sizeof(char) * lines_errors_mask.size());
		for (size_t i = 0; i < lines_with_error.size(); ++i)
			if(lines_with_error[i] < lines_errors_mask.size())
				lines_errors_mask[lines_with_error[i]] = 1;

		for( size_t i = 0; i < lines.size(); ++i)
			std::cout << (i+1) << "  " << (lines_errors_mask[i] ? TermColor::RED : TermColor::WHITE) << lines[i] << TermColor::DEFAULT << std::endl;

		return false;
	}

	glAttachShader(program,handle);
	assert( glGetError() == GL_NO_ERROR );

	return true;
}


void Shader::release()
{
	if (vs)
	{
		glDeleteShader(vs);
		assert (glGetError() == GL_NO_ERROR);
		vs = 0;
	}

	if (fs)
	{
		glDeleteShader(fs);
		assert (glGetError() == GL_NO_ERROR);
		fs = 0;
	}

	if (cs)
	{
		glDeleteShader(cs);
		assert(glGetError() == GL_NO_ERROR);
		cs = 0;
	}

	if (program)
	{
		glDeleteProgram(program);
		assert (glGetError() == GL_NO_ERROR);
		program = 0;
	}

	locations.clear();

	compiled = false;
}


void Shader::enable()
{
	if (current == this)
		return;

	current = this;

	glUseProgram(program);
    GLuint err = glGetError();
	assert (err == GL_NO_ERROR);

	last_slot = 0;
}


void Shader::disable()
{
	current = NULL;

	glUseProgram(0);
	//glActiveTexture(GL_TEXTURE0);
	assert (glGetError() == GL_NO_ERROR);
}

void Shader::disableShaders()
{
	glUseProgram(0);
	assert (glGetError() == GL_NO_ERROR);
}

void Shader::saveShaderInfoLog(GLuint obj )
{
	int len = 0;
	assert(glGetError() == GL_NO_ERROR);
	glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &len);
	assert(glGetError() == GL_NO_ERROR);
    
	if (len == 0)
		return;

	char* ptr = new char[len + 1];
	GLsizei written = 0;
	glGetShaderInfoLog(obj, len, &written, ptr);
	ptr[written - 1] = '\0';
	assert(glGetError() == GL_NO_ERROR);

	std::vector<std::string> lines = tokenize(ptr, "\n");
	delete[] ptr;

	//clear
	lines_with_error.resize(lines.size());
	memset(&lines_with_error[0], 0, lines.size());

	for (size_t i = 0; i < lines.size(); ++i)
	{
		lines_with_error[i] = max(0, stoi(lines[i].substr(2)) - 1);
		std::cout << TermColor::RED << lines[i] << TermColor::DEFAULT << std::endl;
	}
        
	//printf("LOG **********************************************\n%s\n",log.c_str());
}


void Shader::saveProgramInfoLog(GLuint obj)
{
	int len = 0;
	assert(glGetError() == GL_NO_ERROR);
	glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &len);
	assert(glGetError() == GL_NO_ERROR);

	if (len > 0)
	{
		char* ptr = new char[len+1];
		GLsizei written=0;
		glGetProgramInfoLog(obj, len, &written, ptr);
		ptr[written-1]='\0';
		assert(glGetError() == GL_NO_ERROR);
		log.append(ptr);
		delete[] ptr;

		printf("LOG **********************************************\n%s\n",log.c_str());
	}
}

GLint Shader::getLocation(const char* varname, bool is_block )
{
	if(varname == 0)
		return 0;

	GLint loc = 0;

	loctable::iterator cur = locations.find(varname);
	
	if(cur == locations.end()) //not found in the locations table
	{
		if(is_block)
			loc = glGetUniformBlockIndex(program, varname);
		else
			loc = glGetUniformLocation(program, varname);

		if (loc == -1)
		{
			return -1;
		}

		//insert the new value
		locations.insert(loctable::value_type(varname,loc));
	}
	else //found in the table
	{
		loc = cur->second;
	}
	return loc;
}

int Shader::getAttribLocation(const char* varname)
{
	int loc = glGetAttribLocation(program, varname);
	if (loc == -1)
	{
		return loc;
	}
	assert(glGetError() == GL_NO_ERROR);

	return loc;
}

int Shader::getUniformLocation(const char* varname)
{
	int loc = getLocation(varname);
	if (loc == -1)
	{
		return loc;
	}
	assert(glGetError() == GL_NO_ERROR);
	return loc;
}

int Shader::getUniformBlockLocation(const char* varname)
{
	int loc = getLocation(varname);
	if (loc == -1)
	{
		return loc;
	}
	assert(glGetError() == GL_NO_ERROR);
	return loc;
}


void Shader::setTexture(const char* varname, Texture* tex, int slot)
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(tex->texture_type, tex->texture_id);
	setUniform1(varname, slot);
	glActiveTexture(GL_TEXTURE0 + slot);
}

/*
void Shader::setTexture(const char* varname, unsigned int tex)
{
	glActiveTexture(GL_TEXTURE0 + last_slot);
	glBindTexture(GL_TEXTURE_2D,tex);
	setUniform1(varname,last_slot);
	last_slot = (last_slot + 1) % 8;
	glActiveTexture(GL_TEXTURE0 + last_slot);
}
*/

void Shader::setUniform1(const char* varname, bool input1)
{
	GLint loc = getLocation(varname);
	CHECK_SHADER_VAR(loc, varname);
	glUniform1i(loc, input1);
	assert(glGetError() == GL_NO_ERROR);
}

void Shader::setUniform1(const char* varname, int input1)
{
	GLint loc = getLocation(varname);
	CHECK_SHADER_VAR(loc,varname);
	glUniform1i(loc, input1);
	assert (glGetError() == GL_NO_ERROR);
}

void Shader::setUniform2(const char* varname, int input1, int input2)
{
	GLint loc = getLocation(varname);
	CHECK_SHADER_VAR(loc,varname);
	glUniform2i(loc, input1, input2);
	assert (glGetError() == GL_NO_ERROR);
}

void Shader::setUniform3(const char* varname, int input1, int input2, int input3)
{
	GLint loc = getLocation(varname);
	CHECK_SHADER_VAR(loc,varname);
	glUniform3i(loc, input1, input2, input3);
	assert (glGetError() == GL_NO_ERROR);
}

void Shader::setUniform4(const char* varname, const int input1, const int input2, const int input3, const int input4)
{
	GLint loc = getLocation(varname);
	CHECK_SHADER_VAR(loc,varname);
	glUniform4i(loc, input1, input2, input3, input4);
	assert (glGetError() == GL_NO_ERROR);
}

void Shader::setUniform1Array(const char* varname, const int* input, const int count)
{
	GLint loc = getLocation(varname);
	CHECK_SHADER_VAR(loc,varname);
	glUniform1iv(loc,count,input);
	assert (glGetError() == GL_NO_ERROR);
}

void Shader::setUniform2Array(const char* varname, const int* input, const int count)
{
	GLint loc = getLocation(varname);
	CHECK_SHADER_VAR(loc,varname);
	glUniform2iv(loc,count,input);
	assert (glGetError() == GL_NO_ERROR);
}

void Shader::setUniform3Array(const char* varname, const int* input, const int count)
{
	GLint loc = getLocation(varname);
	CHECK_SHADER_VAR(loc,varname);
	glUniform3iv(loc,count,input);
	assert (glGetError() == GL_NO_ERROR);
}

void Shader::setUniform4Array(const char* varname, const int* input, const int count)
{
	GLint loc = getLocation(varname);
	CHECK_SHADER_VAR(loc,varname);
	glUniform4iv(loc,count,input);
	assert (glGetError() == GL_NO_ERROR);
}

void Shader::setUniform1(const char* varname, const float input1)
{
	GLint loc = getLocation(varname);
	CHECK_SHADER_VAR(loc,varname);
	glUniform1f(loc, input1);
	assert (glGetError() == GL_NO_ERROR);
}

void Shader::setUniform2(const char* varname, const float input1, const float input2)
{
	GLint loc = getLocation(varname);
	CHECK_SHADER_VAR(loc,varname);
	glUniform2f(loc, input1, input2);
	assert (glGetError() == GL_NO_ERROR);
}

void Shader::setUniform3(const char* varname, const float input1, const float input2, const float input3)
{
	GLint loc = getLocation(varname);
	CHECK_SHADER_VAR(loc,varname);
	glUniform3f(loc, input1, input2, input3);
	assert (glGetError() == GL_NO_ERROR);
}

void Shader::setUniform4(const char* varname, const float input1, const float input2, const float input3, const float input4)
{
	GLint loc = getLocation(varname);
	CHECK_SHADER_VAR(loc,varname);
	glUniform4f(loc, input1, input2, input3, input4);
	checkGLErrors();
}

void Shader::setUniform1Array(const char* varname, const float* input, const int count)
{
	GLint loc = getLocation(varname);
	CHECK_SHADER_VAR(loc,varname);
	glUniform1fv(loc,count,input);
	assert (glGetError() == GL_NO_ERROR);
}

void Shader::setUniform2Array(const char* varname, const float* input, const int count)
{
	GLint loc = getLocation(varname);
	CHECK_SHADER_VAR(loc,varname);
	glUniform2fv(loc,count,input);
	assert (glGetError() == GL_NO_ERROR);
}

void Shader::setUniform3Array(const char* varname, const float* input, const int count)
{
	GLint loc = getLocation(varname);
	CHECK_SHADER_VAR(loc,varname);
	glUniform3fv(loc,count,input);
	assert (glGetError() == GL_NO_ERROR);
}

void Shader::setUniform4Array(const char* varname, const float* input, const int count)
{
	GLint loc = getLocation(varname);
	CHECK_SHADER_VAR(loc,varname);
	glUniform4fv(loc,count,input);
	assert (glGetError() == GL_NO_ERROR);
}

void Shader::setMatrix44(const char* varname, const float* m)
{
	GLint loc = getLocation(varname);
	CHECK_SHADER_VAR(loc,varname);
	glUniformMatrix4fv(loc, 1, GL_FALSE, m);
	assert (glGetError() == GL_NO_ERROR);
}

void Shader::setMatrix44( const char* varname, const Matrix44 &m )
{
	GLint loc = getLocation(varname);
	CHECK_SHADER_VAR(loc,varname);
	glUniformMatrix4fv(loc, 1, GL_FALSE, m.m);
	assert (glGetError() == GL_NO_ERROR);
}

void Shader::setMatrix44Array( const char* varname, Matrix44* m_array, int num )
{
	GLint loc = getLocation(varname);
	CHECK_SHADER_VAR(loc, varname);
	glUniformMatrix4fv(loc, num, GL_FALSE, (GLfloat*)m_array);
	assert(glGetError() == GL_NO_ERROR);
}

void Shader::init()
{
	static bool firsttime = true;
	Shader::s_ready = true;
	if(firsttime)
	{

	#ifdef LOAD_EXTENSIONS_MANUALLY
		IMPORT_GLEXT( glCreateProgramObject );
		IMPORT_GLEXT( glLinkProgram );
		IMPORT_GLEXT( glGetObjectParameteriv );
		IMPORT_GLEXT( glValidateProgram );
		IMPORT_GLEXT( glCreateShaderObject );
		IMPORT_GLEXT( glShaderSource );
		IMPORT_GLEXT( glCompileShader );
		IMPORT_GLEXT( glAttachObject );
		IMPORT_GLEXT( glDetachObject );
		IMPORT_GLEXT( glUseProgramObject );
		//IMPORT_GLEXT( glActiveTexture );
		IMPORT_GLEXT( glGetInfoLog );
		IMPORT_GLEXT( glGetUniformLocation );
		IMPORT_GLEXT( glGetAttribLocation );
		IMPORT_GLEXT( glUniform1i );
		IMPORT_GLEXT( glUniform2i );
		IMPORT_GLEXT( glUniform3i );
		IMPORT_GLEXT( glUniform4i );
		IMPORT_GLEXT( glUniform1iv );
		IMPORT_GLEXT( glUniform2iv );
		IMPORT_GLEXT( glUniform3iv );
		IMPORT_GLEXT( glUniform4iv );
		IMPORT_GLEXT( glUniform1f );
		IMPORT_GLEXT( glUniform2f );
		IMPORT_GLEXT( glUniform3f );
		IMPORT_GLEXT( glUniform4f );
		IMPORT_GLEXT( glUniform1fv );
		IMPORT_GLEXT( glUniform2fv );
		IMPORT_GLEXT( glUniform3fv );
		IMPORT_GLEXT( glUniform4fv );
		IMPORT_GLEXT( glUniformMatrix4fv );
	#endif
	}
	
	firsttime = false;
}

Shader* Shader::getDefaultShader(std::string name)
{
	auto it = s_Shaders.find(name);
	if (it != s_Shaders.end())
		return it->second;

	std::string vs = "";
	std::string fs = "";

	vs = "attribute vec3 a_vertex; attribute vec3 a_normal; attribute vec2 a_uv; attribute vec4 a_color; \
	uniform mat4 u_model;\n\
	uniform mat4 u_viewprojection;\n\
	varying vec3 v_position;\n\
	varying vec3 v_world_position;\n\
	varying vec4 v_color;\n\
	varying vec3 v_normal;\n\
	varying vec2 v_uv;\n\
	void main()\n\
	{\n\
		v_normal = (u_model * vec4(a_normal, 0.0)).xyz;\n\
		v_position = a_vertex;\n\
		v_color = a_color;\n\
		v_world_position = (u_model * vec4(a_vertex, 1.0)).xyz;\n\
		v_uv = a_uv;\n\
		gl_Position = u_viewprojection * vec4(v_world_position, 1.0);\n\
	}";

	if (name == "flat2D")
	{
		vs = "attribute vec2 a_vertex;\n\
		uniform mat4 u_viewprojection;\n\
		void main()\n\
		{\n\
			gl_Position = u_viewprojection * vec4(a_vertex,0.0, 1.0);\n\
		}";
	}


	if (name == "flat" || name == "flat2D")
	{
		fs = "uniform vec4 u_color;\n\
			void main() {\n\
				gl_FragColor = u_color;\n\
			}";
	}
	else if (name == "color")
	{
		fs = "varying vec4 v_color;\n\
			uniform vec4 u_color;\n\
			void main() {\n\
				gl_FragColor = v_color * u_color;\n\
			}";
	}
	else if(name == "texture")
	{
		fs = "uniform vec4 u_color;\n\
			uniform sampler2D u_texture;\n\
			varying vec2 v_uv;\n\
			void main() {\n\
				gl_FragColor = u_color * texture2D(u_texture, v_uv);\n\
			}";
	}
	else if (name == "grid")
	{
		fs = "uniform vec4 u_color;\n\
			varying vec4 v_color;\n\
			uniform vec3 u_camera_position;\n\
			varying vec3 v_world_position;\n\
			void main() {\n\
				vec4 color = u_color * v_color;\n\
				color.a *= pow( 1.0 - length(v_world_position.xz - u_camera_position.xz) / 5000.0, 4.5);\n\
				if(v_world_position.x == 0.0)\n\
					color.xyz = vec3(1.0, 0.5, 0.5);\n\
				if(v_world_position.z == 0.0)\n\
					color.xyz = vec3(0.5, 0.5, 1.0);\n\
				vec3 E = normalize(v_world_position - u_camera_position);\n\
				gl_FragColor = color;\n\
			}";
	}
	else if (name == "screen") //draws a quad fullscreen
	{
		vs = "attribute vec3 a_vertex; \
			varying vec2 v_uv;\n\
			void main()\n\
			{\n\
				v_uv = a_vertex.xy * 0.5 + vec2(0.5);\n\
				gl_Position = vec4(a_vertex.xy,0.0,1.0);\n\
			}";
		fs = "varying vec2 v_uv;\n\
			uniform sampler2D u_texture;\n\
			void main() {\n\
				gl_FragColor = texture2D( u_texture, v_uv );\n\
			}";
	}
	else if (name == "linear_depth")
	{
		vs = "attribute vec3 a_vertex; \
			varying vec2 v_uv;\n\
			void main()\n\
			{\n\
				v_uv = a_vertex.xy * 0.5 + vec2(0.5);\n\
				gl_Position = vec4(a_vertex.xy,0.0,1.0);\n\
			}";
		fs = "uniform vec2 u_camera_nearfar;\n\
		uniform sampler2D u_texture; //depth map\n\
		varying vec2 v_uv;\n\
		void main()\n\
		{\n\
			float n = u_camera_nearfar.x;\n\
			float f = u_camera_nearfar.y;\n\
			float z = texture2D(u_texture, v_uv).x;\n\
			float color = n * (z + 1.0) / (f + n - z * (f - n));\n\
			gl_FragColor = vec4(color);\n\
		}";
	}
	else if (name == "screen_depth") //draws a quad fullscreen and clones its depth
	{
		vs = "attribute vec3 a_vertex; \
			varying vec2 v_uv;\n\
			void main()\n\
			{\n\
				v_uv = a_vertex.xy * 0.5 + vec2(0.5);\n\
				gl_Position = vec4(a_vertex.xy,0.0,1.0);\n\
			}";
		fs = "varying vec2 v_uv;\n\
			uniform sampler2D u_texture;\n\
			void main() {\n\
				vec4 color = texture2D( u_texture, v_uv );\n\
				gl_FragColor = color;\n\
				gl_FragDepth = color.r;\n\
			}";
	}
	else if (name == "quad" || name == "textured_quad") //draws a quad
	{
		vs = "attribute vec3 a_vertex;\n\
			uniform vec4 u_pos_size;\n\
			varying vec2 v_uv;\n\
			void main()\n\
			{\n\
				v_uv = vec2( a_vertex.x, 1.0 - a_vertex.y );\n\
				gl_Position = vec4( a_vertex.xy * u_pos_size.zw + u_pos_size.xy,0.0,1.0);\n\
			}";
		if(name == "textured_quad")
			fs = "varying vec2 v_uv;\n\
			uniform vec4 u_color;\n\
			uniform sampler2D u_texture;\n\
			void main() {\n\
				gl_FragColor = u_color * texture2D( u_texture, v_uv );\n\
			}";
		else
			fs = "uniform vec4 u_color;\n\
			void main() {\n\
				gl_FragColor = u_color;\n\
			}";
	}
	else
	{
		assert(0 && "unknown default shader");
		return NULL;
	}


	Shader* sh = new Shader();
	if (!sh->compileFromMemory(vs, fs))
	{
		assert(0 && "error in default shader");
		return NULL;
	}

	sh->enable();
	sh->setUniform4("u_color", Vector4f(1, 1, 1, 1));
	sh->disable();

	s_Shaders[name] = sh;
	return sh;
}

bool Shader::LoadAtlas(const char* filename, const char* base_path_cstr)
{
	std::string content;
	std::string base_path = base_path_cstr ? base_path_cstr : "";

	if (!readFile(filename, content))
	{
		std::cout << "Error: Shader atlas file not found" << std::endl;
		return false;
	}

	//separate subfiles
	s_shader_atlas_filename = filename;
	std::vector<std::string> lines = tokenize(content, "\n");
	std::string subfile_name = "";
	std::string subfile_content = "";

	//separate subfiles
	std::map<std::string, std::string> subfiles;
	for (size_t i = 0; i < lines.size(); ++i)
	{
		std::string& line = lines[i];
		std::string line_trimmed = trim(line);
		if (line[0] == '\\')
		{
			subfiles[subfile_name] = subfile_content; //store previous one
			subfile_name = trim(line.substr(1, std::string::npos));
			subfile_content = "";
			continue;
		}
		if(line_trimmed.size())
			subfile_content += line + "\n";
	}
	subfiles[subfile_name] = subfile_content;

	//expand includes
	for (auto it : subfiles)
	{
		subfile_content = ExpandIncludes(it.first, it.second, subfiles, base_path);
		s_shader_files[it.first] = subfile_content;
	}

	//compile shaders
	std::string shaders = s_shader_files[""];

	lines = tokenize(shaders, "\n");
	for (size_t i = 0; i < lines.size(); ++i)
	{
		std::string& line = lines[i];
		line = trim(line);
		if (line.size() == 0 || line.substr(0, 2) == "//")
			continue;
		int pos = line.find_first_of(' ');
		int pos2 = line.find_first_of(' ', pos + 1);
		int pos3 = line.find_first_of(' ', pos2 + 1);
		if (pos3 == -1)
			pos3 = std::string::npos;
		std::string name = line.substr(0, pos);
		std::string vs_filename = trim(line.substr(pos + 1, pos2 - pos));
		std::string fs_filename = trim(line.substr(pos2 + 1, pos3 - pos2));
		std::string macros = "";
		if (pos3 != std::string::npos)
			macros = line.substr(pos3 + 1);

		if (name[0] == '@') //ubershader
		{
			std::vector<std::string> macros_tokens;
			if (macros.size())
				macros_tokens = tokenize(macros, ",");
			s_ubershaders[name] = new UberShader( name, vs_filename, fs_filename, macros_tokens);
		}
		else //regular shader
		{
			std::string vs_code;
			std::string fs_code;

			if (!GetShaderFile(vs_filename.c_str(), vs_code) ||
				!GetShaderFile(fs_filename.c_str(), fs_code))
			{
				std::cout << " * Error in shader atlas, couldnt find files for " << name << std::endl;
				return nullptr;
			}

			Shader* shader = CompileShader(name.c_str(), vs_code.c_str(), fs_code.c_str(), macros.c_str());
			if (!shader)
			{
				std::cout << TermColor::RED << "[ERROR]" << TermColor::DEFAULT << " Problem compiling shaders in atlas, canceling compilation." << std::endl;
				return false;
			}
			shader->vs_filename = vs_filename;
			shader->fs_filename = fs_filename;
			shader->from_atlas = true;
			std::cout << " + Shader from atlas: " << TermColor::CYAN << name << TermColor::DEFAULT << std::endl;
		}
	}

	return true;
}

//called after parsing atlas
std::string Shader::ExpandIncludes(std::string name, std::string content, std::map<std::string, std::string>& subfiles, const std::string& base_path)
{
	std::vector<std::string> lines = tokenize(content, "\n");
	std::string subfile_content = "";
	for (size_t i = 0; i < lines.size(); ++i)
	{
		std::string& line = lines[i];
		std::string line_trimmed = trim(line);
		if (line_trimmed[0] == '#')
		{
			int pos = line_trimmed.find_first_of(' ');
			if (pos != std::string::npos)
			{
				std::string cmd = line_trimmed.substr(1, pos - 1);
				std::string param = trim(line_trimmed.substr(pos));
				if (cmd == "include" && !param.empty())
				{
					if (param[0] == '\"')
						param = param.substr(1, param.size() - 2);
					if (param == name)
						break;

					std::string include_content = "";

					auto it2 = subfiles.find(param);
					if (it2 != subfiles.end())
						include_content = it2->second + "\n";
					else if (param.size() > 2 && param[0] == '.' && param[1] == '/') //external file
					{
						std::string file_content;
						if (readFile(base_path + param.substr(1), file_content))
						{
							subfiles[param] = file_content + "\n";
							include_content = file_content + "\n";
						}
						else
							std::cout << " - Error: Shader #include not found: " << param << std::endl;
					}
					else
						std::cout << " - Error: Shader #include not found: " << param << std::endl;

					if (include_content.find("#include") != std::string::npos)
						include_content = ExpandIncludes(name, include_content, subfiles, base_path);

					subfile_content += include_content;
					continue;
				}
			}
		}
		subfile_content += line_trimmed + "\n";
	}
	return subfile_content;
}

Shader* Shader::CompileShader(const char* name, const char* vs_code, const char* fs_code, const char* macros = nullptr)
{
	//expand macros
	std::string macros_str = "";
	if (macros)
	{
		auto t = tokenize(macros, ",");
		for (size_t j = 0; j < t.size(); ++j)
			macros_str += "#define " + t[j] + "\n";
	}

	std::string vs(vs_code);
	std::string fs(fs_code);
	std::string version;

	//add macros after #version, otherwise it crashes
	if (vs[0] == '#')
	{
		size_t index = vs.find_first_of('\n');
		version = vs.substr(0, index);
		vs = vs.substr(index);
		index = fs.find_first_of('\n');
		fs = fs.substr(index);
	}

	vs = version + "\n" + macros_str + "\n" + vs;
	fs = version + "\n" + macros_str + "\n" + fs;

	Shader* shader = NULL;
	auto it2 = s_Shaders.find(name);
	if (it2 == s_Shaders.end())
	{
		shader = new Shader();
		s_Shaders[name] = shader;
	}
	else
		shader = it2->second;

	if (!shader->compileFromMemory(vs.c_str(), fs.c_str()))
	{
		delete shader;
		std::cout << " * Compilation error in shader at atlas: " << name << std::endl;
		return nullptr; //stop here
	}

	//shader->vs_filename = subshader.vs_name;
	//shader->ps_filename = subshader.fs_name;
	//if(macros)
	//	subshader.default_macros = macros;

	return shader;
}

bool Shader::GetShaderFile(const char* filename, std::string& content)
{
	auto it = s_shader_files.find(filename);
	if (it != s_shader_files.end())
	{
		content = it->second;
		return true;
	}

	//check if local file
	//...

	return false;
}

Shader* Shader::UberShader::get(uint64 macros)
{
	auto it = compiled_shaders.find(macros);
	if (it != compiled_shaders.end())
		return it->second;

	std::string fullname = name + "[" + std::to_string(macros) + "]";

	std::string vs_code;
	std::string fs_code;

	if( !Shader::GetShaderFile(this->vs_name.c_str(),vs_code) ||
		!Shader::GetShaderFile(this->fs_name.c_str(), fs_code) )
		return nullptr;

	std::string macros_str;
	if (macros)
	{
		int bit = 1;
		int max_macros = min(64, this->macros.size());
		for (int i = 0; i < max_macros; ++i)
		{
			if (macros & bit << i)
				macros_str += this->macros[i] + ",";
		}
		macros_str = macros_str.substr(0, macros_str.size() - 1); //remove last comma
	}

	Shader* shader = Shader::CompileShader(fullname.c_str(), vs_code.c_str(), fs_code.c_str(), macros_str.c_str() );
	if (!shader)
		return nullptr;
	compiled_shaders[macros] = shader;
	shader->vs_filename = this->vs_name;
	shader->fs_filename = this->fs_name;
	shader->from_atlas = true;
	std::cout << " + Shader from Ubershader: " << TermColor::CYAN << fullname << TermColor::DEFAULT << std::endl;
	return shader;
}

void Shader::UberShader::clear()
{
	compiled_shaders.clear();
	//no need to delete shaders, as they are already in the global s_shaders container
}

Shader::UberShader* Shader::GetUberShader(const char* name)
{
	auto it = s_ubershaders.find(name);
	if (it == s_ubershaders.end())
		return nullptr;
	return it->second;
}

// **************************************

BufferObject::BufferObject()
{
	id = 0;
	size = 0;
	type = GL_UNIFORM_BUFFER;
}

BufferObject::BufferObject(const char* name)
{
	id = 0;
	size = 0;
	type = GL_UNIFORM_BUFFER;
	if(name)
		this->name = name;
}

BufferObject::~BufferObject()
{
	deallocate();
}

void BufferObject::deallocate()
{
	if (!id)
		return;
	glDeleteBuffers(1, &id);
	id = size = 0;
}

void BufferObject::allocate(int size)
{
	assert(size);
	if (size != this->size)
	{
		deallocate();
		if (!id)
			glGenBuffers(1, &id);
		this->size = size;
	}

	//allocate
	glBindBuffer(type, id);
	glBufferData(type, size, 0, GL_STREAM_DRAW);
	glBindBuffer(type, 0);
}

void BufferObject::updateFromPointer(const void* data, int size)
{
	assert(size);
	if (size != this->size)
	{
		deallocate();
		if (!id)
			glGenBuffers(1, &id);
		this->size = size;
	}

	//allocate and upload
	glBindBuffer(type, id);
	glBufferData(type, size, data, GL_STREAM_DRAW);
	glBindBuffer(type, 0);
}

void BufferObject::bind(Shader* shader, int index, int start, int length)
{
	assert(size);
	if (shader && name.size())
	{
		int loc = shader->getLocation(name.c_str(), true);
		if(loc != -1)
			glUniformBlockBinding( shader->program, loc, index );
	}

	if (length == -1 && start == 0) //it matters to use base instead of range?
	{
		glBindBufferBase(type, index, id);
	}
	else
	{
		if (length == -1)
			length = size - start;
		assert(start >= 0 && (start + length) <= size);
		glBindBufferRange(type, index, id, start, length);
	}
}


};
