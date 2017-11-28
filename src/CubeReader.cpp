#include "CubeReader.h"


CallgraphManager CubeCallgraphBuilder::build(std::string filePath, Config* c) {

	CallgraphManager* cg = new CallgraphManager(c);

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
				cg->findOrCreateNode(c->useMangledNames ? cnode->get_callee()->get_mangled_name() : cnode->get_callee()->get_name(), cube.get_sev(timeMetric, cnode, threads.at(0)));
				continue;
			}

			// Put the parent/child pair into our call graph
			auto parentNode = cnode->get_parent()->get_callee();	// RN: don't trust no one. It IS the parent node
			auto childNode = cnode->get_callee();

			auto parentName = c->useMangledNames ? parentNode->get_mangled_name() : parentNode->get_name();
			auto childName = c->useMangledNames ? childNode->get_mangled_name() : childNode->get_name();

			for(unsigned int i = 0; i < threads.size(); i++) {
				unsigned long long numberOfCalls = (unsigned long long) cube.get_sev(visitsMetric, cnode, threads.at(i));
				double timeInSeconds = cube.get_sev(timeMetric, cnode, threads.at(i));

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

		// read in samples per second TODO these are hardcoded for 10kHz
		auto samplesFilename = c->samplesFile;
		if (!samplesFilename.empty()) {
			std::ifstream inFile(samplesFilename);
			if (!inFile.is_open())  {
				std::cerr << "Can not open samples-file: " << samplesFilename << std::endl;
				exit(1);
			}
			std::string line;
			while (std::getline(inFile, line)) {
				std::string name;
				unsigned long long numberOfSamples;

				inFile >> numberOfSamples >> name;

				cg->putNumberOfSamples(name, numberOfSamples);
			}
		}

		c->actualRuntime = overallRuntime;
		bool hasRefTime = c->referenceRuntime > .0;
		unsigned long long normalProbeNanos = overallNumberOfCalls * CgConfig::nanosPerNormalProbe;

		double probeSeconds = (double (normalProbeNanos)) / (1000*1000*1000);
		double probePercent = probeSeconds / (overallRuntime-probeSeconds) * 100;
		if (hasRefTime) {
			probeSeconds = overallRuntime - c->referenceRuntime;
			probePercent = probeSeconds / c->referenceRuntime * 100;
		}

		std::cout << "####################### " << c->appName << " #######################" << std::endl;
		if (!hasRefTime) {
			std::cout << "HAS NO REF TIME" << std::endl;
		}
		std::cout << "    " << "numberOfCalls: " << overallNumberOfCalls
				<< " | " << "samplesPerSecond : " << CgConfig::samplesPerSecond << std::endl
				<< "    " << "runtime: " << overallRuntime << " s (ref " << c->referenceRuntime << " s)";
		std::cout << " | " << "overhead: " << (hasRefTime ? "" : "(est.) ") << probeSeconds << " s"
						<< " or " << std::setprecision(4) << probePercent << " %" << std::endl;

		std::cout
				<< "    smallestFunction : " << smallestFunctionName << " : " << smallestFunctionInSeconds * 1e9 << "ns"
				<< " | edgesWithZeroRuntime: " << edgesWithZeroRuntime
				<< std::setprecision(6) << std::endl << std::endl;

		return *cg;

	} catch (const cube::RuntimeError& e) {
		std::cout << "CubeReader failed: " << std::endl
				<< e.get_msg() << std::endl;
		exit(-1);
	}

}


float CubeCallgraphBuilder::CalculateRuntimeThreshold(CallgraphManager *cg) {
    int i=0;
    Callgraph cgptr = cg->getCallgraph(cg);
    CgNodePtrSet cgptrset = cgptr.getGraph();
    int nodes_count = cgptrset.size();
    float data[nodes_count];
    //CgNodePtrSet::iterator it
    for ( auto it = std::begin( cgptrset ); it != std::end(cgptrset ); ++it )
    {
        std::shared_ptr<CgNode> CgNodeptr = *it;
        std::cout<<CgNodeptr->getFunctionName()<<"->"<<CgNodeptr->getInclusiveRuntimeInSeconds()<<"\n";
        if(CgNodeptr->getInclusiveRuntimeInSeconds() > 0){
            data[i++] = CgNodeptr->getInclusiveRuntimeInSeconds();
        }
    }
    float ret_val = bucket_sort(data,i);
    return ret_val;
}

