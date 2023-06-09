/*
 * picoc.h
 *
 * Created: 11/27/2011 5:45:55 PM
 *  Author: tjaekel
 */ 

#ifndef PICOC_H_
#define PICOC_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Attention: include this file first, before printf.h !
 */

//instead to set the macro definitions in makefile or on compile tools - we do it here
#define NO_FP
#define NO_CALLOC
#define NO_REALLOC
#undef  NO_STRING_FUNCTIONS
#undef  USE_PLATFORM_LIB
#undef 	NO_HASH_INCLUDE			  //we do not support #include
#undef  PICOC_MATH_LIBRARY		//compile errors with math library
#undef  DEBUG_LEXER
#undef  DEBUG_HEAP
#define WITHOUT_SYSSTAT_H
#define WITH_SCRIPTS			//send and execute scripts via TeraTerm - broken
#define SAVE_SPACE				//minimal functions defined
////#define FANCY_ERROR_REPORTING

#define	MAX_SCRIPT_SIZE	1024	//size of text script files loaded/sent via UART

#include "platform.h"

/* handy definitions */
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef min
#define min(x,y) (((x)<(y))?(x):(y))
#endif

#define MEM_ALIGN(x) ((unsigned long)(((x) + sizeof(ALIGN_TYPE) - 1) & ~(sizeof(ALIGN_TYPE)-1)))

#define GETS_BUF_MAX 256

/* coercion of numeric types to other numeric types */
#ifndef NO_FP
#define IS_FP(v) ((v)->Typ->Base == TypeFP)
#define FP_VAL(v) ((v)->Val->FP)
#else
#define IS_FP(v) 0
#define FP_VAL(v) 0
#endif

#define IS_POINTER_COERCIBLE(v, ap) ((ap) ? ((v)->Typ->Base == TypePointer) : 0)
#define POINTER_COERCE(v) ((int)(v)->Val->NativePointer)

#define IS_INTEGER_NUMERIC_TYPE(t) ((t)->Base >= TypeInt && (t)->Base <= TypeUnsignedLong)
#define IS_INTEGER_NUMERIC(v) IS_INTEGER_NUMERIC_TYPE((v)->Typ)
#define IS_NUMERIC_COERCIBLE(v) (IS_INTEGER_NUMERIC(v) || IS_FP(v))
#define IS_NUMERIC_COERCIBLE_PLUS_POINTERS(v,ap) (IS_NUMERIC_COERCIBLE(v) || IS_POINTER_COERCIBLE(v,ap))

struct Table;

/* lexical tokens */
enum LexToken
{
    /* 0x00 */ TokenNone,
    /* 0x01 */ TokenComma,
    /* 0x02 */ TokenAssign, TokenAddAssign, TokenSubtractAssign, TokenMultiplyAssign, TokenDivideAssign, TokenModulusAssign,
    /* 0x08 */ TokenShiftLeftAssign, TokenShiftRightAssign, TokenArithmeticAndAssign, TokenArithmeticOrAssign, TokenArithmeticExorAssign,
    /* 0x0d */ TokenQuestionMark, TokenColon,
    /* 0x0f */ TokenLogicalOr,
    /* 0x10 */ TokenLogicalAnd,
    /* 0x11 */ TokenArithmeticOr,
    /* 0x12 */ TokenArithmeticExor,
    /* 0x13 */ TokenAmpersand,
    /* 0x14 */ TokenEqual, TokenNotEqual,
    /* 0x16 */ TokenLessThan, TokenGreaterThan, TokenLessEqual, TokenGreaterEqual,
    /* 0x1a */ TokenShiftLeft, TokenShiftRight,
    /* 0x1c */ TokenPlus, TokenMinus,
    /* 0x1e */ TokenAsterisk, TokenSlash, TokenModulus,
    /* 0x21 */ TokenIncrement, TokenDecrement, TokenUnaryNot, TokenUnaryExor, TokenSizeof, TokenCast,
    /* 0x27 */ TokenLeftSquareBracket, TokenRightSquareBracket, TokenDot, TokenArrow,
    /* 0x2b */ TokenOpenBracket, TokenCloseBracket,
    /* 0x2d */ TokenIdentifier, TokenIntegerConstant, TokenFPConstant, TokenStringConstant, TokenCharacterConstant,
    /* 0x32 */ TokenSemicolon, TokenEllipsis,
    /* 0x34 */ TokenLeftBrace, TokenRightBrace,
    /* 0x36 */ TokenIntType, TokenCharType, TokenFloatType, TokenDoubleType, TokenVoidType, TokenEnumType,
    /* 0x3c */ TokenLongType, TokenSignedType, TokenShortType, TokenStructType, TokenUnionType, TokenUnsignedType, TokenTypedef,
    /*      */ TokenUint32Type, TokenInt32Type, TokenUint16Type, TokenInt16Type, TokenUint8Type, TokenInt8Type,
    /* 0x43 */ TokenContinue, TokenDo, TokenElse, TokenFor, TokenIf, TokenWhile, TokenBreak, TokenSwitch, TokenCase, TokenDefault, TokenReturn,
    /* 0x4e */ TokenHashDefine, TokenHashUndef, TokenHashInclude, TokenNew, TokenDelete,
    /* 0x53 */ TokenEOF, TokenEndOfLine, TokenEndOfFunction,
    /* 0x55 */ TokenIgnore,
    /* 0x56 */ TokenIfDefine, TokenIfDefineVal, TokenElseDefine, TokenEndDefine, TokenIfNDefine
};

