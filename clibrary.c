/*
 * clibrary.c
 *
 * Created: 11/27/2011 5:37:43 PM
 *  Author: tjaekel
 */ 

#include "picoc.h"
#include "picoc_commands.h"
#include "CMD_thread.h"

#ifdef DV_SIM
#include "amba_access.h"
#endif

struct OutputStream CStdOut;

static int TRUEValue = 1;
static int ZeroValue = 0;

/* initialize a library */
void LibraryInit(struct Table *GlobalTable, const char *LibraryName, const struct LibraryFunction (*FuncList)[10])
{
    (void)GlobalTable;

    struct ParseState Parser;
    int Count;
    char *Identifier;
    struct ValueType *ReturnType;
    struct Value *NewValue;
    void *Tokens;

    /* why we don't use the parameter LibraryName? */
    //const char *IntrinsicName = TableStrRegister("c library");
    // more correct would be:
    const char *IntrinsicName = TableStrRegister(LibraryName);

    /*
     * LibraryName is not used, we set here IntrinsicName to "c library", for what?
     */

    CStdOut.Putch = &PlatformPutc;

    for (Count = 0; (*FuncList)[Count].Prototype != NULL; Count++)
    {
        Tokens = LexAnalyse(IntrinsicName, (*FuncList)[Count].Prototype, strlen((char *)(*FuncList)[Count].Prototype), NULL);
        LexInitParser(&Parser, (*FuncList)[Count].Prototype, Tokens, IntrinsicName, TRUE);
        TypeParse(&Parser, &ReturnType, &Identifier);
        NewValue = ParseFunctionDefinition(&Parser, ReturnType, Identifier);
        NewValue->Val->FuncDef.Intrinsic = (void (*)())(*FuncList)[Count].Func;
        HeapFreeMem(Tokens);
    }
}

/* initialise the C library */
void CLibraryInit(void)
{
    /* define some constants */
    VariableDefinePlatformVar(NULL, (char*)"NULL", &IntType, (union AnyValue *)&ZeroValue, FALSE);
    VariableDefinePlatformVar(NULL, (char*)"TRUE", &IntType, (union AnyValue *)&TRUEValue, FALSE);
    VariableDefinePlatformVar(NULL, (char*)"FALSE", &IntType, (union AnyValue *)&ZeroValue, FALSE);
}

/* stream for writing into strings */
void SPutc(unsigned char Ch, union OutputStreamInfo *Stream)
{
    struct StringOutputStream *Out = (struct StringOutputStream *)&Stream->Str;
    *Out->WritePos++ = (char)Ch;
}

/* print a character to a stream without using printf/sprintf */
void PrintCh(char OutCh, struct OutputStream *Stream)
{
    (*Stream->Putch)(OutCh, &Stream->i);
}

/* print a string to a stream without using printf/sprintf */
void PrintStr(const char *Str, struct OutputStream *Stream)
{
    /* XXXX: TODO: optimize, take the string to merge into output string on DV_SIM */
    while (*Str != 0)
        PrintCh(*Str++, Stream);
}

/* print a single character a given number of times */
void PrintRepeatedChar(char ShowChar, int Length, struct OutputStream *Stream)
{
    while (Length-- > 0)
        PrintCh(ShowChar, Stream);
}

/* print an unsigned integer to a stream without using printf/sprintf */
void PrintUnsigned(unsigned long Num, unsigned int Base, int FieldWidth, int ZeroPad, int LeftJustify, struct OutputStream *Stream)
{
    static char Result[33] = {'\0'};
    int ResPos = sizeof(Result);

    Result[--ResPos] = '\0';
    if (Num == 0)
        Result[--ResPos] = '0';

    while (Num > 0)
    {
        unsigned long NextNum = Num / Base;
        unsigned long Digit = Num - NextNum * Base;
        if (Digit < 10)
            Result[--ResPos] = '0' + (char)Digit;
        else
            Result[--ResPos] = 'a' + (char)Digit - 10;

        Num = NextNum;
    }

    if (FieldWidth > 0 && !LeftJustify)
        PrintRepeatedChar(ZeroPad ? '0' : ' ', FieldWidth - (sizeof(Result) - 1 - ResPos), Stream);

    PrintStr(&Result[ResPos], Stream);

    if (FieldWidth > 0 && LeftJustify)
        PrintRepeatedChar(' ', FieldWidth - (sizeof(Result) - 1 - ResPos), Stream);
}

