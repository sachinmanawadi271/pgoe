
#ifndef CGNODEHELPER_H_
#define CGNODEHELPER_H_

#include <memory>
#include "CgNode.h"

namespace CgHelper {

	bool isConjunction(std::shared_ptr<CgNode> node);

	bool isOnInstrumentedPath(std::shared_ptr<CgNode> node);

	bool isConnectedOnSpantree(std::shared_ptr<CgNode> n1, std::shared_ptr<CgNode> n2);
}

#endif
