/*******************************************************************
 * @file   main.cpp
 * @brief  ���е���ڣ����е���Ҫ����  
 *         
 * 
 * @author ����
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

OptimizeLevel optimizeLevel = OptimizeLevel::O0;  // �����Ż��ȼ�
bool needIrCheck = false;  // ��ʼIR�������Ż���IR���
bool needIrPassCheck = true;  // ÿһ���Ż��󶼽��м��

string sourceCodeFile;  // Դ����·��
string targetCodeFile;  // Ŀ�����·�� 
string debugMessageDirectory;  // debug��Ϣ·��

void printHelp(const char *exec);

int setCompileOptions(int argc, char **argv);

int initConfig();

/**
 * @brief setCompileOptions -> initConfig -> lexicalAnalyze -> syntaxAnalyze -> buildIrModule -> optimizeIr -> buildMachineModule
 *        ���ñ���ѡ�� -> �������� -> �ʷ����� -> �﷨���� -> �����м�IR -> �Ż�IR -> ����������
 * @param argc  �����в����ĸ���
 * @param argv  �����в���  ��һ��Ϊִ���ļ���·��
 * @return ���򷵻�ֵ
 */
int main(int argc, char **argv)
{
    int r;

    if ((r = setCompileOptions(argc, argv)) != 0)
        return r;

    if (sourceCodeFile.empty() || targetCodeFile.empty())
    {
        printHelp(argv[0]);
        return _SCO_ST_ERR;
    }

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
            phiElimination(func);
            getFunctionRequiredStackSize(func);
        }
        if (_debugIr)
        {
            ofstream irStream;
            irStream.open(debugMessageDirectory + "ir_final.txt", ios::out | ios::trunc);
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
 * @brief ͨ����ȡ�����еĲ��������Խ��Ķ�����������
 * @param argc  �����в����ĸ���
 * @param argv  �����в���  ��һ��Ϊִ���ļ���·��
 * @return _SCO_ARG_ERR ������ʽ����ȷ��ִֹͣ�У�����ӡhelp˵����_SCO_HELP ��ӡhelp��_SCO_DBG_ERR debug��������_SCO_OP_ERR optimize��������
 *         _SCO_CHK_ERR check��������_SCO_DBG_PATH_ERR debug path����ȷ��_SCO_SUCCESS �ɹ�
 */
int setCompileOptions(int argc, char **argv)
{
    bool argOptimizeFlag = false; 
    bool argSourceFlag = false;
    bool argOutputFlag = false;
    bool argDebugDirectoryFlag = false;

    if (argc < 1)
    {
        printHelp(argv[0]);
        return _SCO_ARG_ERR;
    }

    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] == "-h"s || argv[i] == "--help"s)  // -h
        {
            printHelp(argv[0]);
            return _SCO_HELP;
        }
        else if (argv[i] == "-S"s && !argSourceFlag)  // -s
        {
            argSourceFlag = true;
        }
        else if (argv[i] == "-o"s && !argOutputFlag)  // -o
        {
            argOutputFlag = true;
        }
        else if (!argOptimizeFlag && (string(argv[i]).find("-O") == 0))
        {
            argOptimizeFlag = true;
            argv[i] += 2;
            if (*argv[i] == '\0')
            {
                if (i + 1 == argc)
                {
                    printHelp(argv[0]);
                    return _SCO_OP_ERR;
                }
                ++i;
            }
            if (argv[i] == "1"s)
                optimizeLevel = OptimizeLevel::O1;
            else if (argv[i] == "2"s)
                optimizeLevel = OptimizeLevel::O2;
            if (optimizeLevel >= OptimizeLevel::O2)
            {
                _optimizeMachineIr = true;
            }
        }
        else if (!argDebugDirectoryFlag && string(argv[i]).find("--set-debug-path=") == 0)
        {
            argDebugDirectoryFlag = true;
            argv[i] += 17;
            if (*argv[i] == '\0')
            {
                printHelp(argv[0]);
                return _SCO_DBG_PATH_ERR;
            }
            debugMessageDirectory = argv[i];
        }
        else
        {
            if (targetCodeFile.empty())
                targetCodeFile = argv[i];
            else if (sourceCodeFile.empty())
                sourceCodeFile = argv[i];
            else
            {
                printHelp(argv[0]);
                return _SCO_ARG_ERR;
            }
        }
    }
    return _SCO_SUCCESS;
}