/* print an integer to a stream without using printf/sprintf */
void PrintInt(long Num, int FieldWidth, int ZeroPad, int LeftJustify, struct OutputStream *Stream)
{
    if (Num < 0)
    {
        PrintCh('-', Stream);
        Num = -Num;
        if (FieldWidth != 0)
            FieldWidth--;
    }

    PrintUnsigned((unsigned long)Num, 10, FieldWidth, ZeroPad, LeftJustify, Stream);
}

#ifndef NO_FP
/* print a double to a stream without using printf/sprintf */
void PrintFP(double Num, struct OutputStream *Stream)
{
    int Exponent = 0;
    int MaxDecimal;

    if (Num < 0)
    {
        PrintCh('-', Stream);
        Num = -Num;
    }

    if (Num >= 1e7)
        Exponent = (int)math_log10(Num);
    else if (Num <= 1e-7 && Num != 0.0)
        Exponent = (int)(math_log10(Num) - 0.999999999);

    Num /= math_pow(10.0, Exponent);
    PrintInt((long)Num, 0, FALSE, FALSE, Stream);
    PrintCh('.', Stream);
    Num = (Num - (long)Num) * 10;
    if (math_abs(Num) >= 1e-7)
    {
        for (MaxDecimal = 6; MaxDecimal > 0 && math_abs(Num) >= 1e-7; Num = (Num - (long)(Num + 1e-7)) * 10, MaxDecimal--)
            PrintCh('0' + (char)(Num + 1e-7), Stream);
    }
    else
        PrintCh('0', Stream);

    if (Exponent != 0)
    {
        PrintCh('e', Stream);
        PrintInt(Exponent, 0, FALSE, FALSE, Stream);
    }
}
#endif

/* print a type to a stream without using printf/sprintf */
void PrintType(struct ValueType *Typ, struct OutputStream *Stream)
{
    switch (Typ->Base)
    {
        case TypeVoid:          PrintStr("void", Stream); break;
        case TypeInt:           PrintStr("int", Stream); break;
        case TypeShort:         PrintStr("short", Stream); break;
        case TypeChar:          PrintStr("char", Stream); break;
        case TypeLong:          PrintStr("long", Stream); break;
        case TypeUnsignedInt:   PrintStr("unsigned int", Stream); break;
        case TypeUnsignedShort: PrintStr("unsigned short", Stream); break;
        case TypeUnsignedLong:  PrintStr("unsigned long", Stream); break;
#ifndef NO_FP
        case TypeFP:            PrintStr("double", Stream); break;
#endif
        case TypeFunction:      PrintStr("function", Stream); break;
        case TypeMacro:         PrintStr("macro", Stream); break;
        case TypePointer:       if (Typ->FromType) PrintType(Typ->FromType, Stream); PrintCh('*', Stream); break;
        case TypeArray:         PrintType(Typ->FromType, Stream); PrintCh('[', Stream); if (Typ->ArraySize != 0) PrintInt(Typ->ArraySize, 0, FALSE, FALSE, Stream); PrintCh(']', Stream); break;
        case TypeStruct:        PrintStr("struct ", Stream); PrintStr(Typ->Identifier, Stream); break;
        case TypeUnion:         PrintStr("union ", Stream); PrintStr(Typ->Identifier, Stream); break;
        case TypeEnum:          PrintStr("enum ", Stream); PrintStr(Typ->Identifier, Stream); break;
        case Type_Type:         PrintStr("type ", Stream); break;
    }
}

