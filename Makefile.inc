# Common included file for all makefiles in the project

CC       	= gcc
CXX		= g++
CFLAGS   	= -g -Wall -Wno-unused-variable 
CXXFLAGS   	= -g -Wall -Wno-unused-variable -std=c11 
CPPFLAGS	= -DGCC -DSKIP_IMGUI
#CFLAGS   	= -O2 -Wall -Werror
#CXXFLAGS   	= -O2 -Wall -Werror
AR		= ar
MAKE        = make
