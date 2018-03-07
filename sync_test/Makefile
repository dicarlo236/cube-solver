#Makefile with SDL
appname := cube_sequence 

CXX := g++
CXXFLAGS := -O3 -Wall -g -std=c++11 #enable all warnings and debugging
LINKER_FLAGS := -L. -lpthread 

srcfiles := $(shell find . -maxdepth 1 -name "*.cpp") #find all .cpp files
objects  := $(patsubst %.cpp, %.o, $(srcfiles)) #name object files with .o suffix

all: $(appname)

$(appname): $(objects)
		$(CXX) $(CXXFLAGS)  -o $(appname) $(objects) $(LINKER_FLAGS) #g++ is sad unless linker flags are at the end...


clean:
		rm -f $(objects)

dist-clean: clean
		rm -f *~ .depend

