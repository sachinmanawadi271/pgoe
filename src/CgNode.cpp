
#include "CgNode.h"

CgNode::CgNode(std::string function){
	this->functionName = function;
	this->parentNodes = std::set<CgNodePtr>();
	this->childNodes = std::set<CgNodePtr>();

	this->spantreeParents = std::set<CgNodePtr>();

	this->line = -1;
	this->state = CgNodeState::NONE;

	this->runtimeInSeconds = 0.0;
	this->expectedNumberOfSamples = 0L;

	this->uniqueCallPath = false;
	this->leafNode = false;
}


void CgNode::addChildNode(CgNodePtr childNode){

	childNodes.insert(childNode);
}


void CgNode::addParentNode(CgNodePtr parentNode){
	for(auto node : parentNodes){
		if(node->isSameFunction(parentNode)) {
			return;
		}
	}
	parentNodes.insert(parentNode);
}

void CgNode::addSpantreeParent(CgNodePtr parentNode) {
	this->spantreeParents.insert(parentNode);
}

bool CgNode::isSpantreeParent(CgNodePtr parentNode) {
	return this->spantreeParents.find(parentNode) != spantreeParents.end();
}

void CgNode::updateNodeAttributes(int samplesPerSecond) {

	// is leaf node
	this->leafNode = (getChildNodes().size() == 0);

	// has unique call path
	auto parents = getParentNodes();
	while(parents.size()==1) {
		parents = (*parents.begin())->getParentNodes();
	}
	this->uniqueCallPath = (parents.size() == 0);

	// expected samples in this function
	this->expectedNumberOfSamples = samplesPerSecond * runtimeInSeconds;

}

bool CgNode::hasUniqueCallPath() {
	return uniqueCallPath;
}

bool CgNode::isLeafNode() {
	return leafNode;
}

bool CgNode::isRootNode() {
	return getParentNodes().empty();
}

bool CgNode::isSameFunction(CgNodePtr cgNodeToCompareTo){
	if(this->functionName.compare(cgNodeToCompareTo->getFunctionName()) == 0) {
		return true;
	}
	return false;
}


std::string CgNode::getFunctionName(){
	return this->functionName;
}


void CgNode::dumpToDot(std::ofstream& outStream){
	for(auto parentNode : parentNodes){

		std::string edgeColor = "";
		if(!isSpantreeParent(parentNode)) {
			edgeColor = ", color=red, fontcolor=red";
		}

		outStream << "\"" << parentNode->getFunctionName() << "\" -> \"" << this->functionName
				<< "\" [label=" << this->getNumberOfCalls(parentNode) << edgeColor << "];"
				<< std::endl;
	}
}

std::set<CgNodePtr> CgNode::getChildNodes(){
	return childNodes;
}

std::set<CgNodePtr> CgNode::getParentNodes(){
	return parentNodes;
}

void CgNode::addCallData(CgNodePtr parentNode,
		unsigned long long calls, double timeInSeconds) {

	this->numberOfCallsBy[parentNode] += calls;
	this->runtimeInSeconds += timeInSeconds;
}

void CgNode::setState(CgNodeState state){
	this->state = state;
}

bool CgNode::isInstrumented(){
	return state == CgNodeState::INSTRUMENT;
}

bool CgNode::isUnwound() {
	return state == CgNodeState::UNWIND;
}

unsigned long long CgNode::getNumberOfCalls(){

	unsigned long long numberOfCalls = 0;
	for(auto n : numberOfCallsBy) {
		numberOfCalls += n.second;
	}

	return numberOfCalls;
}

unsigned long long CgNode::getNumberOfCalls(CgNodePtr parentNode) {
	return numberOfCallsBy[parentNode];
}

double CgNode::getRuntimeInSeconds() {
	return runtimeInSeconds;
}

unsigned long long CgNode::getExpectedNumberOfSamples() {
	return expectedNumberOfSamples;
}

void CgNode::printMinimal(){
	std::cout << this->functionName;
}

void CgNode::print(){
	std::cout << this->functionName << std::endl;
	for(auto n : childNodes){
		std::cout << "--" << n->getFunctionName() << std::endl;
	}
}


void CgNode::setFilename(std::string filename){
	this->filename = filename;
}


void CgNode::setLineNumber(int line){
	this->line = line;
}
