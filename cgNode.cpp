
#include "cgNode.h"

CgNode::CgNode(std::string function){
	this->functionName = function;
	this->parentNodes = std::set<std::shared_ptr<CgNode> >();
	this->childNodes = std::set<std::shared_ptr<CgNode> >();

	this->line = -1;
	this->needsInstrumentation = false;

	this->uniqueCallPath = false;
	this->leafNode = false;
}


void CgNode::addChildNode(std::shared_ptr<CgNode> childNode){

	childNodes.insert(childNode);
}


void CgNode::addParentNode(std::shared_ptr<CgNode> parentNode){
	for(auto node : parentNodes){
		if(node->isSameFunction(parentNode)) {
			return;
		}
	}
	parentNodes.insert(parentNode);

	updateUniqueCallPathAttribute();
}

void CgNode::updateUniqueCallPathAttribute() {

	auto parents = getParentNodes();
	while(parents.size()==1) {
		parents = (*parents.begin())->getParentNodes();
	}

	this->uniqueCallPath = (parents.size() == 0);

}

bool CgNode::hasUniqueCallPath() {
	return uniqueCallPath;
}

bool CgNode::isSameFunction(std::shared_ptr<CgNode> cgNodeToCompareTo){
	if(this->functionName.compare(cgNodeToCompareTo->getFunctionName()) == 0) {
		return true;
	}
	return false;
}


std::string CgNode::getFunctionName(){
	return this->functionName;
}


void CgNode::dumpToDot(std::ofstream& outStream){
	for(auto isCalledByNode : parentNodes){
		outStream << "\"" << isCalledByNode->getFunctionName() << "\" -> \"" << this->functionName
				<< "\" [label=" << this->getNumberOfCalls(isCalledByNode) <<"];" << std::endl;
	}
}

std::set<std::shared_ptr<CgNode> > CgNode::getChildNodes(){
	return childNodes;
}

std::set<std::shared_ptr<CgNode> > CgNode::getParentNodes(){
	return parentNodes;
}

void CgNode::addNumberOfCalls(int calls, std::shared_ptr<CgNode> parentNode) {
	this->numberOfCallsBy[parentNode] += calls;
}

void CgNode::setNeedsInstrumentation(bool needsInstrumentation){
	this->needsInstrumentation = needsInstrumentation;
}

bool CgNode::getNeedsInstrumentation(){
	return this->needsInstrumentation;
}

unsigned int CgNode::getNumberOfCalls(){

	unsigned int numberOfCalls = 0;
	for(auto n : numberOfCallsBy) {
		numberOfCalls += n.second;
	}

	return numberOfCalls;
}

unsigned int CgNode::getNumberOfCalls(std::shared_ptr<CgNode> parentNode) {
	return numberOfCallsBy[parentNode];
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
