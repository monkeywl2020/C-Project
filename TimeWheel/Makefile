#include ../Makefile.param

CC = $(CROSS_TOOL)gcc
CXX = $(CROSS_TOOL)g++
LD = $(CROSS_TOOL)ld
AR = $(CROSS_TOOL)ar
STRIP = $(CROSS_TOOL)strip
RANLIB = $(CROSS_TOOL)ranlib

INC_PATH := -I./

CFLAGS := -Wall -g
CFLAGS += $(_AVC_CFLAGS)

ARFLAGS := -r
RANLIBFLAGS := 
LDFLAGS := -L.

TARGET := timewheel
SRCC := $(wildcard *.c ) 
COBJ := $(SRCC:%.c=%.o)
SRCXX := $(wildcard *.cpp ) 
CXXOBJ := $(SRCXX:%.cpp=%.o)
OBJ := $(COBJ) $(CXXOBJ)

.PHONY : clean all

all: $(TARGET)

$(TARGET):$(OBJ)
	$(CXX) $^ -g -o $@ $(LDFLAGS)  -lpthread -lrt
	
$(COBJ):%.o:%.c
	$(CC) $(INC_PATH) $(CFLAGS) -o $@ -c $<

$(CXXOBJ):%.o:%.cpp
	$(CXX) $(INC_PATH) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) 
	rm -f $(OBJ)