/* intrinsic functions made available to the language */
void GenericPrintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs, struct OutputStream *Stream)
{
    (void)Parser;
    (void)ReturnValue;

    unsigned char *FPos;
    struct Value *NextArg = Param[0];
    struct ValueType *FormatType;
    int ArgCount = 1;
    int LeftJustify = FALSE;
    int ZeroPad = FALSE;
    int FieldWidth = 0;
    unsigned char *Format = (unsigned char *)Param[0]->Val->NativePointer;

    for (FPos = Format; *FPos != '\0'; FPos++)
    {
        if (*FPos == '%')
        {
            FPos++;
            if (*FPos == '-')
            {
                /* a leading '-' means left justify */
                LeftJustify = TRUE;
                FPos++;
            }

            if (*FPos == '0')
            {
                /* a leading zero means zero pad a decimal number */
                ZeroPad = TRUE;
                FPos++;
            }

            /* get any field width in the format */
            while (isdigit(*FPos))
                FieldWidth = FieldWidth * 10 + (*FPos++ - '0');

            /* now check the format type */
            switch (*FPos)
            {
                case 's': FormatType = CharPtrType; break;
            case 'd': case 'u': case 'x': case 'h': case 'b': case 'c': FormatType = &IntType; break;
#ifndef NO_FP
                case 'f': FormatType = &FPType; break;
#endif
                case '%': PrintCh('%', Stream); FormatType = NULL; break;
                case '\0': FPos--; FormatType = NULL; break;
                default:  PrintCh(*FPos, Stream); FormatType = NULL; break;
            }

            if (FormatType != NULL)
            {
                /* we have to format something */
                if (ArgCount >= NumArgs)
                    //PrintStr("XXX", Stream);   /* not enough parameters for format */
                    ProgramFail(Parser, "not enough arguments");
                else
                {
                    NextArg = (struct Value *)((char *)NextArg + MEM_ALIGN(sizeof(struct Value) + TypeStackSizeValue(NextArg)));
                    if (NextArg->Typ != FormatType &&
                            !((FormatType == &IntType || *FPos == 'f') && IS_NUMERIC_COERCIBLE(NextArg)) &&
                            !(FormatType == CharPtrType && (NextArg->Typ->Base == TypePointer ||
                                                             (NextArg->Typ->Base == TypeArray && NextArg->Typ->FromType->Base == TypeChar) ) ) )
                        //PrintStr("XXX", Stream);   /* bad type for format */
                        ProgramFail(Parser, "pointer incorrect");
                    else
                    {
                        switch (*FPos)
                        {
                            case 's':
                            {
                                char *Str;

                                if (NextArg->Typ->Base == TypePointer)
                                    Str = (char *)NextArg->Val->NativePointer;
                                else
                                    Str = &NextArg->Val->ArrayMem[0];

                                if (Str == NULL)
                                    PrintStr("NULL", Stream);
                                else
                                    PrintStr(Str, Stream);
                                break;
                            }
                            case 'd': PrintInt(ExpressionCoerceInteger(NextArg), FieldWidth, ZeroPad, LeftJustify, Stream); break;
                            case 'u': PrintUnsigned(ExpressionCoerceUnsignedInteger(NextArg), 10, FieldWidth, ZeroPad, LeftJustify, Stream); break;
                            case 'x': case 'h': PrintUnsigned(ExpressionCoerceUnsignedInteger(NextArg), 16, FieldWidth, ZeroPad, LeftJustify, Stream); break;
                            case 'b': PrintUnsigned(ExpressionCoerceUnsignedInteger(NextArg), 2, FieldWidth, ZeroPad, LeftJustify, Stream); break;
                            case 'c': PrintCh((char)ExpressionCoerceUnsignedInteger(NextArg), Stream); break;
#ifndef NO_FP
                            case 'f': PrintFP(ExpressionCoerceFP(NextArg), Stream); break;
#endif
                        }
                    }
                }

                ArgCount++;
                //reset the previous formats
                LeftJustify = FALSE;
                ZeroPad = FALSE;
                FieldWidth = 0;
            }
        }
        else
            PrintCh(*FPos, Stream);
    }
}

/* printf(): print to console output */
void LibPrintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    struct OutputStream ConsoleStream;

    ConsoleStream.Putch = &PlatformPutc;
    GenericPrintf(Parser, ReturnValue, Param, NumArgs, &ConsoleStream);
}

void LibPrintLog(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    struct OutputStream ConsoleStream;

    ConsoleStream.Putch = &PlatformPutc;
    PrintStr("INFO: ", &ConsoleStream);
    GenericPrintf(Parser, ReturnValue, Param, NumArgs, &ConsoleStream);
}

void LibPrintErr(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    struct OutputStream ConsoleStream;

    ConsoleStream.Putch = &PlatformPutc;
    PrintStr("ERROR: ", &ConsoleStream);
    GenericPrintf(Parser, ReturnValue, Param, NumArgs, &ConsoleStream);
}

