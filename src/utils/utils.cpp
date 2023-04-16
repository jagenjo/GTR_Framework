#include "utils.h"

#include <cassert>
#include <iostream>
#include <algorithm>

#include "../core/includes.h"
#include "../core/core.h"

#ifndef WIN32
	#include <sys/time.h>
#endif


long getTime()
{
	#ifdef WIN32
		return GetTickCount();
	#else
		struct timeval tv;
		gettimeofday(&tv,NULL);
		return (int)(tv.tv_sec*1000 + (tv.tv_usec / 1000));
	#endif
}

//this function is used to access OpenGL Extensions (special features not supported by all cards)
void* getGLProcAddress(const char* name)
{
	return SDL_GL_GetProcAddress(name);
}

std::string getFolderName(std::string path)
{
	path = cleanPath(path); //remove weird slashes
	size_t pos = path.find_last_of('/');
	if (pos == std::string::npos)
		return "";
	return path.substr(0, pos);
}

std::string getExtension(std::string path)
{
	size_t pos = path.find_last_of('.');
	if (pos == std::string::npos)
		return "";
	return path.substr(pos + 1);
}

std::string toLowerCase(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	return str;
}

std::string cleanPath(std::string filename)
{
	std::string result = filename;
	for (size_t i = 0; i < result.size(); ++i)
	{
		if (result[i] == '\\')
			result[i] = '/';
	}
	return result;
}

std::string makePathRelative(std::string filename)
{
	std::string folder = CORE::base_path;
	std::string relpath = cleanPath(filename);
	if (relpath.find(folder) == 0)
		relpath = relpath.substr(folder.size() + 1);
	return relpath;
}

bool readFile(const std::string& filename, std::string& content)
{
	content.clear();

	long count = 0;

	FILE *fp = fopen(filename.c_str(), "rb");
	if (fp == NULL)
	{
		std::cerr << "::readFile: file not found " << filename << std::endl;
		return false;
	}

	fseek(fp, 0, SEEK_END);
	count = ftell(fp);
	rewind(fp);

	content.resize(count);
	if (count > 0)
	{
		count = fread(&content[0], sizeof(char), count, fp);
	}
	fclose(fp);

	return true;
}

bool readFileBin(const std::string& filename, std::vector<unsigned char>& buffer)
{
	buffer.clear();
	FILE* fp = nullptr;
	fp = fopen(filename.c_str(), "rb");
	if (fp == nullptr)
	{
		std::cout << "CANT OPEN FILE" << std::endl;
		return false;
	}
	fseek(fp, 0L, SEEK_END);
	int size = ftell(fp);
	rewind(fp);
	buffer.resize(size);
	fread(&buffer[0], sizeof(char), buffer.size(), fp);
	fclose(fp);
	return true;
}

bool writeFile(const std::string& filename, std::string& content)
{
	FILE* f = fopen(filename.c_str(), "w");
	if (f == NULL)
	{
		std::cout << "[ERROR] cannot write JSON: " << filename.c_str() << std::endl;
		return false;
	}

	//save data
	fwrite(content.c_str(), sizeof(char), content.size(), f);
	fclose( f );

	return true;
}

void stdlog(std::string str)
{
	std::cout << str << std::endl;
}

std::vector<std::string>& split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

std::string join(std::vector<std::string>& strings, const char* delim)
{
	std::string str;
	for (size_t i = 0; i < strings.size(); ++i)
		str += strings[i] + (i < strings.size() - 1 ? std::string(delim) : "");
	return str;
}



std::vector<std::string> tokenize(const std::string& source, const char* delimiters, bool process_strings)
{
	std::vector<std::string> tokens;

	std::string str;
	size_t del_size = strlen(delimiters);
	const char* pos = source.c_str();
	char in_string = 0;
	unsigned int i = 0;
	while (*pos != 0)
	{
		bool split = false;

		if (!process_strings || (process_strings && in_string == 0))
		{
			for (i = 0; i < del_size && *pos != delimiters[i]; i++);
			if (i != del_size) split = true;
		}

		if (process_strings && (*pos == '\"' || *pos == '\''))
		{
			if (!str.empty() && in_string == 0) //some chars remaining
			{
				tokens.push_back(str);
				str.clear();
			}

			in_string = (in_string != 0 ? 0 : *pos);
			if (in_string == 0)
			{
				str += *pos;
				split = true;
			}
		}

		if (split)
		{
			if (!str.empty())
			{
				tokens.push_back(str);
				str.clear();
			}
		}
		else
			str += *pos;
		pos++;
	}
	if (!str.empty())
		tokens.push_back(str);
	return tokens;
}

