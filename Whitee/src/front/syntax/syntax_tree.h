/*********************************************************************
 * @file   syntax_tree.h
 * @brief  根据不同的类型，定义了不同的类
 * 
 * @author 神祖
 * @date   May 2022
 *********************************************************************/
#ifndef COMPILER_SYNTAX_TREE_H
#define COMPILER_SYNTAX_TREE_H

#include <string>
#include <memory>
#include <utility>
#include <vector>
#include <unordered_map>

#include "../../basic/std/compile_std.h"

using namespace std;

/**
 * 语法树结点
 */
// Basic tree node class.
class SyntaxNode;

// Derived classes of SyntaxNode.
class CompUnitNode;

class DeclNode;

class ConstDeclNode;

class ConstDefNode;

class ConstInitValNode;

class VarDeclNode;

class VarDefNode;

class InitValNode;

class FuncDefNode;

class FuncFParamsNode;

class FuncFParamNode;

class BlockNode;

class BlockItemNode;

class StmtNode;

class ExpNode;

class CondNode;

class LValNode;

class PrimaryExpNode;

class UnaryExpNode;

class FuncRParamsNode;

class MulExpNode;

class AddExpNode;

class RelExpNode;

class EqExpNode;

class LAndExpNode;

class LOrExpNode;

class IdentNode;

// const数字初始值
class ConstInitValValNode;

// const数组初始值
class ConstInitValArrNode;

// 变量初始值
class InitValValNode;

// 数组变量初始值
class InitValArrNode;

// String node.
class StringNode;

enum SymbolType
{
    CONST_VAR,
    CONST_ARRAY,
    VAR,
    ARRAY,
    VOID_FUNC,
    RET_FUNC
};

/**
 * @brief 符号项目
 * 记录关于一个符号的详细信息。
 * usageName是用来区分在IR中可能有相同名称的符号。
 * 在<2, 1>块中的a(int, function)被重新命名为F*2_1$a。
 * <3, 2>块中的b(int, VAR)被重新命名为V*3_2$b。
 * uniqueName被设置为区分同一块中的F和V的关键。
 * <2, 1>块中的a(int, function)被重命名为F*a。
 * <3, 2>块中的b(int, VAR)被重命名为V*b。
 */
class SymbolTableItem
{
public:
    SymbolType symbolType;  // 符号类型

    int dimension;  // 数组维数
    vector<int> numOfEachDimension;  // 数组各维大小
    vector<shared_ptr<ExpNode>> expressionOfEachDimension;  // 数组各维大小（非常数）
    string name;
    string uniqueName;  // 用来区分在IR中可能有相同名称的符号。
    string usageName;   // 设置为区分同一块中的F和V的关键。
    pair<int, int> blockId;
    shared_ptr<ConstInitValNode> constInitVal;  // const初始化
    shared_ptr<InitValNode> initVal;  // 初始化
    shared_ptr<ConstInitValNode> globalVarInitVal;  // globalVarInitVal 对于所有全局变量的val是0或可计算。使用ConstInitValNode来计算它。
    bool isRecursion = false;    // 函数递归
    unordered_map<string, int> eachFuncUseNum;  // 此变量或函数被使用的次数
    vector<shared_ptr<SymbolTableItem>> eachFunc; // 此变量被使用的函数

    /**
     *
     * @brief Use for usage.
     * 使用时，括号内的所有数值[]肯定不是constExp。
     * 用Exp代替。
     */
    SymbolTableItem(SymbolType &symbolType, int &dimension, vector<shared_ptr<ExpNode>> &expressionOfEachDimension, string &name, pair<int, int> &blockId);

    /**
     * @brief 用来定义。
     * 当定义时，括号内的所有值[]是constExp。
     * 可以得到一个int值。
     * @note SymbolTable中的所有变量项都应该由这个构造。
     * 对于只定义一个变量或一个函数，需要将其记录到SymbolTable中。
     * 在DefineVar Node->SymbolTableItem中，应该使用numOfEachDimension。
     * 同时，在FuncFParam中的Id应该使用这个，现在假设所有的exp都是constExp。
     * 其他情况应该使用expressionOfEachDimension。
     */
    SymbolTableItem(SymbolType &symbolType, int &dimension, vector<int> &numOfEachDimension, string &name, pair<int, int> &blockId);

    /**
     * @brief 用于非数组
     */
    SymbolTableItem(SymbolType &symbolType, string &name, pair<int, int> &blockId);

    // 变量单次用于非递归函数
    bool isVarSingleUseInUnRecursionFunction();
};