/*

float CubeCallgraphBuilder::CalculateRuntimeThreshold(std::string filePath, Config* c) {

	CallgraphManager* cg = new CallgraphManager(c);

	try {
		// Create cube instance
		cube::Cube cube;
		// Read our cube file
		cube.openCubeReport( filePath );
		// Get the cube nodes
		const std::vector<cube::Cnode*>& cnodes = cube.get_cnodev();
        	int nodes_count = cnodes.size();
        	float data[nodes_count];
        	int j=0;
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
				//cg->findOrCreateNode(c->useMangledNames ? cnode->get_callee()->get_mangled_name() : cnode->get_callee()->get_name(), cube.get_sev(timeMetric, cnode, threads.at(0)));
                double timeInSeconds = cube.get_sev(timeMetric, cnode, threads[0]);
                data[j] = timeInSeconds;
                std::cout<<"main"<<"->"<<timeInSeconds<<"\n";
                continue;
			}
            j++;

			// Put the parent/child pair into our call graph
			auto parentNode = cnode->get_parent()->get_callee();	// RN: don't trust no one. It IS the parent node
			auto childNode = cnode->get_callee();
			//childNode->setState(CgNodeState::INSTRUMENT_WITNESS);
			//childNode->isCubeInstr = 1;
			auto parentName = c->useMangledNames ? parentNode->get_mangled_name() : parentNode->get_name();
			auto childName = c->useMangledNames ? childNode->get_mangled_name() : childNode->get_name();

			for(unsigned int i = 0; i < threads.size(); i++) {
				//unsigned long long numberOfCalls = (unsigned long long) cube.get_sev(visitsMetric, cnode, threads.at(i));
				double timeInSeconds = cube.get_sev(timeMetric, cnode, threads.at(i));
                		data[j] = timeInSeconds;
                		std::cout<<childName<<"->"<<timeInSeconds<<"\n";
				//childNode->setState(CgNodeState::INSTRUMENT_WITNESS);
				//cg->putEdge(parentName, parentNode->get_mod(), parentNode->get_begn_ln(),
							//childName, numberOfCalls, timeInSeconds);

				//overallNumberOfCalls += numberOfCalls;
				overallRuntime += timeInSeconds;

				//double runtimePerCallInSeconds = timeInSeconds / numberOfCalls;
				//if (runtimePerCallInSeconds < smallestFunctionInSeconds) {
				//	smallestFunctionInSeconds = runtimePerCallInSeconds;
				//	smallestFunctionName = childName;
				//}
			}
            //data[j] = overallRuntime;
            //std::cout<<parentName<<"->"<<overallRuntime<<"\n";
		}
		//std::cout<<"Cube Nodes\n"<<cube_nodes;
		// read in samples per second TODO these are hardcoded for 10kHz


		std::cout << "Total nodes in Cube file:"<<nodes_count<<"\n";
        float ret_val = bucket_sort(data,nodes_count);
		return ret_val;

	} catch (const cube::RuntimeError& e) {
		std::cout << "CubeReader failed: " << std::endl
				  << e.get_msg() << std::endl;
		exit(-1);
	}

}
 */

float CubeCallgraphBuilder::bucket_sort(float arr[],int n) {
    /*float minValue = data[0];
    float maxValue = data[0];

    for (int i = 1; i < count; i++)
    {
        if (data[i] > maxValue)
            maxValue = data[i];
        if (data[i] < minValue)
            minValue = data[i];
    }

    int bucketLength = maxValue - minValue + 1;
    std::vector<float>* bucket = new std::vector<float>[bucketLength];

    for (int i = 0; i < bucketLength; i++)
    {
        bucket[i] = std::vector<float>();
    }

    for (int i = 0; i < count; i++)
    {
        bucket[data[i] - minValue].push_back(data[i]);
    }

    int k = 0;
    for (int i = 0; i < bucketLength; i++)
    {
        int bucketSize = bucket[i].size();

        if (bucketSize > 0)
        {
            for (int j = 0; j < bucketSize; j++)
            {
                data[k] = bucket[i][j];
                k++;
            }
        }
    }*/

    float minValue = arr[0];
    float maxValue = arr[0];

    for (int i = 0; i < n; i++)
    {
        if (arr[i] > maxValue)
            maxValue = arr[i];
        if (arr[i] < minValue)
            minValue = arr[i];
    }
    int bucketLength = (int)(maxValue - minValue + 1);
    std::vector<float> b[bucketLength];
    int bi=0;

    // 2) Put array elements in different buckets
    for (int i=0; i<n; i++)
    {

        /*if(arr[i] < 1){ bi = 0; }
        else if(arr[i] < 2){bi=1;}
        else if(arr[i] < 3){bi=2;}
        else if(arr[i] < 4){bi=3;}
        else if(arr[i] < 5){bi=4;}
        else{bi = 5;}*/
        int bi = (int)(arr[i] - minValue); // Index in bucket
        b[bi].push_back(arr[i]);

    }

    // 3) Sort individual buckets
    for (int i=0; i<bucketLength; i++)
        sort(b[i].begin(), b[i].end());

    int index = 0;
    for (int i = 0; i < bucketLength; i++)
        for (int j = 0; j < b[i].size(); j++)
            arr[index++] = b[i][j];

    int threshold_index = (90*n)/100;
    return arr[threshold_index];


}