/* sprintf(): print to a string */
void LibSPrintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    struct OutputStream StrStream;

    StrStream.Putch = &SPutc;
    StrStream.i.Str.Parser = Parser;
    StrStream.i.Str.WritePos = (char *)Param[0]->Val->NativePointer;

    GenericPrintf(Parser, ReturnValue, Param+1, NumArgs-1, &StrStream);
    PrintCh(0, &StrStream);
    ReturnValue->Val->NativePointer = *Param;
}

/* $formatf(): UVM specific, generate and return formatted string */
void LibUVMSPrintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    static char fmtOutput[1024];

    struct OutputStream StrStream;

    StrStream.Putch = &SPutc;
    StrStream.i.Str.Parser = Parser;
    StrStream.i.Str.WritePos = fmtOutput;

    GenericPrintf(Parser, ReturnValue, Param, NumArgs, &StrStream);
    PrintCh(0, &StrStream);
    ReturnValue->Val->NativePointer = fmtOutput;
}

/* get a line of input, protected from buffer overrun */
void LibGets(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;

    char *ReadBuffer = (char *)Param[0]->Val->NativePointer;

    PlatformGetLine(ReadBuffer, Param[1]->Val->Integer);

    /* ATTENTION: when we assign to ReturnValue but the function is void function() -
     * we will get this error message: ERROR: :1: FATAL: not initialized pointer? internal crash?
     * So, make sure not to assign ReturnValue if nothing is returned!
     */
}

void LibGetc(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)Param;
    (void)NumArgs;

    ReturnValue->Val->Integer = PlatformGetCharacter();
}

#if 0
/* read a 32bit value from memory, param0 is address as LongInteger */
void LibReadMem(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
//#ifdef DV_SIM
  ReturnValue->Val->Integer = ReadMem32(Param[0]->Val->LongInteger);
//#else
  //ReturnValue->Val->Integer = Param[0]->Val->LongInteger;
//#endif
}

/* write a 32bit value to memory, param0 is address (as LongInteger), param1 is value (32bit LongInteger) */
void LibWriteMem(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
//#ifdef DV_SIM
  ReturnValue->Val->Integer = ReadMem32(Param[0]->Val->LongInteger);  /* read the previous value */
  WriteMem32(Param[0]->Val->LongInteger, Param[1]->Val->LongInteger);  /* write the new value */
//#else
  //ReturnValue->Val->Integer = Param[1]->Val->LongInteger;
//#endif
}
#endif

int ExitValue;

void LibExit(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

    ExitValue = Param[0]->Val->LongInteger;
    picoc_SetStopped();
    PlatformExit();
}

void LibQuit(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)ReturnValue;
    (void)Param;
    (void)NumArgs;

    ExitValue = -15;
    picoc_SetStopped();
    PlatformExit();
}

#ifdef PICOC_MATH_LIBRARY
void LibSin(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = math_sin(Param[0]->Val->FP);
}

void LibCos(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = math_cos(Param[0]->Val->FP);
}

void LibTan(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = math_tan(Param[0]->Val->FP);
}

void LibAsin(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = math_asin(Param[0]->Val->FP);
}

void LibAcos(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = math_acos(Param[0]->Val->FP);
}

void LibAtan(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = math_atan(Param[0]->Val->FP);
}

void LibSinh(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = math_sinh(Param[0]->Val->FP);
}

void LibCosh(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = math_cosh(Param[0]->Val->FP);
}

void LibTanh(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = math_tanh(Param[0]->Val->FP);
}

void LibExp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = math_exp(Param[0]->Val->FP);
}

void LibFabs(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = math_fabs(Param[0]->Val->FP);
}

void LibLog(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = math_log(Param[0]->Val->FP);
}

void LibLog10(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = math_log10(Param[0]->Val->FP);
}

void LibPow(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = math_pow(Param[0]->Val->FP, Param[1]->Val->FP);
}

void LibSqrt(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = math_sqrt(Param[0]->Val->FP);
}

void LibRound(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = math_floor(Param[0]->Val->FP + 0.5);   /* XXX - fix for soft float */
}

void LibCeil(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = math_ceil(Param[0]->Val->FP);
}

void LibFloor(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = math_floor(Param[0]->Val->FP);
}
#endif

