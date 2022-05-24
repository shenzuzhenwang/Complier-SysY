#include "compile_std.h"

bool _debugSyntax = true;  // 输出语法
bool _debugLexer = false;  // 输出词法
bool _debugIr = false;  // 输出IR
bool _debugAst = false;  // 输出AST
bool _debugIrOptimize = false;  // IR优化
bool _debugMachineIr = false;  // 机器IR

bool _isBuildingIr = true; // Used for IR Phi.

bool _optimizeMachineIr = false;  // 机器IR优化
bool _optimizeDivAndMul = false;  // 乘除法优化
