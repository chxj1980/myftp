#/bin/sh

TOP_PATH = $(shell pwd)

SRCPTH = $(TOP_PATH)
INPTH = $(TOP_PATH)

SDK_PATH = ./

CXX  = g++ -std=c++11
INLPTH = -g -Wall -O3
INLPTH += -I$(INPTH)  \    
	    		
	   
LPTH :=  -lpthread  -ldl	

OBJ = $(SDK_PATH)obj  

TARG = ftp_client
DIRS = $(SRCPTH)

FILES   = $(foreach dir, $(DIRS),  $(wildcard $(dir)/*.cpp))
OBJS 	= $(patsubst %.cpp, %.o, $(FILES))

all:$(TARG)

#链接
$(TARG):$(OBJS)
	$(CXX) -o $(TARG)  $^ $(LPTH)   

#编译
#.cpp.o:报错:g++: error:  : 没有那个文件或目录
.c.o:
	$(CXX) -c $(INLPTH)  $<  -o $@    
	
clean:
	rm -rf $(OBJS) 
	rm -rf $(TARG)
