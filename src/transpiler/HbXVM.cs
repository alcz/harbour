using System;
using System.Runtime.InteropServices;

/* Harbour primitive aliases
 * NOTE: global using directives must appear before any namespace declaration (CS8915),
 * so they were moved to the top of the file.
 */

/* global using HB_BOOL     = System.Boolean; */
global using HB_BOOL     = System.Int32;
global using HB_BYTE     = System.Byte;
global using HB_SHORT    = System.Int16;
global using HB_USHORT   = System.UInt16;
global using HB_LONG     = System.Int32;
global using HB_ULONG    = System.UInt32;
global using HB_LONGLONG = System.Int64;
global using HB_SIZE     = System.UIntPtr;

global using PHB_ITEM = HbTypes.PHB_ITEM;
global using PHB_SYMB = HbTypes.PHB_SYMB;

namespace HbTypes
{
    /* Opaque Harbour handles */

    [StructLayout(LayoutKind.Sequential)]
    public readonly struct PHB_SYMB
    {
        public readonly nint Handle;

        public PHB_SYMB(nint handle) => Handle = handle;

        public static implicit operator nint(PHB_SYMB value) => value.Handle;
        public static implicit operator PHB_SYMB(nint value) => new(value);

        public override string ToString() => $"0x{Handle:X}";
    }

    [StructLayout(LayoutKind.Sequential)]
    public readonly struct PHB_ITEM
    {
        public readonly nint Handle;

        public PHB_ITEM(nint handle) => Handle = handle;

        public static implicit operator nint(PHB_ITEM value) => value.Handle;
        public static implicit operator PHB_ITEM(nint value) => new(value);

        public override string ToString() => $"0x{Handle:X}";
    }
}

namespace HbXVM
{
    public static class XVM
    {
        private const string DLL = "harbour-32.dll";

        #region Procedure

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmExitProc();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmEndProc();

        #endregion

        #region Sequence

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmSeqBegin();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmSeqEnd();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmSeqEndTest();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmSeqRecover();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmSeqAlways();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmAlwaysBegin();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmAlwaysEnd();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmSeqBlock();

        #endregion

        #region FOR EACH / WITH OBJECT / SWITCH

        /* prepare FOR EACH loop */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmEnumStart(
            int iArgs,
            int iDescend);

        /* increment FOR EACH loop counter */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmEnumNext();

        /* decrement FOR EACH loop counter */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmEnumPrev();

        /* rewind the stack after FOR EACH loop counter */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmEnumEnd();

        /* prepare WITH OBJECT statement */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmWithObjectStart();

        /* rewind the stack after normal WITH OBJECT */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmWithObjectEnd();

        /* send WITH OBJECT message to current WITH OBJECT control variable */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmWithObjectMessage(
            PHB_SYMB pSymbol);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmSwitchGet(
            out PHB_ITEM pSwitchVal);

        #endregion

        #region Line info

        /* set .prg line number information */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmSetLine(
            HB_USHORT uiLine);

        #endregion

        #region Frame

        /* increases the stack pointer for the amount of locals and params supplied */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmFrame(
            int iLocals,
            int iParams);

        /* increases the stack pointer for the amount of locals and variable params */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmVFrame(
            int iLocals,
            int iParams);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmSFrame(
            PHB_SYMB pSymbol);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmStatics(
            PHB_SYMB pSymbol,
            HB_USHORT uiStatics);

        /* statics points to an unmanaged HB_BYTE buffer */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmThreadStatics(
            HB_USHORT uiStatics,
            nint statics);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmParameter(
            PHB_SYMB pSymbol,
            int iParams);

        #endregion

        #region Return

        /* pops the latest stack value into stack.Return */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmRetValue();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmRetNil();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmRetInt(
            HB_LONG lValue);

        #endregion

        #region Calls

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmDo(
            HB_USHORT uiParams);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmFunction(
            HB_USHORT uiParams);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmSend(
            HB_USHORT uiParams);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPushFuncSymbol(
            PHB_SYMB pSym);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmFuncPtr();

        #endregion

        #region Variables: statics / locals / self / params

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmPushObjectVarRef();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPushStatic(
            HB_USHORT uiStatic);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPushStaticByRef(
            HB_USHORT uiStatic);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPopStatic(
            HB_USHORT uiStatic);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmPushVariable(
            PHB_SYMB pSymbol);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmPopVariable(
            PHB_SYMB pSymbol);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPushSelf();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPushVParams();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPushAParams();

        /* pushes the content of a local onto the stack */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPushLocal(
            HB_SHORT iLocal);

