CXXFLAGS=-std=c++11 -Wall

INCLUDEFLAGS=`cube-config --cube-cxxflags`
LDFLAGS=`cube-config --cube-ldflags`

DEBUG=-g -Og

SOURCES= \
src/CgNode.cpp src/CallgraphManager.cpp src/Callgraph.cpp src/CubeReader.cpp src/EstimatorPhase.cpp \
src/SanityCheckEstimatorPhase.cpp src/EdgeBasedOptimumEstimatorPhase.cpp src/CgHelper.cpp \
src/NodeBasedOptimumEstimatorPhase.cpp src/ProximityMeasureEstimatorPhase.cpp \
src/IPCGReader.cpp src/IPCGEstimatorPhase.cpp \

OBJ=$(SOURCES:.cpp=.o)
DEP=$(OBJ:.o=.d)

all: CubeCallGraphTool SimpleOverheadEliminator

#check for cube-config in PATH
cube-config-exists: ; @which cube-config > /dev/null

# those strange flags build dependency files, so headers are dependencies too
%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDEFLAGS) -c -o $@ $(DEBUG)  -MD -MP -MF ${@:.o=.d}  $<

CubeCallGraphTool: cube-config-exists $(OBJ) src/main.o
	$(CXX) $(CXXFLAGS) $(INCLUDEFLAGS) -o $@ $(OBJ) src/main.o $(LDFLAGS) $(DEBUG)

SimpleOverheadEliminator: cube-config-exists $(OBJ) src/main_SimpleOverheadElimination.o
	$(CXX) $(CXXFLAGS) $(INCLUDEFLAGS) -o $@ $(OBJ) src/main_SimpleOverheadElimination.o $(LDFLAGS) $(DEBUG)

clean:
	rm -rf $(OBJ) $(DEP) src/*.o src/*.d CubeCallgraphTool
	
# first run has no dep files
-include $(DEP)
