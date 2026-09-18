using System;
using System.Runtime.InteropServices;

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

/* Harbour primitive aliases */

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

namespace HbXVM
{
    public static class XVM
    {
        private const string DLL = "harbour-32.dll";

        #region Sequence

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmSeqBegin();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmSeqEnd();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern HB_BOOL hb_xvmSeqRecover();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmSeqAlways();

        #endregion

        #region Frame

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmFrame(
            int iLocals,
            int iParams);

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

        #endregion

        #region Variables

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
        public static extern void hb_xvmPushLocal(
            HB_SHORT iLocal);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPushLocalByRef(
            HB_SHORT iLocal);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPopLocal(
            HB_SHORT iLocal);

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

        #endregion

        #region Return

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmRetValue();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmRetNil();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmRetInt(
            HB_LONG value);

        #endregion

        #region Blocks

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPushBlock(
            nint pCode,
            PHB_SYMB pSymbols);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPushBlockShort(
            nint pCode,
            PHB_SYMB pSymbols);

        #endregion

        #region Arrays

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmArrayGen(
            HB_SIZE nElements);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmArrayDim(
            HB_USHORT uiDimensions);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]

        public static extern HB_BOOL hb_xvmArrayPush();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]

        public static extern HB_BOOL hb_xvmArrayPop();

        #endregion

        #region Constants

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPushDouble(
            double dNumber,
            int iWidth,
            int iDec);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPushLongLong(
            HB_LONGLONG llNumber);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_xvmPushStringHidden(
            int iMethod,
            [MarshalAs(UnmanagedType.LPUTF8Str)] string szText,
            HB_SIZE nSize);

        #endregion

        #region Names
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

    }
}
