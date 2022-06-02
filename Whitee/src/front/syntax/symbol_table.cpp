/*********************************************************************
 * @file   symbol_table.cpp
 * @brief  ���ű�Ķ���
 * 
 * @author ����
 * @date   May 2022
 *********************************************************************/
#include "symbol_table.h"

#include <memory>
#include <utility>
#include <iostream>

 /**
  * @brief ���ڰ����������������ĳ������
  * @details ͨ����ʼ���id�����ҷ��ţ��ݹ���ҵ�������Ŀ顣
  * Ӧ�����ֺ����ͱ�������Ϊ������һ�����п�������ͬ�����֡�
  * @param startBlockId: Ѱ����ʼ���ID���ڵݹ�����иı����������
  * @param symbolName: ���ҷ��ŵ����ơ�
  * @return SymbolTableItem �ҵ��ķ���
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
 * @brief ����block��ID����SymbolTableItem����symbolTable
 * @param blockId ��block��ID
 * @param symbol ��Ҫ�����symbol
 */
void insertSymbol(pair<int, int> blockId, shared_ptr<SymbolTableItem> symbol)
{
    symbolTable[blockId]->symbolTableThisBlock[symbol->uniqueName] = move(symbol);
}

/**
 * @brief ��һ���µ�Block����symbolTable
 * @param blockId ��block��ID
 * @param fatherBlockId ��Block����һ���ID
 */
void insertBlock(pair<int, int> blockId, pair<int, int> fatherBlockId)
{
    bool isTop = (blockId.first == 0);
    symbolTable[blockId] = make_shared<SymbolTablePerBlock>(blockId, isTop, fatherBlockId);
}

/**
 * @brief ����һ����ID
 * @param layerId �˿�Ĳ���
 * @param fatherBlockId ��Block����һ���ID
 * @return �˿��ID
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
