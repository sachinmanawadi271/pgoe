#include "CgHelper.h"

namespace CgHelper {

	bool isConjunction(std::shared_ptr<CgNode> node) {
		return (node->getParentNodes().size() > 1);
	}

}
