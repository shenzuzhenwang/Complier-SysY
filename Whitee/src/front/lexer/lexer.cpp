/*********************************************************************
 * @file   lexer.cpp
 * @brief  词法分析，将每个词标志为相应的token
 * 
 * @author 神祖
 * @date   May 2022
 *********************************************************************/
#include "lexer.h"
#include "../../basic/std/compile_std.h"

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <fstream>
using namespace std;

int c;  // 缓冲区
string token;  // 词法单元

vector<TokenInfo> tokenInfoList;  // 词法分析完生成的结果

extern string debugMessageDirectory;

unordered_map<string, TokenType> reverseTable{   // 词语token转换表
                 {"int", INT_TK},
                 {"const", CONST_TK},
                 {"void", VOID_TK},
                 {"if", IF_TK},
                 {"else", ELSE_TK},
                 {"while", WHILE_TK},
                 {"break", BREAK_TK},
                 {"continue", CONTINUE_TK},
                 {"return", RETURN_TK},
                 {",", COMMA},
                 {";", SEMICOLON},
                 {"[", LBRACKET},
                 {"]", RBRACKET},
                 {"{", LBRACE},
                 {"}", RBRACE},
                 {"(", LPAREN},
                 {")", RPAREN},
                 {"=", ASSIGN},
                 {"+", PLUS},
                 {"-", MINUS},
                 {"!", NOT},
                 {"*", MULT},
                 {"/", DIV},
                 {"%", REMAIN},
                 {"<", LESS},
                 {">", LARGE},
                 {"<=", LEQ},
                 {">=", LAQ},
                 {"==", EQUAL},
                 {"!=", NEQUAL},
                 {"&&", AND},
                 {"||", OR}};

// type1:/* */ type2://
/**
 * @brief 消去注释  
 * @param type 
 * @param in 词法分析的源文件
 */
void skipComment(int type, FILE *in)
{
    if (type == 1)
    {
        while (true)
        {
            c = fgetc(in);
            if (c == '*')
            {
                c = fgetc(in);
                if (c == '/')
                {
                    break;
                }
            }
        }
    }
    else
    {
        while (true)
        {
            c = fgetc(in);
            if (c == '\n')
            {
                break;
            }
        }
    }
}

/**
 * @brief 去除空格换行等字符
 * @param in 词法分析的源文件
 */
void skipSpaceOrNewlineOrTab(FILE *in)
{
    while (isSpace() || isNewline() || isTab())
    {
        c = fgetc(in);
    }
}

/**
 * @brief 处理关键字或标识符
 * @param in 词法分析的源文件
 */
void dealWithKeywordOrIdent(FILE *in)
{
    while ((isLetter() || isDigit()))
    {
        catToken();
        c = fgetc(in); // if c is not a whitespace, then jump to error
    }
    ungetc(c, in);

    string name(token);
    if (reverseTable.find(token) != reverseTable.end())  // 是否为关键字
    {
        TokenInfo tmp(reverseTable.at(token));
        tmp.setName(name);
        tokenInfoList.push_back(tmp);
    }
    else  // 为标识符
    {
        TokenInfo tmp(IDENT);
        tmp.setName(token);
        tokenInfoList.push_back(tmp);
    }
}

/**
 * @brief 处理连续的数字
 * @param in 词法分析的源文件
 */
void dealWithConstDigit(FILE *in)
{
    // deal with 0Xxxx
    bool isHex = false;
    bool isOct = false;
    if (c == '0')  // 进制转换
    {
        c = fgetc(in);
        if (c == 'x' || c == 'X')
        {
            isHex = true;
            c = fgetc(in);
        }
        else
        {
            isOct = true;
            ungetc(c, in);
            c = '0';
        }
    }
    while (isDigit())
    {
        catToken();
        c = fgetc(in);
    }
    ungetc(c, in);
    int64_t integer = strToInt(isHex, isOct);
    TokenInfo tmp(INTCONST);
    tmp.setName(token);
    if (integer == 2147483648 && tokenInfoList[tokenInfoList.size () - 1].getSym () == MINUS)  // 超过int上限
    {
        tokenInfoList.pop_back ();
        tmp.setValue (-integer);
    }
    else
    {
        tmp.setValue(integer);
    }
    tokenInfoList.push_back(tmp);
}

/**
 * @brief 处理字符串
 * @param in 词法分析的源文件
 */
