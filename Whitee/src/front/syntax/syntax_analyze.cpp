#include "syntax_analyze.h"
#include "symbol_table.h"
#include "../lexer/lexer.h"
#include "../../basic/std/compile_std.h"

#include <iostream>
#include <memory>
#include <unordered_set>

/**
 * Declaration of analyzeFunctions:
 * Note: ConstExp -> AddExp (Ident should be Const).
 * That Const value is calculated while analyzing.
 */

/**
 * @brief get const value(int).
 * @details store the value get in symbol table,
 * for next replace all the const var to a single int value.
 */
int getConstExp();

int getMulExp();

int getUnaryExp();

int getConstVarExp();
// CompUnit -> Decl | FuncDef
shared_ptr<CompUnitNode> analyzeCompUnit ();
// Decl -> ConstDecl | VarDecl
shared_ptr<DeclNode> analyzeDecl();
// ConstDecl -> 'const' 'int' ConstDef { ',' ConstDef } ';'
shared_ptr<ConstDeclNode> analyzeConstDecl();
// ConstDef -> IdentDef '=' ConstInitVal
shared_ptr<ConstDefNode> analyzeConstDef();
// ConstInitVal -> ConstExp  |  ConstInitValArr
shared_ptr<ConstInitValNode> analyzeConstInitVal(const std::shared_ptr<SymbolTableItem> &ident);
// ConstInitValArr -> '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
shared_ptr<ConstInitValArrNode> analyzeConstInitValArr(int dimension, std::vector<int> &numOfEachDimension);
// VarDecl -> 'int' VarDef { ',' VarDef } ';'
shared_ptr<VarDeclNode> analyzeVarDecl();
// VarDef -> IdentDefine [ '=' InitVal ]
shared_ptr<VarDefNode> analyzeVarDef();
// InitVal -> ConstExp  |  Exp  |  ConstInitValArr  |  InitValArr
void analyzeInitVal(const std::shared_ptr<SymbolTableItem> &ident);
// InitValArr -> '{' [ Exp { ',' Exp } ] '}'
shared_ptr<InitValArrNode> analyzeInitValArr(int dimension, std::vector<int> &numOfEachDimension);
// FuncDef -> ('int','void') IdentDefine '(' [FuncFParams] ')' Block
shared_ptr<FuncDefNode> analyzeFuncDef();
// FuncFParams -> FuncFParam { ',' FuncFParam }
shared_ptr<FuncFParamsNode> analyzeFuncFParams();
// FuncFParam -> 'int' IdentDefineInFunction
shared_ptr<FuncFParamNode> analyzeFuncFParam();
// Block -> '{' { BlockItem } '}'
shared_ptr<BlockNode> analyzeBlock(bool isFuncBlock, bool isVoid, bool isInWhileFirstBlock);
// BlockItem -> Decl | Stmt
shared_ptr<BlockItemNode> analyzeBlockItem(bool isInWhileFirstBlock);
// Stmt -> LVal '=' Exp ';'  |  [Exp] ';'  |  Block  |  'if' '( Cond ')' Stmt [ 'else' Stmt ]  |  While  |  'break' ';'  |  'continue' ';'  |  'return' [Exp] ';'
shared_ptr<StmtNode> analyzeStmt(bool isInWhileFirstBlock);
// Exp -> AddExp
shared_ptr<ExpNode> analyzeExp();
// Cond -> LOrExp
shared_ptr<CondNode> analyzeCond();
// LVal -> IdentUsage
shared_ptr<LValNode> analyzeLVal();
// PrimaryExp -> '(' Exp ')'  |  IntConst  |  LVal
shared_ptr<PrimaryExpNode> analyzePrimaryExp();
// UnaryExp -> PrimaryExp  |  IdentUsage '(' [FuncRParams] ')'  |  ('+' | '−' | '!') UnaryExp
shared_ptr<UnaryExpNode> analyzeUnaryExp();
// FuncRParams -> Exp { ',' Exp }
shared_ptr<FuncRParamsNode> analyzeFuncRParams();
// MulExp -> UnaryExp { ('*' | '/' | '%') UnaryExp }
shared_ptr<MulExpNode> analyzeMulExp();
// AddExp -> MulExp { ('+' | '−') MulExp }
shared_ptr<AddExpNode> analyzeAddExp();
// RelExp -> AddExp { ('<' | '>' | '<=' | '>=') AddExp }
shared_ptr<RelExpNode> analyzeRelExp();
// EqExp -> RelExp { ('==' | '!=') RelExp }
shared_ptr<EqExpNode> analyzeEqExp();
// LAndExp -> EqExp { '&&' EqExp }
shared_ptr<LAndExpNode> analyzeLAndExp();
// LOrExp -> LAndExp { '||' LAndExp }
shared_ptr<LOrExpNode> analyzeLOrExp();

/**
 * 语法分析中词法的序号
 */
static int lexerPosition = 0;

//  当前的语法分析对象
static TokenInfo *nowPointerToken;

// 前进到下一个词
void popNextLexer()
{
    if (nowPointerToken->getSym() == TokenType::END)
    {
        std::cerr << "Out of lexer vector range." << endl;
    }
    nowPointerToken = &tokenInfoList[++lexerPosition];
    if (_debugSyntax)
    {
        if (tokenInfoList[lexerPosition - 1].getSym() == TokenType::INTCONST)
        {
            std::cout << tokenInfoList[lexerPosition - 1].getValue() << '\n';
        }
        else
        {
            std::cout << tokenInfoList[lexerPosition - 1].getName() << '\n';
        }
    }
}

// 查看下num个词
TokenInfo *peekNextLexer(int num)
{
    if (lexerPosition + num > tokenInfoList.size())
    {
        std::cerr << "Out of lexer vector range." << endl;
    }
    return &tokenInfoList[lexerPosition + num];
}

static shared_ptr<SymbolTableItem> nowFuncSymbol;  // 当前所在的函数对象
unordered_map<std::string, std::string> usageNameListOfVarSingleUseInUnRecursionFunction;  // 在非递归函数中单次使用变量名称列表

static bool isInWhileLockedCond = false;
static int step = 0;
static bool folderShouldBeOpen = false;
static int folderOpenNum = 0;
static bool isInDirectWhile = false;
static shared_ptr<StmtNode> whileBlockAssignStmt;
static bool lockingOpenFolder = false;
static bool isAssignLValWhileControlVar = false;
static bool judgeWhetherAssignLValWhileControlVar = false;
static int whileControlVarAssignNum = 0;
static shared_ptr<SymbolTableItem> lastAssignOrDeclVar;
static bool isLastValueGetableAssignOrDecl = false;
static bool getAssignLVal = false;
static int lastValue = 0;

bool isExpConstOrIMM(std::shared_ptr<ExpNode> exp)
{
    auto addExp = s_p_c<AddExpNode>(exp);
    if (addExp->addExp)
    {
        return false;
    }
    auto mulExp = addExp->mulExp;
    if (mulExp->mulExp)
    {
        return false;
    }
    auto unaryExp = mulExp->unaryExp;
    bool isOpPlus = true;
    while (unaryExp->type == UnaryExpType::UNARY_DEEP)
    {
        if (unaryExp->op == "-")
        {
            isOpPlus = !isOpPlus;
        }
        unaryExp = unaryExp->unaryExp;
    }
    if (unaryExp->type == UnaryExpType::UNARY_PRIMARY)
    {
        auto primaryExp = unaryExp->primaryExp;
        if (primaryExp->type == PrimaryExpType::PRIMARY_NUMBER)
        {
            lastValue = isOpPlus ? primaryExp->number : -primaryExp->number;
            return true;
        }
        if (primaryExp->type == PrimaryExpType::PRIMARY_L_VAL)
        {
            auto lValExp = primaryExp->lVal;
            if (lValExp->ident->ident->symbolType == SymbolType::CONST_VAR)
            {
                lastValue = s_p_c<ConstInitValValNode>(lValExp->ident->ident->constInitVal)->value;
                lastValue = isOpPlus ? lastValue : -lastValue;
                return true;
            }
        }
    }
    return false;
}

