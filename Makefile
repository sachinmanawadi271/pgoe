
# check if variables are defined, as of: http://stackoverflow.com/a/22540516
check-var-defined = $(if $(strip $($1)),,$(error "$1" is not defined))
$(call check-var-defined,CUBE_INCLUDE_PATH)
$(call check-var-defined,CUBE_LIBRARY_PATH)

#CXXFLAGS=-std=c++11
CXXFLAGS=-std=gnu++0x -Wall	# mice only has gcc-4.6.1 installed

DEBUG=-g -Og

SOURCES=src/main.cpp src/CgNode.cpp src/CallgraphManager.cpp src/Callgraph.cpp src/CubeReader.cpp src/EstimatorPhase.cpp \
src/SanityCheckEstimatorPhase.cpp src/EdgeBasedOptimumEstimatorPhase.cpp src/CgHelper.cpp \
src/NodeBasedOptimumEstimatorPhase.cpp src/ProximityMeasureEstimatorPhase.cpp \

OBJ=$(SOURCES:.cpp=.o)


# MICE
# source a script (e.g. /opt/scorep/load_cube-4.2.2-gcc4.6.sh) to load cube before compilation
INCLUDEFLAGS=-I. -I$(CUBE_INCLUDE_PATH)
LDFLAGS+=-L$(CUBE_LIBRARY_PATH) -lcube4 -lz

%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDEFLAGS) -c $< -o $@ $(DEBUG)

CubeCallGraphTool: $(OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDEFLAGS) -o CubeCallgraphTool $(OBJ) $(LDFLAGS) $(DEBUG)

clean:
	rm -rf $(OBJ) *.o CubeCallgraphTool
