#include "CubeReader.h"


Callgraph CubeCallgraphBuilder::build(std::string filePath, int samplesPerSecond) {

	Callgraph* cg = new Callgraph(samplesPerSecond);

	try {
	// Create cube instance
		cube::Cube cube;
		// Read our cube file
		cube.openCubeReport( filePath );
		// Get the cube nodes
		const std::vector<cube::Cnode*>& cnodes = cube.get_cnodev();
		unsigned long long overallNumberOfCalls = 0;
		double overallRuntime = 0.0;

		cube::Metric* timeMetric = cube.get_met("time");
		cube::Metric* visitsMetric = cube.get_met("visits");

		const std::vector<cube::Thread*> threads = cube.get_thrdv();

		for(auto cnode : cnodes){
			// I don't know when this happens, but it does.
			if(cnode->get_parent() == NULL) {
				continue;
			}

			// Put the parent/child pair into our call graph
			auto parentNode = cnode->get_parent()->get_callee();	// RN: don't trust no one. It IS the parent node
			auto childnode = cnode->get_callee();

			for(unsigned int i = 0; i < threads.size(); i++) {
				unsigned long long numberOfCalls = (unsigned long long) cube.get_sev(visitsMetric, cnode, threads.at(i));
				double timeInSeconds = cube.get_sev(timeMetric, cnode, threads.at(i));

				cg->putFunction(parentNode->get_name(), parentNode->get_mod(), parentNode->get_begn_ln(),
						childnode->get_name(), numberOfCalls, timeInSeconds);

				overallNumberOfCalls += numberOfCalls;
				overallRuntime += timeInSeconds;
			}
		}
		std::cout << "Finished construction .."
				<< " numberOfCalls: " << overallNumberOfCalls
				<< " | runtime: " << overallRuntime << " s" << std::endl;

		return *cg;

	} catch (const cube::RuntimeError& e) {
		std::cout << "CubeReader failed: " << std::endl
				<< e.get_msg() << std::endl;
		exit(-1);
	}

}