char* fetchWord(char* data, char* word)
{
	int pos = 0;
	while (*data != ',' && *data != '\n' && pos < 254) { word[pos++] = *data; data++; }
	word[pos] = 0;
	if (pos < 254)
		data++; //skip ',' or '\n'
	return data;
}

char* fetchFloat(char* data, float& v)
{
	char w[255];
	data = fetchWord(data,w);
	v = (float)atof(w);
	return data;
}

char* fetchMatrix44(char* data, Matrix44& m)
{
	char word[255];
	for (int i = 0; i < 16; ++i)
	{
		data = fetchWord(data, word);
		m.m[i] = (float)atof(word);
	}
	return data;
}

char* fetchEndLine(char* data)
{
	while (*data && *data != '\n') { data++; }
	if (*data == '\n')
		data++;
	return data;
}

char* fetchBufferFloat(char* data, std::vector<float>& vector, int num )
{
	int pos = 0;
	char word[255];
	if (num)
		vector.resize(num);
	else //read size with the first number
	{
		data = fetchWord(data, word);
		int v = (int)atof(word);
		assert(v);
		vector.resize(v);
	}

	size_t index = 0;
	while (*data != 0) {
		if (*data == ',' || *data == '\n')
		{
			if (pos == 0)
			{
				data++;
				continue;
			}
			word[pos] = 0;
			float v = (float)atof(word);
			vector[index++] = v;
			if (*data == '\n' || *data == 0)
			{
				if (*data == '\n')
					data++;
				return data;
			}
			data++;
			if (index >= vector.size())
				return data;
			pos = 0;
		}
		else
		{
			word[pos++] = *data;
			data++;
		}
	}

	return data;
}

char* fetchBufferVec3(char* data, std::vector<Vector3f>& vector)
{
	std::vector<float> floats;
	data = fetchBufferFloat(data, floats);
	vector.resize(floats.size() / 3);
	memcpy(&vector[0], &floats[0], sizeof(float)*floats.size());
	return data;
}

char* fetchBufferVec2(char* data, std::vector<Vector2f>& vector)
{
	std::vector<float> floats;
	data = fetchBufferFloat(data, floats);
	vector.resize(floats.size() / 2);
	memcpy(&vector[0], &floats[0], sizeof(float)*floats.size());
	return data;
}

char* fetchBufferVec3u(char* data, std::vector<Vector3u>& vector)
{
	std::vector<float> floats;
	data = fetchBufferFloat(data, floats);
	vector.resize(floats.size() / 3);
	for (size_t i = 0; i < floats.size(); i += 3)
		vector[i / 3].set((unsigned int)floats[i], (unsigned int)floats[i + 1], (unsigned int)floats[i + 2]);
	return data;
}

char* fetchBufferVec3u(char* data, std::vector<unsigned int>& vector)
{
	std::vector<float> floats;
	data = fetchBufferFloat(data, floats);
	vector.resize(floats.size());
	for (size_t i = 0; i < floats.size(); i++)
		vector[i] = (unsigned int)(floats[i]);
	return data;
}

char* fetchBufferVec4ub(char* data, std::vector<Vector4ub>& vector)
{
	std::vector<float> floats;
	data = fetchBufferFloat(data, floats);
	vector.resize(floats.size() / 4);
	for (size_t i = 0; i < floats.size(); i += 4)
		vector[i / 4].set((uint8)floats[i], (uint8)floats[i + 1], (uint8)floats[i + 2], (uint8)floats[i + 3]);
	return data;
}

char* fetchBufferVec4(char* data, std::vector<Vector4f>& vector)
{
	std::vector<float> floats;
	data = fetchBufferFloat(data, floats);
	vector.resize(floats.size() / 4);
	memcpy(&vector[0], &floats[0], sizeof(float)*floats.size());
	return data;
}

