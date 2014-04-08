#include <fstream>
#include "callgraph.h"
#include "Cube.h"
#include "CubeMetric.h"
#include <cstdlib>

#include <cassert>

int main(int argc, char** argv){


//Callgraph cg;
// The cube documentation says one should always use a try/catch thing around
try{
	// Create cube instance
	cube::Cube cube;
	// Read our cube file
	cube.openCubeReport( argv[1] );
	std::cout << "Reading input.. done." << std::endl;


	const std::vector<cube::Cnode*>& cnodes = cube.get_cnodev();

#if 0
	std::cout << cnodes[0]->get_callee()->get_name() << std::endl;
//	std::cout << cnodes[0]->get_callee()->get_mod() << std::endl;

	for(auto node : cnodes){
		if(node->get_parent() == NULL)
			continue;
		cg.putFunction(node->get_parent()->get_callee()->get_name(), node->get_callee()->get_name());
//		cg.putFunction(node->get_callee()->get_name(), node->get_parent()->get_callee()->get_name());
	}


	cg.print();
	cg.printDOT();

	for(auto node : cg.getNodesToMark())
		node->printMinimal();
#endif

	/** JP: This is code to estimate the generated overhead via call-graph guided hook placement. */
	bool once = true;
	std::vector<double> calls;
	cube::Metric* metric = cube.get_met( "visits" );
	const std::vector<cube::Thread*> threads = cube.get_thrdv();
	for( auto node : cnodes){
		std::cout << node->get_mod() << " : " << node->get_callee()->get_name() << " : " << cube.get_sev(metric, node, threads.at(0)) << std::endl;
	}
#if 0
//		calls.push_back(cube.get_sev(metric, node, threads.at(0)));
		if(!once){
			const std::vector<cube::Metric*> metrics = cube.get_metv();
			for(auto m : metrics){
				std::cout << "Metric unique name: " << m->get_uniq_name() << " Metric display name: " << m->get_disp_name() << std::endl;
			}
			once = true;
		}
	}

	for(auto d : calls){
		std::cout << d << std::endl;
	}
#endif

} catch(cube::RuntimeError e){
	std::cout << e.get_msg() << std::endl;
}

return EXIT_SUCCESS;
}
