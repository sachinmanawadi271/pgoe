#include "CubeReader.h"


CallgraphManager CubeCallgraphBuilder::build(std::string filePath, int samplesPerSecond) {

	CallgraphManager* cg = new CallgraphManager(samplesPerSecond);

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
			if(cnode->get_parent() == nullptr) {
				continue;
			}

			// Put the parent/child pair into our call graph
			auto parentNode = cnode->get_parent()->get_callee();	// RN: don't trust no one. It IS the parent node
			auto childnode = cnode->get_callee();

			for(unsigned int i = 0; i < threads.size(); i++) {
				unsigned long long numberOfCalls = (unsigned long long) cube.get_sev(visitsMetric, cnode, threads.at(i));
				double timeInSeconds = cube.get_sev(timeMetric, cnode, threads.at(i));

				cg->putEdge(parentNode->get_name(), parentNode->get_mod(), parentNode->get_begn_ln(),
						childnode->get_name(), numberOfCalls, timeInSeconds);

				overallNumberOfCalls += numberOfCalls;
				overallRuntime += timeInSeconds;
			}
		}

		unsigned long long numberOfMPICalls = 0;

		for (auto function : *cg) {
			if (function->getFunctionName().find("MPI") != std::string::npos) {
				numberOfMPICalls += function->getNumberOfCallsWithCurrentEdges();
			}
		}
		unsigned long long numberOfNormalCalls = overallNumberOfCalls - numberOfMPICalls;
		unsigned long long MPIProbeNanos = numberOfMPICalls * CgConfig::nanosPerMPIProbe;
		unsigned long long normalProbeNanos = numberOfNormalCalls * CgConfig::nanosPerNormalProbe;

		double probeSeconds = (double (MPIProbeNanos + normalProbeNanos)) / (1000*1000*1000);
		double probePercent = probeSeconds / overallRuntime * 100;

		std::cout << "Finished construction .." << std::endl
				<< "\t" << "numberOfCalls: " << overallNumberOfCalls << " | MPI: " << numberOfMPICalls
				<< " | normal: " << numberOfNormalCalls << std::endl
				<< "\t" << "runtime: "  << overallRuntime << " seconds"
				<< " | estimatedOverhead: " << probeSeconds << " seconds"
				<< " or " << std::setprecision(3) << probePercent << " %" << std::endl
				<< "\t" << "target samplesPerSecond : " << samplesPerSecond
				<< std::setprecision(6) << std::endl;

		return *cg;

	} catch (const cube::RuntimeError& e) {
		std::cout << "CubeReader failed: " << std::endl
				<< e.get_msg() << std::endl;
		exit(-1);
	}

}