CallgraphManager CubeCallgraphBuilder::build_from_ipcg(std::string filePath, Config* c,CallgraphManager* cg) {

    if(cg == NULL){
        CallgraphManager* cg = new CallgraphManager(c);
    }

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
        //int cube_nodes = 0;
        for(auto cnode : cnodes){
            //cube_nodes++;
            // I don't know when this happens, but it does.
            if(cnode->get_parent() == nullptr) {
                cg->findOrCreateNode(c->useMangledNames ? cnode->get_callee()->get_mangled_name() : cnode->get_callee()->get_name(), cube.get_sev(timeMetric, cnode, threads.at(0)));
                continue;
            }

            // Put the parent/child pair into our call graph
            auto parentNode = cnode->get_parent()->get_callee();	// RN: don't trust no one. It IS the parent node
            auto childNode = cnode->get_callee();
            auto parentName = c->useMangledNames ? parentNode->get_mangled_name() : parentNode->get_name();
            auto childName = c->useMangledNames ? childNode->get_mangled_name() : childNode->get_name();

            for(unsigned int i = 0; i < threads.size(); i++) {
                unsigned long long numberOfCalls = (unsigned long long) cube.get_sev(visitsMetric, cnode, threads.at(i));
                double timeInSeconds = cube.get_sev(timeMetric, cnode, threads.at(i));

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
        //std::cout<<"Cube Nodes\n"<<cube_nodes;
        // read in samples per second TODO these are hardcoded for 10kHz
        auto samplesFilename = c->samplesFile;
        if (!samplesFilename.empty()) {
            std::ifstream inFile(samplesFilename);
            if (!inFile.is_open())  {
                std::cerr << "Can not open samples-file: " << samplesFilename << std::endl;
                exit(1);
            }
            std::string line;
            while (std::getline(inFile, line)) {
                std::string name;
                unsigned long long numberOfSamples;

                inFile >> numberOfSamples >> name;

                cg->putNumberOfSamples(name, numberOfSamples);
            }
        }

        c->actualRuntime = overallRuntime;
        bool hasRefTime = c->referenceRuntime > .0;
        unsigned long long normalProbeNanos = overallNumberOfCalls * CgConfig::nanosPerNormalProbe;

        double probeSeconds = (double (normalProbeNanos)) / (1000*1000*1000);
        double probePercent = probeSeconds / (overallRuntime-probeSeconds) * 100;
        if (hasRefTime) {
            probeSeconds = overallRuntime - c->referenceRuntime;
            probePercent = probeSeconds / c->referenceRuntime * 100;
        }
        std::cout << "####################### " << c->appName << " #######################" << std::endl;
        if (!hasRefTime) {
            std::cout << "HAS NO REF TIME" << std::endl;
        }
        std::cout << "    " << "numberOfCalls: " << overallNumberOfCalls
                  << " | " << "samplesPerSecond : " << CgConfig::samplesPerSecond << std::endl
                  << "    " << "runtime: " << overallRuntime << " s (ref " << c->referenceRuntime << " s)";
        std::cout << " | " << "overhead: " << (hasRefTime ? "" : "(est.) ") << probeSeconds << " s"
                  << " or " << std::setprecision(4) << probePercent << " %" << std::endl;

        std::cout
                << "    smallestFunction : " << smallestFunctionName << " : " << smallestFunctionInSeconds * 1e9 << "ns"
                << " | edgesWithZeroRuntime: " << edgesWithZeroRuntime
                << std::setprecision(6) << std::endl << std::endl;

        return *cg;

    } catch (const cube::RuntimeError& e) {
        std::cout << "CubeReader failed: " << std::endl
                  << e.get_msg() << std::endl;
        exit(-1);
    }

}
