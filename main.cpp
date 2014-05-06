#include <fstream>
#include "callgraph.h"
#include "Cube.h"
#include "CubeMetric.h"
#include <cstdlib>

#include <cassert>

#define VERBOSE 2 
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
	std::cout << cnodes[0]->get_callee()->get_name() << std::endl;

	cube::Metric* metric = cube.get_met("visits");
	const std::vector<cube::Thread*> threads = cube.get_thrdv();

	for(auto node : cnodes){
		// I don't know when this happens, but it does.
		if(node->get_parent() == NULL)
			continue;
		// Put the caller/callee pair into our callgraph
		cg.putFunction(node->get_parent()->get_callee()->get_name(), node->get_parent()->get_callee()->get_mod(), node->get_parent()->get_callee()->get_begn_ln(), node->get_callee()->get_name(), cube.get_sev(metric, node, threads.at(0)));
/*		std::cout << "[ node->get_callee()->get_name(): " << node->get_callee()->get_name() << " ]\n";
		if(node->get_parent() != NULL)
			std::cout << "[ node->get_parent()->get_callee()->get_name()" << node->get_parent()->get_callee()->get_name() << " ]" << std::endl;
//		cg.putFunction(node->get_callee()->get_name(), node->get_callee()->get_mod(), node->get_callee()->get_begn_ln(), node->get_callee()->get_name(), cube.get_sev(metric, node, threads.at(0)));
*/
		numberOfCalls += cube.get_sev(metric, node, threads.at(0));
		// Also keep track of threading things...
		if(threads.size() > 1)
			for(int i = 1; i < threads.size(); i++){
				cg.putFunction(node->get_parent()->get_callee()->get_name(), node->get_parent()->get_callee()->get_mod(), node->get_parent()->get_callee()->get_begn_ln(), node->get_callee()->get_name(), cube.get_sev(metric, node, threads.at(i)));
				numberOfCalls += cube.get_sev(metric, node, threads.at(i));
}
	}
	std::cout << "Finished construction of cg. Now estimating InstROs overhead..." << std::endl;
//return 0;
#if PRINT_DOT == 1
	cg.printDOT();
#endif

	/** JP: This is code to estimate the generated overhead via call-graph guided hook placement. */
	bool once = true; // What exactly is that good for...?
	const int overheadPerCallInNanos = 4; 
	unsigned long long overAllOverhead = 0;
	unsigned long long numberOfInstrCalls = 0;
		
#if VERBOSE > 1
	std::cout << " ---- CubeCallGraphTool VERBOSE info begin ---- \n" << "Graph includes: " << cg.getSize() << "\nOur algorithm would mark: \n";
	for(auto node : cg.getNodesToMark()){
		node->printMinimal();
		std::cout << "\n";
	}
	std::cout << " ---- CubeCallGraphTool VERBOSE info end ---- \n" << std::endl;
#endif
	// sum up the calls to function we would instrument
	for(auto node : cg.getNodesToMark()){
		overAllOverhead += overheadPerCallInNanos * node->getNumberOfCalls();
		numberOfInstrCalls += node->getNumberOfCalls();
	}
		
	std::cout << " ------ Statistics ------ \nA cg-analysis instrumentation would mark: " << cg.getNodesToMark().size() << " out of " << cg.getSize() << "\n" ;
	std::cout << "Function calls:\t\t\t" << numberOfCalls << "\n# instr. Function Calls:\t" << numberOfInstrCalls << std::endl;
	std::cout << "OVH:\t\t\t\t" << (numberOfInstrCalls * overheadPerCallInNanos) / (1e9) << std::endl;
	std::cout << "Adding:\t\t\t\t" << overAllOverhead << " nanos" << std::endl;
	std::cout << "In Seconds:\t\t\t" << (overAllOverhead / (1e9)) << std::endl;

} catch(cube::RuntimeError e){
	std::cout << e.get_msg() << std::endl;
}

return EXIT_SUCCESS;
}
