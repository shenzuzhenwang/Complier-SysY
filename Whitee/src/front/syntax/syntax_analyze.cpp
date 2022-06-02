/*********************************************************************
 * @file   syntax_analyze.cpp
 * @brief  语法分析
 * CompUnit -> Decl | FuncDef
 * Decl -> ConstDecl | VarDecl
 * ConstDecl -> 'const' 'int' ConstDef { ',' ConstDef } ';'
 * ConstDef -> IdentDef '=' ConstInitVal
 * ConstInitVal -> ConstExp  |  ConstInitValArr
 * ConstInitValArr -> '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
 * VarDecl -> 'int' VarDef { ',' VarDef } ';'
 * VarDef -> IdentDefine [ '=' InitVal ]
 * InitVal -> ConstExp  |  Exp  |  ConstInitValArr  |  InitValArr
 * InitValArr -> '{' [ Exp { ',' Exp } ] '}'
 * FuncDef -> ('int','void') IdentDefine '(' [FuncFParams] ')' Block
 * FuncFParams -> FuncFParam { ',' FuncFParam }
 * FuncFParam -> 'int' IdentDefineInFunction
 * Block -> '{' { BlockItem } '}'
 * BlockItem -> Decl | Stmt
 * Stmt -> LVal '=' Exp ';'  |  [Exp] ';'  |  Block  |  'if' '( Cond ')' Stmt [ 'else' Stmt ]  |  While  |  'break' ';'  |  'continue' ';'  |  'return' [Exp] ';'
 * Exp -> AddExp
 * Cond -> LOrExp
 * LVal -> IdentUsage
 * PrimaryExp -> '(' Exp ')'  |  IntConst  |  LVal
 * UnaryExp -> PrimaryExp  |  IdentUsage '(' [FuncRParams] ')'  |  ('+' | '−' | '!') UnaryExp
 * FuncRParams -> Exp { ',' Exp }
 * MulExp -> UnaryExp { ('*' | '/' | '%') UnaryExp }
 * AddExp -> MulExp { ('+' | '−') MulExp }
 * RelExp -> AddExp { ('<' | '>' | '<=' | '>=') AddExp }
 * EqExp -> RelExp { ('==' | '!=') RelExp }
 * LAndExp -> EqExp { '&&' EqExp }
 * LOrExp -> LAndExp { '||' LAndExp }
 * 
 * @author 神祖
 * @date   June 2022
 *********************************************************************/
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
 * @details 将得到的值存储在符号表中。
 * 对于下一步，将所有的const var替换成一个单一的int值。
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
shared_ptr<ConstInitValNode> analyzeConstInitVal(const shared_ptr<SymbolTableItem> &ident);
// ConstInitValArr -> '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
shared_ptr<ConstInitValArrNode> analyzeConstInitValArr(int dimension, vector<int> &numOfEachDimension);
// VarDecl -> 'int' VarDef { ',' VarDef } ';'
shared_ptr<VarDeclNode> analyzeVarDecl();
// VarDef -> IdentDefine [ '=' InitVal ]
shared_ptr<VarDefNode> analyzeVarDef();
// InitVal -> ConstExp  |  Exp  |  ConstInitValArr  |  InitValArr
void analyzeInitVal(const shared_ptr<SymbolTableItem> &ident);
// InitValArr -> '{' [ Exp { ',' Exp } ] '}'
shared_ptr<InitValArrNode> analyzeInitValArr(int dimension, vector<int> &numOfEachDimension);
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

// 语法分析中词法的序号
static int lexerPosition = 0;

//  当前的语法分析对象
static TokenInfo *nowPointerToken;

// 前进到下一个词
void popNextLexer()
{
    if (nowPointerToken->getSym() == TokenType::END)
    {
        cerr << "Out of lexer vector range." << endl;
    }
    nowPointerToken = &tokenInfoList[++lexerPosition];
    if (_debugSyntax)
    {
        if (tokenInfoList[lexerPosition - 1].getSym() == TokenType::INTCONST)
        {
            cout << tokenInfoList[lexerPosition - 1].getValue() << '\n';
        }
        else
        {
            cout << tokenInfoList[lexerPosition - 1].getName() << '\n';
        }
    }
}

// 查看下num个词
TokenInfo *peekNextLexer(int num)
{
    if (lexerPosition + num > tokenInfoList.size())
    {
        cerr << "Out of lexer vector range." << endl;
    }
    return &tokenInfoList[lexerPosition + num];
}

static shared_ptr<SymbolTableItem> nowFuncSymbol;  // 当前所在的函数对象
unordered_map<string, string> usageNameListOfVarSingleUseInUnRecursionFunction;  // 在非递归函数中单次使用变量名称列表

//static bool isInWhileLockedCond = false;
//static int step = 0;
//static bool folderShouldBeOpen = false;
//static int folderOpenNum = 0;
//static bool isInDirectWhile = false;
//static shared_ptr<StmtNode> whileBlockAssignStmt;
//static bool lockingOpenFolder = false;
//static bool isAssignLValWhileControlVar = false;
//static bool judgeWhetherAssignLValWhileControlVar = false;
//static int whileControlVarAssignNum = 0;
//static shared_ptr<SymbolTableItem> lastAssignOrDeclVar;
//static bool isLastValueGetableAssignOrDecl = false;
//static bool getAssignLVal = false;
static int lastValue = 0;