/**
 * Symbol Table:
 */
unordered_map<std::pair<int, int>, std::shared_ptr<SymbolTablePerBlock>, pair_hash> symbolTable; // NOLINT  根据ID存储symbol

std::unordered_map<int, int> blockLayerId2LayerNum; // NOLINT 第i层的最大块ID

static int nowLayer = 0;

static pair<int, int> nowLayId = distributeBlockId(0, {0, 0}); // NOLINT 目前的块ID
static pair<int, int> layIdInFuncFParams;                      // NOLINT 记录函数形参的块ID，不用多次使用
static pair<int, int> globalLayId = {0, 1};                    // NOLINT 全局变量块

bool isGlobalBlock(pair<int, int> layId)
{
    return globalLayId == layId;
}

 /**
  * @brief get a ident
  * @details 当是一个变量:
  * 1) var是正常的var
  * - var是非Ary var.
  * - 变量是无名变量。
  * 2) var is const
  * - var 是 const non ary var.
  * - var 是 const ary var.
  * 所有这些类型都是这样做的。
  * 1. 将创建的符号项插入到符号表中。
  * 2. ret IdentNode {
  *     symbolTableItem {
  *         所有的细节。
  *     }
  * }
  *
  * 当是为一个函数时。
  * 1. 将创建的符号表项插入符号表。
  * 2. ret IdentNode {
  *     symbolTableItem {
  *            名字。
  *         符号类型。
  *         blockId.
  *     }
  * }
  * 3. IdentNode ret将被用于一个FuncNode，FuncNode包括一个identNode。
  * 
  * @param isF 无论Ident是用于一个函数还是一个var。
  * @param isVoid 当Ident是用于一个函数时，区分函数的类型。RET & VOID
  * @param isConst 当身份是var时，区分var的类型。CONST & VAR
  * @return a identNode.
  */
// IdentDef -> Ident { '[' ConstExp ']' }
shared_ptr<IdentNode> getIdentDefine(bool isF, bool isVoid, bool isConst)
{
    if (_debugSyntax)
    {
        std::cout << "--------getIdentDefine--------\n";
    }
    std::string name = nowPointerToken->getName();
    popNextLexer();
    if (isF)
    {
        SymbolType symbolType = isVoid ? SymbolType::VOID_FUNC : SymbolType::RET_FUNC;
        auto symbolTableItem = std::make_shared<SymbolTableItem>(symbolType, name, nowLayId);
        nowFuncSymbol = symbolTableItem;
        insertSymbol(nowLayId, symbolTableItem);
        return std::make_shared<IdentNode>(symbolTableItem);
    }
    int dimension = 0;
    std::vector<int> numOfEachDimension;
    while (nowPointerToken->getSym() == TokenType::LBRACKET)
    {
        popNextLexer(); // LBRACKET
        dimension++;
        numOfEachDimension.push_back(getConstExp());
        popNextLexer(); // RBRACKET
    }
    if (dimension == 0)
    {
        SymbolType symbolType = (isConst ? SymbolType::CONST_VAR : SymbolType::VAR);
        auto symbolTableItem = std::make_shared<SymbolTableItem>(symbolType, name, nowLayId);
        if (openFolder && !lockingOpenFolder)
        {
            lastAssignOrDeclVar = symbolTableItem;
        }
        insertSymbol(nowLayId, symbolTableItem);
        return std::make_shared<IdentNode>(symbolTableItem);
    }
    else
    {
        SymbolType symbolType = (isConst ? SymbolType::CONST_ARRAY : SymbolType::ARRAY);
        auto symbolTableItem = std::make_shared<SymbolTableItem>(symbolType, dimension, numOfEachDimension, name,
                                                                 nowLayId);
        insertSymbol(nowLayId, symbolTableItem);
        return std::make_shared<IdentNode>(symbolTableItem);
    }
}

/**
 * @details 在函数中对ident的使用
 * 对于维度>0的身份是一个[][*][*]。
 * 应该考虑第一个[]。
 * @return
 */
// IdentDefineInFunction -> [ '[' ']' { '[' ConstExp ']' } ]
shared_ptr<IdentNode> getIdentDefineInFunction()
{
    if (_debugSyntax)
    {
        std::cout << "--------getIdentDefine--------\n";
    }
    std::string name = nowPointerToken->getName();
    popNextLexer();
    int dimension = 0;
    std::vector<int> numOfEachDimension;
    if (nowPointerToken->getSym() == TokenType::LBRACKET)
    {
        popNextLexer();
        dimension++;
        numOfEachDimension.push_back(0);
        popNextLexer();
    }
    while (nowPointerToken->getSym() == TokenType::LBRACKET)
    {
        popNextLexer(); // LBRACKET
        dimension++;
        numOfEachDimension.push_back(getConstExp());
        popNextLexer(); // RBRACKET
    }
    if (dimension == 0)
    {
        SymbolType symbolType = (SymbolType::VAR);
        auto symbolTableItem = std::make_shared<SymbolTableItem>(symbolType, name, nowLayId);
        insertSymbol(nowLayId, symbolTableItem);
        return std::make_shared<IdentNode>(symbolTableItem);
    }
    else
    {
        SymbolType symbolType = (SymbolType::ARRAY);
        auto symbolTableItem = std::make_shared<SymbolTableItem>(symbolType, dimension, numOfEachDimension, name,
                                                                 nowLayId);
        insertSymbol(nowLayId, symbolTableItem);
        return std::make_shared<IdentNode>(symbolTableItem);
    }
}

 /**
  * @details
  * !!!当 !isF时，应该复制 symbolTableItem, 因为 expressionOfPerDimension 在 symbolTableItem 中。
  * 当使用shared_ptr时，所有的是共享的。在一个地方改变，在另一个地方改变，会导致错误。
  * 应该复制一个新的。
  * 不需要函数。
  * @param isF
  * @return
  */
