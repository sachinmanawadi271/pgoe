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

void registerEstimatorPhases(CallgraphManager& cg, Config* c, int Isipcg,float threshold_Runtime) {
    cg.registerEstimatorPhase(new OverheadCompensationEstimatorPhase(c->nanosPerHalfProbe));
    cg.registerEstimatorPhase(new RemoveUnrelatedNodesEstimatorPhase(true, false));         // remove unrelated

    cg.registerEstimatorPhase(new ResetEstimatorPhase());
    if(!Isipcg){
        //double threshold_Runtime = 0.0;
        //threshold_Runtime = (75*threshold_Runtime)/100;
        std::cout << "New threshold runtime for profiling:"<<threshold_Runtime<<std::endl;
        cg.registerEstimatorPhase(new RuntimeEstimatorPhase(threshold_Runtime));
       // cg.registerEstimatorPhase(new RuntimeEstimatorPhase(0));
    }
    else{
        cg.registerEstimatorPhase(new StatementCountEstimatorPhase(150));
    }

    //cg.registerEstimatorPhase(new WLCallpathDifferentiationEstimatorPhase());
    cg.registerEstimatorPhase(new ResetEstimatorPhase());

/*
    cg.registerEstimatorPhase(new OverheadCompensationEstimatorPhase(c->nanosPerHalfProbe));
	cg.registerEstimatorPhase(new RemoveUnrelatedNodesEstimatorPhase(true, false)); 	// remove unrelated

	cg.registerEstimatorPhase(new LibUnwindEstimatorPhase(false));	// unwind till main
	cg.registerEstimatorPhase(new ResetEstimatorPhase());
	cg.registerEstimatorPhase(new LibUnwindEstimatorPhase(true));		// unwind till unique
	cg.registerEstimatorPhase(new ResetEstimatorPhase());

	cg.registerEstimatorPhase(new InstrumentEstimatorPhase(true));	// instrument all
	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());
	cg.registerEstimatorPhase(new ResetEstimatorPhase());

	cg.registerEstimatorPhase(new InstrumentEstimatorPhase());			// instrument
	cg.registerEstimatorPhase(new UnwindEstimatorPhase(false));			// hybrid (unwind all)
	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());
	cg.registerEstimatorPhase(new ResetEstimatorPhase());

	cg.registerEstimatorPhase(new InstrumentEstimatorPhase(), true);
	cg.registerEstimatorPhase(new UnwStaticLeafEstimatorPhase());			// hybrid static
	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());
	cg.registerEstimatorPhase(new ResetEstimatorPhase());

	cg.registerEstimatorPhase(new RemoveUnrelatedNodesEstimatorPhase(false, true)); 	// aggressive reduction

	cg.registerEstimatorPhase(new InstrumentEstimatorPhase(), true);
	cg.registerEstimatorPhase(new MinInstrHeuristicEstimatorPhase());		// min instrument
	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());
	cg.registerEstimatorPhase(new ResetEstimatorPhase());

	cg.registerEstimatorPhase(new InstrumentEstimatorPhase(), true);
	cg.registerEstimatorPhase(new ConjunctionInstrumentHeuristicEstimatorPhase());		// conj instrument
	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());
	cg.registerEstimatorPhase(new ResetEstimatorPhase());

//		cg.registerEstimatorPhase(new InstrumentEstimatorPhase());			// instrument
//		cg.registerEstimatorPhase(new UnwindEstimatorPhase(false, true));			// hybrid (unwind all)
////		cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());
//		cg.registerEstimatorPhase(new ResetEstimatorPhase());
//
//	cg.registerEstimatorPhase(new GraphStatsEstimatorPhase());*/

}