//bool isExpConstOrIMM(shared_ptr<ExpNode> exp)
//{
//    auto addExp = s_p_c<AddExpNode>(exp);
//    if (addExp->addExp)
//    {
//        return false;
//    }
//    auto mulExp = addExp->mulExp;
//    if (mulExp->mulExp)
//    {
//        return false;
//    }
//    auto unaryExp = mulExp->unaryExp;
//    bool isOpPlus = true;
//    while (unaryExp->type == UnaryExpType::UNARY_DEEP)
//    {
//        if (unaryExp->op == "-")
//        {
//            isOpPlus = !isOpPlus;
//        }
//        unaryExp = unaryExp->unaryExp;
//    }
//    if (unaryExp->type == UnaryExpType::UNARY_PRIMARY)
//    {
//        auto primaryExp = unaryExp->primaryExp;
//        if (primaryExp->type == PrimaryExpType::PRIMARY_NUMBER)
//        {
//            lastValue = isOpPlus ? primaryExp->number : -primaryExp->number;
//            return true;
//        }
//        if (primaryExp->type == PrimaryExpType::PRIMARY_L_VAL)
//        {
//            auto lValExp = primaryExp->lVal;
//            if (lValExp->ident->ident->symbolType == SymbolType::CONST_VAR)
//            {
//                lastValue = s_p_c<ConstInitValValNode>(lValExp->ident->ident->constInitVal)->value;
//                lastValue = isOpPlus ? lastValue : -lastValue;
//                return true;
//            }
//        }
//    }
//    return false;
//}

/**
 * Symbol Table:
 */
unordered_map<pair<int, int>, shared_ptr<SymbolTablePerBlock>, pair_hash> symbolTable; // 块ID <--> 块内对象

unordered_map<int, int> blockLayerId2LayerNum; // 第i层的最大块ID  记录每一层的编号  <2, ...> if ...max is 2, record <2, 2>, next layer2 block id is <2, 3>

static int nowLayer = 0;  // 块的层数

static pair<int, int> nowLayId = distributeBlockId(0, {0, 0}); // 目前的块ID
static pair<int, int> layIdInFuncFParams;                      // 记录函数形参的块ID，不用多次使用
static pair<int, int> globalLayId = {0, 1};                    // 全局变量块

bool isGlobalBlock(pair<int, int> layId)
{
    return globalLayId == layId;
}

 /**
  * @brief get a ident   IdentDef -> Ident { '[' ConstExp ']' }
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
shared_ptr<IdentNode> getIdentDefine(bool isF, bool isVoid, bool isConst)
{
    if (_debugSyntax)
    {
        cout << "--------getIdentDefine--------\n";
    }
    string name = nowPointerToken->getName();
    popNextLexer();
    if (isF)
    {
        SymbolType symbolType = isVoid ? SymbolType::VOID_FUNC : SymbolType::RET_FUNC;
        auto symbolTableItem = make_shared<SymbolTableItem>(symbolType, name, nowLayId);
        nowFuncSymbol = symbolTableItem;
        insertSymbol(nowLayId, symbolTableItem);
        return make_shared<IdentNode>(symbolTableItem);
    }
    int dimension = 0;
    vector<int> numOfEachDimension;
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
        auto symbolTableItem = make_shared<SymbolTableItem>(symbolType, name, nowLayId);
        //if (openFolder && !lockingOpenFolder)
        //{
        //    lastAssignOrDeclVar = symbolTableItem;
        //}
        insertSymbol(nowLayId, symbolTableItem);
        return make_shared<IdentNode>(symbolTableItem);
    }
    else
    {
        SymbolType symbolType = (isConst ? SymbolType::CONST_ARRAY : SymbolType::ARRAY);
        auto symbolTableItem = make_shared<SymbolTableItem>(symbolType, dimension, numOfEachDimension, name,
                                                                 nowLayId);
        insertSymbol(nowLayId, symbolTableItem);
        return make_shared<IdentNode>(symbolTableItem);
    }
}

/**
 * @brief 在函数中对ident的定义  IdentDefineInFunction -> [ '[' ']' { '[' ConstExp ']' } ]
 * 对于维度>0的身份是一个[][*][*]。
 * 应该考虑第一个[]。
 * @return identNode
 */
shared_ptr<IdentNode> getIdentDefineInFunction()
{
    if (_debugSyntax)
    {
        cout << "--------getIdentDefine--------\n";
    }
    string name = nowPointerToken->getName();
    popNextLexer();
    int dimension = 0;
    vector<int> numOfEachDimension;  // 数组每个维数的大小
    if (nowPointerToken->getSym() == TokenType::LBRACKET)  // 有中括号，为数组
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
        auto symbolTableItem = make_shared<SymbolTableItem>(symbolType, name, nowLayId);
        insertSymbol(nowLayId, symbolTableItem);
        return make_shared<IdentNode>(symbolTableItem);
    }
    else
    {
        SymbolType symbolType = (SymbolType::ARRAY);
        auto symbolTableItem = make_shared<SymbolTableItem>(symbolType, dimension, numOfEachDimension, name, nowLayId);
        insertSymbol(nowLayId, symbolTableItem);
        return make_shared<IdentNode>(symbolTableItem);
    }
}

 /**
  * @brief 使用一个标识符 IdentUsage -> Ident  |  Ident {'[' Exp ']'}
  * @details
  * !!!当 !isF时，应该复制 symbolTableItem, 因为 expressionOfPerDimension 在 symbolTableItem 中。
  * 当使用shared_ptr时，所有的是共享的。在一个地方改变，在另一个地方改变，会导致错误。
  * 应该复制一个新的。
  * 不需要函数。
  * @param isF 标识符是函数
  * @return IdentNode
  */
