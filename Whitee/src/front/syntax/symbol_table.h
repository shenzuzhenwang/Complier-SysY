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
    std::pair<int, int> blockId;
    bool isTop; // blockId.first == 0
    std::pair<int, int> fatherBlockId;
    std::unordered_map<std::string, std::shared_ptr<SymbolTableItem>> symbolTableThisBlock; // 名字<-->对象

    SymbolTablePerBlock(std::pair<int, int> &blockId, bool &isTop, std::pair<int, int> &fatherBlockId)
        : blockId(blockId), isTop(isTop), fatherBlockId(fatherBlockId){};
};

extern std::unordered_map<std::pair<int, int>, std::shared_ptr<SymbolTablePerBlock>, pair_hash> symbolTable;  // 块ID<-->块内对象

/**
 * @brief 用于按名字依次向外查找某个变量
 * @details 通过起始块的id来查找符号，递归查找到最上面的块。
 * 应该区分函数和变量，因为它们在一个块中可能有相同的名字。
 * @param startBlockId: 寻找起始块的ID，在递归过程中改变这个函数。
 * @param symbolName: 查找符号的名称。
 * @return SymbolTableItem 找到的符号
 */
extern std::shared_ptr<SymbolTableItem> findSymbol(std::pair<int, int> startBlockId, std::string &symbolName, bool isF);

// // 根据block的ID来将SymbolTableItem加入symbolTable
extern void insertSymbol(std::pair<int, int> blockId, std::shared_ptr<SymbolTableItem> symbol);

// 记录每一层的编号
// <2, ...> if ...max is 2, record <2, 2>, next layer2 block id is <2, 3>
extern std::unordered_map<int, int> blockLayerId2LayerNum;

/**
 * @brief 根据blockId并将symbolTablePerBlock插入symbolTable中。
 */
extern std::pair<int, int> distributeBlockId(int layerId, std::pair<int, int> fatherBlockId);

#endif
