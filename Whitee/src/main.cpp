/*******************************************************************
 * @file   main.cpp
 * @brief  运行的入口，运行的主要流程  
 *         https://buaa-se-compiling.github.io/miniSysY-tutorial/pre/design_hints.html
 *          https://www.llvm.org/docs/ProgrammersManual.html#iterating-over-the-basicblock-in-a-function
 * @author 神祖
 * @date   May 2022
 *********************************************************************/

#include <iostream>
#include <fstream>
#include <cstring>
#include <io.h> 
#include <process.h> 

#define R_OK 4 /* Test for read permission. */
#define W_OK 2 /* Test for write permission. */
#define F_OK 0 /* Test for existence. */

#include "front/lexer/lexer.h"
#include "front/syntax/syntax_analyze.h"
#include "ir/ir_build.h"
#include "ir/ir_check.h"
#include "optimize/ir/ir_optimize.h"
#include "machine_ir/machine_ir_build.h"
using namespace std;

OptimizeLevel optimizeLevel = OptimizeLevel::O1;  // 代码优化等级
bool needIrCheck = true;  // 初始IR和最终优化后IR检查
bool needIrPassCheck = false;  // 每一遍优化后都进行检查

string sourceCodeFile = "D:\\complier_test\\function\\101_insert_order.sy";  // 源程序路径
string targetCodeFile;  // 目标程序路径 
string debugMessageDirectory;  // debug信息路径

int initConfig();

/**
 * @brief setCompileOptions -> initConfig -> lexicalAnalyze -> syntaxAnalyze -> buildIrModule -> optimizeIr -> buildMachineModule
 *        设置编译选项 -> 编译设置 -> 词法分析 -> 语法分析 -> 构建中间IR -> 优化IR -> 构建机器码
 * @param argc  命令行参数的个数
 * @param argv  命令行参数  第一个为执行文件的路径
 * @return 程序返回值
 */
int main(int argc, char **argv)
{
    int r;

    if ((r = initConfig()) != 0)
        return r;

    if (!lexicalAnalyze(sourceCodeFile))
    {
        cout << "Error: Cannot parse source code file." << endl;
        return _LEX_ERR;
    }

    cout << "[AST]" << endl
         << "Start building AST..." << endl;
    auto root = syntaxAnalyze(); 
    cout << "AST built successfully." << endl;
    if (_debugAst)
    {
        ofstream astStream;
        astStream.open(debugMessageDirectory + "ast.txt", ios::out | ios::trunc);
        astStream << root->toString(0) << endl;
        astStream.close();
        cout << "AST written to ast.txt." << endl;
    }
    cout << endl;

    cout << "[IR]" << endl
         << "Start building IR in SSA form..." << endl;
    auto module = buildIrModule(root);
    cout << "IR built successfully." << endl;

    removePhiUserBlocksAndMultiCmp(module);

    if (_debugIr)
    {
        ofstream irStream;
        irStream.open(debugMessageDirectory + "ir.txt", ios::out | ios::trunc);
        irStream << module->toString() << endl;
        irStream.close();
        cout << "IR written to ir.txt." << endl;
    }
    cout << endl;

    if (needIrCheck && !irCheck(module))
    {
        cout << "Error: IR is not correct." << endl;
        return _IR_CHK_ERR;
    }

    _isBuildingIr = false;

    if (optimizeLevel != OptimizeLevel::O0)
    {
        cout << "[Optimize]" << endl
             << "Start IR optimizing..." << endl;
        optimizeIr(module, optimizeLevel);
        cout << "IR Optimized successfully." << endl;
        if (_debugIr)
        {
            cout << "Optimized IR written to ir_optimize.txt." << endl;
        }
        cout << endl;
    }
    else 
    {
        fixRightValue(module);
        for (auto &func : module->functions)
        {
            phi_elimination(func);
            getFunctionRequiredStackSize(func);
        }
        if (_debugIr)
        {
            ofstream irStream;
            irStream.open(debugMessageDirectory + "final_ir.txt", ios::out | ios::trunc);
            irStream << module->toString() << endl;
            irStream.close();
        }
    }

    cout << "[Machine IR]" << endl
         << "Start building Machine IR..." << endl;
    auto machineModule = buildMachineModule(module);
    cout << "Machine IR built successfully." << endl;
    cout << endl;

    cout << "[ARM]" << endl
         << "Start building ARM..." << endl;
    machineIrStream.open(targetCodeFile, ios::out | ios::trunc);
    machineModule->toARM();
    machineIrStream.close();
    cout << "ARM built successfully!" << endl;
    cout << endl;

    return 0;
}

