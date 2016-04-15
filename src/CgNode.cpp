
#include "CgNode.h"
#include "CgHelper.h"

#define RENDER_DEPS 0

CgNode::CgNode(std::string function) {
  this->functionName = function;
  this->parentNodes = CgNodePtrSet();
  this->childNodes = CgNodePtrSet();

  this->spantreeParents = CgNodePtrSet();

  this->line = -1;
  this->state = CgNodeState::NONE;
  this->numberOfUnwindSteps = 0;

  this->runtimeInSeconds = 0.0;
  this->inclusiveRuntimeInSeconds = .0;
  this->expectedNumberOfSamples = 0L;

  this->numberOfCalls = 0;
  this->uniqueCallPath = false;

  this->numberOfStatements = 0;
}

void CgNode::addChildNode(CgNodePtr childNode) { childNodes.insert(childNode); }

void CgNode::addParentNode(CgNodePtr parentNode) {
  parentNodes.insert(parentNode);
}

void CgNode::removeChildNode(CgNodePtr childNode) {
  childNodes.erase(childNode);
}

void CgNode::removeParentNode(CgNodePtr parentNode) {
  numberOfCallsBy.erase(parentNode);
  parentNodes.erase(parentNode);
}

CgNodePtrSet &CgNode::getMarkerPositions() { return potentialMarkerPositions; }
CgNodePtrSet &CgNode::getDependentConjunctions() {
  return dependentConjunctions;
}

void CgNode::addSpantreeParent(CgNodePtr parentNode) {
  this->spantreeParents.insert(parentNode);
}

bool CgNode::isSpantreeParent(CgNodePtr parentNode) {
  return this->spantreeParents.find(parentNode) != spantreeParents.end();
}

void CgNode::reset() {
  this->state = CgNodeState::NONE;
  this->numberOfUnwindSteps = 0;

  this->spantreeParents.clear();
}

void CgNode::updateNodeAttributes() {

  // this number will not change
  this->numberOfCalls = getNumberOfCallsWithCurrentEdges();

  // has unique call path
  auto parents = getParentNodes();
  CgNodePtrSet visitedNodes;
  while (parents.size() == 1) {
  	if (visitedNodes.find(getUniqueParent()) != visitedNodes.end()) {
  		break;	// this can happen in unconnected subgraphs
  	} else {
  		visitedNodes.insert(getUniqueParent());
  	}
    parents = getUniqueParent()->getParentNodes();
  }
  this->uniqueCallPath = (parents.size() == 0);

  updateExpectedNumberOfSamples();
}

void CgNode::updateExpectedNumberOfSamples() {
	// expected samples in this function (always round up)
	this->expectedNumberOfSamples = (unsigned long long) ( (double) CgConfig::samplesPerSecond * runtimeInSeconds + 0.5);
}

bool CgNode::hasUniqueCallPath() { return uniqueCallPath; }

bool CgNode::isLeafNode() { return getChildNodes().empty(); }
bool CgNode::isRootNode() { return getParentNodes().empty(); }

bool CgNode::hasUniqueParent() { return getParentNodes().size() == 1; }
bool CgNode::hasUniqueChild() { return getChildNodes().size() == 1; }
CgNodePtr CgNode::getUniqueParent() {
  if (!hasUniqueParent()) {
    std::cerr << "Error: no unique parent." << std::endl;
    exit(1);
  }
  return *(getParentNodes().begin());
}
CgNodePtr CgNode::getUniqueChild() {
  if (!hasUniqueChild()) {
    std::cerr << "Error: no unique child." << std::endl;
    exit(1);
  }
  return *(getChildNodes().begin());
}

bool CgNode::isSameFunction(CgNodePtr cgNodeToCompareTo) {
  if (this->functionName.compare(cgNodeToCompareTo->getFunctionName()) == 0) {
    return true;
  }
  return false;
}

std::string CgNode::getFunctionName() const { return this->functionName; }

void CgNode::dumpToDot(std::ofstream &outStream) {
  for (auto parentNode : parentNodes) {

    std::string edgeColor = "";
    if (isSpantreeParent(parentNode)) {
      edgeColor = ", color=red, fontcolor=red";
    }

    outStream << *parentNode << " -> \"" << this->functionName
              << "\" [label=" << this->getNumberOfCalls(parentNode) << edgeColor
              << "];" << std::endl;
  }

#if RENDER_DEPS
  for (auto markerPosition : potentialMarkerPositions) {
    outStream << *markerPosition << " -> \"" << this->functionName
              << "\" [style=dotted, color=grey];" << std::endl;
  }
  for (auto dependentConjunction : dependentConjunctions) {
    outStream << "\"" << this->functionName << "\" -> " << *dependentConjunction
              << " [style=dotted, color=green];" << std::endl;
  }
#endif
}

