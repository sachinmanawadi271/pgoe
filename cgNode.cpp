
#include "cgNode.h"

CgNode::CgNode(std::string function){
	this->functionName = function;
	this->numberOfCalls = 0;
	this->isCalledByNodes = std::vector<std::shared_ptr<CgNode> >();
	this->calledNodes = std::vector<std::shared_ptr<CgNode> >();

	this->uniqueParent = false;
}


void CgNode::addCallsNode(std::shared_ptr<CgNode> functionWhichIsCalledNode){

	for(auto node : calledNodes){
		if(node->isSameFunction(functionWhichIsCalledNode))
			return;
	}

	calledNodes.push_back(functionWhichIsCalledNode);
}


void CgNode::addIsCalledByNode(std::shared_ptr<CgNode> functionByWhichItIsCalledNode){
	for(auto node : isCalledByNodes){
		if(node->isSameFunction(functionByWhichItIsCalledNode))
			return;
	}
	isCalledByNodes.push_back(functionByWhichItIsCalledNode);

	updateUniqueParent();
}

void CgNode::updateUniqueParent() {
	uniqueParent = (getCallers().size() <= 1);
}

bool CgNode::hasUniqueParent() {
	return uniqueParent;
}

bool CgNode::isSameFunction(std::shared_ptr<CgNode> cgNodeToCompareTo){
	if(this->functionName.compare(cgNodeToCompareTo->getFunctionName()) == 0)
		return true;

	return false;
}


std::string CgNode::getFunctionName(){
	return this->functionName;
}


void CgNode::dumpToDot(std::ofstream& outStream){

	// XXX RN
	sanityCheck();

	for(auto calledNode : calledNodes){
		outStream << "\"" << this->functionName << "\" -> \"" << calledNode->getFunctionName() << "\";" << std::endl;
	}
}

std::vector<std::shared_ptr<CgNode> > CgNode::getCallers(){
	return isCalledByNodes;
}

std::vector<std::shared_ptr<CgNode> > CgNode::getCallees(){
	return calledNodes;
}
void CgNode::addNumberOfCalls(int calls){
	this->numberOfCalls += calls;
}

void CgNode::setNeedsInstrumentation(bool needsInstrumentation){
	this->needsInstrumentation = needsInstrumentation;
}

bool CgNode::getNeedsInstrumentation(){
	return this->needsInstrumentation;
}

unsigned int CgNode::getNumberOfCalls(){
	return this->numberOfCalls;
}

void CgNode::printMinimal(){
	std::cout << this->functionName <<  ",";
}

void CgNode::print(){
	std::cout << this->functionName << "\n";
	for(auto n : calledNodes){
		std::cout << "--" << n->getFunctionName() << "\n";
	}
}


void CgNode::setFilename(std::string filename){
	this->filename = filename;
}


void CgNode::setLineNumber(int line){
	this->line = line;
}
