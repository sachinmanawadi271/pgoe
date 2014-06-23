
# check if variables are defined, as of: http://stackoverflow.com/a/22540516
check-var-defined = $(if $(strip $($1)),,$(error "$1" is not defined))
$(call check-var-defined,CUBE_INCLUDE_PATH)
$(call check-var-defined,CUBE_LIBRARY_PATH)

#CXXFLAGS=-std=c++11
CXXFLAGS=-std=gnu++0x -Wall	# mice only has gcc-4.6.1 installed

# MICE
# source a script (e.g. /opt/scorep/load_cube-4.2.2-gcc4.6.sh) to load cube before compilation
INCLUDEFLAGS=-I. -I$(CUBE_INCLUDE_PATH)
LDFLAGS+=-L$(CUBE_LIBRARY_PATH) -lcube4 -lz

#Lcluster
#INCLUDEFLAGS=-I. -I/home/ci24amun/myRoot/gcc/openmpi/cube/4.2.1/include/cube/
#LDFLAGS+=-L/home/ci24amun/myRoot/gcc/openmpi/cube/4.2.1/lib -lcube4 -lz

CubeCallGraphTool-mice:
	$(CXX) $(CXXFLAGS) $(INCLUDEFLAGS) -o CubeCallgraphTool main.cpp cgNode.cpp callgraph.cpp $(LDFLAGS) $(WARN)

CubeCallGraphTool-lcluster:	# RN: i have no idea if this is still valid
	$(CXX) $(CXXFLAGS) $(INCLUDEFLAGS) -o CubeCallgraphTool main.cpp cgNode.cpp callgraph.cpp $(LDFLAGS)


clean:
	rm -rf *.o CubeCallgraphTool CubeOverheadEstimation
