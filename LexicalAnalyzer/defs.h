#ifndef LAB4_DEFS_H
#define LAB4_DEFS_H

#define MAX_TEXT 10000 // длина исходного текста вместе с завершающим '\0'
#define MAX_LEX 100    // длина лексемы вместе с '\0'
#define MAX_KEYW 9     // 7 ключевых слов + true и false

typedef char LEX[MAX_LEX];
typedef char TypeMod[MAX_TEXT];

// Ключевые слова.
#define TClass 1
#define TInt 2
#define TBool 3
#define TDouble 4
#define TFor 5
#define TReturn 6
#define TMain 7

// Идентификаторы и константы.
#define TIdent 20
#define TConsInt 21
#define TConsBool 22

// Специальные знаки.
#define TDot 30
#define TComma 31
#define TSemicolon 32
#define TLParen 33
#define TRParen 34
#define TLBrace 35
#define TRBrace 36

// Знаки операций.
#define TAssign 40
#define TEq 41
#define TNeq 42
#define TLt 43
#define TLe 44
#define TGt 45
#define TGe 46
#define TPlus 47
#define TMinus 48
#define TMult 49
#define TDiv 50
#define TMod 51
#define TNot 52
#define TAnd 53
#define TOr 54

// Служебные типы.
#define TEnd 100
#define TErr 200

// Символьное имя типа нужно отладочной главной программе.
inline const char* tokenTypeName(const int type) {
    switch (type) {
        case TClass: return "TClass";
        case TInt: return "TInt";
        case TBool: return "TBool";
        case TDouble: return "TDouble";
        case TFor: return "TFor";
        case TReturn: return "TReturn";
        case TMain: return "TMain";
        case TIdent: return "TIdent";
        case TConsInt: return "TConsInt";
        case TConsBool: return "TConsBool";
        case TDot: return "TDot";
        case TComma: return "TComma";
        case TSemicolon: return "TSemicolon";
        case TLParen: return "TLParen";
        case TRParen: return "TRParen";
        case TLBrace: return "TLBrace";
        case TRBrace: return "TRBrace";
        case TAssign: return "TAssign";
        case TEq: return "TEq";
        case TNeq: return "TNeq";
        case TLt: return "TLt";
        case TLe: return "TLe";
        case TGt: return "TGt";
        case TGe: return "TGe";
        case TPlus: return "TPlus";
        case TMinus: return "TMinus";
        case TMult: return "TMult";
        case TDiv: return "TDiv";
        case TMod: return "TMod";
        case TNot: return "TNot";
        case TAnd: return "TAnd";
        case TOr: return "TOr";
        case TEnd: return "TEnd";
        case TErr: return "TErr";
        default: return "UnknownToken";
    }
}

#endif
