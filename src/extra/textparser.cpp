#include "textparser.h"

//#include "includes.h"

#include <sys/stat.h>
#include <string>
#include <algorithm>

#pragma warning(disable: 4996)
#pragma warning(disable: 4244)
#pragma warning(disable: 4018)

//extern HWND GLOBmainwindow;
char g_string_temporal[256];

TextParser::TextParser()
: data(NULL)
{}

TextParser::TextParser(const char *name)
: data(NULL)
{
  FILE *f;
  struct stat stbuffer;
  
  stat(name,&stbuffer);
  f = fopen(name,"rb");
  size = (int)stbuffer.st_size;
  data = new char[size];
  sl = 0;
  fread(data,size,1,f);
  fclose(f);
}

TextParser::~TextParser()
{
  if (data!=NULL) 
    delete data;
}

bool TextParser::create(const char *name)
{
  FILE *f;
  struct stat stbuffer;
  
  stat(name,&stbuffer);
  f = fopen(name,"rb");
  if (f == NULL)
    {
      char s[256];
      sprintf(s,"The resource file %s does not exist\n",name);
      //MessageBox(GLOBmainwindow,s,"SR",MB_OK);
      return false;
    }
  size = (int)stbuffer.st_size;
  data = new char[size];
  sl = 0;
  fread(data,size,1,f);
  fclose(f);
	return true;
}

int legal(char c)
{
  int res;
  
  res = (c>32);
  return res;
}

char *TextParser::getword()
{
  int p0,p1,i;

  p0 = sl;
  if (p0 >= size)
    return NULL;
  while (!legal(data[p0]) && p0<size)
    p0++;
  if (p0 >= size)
    return NULL;
  p1 = p0+1;
  while (legal(data[p1]))
    p1++;
  
  for (i=p0;i<p1;i++)
    {
      if ((data[i]<='z') && (data[i]>='a'))
	data[i] += ('A'-'a');
      g_string_temporal[i-p0] = data[i];
    }
  g_string_temporal[p1-p0] = '\0';
  sl = p1;
	std::string s(g_string_temporal);
	std::transform(s.begin(),s.end(),s.begin(),toupper);
  //strupr(g_string_temporal);
	strcpy(g_string_temporal,s.c_str());
  return g_string_temporal;
}


char *TextParser::getcommaword()
{
  int p0,p1,i;

  p0 = sl;
  while (data[p0]!='"')
    p0++;
  p0++;
  p1 = p0+1;
  while (data[p1]!='"')
    p1++;
  for (i=p0;i<p1;i++)
    {
      //if ((data[i]<='z') && (data[i]>='a')) data[i]+=('A'-'a');
      g_string_temporal[i-p0] = data[i];
    }
  g_string_temporal[p1-p0] = '\0';
  sl = p1+1;
  return g_string_temporal;
}

int TextParser::getint()
{
  return( atoi(getword()) );
}

double TextParser::getfloat()
{
  return( atof(getword()) );
}

void TextParser::goback()
{
  int p0,p1;

  p0=sl;
  while (!legal(data[p0])) p0--;
  p1=p0-1;
  while (legal(data[p1])) p1--;
  sl=p1;
}


int TextParser::countchar(char c)
{
  int res;
  unsigned int i;
  
  res=0;
  for (i=0;i<size;i++)
    if (data[i]==c) res++;
  return res;
}


void TextParser::reset()
{  
  sl=0;
}

void TextParser::destroy()
{
  if (data!=NULL) 
    delete data;
}


int TextParser::countword(char *s)
{
  int res;
  unsigned int i;
  int final;
  unsigned int si;

  res=0;
  final=0;
  i=0;
  while (!final)
    {
      si=0;
      while (toupper(data[i])==toupper(s[si]))
	{
	  i++;
	  si++;
	}
      res+=(si==strlen(s));
      i+=si;
      i++;
      final=(i>=size);
    }
  return res;
}


int TextParser::countwordfromhere(char *s)
{
  int res;
  unsigned int i;
  int final;
  unsigned int si;
  
  res=0;
  final=0;
  i=sl;
  while (!final)
    {
      si=0;
      while (toupper(data[i])==toupper(s[si]))
	{
	  i++;
	  si++;
	}
      res+=(si==strlen(s));
      i+=si;
      i++;
      final=(i>=size);
    }
  return res;
}

int TextParser::eof()
{  
  return (sl>size);
}


void TextParser::seek(const char *token)
{
  char *dummy=getword();

  while (strcmp(dummy,token) && (sl<size))
    dummy = getword();  
}

int TextParser::CountObjs()
{
	char *dummy=getword();
	int objCount = 0;

	while (sl < size)
	{
		if( strcmp(dummy, "*GEOMOBJECT") == 0 ) objCount++;
		dummy = getword();
	}

	return objCount;
}
