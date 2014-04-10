
CXXFLAGS=-std=c++11
#MICE
#INCLUDEFLAGS=-I. -I/home/jpatrick/libs/cube/install-gcc-4.8.2/include/cube
#LDFLAGS+=-L/home/jpatrick/libs/cube/install-gcc-4.8.2/lib -lcube4 -lz
#Lcluster
INCLUDEFLAGS=-I. -I/home/ci24amun/myRoot/gcc/openmpi/cube/4.2.1/include/cube/
LDFLAGS+=-L/home/ci24amun/myRoot/gcc/openmpi/cube/4.2.1/lib -lcube4 -lz

CubeCallGraphTool-lcluster:
	$(CXX) $(CXXFLAGS) $(INCLUDEFLAGS) -o CubeCallgraphTool.exe main.cpp cgNode.cpp callgraph.cpp $(LDFLAGS)

CubeCallGraphTool-mice:
	$(CXX) $(CXXFLAGS) $(INCLUDEFLAGS) -o CubeCallgraphTool.exe main.cpp cgNode.cpp callgraph.cpp $(LDFLAGS)



CubeOverheadEstimater-mice:
	$(CXX) $(CXXFLAGS) -I /home/jpatrick/libs/cube/install-gcc-4.8.2/include/cube -o CubeOverheadEstimation main.cpp -L/home/jpatrick/libs/cube/install-gcc-4.8.2/lib -lcube4 -lz

clean:
	rm -rf *.o
