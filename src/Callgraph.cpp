#include "Callgraph.h"

#include <chrono>	// easy time measurement

#define VERBOSE 0
#define DEBUG 0

#define BENCHMARK_PHASES 0
#define PRINT_FINAL_DOT 1
#define PRINT_DOT_AFTER_EVERY_PHASE 1

CgNodePtr Callgraph::findMain() {
	if( findNode("main") ) {
		return findNode("main");
	} else if( findNode("_Z4main")) {
		return findNode("_Z4main");
	} else {
		// simply search a method containing "main" somewhere
		for (auto node : *this) {
			auto fName = node->getFunctionName();
			if (fName.find("main") != fName.npos) {
				return node;
			}
		}
		return nullptr;
	}
}

CgNodePtr Callgraph::findNode(std::string functionName) {
	for (auto node : *this ) {

		auto fName = node->getFunctionName();
		if (fName == functionName) {
			return node;
		}
	}
	return nullptr;
}

void Callgraph::insert(CgNodePtr node) {
	graph.insert(node);
}

void Callgraph::eraseInstrumentedNode(CgNodePtr node) {
	// TODO implement
	// since the node is instrumented, all paths till the next conjunction are also instrumented
	// but only if there is exactly one unique path from the instrumented note to the conjunction
}

void Callgraph::erase(CgNodePtr node, bool rewireAfterDeletion, bool force) {
	if (!force) {
		if (CgHelper::isConjunction(node) && node->getChildNodes().size() > 1) {
			std::cerr << "Error: Cannot remove node with multiple parents AND multiple children." << std::endl;
			exit(EXIT_FAILURE);
		}
		if (CgHelper::isConjunction(node) && node->isLeafNode()) {
			std::cerr << "Error: Cannot remove conjunction node that is a leaf." << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	for (auto parent : node->getParentNodes()) {
		parent->removeChildNode(node);
	}
	for (auto child : node->getChildNodes()) {
		child->removeParentNode(node);
	}

	// a conjunction can only be erased if it has exactly one child
	if (!force && CgHelper::isConjunction(node)) {
		auto child = node->getUniqueChild();
		child->getMarkerPositions() = node->getMarkerPositions();

		for (auto markerPosition : node->getMarkerPositions()) {
			markerPosition->getDependentConjunctions().erase(node);
			markerPosition->getDependentConjunctions().insert(child);
		}
		// XXX erasing a node with multiple parents invalidates edge weights (they are visible in the dot output)
	}
	for (auto dependentConjunction : node->getDependentConjunctions()) {
		dependentConjunction->getMarkerPositions().erase(node);
	}

	if (rewireAfterDeletion) {
		for (auto parent : node->getParentNodes()) {
			for (auto child : node->getChildNodes()) {
				parent->addChildNode(child);
				child->addParentNode(parent);
			}
		}
	}

	graph.erase(node);

//	std::cout << "  Erasing node: " << *node << std::endl;
//	std::cout << "  UseCount: " << node.use_count() << std::endl;
}

CgNodePtrSet::iterator Callgraph::begin() {
	return graph.begin();
}
CgNodePtrSet::iterator Callgraph::end() {
	return graph.end();
}

CgNodePtrSet::const_iterator Callgraph::begin() const {
	return graph.cbegin();
}
CgNodePtrSet::const_iterator Callgraph::end() const {
	return graph.cend();
}

size_t Callgraph::size() {
	return graph.size();
}

CgNodePtrSet Callgraph::getGraph(){
	return graph;
}