/* used in dynamic memory allocation */
struct AllocNode
{
    unsigned int Size;
    struct AllocNode *NextFree;
};

/* whether we're running or skipping code */
enum RunMode
{
    RunModeRun,                 /* we're running code as we parse it */
    RunModeSkip,                /* skipping code, not running */
    RunModeReturn,              /* returning from a function */
    RunModeCaseSearch,          /* searching for a case label */
    RunModeBreak,               /* breaking out of a switch/while/do */
    RunModeContinue             /* as above but repeat the loop */
};

/* parser state - has all this detail so we can parse nested files */
struct ParseState
{
    const unsigned char *Pos;
    int Line;
    const char *FileName;
    enum RunMode Mode;          /* whether to skip or run code */
    int SearchLabel;            /* what case label we're searching for */
#ifdef FANCY_ERROR_REPORTING
    int CharacterPos;
    const char *SourceText;
#endif
};

/* values */
enum BaseType
{
    TypeVoid,                   /* no type */
    TypeInt,                    /* integer */
    TypeShort,                  /* short integer */
    TypeChar,                   /* a single character (unsigned) */
    TypeLong,                   /* long integer */
    TypeUnsignedInt,            /* unsigned integer */
    TypeUnsignedShort,          /* unsigned short integer */
    TypeUnsignedLong,           /* unsigned long integer */
#ifndef NO_FP
    TypeFP,                     /* floating point */
#endif
    TypeFunction,               /* a function */
    TypeMacro,                  /* a macro */
    TypePointer,                /* a pointer */
    TypeArray,                  /* an array of a sub-type */
    TypeStruct,                 /* aggregate type */
    TypeUnion,                  /* merged type */
    TypeEnum,                   /* enumerated integer type */
    Type_Type                   /* a type for storing types */
};

/* data type */
struct ValueType
{
    enum BaseType Base;             /* what kind of type this is */
    int ArraySize;                  /* the size of an array type */
    int Sizeof;                     /* the storage required */
    const char *Identifier;         /* the name of a struct or union */
    struct ValueType *FromType;     /* the type we're derived from (or NULL) */
    struct ValueType *DerivedTypeList;  /* first in a list of types derived from this one */
    struct ValueType *Next;         /* next item in the derived type list */
    struct Table *Members;          /* members of a struct or union */
    int OnHeap;                     /* true if allocated on the heap */
};

typedef struct ValueType TValueType;

/* function definition */
struct FuncDef
{
    struct ValueType *ReturnType;   /* the return value type */
    int NumParams;                  /* the number of parameters */
    int VarArgs;                    /* has a variable number of arguments after the explicitly specified ones */
    struct ValueType **ParamType;   /* array of parameter types */
    char **ParamName;               /* array of parameter names */
    void (*Intrinsic)();            /* intrinsic call address or NULL */
    struct ParseState Body;         /* lexical tokens of the function body if not intrinsic */
};