/**
 * Definition of enums.
 */
enum FuncType
{
    FUNC_INT,  // int返回函数
    FUNC_VOID  // void返回函数
};

enum StmtType
{
    STMT_ASSIGN,
    STMT_EXP,
    STMT_BLOCK,
    STMT_IF,
    STMT_IF_ELSE,
    STMT_WHILE,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_RETURN,
    STMT_RETURN_VOID,
    STMT_EMPTY
};

enum UnaryExpType // UnaryExp -> PrimaryExp  |  IdentUsage '(' [FuncRParams] ')'  |  ('+' | '−' | '!') UnaryExp
{
    UNARY_PRIMARY,
    UNARY_FUNC,
    UNARY_FUNC_NON_PARAM,
    UNARY_DEEP
};

enum PrimaryExpType // PrimaryExp -> '(' Exp ')'  |  IntConst  |  LVal
{
    PRIMARY_PARENT_EXP,
    PRIMARY_L_VAL,
    PRIMARY_NUMBER,
    PRIMARY_STRING
};

enum InitType
{
    INIT,  // 初始化
    NON_INIT  // 未初始化
};

/**
 * 最底层基类，纯虚类
 */
class SyntaxNode
{
public:
    virtual ~SyntaxNode() = default;

    virtual string toString(int tabCnt) = 0;
};

class CompUnitNode : public SyntaxNode
{
public:
    vector<shared_ptr<DeclNode>> declList;  // 变量定义表
    vector<shared_ptr<FuncDefNode>> funcDefList;  // 函数定义表

    explicit CompUnitNode(vector<shared_ptr<DeclNode>> &declList, vector<shared_ptr<FuncDefNode>> &funcDefList)
        : declList(declList), funcDefList(funcDefList){};

    string toString(int tabCnt) override;
};

class BlockNode : public SyntaxNode
{
public:
    int itemCnt;  // block内blockItem数量
    vector<shared_ptr<BlockItemNode>> blockItems;  // blockItem表

    BlockNode(int itemCnt, vector<shared_ptr<BlockItemNode>> &blockItems) 
        : itemCnt(itemCnt), blockItems(blockItems){};

    BlockNode() : itemCnt(0){};

    string toString(int tabCnt) override;
};

class BlockItemNode : public SyntaxNode
{
public:
    string toString(int tabCnt) override = 0;
};

class DeclNode : public BlockItemNode
{
public:
    string toString(int tabCnt) override = 0;
};

class ConstDeclNode : public DeclNode
{
public:
    vector<shared_ptr<ConstDefNode>> constDefList;  // const变量定义表

    explicit ConstDeclNode(vector<shared_ptr<ConstDefNode>> &constDefList)
        : constDefList(constDefList){};

    string toString(int tabCnt) override;
};

class VarDeclNode : public DeclNode
{
public:
    vector<shared_ptr<VarDefNode>> varDefList;  // 变量定义表

    explicit VarDeclNode(vector<shared_ptr<VarDefNode>> &varDefList)
        : varDefList(varDefList){};

    string toString(int tabCnt) override;
};

class ConstDefNode : public SyntaxNode
{
public:
    shared_ptr<IdentNode> ident;  // 变量标识符
    shared_ptr<ConstInitValNode> constInitVal;  // 初始情况化
     
    ConstDefNode(shared_ptr<IdentNode> &ident, shared_ptr<ConstInitValNode> &constInitVal)
        : ident(ident), constInitVal(constInitVal){};

    string toString(int tabCnt) override;
};

class ConstInitValNode : public SyntaxNode
{
public:
    string toString(int tabCnt) override = 0;

    virtual vector<pair<int, int>> toOneDimensionArray(int start, int size) = 0;  // 将多维数组转换成一维
};

class ConstInitValValNode : public ConstInitValNode
{
public:
    int value;

    explicit ConstInitValValNode(int &val) : value(val){};

    string toString(int tabCnt) override;

    // 转换至一维数组
    vector<pair<int, int>> toOneDimensionArray(int start, int size) override;
};

class ConstInitValArrNode : public ConstInitValNode
{
public:
    int expectedSize;
    vector<shared_ptr<ConstInitValNode>> valList;

    explicit ConstInitValArrNode(int expectedSize, vector<shared_ptr<ConstInitValNode>> &valList)
        : expectedSize(expectedSize), valList(valList){};

    string toString(int tabCnt) override;

    // 转换至一维数组
    vector<pair<int, int>> toOneDimensionArray(int start, int size) override;
};