// IdentUsage -> Ident  |  Ident {'[' Exp ']'}
shared_ptr<IdentNode> getIdentUsage(bool isF)
{
    if (_debugSyntax)
    {
        std::cout << "--------getIdentUsage--------\n";
    }
    std::string name = nowPointerToken->getName();
    popNextLexer();
    auto symbolInTable = findSymbol(nowLayId, name, isF);
    if (symbolInTable->eachFuncUseNum.find(nowFuncSymbol->usageName) == symbolInTable->eachFuncUseNum.end())
    {
        symbolInTable->eachFuncUseNum[nowFuncSymbol->usageName] = 0;
        symbolInTable->eachFunc.push_back(nowFuncSymbol);
    }
    symbolInTable->eachFuncUseNum[nowFuncSymbol->usageName]++;
    if (openFolder && getAssignLVal && !lockingOpenFolder)
    {
        lastAssignOrDeclVar = symbolInTable;
        getAssignLVal = false;
    }
    if (openFolder && lockingOpenFolder && judgeWhetherAssignLValWhileControlVar)
    {
        isAssignLValWhileControlVar = (symbolInTable == lastAssignOrDeclVar);
        judgeWhetherAssignLValWhileControlVar = false;
    }
    if (isF)
    {
        unordered_set<string> sysFunc({"getint", "getch", "getarray", "putint", "putch", "putarray", "putf", "starttime", "stoptime"});

        if (nowFuncSymbol == symbolInTable)
        {
            symbolInTable->isRecursion = true;
        }
        if (openFolder)
        {
            if (sysFunc.find(nowFuncSymbol->name) == sysFunc.end())
            {
                whileControlVarAssignNum += 2;
            }
        }
        return std::make_shared<IdentNode>(
            std::make_shared<SymbolTableItem>(symbolInTable->symbolType, name, symbolInTable->blockId));
    }
    else
    {
        std::vector<std::shared_ptr<ExpNode>> expressionOfEachDimension;
        while (nowPointerToken->getSym() == TokenType::LBRACKET)
        {
            popNextLexer(); // LBRACKET
            expressionOfEachDimension.push_back(analyzeExp());
            popNextLexer(); // RBRACKET
        }
        std::shared_ptr<SymbolTableItem> symbolInIdent(
            new SymbolTableItem(symbolInTable->symbolType, symbolInTable->dimension,
                                expressionOfEachDimension, name,
                                symbolInTable->blockId));
        if (symbolInTable->symbolType == SymbolType::CONST_ARRAY ||
            symbolInTable->symbolType == SymbolType::CONST_VAR)
        {
            symbolInIdent->constInitVal = symbolInTable->constInitVal;
        }
        symbolInIdent->numOfEachDimension = symbolInTable->numOfEachDimension;
        return std::make_shared<IdentNode>(symbolInIdent);
    }
}

/**
 * Definition of analyzeFunctions.
 */
// Decl -> ConstDecl | VarDecl
shared_ptr<DeclNode> analyzeDecl()
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeDecl--------\n";
    }
    shared_ptr<DeclNode> declNode(nullptr);
    if (openFolder)
    {
        whileControlVarAssignNum += 2;
    }
    if (nowPointerToken->getSym() == TokenType::CONST_TK)
    {
        declNode = analyzeConstDecl();
    }
    else
    {
        declNode = analyzeVarDecl();
    }
    return declNode;
}

// ConstDecl -> 'const' 'int' ConstDef { ',' ConstDef } ';'
shared_ptr<ConstDeclNode> analyzeConstDecl()
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeConstDecl--------\n";
    }
    popNextLexer(); // CONST_TK
    popNextLexer(); // INT_TK
    vector<shared_ptr<ConstDefNode>> constDefList;
    constDefList.push_back(analyzeConstDef());
    while (nowPointerToken->getSym() == TokenType::COMMA)
    {
        popNextLexer(); // COMMA
        constDefList.push_back(analyzeConstDef());
    }
    popNextLexer(); // SEMICOLON
    return std::make_shared<ConstDeclNode>(constDefList);
}

// ConstDef -> IdentDef '=' ConstInitVal
shared_ptr<ConstDefNode> analyzeConstDef()
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeConstDef--------\n";
    }
    auto ident = getIdentDefine(false, false, true);
    popNextLexer(); // ASSIGN
    auto constInitVal = analyzeConstInitVal(ident->ident);
    return std::make_shared<ConstDefNode>(ident, constInitVal);
}

/**
 * @param ident是一个SymbolTableItem，用它来获得这个const var的尺寸。
 * 判断constInitVal的类型。
 * 1. ConstInitValValNode
 * 2. ConstInitValAryNode
 * @return
 */
// ConstInitVal -> ConstExp  |  ConstInitValArr
shared_ptr<ConstInitValNode> analyzeConstInitVal(const std::shared_ptr<SymbolTableItem> &ident)
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeInitVal--------\n";
    }
    if (ident->dimension == 0)
    {
        int value = getConstExp();
        ident->constInitVal = std::make_shared<ConstInitValValNode>(value);
    }
    else
    {
        ident->constInitVal = analyzeConstInitValArr(0, ident->numOfEachDimension);
    }
    return ident->constInitVal;
}

/**
 * @deprecated
 * for case: const int a[10000000] = {1};
 * Time usage is not bearable.
 */
shared_ptr<ConstInitValArrNode> constZeroDefaultArr(int dimension, std::vector<int> &numOfEachDimension)
{
    int thisDimensionNum = numOfEachDimension[dimension];
    vector<shared_ptr<ConstInitValNode>> valueList;
    if (dimension + 1 == numOfEachDimension.size())
    {
        for (int i = 0; i < thisDimensionNum; i++)
        {
            int exp = 0;
            valueList.push_back(std::make_shared<ConstInitValValNode>(exp));
        }
    }
    else
    {
        for (int i = 0; i < thisDimensionNum; i++)
        {
            valueList.push_back(constZeroDefaultArr(dimension + 1, numOfEachDimension));
        }
    }
    return std::make_shared<ConstInitValArrNode>(thisDimensionNum, valueList);
}

/**
 * @brief Get Const Arr Init Value.
 * @details Dimension是一个重要的参数。
 * 对于这个函数来说，它是用于递归的。
 * Dimension = 1意味着Arr是一维的。
 * 数组中的项目应该是ConstInitValValNode而不是ConstInitValArrNode。
 */
// ConstInitValArr -> '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
shared_ptr<ConstInitValArrNode> analyzeConstInitValArr(int dimension, std::vector<int> &numOfEachDimension)
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeConstInitValArr--------\n";
    }
    int thisDimensionNum = numOfEachDimension[dimension];
    vector<shared_ptr<ConstInitValNode>> valueList;
    if (dimension + 1 == numOfEachDimension.size())
    {
        if (nowPointerToken->getSym() == TokenType::LBRACE)
        {
            popNextLexer(); // LBRACE
            for (int i = 0; i < thisDimensionNum; i++)
            {
                if (nowPointerToken->getSym() == TokenType::RBRACE)
                {
                    break;
                }
                else
                {
                    auto exp = getConstExp();
                    valueList.push_back(std::make_shared<ConstInitValValNode>(exp));
                    if (nowPointerToken->getSym() == TokenType::COMMA)
                    {
                        popNextLexer(); // COMMA
                    }
                }
            }
            popNextLexer(); // RBRACE
        }
        else
        {
            for (int i = 0; i < thisDimensionNum; i++)
            {
                if (nowPointerToken->getSym() == TokenType::LBRACE || nowPointerToken->getSym() == TokenType::RBRACE)
                {
                    break;
                }
                auto exp = getConstExp();
                valueList.push_back(std::make_shared<ConstInitValValNode>(exp));
                if (nowPointerToken->getSym() == TokenType::COMMA)
                {
                    popNextLexer(); // COMMA
                }
            }
        }
        return std::make_shared<ConstInitValArrNode>(thisDimensionNum, valueList);
    }
    if (nowPointerToken->getSym() == TokenType::LBRACE)
    {
        popNextLexer(); // LBRACE
        for (int i = 0; i < thisDimensionNum; i++)
        {
            if (nowPointerToken->getSym() == TokenType::RBRACE)
            {
                break;
            }
            else
            {
                valueList.push_back(analyzeConstInitValArr(dimension + 1, numOfEachDimension));
                if (nowPointerToken->getSym() == TokenType::COMMA)
                {
                    popNextLexer(); // COMMA
                }
            }
        }
        popNextLexer(); // RBRACE
    }
    else
    {
        for (int i = 0; i < thisDimensionNum; i++)
        {
            valueList.push_back(analyzeConstInitValArr(dimension + 1, numOfEachDimension));
            if (nowPointerToken->getSym() == TokenType::COMMA)
            {
                popNextLexer(); // COMMA
            }
        }
    }
    return std::make_shared<ConstInitValArrNode>(thisDimensionNum, valueList);
}

