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
    std::pair<int, int> blockId;
    bool isTop; // blockId.first == 0
    std::pair<int, int> fatherBlockId;
    std::unordered_map<std::string, std::shared_ptr<SymbolTableItem>> symbolTableThisBlock; // ����<-->����

    SymbolTablePerBlock(std::pair<int, int> &blockId, bool &isTop, std::pair<int, int> &fatherBlockId)
        : blockId(blockId), isTop(isTop), fatherBlockId(fatherBlockId){};
};

extern std::unordered_map<std::pair<int, int>, std::shared_ptr<SymbolTablePerBlock>, pair_hash> symbolTable;  // ��ID<-->���ڶ���

/**
 * @brief ���ڰ����������������ĳ������
 * @details ͨ����ʼ���id�����ҷ��ţ��ݹ���ҵ�������Ŀ顣
 * Ӧ�����ֺ����ͱ�������Ϊ������һ�����п�������ͬ�����֡�
 * @param startBlockId: Ѱ����ʼ���ID���ڵݹ�����иı����������
 * @param symbolName: ���ҷ��ŵ����ơ�
 * @return SymbolTableItem �ҵ��ķ���
 */
extern std::shared_ptr<SymbolTableItem> findSymbol(std::pair<int, int> startBlockId, std::string &symbolName, bool isF);

// // ����block��ID����SymbolTableItem����symbolTable
extern void insertSymbol(std::pair<int, int> blockId, std::shared_ptr<SymbolTableItem> symbol);

// ��¼ÿһ��ı��
// <2, ...> if ...max is 2, record <2, 2>, next layer2 block id is <2, 3>
extern std::unordered_map<int, int> blockLayerId2LayerNum;

/**
 * @brief ����blockId����symbolTablePerBlock����symbolTable�С�
 */
extern std::pair<int, int> distributeBlockId(int layerId, std::pair<int, int> fatherBlockId);

#endif