/* values */
union AnyValue
{
    unsigned char Character;
    short ShortInteger;
    int Integer;
    long LongInteger;
    unsigned short UnsignedShortInteger;
    unsigned int UnsignedInteger;
    unsigned long UnsignedLongInteger;
    char *Identifier;
    char ArrayMem[2];               /* placeholder for where the data starts, doesn't point to it */
    struct ParseState Parser;
    struct ValueType *Typ;
    struct FuncDef FuncDef;
#ifndef NO_FP
    double FP;
#endif
    void *NativePointer;            /* unsafe native pointers */
};

struct Value
{
    struct ValueType *Typ;          /* the type of this value */
    union AnyValue *Val;            /* pointer to the AnyValue which holds the actual content */
    struct Value *LValueFrom;       /* if an LValue, this is a Value our LValue is contained within (or NULL) */
    char ValOnHeap;                 /* the AnyValue is on the heap (but this Value is on the stack) */
    char ValOnStack;                /* the AnyValue is on the stack along with this Value */
    char IsLValue;                  /* is modifiable and is allocated somewhere we can usefully modify it */
};

/* hash table data structure */
struct TableEntry
{
    struct TableEntry *Next;        /* next item in this hash chain */
    union TableEntryPayload
    {
        struct ValueEntry
        {
            char *Key;              /* points to the shared string table */
            struct Value *Val;      /* the value we're storing */
        } v;                        /* used for tables of values */

        char Key[1];                /* dummy size - used for the shared string table */
    } p;
};

struct Table
{
    short Size;
    short OnHeap;
    struct TableEntry **HashTable;
};

/* stack frame for function calls */
struct StackFrame
{
    struct ParseState ReturnParser;         /* how we got here */
    struct Value *ReturnValue;              /* copy the return value here */
    struct Value **Parameter;               /* array of parameter values */
    int NumParams;                          /* the number of parameters */
    struct Table LocalTable;                /* the local variables and parameters */
    struct TableEntry *LocalHashTable[LOCAL_TABLE_SIZE];
    struct StackFrame *PreviousStackFrame;  /* the next lower stack frame */
};

/* lex state */
struct LexState
{
    const char *Pos;
    const char *End;
    const char *FileName;
    int Line;
#ifdef FANCY_ERROR_REPORTING
    int CharacterPos;
    const char *SourceText;
#endif
};

/* library function definition */
struct LibraryFunction
{
    const void (*Func)(struct ParseState *Parser, struct Value *, struct Value **, int);
    const char *Prototype;
};

/* output stream-type specific state information */
union OutputStreamInfo
{
    struct StringOutputStream
    {
        struct ParseState *Parser;
        char *WritePos;
    } Str;
};

/* stream-specific method for writing characters to the console */
typedef void CharWriter(unsigned char, union OutputStreamInfo *);

/* used when writing output to a string - eg. sprintf) */
struct OutputStream
{
    CharWriter *Putch;
    union OutputStreamInfo i;
};

/* possible results of parsing a statement */
enum ParseResult { ParseResultEOF, ParseResultError, ParseResultOk };

/* globals */
extern void   *HeapStackTop;
extern void   *HeapStart;
extern void   *HeapBottom;
extern struct Table GlobalTable;
extern struct StackFrame *TopStackFrame;
extern struct ValueType UberType;
extern struct ValueType IntType;
extern struct ValueType CharType;
#ifndef NO_FP
extern struct ValueType FPType;
#endif
extern struct ValueType VoidType;
extern struct ValueType TypeType;
extern struct ValueType FunctionType;
extern struct ValueType MacroType;
extern struct ValueType *CharPtrType;
extern struct ValueType *CharArrayType;
extern struct ValueType *VoidPtrType;
extern char   *StrEmpty;
extern struct PointerValue NULLPointer;
extern const struct LibraryFunction CLibrary[];
#ifdef USE_PLATFORM_LIB
extern struct LibraryFunction PlatformLibrary[];
#endif
extern struct OutputStream CStdOut;

/* table.c */
void TableInit(void);
char *TableStrRegister(const char *Str);
char *TableStrRegister2(const char *Str, int Len);
void TableInitTable(struct Table *Tbl, struct TableEntry **HashTable, int Size, int OnHeap);
int TableSet(struct Table *Tbl, char *Key, struct Value *Val);
int TableGet(struct Table *Tbl, const char *Key, struct Value **Val);
struct Value *TableDelete(struct Table *Tbl, const char *Key);
char *TableSetIdentifier(struct Table *Tbl, const char *Ident, int IdentLen);
void TableStrFree(void);