class VarDefNode : public SyntaxNode
{
public:
    shared_ptr<IdentNode> ident;  // 变量标识符
    InitType type;    // 初始化情况
    int dimension;    // 维数
    vector<int> dimensions;  // 各维大小
    shared_ptr<InitValNode> initVal;  // 初始化情况

    VarDefNode(shared_ptr<IdentNode> &ident, int dimension, vector<int> &dimensions, shared_ptr<InitValNode> &initVal)
        : ident(ident), type(InitType::INIT), dimension(dimension), dimensions(dimensions), initVal(initVal){};

    VarDefNode(shared_ptr<IdentNode> &ident, int dimension, vector<int> &dimensions)
        : ident(ident), type(InitType::NON_INIT), dimension(dimension), dimensions(dimensions){};

    VarDefNode(shared_ptr<IdentNode> &ident, shared_ptr<InitValNode> &initVal)
        : ident(ident), type(InitType::INIT), dimension(0), initVal(initVal){};

    explicit VarDefNode(shared_ptr<IdentNode> &ident)
        : ident(ident), type(InitType::NON_INIT), dimension(0){};

    string toString(int tabCnt) override;
};

class InitValNode : public SyntaxNode
{
public:
    string toString(int tabCnt) override = 0;

    virtual vector<pair<int, shared_ptr<ExpNode>>> toOneDimensionArray(int start, int size) = 0;// 将多维数组转换成一维
};

class InitValValNode : public InitValNode
{
public:
    shared_ptr<ExpNode> exp;  // 初始化时的表达式

    explicit InitValValNode(shared_ptr<ExpNode> &exp) 
        : exp(exp){};

    string toString(int tabCnt) override;

    vector<pair<int, shared_ptr<ExpNode>>> toOneDimensionArray(int start, int size) override; // 将多维数组转换成一维
};

class InitValArrNode : public InitValNode
{
public:
    int expectedSize;  // 数组总共大小
    vector<shared_ptr<InitValNode>> valList;  // 数组每个地方的元素值

    InitValArrNode(int expectedSize, vector<shared_ptr<InitValNode>> &valList)
        : expectedSize(expectedSize), valList(valList){};

    string toString(int tabCnt) override;

    vector<pair<int, shared_ptr<ExpNode>>> toOneDimensionArray(int start, int size) override; // 将多维数组转换成一维
};

class FuncDefNode : public SyntaxNode
{
public:
    FuncType funcType;  // 函数类型
    shared_ptr<IdentNode> ident;  // 标识符
    shared_ptr<FuncFParamsNode> funcFParams;  // 函数参数
    shared_ptr<BlockNode> block;  // 函数内部块

    FuncDefNode(FuncType funcType, shared_ptr<IdentNode> &ident, shared_ptr<FuncFParamsNode> &funcFParams, shared_ptr<BlockNode> &block)
        : funcType(funcType), ident(ident), funcFParams(funcFParams), block(block){};

    FuncDefNode(FuncType funcType, shared_ptr<IdentNode> &ident, shared_ptr<BlockNode> &block)
        : funcType(funcType), ident(ident), block(block){};

    string toString(int tabCnt) override;
};

class FuncFParamsNode : public SyntaxNode
{
public:
    vector<shared_ptr<FuncFParamNode>> funcParamList;  // 参数表

    explicit FuncFParamsNode(vector<shared_ptr<FuncFParamNode>> &funcParamList)
        : funcParamList(funcParamList){};

    string toString(int tabCnt) override;
};

class FuncFParamNode : public SyntaxNode
{
public:
    shared_ptr<IdentNode> ident;  // 标识符
    int dimension;  // 维数
    /*
     * NOTE:
     * 1. The first dimensions[0] can be any number.
     * 2. The rest dimensions must be a constant.
     */
    vector<int> dimensions;  // 各维大小

    FuncFParamNode(shared_ptr<IdentNode> &ident, int dimension, vector<int> &dimensions)
        : ident(ident), dimension(dimension), dimensions(dimensions){};

    explicit FuncFParamNode(shared_ptr<IdentNode> &ident) 
        : ident(ident), dimension(0){};

    string toString(int tabCnt) override;
};

class StmtNode : public BlockItemNode
{
public:
    /*
     * NOTE:
     * 1. 'break', 'continue' and 'empty' does not use any members.
     * 2. 'return' may use 0 member.
     * 3. Other type use at least 1 member.
     */
    StmtType type;  // 语句的类型
    shared_ptr<LValNode> lVal;
    shared_ptr<ExpNode> exp;
    shared_ptr<BlockNode> block;
    shared_ptr<CondNode> cond;
    shared_ptr<StmtNode> stmt;
    shared_ptr<StmtNode> elseStmt;