// ConstExp -> MulExp
int getConstExp()
{
    if (_debugSyntax)
    {
        std::cout << "--------getConstExp--------\n";
    }
    int value = getMulExp();
    while (nowPointerToken->getSym() == TokenType::PLUS || nowPointerToken->getSym() == TokenType::MINUS)
    {
        if (nowPointerToken->getSym() == TokenType::PLUS)
        {
            popNextLexer();
            value += getMulExp();
        }
        else
        {
            popNextLexer();
            value -= getMulExp();
        }
    }
    return value;
}

// MulExp -> UnaryExp  { ('*' | '/' | '%') UnaryExp}
int getMulExp()
{
    if (_debugSyntax)
    {
        std::cout << "--------getMulExp--------\n";
    }
    int value = getUnaryExp();
    while (nowPointerToken->getSym() == TokenType::MULT || nowPointerToken->getSym() == TokenType::DIV ||
           nowPointerToken->getSym() == TokenType::REMAIN)
    {
        if (nowPointerToken->getSym() == TokenType::MULT)
        {
            popNextLexer();
            value *= getUnaryExp();
        }
        else if (nowPointerToken->getSym() == TokenType::DIV)
        {
            popNextLexer();
            value /= getUnaryExp();
        }
        else
        {
            popNextLexer();
            value %= getUnaryExp();
        }
    }
    return value;
}

// UnaryExp -> '(' ConstExp  ')'  |  Number  |  ('+','-') UnaryExp   |   ConstVarExp
int getUnaryExp()
{
    if (_debugSyntax)
    {
        std::cout << "--------getUnaryExp--------\n";
    }
    int value;
    if (nowPointerToken->getSym() == TokenType::LPAREN)
    {
        popNextLexer();
        value = getConstExp();
        popNextLexer();
    }
    else if (nowPointerToken->getSym() == TokenType::INTCONST)
    {
        value = nowPointerToken->getValue();
        popNextLexer();
    }
    else if (nowPointerToken->getSym() == TokenType::PLUS)
    {
        popNextLexer();
        value = getUnaryExp();
    }
    else if (nowPointerToken->getSym() == TokenType::MINUS)
    {
        popNextLexer();
        value = -getUnaryExp();
    }
    else
    {
        value = getConstVarExp();
    }
    return value;
}

// ConstVarExp -> Ident { '[' ConstExp ']' }
int getConstVarExp()
{
    if (_debugSyntax)
    {
        std::cout << "--------getConstVarExp--------\n";
    }
    std::string name = nowPointerToken->getName();
    popNextLexer(); // IDENT
    auto symbolInTable = findSymbol(nowLayId, name, false);
    int dimension = symbolInTable->dimension;
    vector<shared_ptr<ConstInitValNode>> valList;
    if (dimension > 0)
    {
        if (symbolInTable->symbolType == SymbolType::CONST_ARRAY)
        {
            valList = (s_p_c<ConstInitValArrNode>(symbolInTable->constInitVal))->valList;
        }
        else
        {
            valList = (s_p_c<ConstInitValArrNode>(symbolInTable->globalVarInitVal))->valList;
        }
    }
    else
    {
        if (symbolInTable->symbolType == SymbolType::CONST_VAR)
        {
            return (s_p_c<ConstInitValValNode>(symbolInTable->constInitVal))->value;
        }
        return (s_p_c<ConstInitValValNode>(symbolInTable->globalVarInitVal))->value;
    }
    while (dimension > 0)
    {
        dimension--;
        popNextLexer(); // LBRACKET
        int offset = getConstExp();
        popNextLexer(); // RBRACKET
        if (offset >= valList.size())
        {
            return 0;
        }
        if (dimension == 0)
        {
            return (s_p_c<ConstInitValValNode>(valList[offset]))->value;
        }
        else
        {
            valList = (s_p_c<ConstInitValArrNode>(valList[offset]))->valList;
        }
    }
    cerr << "getConstVarExp: Source code may have errors." << endl;
    return -1;
} // NOLINT

// VarDecl -> 'int' VarDef { ',' VarDef } ';'
std::shared_ptr<VarDeclNode> analyzeVarDecl()
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeVarDecl--------\n";
    }
    popNextLexer(); // INT_TK
    vector<shared_ptr<VarDefNode>> varDefList;
    varDefList.push_back(analyzeVarDef());
    while (nowPointerToken->getSym() == TokenType::COMMA)
    {
        popNextLexer(); // INT_TK
        varDefList.push_back(analyzeVarDef());
    }
    popNextLexer(); // SEMICOLON
    return std::make_shared<VarDeclNode>(varDefList);
}

// VarDef -> IdentDefine [ '=' InitVal ]
std::shared_ptr<VarDefNode> analyzeVarDef()
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeVarDef--------\n";
    }
    auto ident = getIdentDefine(false, false, false);
    bool hasAssigned = false;
    if (nowPointerToken->getSym() == TokenType::ASSIGN)
    {
        hasAssigned = true;
        popNextLexer(); // ASSIGN
        analyzeInitVal(ident->ident);
        std::shared_ptr<InitValNode> initVal;
        if (!isGlobalBlock(ident->ident->blockId))
        {
            initVal = ident->ident->initVal;
            if (ident->ident->dimension == 0)
            {
                return std::make_shared<VarDefNode>(ident, initVal);
            }
            else
            {
                return std::make_shared<VarDefNode>(ident, ident->ident->dimension, ident->ident->numOfEachDimension,
                                                    initVal);
            }
        }
    }
    if (!hasAssigned && isGlobalBlock(ident->ident->blockId))
    {
        if (ident->ident->dimension == 0)
        {
            int value = 0;
            ident->ident->globalVarInitVal = std::make_shared<ConstInitValValNode>(value);
        }
    }
    if (ident->ident->dimension == 0)
    {
        return std::make_shared<VarDefNode>(ident);
    }
    else
    {
        return std::make_shared<VarDefNode>(ident, ident->ident->dimension, ident->ident->numOfEachDimension);
    }
}

// InitVal -> ConstExp  |  Exp  |  ConstInitValArr  |  InitValArr
void analyzeInitVal(const std::shared_ptr<SymbolTableItem> &ident)
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeInitVal--------\n";
    }
    if (ident->dimension == 0)
    {
        if (isGlobalBlock(ident->blockId))
        {
            int value = getConstExp();
            if (openFolder && !lockingOpenFolder)
            {
                lastValue = value;
                isLastValueGetableAssignOrDecl = true;
            }
            ident->globalVarInitVal = std::make_shared<ConstInitValValNode>(value);
        }
        else
        {
            auto expNode = analyzeExp();
            if (openFolder && isExpConstOrIMM(expNode) && !lockingOpenFolder)
            {
                isLastValueGetableAssignOrDecl = true;
            }
            ident->initVal = std::make_shared<InitValValNode>(expNode);
        }
    }
    else
    {
        if (isGlobalBlock(ident->blockId))
        {
            ident->globalVarInitVal = analyzeConstInitValArr(0, ident->numOfEachDimension);
        }
        else
        {
            ident->initVal = analyzeInitValArr(0, ident->numOfEachDimension);
        }
    }
}

/**
 * @deprecated
 */
std::shared_ptr<InitValArrNode> zeroDefaultArr(int dimension, std::vector<int> &numOfEachDimension)
{
    int thisDimensionNum = numOfEachDimension[dimension];
    vector<shared_ptr<InitValNode>> valueList;
    if (dimension + 1 == numOfEachDimension.size())
    {
        for (int i = 0; i < thisDimensionNum; i++)
        {
            int value = 0;
            auto exp = std::make_shared<PrimaryExpNode>(PrimaryExpNode::numberExp(value));
            auto expExp = s_p_c<ExpNode>(exp);
            valueList.push_back(std::make_shared<InitValValNode>(expExp));
        }
    }
    else
    {
        for (int i = 0; i < thisDimensionNum; i++)
        {
            valueList.push_back(zeroDefaultArr(dimension + 1, numOfEachDimension));
        }
    }
    return std::make_shared<InitValArrNode>(thisDimensionNum, valueList);
}

