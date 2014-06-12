#include <fstream>
#include "callgraph.h"
#include "Cube.h"
#include "CubeMetric.h"
#include <cstdlib>

#include <cassert>

#define VERBOSE 5
#define PRINT_DOT 1

int main(int argc, char** argv){


Callgraph cg;
// The cube documentation says one should always use a try/catch thing around
try{
	// Create cube instance
	cube::Cube cube;
	// Read our cube file
	cube.openCubeReport( argv[1] );
	std::cout << "Reading input.. done." << std::endl;

	// Get the cube nodes
	const std::vector<cube::Cnode*>& cnodes = cube.get_cnodev();
	unsigned long long numberOfCalls = 0;
	// Just see what is our first node
//	std::cout << cnodes[0]->get_callee()->get_name() << std::endl;

	cube::Metric* visitsMetric = cube.get_met("visits");
	const std::vector<cube::Thread*> threads = cube.get_thrdv();

	for(auto node : cnodes){
		// I don't know when this happens, but it does.
		if(node->get_parent() == NULL)
			continue;

		// Put the caller/callee pair into our callgraph
		auto callee = node->get_parent()->get_callee();
		cg.putFunction(callee->get_name(), callee->get_mod(), callee->get_begn_ln(),
				node->get_callee()->get_name(), cube.get_sev(visitsMetric, node, threads.at(0)));

		numberOfCalls += cube.get_sev(visitsMetric, node, threads.at(0));

		// Also keep track of threading things...
		if(threads.size() > 1) {
			for(int i = 1; i < threads.size(); i++){
				cg.putFunction(callee->get_name(), callee->get_mod(), callee->get_begn_ln(),
						node->get_callee()->get_name(), cube.get_sev(visitsMetric, node, threads.at(i)));

				numberOfCalls += cube.get_sev(visitsMetric, node, threads.at(i));
			}
		}
	}
	std::cout << "Finished construction of cg. Now estimating InstROs overhead..." << std::endl;
#if PRINT_DOT
	cg.printDOT("construct");
#endif

	/** JP: This is code to estimate the generated overhead via call-graph guided hook placement. */
	bool once = true; // What exactly is that good for...?
	const int overheadPerCallInNanos = 4; 
	unsigned long long overAllOverhead = 0;
	unsigned long long numberOfInstrCalls = 0;
	unsigned long long optimizedOverhead = 0;
	unsigned long long optimizedNumberOfInstrCall = 0;

	// We analyze the cg and mark the nodes
	cg.markNodes();	


#if VERBOSE > 1
	std::cout << " ---- CubeCallGraphTool VERBOSE info begin ----" << std::endl
			<< "Graph includes: " << cg.getSize() << std::endl
			<< "Our algorithm would mark: " << cg.getNodesToMark().size() << std::endl;
	for(auto node : cg.getNodesToMark()){
		node->printMinimal();
		std::cout << std::endl;
	}
	std::cout << " ---- CubeCallGraphTool VERBOSE info end   ----" << std::endl << std::endl;
#endif
	// sum up the calls to function we would instrument
	for(auto node : cg.getNodesToMark()){
		overAllOverhead += overheadPerCallInNanos * node->getNumberOfCalls();
		numberOfInstrCalls += node->getNumberOfCalls();
	}

	int numberOfHooksMovedUpwards = cg.moveHooksUpwards();
	std::cout << "Move hooks upwards: " << numberOfHooksMovedUpwards << std::endl;

	// Now we can again sum up the calls to functions we would instrument...
	for(auto node : cg.getNodesToMark()){
		optimizedOverhead += overheadPerCallInNanos * node->getNumberOfCalls();
		optimizedNumberOfInstrCall += node->getNumberOfCalls();
	}

#if PRINT_DOT
	cg.printDOT("mark");
#endif	
	std::cout << " ------ Statistics ------ \nA cg-analysis instrumentation would mark: " << cg.getNodesToMark().size() << " out of " << cg.getSize() << "\n" ;
	std::cout << "Function calls:\t\t\t" << numberOfCalls << "\n# instr. Function Calls:\t" << numberOfInstrCalls << std::endl;
	std::cout << "OVH:\t\t\t\t" << (numberOfInstrCalls * overheadPerCallInNanos) / (1e9) << std::endl;
	std::cout << "Adding:\t\t\t\t" << overAllOverhead << " nanos" << std::endl;
	std::cout << "In Seconds:\t\t\t" << (overAllOverhead / (1e9)) << std::endl;
	std::cout << "After Optimization:\t\t" << (optimizedOverhead / (1e9)) << " seconds." << std::endl;
	std::cout << "Function calls optimized:\t" << optimizedNumberOfInstrCall << std::endl;
	std::cout << "Saving:\t\t\t\t" << ((overAllOverhead - optimizedOverhead) / (1e9)) << std::endl;

} catch(cube::RuntimeError e){
	std::cout << e.get_msg() << std::endl;
}

return EXIT_SUCCESS;
}