/**
 * @brief ��ӡhelp˵������Ϊ���������Ϲ���
 * @param exec ִ���ļ���·��
 */
void printHelp(const char *exec)
{
    cout << "Usage: " + string(exec) + " [-S] [-o] [-h | --help] [-d | --debug <level>]";
    cout << endl
         << " [-c | --check <level>] [--set-debug-path=<path>]"
         << " <target-file> <source-file> [-O <level>]" << endl;

    cout << endl;
    cout << "    -S                  "
         << "generate assembly, can be omitted" << endl;
    cout << "    -o                  "
         << "set output file, can be omitted" << endl;
    cout << "    -h, --help          "
         << "show usage" << endl;
    cout << "    -d, --debug <level> "
         << "dump debug messages to certain files" << endl;
    cout << "                        "
         << "level 1: dump IR and optimized IR" << endl;
    cout << "                        "
         << "level 2: append AST" << endl;
    cout << "                        "
         << "level 3: append Lexer, each Optimization Pass "
            "and Register Allocation"
         << endl;
    cout << "    -c, --check <level> "
         << "check IR's relation" << endl;
    cout << "                        "
         << "level 1: check IR and final optimized IR only" << endl;
    cout << "                        "
         << "level 2: check IR after each optimization pass" << endl;
    cout << "    --set-debug-path=<path>" << endl;
    cout << "                        "
         << "set debug messages output path,"
            " default the same path with target assembly file"
         << endl;
    cout << "    <target-file>       "
         << "target assembly file in ARM-v7a" << endl;
    cout << "    <source-file>       "
         << "source code file matching SysY grammar" << endl;
    cout << "    -O <level>          "
         << "set optimization level, default non-optimization -O0" << endl;
    cout << endl;
}

/**
 * @brief ����һ���ļ���
 * @param path �ļ��е�·��
 * @return true �ɹ���false ʧ��
 */
bool createFolder(const char *path)
{
    if (_access(path, F_OK) != -1) // ���д��ļ���
        return true;
    if (strlen(path) > FILENAME_MAX)  // �ļ�·������
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
            system(command.c_str());  // ϵͳִ�д����ļ�������
        }
    }
    return true;
}

/**
 * @brief ���ļ���·����ͳһΪ��׼��ʽ�����ݲ�ͬ��ƽ̨����������Ӧ�������ļ���
 * @return _INIT_CRT_ERR �����ļ���ʧ�ܣ�_INIT_SUCCESS �ɹ�
 */
int initConfig()
{
    if (_debugIr)
    {
        //���ļ�Ŀ¼��'\'��Ϊ'\\'
        while (debugMessageDirectory.find(_O_SLASH_CHAR) != string::npos)
        {
            debugMessageDirectory.replace(debugMessageDirectory.find(_O_SLASH_CHAR), 1, _SLASH_STRING);
        }
        while (targetCodeFile.find(_O_SLASH_CHAR) != string::npos)
        {
            targetCodeFile.replace(targetCodeFile.find(_O_SLASH_CHAR), 1, _SLASH_STRING);
        }
    }
    if (_debugIr && debugMessageDirectory.empty())
    {
        string targetPath = string(targetCodeFile);
        string dirPath;
        if (targetPath.find_last_of(_SLASH_CHAR) != string::npos)
        {
            dirPath = targetPath.substr(0, targetPath.find_last_of(_SLASH_CHAR) + 1);
            targetPath = targetPath.substr(targetPath.find_last_of(_SLASH_CHAR) + 1, targetPath.size());
        }
        debugMessageDirectory = dirPath + "whitee-debug-" + targetPath + _SLASH_STRING;
    }
    if (_debugIr && debugMessageDirectory.at(debugMessageDirectory.size() - 1) != _SLASH_CHAR)
    {
        debugMessageDirectory += _SLASH_STRING;
    }
    if (_debugIr && !createFolder(debugMessageDirectory.c_str()))
    {
        return _INIT_CRT_ERR;
    }
    if (optimizeLevel > O0 && _debugIrOptimize && !createFolder((debugMessageDirectory + "optimize" + _SLASH_STRING).c_str()))
    {
        return _INIT_CRT_ERR;
    }
    return _INIT_SUCCESS;
}