bool readJSONBool(cJSON* obj, const char* name, bool default_value)
{
	cJSON* str_json = cJSON_GetObjectItemCaseSensitive((cJSON*)obj, name);
	if (!str_json)
		return default_value;
	return str_json->type == cJSON_True;
}

float readJSONNumber(cJSON* obj, const char* name, float default_value)
{
	cJSON* str_json = cJSON_GetObjectItemCaseSensitive((cJSON*)obj, name);
	if (!str_json || str_json->type != cJSON_Number)
		return default_value;
	return str_json->valuedouble;
}

std::string readJSONString(cJSON* obj, const char* name, const char* default_str)
{
	cJSON* str_json = cJSON_GetObjectItemCaseSensitive((cJSON*)obj, name);
	if (!str_json || str_json->type != cJSON_String)
		return default_str;
	return str_json->valuestring;
}

bool readJSONVector(cJSON* obj, const char* name, std::vector<float>& dst)
{
	cJSON* array_json = cJSON_GetObjectItemCaseSensitive((cJSON*)obj, name);
	if (!array_json)
		return false;
	if (!cJSON_IsArray(array_json))
		return false;

	dst.resize(cJSON_GetArraySize(array_json));
	for (size_t i = 0; i < dst.size(); ++i)
	{
		cJSON* value_json = cJSON_GetArrayItem(array_json, i);
		if (value_json)
			dst[i] = (float)value_json->valuedouble;
		else
			dst[i] = 0;
	}

	return true;
}

Vector3f readJSONVector3(cJSON* obj, const char* name, Vector3f default_value)
{
	std::vector<float> dst;
	if (readJSONVector(obj, name, dst))
	{
		if (dst.size() == 3)
			return Vector3f(dst[0], dst[1], dst[2]);
	}
	return default_value;
}

Vector4f readJSONVector4(cJSON* obj, const char* name)
{
	std::vector<float> dst;
	if (readJSONVector(obj, name, dst))
	{
		if (dst.size() == 4)
			return Vector4f(dst[0], dst[1], dst[2], dst[3]);
	}
	return Vector4f();
}

void writeJSONBool(cJSON* obj, const char* name, bool value)
{
	cJSON_AddBoolToObject(obj, name, value);
}

void writeJSONNumber(cJSON* obj, const char* name, float value)
{
	cJSON_AddNumberToObject(obj, name, value);
}

void writeJSONString(cJSON* obj, const char* name, const char* str)
{
	cJSON_AddStringToObject(obj, name, str);
}

void writeJSONVector(cJSON* obj, const char* name, std::vector<float>& dst)
{
	cJSON* array = cJSON_CreateArray();
	for(size_t i = 0; i < dst.size();++i)
		cJSON_AddItemToArray(array, cJSON_CreateNumber(dst[i]));
	cJSON_AddItemToObject(obj, name, array);
}

void writeJSONVector3(cJSON* obj, const char* name, Vector3f value)
{
	cJSON* array = cJSON_CreateArray();
	for (int i = 0; i < 3; ++i)
		cJSON_AddItemToArray(array, cJSON_CreateNumber(value.v[i]));
	cJSON_AddItemToObject(obj, name, array);
}

void writeJSONVector4(cJSON* obj, const char* name, Vector4f value)
{
	cJSON* array = cJSON_CreateArray();
	for (int i = 0; i < 4; ++i)
		cJSON_AddItemToArray(array, cJSON_CreateNumber(value.v[i]));
	cJSON_AddItemToObject(obj, name, array);
}


namespace TermColor {
	//bright
	const char* RED = "\u001b[31;1m";
	const char* GREEN = "\u001b[32;1m";
	const char* YELLOW = "\u001b[33;1m";
	const char* BLUE = "\u001b[34;1m";
	const char* MAGENT = "\u001b[35;1m";
	const char* CYAN = "\u001b[36;1m";
	const char* WHITE = "\u001b[37;1m";
	const char* DEFAULT = "\u001b[0m";

	/* pale
	const char* RED = "\033[31m";
	const char* GREEN = "\033[32m";
	const char* YELLOW = "\033[33m";
	const char* BLUE = "\033[34m";
	const char* MAGENTA = "\033[35m";
	const char* CYAN = "\033[36m";
	const char* WHITE = "\033[37m";
	const char* NEXT = "\033[38m";
	const char* DEFAULT = "\033[39m";
	*/
};