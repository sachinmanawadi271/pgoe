#ifndef CUBECALLGRAPHTOOL_IPCG_READER_H
#define CUBECALLGRAPHTOOL_IPCG_READER_H

#include "CallgraphManager.h"

#include <string>

namespace IPCGAnal {
	CallgraphManager build(std::string filepath);
};

#endif
