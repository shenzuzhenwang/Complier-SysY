#ifndef COMPILER_SYNTAX_ANALYZE_H
#define COMPILER_SYNTAX_ANALYZE_H

#include "syntax_tree.h"
#include <memory>
#include <unordered_map>

extern shared_ptr<CompUnitNode> syntaxAnalyze();

extern unordered_map<string, string> usageNameListOfVarSingleUseInUnRecursionFunction;

#endif