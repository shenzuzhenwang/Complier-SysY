#ifndef COMPILER_SYMBOL_TABLE_H
#define COMPILER_SYMBOL_TABLE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include "../../basic/hash/pair_hash.h"
#include "syntax_tree.h"

/**
 * @brief 某一特定区块的符号表。
 * 包括一个fatherBlockId来查找父块。
 * 当寻找某个符号，并且该符号在这个块中没有找到时。
 * if isTop == false. 试图在其父块中找到这个符号。
 * if isTop == true, fatherBlockId为{0, 0}。
 */
class SymbolTablePerBlock
{
public:
    pair<int, int> blockId;
    bool isTop;                        // blockId.first == 0
    pair<int, int> fatherBlockId;  // 上一层块的ID
    unordered_map<string, shared_ptr<SymbolTableItem>> symbolTableThisBlock; // 名字<-->对象

    SymbolTablePerBlock(pair<int, int> &blockId, bool &isTop, pair<int, int> &fatherBlockId)
        : blockId(blockId), isTop(isTop), fatherBlockId(fatherBlockId){};
};

extern unordered_map<pair<int, int>, shared_ptr<SymbolTablePerBlock>, pair_hash> symbolTable;  // 块ID<-->块内对象

extern shared_ptr<SymbolTableItem> findSymbol(pair<int, int> startBlockId, string &symbolName, bool isF);

extern void insertSymbol(pair<int, int> blockId, shared_ptr<SymbolTableItem> symbol);

extern unordered_map<int, int> blockLayerId2LayerNum;

extern pair<int, int> distributeBlockId(int layerId, pair<int, int> fatherBlockId);

#endif
