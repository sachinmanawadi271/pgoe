#include "CubeReader.h"


CallgraphManager CubeCallgraphBuilder::build(std::string filePath, int samplesPerSecond, bool useMangledNames, double uninstrumentedTime) {

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

		double smallestFunctionInSeconds = 1e9;
		std::string smallestFunctionName;
		int edgesWithZeroRuntime = 0;

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
			auto childNode = cnode->get_callee();

			auto parentName = useMangledNames ? parentNode->get_mangled_name() : parentNode->get_name();
			auto childName = useMangledNames ? childNode->get_mangled_name() : childNode->get_name();

			for(unsigned int i = 0; i < threads.size(); i++) {
				unsigned long long numberOfCalls = (unsigned long long) cube.get_sev(visitsMetric, cnode, threads.at(i));
				double timeInSeconds = cube.get_sev(timeMetric, cnode, threads.at(i));

				///XXX overhead compensation wip
//				std::cout << "calls:" << numberOfCalls << " sec:" << timeInSeconds << std::endl;
//				timeInSeconds = timeInSeconds - ((double) (numberOfCalls * CgConfig::nanosPerNormalProbe) / 1e9);
//				if (timeInSeconds < 0) {
//					timeInSeconds = 0;
//					edgesWithZeroRuntime++;
//				}

				cg->putEdge(parentName, parentNode->get_mod(), parentNode->get_begn_ln(),
						childName, numberOfCalls, timeInSeconds);

				overallNumberOfCalls += numberOfCalls;
				overallRuntime += timeInSeconds;

				double runtimePerCallInSeconds = timeInSeconds / numberOfCalls;
				if (runtimePerCallInSeconds < smallestFunctionInSeconds) {
					smallestFunctionInSeconds = runtimePerCallInSeconds;
					smallestFunctionName = childName;
				}
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
		double probePercent = probeSeconds / (overallRuntime-probeSeconds) * 100;

		std::cout << "Finished construction .." << std::endl
				<< "    " << "numberOfCalls: " << overallNumberOfCalls << " | MPI: " << numberOfMPICalls
				<< " | normal: " << numberOfNormalCalls << std::endl
				<< "    " << "runtime: "  << overallRuntime << " seconds" << std::endl
				<< "      " << "estimatedOverhead: " << probeSeconds << " seconds"
				<< " or " << std::setprecision(4) << probePercent << " % (vs cube time)"<< std::endl;

		if (uninstrumentedTime > .0) {
			double deltaSeconds = overallRuntime - probeSeconds - uninstrumentedTime;
			double deltaPercent = deltaSeconds / uninstrumentedTime * 100;
			std::cout << "      " << "delta: " << std::setprecision(6) << deltaSeconds << " seconds"
					<< " or " << std::setprecision(4) << deltaPercent << " % (vs ref time)" << std::endl;
		}

		std::cout << "    " << "target samplesPerSecond : " << samplesPerSecond
				<< " | smallestFunction : " << smallestFunctionName << " : " << smallestFunctionInSeconds * 1e9 << "ns"
				<< " | edgesWithZeroRuntime: " << edgesWithZeroRuntime
				<< std::setprecision(6) << std::endl << std::endl;

		return *cg;

	} catch (const cube::RuntimeError& e) {
		std::cout << "CubeReader failed: " << std::endl
				<< e.get_msg() << std::endl;
		exit(-1);
	}

}