#ifndef NO_STRING_FUNCTIONS
void LibMalloc(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;

    ReturnValue->Val->NativePointer = malloc(Param[0]->Val->Integer);
}

#ifndef NO_CALLOC
void LibCalloc(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->NativePointer = calloc(Param[0]->Val->Integer, Param[1]->Val->Integer);
}
#endif

#ifndef NO_REALLOC
void LibRealloc(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->NativePointer = realloc(Param[0]->Val->NativePointer, Param[1]->Val->Integer);
}
#endif

void LibFree(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

    free(Param[0]->Val->NativePointer);
}

void LibStrcpy(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

    char *To = (char *)Param[0]->Val->NativePointer;
    char *From = (char *)Param[1]->Val->NativePointer;

    while (*From != '\0')
        *To++ = *From++;

    *To = '\0';
}

void LibStrncpy(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

    char *To = (char *)Param[0]->Val->NativePointer;
    char *From = (char *)Param[1]->Val->NativePointer;
    int Len = Param[2]->Val->Integer;

    for (; *From != '\0' && Len > 0; Len--)
        *To++ = *From++;

    if (Len > 0)
        *To = '\0';
}

void LibStrcmp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;

    char *Str1 = (char *)Param[0]->Val->NativePointer;
    char *Str2 = (char *)Param[1]->Val->NativePointer;
    int StrEnded;

    for (StrEnded = FALSE; !StrEnded; StrEnded = (*Str1 == '\0' || *Str2 == '\0'), Str1++, Str2++)
    {
         if (*Str1 < *Str2) { ReturnValue->Val->Integer = -1; return; }
         else if (*Str1 > *Str2) { ReturnValue->Val->Integer = 1; return; }
    }

    ReturnValue->Val->Integer = 0;
}

void LibStrncmp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;

    char *Str1 = (char *)Param[0]->Val->NativePointer;
    char *Str2 = (char *)Param[1]->Val->NativePointer;
    int Len = Param[2]->Val->Integer;
    int StrEnded;

    for (StrEnded = FALSE; !StrEnded && Len > 0; StrEnded = (*Str1 == '\0' || *Str2 == '\0'), Str1++, Str2++, Len--)
    {
         if (*Str1 < *Str2) { ReturnValue->Val->Integer = -1; return; }
         else if (*Str1 > *Str2) { ReturnValue->Val->Integer = 1; return; }
    }

    ReturnValue->Val->Integer = 0;
}

void LibStrcat(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

    char *To = (char *)Param[0]->Val->NativePointer;
    char *From = (char *)Param[1]->Val->NativePointer;

    while (*To != '\0')
        To++;

    while (*From != '\0')
        *To++ = *From++;

    *To = '\0';
}

void LibIndex(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;

    char *Pos = (char *)Param[0]->Val->NativePointer;
    int SearchChar = Param[1]->Val->Integer;

    while (*Pos != '\0' && *Pos != SearchChar)
        Pos++;

    if (*Pos != SearchChar)
        ReturnValue->Val->NativePointer = NULL;
    else
        ReturnValue->Val->NativePointer = Pos;
}

void LibRindex(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;

    char *Pos = (char *)Param[0]->Val->NativePointer;
    int SearchChar = Param[1]->Val->Integer;

    ReturnValue->Val->NativePointer = NULL;
    for (; *Pos != '\0'; Pos++)
    {
        if (*Pos == SearchChar)
            ReturnValue->Val->NativePointer = Pos;
    }
}

void LibStrlen(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;

    char *Pos = (char *)Param[0]->Val->NativePointer;
    int Len;

    for (Len = 0; *Pos != '\0'; Pos++)
        Len++;

    ReturnValue->Val->Integer = Len;
}

void LibMemset(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

    /* we can use the system memset() */
    memset(Param[0]->Val->NativePointer, Param[1]->Val->Integer, Param[2]->Val->Integer);
}

void LibMemcpy(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

    /* we can use the system memcpy() */
    memcpy(Param[0]->Val->NativePointer, Param[1]->Val->NativePointer, Param[2]->Val->Integer);
}

