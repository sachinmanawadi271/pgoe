
#include "CallgraphManager.h"

#include <Cube.h>
#include <CubeMetric.h>

#include <string>

#ifndef CUBEREADER_H_
#define CUBEREADER_H_

/**
 * \author roman
 * \author JPL
 */
namespace CubeCallgraphBuilder {

	CallgraphManager build(std::string filePath, int samplesPerSecond, double uninstrumentedTime = .0);

};



#endif
