CXX=g++ # out hacked clang version on lcluster is not abi compatible with the cube installation
CXXFLAGS=-std=c++11 -Wall

INCLUDEFLAGS=`cube-config --cube-cxxflags`
LDFLAGS=`cube-config --cube-ldflags`

DEBUG=-g

SOURCES= \
src/CgNode.cpp src/CallgraphManager.cpp src/Callgraph.cpp src/CubeReader.cpp src/EstimatorPhase.cpp \
src/SanityCheckEstimatorPhase.cpp src/EdgeBasedOptimumEstimatorPhase.cpp src/CgHelper.cpp \
src/NodeBasedOptimumEstimatorPhase.cpp src/ProximityMeasureEstimatorPhase.cpp \
src/IPCGReader.cpp src/IPCGEstimatorPhase.cpp \

OBJ=$(SOURCES:.cpp=.o)
DEP=$(OBJ:.o=.d)

all: CubeCallGraphTool

Debug: all

#check for cube-config in PATH
cube-config-exists: ; @which cube-config > /dev/null

# those strange flags build dependency files, so headers are dependencies too
%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDEFLAGS) $(MORE) -c -o $@ $(DEBUG)  -MD -MP -MF ${@:.o=.d}  $<

CubeCallGraphTool: cube-config-exists $(OBJ) src/main.o
	$(CXX) $(CXXFLAGS) $(INCLUDEFLAGS) -o $@ $(OBJ) src/main.o $(LDFLAGS) $(DEBUG)

clean:
	rm -rf $(OBJ) $(DEP) src/*.o src/*.d CubeCallgraphTool
	
# first run has no dep files
-include $(DEP)