/* lex.c */
void LexInit(void);
void LexCleanup(void);
void *LexAnalyse(const char *FileName, const char *Source, int SourceLen, int *TokenLen);
void LexInitParser(struct ParseState *Parser, const char *SourceText, void *TokenSource, const char *FileName, int RunIt);
enum LexToken LexGetToken(struct ParseState *Parser, struct Value **Value, int IncPos);
void LexToEndOfLine(struct ParseState *Parser);
void *LexCopyTokens(struct ParseState *StartParser, struct ParseState *EndParser);
void LexInteractiveClear(struct ParseState *Parser);
void LexInteractiveCompleted(struct ParseState *Parser);
void LexInteractiveStatementPrompt(void);

/* parse.c */
enum ParseResult ParseStatement(struct ParseState *Parser, int CheckTrailingSemicolon);
struct Value *ParseFunctionDefinition(struct ParseState *Parser, struct ValueType *ReturnType, char *Identifier);
void Parse(const char *FileName, const char *Source, int SourceLen, int RunIt);
void ParseInteractive(void);
void ParseCleanup(void);
void ParserCopyPos(struct ParseState *To, struct ParseState *From);
void ParseMacroIfDefine(struct ParseState *Parser);
void ParseMacroElseDefine(struct ParseState *Parser);
void ParseMacroEndDefine(struct ParseState *Parser);

/* expression.c */
int ExpressionParse(struct ParseState *Parser, struct Value **Result);
long ExpressionParseInt(struct ParseState *Parser);
void ExpressionAssign(struct ParseState *Parser, struct Value *DestValue, struct Value *SourceValue, int Force, const char *FuncName, int ParamNo, int AllowPointerCoercion);
long ExpressionCoerceInteger(struct Value *Val);
unsigned long ExpressionCoerceUnsignedInteger(struct Value *Val);
#ifndef NO_FP
double ExpressionCoerceFP(struct Value *Val);
#endif

/* type.c */
void TypeInit(void);
void TypeCleanup(void);
int TypeSize(struct ValueType *Typ, int ArraySize, int Compact);
int TypeSizeValue(struct Value *Val);
int TypeStackSizeValue(struct Value *Val);
int TypeLastAccessibleOffset(struct Value *Val);
int TypeParseFront(struct ParseState *Parser, struct ValueType **Typ);
void TypeParseIdentPart(struct ParseState *Parser, struct ValueType *BasicTyp, struct ValueType **Typ, char **Identifier);
void TypeParse(struct ParseState *Parser, struct ValueType **Typ, char **Identifier);
struct ValueType *TypeGetMatching(struct ParseState *Parser, struct ValueType *ParentType, enum BaseType Base, int ArraySize, const char *Identifier);

/* heap.c */
void HeapInit(void);
void *HeapAllocStack(int Size);
int HeapPopStack(void *Addr, int Size);
void HeapUnpopStack(int Size);
void HeapPushStackFrame(void);
int HeapPopStackFrame(void);
void *HeapAllocMem(int Size);
void HeapFreeMem(void *Mem);

