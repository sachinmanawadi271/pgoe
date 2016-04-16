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

//	cg.registerEstimatorPhase(new LibUnwindEstimatorPhase(true));
//	cg.registerEstimatorPhase(new LibUnwindEstimatorPhase(false));
//	cg.registerEstimatorPhase(new ResetEstimatorPhase());

	cg.registerEstimatorPhase(new InstrumentEstimatorPhase());
//	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());
//
	cg.registerEstimatorPhase(new UnwindEstimatorPhase(false));
	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());

	cg.registerEstimatorPhase(new ResetEstimatorPhase());
	cg.registerEstimatorPhase(new InstrumentEstimatorPhase());

	cg.registerEstimatorPhase(new UnwindEstimatorPhase(true));
	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());

//	cg.registerEstimatorPhase(new InstrumentEstimatorPhase());
//	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());
//
//	cg.registerEstimatorPhase(new UnwindEstimatorPhase(true));
//	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());

//	cg.registerEstimatorPhase(new ResetEstimatorPhase());
//	cg.registerEstimatorPhase(new WLInstrEstimatorPhase("out/instrumented-" + c->appName + "-Instrument.txt"));
//	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());

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
		if (arg=="-ref" || arg=="-r") {
			c.referenceRuntime = atof(argv[++i]);
			continue;
		}
		if (arg=="-mangled" || arg=="-m") {
			c.useMangledNames=true;
			continue;
		}
		if (arg=="-half" || arg=="-h") {
			c.nanosPerHalfProbe = atoi(argv[++i]);
			continue;
		}
		if (arg=="-tiny" || arg=="-t") {
			c.tinyReport = true;
			continue;
		}
	}

	std::string filePath(argv[1]);
	std::string fileName = filePath.substr(filePath.find_last_of('/')+1);
	c.appName = fileName.substr(0, fileName.find_last_of('.'));	// remove .*

	CallgraphManager cg(&c);
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