// InitValArr -> '{' [ Exp { ',' Exp } ] '}'
std::shared_ptr<InitValArrNode> analyzeInitValArr(int dimension, std::vector<int> &numOfEachDimension)
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeInitValArr--------\n";
    }
    int thisDimensionNum = numOfEachDimension[dimension];
    vector<shared_ptr<InitValNode>> valueList;
    if (dimension + 1 == numOfEachDimension.size())
    {
        if (nowPointerToken->getSym() == TokenType::LBRACE)
        {
            popNextLexer(); // LBRACE
            for (int i = 0; i < thisDimensionNum; i++)
            {
                if (nowPointerToken->getSym() == TokenType::RBRACE)
                {
                    break;
                }
                else
                {
                    auto exp = analyzeExp();
                    valueList.push_back(std::make_shared<InitValValNode>(exp));
                    if (nowPointerToken->getSym() == TokenType::COMMA)
                    {
                        popNextLexer(); // COMMA
                    }
                }
            }
            popNextLexer(); // RBRACE
        }
        else
        {
            for (int i = 0; i < thisDimensionNum; i++)
            {
                if (nowPointerToken->getSym() == TokenType::LBRACE || nowPointerToken->getSym() == TokenType::RBRACE)
                {
                    break;
                }
                auto exp = analyzeExp();
                valueList.push_back(std::make_shared<InitValValNode>(exp));
                if (nowPointerToken->getSym() == TokenType::COMMA)
                {
                    popNextLexer(); // COMMA
                }
            }
        }
        return std::make_shared<InitValArrNode>(thisDimensionNum, valueList);
    }
    if (nowPointerToken->getSym() == TokenType::LBRACE)
    {
        popNextLexer(); // LBRACE
        for (int i = 0; i < thisDimensionNum; i++)
        {
            if (nowPointerToken->getSym() == TokenType::RBRACE)
            {
                break;
            }
            else
            {
                valueList.push_back(analyzeInitValArr(dimension + 1, numOfEachDimension));
                if (nowPointerToken->getSym() == TokenType::COMMA)
                {
                    popNextLexer(); // COMMA
                }
            }
        }
        popNextLexer(); // RBRACE
    }
    else
    {
        for (int i = 0; i < thisDimensionNum; i++)
        {
            valueList.push_back(analyzeInitValArr(dimension + 1, numOfEachDimension));
            if (nowPointerToken->getSym() == TokenType::COMMA)
            {
                popNextLexer(); // COMMA
            }
        }
    }
    return std::make_shared<InitValArrNode>(thisDimensionNum, valueList);
}

// FuncDef -> ('int','void') IdentDefine '(' [FuncFParams] ')' Block
shared_ptr<FuncDefNode> analyzeFuncDef()
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeFuncDef--------\n";
    }
    bool isVoid = nowPointerToken->getSym() == TokenType::VOID_TK;
    popNextLexer(); // VOID_TK || INT_TK
    FuncType funcType = isVoid ? FuncType::FUNC_VOID : FuncType::FUNC_INT;
    bool hasParams = false;
    auto ident = getIdentDefine(true, isVoid, false);
    popNextLexer(); // LPAREN
    shared_ptr<FuncFParamsNode> funcFParams;
    pair<int, int> fatherLayId = nowLayId;
    nowLayer++;
    nowLayId = distributeBlockId(nowLayer, fatherLayId);
    layIdInFuncFParams = nowLayId;
    if (nowPointerToken->getSym() != TokenType::RPAREN)
    {
        funcFParams = analyzeFuncFParams();
        hasParams = true;
    }
    popNextLexer(); // RPAREN
    nowLayer--;
    nowLayId = fatherLayId;
    auto block = analyzeBlock(true, isVoid, false);
    if (hasParams)
    {
        return std::make_shared<FuncDefNode>(funcType, ident, funcFParams, block);
    }
    return std::make_shared<FuncDefNode>(funcType, ident, block);
}

// FuncFParams -> FuncFParam { ',' FuncFParam }
shared_ptr<FuncFParamsNode> analyzeFuncFParams()
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeFuncFParams--------\n";
    }
    vector<shared_ptr<FuncFParamNode>> funcParamList;
    funcParamList.push_back(analyzeFuncFParam());
    while (nowPointerToken->getSym() == TokenType::COMMA)
    {
        popNextLexer(); // COMMA
        funcParamList.push_back(analyzeFuncFParam());
    }
    return std::make_shared<FuncFParamsNode>(funcParamList);
}

// FuncFParam -> 'int' IdentDefineInFunction
shared_ptr<FuncFParamNode> analyzeFuncFParam()
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeFuncFParam--------\n";
    }
    popNextLexer(); // INT_TK
    auto ident = getIdentDefineInFunction();
    return std::make_shared<FuncFParamNode>(ident, ident->ident->dimension, ident->ident->numOfEachDimension);
}

/**
 * @brief 获取一个块的代码
 * @details 当进入一个区块时，应该注意到blockId。
 * 需要做的是添加层，并得到一个新的layerId。
 * 当从块中出来时，将layerId设置为现在的layer的父亲。
 * @param isFuncBlock 如果是FuncBlock，则使用layIdInFuncParam。
 * 因为它是在同一个块中。不要再生成一个块。
 * @return
 */
// Block -> '{' { BlockItem } '}'
shared_ptr<BlockNode> analyzeBlock(bool isFuncBlock, bool isVoid, bool isInWhileFirstBlock)
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeBlock--------\n";
    }
    pair<int, int> fatherLayId = nowLayId;
    nowLayer++;
    if (isFuncBlock)
    {
        nowLayId = layIdInFuncFParams;
    }
    else
    {
        nowLayId = distributeBlockId(nowLayer, fatherLayId);
    }
    int itemCnt = 0;
    vector<shared_ptr<BlockItemNode>> blockItems;
    popNextLexer(); // LBRACE
    while (nowPointerToken->getSym() != TokenType::RBRACE)
    {
        auto blockItem = analyzeBlockItem(isInWhileFirstBlock);
        if (openFolder && folderShouldBeOpen)
        {
            for (int i = 0; i < folderOpenNum; i++)
            {
                blockItems.push_back(blockItem);
                ++itemCnt;
            }
            folderShouldBeOpen = false;
        }
        else
        {
            blockItems.push_back(blockItem);
            ++itemCnt;
        }
    }
    if (isVoid)
    {
        auto defaultReturn = std::make_shared<StmtNode>(StmtNode::returnStmt());
        blockItems.push_back(defaultReturn);
    }
    popNextLexer(); // RBRACE
    nowLayer--;
    nowLayId = fatherLayId;
    return std::make_shared<BlockNode>(itemCnt, blockItems);
}

// BlockItem -> Decl | Stmt
shared_ptr<BlockItemNode> analyzeBlockItem(bool isInWhileFirstBlock)
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeBlockItem--------\n";
    }
    if (nowPointerToken->getSym() == TokenType::INT_TK || nowPointerToken->getSym() == TokenType::CONST_TK)
    {
        return analyzeDecl();
    }
    else
    {
        return analyzeStmt(isInWhileFirstBlock);
    }
}

