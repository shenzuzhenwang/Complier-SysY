#include "compile_std.h"

bool _debugSyntax = true;  // ����﷨
bool _debugLexer = true;  // ����ʷ�
bool _debugIr = true;  // ���IR
bool _debugAst = true;  // ���AST
bool _debugIrOptimize = true;  // IR�Ż�
bool _debugMachineIr = true;  // ����IR

bool _optimizeMachineIr = false;  // ����IR�Ż� O2

bool _isBuildingIr = true; // Used for IR Phi.