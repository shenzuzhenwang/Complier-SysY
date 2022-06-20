#ifndef COMPILER_LEXER_H
#define COMPILER_LEXER_H

#include <cwchar>
#include <string>
#include <vector>
using namespace std;

enum TokenType  // 词法类型
{
    IDENT,   // 标识符，函数或变量的名字
    INTCONST,  // 数字常量
    STRCONST,  // 字符串常量
    CONST_TK,  // const
    INT_TK,    // int
    VOID_TK,   // void
    IF_TK,     // if
    ELSE_TK,   // else
    WHILE_TK,  // while
    BREAK_TK,  // break
    CONTINUE_TK,// continue
    RETURN_TK, // return
    COMMA,     // ,
    SEMICOLON, // ;
    LBRACKET,  // [
    RBRACKET,  // ]
    LBRACE,    // {
    RBRACE,    // }
    LPAREN,    // (
    RPAREN,    // )
    ASSIGN,    // =
    PLUS,      // +
    MINUS,     // -
    NOT,       // !
    MULT,      // *
    DIV,       // /
    REMAIN,    // %
    LESS,      // <
    LARGE,     // >
    LEQ,       // <=
    LAQ,       // >=
    EQUAL,     // ==
    NEQUAL,    // !=
    AND,       // &&
    OR,        // ||
    END        // 结束符
};

class TokenInfo
{
private:
    TokenType symbol;
    string name;
    int value;

    // 不同的关键字，不同的type_token
    // which can save message specially and set interface for symbolTable
public:
    explicit TokenInfo(TokenType sym);

    TokenType getSym();

    string getName();

    [[nodiscard]] int getValue() const;

    void setName(string na);

    void setValue(int i);
};

extern vector<TokenInfo> tokenInfoList;

void parseSym(FILE *in);

bool isSpace();

bool isLetter();

bool isDigit();

bool isTab();

bool isNewline();

void clearToken();

void catToken();

int64_t strToInt(bool isHex, bool isOct);

bool lexicalAnalyze(const string &file);

#endif