/* variable.c */
void VariableInit(void);
void VariableCleanup(void);
void VariableFree(struct Value *Val);
void VariableTableCleanup(struct Table *HashTable);
void *VariableAlloc(struct ParseState *Parser, int Size, int OnHeap);
void VariableStackPop(struct ParseState *Parser, struct Value *Var);
struct Value *VariableAllocValueAndData(struct ParseState *Parser, int DataSize, int IsLValue, struct Value *LValueFrom, int OnHeap);
struct Value *VariableAllocValueAndCopy(struct ParseState *Parser, struct Value *FromValue, int OnHeap);
struct Value *VariableAllocValueFromType(struct ParseState *Parser, struct ValueType *Typ, int IsLValue, struct Value *LValueFrom, int OnHeap);
struct Value *VariableAllocValueFromExistingData(struct ParseState *Parser, struct ValueType *Typ, union AnyValue *FromValue, int IsLValue, struct Value *LValueFrom);
struct Value *VariableAllocValueShared(struct ParseState *Parser, struct Value *FromValue);
struct Value *VariableDefine(struct ParseState *Parser, char *Ident, struct Value *InitValue, struct ValueType *Typ, int MakeWritable);
int VariableDefined(const char *Ident);
void VariableGet(struct ParseState *Parser, const char *Ident, struct Value **LVal);
void VariableDefinePlatformVar(struct ParseState *Parser, char *Ident, struct ValueType *Typ, union AnyValue *FromValue, int IsWritable);
void VariableStackFrameAdd(struct ParseState *Parser, int NumParams);
void VariableStackFramePop(struct ParseState *Parser);
struct Value *VariableStringLiteralGet(char *Ident);
void VariableStringLiteralDefine(char *Ident, struct Value *Val);
void *VariableDereferencePointer(struct ParseState *Parser, struct Value *PointerValue, struct Value **DerefVal, int *DerefOffset, struct ValueType **DerefType, int *DerefIsLValue);

/* clibrary.c */
void LibraryInit(struct Table *GlobalTable, const char *LibraryName, const struct LibraryFunction (*FuncList)[10]);
void CLibraryInit(void);
void PrintCh(char OutCh, struct OutputStream *Stream);
void PrintInt(long Num, int FieldWidth, int ZeroPad, int LeftJustify, struct OutputStream *Stream);
void PrintStr(const char *Str, struct OutputStream *Stream);
void PrintFP(double Num, struct OutputStream *Stream);
void PrintType(struct ValueType *Typ, struct OutputStream *Stream);
void LibPrintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);

void GenericPrintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs, struct OutputStream *Stream);
void SPutc(unsigned char Ch, union OutputStreamInfo *Stream);

/* platform.c */
void ProgramFail(struct ParseState *Parser, const char *Message, ...);
void AssignFail(struct ParseState *Parser, const char *Format, struct ValueType *Type1, struct ValueType *Type2, int Num1, int Num2, const char *FuncName, int ParamNo);
void LexFail(struct LexState *Lexer, const char *Message, ...);
void PlatformCleanup(void);
void PlatformScanFile(const char *FileName);
char *PlatformGetLine(char *Buf, int MaxLen);
int  PlatformGetCharacter(void);
void PlatformPutc(unsigned char OutCh, union OutputStreamInfo *);
void PlatformErrorPrefix(struct ParseState *Parser);
void PlatformPrintf(const char *Format, ...);
void PlatformVPrintf(const char *Format, va_list Args);
void PlatformExit(void);
void PlatformLibraryInit(void);
void Initialise(void);
void Cleanup(void);

int picoc_INThandler(void);
int picoc_INThandler2(void);

int pico_c_isRunning(void);
int pico_c_getExitValue(void);

void put_string(char *s);

/* for Qt integration */
extern char pico_c_cmd_buffer[LINEBUFFER_MAX];
void    put_character(unsigned char c);
char *  picoc_GetCommandLine(char *s, int MaxLen);

int     pico_c_main_interactive(int argc, char **argv);
int     pico_c_init(void);
int     pico_c_deinit(void);
void    picoc_ModifyInput(char *dst, char *src);

extern int GpicocAbort;
int     picoc_CheckForAbort(void);
int     picoc_CheckForStopped(void);
void	  picoc_SetStopped(void);
void 	  picoc_ClearStopped(void);
void    picoc_MsSleep(unsigned long ms);
int     picoc_ExecuteCommand(char *s);
int     picoc_SpiTransaction(unsigned char *tx, unsigned char *rx, int bytes);
unsigned short picoc_GetINTfifo(int num, unsigned char *rx, int bytes);
int     picoc_I2CRead(unsigned char slaveAddr, unsigned char regAddr, unsigned char *data, int bytes, int flags);
int     picoc_I2CWrite(unsigned char slaveAddr, unsigned char *data, int bytes, int flags);
void    picoc_WriteGPIO(unsigned long val);
unsigned long picoc_ReadGPIO(void);

unsigned short picoc_GetINTStatus(int num);

void picoc_DefaultINTHandlerC(int num);

void picoc_decodePRIfifo(void);
#ifdef __cplusplus
}
#endif

#endif /* PICOC_H_ */