    StmtNode() 
        : type(StmtType::STMT_EMPTY){};

    static StmtNode assignStmt(shared_ptr<LValNode> &lVal, shared_ptr<ExpNode> &exp);

    static StmtNode emptyStmt();

    static StmtNode blockStmt(shared_ptr<BlockNode> &block);

    static StmtNode expStmt(shared_ptr<ExpNode> &exp);

    static StmtNode ifStmt(shared_ptr<CondNode> &cond, shared_ptr<StmtNode> &stmt, shared_ptr<StmtNode> &elseStmt);

    static StmtNode ifStmt(shared_ptr<CondNode> &cond, shared_ptr<StmtNode> &stmt);

    static StmtNode whileStmt(shared_ptr<CondNode> &cond, shared_ptr<StmtNode> &stmt);

    static StmtNode breakStmt();

    static StmtNode continueStmt();

    static StmtNode returnStmt(shared_ptr<ExpNode> &exp);

    static StmtNode returnStmt();

    string toString(int tabCnt) override;

private:
    StmtNode(StmtType type, shared_ptr<LValNode> &lVal, shared_ptr<ExpNode> &exp, shared_ptr<BlockNode> &block,
             shared_ptr<CondNode> &cond, shared_ptr<StmtNode> &stmt, shared_ptr<StmtNode> &elseStmt)
        : type(type), lVal(lVal), exp(exp), block(block), cond(cond), stmt(stmt), elseStmt(elseStmt){};
};

class ExpNode : public SyntaxNode
{
public:
    string toString(int tabCnt) override;
};

class PrimaryExpNode : public ExpNode
{
public:
    /*
     * NOTE: This primaryExpNode can be type of (Exp), LVal or Number or string.
     */
    PrimaryExpType type;  // PrimaryExp 类型
    shared_ptr<ExpNode> exp;
    shared_ptr<LValNode> lVal;
    int number;
    string str;

    static PrimaryExpNode parentExp(shared_ptr<ExpNode> &exp);

    static PrimaryExpNode lValExp(shared_ptr<LValNode> &lVal);

    static PrimaryExpNode numberExp(int &number);

    static PrimaryExpNode stringExp(string &str);

    string toString(int tabCnt) override;

private:
    PrimaryExpNode(PrimaryExpType type, shared_ptr<ExpNode> &exp, shared_ptr<LValNode> &lVal, int &number, string &str) 
        : type(type), exp(exp), lVal(lVal), number(number), str(str){};
};

class CondNode : public ExpNode
{
public:
    shared_ptr<LOrExpNode> lOrExp;

    explicit CondNode(shared_ptr<LOrExpNode> &lOrExp) 
        : lOrExp(lOrExp){};

    string toString(int tabCnt) override;
};

class LValNode : public ExpNode
{
public:
    shared_ptr<IdentNode> ident;  // 标识符
    int dimension;  // 维数
    vector<shared_ptr<ExpNode>> exps;  // 各元素大小

    LValNode(shared_ptr<IdentNode> &ident, int dimension, vector<shared_ptr<ExpNode>> &exps)
        : ident(ident), dimension(dimension), exps(exps){};

    explicit LValNode(shared_ptr<IdentNode> &ident) : ident(ident), dimension(0){};

    string toString(int tabCnt) override;
};

class UnaryExpNode : public ExpNode
{
public:
    UnaryExpType type;
    shared_ptr<PrimaryExpNode> primaryExp;
    shared_ptr<IdentNode> ident;
    shared_ptr<FuncRParamsNode> funcRParams;
    /*
     * 可为 '+'  '-'  '!'
     */
    string op;
    shared_ptr<UnaryExpNode> unaryExp;

    static UnaryExpNode primaryUnaryExp(shared_ptr<PrimaryExpNode> &primaryExp, string &op);

    static UnaryExpNode funcCallUnaryExp(shared_ptr<IdentNode> &ident, shared_ptr<FuncRParamsNode> &funcRParams, string &op);

    static UnaryExpNode funcCallUnaryExp(shared_ptr<IdentNode> &ident, string &op);

    static UnaryExpNode unaryUnaryExp(shared_ptr<UnaryExpNode> &unaryExp, string &op);

