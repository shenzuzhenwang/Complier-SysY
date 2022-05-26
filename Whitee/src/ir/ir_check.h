#ifndef COMPILER_IR_CHECK_H
#define COMPILER_IR_CHECK_H

#include "ir.h"
#include "ir_utils.h"

/**
 * @return true if IR has no bugs.
 */
extern bool irCheck(const shared_ptr<Module> &);

extern bool globalIrCorrect;  // 全局IR都正确
extern bool irUserCheck;      // 检查是否有使用指令

extern bool globalWarningPermit;  // 启用警告

#endif