/**
 * @brief 创建一个文件夹
 * @param path 文件夹的路径
 * @return true 成功；false 失败
 */
bool createFolder(const char *path)
{
    if (_access(path, F_OK) != -1) // 已有此文件夹
        return true;
    if (strlen(path) > FILENAME_MAX)  // 文件路径过长 
    {
        cout << "Error: debug path is too long." << endl;
        return false;
    }
    char tempPath[FILENAME_MAX] = {'\0'};
    for (int i = 0; i < strlen(path); ++i)
    {
        tempPath[i] = path[i];
        if ((tempPath[i] == _SLASH_CHAR) && _access(tempPath, F_OK) == -1)
        {
            string command = "mkdir " + string(tempPath);
            system(command.c_str());  // 系统执行创建文件夹命令
        }
    }
    return true;
}

/**
 * @brief 将文件的路径名统一为标准格式（根据不同的平台），并创建应创建的文件夹
 * @return _INIT_CRT_ERR 创建文件夹失败；_INIT_SUCCESS 成功
 */
int initConfig()
{
    if (targetCodeFile.empty ())
    {
        if (sourceCodeFile.find_last_of ('.') != string::npos)
        {
            targetCodeFile = sourceCodeFile.substr (0, sourceCodeFile.find_last_of ('.') + 1);
            targetCodeFile += 's';
        }
    }
    if (_debugIr)
    {
        //将文件目录的'/'换为'\\'
        while (debugMessageDirectory.find(_O_SLASH_CHAR) != string::npos)
        {
            debugMessageDirectory.replace(debugMessageDirectory.find(_O_SLASH_CHAR), 1, _SLASH_STRING);
        }
        while (targetCodeFile.find(_O_SLASH_CHAR) != string::npos)
        {
            targetCodeFile.replace(targetCodeFile.find(_O_SLASH_CHAR), 1, _SLASH_STRING);
        }
    }
    if (_debugIr && debugMessageDirectory.empty())  // 如果没有debug目录，则在dirPath + "whitee-debug-" + targetPath 路径创建目录
    {
        string targetPath = string(targetCodeFile);
        string dirPath;
        if (targetPath.find_last_of(_SLASH_CHAR) != string::npos)
        {
            dirPath = targetPath.substr(0, targetPath.find_last_of(_SLASH_CHAR) + 1);
            targetPath = targetPath.substr(targetPath.find_last_of(_SLASH_CHAR) + 1, targetPath.size());
        }
        debugMessageDirectory = dirPath + "-debug-" + targetPath + _SLASH_STRING;
    }
    if (_debugIr && debugMessageDirectory.at(debugMessageDirectory.size() - 1) != _SLASH_CHAR)
    {
        debugMessageDirectory += _SLASH_STRING;
    }
    if (_debugIr && !createFolder(debugMessageDirectory.c_str()))  // debug目录生成
    {
        return _INIT_CRT_ERR;
    }
    if (optimizeLevel > O0 && _debugIrOptimize && !createFolder((debugMessageDirectory + "optimize" + _SLASH_STRING).c_str())) // optimize目录生成
    {
        return _INIT_CRT_ERR;
    }
    return _INIT_SUCCESS;
}