shared_ptr<IdentNode> getIdentUsage(bool isF)
{
    if (_debugSyntax)
    {
        cout << "--------getIdentUsage--------\n";
    }
    string name = nowPointerToken->getName();
    popNextLexer();
    auto symbolInTable = findSymbol(nowLayId, name, isF);
    if (symbolInTable->eachFuncUseNum.find(nowFuncSymbol->usageName) == symbolInTable->eachFuncUseNum.end())
    {
        symbolInTable->eachFuncUseNum[nowFuncSymbol->usageName] = 0;
        symbolInTable->eachFunc.push_back(nowFuncSymbol);
    }
    symbolInTable->eachFuncUseNum[nowFuncSymbol->usageName]++;
    //if (openFolder && getAssignLVal && !lockingOpenFolder)
    //{
    //    lastAssignOrDeclVar = symbolInTable;
    //    getAssignLVal = false;
    //}
    //if (openFolder && lockingOpenFolder && judgeWhetherAssignLValWhileControlVar)
    //{
    //    isAssignLValWhileControlVar = (symbolInTable == lastAssignOrDeclVar);
    //    judgeWhetherAssignLValWhileControlVar = false;
    //}
    if (isF)
    {
        unordered_set<string> sysFunc({"getint", "getch", "getarray", "putint", "putch", "putarray", "putf", "starttime", "stoptime"});

        if (nowFuncSymbol == symbolInTable)
        {
            symbolInTable->isRecursion = true;
        }
        //if (openFolder)
        //{
        //    if (sysFunc.find(nowFuncSymbol->name) == sysFunc.end())
        //    {
        //        whileControlVarAssignNum += 2;
        //    }
        //}
        return make_shared<IdentNode>(make_shared<SymbolTableItem>(symbolInTable->symbolType, name, symbolInTable->blockId));
    }
    else
    {
        vector<shared_ptr<ExpNode>> expressionOfEachDimension;
        while (nowPointerToken->getSym() == TokenType::LBRACKET)
        {
            popNextLexer(); // LBRACKET
            expressionOfEachDimension.push_back(analyzeExp());
            popNextLexer(); // RBRACKET
        }
        shared_ptr<SymbolTableItem> symbolInIdent(new SymbolTableItem(symbolInTable->symbolType, symbolInTable->dimension,
                                expressionOfEachDimension, name, symbolInTable->blockId));
        if (symbolInTable->symbolType == SymbolType::CONST_ARRAY || symbolInTable->symbolType == SymbolType::CONST_VAR)
        {
            symbolInIdent->constInitVal = symbolInTable->constInitVal;
        }
        symbolInIdent->numOfEachDimension = symbolInTable->numOfEachDimension;
        return make_shared<IdentNode>(symbolInIdent);
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
        cout << "--------analyzeDecl--------\n";
    }
    shared_ptr<DeclNode> declNode(nullptr);
    //if (openFolder)
    //{
    //    whileControlVarAssignNum += 2;
    //}
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
        cout << "--------analyzeConstDecl--------\n";
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
    return make_shared<ConstDeclNode>(constDefList);
}

// ConstDef -> IdentDef '=' ConstInitVal
shared_ptr<ConstDefNode> analyzeConstDef()
{
    if (_debugSyntax)
    {
        cout << "--------analyzeConstDef--------\n";
    }
    auto ident = getIdentDefine(false, false, true);
    popNextLexer(); // ASSIGN
    auto constInitVal = analyzeConstInitVal(ident->ident);
    return make_shared<ConstDefNode>(ident, constInitVal);
}

/**
 * @brief 一个const对象 ConstInitVal -> ConstExp  |  ConstInitValArr
 * @param ident是一个SymbolTableItem，用它来获得这个const var的尺寸。
 * 判断constInitVal的类型。
 * 1. ConstInitValValNode
 * 2. ConstInitValAryNode
 * @return ConstInitValNode
 */