bool stringEndsWith(const std::string& s, const std::string& suffix) {
	return s.size() >= suffix.size()
			&& s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int main(int argc, char** argv) {

	if (argc == 1) {
		std::cerr << "ERROR: too few arguments." << std::endl;
		exit(-1);
	}

	Config c;
    std::cout<<argc;
	for(int i = argc; i < argc; ++i) {
		auto arg = std::string(argv[i]);

		if (arg=="--other") {
			c.otherPath = std::string(argv[++i]);
			continue;
		}
		if (arg=="--samples" || arg=="-s") {
			CgConfig::samplesPerSecond = atoi(argv[++i]);
			continue;
		}
		if (arg=="--ref" || arg=="-r") {
			c.referenceRuntime = atof(argv[++i]);
			continue;
		}
		if (arg=="--mangled" || arg=="-m") {
			c.useMangledNames=true;
			continue;
		}
		if (arg=="--half" || arg=="-h") {
			c.nanosPerHalfProbe = atoi(argv[++i]);
			continue;
		}
		if (arg=="--tiny" || arg=="-t") {
			c.tinyReport = true;
			continue;
		}
		if (arg=="--ignore-sampling" || arg=="-i") {
			c.ignoreSamplingOv = true;
			continue;
		}
		if (arg=="--samples-file" || arg=="-f") {
			c.samplesFile = "active";	// ugly hack
			continue;
		}
		if (arg=="--greedy-Unwind" || arg=="-g") {
			c.greedyUnwind = true;
			continue;
		}

		std::cerr << "Unknown option: " << argv[i] << std::endl;

		std::cout << "Usage: " << argv[0] << " /PATH/TO/CUBEX/PROFILE"
				<< " [--other|-o /PATH/TO/PROFILE/TO/COMPARE/TO]"
				<< " [--samples|-s NUMBER_OF_SAMPLES_PER_SECOND]"
				<< " [--ref|-r UNINSTRUMENTED_RUNTIME_SECONDS]"
				<< " [--half|-h NANOS_FOR_OVERHEAD_COMPENSATION"
				<< " [--mangled|-m]"
				<< " [--tiny|-t]"
				<< std::endl << std::endl;
	}


    //for static instrumentation
    std::string filePath_ipcg(argv[1]);
    std::string ipcg_fileName = filePath_ipcg.substr(filePath_ipcg.find_last_of('/')+1);
    c.appName = ipcg_fileName.substr(0, ipcg_fileName.find_last_of('.'));

    float runTimethreshold = 0;
    CallgraphManager cg(&c);
    CallgraphManager cg_ipcg(&c);
	if(stringEndsWith(filePath_ipcg, ".ipcg")){
        cg_ipcg = IPCGAnal::build(ipcg_fileName, &c);
        registerEstimatorPhases(cg_ipcg, &c, 1,0);

        cg_ipcg.thatOneLargeMethod();
    }


    if(argc > 2) {
        //for dynamic instrumentation
        std::string filePath(argv[2]);
        std::string fileName = filePath.substr(filePath.find_last_of('/') + 1);
        c.appName = fileName.substr(0, fileName.find_last_of('.'));    // remove .*

        if (!c.samplesFile.empty()) {
            c.samplesFile = filePath.substr(0, filePath.find_last_of('.'))+".samples";
            std::cout << c.samplesFile << std::endl;
        }

        if (stringEndsWith(filePath, ".cubex")) {

            cg = CubeCallgraphBuilder::build_from_ipcg(filePath, &c, &cg_ipcg);
            runTimethreshold = CubeCallgraphBuilder::CalculateRuntimeThreshold(&cg);
            //cg = CubeCallgraphBuilder::build(filePath, &c);
        } else if (stringEndsWith(filePath, ".dot")) {
            cg = DOTCallgraphBuilder::build(filePath, &c);
        } /*else if (stringEndsWith(filePath, ".ipcg")){
		    cg = IPCGAnal::build(filePath, &c);
	    }*/    else {
            std::cerr << "ERROR: Unknown file ending in " << filePath << std::endl;
            exit(-1);
        }
        c.totalRuntime = c.actualRuntime;
        registerEstimatorPhases(cg, &c, 0,runTimethreshold);

        cg.thatOneLargeMethod();
        std::cout << "Total Running time by me : " << c.totalRuntime;
    }
    return EXIT_SUCCESS;
}