// 赋值语句
bool isAssign()
{
    int peekNum = 0;
    while (peekNextLexer(peekNum)->getSym() != TokenType::SEMICOLON)
    {
        if (peekNextLexer(peekNum)->getSym() == TokenType::ASSIGN)
        {
            return true;
        }
        peekNum++;
    }
    return false;
}


bool isSingleCondAddExpThisSymbol(std::shared_ptr<AddExpNode> addExp, std::shared_ptr<SymbolTableItem> symbol)
{
    if (addExp->addExp)
    {
        return false;
    }
    auto mulExp = addExp->mulExp;
    if (mulExp->mulExp)
    {
        return false;
    }
    auto unaryExp = mulExp->unaryExp;
    if (unaryExp->type == UnaryExpType::UNARY_PRIMARY)
    {
        auto primaryExp = unaryExp->primaryExp;
        if (primaryExp->type == PrimaryExpType::PRIMARY_L_VAL)
        {
            auto lValExp = primaryExp->lVal;
            if (lValExp->ident->ident->symbolType == SymbolType::VAR)
            {
                if (lValExp->ident->ident->name == symbol->name)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

static int whileControlVarEndValue = 0;
static TokenType relation = TokenType::EQUAL;

bool isCondSingleAndUseLastInitOrDeclVar(std::shared_ptr<CondNode> cond, std::shared_ptr<SymbolTableItem> symbol)
{
    auto lOrExp = cond->lOrExp;
    if (lOrExp->lOrExp)
    {
        return false;
    }
    auto lAndExp = lOrExp->lAndExp;
    if (lAndExp->lAndExp)
    {
        return false;
    }
    auto eqExp = lAndExp->eqExp;
    if (eqExp->eqExp)
    {
        return false;
    }
    auto relExp = eqExp->relExp;
    if (relExp->relExp && relExp->addExp)
    {
        if (relExp->relExp->relExp)
        {
            return false;
        }
        if (!isSingleCondAddExpThisSymbol(relExp->relExp->addExp, symbol))
        {
            return false;
        }
        if (!isExpConstOrIMM(relExp->addExp))
        {
            return false;
        }
        whileControlVarEndValue = lastValue;
        return true;
    }
    return false;
}

/**
 * @brief lVal已经被判断出来了
 * 只有whileControlVar的lVal赋值应该被记录并使用这个判断。
 * mulExp应该是一个 "num"。
 */
bool isAssignStepStyle(shared_ptr<StmtNode> assignStmt)
{
    auto exp = assignStmt->exp;
    auto addExp = dynamic_pointer_cast<AddExpNode>(exp);
    bool isStepPlus = true;
    if (!addExp->addExp)
    {
        return false;
    }
    if (addExp->op == "-")
    {
        isStepPlus = !isStepPlus;
    }
    auto mulExp = addExp->mulExp;
    addExp = addExp->addExp;
    if (mulExp->mulExp)
    {
        return false;
    }
    auto unaryExp = mulExp->unaryExp;
    while (unaryExp->type == UnaryExpType::UNARY_DEEP)
    {
        if (unaryExp->op == "-")
        {
            isStepPlus = !isStepPlus;
            unaryExp = unaryExp->unaryExp;
        }
    }
    if (unaryExp->type != UnaryExpType::UNARY_PRIMARY)
    {
        return false;
    }
    auto primaryExp = unaryExp->primaryExp;
    if (primaryExp->type == PrimaryExpType::PRIMARY_NUMBER)
    {
        step = isStepPlus ? primaryExp->number : -primaryExp->number;
    }
    else
    {
        return false;
    }
    if (addExp->addExp)
    {
        return false;
    }
    mulExp = addExp->mulExp;
    if (mulExp->mulExp)
    {
        return false;
    }
    unaryExp = mulExp->unaryExp;
    if (unaryExp->type != UnaryExpType::UNARY_PRIMARY)
    {
        return false;
    }
    primaryExp = unaryExp->primaryExp;
    return primaryExp->type == PrimaryExpType::PRIMARY_L_VAL && primaryExp->lVal->ident->ident->usageName == lastAssignOrDeclVar->usageName;
}

// While -> 'while' '(' Cond ')' Stmt
shared_ptr<StmtNode> analyzeWhile()
{
    int whileControlStartValue = lastValue;
    popNextLexer(); // While
    popNextLexer();
    if (openFolder)
    {
        isInWhileLockedCond = true;
    }
    auto cond = analyzeCond();
    popNextLexer();
    shared_ptr<StmtNode> whileInsideStmt;
    if (openFolder && !lockingOpenFolder && isLastValueGetableAssignOrDecl)
    {
        isLastValueGetableAssignOrDecl = false;
        auto whileControlVar = lastAssignOrDeclVar;
        lockingOpenFolder = true;
        if (isCondSingleAndUseLastInitOrDeclVar(cond, whileControlVar))
        {
            whileControlVarAssignNum = 0;
            isInDirectWhile = true;
            whileInsideStmt = analyzeStmt(true);
            if (whileControlVarAssignNum == 1)
            {
                if (isAssignStepStyle(whileBlockAssignStmt))
                {
                    if (relation == TokenType::LARGE || relation == TokenType::LEQ || relation == TokenType::LESS ||
                        relation == TokenType::LAQ)
                    {
                        if (relation == TokenType::LARGE)
                        {
                            if (whileControlStartValue <= whileControlVarEndValue)
                            {
                                folderOpenNum = 0;
                                return whileInsideStmt;
                            }
                            else
                            {
                                folderOpenNum = (whileControlVarEndValue - whileControlStartValue) / step;
                            }
                        }
                        if (relation == TokenType::LAQ)
                        {
                            if (whileControlStartValue < whileControlVarEndValue)
                            {
                                folderOpenNum = 0;
                                return whileInsideStmt;
                            }
                            else
                            {
                                folderOpenNum = (whileControlVarEndValue - whileControlStartValue - 1) / step;
                            }
                        }
                        if (relation == TokenType::LESS)
                        {
                            if (whileControlStartValue > whileControlVarEndValue)
                            {
                                folderOpenNum = 0;
                                return whileInsideStmt;
                            }
                            else
                            {
                                folderOpenNum = (whileControlVarEndValue - whileControlStartValue) / step;
                            }
                        }
                        if (relation == TokenType::LEQ)
                        {
                            if (whileControlStartValue >= whileControlVarEndValue)
                            {
                                folderOpenNum = 0;
                                return whileInsideStmt;
                            }
                            else
                            {
                                folderOpenNum = (whileControlVarEndValue - whileControlStartValue + 1) / step;
                            }
                        }
                        isInDirectWhile = false;
                        lockingOpenFolder = false;
                        if (folderOpenNum >= 0 && folderOpenNum < 1000)
                        {
                            folderShouldBeOpen = true;
                            return whileInsideStmt;
                        }
                    }
                }
            }
            isInDirectWhile = false;
            lockingOpenFolder = false;
            return std::make_shared<StmtNode>(StmtNode::whileStmt(cond, whileInsideStmt));
        }
        lockingOpenFolder = false;
    }
    auto tmp = isInDirectWhile;
    if (openFolder)
    {
        isLastValueGetableAssignOrDecl = false;
        isInDirectWhile = false;
    }
    whileInsideStmt = analyzeStmt(false);
    if (openFolder)
    {
        isInDirectWhile = tmp;
    }
    return std::make_shared<StmtNode>(StmtNode::whileStmt(cond, whileInsideStmt));
}

// Stmt -> Block  |  'break' ';'  |  'continue' ';'  |  'if' '( Cond ')' Stmt [ 'else' Stmt ]  |  While  |  'return' [Exp] ';'  |  ';'  |  LVal '=' Exp ';'  |  Exp
shared_ptr<StmtNode> analyzeStmt(bool isInWhileFirstBlock)
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeStmt--------\n";
    }
    if (nowPointerToken->getSym() == TokenType::LBRACE)
    {
        if (openFolder)
        {
            isLastValueGetableAssignOrDecl = false;
        }
        auto blockNode = analyzeBlock(false, false, isInWhileFirstBlock);
        return std::make_shared<StmtNode>(StmtNode::blockStmt(blockNode));
    }
    else if (nowPointerToken->getSym() == TokenType::BREAK_TK)
    {
        if (openFolder)
        {
            isLastValueGetableAssignOrDecl = false;
        }
        if (openFolder && isInWhileFirstBlock)
        {
            whileControlVarAssignNum = 2;
        }
        popNextLexer();
        popNextLexer();
        return std::make_shared<StmtNode>(StmtNode::breakStmt());
    }
    else if (nowPointerToken->getSym() == TokenType::CONTINUE_TK)
    {
        if (openFolder)
        {
            isLastValueGetableAssignOrDecl = false;
        }
        if (openFolder && isInWhileFirstBlock)
        {
            whileControlVarAssignNum = 2;
        }
        popNextLexer();
        popNextLexer();
        return std::make_shared<StmtNode>(StmtNode::continueStmt());
    }
    else if (nowPointerToken->getSym() == TokenType::IF_TK)
    {
        if (openFolder)
        {
            isLastValueGetableAssignOrDecl = false;
        }
        popNextLexer();
        popNextLexer(); // LPAREN
        auto cond = analyzeCond();
        popNextLexer(); // RPAREN
        auto tmp = isInDirectWhile;
        if (openFolder)
        {
            isInDirectWhile = false;
        }
        auto ifBranchStmt = analyzeStmt(isInWhileFirstBlock);
        if (nowPointerToken->getSym() == TokenType::ELSE_TK)
        {
            popNextLexer();
            auto elseBranchStmt = analyzeStmt(isInWhileFirstBlock);
            if (openFolder)
            {
                isInDirectWhile = tmp;
            }
            return std::make_shared<StmtNode>(StmtNode::ifStmt(cond, ifBranchStmt, elseBranchStmt));
        }
        if (openFolder)
        {
            isInDirectWhile = tmp;
        }
        return std::make_shared<StmtNode>(StmtNode::ifStmt(cond, ifBranchStmt));
    }
    else if (nowPointerToken->getSym() == TokenType::WHILE_TK)
    {
        return analyzeWhile();
    }
    else if (nowPointerToken->getSym() == TokenType::RETURN_TK)
    {
        if (openFolder)
        {
            isLastValueGetableAssignOrDecl = false;
        }
        popNextLexer();
        if (nowPointerToken->getSym() == TokenType::SEMICOLON)
        {
            popNextLexer();
            return std::make_shared<StmtNode>(StmtNode::returnStmt());
        }
        auto exp = analyzeExp();
        popNextLexer();
        return std::make_shared<StmtNode>(StmtNode::returnStmt(exp));
    }
    else if (nowPointerToken->getSym() == TokenType::SEMICOLON)
    {
        popNextLexer();
        return std::make_shared<StmtNode>(StmtNode::emptyStmt());
    }
    else if (isAssign())
    {
        if (openFolder && !lockingOpenFolder)
        {
            getAssignLVal = true;
            isLastValueGetableAssignOrDecl = true;
        }
        if (openFolder && lockingOpenFolder)
        {
            judgeWhetherAssignLValWhileControlVar = true;
        }
        auto lVal = analyzeLVal();
        if (openFolder && lockingOpenFolder)
        {
            judgeWhetherAssignLValWhileControlVar = false;
            if (isAssignLValWhileControlVar)
            {
                whileControlVarAssignNum += 1;
                if (!isInDirectWhile)
                {
                    whileControlVarAssignNum += 1;
                }
            }
        }
        popNextLexer(); // ASSIGN
        auto exp = analyzeExp();
        if (openFolder && !lockingOpenFolder)
        {
            if (!isExpConstOrIMM(exp))
            {
                isLastValueGetableAssignOrDecl = false;
            }
        }
        popNextLexer();
        auto assignStmt = std::make_shared<StmtNode>(StmtNode::assignStmt(lVal, exp));
        if (openFolder && lockingOpenFolder && isAssignLValWhileControlVar)
        {
            whileBlockAssignStmt = assignStmt;
        }
        return assignStmt;
    }
    else
    {
        if (openFolder)
        {
            isLastValueGetableAssignOrDecl = false;
        }
        auto exp = analyzeExp();
        popNextLexer();
        return std::make_shared<StmtNode>(StmtNode::expStmt(exp));
    }
}

/**
 * @brief Exp -> AddExp
 * @details Calling Exp, return AddExp.
 * @return
 */
// Exp -> AddExp
std::shared_ptr<ExpNode> analyzeExp()
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeExp--------\n";
    }
    return analyzeAddExp();
}