        /* pushes a local by reference onto the stack */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPushLocalByRef(
            HB_SHORT iLocal);

        /* pops the stack latest value onto a local */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPopLocal(
            HB_SHORT iLocal);

        #endregion

        #region Variables: fields / memvars / aliases

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmPushField(
            PHB_SYMB pSymbol);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmPopField(
            PHB_SYMB pSymbol);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmPushMemvar(
            PHB_SYMB pSymbol);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmPushMemvarByRef(
            PHB_SYMB pSymbol);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmPopMemvar(
            PHB_SYMB pSymbol);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmPushAliasedField(
            PHB_SYMB pSymbol);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmPopAliasedField(
            PHB_SYMB pSymbol);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmPushAliasedFieldExt(
            PHB_SYMB pAlias,
            PHB_SYMB pField);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmPopAliasedFieldExt(
            PHB_SYMB pAlias,
            PHB_SYMB pField);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmPushAliasedVar(
            PHB_SYMB pSymbol);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmPopAliasedVar(
            PHB_SYMB pSymbol);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmPushAlias();

        /* select the workarea using a given item or a substituted value */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmPopAlias();

        /* pops the stack latest value and returns its logical value */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmPopLogical(
            out HB_BOOL pValue);

        /* swaps items on the eval stack and pops the workarea number */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmSwapAlias();

        #endregion

        #region Local variable helpers

        /* add integer to given local variable */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmLocalAddInt(
            int iLocal,
            HB_LONG lAdd);

        /* increment given local variable */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmLocalInc(
            int iLocal);

        /* decrement given local variable */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmLocalDec(
            int iLocal);

        /* increment given local variable and push it on HVM stack */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmLocalIncPush(
            int iLocal);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmLocalAdd(
            int iLocal);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmLocalSetInt(
            int iLocal,
            HB_LONG lValue);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmCopyLocals(
            int iDest,
            int iSource);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmStaticAdd(
            HB_USHORT uiStatic);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMemvarAdd(
            PHB_SYMB pSymbol);

        #endregion

        #region Blocks

        /* creates a codeblock (pCode points to unmanaged pcode) */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPushBlock(
            nint pCode,
            PHB_SYMB pSymbols);

        /* creates a codeblock */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPushBlockShort(
            nint pCode,
            PHB_SYMB pSymbols);

        #endregion

        #region Logical / stack operations

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmAnd();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmOr();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmNot();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmNegate();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmDuplicate();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmDuplUnRef();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPushUnRef();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmSwap(
            int iCount);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmForTest();

        #endregion

        #region Comparison

        /* checks if the two latest values on the stack are equal, removes both and leaves result */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmEqual();

        /* checks if the two latest values on the stack are exactly equal, removes both and leaves result */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmExactlyEqual();

        /* checks if the two latest values on the stack are not equal, removes both and leaves result */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmNotEqual();

        /* checks if the latest - 1 value is less than the latest, removes both and leaves result */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmLess();

        /* checks if the latest - 1 value is less than or equal the latest, removes both and leaves result */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmLessEqual();

        /* checks if the latest - 1 value is greater than the latest, removes both and leaves result */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmGreater();

        /* checks if the latest - 1 value is greater than or equal the latest, removes both and leaves result */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmGreaterEqual();

        /* check whether string 1 is contained in string 2 */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmInstring();

        #endregion

        #region Arithmetic

        /* sums the latest two values on the stack, removes them and leaves the result */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmPlus();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmPlusEq();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmPlusEqPop();

        /* subtracts the latest two values on the stack, removes them and leaves the result */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMinus();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMinusEq();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMinusEqPop();

        /* multiplies the latest two values on the stack, removes them and leaves the result */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMult();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMultEq();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMultEqPop();

        /* divides the latest two values on the stack, removes them and leaves the result */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmDivide();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmDivEq();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmDivEqPop();

        /* calculates the modulus of latest two values on the stack, removes them and leaves the result */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmModulus();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmModEq();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmModEqPop();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmPower();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmExpEq();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmExpEqPop();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmInc();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmIncEq();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmIncEqPop();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmDec();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmDecEq();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmDecEqPop();

        #endregion

        #region Arrays / Hashes

        /* generates an uiDimensions Array and initialize those dimensions from the stack values */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmArrayDim(
            HB_USHORT uiDimensions);

        /* generates an nElements Array and fills it from the stack values */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmArrayGen(
            HB_SIZE nElements);

        /* pushes an array element to the stack, removing the array and the index from the stack */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmArrayPush();

