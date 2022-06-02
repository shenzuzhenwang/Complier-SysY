#ifndef COMPILER_SYMBOL_TABLE_H
#define COMPILER_SYMBOL_TABLE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include "../../basic/hash/pair_hash.h"
#include "syntax_tree.h"

/**
 * @brief ĳһ�ض�����ķ��ű�
 * ����һ��fatherBlockId�����Ҹ��顣
 * ��Ѱ��ĳ�����ţ����Ҹ÷������������û���ҵ�ʱ��
 * if isTop == false. ��ͼ���丸�����ҵ�������š�
 * if isTop == true, fatherBlockIdΪ{0, 0}��
 */
class SymbolTablePerBlock
{
public:
    pair<int, int> blockId;
    bool isTop;                        // blockId.first == 0
    pair<int, int> fatherBlockId;  // ��һ����ID
    unordered_map<string, shared_ptr<SymbolTableItem>> symbolTableThisBlock; // ����<-->����

    SymbolTablePerBlock(pair<int, int> &blockId, bool &isTop, pair<int, int> &fatherBlockId)
        : blockId(blockId), isTop(isTop), fatherBlockId(fatherBlockId){};
};

extern unordered_map<pair<int, int>, shared_ptr<SymbolTablePerBlock>, pair_hash> symbolTable;  // ��ID<-->���ڶ���

extern shared_ptr<SymbolTableItem> findSymbol(pair<int, int> startBlockId, string &symbolName, bool isF);

extern void insertSymbol(pair<int, int> blockId, shared_ptr<SymbolTableItem> symbol);

extern unordered_map<int, int> blockLayerId2LayerNum;

extern pair<int, int> distributeBlockId(int layerId, pair<int, int> fatherBlockId);

#endif