// AddExp -> MulExp { ('+' | '−') MulExp }
std::shared_ptr<AddExpNode> analyzeAddExp()
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeAddExp--------\n";
    }
    auto mulExp = analyzeMulExp();
    auto addExp = std::make_shared<AddExpNode>(mulExp);
    while (nowPointerToken->getSym() == TokenType::MINUS || nowPointerToken->getSym() == TokenType::PLUS)
    {
        string op = nowPointerToken->getSym() == TokenType::MINUS ? "-" : "+";
        popNextLexer();
        mulExp = analyzeMulExp();
        addExp = std::make_shared<AddExpNode>(mulExp, addExp, op);
    }
    return addExp;
}

// MulExp -> UnaryExp { ('*' | '/' | '%') UnaryExp }
std::shared_ptr<MulExpNode> analyzeMulExp()
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeMulExp--------\n";
    }
    auto unaryExp = analyzeUnaryExp();
    auto relExp = std::make_shared<MulExpNode>(unaryExp);
    while (nowPointerToken->getSym() == TokenType::MULT || nowPointerToken->getSym() == TokenType::DIV ||
           nowPointerToken->getSym() == TokenType::REMAIN)
    {
        string op = nowPointerToken->getSym() == TokenType::MULT ? "*" : nowPointerToken->getSym() == TokenType::DIV ? "/"
                                                                                                                     : "%";
        popNextLexer();
        unaryExp = analyzeUnaryExp();
        relExp = std::make_shared<MulExpNode>(unaryExp, relExp, op);
    }
    return relExp;
}

// UnaryExp -> IdentUsage '(' [FuncRParams] ')'  |  ('+' | '−' | '!') UnaryExp  |  PrimaryExp
std::shared_ptr<UnaryExpNode> analyzeUnaryExp()
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeUnaryExp--------\n";
    }
    if (nowPointerToken->getSym() == TokenType::IDENT && peekNextLexer(1)->getSym() == TokenType::LPAREN)
    {
        auto ident = getIdentUsage(true);
        string op = "+";
        popNextLexer(); // LPAREN
        if (nowPointerToken->getSym() == TokenType::RPAREN)
        {
            popNextLexer();
            return std::make_shared<UnaryExpNode>(UnaryExpNode::funcCallUnaryExp(ident, op));
        }
        auto funcRParams = analyzeFuncRParams();
        popNextLexer(); // RPAREN
        return std::make_shared<UnaryExpNode>(UnaryExpNode::funcCallUnaryExp(ident, funcRParams, op));
    }
    else if (nowPointerToken->getSym() == TokenType::PLUS || nowPointerToken->getSym() == TokenType::MINUS ||
             nowPointerToken->getSym() == TokenType::NOT)
    {
        string op = nowPointerToken->getSym() == TokenType::PLUS ? "+" : nowPointerToken->getSym() == TokenType::MINUS ? "-"
                                                                                                                       : "!";
        popNextLexer();
        auto unaryExp = analyzeUnaryExp();
        return std::make_shared<UnaryExpNode>(UnaryExpNode::unaryUnaryExp(unaryExp, op));
    }
    else
    {
        string op = "+";
        auto primaryExp = analyzePrimaryExp();
        return std::make_shared<UnaryExpNode>(UnaryExpNode::primaryUnaryExp(primaryExp, op));
    }
}