const CgNodePtrSet &CgNode::getChildNodes() const { return childNodes; }

const CgNodePtrSet &CgNode::getParentNodes() const { return parentNodes; }

void CgNode::addCallData(CgNodePtr parentNode, unsigned long long calls,
                         double timeInSeconds) {

  this->numberOfCallsBy[parentNode] += calls;
  this->runtimeInSeconds += timeInSeconds;
}

void CgNode::setState(CgNodeState state, int numberOfUnwindSteps) {

	// TODO i think this breaks something
	if (this->state == CgNodeState::INSTRUMENT_CONJUNCTION && this->state != state) {
		std::cerr << "# setState old:" << this->state << " new:" << state << std::endl;

		if (state == CgNodeState::INSTRUMENT_WITNESS) {
			return;	// instrument conjunction is stronger
		}
	}

  this->state = state;

  if (state == CgNodeState::UNWIND_SAMPLE || state == CgNodeState::UNWIND_INSTR) {
    this->numberOfUnwindSteps = numberOfUnwindSteps;
  } else {
    this->numberOfUnwindSteps = 0;
  }
}

void CgNode::setInclusiveRuntimeInSeconds(double newInclusiveRuntimeInSeconds){
	inclusiveRuntimeInSeconds = newInclusiveRuntimeInSeconds;
}

double CgNode::getInclusiveRuntimeInSeconds() {
  if (childNodes.size() == 0) {
    inclusiveRuntimeInSeconds = runtimeInSeconds;
  }
	return inclusiveRuntimeInSeconds;
}

bool CgNode::isInstrumented() { return isInstrumentedWitness() || isInstrumentedConjunction(); }
bool CgNode::isInstrumentedWitness() { return state == CgNodeState::INSTRUMENT_WITNESS; }
bool CgNode::isInstrumentedConjunction() { return state == CgNodeState::INSTRUMENT_CONJUNCTION; }

bool CgNode::isUnwound() {
	return state == CgNodeState::UNWIND_SAMPLE || state == CgNodeState::UNWIND_INSTR;
}
bool CgNode::isUnwoundSample() {
	return state == CgNodeState::UNWIND_SAMPLE;
}
bool CgNode::isUnwoundInstr() {
	return state == CgNodeState::UNWIND_INSTR;
}

int CgNode::getNumberOfUnwindSteps() { return numberOfUnwindSteps; }

unsigned long long CgNode::getNumberOfCalls() { return numberOfCalls; }

unsigned long long CgNode::getNumberOfCallsWithCurrentEdges() {

  unsigned long long numberOfCalls = 0;
  for (auto n : numberOfCallsBy) {
    numberOfCalls += n.second;
  }

  return numberOfCalls;
}

unsigned long long CgNode::getNumberOfCalls(CgNodePtr parentNode) {
  return numberOfCallsBy[parentNode];
}

double CgNode::getRuntimeInSeconds() { return runtimeInSeconds; }

void CgNode::setRuntimeInSeconds(double newRuntimeInSeconds) {
  runtimeInSeconds = newRuntimeInSeconds;
}

unsigned long long CgNode::getExpectedNumberOfSamples() {
  return expectedNumberOfSamples;
}

void CgNode::setNumberOfStatements(int numStmts){
	numberOfStatements = numStmts;
}

int CgNode::getNumberOfStatements(){
	return numberOfStatements;
}
void CgNode::printMinimal() { std::cout << this->functionName; }

void CgNode::print() {
  std::cout << this->functionName << std::endl;
  for (auto n : childNodes) {
    std::cout << "--" << *n << std::endl;
  }
}

void CgNode::setDominance(CgNodePtr child, double dominance) {
  dominanceMap[child] = dominance;
}

double CgNode::getDominance(CgNodePtr child) { return dominanceMap[child]; }

void CgNode::setFilename(std::string filename) { this->filename = filename; }

void CgNode::setLineNumber(int line) { this->line = line; }

std::ostream &operator<<(std::ostream &stream, const CgNode &n) {
  stream << "\"" << n.getFunctionName() << "\"";

  return stream;
}