void LibMemcmp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;

    unsigned char *Mem1 = (unsigned char *)Param[0]->Val->NativePointer;
    unsigned char *Mem2 = (unsigned char *)Param[1]->Val->NativePointer;
    int Len = Param[2]->Val->Integer;

    for (; Len > 0; Mem1++, Mem2++, Len--)
    {
         if (*Mem1 < *Mem2) { ReturnValue->Val->Integer = -1; return; }
         else if (*Mem1 > *Mem2) { ReturnValue->Val->Integer = 1; return; }
    }

    ReturnValue->Val->Integer = 0;
}
#endif

void LibMsSleep(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;

    CMD_delay(Param[0]->Val->Integer);
}

void LibHelp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Param;
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

    const struct LibraryFunction *p;
    p = CLibrary;
#ifndef SAVE_SPACE
    PlatformPrintf("Help - available functions:\n");
#endif
    while (p->Func)
    {
		PlatformPrintf("  %s\n", p->Prototype);
		p++;
    }
}

/* ---- list of all library functions and their prototypes ---- */
const struct LibraryFunction CLibrary[] =
{
#if 1
    { LibPrintf,        (const char *)"void printf(char *,...);" },
    { LibPrintLog,      (const char *)"void print_log(char *,...);" },
    { LibPrintErr,      (const char *)"void error_log(char *,...);" },
    { LibPrintf,        (const char *)"void $display(char *,...);" },
    { LibSPrintf,       (const char *)"char *sprintf(char *,char *, ...);" },
    { LibUVMSPrintf,    (const char *)"char *$sformatf(char *,...);" },
    { LibUVMSPrintf,    (const char *)"char *$psprintf(char *,...);" },
    { LibGets,          (const char *)"void gets(char *,int);" },
    { LibGetc,          (const char *)"int getchar();" },
    { LibExit,          (const char *)"void exit(int);" },
    { LibQuit,          (const char *)"void quit();"},
#endif
#ifdef PICOC_MATH_LIBRARY
    { LibSin,           (const char *)"float sin(float);" },
    { LibCos,           (const char *)"float cos(float);" },
    { LibTan,           (const char *)"float tan(float);" },
    { LibAsin,          (const char *)"float asin(float);" },
    { LibAcos,          (const char *)"float acos(float);" },
    { LibAtan,          (const char *)"float atan(float);" },
    { LibSinh,          (const char *)"float sinh(float);" },
    { LibCosh,          (const char *)"float cosh(float);" },
    { LibTanh,          (const char *)"float tanh(float);" },
    { LibExp,           (const char *)"float exp(float);" },
    { LibFabs,          (const char *)"float fabs(float);" },
    { LibLog,           (const char *)"float log(float);" },
    { LibLog10,         (const char *)"float log10(float);" },
    { LibPow,           (const char *)"float pow(float,float);" },
    { LibSqrt,          (const char *)"float sqrt(float);" },
    { LibRound,         (const char *)"float round(float);" },
    { LibCeil,          (const char *)"float ceil(float);" },
    { LibFloor,         (const char *)"float floor(float);" },
#endif
#ifndef NO_STRING_FUNCTIONS
    { LibMalloc,        (const char *)"void *malloc(int);" },
    { LibFree,          (const char *)"void free(void *);" },
#ifndef NO_CALLOC
    { LibCalloc,        (const char *)"void *calloc(int,int);" },
#endif
#ifndef NO_REALLOC
    { LibRealloc,       (const char *)"void *realloc(void *,int);" },
#endif
    { LibStrcpy,        (const char *)"void strcpy(char *,char *);" },
    { LibStrncpy,       (const char *)"void strncpy(char *,char *,int);" },
    { LibStrcmp,        (const char *)"int strcmp(char *,char *);" },
    { LibStrncmp,       (const char *)"int strncmp(char *,char *,int);" },
    { LibStrcat,        (const char *)"void strcat(char *,char *);" },
    { LibIndex,         (const char *)"char *index(char *,int);" },
    { LibRindex,        (const char *)"char *rindex(char *,int);" },
    { LibStrlen,        (const char *)"int strlen(char *);" },
    { LibMemset,        (const char *)"void memset(void *,int,int);" },
    { LibMemcpy,        (const char *)"void memcpy(void *,void *,int);" },
    { LibMemcmp,        (const char *)"int memcmp(void *,void *,int);" },
#endif
    /* Extensions for UVM */
    { UVMInfo,          (const char *)"void `uvm_info(char *,char *,int);" },
    { UVMError,         (const char *)"void `uvm_error(char *,char *);" },        //ATT: if same name again - silent crash!

    /* extension for ReadMem and WriteMem */
#if 1
    { LibHelp,			    (const char *)"void help();" },
    { LibHelp,			    (const char *)"void CHelp();" },
    { LibPicocRestart,  (const char *)"void PicocRestart();" },
    //{ LibReadMem,		(const char *)"unsigned long ReadMem(unsigned long);" },
    //{ LibWriteMem,	(const char *)"void WriteMem(unsigned long,unsigned long);" },

    //{ CmdReadMem,		(const char *)"unsigned long PReadMem(unsigned long);" },
    //{ CmdWriteMem,	(const char *)"unsigned long PWriteMem(unsigned long,unsigned long);" },
    { CmdMemWordPrint,	(const char *)"void PWords(void *,unsigned long);" },
    { CmdMemShortPrint,	(const char *)"void PShorts(void *,unsigned long);" },
    { CmdMemBytePrint,  (const char *)"void PBytes(void *,unsigned long);" },
#ifndef SAVE_SPACE		
    { CmdWriteOMem,		  (const char *)"void PWriteOMem(unsigned short,unsigned short);" },					//no need: use WriteMem
    { CmdFillMem,		    (const char *)"void FillMem(unsigned short,unsigned short,unsigned short);" },		//no need: use memset
    { CmdCmpMem,		    (const char *)"int PCmpMem(unsigned short,unsigned short,unsigned short);" },
    { CmdCpyMem,		    (const char *)"void MemCpy(unsigned short,unsigned short,unsigned short);" },			//no need: use memcmp
    { CmdCheckRead,		  (const char *)"int PCheckRead(unsigned short,unsigned short,unsigned short);" },
    { CmdCheckWrite,	  (const char *)"int PCheckWrite(unsigned short,unsigned short,unsigned short);" },
    { CmdPoll,			    (const char *)"void Poll(unsigned short,unsigned short,unsigned short);" },
#endif
#ifdef WITH_SCRIPTS
    /* open a script file and process it */
    { LibOpenScript,	  (const char *)"int RunScript(char *);" },
    { LibGetExitVal,	  (const char *)"int ExitValue();" },
#endif
#if 1
    { LibMsSleep,       (const char *)"void mssleep(unsigned long);" },
#endif
#if 0
    { LibExecuteCommand,(const char *)"int ShellCommand(char *);" },
#else
    { LibExecuteCommand,(const char *)"int ShellCommand(char *,...);" },
#if 0
    { LibWriteConsec,   (const char *)"int WriteConsecutive(void *,unsigned long,unsigned long,unsigned long);" },
    { LibReadConsec,    (const char *)"int ReadConsecutive(void *,unsigned long,unsigned long,unsigned long);" },
#endif
#if 0
    { LibCpuReadSingle, (const char *)"unsigned long CPU_READ_SINGLE(unsigned long,int);" },
    { LibCpuWriteSingle,(const char *)"void CPU_WRITE_SINGLE(unsigned long,unsigned long,int);" },
#endif

    { LibSpiTrans,      (const char *)"int SpiTransaction(unsigned char *,unsigned char *,int);" },
    { LibSpiINTfifo,    (const char *)"unsigned short SpiINTfifo(int,unsigned char *,int);" },
#if 0
    { LibI2CRead,       (const char *)"int I2CRead(unsigned char,unsigned char,unsigned char *,int,int);" },
    { LibI2CWrite,      (const char *)"int I2CWrite(unsigned char,unsigned char *,int,int);" },

    { LibGetGPIO,       (const char *)"unsigned long GetGPIO();" },
    { LibPutGPIO,       (const char *)"void PutGPIO(unsigned long);" },

	  { LibSetLED,       	(const char *)"void SetLED(int,int);" },
#endif

    { LibUDPSend,      (const char *)"int UDPsend(int port,unsigned char *,unsigned long);" },

	  { LibSetINThandler,	      (const char *)"void SetINTHandler(char *);" },
    { LibSetINThandler2,      (const char *)"void SetINTHandler2(char *);" },
    { LibDefaultINTHandler,   (const char *)"void DefaultINTHandler(int);" },

    { LibMethodSize,    (const char *)"int size();"},	//use as: variable.size() - returns number of elements for arrays

#endif

#endif

    { NULL,             NULL }
};
