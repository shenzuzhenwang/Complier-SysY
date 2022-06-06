#include "compile_std.h"

bool _debugSyntax = true;  // 输出语法
bool _debugLexer = true;  // 输出词法
bool _debugIr = true;  // 输出IR
bool _debugAst = true;  // 输出AST
bool _debugIrOptimize = true;  // IR优化
bool _debugMachineIr = true;  // 机器IR

bool _optimizeMachineIr = false;  // 机器IR优化 O2

bool _isBuildingIr = true; // Used for IR Phi.