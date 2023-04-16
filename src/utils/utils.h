/*  by Javi Agenjo 2013 UPF  javi.agenjo@gmail.com
	This contains several functions that can be useful when programming your game.
*/
#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <sstream>
#include <vector>
#include "../extra/cJSON.h"


#include "../core/includes.h"
#include "../core/math.h"

//General functions **************
long getTime(); //there is also CORE::getTime
bool readFile(const std::string& filename, std::string& content);
bool readFileBin(const std::string& filename, std::vector<unsigned char>& buffer);
bool writeFile(const std::string& filename, std::string& content);

//work with file paths
std::string getFolderName(std::string path);
std::string getExtension(std::string path);
std::string toLowerCase(std::string str);
std::string cleanPath(std::string);
std::string makePathRelative(std::string filename);

//to work with strings (split, join, etc)
std::vector<std::string> tokenize(const std::string& source, const char* delimiters, bool process_strings = false);
std::vector<std::string>& split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim);
std::string join(std::vector<std::string>& strings, const char* delim);

void stdlog(std::string str);

//Used in the MESH and ANIM parsers to read and parse binary chunks from pointer address
char* fetchWord(char* data, char* word);
char* fetchFloat(char* data, float& f);
char* fetchMatrix44(char* data, Matrix44& m);
char* fetchEndLine(char* data);
char* fetchBufferFloat(char* data, std::vector<float>& vector, int num = 0);
char* fetchBufferVec3(char* data, std::vector<Vector3f>& vector);
char* fetchBufferVec2(char* data, std::vector<Vector2f>& vector);
char* fetchBufferVec3u(char* data, std::vector<Vector3u>& vector);
char* fetchBufferVec3u(char* data, std::vector<unsigned int>& vector);
char* fetchBufferVec4ub(char* data, std::vector<Vector4ub>& vector);
char* fetchBufferVec4(char* data, std::vector<Vector4f>& vector);

//used to parse JSONs
bool readJSONBool(cJSON* obj, const char* name, bool default_value);
float readJSONNumber(cJSON* obj, const char* name, float default_value);
std::string readJSONString(cJSON* obj, const char* name, const char* default_str);
bool readJSONVector(cJSON* obj, const char* name, std::vector<float>& dst);
Vector3f readJSONVector3(cJSON* obj, const char* name, Vector3f default_value);
Vector4f readJSONVector4(cJSON* obj, const char* name);

void writeJSONBool(cJSON* obj, const char* name, bool value);
void writeJSONNumber(cJSON* obj, const char* name, float value);
void writeJSONString(cJSON* obj, const char* name, const char* str);
void writeJSONVector(cJSON* obj, const char* name, std::vector<float>& dst);
void writeJSONVector3(cJSON* obj, const char* name, Vector3f value);
void writeJSONVector4(cJSON* obj, const char* name, Vector4f value);

//used to colorize terminal: std::cout << TermColor::RED << "Text in red" << TermColor::DEFAULT << std::endl;
namespace TermColor {
	extern const char* RED;
	extern const char* GREEN;
	extern const char* YELLOW;
	extern const char* BLUE;
	extern const char* MAGENTA;
	extern const char* CYAN;
	extern const char* WHITE;
	extern const char* DEFAULT;
};

#endif