        /* pushes a reference to an array element to the stack, removing the array and the index from the stack */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmArrayPushRef();

        /* pops a value from the stack */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmArrayPop();

        /* generates an nElements Hash and fills it from the stack values */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmHashGen(
            HB_SIZE nElements);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmArrayItemPush(
            HB_SIZE nIndex);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmArrayItemPop(
            HB_SIZE nIndex);

        #endregion

        #region Debug names

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmModuleName(
            [MarshalAs(UnmanagedType.LPUTF8Str)] string szModuleName);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmLocalName(
            HB_USHORT uiLocal,
            [MarshalAs(UnmanagedType.LPUTF8Str)] string szLocalName);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmStaticName(
            HB_BYTE bIsGlobal,
            HB_USHORT uiStatic,
            [MarshalAs(UnmanagedType.LPUTF8Str)] string szStaticName);

        #endregion

        #region Macro

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMacroDo(
            HB_USHORT uiArgSets);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMacroFunc(
            HB_USHORT uiArgSets);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMacroSend(
            HB_USHORT uiArgSets);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMacroArrayGen(
            HB_USHORT uiArgSets);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMacroPush(
            int bFlags);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMacroPushRef();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMacroPushIndex();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMacroPushList(
            int bFlags);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMacroPushAliased(
            int bFlags);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMacroPushPare(
            int bFlags);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMacroPop(
            int bFlags);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMacroPopAliased(
            int bFlags);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMacroSymbol();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMacroText();

        #endregion

        #region Constants

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPushStringHidden(
            int iMethod,
            [MarshalAs(UnmanagedType.LPUTF8Str)] string szText,
            HB_SIZE nSize);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPushDouble(
            double dNumber,
            int iWidth,
            int iDec);

        /* (HB_LONG_LONG_OFF variant taking a double is not exposed) */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPushLongLong(
            HB_LONGLONG llNumber);

        #endregion

        #region Constants - C macros forwarded to hb_vmPush*
        /* In the C header these are #define macros, so they are not exported as hb_xvm* symbols.
         * They are imported here from the real hb_vm* exports via EntryPoint, keeping the hb_xvm* names. */

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, EntryPoint = "hb_vmPushLogical")]
        public static extern void hb_xvmPushLogical(
            HB_BOOL f);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, EntryPoint = "hb_vmPushInteger")]
        public static extern void hb_xvmPushInteger(
            int i);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, EntryPoint = "hb_vmPushLong")]
        public static extern void hb_xvmPushLong(
            HB_LONG l);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, EntryPoint = "hb_vmPushNil")]
        public static extern void hb_xvmPushNil();

        /* psz must point to memory that stays alive (pcode string constant) - it is NOT copied */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, EntryPoint = "hb_vmPushStringPcode")]
        public static extern void hb_xvmPushStringConst(
            nint psz,
            HB_SIZE ul);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, EntryPoint = "hb_vmPushSymbol")]
        public static extern void hb_xvmPushSymbol(
            PHB_SYMB p);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, EntryPoint = "hb_vmPushDate")]
        public static extern void hb_xvmPushDate(
            HB_LONG l);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, EntryPoint = "hb_vmPushTimeStamp")]
        public static extern void hb_xvmPushTimeStamp(
            HB_LONG d,
            HB_LONG t);

        #endregion

        #region Multi PCODE operations

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmMultByInt(
            HB_LONG lValue);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmDivideByInt(
            HB_LONG lValue);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmModulusByInt(
            HB_LONG lValue);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmAddInt(
            HB_LONG lValue);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmLessThenInt(
            HB_LONG lValue);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmLessThenIntIs(
            HB_LONG lValue,
            out HB_BOOL fValue);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmLessEqualThenInt(
            HB_LONG lValue);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmLessEqualThenIntIs(
            HB_LONG lValue,
            out HB_BOOL fValue);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmGreaterThenInt(
            HB_LONG lValue);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmGreaterThenIntIs(
            HB_LONG lValue,
            out HB_BOOL fValue);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmGreaterEqualThenInt(
            HB_LONG lValue);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmGreaterEqualThenIntIs(
            HB_LONG lValue,
            out HB_BOOL fValue);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmEqualInt(
            HB_LONG lValue);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmEqualIntIs(
            HB_LONG lValue,
            out HB_BOOL fValue);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmNotEqualInt(
            HB_LONG lValue);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmNotEqualIntIs(
            HB_LONG lValue,
            out HB_BOOL fValue);

        #endregion
    }
}