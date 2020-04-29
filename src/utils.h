/*  by Javi Agenjo 2013 UPF  javi.agenjo@gmail.com
	This contains several functions that can be useful when programming your game.
*/
#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <sstream>
#include <vector>

#include "includes.h"
#include "framework.h"

//General functions **************
long getTime();
float * snapshot();
bool readFile(const std::string& filename, std::string& content);

//generic purposes fuctions
void drawGrid();
bool drawText(float x, float y, std::string text, Vector3 c, float scale = 1);

//check opengl errors
bool checkGLErrors();

std::string getPath();

Vector2 getDesktopSize( int display_index = 0 );

std::vector<std::string> tokenize(const std::string& source, const char* delimiters, bool process_strings = false);
std::vector<std::string>& split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim);
std::string join(std::vector<std::string>& strings, const char* delim);

void ImGuiMatrix44(Matrix44& matrix, const char* text);

std::string getGPUStats();
void drawGrid();

//Used in the MESH and ANIM parsers
char* fetchWord(char* data, char* word);
char* fetchFloat(char* data, float& f);
char* fetchMatrix44(char* data, Matrix44& m);
char* fetchEndLine(char* data);
char* fetchBufferFloat(char* data, std::vector<float>& vector, int num = 0);
char* fetchBufferVec3(char* data, std::vector<Vector3>& vector);
char* fetchBufferVec2(char* data, std::vector<Vector2>& vector);
char* fetchBufferVec3u(char* data, std::vector<Vector3u>& vector);
char* fetchBufferVec4ub(char* data, std::vector<Vector4ub>& vector);
char* fetchBufferVec4(char* data, std::vector<Vector4>& vector);


#endif
