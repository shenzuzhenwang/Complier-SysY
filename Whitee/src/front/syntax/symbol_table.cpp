/*********************************************************************
 * @file   symbol_table.cpp
 * @brief  符号表的定义
 * 
 * @author 神祖
 * @date   May 2022
 *********************************************************************/
#include "symbol_table.h"

#include <memory>
#include <utility>
#include <iostream>

 /**
  * @brief 用于按名字依次向外查找某个变量
  * @details 通过起始块的id来查找符号，递归查找到最上面的块。
  * 应该区分函数和变量，因为它们在一个块中可能有相同的名字。
  * @param startBlockId: 寻找起始块的ID，在递归过程中改变这个函数。
  * @param symbolName: 查找符号的名称。
  * @return SymbolTableItem 找到的符号
  */
shared_ptr<SymbolTableItem> findSymbol(pair<int, int> startBlockId, string &symbolName, bool isF)
{
    string findName = (isF ? "F*" : "V*") + symbolName;
    if (symbolTable[startBlockId]->symbolTableThisBlock.find(findName) != symbolTable[startBlockId]->symbolTableThisBlock.end())
    {
        return symbolTable[startBlockId]->symbolTableThisBlock[findName];
    }
    else
    {
        while (!symbolTable[startBlockId]->isTop)
        {
            startBlockId = symbolTable[startBlockId]->fatherBlockId;
            if (symbolTable[startBlockId]->symbolTableThisBlock.find(findName) != symbolTable[startBlockId]->symbolTableThisBlock.end())
            {
                return symbolTable[startBlockId]->symbolTableThisBlock[findName];
            }
        }
    }
    cerr << "findSymbol: Source code may have errors." << endl;
    return nullptr;
} 

/**
 * @brief 根据block的ID来将SymbolTableItem加入symbolTable
 * @param blockId 此block的ID
 * @param symbol 需要插入的symbol
 */
void insertSymbol(pair<int, int> blockId, shared_ptr<SymbolTableItem> symbol)
{
    symbolTable[blockId]->symbolTableThisBlock[symbol->uniqueName] = move(symbol);
}

/**
 * @brief 将一个新的Block加入symbolTable
 * @param blockId 此block的ID
 * @param fatherBlockId 此Block的上一层块ID
 */
void insertBlock(pair<int, int> blockId, pair<int, int> fatherBlockId)
{
    bool isTop = (blockId.first == 0);
    symbolTable[blockId] = make_shared<SymbolTablePerBlock>(blockId, isTop, fatherBlockId);
}

/**
 * @brief 分配一个块ID
 * @param layerId 此块的层数
 * @param fatherBlockId 此Block的上一层块ID
 * @return 此块的ID
 */
pair<int, int> distributeBlockId(int layerId, pair<int, int> fatherBlockId)
{
    if (blockLayerId2LayerNum.find(layerId) == blockLayerId2LayerNum.end())
    {
        blockLayerId2LayerNum[layerId] = 0;
    }
    pair<int, int> blockId = {layerId, ++blockLayerId2LayerNum[layerId]};
    insertBlock(blockId, fatherBlockId);
    return blockId;
}