void dealWithStr(FILE *in)
{
    while (true)
    {
        c = fgetc(in);
        catToken();
        if (c == '"')
        {
            break;
        }
    }
    TokenInfo tmp(STRCONST);  // 字符串常量
    tmp.setName(token);
    tokenInfoList.push_back(tmp);
}

/**
 * @brief 处理部分其他字符
 * @param in 词法分析的源文件
 */
void dealWithOtherTk(FILE *in)
{
    switch (c)
    {
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case ':':
    case '(':
    case ')':
    case '[':
    case ']':
    case '{':
    case '}':
    case ',':
    case ';':
    {
        TokenInfo tmp(reverseTable.at(string(1, c)));
        tmp.setName(string(1, c));
        tokenInfoList.push_back(tmp);
        break;
    }
    case '|':
    case '&':
    {
        fgetc(in);
        TokenInfo tmp(reverseTable.at(string(2, c)));
        tmp.setName(string(2, c));
        tokenInfoList.push_back(tmp);
        break;
    }
    case '<':
    case '>':
    case '!':
    case '=':
    {
        catToken();
        c = fgetc(in);
        if (c == '=')
        {
            catToken();
        }
        else
        {
            ungetc(c, in);
        }
        TokenInfo tmp(reverseTable.at(token));
        tmp.setName(token);
        tokenInfoList.push_back(tmp);
        break;
    }
    default:
        break;
    }
}

/**
 * @brief 去除不合规则字符
 * @param in 词法分析的源文件
 */
void skipIllegalChar(FILE *in)
{
    while (!isNewline() && c < 32)
    {
        c = fgetc(in);
        if (c == EOF)
        {
            break;
        }
    }
}

/**
 * @brief 词法分析
 * @param file 词法分析的源文件
 * @return true 成功；false 失败
 */
bool lexicalAnalyze(const string &file)
{
    FILE *in = fopen(file.c_str(), "r");
    if (!in)
    {
        return false;
    }
    parseSym(in);
    if (_debugLexer)
    {
        ofstream out (debugMessageDirectory + "lexer.txt", ios::out | ios::trunc);
        out << "[Lexer]\n";
        for (TokenInfo tokenInfo : tokenInfoList)
        {
            out << tokenInfo.getName ().c_str () << '\n';
        }
        out.close ();
    }
    fclose(in);
    return true;
}

/**
 * @brief 词法分析
 * @param in 词法分析的源文件
 */
void parseSym(FILE *in)
{
    while (c != EOF)
    {
        clearToken();
        c = fgetc(in);

        skipIllegalChar(in);
        skipSpaceOrNewlineOrTab(in);

        // 判断注释并去除
        if (c == '/')
        {
            c = fgetc(in);
            if (c == '/')
            {
                skipComment(2, in);
                continue;
            }
            else if (c == '*')
            {
                skipComment(1, in);
                continue;
            }
            else
            {
                ungetc(c, in);
                c = '/';
            }
        }

        if (isLetter())
        {
            dealWithKeywordOrIdent(in);
        }
        else if (isDigit())
        {
            dealWithConstDigit(in);
        }
        else if (c == '"')
        { // turn '/' to '/'
            dealWithStr(in);
        }
        else
        {
            dealWithOtherTk(in);
        }
    }
    tokenInfoList.emplace_back(TokenType::END);
}

// 空格
bool isSpace()
{
    return c == ' ';
}
// 英文字符或_
bool isLetter()
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c == '_');
}
// 是数字
bool isDigit()
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}
// tab
bool isTab()
{
    return c == '\t';
}
// 换行
bool isNewline()
{
    return c == '\n';
}
// 清除词法单元
void clearToken()
{
    token.clear();
}
// 词法单元后添加
void catToken()
{
    token.push_back(c);
}

/**
 * @brief 数字字符串转int64_t
 * @param isHex 是否为16进制
 * @param isOct 是否为8进制
 * @return 返回int64_t
 */
int64_t strToInt(bool isHex, bool isOct)
{
    int64_t integer;
    if (isHex)
    {
        integer = strtoll (token.c_str (), NULL, 16);
        //sscanf(token.c_str(), "%llx", &integer);
    }
    else if (isOct)
    {
        integer = strtoll (token.c_str (), NULL, 8);
        //sscanf(token.c_str(), "%llo", &integer);
    }
    else
    {
        integer = strtoll (token.c_str (), NULL, 10);
        //sscanf(token.c_str(), "%lld", &integer);
    }
    return integer;
}
