#include "compile_std.h"

bool _debugSyntax = true;  // ����﷨
bool _debugLexer = false;  // ����ʷ�
bool _debugIr = false;  // ���IR
bool _debugAst = false;  // ���AST
bool _debugIrOptimize = false;  // IR�Ż�
bool _debugMachineIr = false;  // ����IR

bool _isBuildingIr = true; // Used for IR Phi.

bool _optimizeMachineIr = false;  // ����IR�Ż�
bool _optimizeDivAndMul = false;  // �˳����Ż�
