# Whitee

[![readme.zh-CN](https://img.shields.io/badge/readme-中文-g.svg)](README.zh-CN.md) [![license](https://img.shields.io/badge/license-GPL--3.0-red.svg)](LICENSE) [![language](https://img.shields.io/badge/language-C++-f34b7d.svg)](https://www.cplusplus.com/) [![source](https://img.shields.io/badge/source_language-SysY-yellow.svg)](https://gitlab.eduxiji.net/nscscc/compiler2021/-/blob/master/SysY%E8%AF%AD%E8%A8%80%E5%AE%9A%E4%B9%89.pdf) [![assembly](https://img.shields.io/badge/target_assembly-ARM--v7a-blue.svg)](https://developer.arm.com/) [![platform](https://img.shields.io/badge/platform-Linux_|_Windows-lightgrey.svg)](https://github.com/Forever518/Whitee)

主要参考：[*北航 早安！白给人*](https://github.com/Forever518/Whitee)and *《Efficient Construction of Static Single Assignment Form》*

### Log

1. 词法分析与语法分析（自制）
2. SSA生成IR，常量传播、复制传播优化
3. IR优化，死代码删除、常量折叠、局部数组传播、常量数组全局化
4. 基于图着色的寄存器分配
5. 生成汇编指令（仅使用部分常用指令），部分借鉴，有些地方不太明白
6. 窥孔优化（全部借鉴），性能未测试

使用架构armv7ve，为了可以使用SDIV除法
