#include "TaintAnalysis/Program/DefUseFunction.h"
#include "Util/IO/PointerAnalysis/Printer.h"
#include "Util/IO/TaintAnalysis/Printer.h"
#include "Util/IO/TaintAnalysis/WriteDotFile.h"

#include <llvm/IR/Function.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/ToolOutputFile.h>

#include <unordered_set>

using namespace llvm;
using namespace taint;
using namespace tpa;

namespace util
{
namespace io
{

/**
 * Writes a DOT graph representation of a DefUseFunction to an output stream.
 * This function generates a visualization of the def-use graph, showing both
 * top-level (dotted) and memory-level edges between instructions.
 *
 * @param os Output stream to write the DOT graph to
 * @param duFunc The DefUseFunction to visualize
 */
void writeDefUseFunc(raw_ostream& os, const DefUseFunction& duFunc)
{
	os << "digraph \"" << "DefUseGraph for function " << duFunc.getFunction().getName() << "\" {\n";

	using MemSet = std::unordered_set<const MemoryObject*>;

	auto dumpInst = [&os] (const DefUseInstruction* duInst)
	{
		os << "\tNode" << duInst << " [shape=record, label=\"" << *duInst << "\"]\n";
		
		for (auto succ: duInst->top_succs())
			os << "\tNode" << duInst << " -> " << "Node" << succ << " [style=dotted]\n";

		std::unordered_map<const DefUseInstruction*, MemSet> succMap;
		for (auto const& mapping: duInst->mem_succs())
			for (auto succ: mapping.second)
				succMap[succ].insert(mapping.first);

		for (auto const& mapping: succMap)
		{
			os << "\tNode" << duInst << " -> " << "Node" << mapping.first << " [label=\"";
			for (auto loc: mapping.second)
				os << *loc << "\\n";
			os << "\"]\n";
		}
	};

	dumpInst(duFunc.getEntryInst());
	for (auto const& bb: duFunc.getFunction())
	{
		for (auto& inst: bb)
		{
			auto duInst = duFunc.getDefUseInstruction(&inst);
			if (duInst != nullptr)
				dumpInst(duInst);
		}
	}

	os << "}\n";
}

/**
 * Writes a DOT graph representation of a DefUseFunction to a file.
 * This is a convenience wrapper around writeDefUseFunc that handles
 * opening and closing the output file.
 *
 * @param filePath Path to the output file
 * @param duFunc The DefUseFunction to visualize
 */
void writeDotFile(const char* filePath, const DefUseFunction& duFunc)
{
	std::error_code ec;
	tool_output_file out(filePath, ec, sys::fs::F_None);
	if (ec)
	{
		errs() << "Failed to open file " << filePath << ": " << ec.message() << "\n";
		return;
	}

	writeDefUseFunc(out.os(), duFunc);

	out.keep();
}

}
}