// PrimaryExp -> '(' Exp ')'  |  IntConst  |  LVal
std::shared_ptr<PrimaryExpNode> analyzePrimaryExp()
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzePrimaryExp--------\n";
    }
    if (nowPointerToken->getSym() == TokenType::LPAREN)
    {
        popNextLexer(); // LPAREN
        auto exp = analyzeExp();
        popNextLexer(); // RPAREN
        return std::make_shared<PrimaryExpNode>(PrimaryExpNode::parentExp(exp));
    }
    else if (nowPointerToken->getSym() == TokenType::INTCONST)
    {
        int value = nowPointerToken->getValue();
        popNextLexer();
        return std::make_shared<PrimaryExpNode>(PrimaryExpNode::numberExp(value));
    }
    else
    {
        auto lVal = analyzeLVal();
        return std::make_shared<PrimaryExpNode>(PrimaryExpNode::lValExp(lVal));
    }
}

// LVal -> IdentUsage
std::shared_ptr<LValNode> analyzeLVal()
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeLVal--------\n";
    }
    auto ident = getIdentUsage(false);
    if (ident->ident->dimension == 0)
    {
        return std::make_shared<LValNode>(ident);
    }
    return std::make_shared<LValNode>(ident, ident->ident->dimension, ident->ident->expressionOfEachDimension);
}

// CompUnit -> Decl | FuncDef
std::shared_ptr<CompUnitNode> analyzeCompUnit()
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeCompUnit--------\n";
    }
    vector<shared_ptr<DeclNode>> declList;
    vector<shared_ptr<FuncDefNode>> funcDefList;
    while (nowPointerToken->getSym() != TokenType::END)
    {
        if (peekNextLexer(2)->getSym() == TokenType::LPAREN)
        {
            funcDefList.push_back(analyzeFuncDef());
        }
        else
        {
            declList.push_back(analyzeDecl());
        }
    }
    for (const auto &symbolTableItem : symbolTable[{0, 1}]->symbolTableThisBlock)
    {
        if (symbolTableItem.second->isVarSingleUseInUnRecursionFunction())
        {
            usageNameListOfVarSingleUseInUnRecursionFunction[symbolTableItem.second->usageName] = symbolTableItem.second->eachFunc[0]->usageName;
        }
    }
    return std::make_shared<CompUnitNode>(declList, funcDefList);
}

// FuncRParams -> Exp { ',' Exp }
std::shared_ptr<FuncRParamsNode> analyzeFuncRParams()
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeFuncRParams--------\n";
    }
    vector<shared_ptr<ExpNode>> exps;
    auto exp = analyzeExp();
    exps.push_back(exp);
    while (nowPointerToken->getSym() == TokenType::COMMA)
    {
        popNextLexer(); // COMMA
        exp = analyzeExp();
        exps.push_back(exp);
    }
    return std::make_shared<FuncRParamsNode>(exps);
}

// Cond -> LOrExp
std::shared_ptr<CondNode> analyzeCond()
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeCond--------\n";
    }
    auto lOrExp = analyzeLOrExp();
    auto condExp = std::make_shared<CondNode>(lOrExp);
    if (openFolder)
    {
        changeCondDivideIntoMul(condExp);
    }
    return condExp;
}

// LOrExp -> LAndExp { '||' LAndExp }
std::shared_ptr<LOrExpNode> analyzeLOrExp()
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeLOrExp--------\n";
    }
    auto lAndExp = analyzeLAndExp();
    auto lOrExp = std::make_shared<LOrExpNode>(lAndExp);
    while (nowPointerToken->getSym() == TokenType::OR)
    {
        string op = "||";
        popNextLexer();
        lAndExp = analyzeLAndExp();
        lOrExp = std::make_shared<LOrExpNode>(lAndExp, lOrExp, op);
    }
    return lOrExp;
}

// LAndExp -> EqExp { '&&' EqExp }
std::shared_ptr<LAndExpNode> analyzeLAndExp()
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeLAndExp--------\n";
    }
    auto eqExp = analyzeEqExp();
    auto lAndExp = std::make_shared<LAndExpNode>(eqExp);
    while (nowPointerToken->getSym() == TokenType::AND)
    {
        string op = "&&";
        popNextLexer();
        eqExp = analyzeEqExp();
        lAndExp = std::make_shared<LAndExpNode>(eqExp, lAndExp, op);
    }
    return lAndExp;
}

// EqExp -> RelExp { ('==' | '!=') RelExp }
std::shared_ptr<EqExpNode> analyzeEqExp()
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeEqExp--------\n";
    }
    auto relExp = analyzeRelExp();
    auto eqExp = std::make_shared<EqExpNode>(relExp);
    while (nowPointerToken->getSym() == TokenType::EQUAL || nowPointerToken->getSym() == TokenType::NEQUAL)
    {
        string op = nowPointerToken->getSym() == TokenType::EQUAL ? "==" : "!=";
        popNextLexer();
        relExp = analyzeRelExp();
        eqExp = std::make_shared<EqExpNode>(relExp, eqExp, op);
    }
    return eqExp;
}

// RelExp -> AddExp { ('<' | '>' | '<=' | '>=') AddExp }
std::shared_ptr<RelExpNode> analyzeRelExp()
{
    if (_debugSyntax)
    {
        std::cout << "--------analyzeRelExp--------\n";
    }
    auto addExp = analyzeAddExp();
    auto relExp = std::make_shared<RelExpNode>(addExp);
    while (nowPointerToken->getSym() == TokenType::LARGE || nowPointerToken->getSym() == TokenType::LAQ ||
           nowPointerToken->getSym() == TokenType::LESS || nowPointerToken->getSym() == TokenType::LEQ)
    {
        if (openFolder && isInWhileLockedCond)
        {
            relation = nowPointerToken->getSym();
            isInWhileLockedCond = false;
        }
        string op = nowPointerToken->getSym() == TokenType::LARGE ? ">" : nowPointerToken->getSym() == TokenType::LAQ ? ">="
                                                                      : nowPointerToken->getSym() == TokenType::LESS  ? "<"
                                                                                                                      : "<=";
        popNextLexer();
        addExp = analyzeAddExp();
        relExp = std::make_shared<RelExpNode>(addExp, relExp, op);
    }
    return relExp;
}

// 开始语法分析
std::shared_ptr<CompUnitNode> syntaxAnalyze()
{
    nowPointerToken = &tokenInfoList[0];
    auto retFuncType = SymbolType::RET_FUNC;
    auto voidFuncType = SymbolType::VOID_FUNC;
    // 添加SysY语法自带函数
    string retNames[] = {"getint", "getch", "getarray"};
    string voidNames[] = {"putint", "putch", "putarray", "putf", "starttime", "stoptime"};
    for (string &retName : retNames)
    {
        auto retFunc = std::make_shared<SymbolTableItem>(retFuncType, retName, nowLayId);
        insertSymbol(nowLayId, retFunc);
    }
    for (string &voidName : voidNames)
    {
        auto voidFunc = std::make_shared<SymbolTableItem>(voidFuncType, voidName, nowLayId);
        insertSymbol(nowLayId, voidFunc);
    }
    return analyzeCompUnit();
}
