#include "lexer.h"
#include "../../basic/std/compile_std.h"

#include <vector>
#include <unordered_map>
#include <algorithm>

using namespace std;

int c;  // 缓冲区
string token;  // 词法单元

std::vector<TokenInfo> tokenInfoList;  // 词法分析完生成的结果

extern string debugMessageDirectory;

std::unordered_map<string, TokenType>
    reverseTable{// NOLINT
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

// type1:/* */ type2:// // 去除注释
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

// 去除空格换行等字符
void skipSpaceOrNewlineOrTab(FILE *in)
{
    while (isSpace() || isNewline() || isTab())
    {
        c = fgetc(in);
    }
}

void dealWithKeywordOrIdent(FILE *in)
{
    while ((isLetter() || isDigit()))
    {
        catToken();
        c = fgetc(in); // if c is not a whitespace, then jump to error
    }
    // fseek(in, -1L, 1);
    ungetc(c, in);

    string name(token);
    if (reverseTable.find(token) != reverseTable.end())  // 是否为关键字
    {
        TokenInfo tmp(reverseTable.at(token));
        tmp.setName(name);
        tokenInfoList.push_back(tmp);
    }
    else  // 为表示符
    {
        TokenInfo tmp(IDENT);
        tmp.setName(token);
        tokenInfoList.push_back(tmp);
    }
}

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
            // fseek(in, -1L, 1);
            ungetc(c, in);
            c = '0';
        }
    }
    while (isDigit())
    {
        catToken();
        c = fgetc(in);
    }
    // fseek(in, -1L, 1);
    ungetc(c, in);
    long long integer = strToInt(isHex, isOct);
    TokenInfo tmp(INTCONST);
    tmp.setName(token);
    //if (integer == 2147483648 && tokenInfoList[tokenInfoList.size () - 1].getSym () == MINUS)  ????
    //{
    //    tokenInfoList.pop_back ();
    //    tmp.setValue (-integer);
    //}
    if (tokenInfoList[tokenInfoList.size() - 1].getSym() == MINUS && tokenInfoList[tokenInfoList.size () - 2].getSym () != INTCONST && tokenInfoList[tokenInfoList.size () - 2].getSym () != IDENT)  // 为负数？？
    {
        tokenInfoList.pop_back();
        tmp.setValue(-integer);
    }
    else
    {
        tmp.setValue(integer);
    }
    tokenInfoList.push_back(tmp);
}

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

// 部分其他字符
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
            // fseek(in, -1L, 1);
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

// 去除不合规则字符
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

// 词法分析
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
        FILE *out = fopen((debugMessageDirectory + "lexer.txt").c_str(), "w+");
        fprintf(out, "[Lexer]\n");
        for (TokenInfo tokenInfo : tokenInfoList)
        {
            fprintf(out, "%s", tokenInfo.getName().c_str());
            // if (tokenInfo.getSym() == INTCONST)
            // {
            //     fprintf(out, " %d", tokenInfo.getValue());
            // }
            fprintf(out, "\n");
        }
        fclose(out);
    }
    fclose(in);
    return true;
}

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
                // fseek(in, -1L, 1);
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

bool isTab()
{
    return c == '\t';
}

bool isNewline()
{
    return c == '\n';
}

void clearToken()
{
    token.clear();
}

void catToken()
{
    token.push_back(c);
}

long long strToInt(bool isHex, bool isOct)
{
    long long integer;
    if (isHex)
    {
        sscanf(token.c_str(), "%llx", &integer);
    }
    else if (isOct)
    {
        sscanf(token.c_str(), "%llo", &integer);
    }
    else
    {
        sscanf(token.c_str(), "%lld", &integer);
    }
    return integer;
}