    string toString(int tabCnt) override;

private:
    UnaryExpNode(UnaryExpType type, shared_ptr<PrimaryExpNode> &primaryExp, shared_ptr<IdentNode> &ident,
                 shared_ptr<FuncRParamsNode> &funcRParams, string &op, shared_ptr<UnaryExpNode> &unaryExp)
        : type(type), primaryExp(primaryExp), ident(ident), funcRParams(funcRParams), op(op), unaryExp(unaryExp){};
};

class FuncRParamsNode : public SyntaxNode
{
public:
    vector<shared_ptr<ExpNode>> exps;

    explicit FuncRParamsNode(vector<shared_ptr<ExpNode>> &exps) 
        : exps(exps){};

    string toString(int tabCnt) override;
};

class MulExpNode : public ExpNode
{
public:
    shared_ptr<UnaryExpNode> unaryExp;
    shared_ptr<MulExpNode> mulExp;
    /*
     * 可为 ('*' | '/' | '%')
     */
    string op;

    MulExpNode(shared_ptr<UnaryExpNode> &unaryExp, shared_ptr<MulExpNode> &mulExp, string &op)
        : unaryExp(unaryExp), mulExp(mulExp), op(op){};

    explicit MulExpNode(shared_ptr<UnaryExpNode> &unaryExp) : unaryExp(unaryExp){};

    string toString(int tabCnt) override;
};

class AddExpNode : public ExpNode
{
public:
    shared_ptr<MulExpNode> mulExp;
    shared_ptr<AddExpNode> addExp;
    /*
     * 可为 ('+' | '−') 
     */
    string op;

    AddExpNode(shared_ptr<MulExpNode> &mulExp, shared_ptr<AddExpNode> &addExp, string &op)
        : mulExp(mulExp), addExp(addExp), op(op){};

    explicit AddExpNode(shared_ptr<MulExpNode> &mulExp) : mulExp(mulExp){};

    string toString(int tabCnt) override;
};

class RelExpNode : public ExpNode
{
public:
    shared_ptr<AddExpNode> addExp;
    shared_ptr<RelExpNode> relExp;
    /*
     * 可为 ('<' | '>' | '<=' | '>=')
     */
    string op;

    RelExpNode(shared_ptr<AddExpNode> &addExp, shared_ptr<RelExpNode> &relExp, string &op)
        : addExp(addExp), relExp(relExp), op(op){};

    explicit RelExpNode(shared_ptr<AddExpNode> &addExp) : addExp(addExp){};

    string toString(int tabCnt) override;
};

class EqExpNode : public ExpNode
{
public:
    shared_ptr<RelExpNode> relExp;
    shared_ptr<EqExpNode> eqExp;
    /*
     * 可为 ('==' | '!=')
     */
    string op;

    EqExpNode(shared_ptr<RelExpNode> &relExp, shared_ptr<EqExpNode> &eqExp, string &op)
        : relExp(relExp), eqExp(eqExp), op(op){};

    explicit EqExpNode(shared_ptr<RelExpNode> &relExp) : relExp(relExp){};

    string toString(int tabCnt) override;
};

class LAndExpNode : public ExpNode
{
public:
    shared_ptr<EqExpNode> eqExp;
    shared_ptr<LAndExpNode> lAndExp;
    /*
     * 可为 '&&'
     */
    string op;

    LAndExpNode(shared_ptr<EqExpNode> &eqExp, shared_ptr<LAndExpNode> &lAndExp, string &op)
        : eqExp(eqExp), lAndExp(lAndExp), op(op){};

    explicit LAndExpNode(shared_ptr<EqExpNode> &eqExp) : eqExp(eqExp){};

    string toString(int tabCnt) override;
};

class LOrExpNode : public ExpNode
{
public:
    shared_ptr<LAndExpNode> lAndExp;
    shared_ptr<LOrExpNode> lOrExp;
    /*
     * 可为 '||'
     */
    string op;

    LOrExpNode(shared_ptr<LAndExpNode> &lAndExp, shared_ptr<LOrExpNode> &lOrExp, string &op)
        : lAndExp(lAndExp), lOrExp(lOrExp), op(op){};

    explicit LOrExpNode(shared_ptr<LAndExpNode> &lAndExp) : lAndExp(lAndExp){};

    string toString(int tabCnt) override;
};

class IdentNode : public SyntaxNode
{
public:
    shared_ptr<SymbolTableItem> ident;

    explicit IdentNode(shared_ptr<SymbolTableItem> ident) 
        : ident(move(ident)){};

    string toString(int tabCnt) override;
};

#endif