shared_ptr<ConstInitValNode> analyzeConstInitVal(const shared_ptr<SymbolTableItem> &ident)
{
    if (_debugSyntax)
    {
        cout << "--------analyzeInitVal--------\n";
    }
    if (ident->dimension == 0)
    {
        int value = getConstExp();
        ident->constInitVal = make_shared<ConstInitValValNode>(value);
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
shared_ptr<ConstInitValArrNode> constZeroDefaultArr(int dimension, vector<int> &numOfEachDimension)
{
    int thisDimensionNum = numOfEachDimension[dimension];
    vector<shared_ptr<ConstInitValNode>> valueList;
    if (dimension + 1 == numOfEachDimension.size())
    {
        for (int i = 0; i < thisDimensionNum; i++)
        {
            int exp = 0;
            valueList.push_back(make_shared<ConstInitValValNode>(exp));
        }
    }
    else
    {
        for (int i = 0; i < thisDimensionNum; i++)
        {
            valueList.push_back(constZeroDefaultArr(dimension + 1, numOfEachDimension));
        }
    }
    return make_shared<ConstInitValArrNode>(thisDimensionNum, valueList);
}

/**
 * @brief 获取一个const数组的初始化.  ConstInitValArr -> '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
 * @details Dimension是一个重要的参数。
 * 对于这个函数来说，它是用于递归的。
 * Dimension = 1意味着Arr是一维的。
 * 数组中的项目应该是ConstInitValValNode而不是ConstInitValArrNode。
 */
shared_ptr<ConstInitValArrNode> analyzeConstInitValArr(int dimension, vector<int> &numOfEachDimension)
{
    if (_debugSyntax)
    {
        cout << "--------analyzeConstInitValArr--------\n";
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
                    valueList.push_back(make_shared<ConstInitValValNode>(exp));
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
                valueList.push_back(make_shared<ConstInitValValNode>(exp));
                if (nowPointerToken->getSym() == TokenType::COMMA)
                {
                    popNextLexer(); // COMMA
                }
            }
        }
        return make_shared<ConstInitValArrNode>(thisDimensionNum, valueList);
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
    return make_shared<ConstInitValArrNode>(thisDimensionNum, valueList);
}

// ConstExp -> MulExp
int getConstExp()
{
    if (_debugSyntax)
    {
        cout << "--------getConstExp--------\n";
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
        cout << "--------getMulExp--------\n";
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
        cout << "--------getUnaryExp--------\n";
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
        cout << "--------getConstVarExp--------\n";
    }
    string name = nowPointerToken->getName();
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
shared_ptr<VarDeclNode> analyzeVarDecl()
{
    if (_debugSyntax)
    {
        cout << "--------analyzeVarDecl--------\n";
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
    return make_shared<VarDeclNode>(varDefList);
}

// VarDef -> IdentDefine [ '=' InitVal ]
shared_ptr<VarDefNode> analyzeVarDef()
{
    if (_debugSyntax)
    {
        cout << "--------analyzeVarDef--------\n";
    }
    auto ident = getIdentDefine(false, false, false);
    bool hasAssigned = false;
    if (nowPointerToken->getSym() == TokenType::ASSIGN)
    {
        hasAssigned = true;
        popNextLexer(); // ASSIGN
        analyzeInitVal(ident->ident);
        shared_ptr<InitValNode> initVal;
        if (!isGlobalBlock(ident->ident->blockId))
        {
            initVal = ident->ident->initVal;
            if (ident->ident->dimension == 0)
            {
                return make_shared<VarDefNode>(ident, initVal);
            }
            else
            {
                return make_shared<VarDefNode>(ident, ident->ident->dimension, ident->ident->numOfEachDimension, initVal);
            }
        }
    }
    if (!hasAssigned && isGlobalBlock(ident->ident->blockId))
    {
        if (ident->ident->dimension == 0)
        {
            int value = 0;
            ident->ident->globalVarInitVal = make_shared<ConstInitValValNode>(value);
        }
    }
    if (ident->ident->dimension == 0)
    {
        return make_shared<VarDefNode>(ident);
    }
    else
    {
        return make_shared<VarDefNode>(ident, ident->ident->dimension, ident->ident->numOfEachDimension);
    }
}

// InitVal -> ConstExp  |  Exp  |  ConstInitValArr  |  InitValArr
void analyzeInitVal(const shared_ptr<SymbolTableItem> &ident)
{
    if (_debugSyntax)
    {
        cout << "--------analyzeInitVal--------\n";
    }
    if (ident->dimension == 0)
    {
        if (isGlobalBlock(ident->blockId))
        {
            int value = getConstExp();
            //if (openFolder && !lockingOpenFolder)
            //{
            //    lastValue = value;
            //    isLastValueGetableAssignOrDecl = true;
            //}
            ident->globalVarInitVal = make_shared<ConstInitValValNode>(value);
        }
        else
        {
            auto expNode = analyzeExp();
            //if (openFolder && isExpConstOrIMM(expNode) && !lockingOpenFolder)
            //{
            //    isLastValueGetableAssignOrDecl = true;
            //}
            ident->initVal = make_shared<InitValValNode>(expNode);
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
shared_ptr<InitValArrNode> zeroDefaultArr(int dimension, vector<int> &numOfEachDimension)
{
    int thisDimensionNum = numOfEachDimension[dimension];
    vector<shared_ptr<InitValNode>> valueList;
    if (dimension + 1 == numOfEachDimension.size())
    {
        for (int i = 0; i < thisDimensionNum; i++)
        {
            int value = 0;
            auto exp = make_shared<PrimaryExpNode>(PrimaryExpNode::numberExp(value));
            auto expExp = s_p_c<ExpNode>(exp);
            valueList.push_back(make_shared<InitValValNode>(expExp));
        }
    }
    else
    {
        for (int i = 0; i < thisDimensionNum; i++)
        {
            valueList.push_back(zeroDefaultArr(dimension + 1, numOfEachDimension));
        }
    }
    return make_shared<InitValArrNode>(thisDimensionNum, valueList);
}

// InitValArr -> '{' [ Exp { ',' Exp } ] '}'
shared_ptr<InitValArrNode> analyzeInitValArr(int dimension, vector<int> &numOfEachDimension)
{
    if (_debugSyntax)
    {
        cout << "--------analyzeInitValArr--------\n";
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
                    valueList.push_back(make_shared<InitValValNode>(exp));
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
                valueList.push_back(make_shared<InitValValNode>(exp));
                if (nowPointerToken->getSym() == TokenType::COMMA)
                {
                    popNextLexer(); // COMMA
                }
            }
        }
        return make_shared<InitValArrNode>(thisDimensionNum, valueList);
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
    return make_shared<InitValArrNode>(thisDimensionNum, valueList);
}

// FuncDef -> ('int','void') IdentDefine '(' [FuncFParams] ')' Block
shared_ptr<FuncDefNode> analyzeFuncDef()
{
    if (_debugSyntax)
    {
        cout << "--------analyzeFuncDef--------\n";
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
        return make_shared<FuncDefNode>(funcType, ident, funcFParams, block);
    }
    return make_shared<FuncDefNode>(funcType, ident, block);
}

// FuncFParams -> FuncFParam { ',' FuncFParam }
shared_ptr<FuncFParamsNode> analyzeFuncFParams()
{
    if (_debugSyntax)
    {
        cout << "--------analyzeFuncFParams--------\n";
    }
    vector<shared_ptr<FuncFParamNode>> funcParamList;
    funcParamList.push_back(analyzeFuncFParam());
    while (nowPointerToken->getSym() == TokenType::COMMA)
    {
        popNextLexer(); // COMMA
        funcParamList.push_back(analyzeFuncFParam());
    }
    return make_shared<FuncFParamsNode>(funcParamList);
}

// FuncFParam -> 'int' IdentDefineInFunction
shared_ptr<FuncFParamNode> analyzeFuncFParam()
{
    if (_debugSyntax)
    {
        cout << "--------analyzeFuncFParam--------\n";
    }
    popNextLexer(); // INT_TK
    auto ident = getIdentDefineInFunction();
    return make_shared<FuncFParamNode>(ident, ident->ident->dimension, ident->ident->numOfEachDimension);
}

/**
 * @brief 获取一个块的代码  Block -> '{' { BlockItem } '}'
 * @details 当进入一个区块时，应该注意到blockId。
 * 需要做的是添加层，并得到一个新的layerId。
 * 当从块中出来时，将layerId设置为现在的layer的父亲。
 * @param isFuncBlock 如果是FuncBlock，则使用layIdInFuncParam。因为它是在同一个块中。不要再生成一个块。
 * @return BlockNode
 */
shared_ptr<BlockNode> analyzeBlock(bool isFuncBlock, bool isVoid, bool isInWhileFirstBlock)
{
    if (_debugSyntax)
    {
        cout << "--------analyzeBlock--------\n";
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
        //if (openFolder && folderShouldBeOpen)
        //{
        //    for (int i = 0; i < folderOpenNum; i++)
        //    {
        //        blockItems.push_back(blockItem);
        //        ++itemCnt;
        //    }
        //    folderShouldBeOpen = false;
        //}
        //else
        //{
        //    blockItems.push_back(blockItem);
        //    ++itemCnt;
        //}
        blockItems.push_back(blockItem);
        ++itemCnt;
    }
    if (isVoid)
    {
        auto defaultReturn = make_shared<StmtNode>(StmtNode::returnStmt());
        blockItems.push_back(defaultReturn);
    }
    popNextLexer(); // RBRACE
    nowLayer--;
    nowLayId = fatherLayId;
    return make_shared<BlockNode>(itemCnt, blockItems);
}

// BlockItem -> Decl | Stmt
shared_ptr<BlockItemNode> analyzeBlockItem(bool isInWhileFirstBlock)
{
    if (_debugSyntax)
    {
        cout << "--------analyzeBlockItem--------\n";
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


//bool isSingleCondAddExpThisSymbol(shared_ptr<AddExpNode> addExp, shared_ptr<SymbolTableItem> symbol)
//{
//    if (addExp->addExp)
//    {
//        return false;
//    }
//    auto mulExp = addExp->mulExp;
//    if (mulExp->mulExp)
//    {
//        return false;
//    }
//    auto unaryExp = mulExp->unaryExp;
//    if (unaryExp->type == UnaryExpType::UNARY_PRIMARY)
//    {
//        auto primaryExp = unaryExp->primaryExp;
//        if (primaryExp->type == PrimaryExpType::PRIMARY_L_VAL)
//        {
//            auto lValExp = primaryExp->lVal;
//            if (lValExp->ident->ident->symbolType == SymbolType::VAR)
//            {
//                if (lValExp->ident->ident->name == symbol->name)
//                {
//                    return true;
//                }
//            }
//        }
//    }
//    return false;
//}
//static int whileControlVarEndValue = 0;
//static TokenType relation = TokenType::EQUAL;
//bool isCondSingleAndUseLastInitOrDeclVar(shared_ptr<CondNode> cond, shared_ptr<SymbolTableItem> symbol)
//{
//    auto lOrExp = cond->lOrExp;
//    if (lOrExp->lOrExp)
//    {
//        return false;
//    }
//    auto lAndExp = lOrExp->lAndExp;
//    if (lAndExp->lAndExp)
//    {
//        return false;
//    }
//    auto eqExp = lAndExp->eqExp;
//    if (eqExp->eqExp)
//    {
//        return false;
//    }
//    auto relExp = eqExp->relExp;
//    if (relExp->relExp && relExp->addExp)
//    {
//        if (relExp->relExp->relExp)
//        {
//            return false;
//        }
//        if (!isSingleCondAddExpThisSymbol(relExp->relExp->addExp, symbol))
//        {
//            return false;
//        }
//        if (!isExpConstOrIMM(relExp->addExp))
//        {
//            return false;
//        }
//        whileControlVarEndValue = lastValue;
//        return true;
//    }
//    return false;
//}
/**
 * @brief lVal已经被判断出来了
 * 只有whileControlVar的lVal赋值应该被记录并使用这个判断。
 * mulExp应该是一个 "num"。
 */
//bool isAssignStepStyle(shared_ptr<StmtNode> assignStmt)
//{
//    auto exp = assignStmt->exp;
//    auto addExp = dynamic_pointer_cast<AddExpNode>(exp);
//    bool isStepPlus = true;
//    if (!addExp->addExp)
//    {
//        return false;
//    }
//    if (addExp->op == "-")
//    {
//        isStepPlus = !isStepPlus;
//    }
//    auto mulExp = addExp->mulExp;
//    addExp = addExp->addExp;
//    if (mulExp->mulExp)
//    {
//        return false;
//    }
//    auto unaryExp = mulExp->unaryExp;
//    while (unaryExp->type == UnaryExpType::UNARY_DEEP)
//    {
//        if (unaryExp->op == "-")
//        {
//            isStepPlus = !isStepPlus;
//            unaryExp = unaryExp->unaryExp;
//        }
//    }
//    if (unaryExp->type != UnaryExpType::UNARY_PRIMARY)
//    {
//        return false;
//    }
//    auto primaryExp = unaryExp->primaryExp;
//    if (primaryExp->type == PrimaryExpType::PRIMARY_NUMBER)
//    {
//        step = isStepPlus ? primaryExp->number : -primaryExp->number;
//    }
//    else
//    {
//        return false;
//    }
//    if (addExp->addExp)
//    {
//        return false;
//    }
//    mulExp = addExp->mulExp;
//    if (mulExp->mulExp)
//    {
//        return false;
//    }
//    unaryExp = mulExp->unaryExp;
//    if (unaryExp->type != UnaryExpType::UNARY_PRIMARY)
//    {
//        return false;
//    }
//    primaryExp = unaryExp->primaryExp;
//    return primaryExp->type == PrimaryExpType::PRIMARY_L_VAL && primaryExp->lVal->ident->ident->usageName == lastAssignOrDeclVar->usageName;
//}

// While -> 'while' '(' Cond ')' Stmt
shared_ptr<StmtNode> analyzeWhile()
{
    int whileControlStartValue = lastValue;
    popNextLexer(); // While
    popNextLexer();
    //if (openFolder)
    //{
    //    isInWhileLockedCond = true;
    //}
    auto cond = analyzeCond();
    popNextLexer();
    shared_ptr<StmtNode> whileInsideStmt;
    //if (openFolder && !lockingOpenFolder && isLastValueGetableAssignOrDecl)
    //{
    //    isLastValueGetableAssignOrDecl = false;
    //    auto whileControlVar = lastAssignOrDeclVar;
    //    lockingOpenFolder = true;
    //    if (isCondSingleAndUseLastInitOrDeclVar(cond, whileControlVar))
    //    {
    //        whileControlVarAssignNum = 0;
    //        isInDirectWhile = true;
    //        whileInsideStmt = analyzeStmt(true);
    //        if (whileControlVarAssignNum == 1)
    //        {
    //            if (isAssignStepStyle(whileBlockAssignStmt))
    //            {
    //                if (relation == TokenType::LARGE || relation == TokenType::LEQ || relation == TokenType::LESS ||
    //                    relation == TokenType::LAQ)
    //                {
    //                    if (relation == TokenType::LARGE)
    //                    {
    //                        if (whileControlStartValue <= whileControlVarEndValue)
    //                        {
    //                            folderOpenNum = 0;
    //                            return whileInsideStmt;
    //                        }
    //                        else
    //                        {
    //                            folderOpenNum = (whileControlVarEndValue - whileControlStartValue) / step;
    //                        }
    //                    }
    //                    if (relation == TokenType::LAQ)
    //                    {
    //                        if (whileControlStartValue < whileControlVarEndValue)
    //                        {
    //                            folderOpenNum = 0;
    //                            return whileInsideStmt;
    //                        }
    //                        else
    //                        {
    //                            folderOpenNum = (whileControlVarEndValue - whileControlStartValue - 1) / step;
    //                        }
    //                    }
    //                    if (relation == TokenType::LESS)
    //                    {
    //                        if (whileControlStartValue > whileControlVarEndValue)
    //                        {
    //                            folderOpenNum = 0;
    //                            return whileInsideStmt;
    //                        }
    //                        else
    //                        {
    //                            folderOpenNum = (whileControlVarEndValue - whileControlStartValue) / step;
    //                        }
    //                    }
    //                    if (relation == TokenType::LEQ)
    //                    {
    //                        if (whileControlStartValue >= whileControlVarEndValue)
    //                        {
    //                            folderOpenNum = 0;
    //                            return whileInsideStmt;
    //                        }
    //                        else
    //                        {
    //                            folderOpenNum = (whileControlVarEndValue - whileControlStartValue + 1) / step;
    //                        }
    //                    }
    //                    isInDirectWhile = false;
    //                    lockingOpenFolder = false;
    //                    if (folderOpenNum >= 0 && folderOpenNum < 1000)
    //                    {
    //                        folderShouldBeOpen = true;
    //                        return whileInsideStmt;
    //                    }
    //                }
    //            }
    //        }
    //        isInDirectWhile = false;
    //        lockingOpenFolder = false;
    //        return make_shared<StmtNode>(StmtNode::whileStmt(cond, whileInsideStmt));
    //    }
    //    lockingOpenFolder = false;
    //}
    //auto tmp = isInDirectWhile;
    //if (openFolder)
    //{
    //    isLastValueGetableAssignOrDecl = false;
    //    isInDirectWhile = false;
    //}
    whileInsideStmt = analyzeStmt(false);
    //if (openFolder)
    //{
    //    isInDirectWhile = tmp;
    //}
    return make_shared<StmtNode>(StmtNode::whileStmt(cond, whileInsideStmt));
}

// Stmt -> Block  |  'break' ';'  |  'continue' ';'  |  'if' '( Cond ')' Stmt [ 'else' Stmt ]  |  While  |  'return' [Exp] ';'  |  ';'  |  LVal '=' Exp ';'  |  Exp
shared_ptr<StmtNode> analyzeStmt(bool isInWhileFirstBlock)
{
    if (_debugSyntax)
    {
        cout << "--------analyzeStmt--------\n";
    }
    if (nowPointerToken->getSym() == TokenType::LBRACE)
    {
        //if (openFolder)
        //{
        //    isLastValueGetableAssignOrDecl = false;
        //}
        auto blockNode = analyzeBlock(false, false, isInWhileFirstBlock);
        return make_shared<StmtNode>(StmtNode::blockStmt(blockNode));
    }
    else if (nowPointerToken->getSym() == TokenType::BREAK_TK)
    {
        //if (openFolder)
        //{
        //    isLastValueGetableAssignOrDecl = false;
        //}
        //if (openFolder && isInWhileFirstBlock)
        //{
        //    whileControlVarAssignNum = 2;
        //}
        popNextLexer();
        popNextLexer();
        return make_shared<StmtNode>(StmtNode::breakStmt());
    }
    else if (nowPointerToken->getSym() == TokenType::CONTINUE_TK)
    {
        //if (openFolder)
        //{
        //    isLastValueGetableAssignOrDecl = false;
        //}
        //if (openFolder && isInWhileFirstBlock)
        //{
        //    whileControlVarAssignNum = 2;
        //}
        popNextLexer();
        popNextLexer();
        return make_shared<StmtNode>(StmtNode::continueStmt());
    }
    else if (nowPointerToken->getSym() == TokenType::IF_TK)
    {
        //if (openFolder)
        //{
        //    isLastValueGetableAssignOrDecl = false;
        //}
        popNextLexer();
        popNextLexer(); // LPAREN
        auto cond = analyzeCond();
        popNextLexer(); // RPAREN

        //auto tmp = isInDirectWhile;
        //if (openFolder)
        //{
        //    isInDirectWhile = false;
        //}
        auto ifBranchStmt = analyzeStmt(isInWhileFirstBlock);
        if (nowPointerToken->getSym() == TokenType::ELSE_TK)
        {
            popNextLexer();
            auto elseBranchStmt = analyzeStmt(isInWhileFirstBlock);
            //if (openFolder)
            //{
            //    isInDirectWhile = tmp;
            //}
            return make_shared<StmtNode>(StmtNode::ifStmt(cond, ifBranchStmt, elseBranchStmt));
        }
        //if (openFolder)
        //{
        //    isInDirectWhile = tmp;
        //}
        return make_shared<StmtNode>(StmtNode::ifStmt(cond, ifBranchStmt));
    }
    else if (nowPointerToken->getSym() == TokenType::WHILE_TK)
    {
        return analyzeWhile();
    }
    else if (nowPointerToken->getSym() == TokenType::RETURN_TK)
    {
        //if (openFolder)
        //{
        //    isLastValueGetableAssignOrDecl = false;
        //}
        popNextLexer();
        if (nowPointerToken->getSym() == TokenType::SEMICOLON)
        {
            popNextLexer();
            return make_shared<StmtNode>(StmtNode::returnStmt());
        }
        auto exp = analyzeExp();
        popNextLexer();
        return make_shared<StmtNode>(StmtNode::returnStmt(exp));
    }
    else if (nowPointerToken->getSym() == TokenType::SEMICOLON)
    {
        popNextLexer();
        return make_shared<StmtNode>(StmtNode::emptyStmt());
    }
    else if (isAssign())
    {
        //if (openFolder && !lockingOpenFolder)
        //{
        //    getAssignLVal = true;
        //    isLastValueGetableAssignOrDecl = true;
        //}
        //if (openFolder && lockingOpenFolder)
        //{
        //    judgeWhetherAssignLValWhileControlVar = true;
        //}
        auto lVal = analyzeLVal();
        //if (openFolder && lockingOpenFolder)
        //{
        //    judgeWhetherAssignLValWhileControlVar = false;
        //    if (isAssignLValWhileControlVar)
        //    {
        //        whileControlVarAssignNum += 1;
        //        if (!isInDirectWhile)
        //        {
        //            whileControlVarAssignNum += 1;
        //        }
        //    }
        //}
        popNextLexer(); // ASSIGN
        auto exp = analyzeExp();
        //if (openFolder && !lockingOpenFolder)
        //{
        //    if (!isExpConstOrIMM(exp))
        //    {
        //        isLastValueGetableAssignOrDecl = false;
        //    }
        //}
        popNextLexer();
        auto assignStmt = make_shared<StmtNode>(StmtNode::assignStmt(lVal, exp));
        //if (openFolder && lockingOpenFolder && isAssignLValWhileControlVar)
        //{
        //    whileBlockAssignStmt = assignStmt;
        //}
        return assignStmt;
    }
    else
    {
        //if (openFolder)
        //{
        //    isLastValueGetableAssignOrDecl = false;
        //}
        auto exp = analyzeExp();
        popNextLexer();
        return make_shared<StmtNode>(StmtNode::expStmt(exp));
    }
}

// Exp -> AddExp
shared_ptr<ExpNode> analyzeExp()
{
    if (_debugSyntax)
    {
        cout << "--------analyzeExp--------\n";
    }
    return analyzeAddExp();
}

// AddExp -> MulExp { ('+' | '−') MulExp }
shared_ptr<AddExpNode> analyzeAddExp()
{
    if (_debugSyntax)
    {
        cout << "--------analyzeAddExp--------\n";
    }
    auto mulExp = analyzeMulExp();
    auto addExp = make_shared<AddExpNode>(mulExp);
    while (nowPointerToken->getSym() == TokenType::MINUS || nowPointerToken->getSym() == TokenType::PLUS)
    {
        string op = nowPointerToken->getSym() == TokenType::MINUS ? "-" : "+";
        popNextLexer();
        mulExp = analyzeMulExp();
        addExp = make_shared<AddExpNode>(mulExp, addExp, op);
    }
    return addExp;
}

// MulExp -> UnaryExp { ('*' | '/' | '%') UnaryExp }
shared_ptr<MulExpNode> analyzeMulExp()
{
    if (_debugSyntax)
    {
        cout << "--------analyzeMulExp--------\n";
    }
    auto unaryExp = analyzeUnaryExp();
    auto relExp = make_shared<MulExpNode>(unaryExp);
    while (nowPointerToken->getSym() == TokenType::MULT || nowPointerToken->getSym() == TokenType::DIV || nowPointerToken->getSym() == TokenType::REMAIN)
    {
        string op = nowPointerToken->getSym() == TokenType::MULT ? "*" : nowPointerToken->getSym() == TokenType::DIV ? "/" : "%";
        popNextLexer();
        unaryExp = analyzeUnaryExp();
        relExp = make_shared<MulExpNode>(unaryExp, relExp, op);
    }
    return relExp;
}

// UnaryExp -> IdentUsage '(' [FuncRParams] ')'  |  ('+' | '−' | '!') UnaryExp  |  PrimaryExp
shared_ptr<UnaryExpNode> analyzeUnaryExp()
{
    if (_debugSyntax)
    {
        cout << "--------analyzeUnaryExp--------\n";
    }
    if (nowPointerToken->getSym() == TokenType::IDENT && peekNextLexer(1)->getSym() == TokenType::LPAREN)
    {
        auto ident = getIdentUsage(true);
        string op = "+";
        popNextLexer(); // LPAREN
        if (nowPointerToken->getSym() == TokenType::RPAREN)
        {
            popNextLexer();
            return make_shared<UnaryExpNode>(UnaryExpNode::funcCallUnaryExp(ident, op));
        }
        auto funcRParams = analyzeFuncRParams();
        popNextLexer(); // RPAREN
        return make_shared<UnaryExpNode>(UnaryExpNode::funcCallUnaryExp(ident, funcRParams, op));
    }
    else if (nowPointerToken->getSym() == TokenType::PLUS || nowPointerToken->getSym() == TokenType::MINUS ||
             nowPointerToken->getSym() == TokenType::NOT)
    {
        string op = nowPointerToken->getSym() == TokenType::PLUS ? "+" : nowPointerToken->getSym() == TokenType::MINUS ? "-" : "!";
        popNextLexer();
        auto unaryExp = analyzeUnaryExp();
        return make_shared<UnaryExpNode>(UnaryExpNode::unaryUnaryExp(unaryExp, op));
    }
    else
    {
        string op = "+";
        auto primaryExp = analyzePrimaryExp();
        return make_shared<UnaryExpNode>(UnaryExpNode::primaryUnaryExp(primaryExp, op));
    }
}

// PrimaryExp -> '(' Exp ')'  |  IntConst  |  LVal
shared_ptr<PrimaryExpNode> analyzePrimaryExp()
{
    if (_debugSyntax)
    {
        cout << "--------analyzePrimaryExp--------\n";
    }
    if (nowPointerToken->getSym() == TokenType::LPAREN)
    {
        popNextLexer(); // LPAREN
        auto exp = analyzeExp();
        popNextLexer(); // RPAREN
        return make_shared<PrimaryExpNode>(PrimaryExpNode::parentExp(exp));
    }
    else if (nowPointerToken->getSym() == TokenType::INTCONST)
    {
        int value = nowPointerToken->getValue();
        popNextLexer();
        return make_shared<PrimaryExpNode>(PrimaryExpNode::numberExp(value));
    }
    else
    {
        auto lVal = analyzeLVal();
        return make_shared<PrimaryExpNode>(PrimaryExpNode::lValExp(lVal));
    }
}

// LVal -> IdentUsage
shared_ptr<LValNode> analyzeLVal()
{
    if (_debugSyntax)
    {
        cout << "--------analyzeLVal--------\n";
    }
    auto ident = getIdentUsage(false);
    if (ident->ident->dimension == 0)
    {
        return make_shared<LValNode>(ident);
    }
    return make_shared<LValNode>(ident, ident->ident->dimension, ident->ident->expressionOfEachDimension);
}

// CompUnit -> Decl | FuncDef
shared_ptr<CompUnitNode> analyzeCompUnit()
{
    if (_debugSyntax)
    {
        cout << "--------analyzeCompUnit--------\n";
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
    return make_shared<CompUnitNode>(declList, funcDefList);
}

// FuncRParams -> Exp { ',' Exp }
shared_ptr<FuncRParamsNode> analyzeFuncRParams()
{
    if (_debugSyntax)
    {
        cout << "--------analyzeFuncRParams--------\n";
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
    return make_shared<FuncRParamsNode>(exps);
}

// Cond -> LOrExp
shared_ptr<CondNode> analyzeCond()
{
    if (_debugSyntax)
    {
        cout << "--------analyzeCond--------\n";
    }
    auto lOrExp = analyzeLOrExp();
    auto condExp = make_shared<CondNode>(lOrExp);
    //if (openFolder)
    //{
    //    changeCondDivideIntoMul(condExp);
    //}
    return condExp;
}

// LOrExp -> LAndExp { '||' LAndExp }
shared_ptr<LOrExpNode> analyzeLOrExp()
{
    if (_debugSyntax)
    {
        cout << "--------analyzeLOrExp--------\n";
    }
    auto lAndExp = analyzeLAndExp();
    auto lOrExp = make_shared<LOrExpNode>(lAndExp);
    while (nowPointerToken->getSym() == TokenType::OR)
    {
        string op = "||";
        popNextLexer();
        lAndExp = analyzeLAndExp();
        lOrExp = make_shared<LOrExpNode>(lAndExp, lOrExp, op);
    }
    return lOrExp;
}

// LAndExp -> EqExp { '&&' EqExp }
shared_ptr<LAndExpNode> analyzeLAndExp()
{
    if (_debugSyntax)
    {
        cout << "--------analyzeLAndExp--------\n";
    }
    auto eqExp = analyzeEqExp();
    auto lAndExp = make_shared<LAndExpNode>(eqExp);
    while (nowPointerToken->getSym() == TokenType::AND)
    {
        string op = "&&";
        popNextLexer();
        eqExp = analyzeEqExp();
        lAndExp = make_shared<LAndExpNode>(eqExp, lAndExp, op);
    }
    return lAndExp;
}

// EqExp -> RelExp { ('==' | '!=') RelExp }
shared_ptr<EqExpNode> analyzeEqExp()
{
    if (_debugSyntax)
    {
        cout << "--------analyzeEqExp--------\n";
    }
    auto relExp = analyzeRelExp();
    auto eqExp = make_shared<EqExpNode>(relExp);
    while (nowPointerToken->getSym() == TokenType::EQUAL || nowPointerToken->getSym() == TokenType::NEQUAL)
    {
        string op = nowPointerToken->getSym() == TokenType::EQUAL ? "==" : "!=";
        popNextLexer();
        relExp = analyzeRelExp();
        eqExp = make_shared<EqExpNode>(relExp, eqExp, op);
    }
    return eqExp;
}

// RelExp -> AddExp { ('<' | '>' | '<=' | '>=') AddExp }
shared_ptr<RelExpNode> analyzeRelExp()
{
    if (_debugSyntax)
    {
        cout << "--------analyzeRelExp--------\n";
    }
    auto addExp = analyzeAddExp();
    auto relExp = make_shared<RelExpNode>(addExp);
    while (nowPointerToken->getSym() == TokenType::LARGE || nowPointerToken->getSym() == TokenType::LAQ ||
           nowPointerToken->getSym() == TokenType::LESS || nowPointerToken->getSym() == TokenType::LEQ)
    {
        //if (openFolder && isInWhileLockedCond)
        //{
        //    relation = nowPointerToken->getSym();
        //    isInWhileLockedCond = false;
        //}
        string op = nowPointerToken->getSym() == TokenType::LARGE ? ">" : nowPointerToken->getSym() == TokenType::LAQ ? ">="
            : nowPointerToken->getSym() == TokenType::LESS  ? "<" : "<=";
        popNextLexer();
        addExp = analyzeAddExp();
        relExp = make_shared<RelExpNode>(addExp, relExp, op);
    }
    return relExp;
}

/**
 * @brief 开始语法分析
 * @return 分析完成的结果CompUnitNode
 */
shared_ptr<CompUnitNode> syntaxAnalyze()
{
    nowPointerToken = &tokenInfoList[0];
    auto retFuncType = SymbolType::RET_FUNC;
    auto voidFuncType = SymbolType::VOID_FUNC;

    // 添加运行时函数
    string retNames[] = {"getint", "getch", "getarray"};
    string voidNames[] = {"putint", "putch", "putarray", "putf", "starttime", "stoptime"};
    for (string &retName : retNames)
    {
        auto retFunc = make_shared<SymbolTableItem>(retFuncType, retName, nowLayId);
        insertSymbol(nowLayId, retFunc);
    }
    for (string &voidName : voidNames)
    {
        auto voidFunc = make_shared<SymbolTableItem>(voidFuncType, voidName, nowLayId);
        insertSymbol(nowLayId, voidFunc);
    }
    return analyzeCompUnit();
}
