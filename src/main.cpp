#include <fstream>
#include <cstdlib>
#include <vector>

#include "CubeReader.h"
#include "DotReader.h"
#include "IPCGReader.h"

#include "Callgraph.h"

#include "SanityCheckEstimatorPhase.h"
#include "EdgeBasedOptimumEstimatorPhase.h"
#include "NodeBasedOptimumEstimatorPhase.h"
#include "ProximityMeasureEstimatorPhase.h"
#include "IPCGEstimatorPhase.h"

void registerEstimatorPhases(CallgraphManager& cg, Config* c) {

	cg.registerEstimatorPhase(new OverheadCompensationEstimatorPhase(c->nanosPerHalfProbe));

	cg.registerEstimatorPhase(new InstrumentEstimatorPhase());
	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());

	cg.registerEstimatorPhase(new UnwindEstimatorPhase());
	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());

//	cg.registerEstimatorPhase(new DeleteOneInstrumentationEstimatorPhase());
//	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());
//
//	cg.registerEstimatorPhase(new ResetEstimatorPhase());
//	cg.registerEstimatorPhase(new ConjunctionEstimatorPhase(true));
//	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());
//
//	cg.registerEstimatorPhase(new ResetEstimatorPhase());
//	cg.registerEstimatorPhase(new ConjunctionEstimatorPhase(false));
//	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());
//
//	cg.registerEstimatorPhase(new ResetEstimatorPhase());
//	cg.registerEstimatorPhase(new RemoveUnrelatedNodesEstimatorPhase(false, true));
//	cg.registerEstimatorPhase(new InstrumentEstimatorPhase());
//	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());
//
//	cg.registerEstimatorPhase(new DeleteOneInstrumentationEstimatorPhase());
//	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());
//
//	cg.registerEstimatorPhase(new ResetEstimatorPhase());
//	cg.registerEstimatorPhase(new ConjunctionEstimatorPhase(true));
//	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());
//
//	cg.registerEstimatorPhase(new ResetEstimatorPhase());
//	cg.registerEstimatorPhase(new ConjunctionEstimatorPhase(false));
//	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());

//	cg.registerEstimatorPhase(new ResetEstimatorPhase());
	// node based
//	cg.registerEstimatorPhase(new OptimalNodeBasedEstimatorPhase());
//	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());

//		cg.registerEstimatorPhase(new ProximityMeasureEstimatorPhase(c->otherPath));
}

bool stringEndsWith(const std::string& s, const std::string& suffix) {
	return s.size() >= suffix.size()
			&& s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int main(int argc, char** argv) {

	std::cout << "Usage: " << argv[0] << " /PATH/TO/CUBEX/PROFILE"
			<< " [-other /PATH/TO/PROFILE/TO/COMPARE/TO]"
			<< " [-samples NUMBER_OF_SAMPLES_PER_SECOND]"
			<< " [-ref UNINSTRUMENTED_RUNTIME_SECONDS]"
			<< " [-mangled]"
			<< std::endl << std::endl;

	if (argc == 1) {
		std::cerr << "ERROR: too few arguments." << std::endl;
		exit(-1);
	}

	Config c;

	for(int i = 1; i < argc; ++i) {
		auto arg = std::string(argv[i]);

		if (arg=="-other") {
			c.otherPath = std::string(argv[++i]);
			continue;
		}
		if (arg=="-samples") {
			CgConfig::samplesPerSecond = atoi(argv[++i]);
			continue;
		}
		if (arg=="-ref") {
			c.uninstrumentedReferenceRuntime = atof(argv[++i]);
			continue;
		}
		if (arg=="-mangled" || arg=="-m") {
			c.useMangledNames=true;
		}
		if (arg=="-half" || arg=="-h") {
			c.nanosPerHalfProbe = atoi(argv[++i]);
		}
	}

	CallgraphManager cg(&c);
	std::string filePath(argv[1]);

	if (stringEndsWith(filePath, ".cubex")) {
		cg = CubeCallgraphBuilder::build(filePath, &c);
	} else if (stringEndsWith(filePath, ".dot")) {
		cg = DOTCallgraphBuilder::build(filePath, &c);
	} else if (stringEndsWith(filePath, ".ipcg")){
		cg = IPCGAnal::build(filePath, &c);
	}	else {
		std::cerr << "ERROR: Unknown file ending in " << filePath << std::endl;
		exit(-1);
	}

	registerEstimatorPhases(cg, &c);

	cg.thatOneLargeMethod();

	return EXIT_SUCCESS;
}
