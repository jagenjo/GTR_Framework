/*  by Javi Agenjo 2013 UPF  javi.agenjo@gmail.com
	This contains several functions that can be useful when programming your game.
*/
#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <sstream>
#include <vector>
#include "extra/cJSON.h"


#include "includes.h"
#include "framework.h"

//General functions **************
long getTime();
float * snapshot();
bool readFile(const std::string& filename, std::string& content);
bool readFileBin(const std::string& filename, std::vector<unsigned char>& buffer);

//generic purposes fuctions
void drawGrid();
bool drawText(float x, float y, std::string text, Vector3 c, float scale = 1);

//check opengl errors
bool checkGLErrors();

//returns the current path
std::string getPath();

Vector2 getDesktopSize( int display_index = 0 );

std::vector<std::string> tokenize(const std::string& source, const char* delimiters, bool process_strings = false);
std::vector<std::string>& split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim);
std::string join(std::vector<std::string>& strings, const char* delim);

void ImGuiMatrix44(Matrix44& matrix, const char* text);

std::string getGPUStats();
void drawGrid();

void stdlog(std::string str);

//Used in the MESH and ANIM parsers
char* fetchWord(char* data, char* word);
char* fetchFloat(char* data, float& f);
char* fetchMatrix44(char* data, Matrix44& m);
char* fetchEndLine(char* data);
char* fetchBufferFloat(char* data, std::vector<float>& vector, int num = 0);
char* fetchBufferVec3(char* data, std::vector<Vector3>& vector);
char* fetchBufferVec2(char* data, std::vector<Vector2>& vector);
char* fetchBufferVec3u(char* data, std::vector<Vector3u>& vector);
char* fetchBufferVec3u(char* data, std::vector<unsigned int>& vector);
char* fetchBufferVec4ub(char* data, std::vector<Vector4ub>& vector);
char* fetchBufferVec4(char* data, std::vector<Vector4>& vector);

bool readJSONBool(cJSON* obj, const char* name, bool default_value);
float readJSONNumber(cJSON* obj, const char* name, float default_value);
std::string readJSONString(cJSON* obj, const char* name, const char* default_str);
bool readJSONVector(cJSON* obj, const char* name, std::vector<float>& dst);
Vector3 readJSONVector3(cJSON* obj, const char* name, Vector3 default_value);
Vector4 readJSONVector4(cJSON* obj, const char* name);

#endif


