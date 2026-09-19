/*
 * Harbour Transpiler - C# declaration-only emitter
 *
 * Variant of the C# code emitter. It produces the same class / method /
 * function / field skeleton as the full emitter (same names, same
 * signatures, same Program partial class, same `<FileBase>_<Var>` field
 * mangling) but NO executable code: every method / function / property
 * body is an empty stub.
 *
 * Kept:   using lines, #include/#define comments and consts, BEGINCSHARP
 *         blocks, class declarations (DATA / CLASS VAR / ACCESS / ASSIGN /
 *         INLINE / METHOD signatures), STATIC / MEMVAR / PUBLIC fields
 *         (without initialisers), standalone function signatures
 *         (ref / nilable / params-spread / defaults), short ref overloads.
 * Gone:   statement + expression emission, ref-shims, hoisting, FOR EACH
 *         pair handling, date/string operator rewrites, INLINE translation,
 *         INIT translation, hash key-type and integer-demotion pre-passes.
 *
 * Copyright 2026 harbour.github.io
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#include "hbcomp.h"
#include "hbast.h"
#include "hbdate.h"
#include "hbreftab.h"
#include "hbfunctab.h"
#include "hbdefinemap.h"
#include "hbfieldtypes.h"
#include "hbhbxcanon.h"
#include "hbfilecase.h"

/* Stub body policy: 0 = `return default;` (empty for void),
   1 = `throw new NotImplementedException();` */
#ifndef HB_CS_STUB_THROWS
#define HB_CS_STUB_THROWS 0
#endif

/* ---- State ---- */

static PHB_COMP     s_pCompCtx  = NULL;
static PHB_AST_NODE s_pClassList = NULL;
static PHB_REFTAB   s_pRefTab   = NULL;
static char         s_szFileBase[ 64 ] = "";

/* File-scope MEMVAR / PUBLIC field registry (dedup only). */
#define HB_CS_MAX_FILE_MEMVARS 256
static const char * s_pFileMemvars[ HB_CS_MAX_FILE_MEMVARS ];
static int s_iFileMemvarCount = 0;

/* File-scope STATIC FUNCTION / PROCEDURE names (mangled with file base). */
#define HB_CS_MAX_FILE_STATIC_FUNCS 256
static const char * s_pFileStaticFuncs[ HB_CS_MAX_FILE_STATIC_FUNCS ];
static int s_iFileStaticFuncCount = 0;

/* ---- Small helpers ---- */

static void hb_csEmitIndent( FILE * yyc, int iIndent )
{
   int i;
   for( i = 0; i < iIndent; i++ )
      fprintf( yyc, "    " );
}

static HB_BOOL hb_csIsFileMemvar( const char * szName )
{
   int i;
   if( ! szName )
      return HB_FALSE;
   for( i = 0; i < s_iFileMemvarCount; i++ )
      if( hb_stricmp( s_pFileMemvars[ i ], szName ) == 0 )
         return HB_TRUE;
   return HB_FALSE;
}

static void hb_csAddFileMemvar( const char * szName )
{
   if( ! szName || s_iFileMemvarCount >= HB_CS_MAX_FILE_MEMVARS ||
       hb_csIsFileMemvar( szName ) )
      return;
   s_pFileMemvars[ s_iFileMemvarCount++ ] = szName;
}

static HB_BOOL hb_csIsFileStaticFunc( const char * szName )
{
   int i;
   if( ! szName )
      return HB_FALSE;
   for( i = 0; i < s_iFileStaticFuncCount; i++ )
      if( hb_stricmp( s_pFileStaticFuncs[ i ], szName ) == 0 )
         return HB_TRUE;
   return HB_FALSE;
}

static void hb_csAddFileStaticFunc( const char * szName )
{
   if( ! szName || s_iFileStaticFuncCount >= HB_CS_MAX_FILE_STATIC_FUNCS ||
       hb_csIsFileStaticFunc( szName ) )
      return;
   s_pFileStaticFuncs[ s_iFileStaticFuncCount++ ] = szName;
}

/* `<FileBase>_<Name>` for a file-STATIC function, declaration-site casing. */
static const char * hb_csMangleStaticFunc( const char * szName,
                                           char * szBuf, size_t nBufSize )
{
   if( hb_csIsFileStaticFunc( szName ) && s_szFileBase[ 0 ] )
   {
      const char * szCanon = szName;
      int i;
      for( i = 0; i < s_iFileStaticFuncCount; i++ )
         if( hb_stricmp( s_pFileStaticFuncs[ i ], szName ) == 0 )
         {
            szCanon = s_pFileStaticFuncs[ i ];
            break;
         }
      hb_snprintf( szBuf, nBufSize, "%s_%s", s_szFileBase, szCanon );
      return szBuf;
   }
   return szName;
}

/* reftab key of a function: file-STATICs are `<FileBase>::<Name>`. */
static const char * hb_csFuncRefKey( const char * szName,
                                     char * szBuf, HB_SIZE nBuf )
{
   if( szName && hb_csIsFileStaticFunc( szName ) && s_szFileBase[ 0 ] )
   {
      hb_snprintf( szBuf, nBuf, "%s::%s", s_szFileBase, szName );
      return szBuf;
   }
   return szName;
}

/* ---- Type mapping ---- */

static const char * hb_csTypeMap( const char * szHbType )
{
   if( ! szHbType || hb_stricmp( szHbType, "USUAL" ) == 0 )
      return "dynamic";
   if( hb_stricmp( szHbType, "NUMERIC" ) == 0 )
      return "decimal";
   if( hb_stricmp( szHbType, "INTEGER" ) == 0 ||
       hb_stricmp( szHbType, "int" ) == 0 )
      return "long";
   if( hb_stricmp( szHbType, "STRING" ) == 0 ||
       hb_stricmp( szHbType, "CHARACTER" ) == 0 )
      return "string";
   if( hb_stricmp( szHbType, "LOGICAL" ) == 0 )
      return "bool";
   if( hb_stricmp( szHbType, "DATE" ) == 0 )
      return "DateOnly";
   if( hb_stricmp( szHbType, "TIMESTAMP" ) == 0 )
      return "DateTime";
   if( hb_stricmp( szHbType, "OBJECT" ) == 0 )
      return "dynamic";
   if( hb_stricmp( szHbType, "ARRAY" ) == 0 )
      return "dynamic[]";
   if( hb_stricmp( szHbType, "HASH"  ) == 0 ||
       hb_stricmp( szHbType, "HASHC" ) == 0 )
      return "Dictionary<string, dynamic>";
   if( hb_stricmp( szHbType, "HASHN" ) == 0 )
      return "Dictionary<decimal, dynamic>";
   if( hb_stricmp( szHbType, "BLOCK" ) == 0 ||
       hb_stricmp( szHbType, "CODEBLOCK" ) == 0 )
      return "dynamic";
   if( s_pRefTab && hb_refTabIsClassDynamic( s_pRefTab, szHbType ) )
      return "dynamic";
   if( hb_stricmp( szHbType, "TOleAuto" ) == 0 )
      return "dynamic";
   return szHbType;
}

static const char * hb_csScopeStr( int iScope )
{
   switch( iScope )
   {
      case HB_AST_SCOPE_PROTECTED: return "protected";
      case HB_AST_SCOPE_HIDDEN:    return "private";
      default:                      return "public";
   }
}

/* ---- Parameter / signature helpers (reftab driven) ---- */

/* Reftab row of parameter iPos of a function; file-STATIC rows first. */
static const HB_REFPARAM * hb_csCallParam( const char * szFunc, int iPos )
{
   if( ! szFunc || ! s_pRefTab )
      return NULL;
   if( hb_csIsFileStaticFunc( szFunc ) && s_szFileBase[ 0 ] )
   {
      char szKey[ 256 ];
      const HB_REFPARAM * pP;
      hb_snprintf( szKey, sizeof( szKey ), "%s::%s", s_szFileBase, szFunc );
      pP = hb_refTabParam( s_pRefTab, szKey, iPos );
      if( pP )
         return pP;
   }
   return hb_refTabParam( s_pRefTab, szFunc, iPos );
}

/* By-ref ARRAY slot the callee never reassigns: emitted as plain dynamic[]. */
static HB_BOOL hb_csParamElidesArrayRef( const char * szFunc, int iPos )
{
   const HB_REFPARAM * pP = hb_csCallParam( szFunc, iPos );
   return pP && pP->fByRef && ! pP->fReassigned &&
          pP->szType && hb_stricmp( pP->szType, "ARRAY" ) == 0;
}

static HB_BOOL hb_csParamEmitsRef( const char * szFunc, int iPos )
{
   return s_pRefTab && hb_refTabIsRef( s_pRefTab, szFunc, iPos ) &&
          ! hb_csParamElidesArrayRef( szFunc, iPos );
}

/* Slot type as the signature emits it: reftab type unless USUAL, else
   Hungarian inference. */
static const char * hb_csSlotTypeName( const char * szFnKey, int iPos,
                                       const char * szParam )
{
   const HB_REFPARAM * pP = hb_refTabParam( s_pRefTab, szFnKey, iPos );
   if( pP && pP->szType && hb_stricmp( pP->szType, "USUAL" ) != 0 )
      return pP->szType;
   return hb_astInferType( szParam, NULL );
}

/* Empty body. */
static void hb_csEmitStubBody( FILE * yyc, int iIndent, HB_BOOL fVoid )
{
   hb_csEmitIndent( yyc, iIndent );
   fprintf( yyc, "{\n" );
#if HB_CS_STUB_THROWS
   hb_csEmitIndent( yyc, iIndent + 1 );
   fprintf( yyc, "throw new NotImplementedException();\n" );
   HB_SYMBOL_UNUSED( fVoid );
#else
   if( ! fVoid )
   {
      hb_csEmitIndent( yyc, iIndent + 1 );
      fprintf( yyc, "return default;\n" );
   }
#endif
   hb_csEmitIndent( yyc, iIndent );
   fprintf( yyc, "}\n" );
}

/* The "untyped" twin of a typed declaration: same name, returns void,
   and a single marker parameter of type HbVmStack (an enum declared once
   in HbRuntime.cs: `public enum HbVmStack { Args }`) meaning "the actual
   arguments sit on the Harbour VM stack", as in harbour -gc3 output.
   The marker parameter keeps the signature distinct even for
   parameterless originals (a plain `void Name()` would be CS0111 against
   `Name()`), and needs no default so overload resolution never sees an
   ambiguity with the typed overload. Body is a placeholder for the
   future VM-driven code. */
static void hb_csEmitVmStackOverload( FILE * yyc, int iIndent,
                                      const char * szScope, HB_BOOL fStatic,
                                      const char * szName )
{
   fprintf( yyc, "\n" );
   hb_csEmitIndent( yyc, iIndent );
   fprintf( yyc, "%s %svoid %s( HbVmStack _hbvm )\n", szScope,
            fStatic ? "static " : "", szName );
   hb_csEmitIndent( yyc, iIndent );
   fprintf( yyc, "{\n" );
   hb_csEmitIndent( yyc, iIndent + 1 );
   fprintf( yyc, "/* placeholder: arguments sit on the Harbour VM stack"
                 " (cf. harbour -gc3 output) */\n" );
   hb_csEmitIndent( yyc, iIndent );
   fprintf( yyc, "}\n" );
}

/* Emit the parameter list (between the parentheses) of a function or
   method. Returns HB_TRUE when the signature was widened to
   `params dynamic[] hbva`. Defaults are `= default` / `= null` only:
   the declared-default (`DEFAULT p TO v`) lifting needs body analysis
   and is not part of this variant. */
static HB_BOOL hb_csEmitParams( FILE * yyc, PHB_AST_NODE pFunc,
                                PHB_HFUNC pCompFunc, const char * szKey,
                                HB_BOOL fIsMain )
{
   PHB_HVAR pVar = pFunc->value.asFunc.pParams;
   int iLastRef = -1, k, iPos = 0;
   HB_BOOL fSpread = HB_FALSE;
   HB_BOOL fFirst = HB_TRUE;

   if( fIsMain )
   {
      if( ! pVar || pCompFunc->wParamCount == 0 )
      {
         fprintf( yyc, "string[] args" );
         fFirst = HB_FALSE;
      }
   }
   else
   {
      for( k = 0; k < ( int ) pCompFunc->wParamCount; k++ )
         if( hb_refTabIsRef( s_pRefTab, szKey, k ) )
            iLastRef = k;
      /* C# `params` cannot combine with `ref` */
      fSpread = iLastRef < 0 &&
                ( hb_refTabIsCalledVarargs( s_pRefTab, szKey ) ||
                  hb_refTabIsVariadic( s_pRefTab, szKey ) );
   }

   if( fSpread )
   {
      fprintf( yyc, "params dynamic[] hbva" );
      return HB_TRUE;
   }

   for( ; pVar && iPos < ( int ) pCompFunc->wParamCount;
        pVar = pVar->pNext, iPos++ )
   {
      HB_BOOL fRef = hb_refTabIsRef( s_pRefTab, szKey, iPos );
      HB_BOOL fNil = hb_refTabIsNilable( s_pRefTab, szKey, iPos );

      if( fRef && hb_csParamElidesArrayRef( szKey, iPos ) )
         fRef = HB_FALSE;
      if( ! fFirst )
         fprintf( yyc, ", " );
      fFirst = HB_FALSE;
      if( fRef )
         fprintf( yyc, "ref " );
      fprintf( yyc, "%s%s %s",
               hb_csTypeMap( hb_csSlotTypeName( szKey, iPos, pVar->szName ) ),
               fNil ? "?" : "", pVar->szName );
      if( ! fIsMain && ! fRef && iPos > iLastRef )
         fprintf( yyc, fNil ? " = null" : " = default" );
   }
   return HB_FALSE;
}

/* Short overload taking only the parameters before the first `ref` one,
   emitted when some caller (per the reftab arity bitmap) needs it. */
static void hb_csEmitShortOverload( FILE * yyc, PHB_AST_NODE pFunc,
                                    PHB_HFUNC pCompFunc, const char * szKey,
                                    int iIndent )
{
   int iFirstRef = -1, k;
   int iMax = ( int ) pCompFunc->wParamCount;
   HB_U64 bitArities, shortMask;
   char szMangledBuf[ 256 ];
   PHB_HVAR pP;

   if( hb_refTabIsCalledVarargs( s_pRefTab, szKey ) )
      return;
   for( k = 0; k < iMax; k++ )
      if( hb_csParamEmitsRef( szKey, k ) )
      {
         iFirstRef = k;
         break;
      }
   if( iFirstRef < 0 )
      return;

   bitArities = hb_refTabCallArities( s_pRefTab, szKey );
   shortMask = ( ( ( HB_U64 ) 1 ) << ( iFirstRef + 1 ) ) - 1;
   if( bitArities != 0 && ( bitArities & shortMask ) == 0 )
      return;

   fprintf( yyc, "\n" );
   hb_csEmitIndent( yyc, iIndent );
   fprintf( yyc, "public static %s %s(",
            pFunc->value.asFunc.fProcedure ? "void" : "dynamic",
            hb_csMangleStaticFunc( pFunc->value.asFunc.szName,
                                   szMangledBuf, sizeof( szMangledBuf ) ) );
   pP = pFunc->value.asFunc.pParams;
   for( k = 0; pP && k < iFirstRef; k++, pP = pP->pNext )
   {
      HB_BOOL fNil = hb_refTabIsNilable( s_pRefTab, szKey, k );
      if( k > 0 )
         fprintf( yyc, ", " );
      fprintf( yyc, "%s%s %s = %s",
               hb_csTypeMap( hb_csSlotTypeName( szKey, k, pP->szName ) ),
               fNil ? "?" : "", pP->szName, fNil ? "null" : "default" );
   }
   fprintf( yyc, ")\n" );
   hb_csEmitStubBody( yyc, iIndent, pFunc->value.asFunc.fProcedure );
}

/* ---- Body scan kept only because it decides a class's base type ---- */

/* obj:&(name) macro send anywhere in the class → HbDynamicObject base. */
static HB_BOOL hb_csExprHasMacroSend( PHB_EXPR pExpr )
{
   if( ! pExpr )
      return HB_FALSE;
   switch( pExpr->ExprType )
   {
      case HB_ET_SEND:
         if( pExpr->value.asMessage.pMessage &&
             pExpr->value.asMessage.pMessage->ExprType == HB_ET_MACRO )
            return HB_TRUE;
         return hb_csExprHasMacroSend( pExpr->value.asMessage.pObject ) ||
                hb_csExprHasMacroSend( pExpr->value.asMessage.pParms );
      case HB_ET_LIST: case HB_ET_ARGLIST: case HB_ET_MACROARGLIST:
      {
         PHB_EXPR p = pExpr->value.asList.pExprList;
         for( ; p; p = p->pNext )
            if( hb_csExprHasMacroSend( p ) )
               return HB_TRUE;
         break;
      }
      default:
         if( pExpr->ExprType >= HB_EO_ASSIGN && pExpr->ExprType <= HB_EO_PREDEC )
            return hb_csExprHasMacroSend( pExpr->value.asOperator.pLeft ) ||
                   hb_csExprHasMacroSend( pExpr->value.asOperator.pRight );
         break;
   }
   return HB_FALSE;
}

static HB_BOOL hb_csBlockHasMacroSend( PHB_AST_NODE pBlock )
{
   PHB_AST_NODE pStmt;
   if( ! pBlock || pBlock->type != HB_AST_BLOCK )
      return HB_FALSE;
   for( pStmt = pBlock->value.asBlock.pFirst; pStmt; pStmt = pStmt->pNext )
   {
      switch( pStmt->type )
      {
         case HB_AST_EXPRSTMT:
            if( hb_csExprHasMacroSend( pStmt->value.asExprStmt.pExpr ) )
               return HB_TRUE;
            break;
         case HB_AST_RETURN:
            if( hb_csExprHasMacroSend( pStmt->value.asReturn.pExpr ) )
               return HB_TRUE;
            break;
         case HB_AST_IF:
            if( hb_csExprHasMacroSend( pStmt->value.asIf.pCondition ) ||
                hb_csBlockHasMacroSend( pStmt->value.asIf.pThen ) )
               return HB_TRUE;
            break;
         case HB_AST_DOWHILE:
            if( hb_csBlockHasMacroSend( pStmt->value.asWhile.pBody ) )
               return HB_TRUE;
            break;
         case HB_AST_FOR:
            if( hb_csBlockHasMacroSend( pStmt->value.asFor.pBody ) )
               return HB_TRUE;
            break;
         case HB_AST_FOREACH:
            if( hb_csBlockHasMacroSend( pStmt->value.asForEach.pBody ) )
               return HB_TRUE;
            break;
         default:
            break;
      }
   }
   return HB_FALSE;
}

/* ---- File-level (header) nodes: #include, comments, #define consts ---- */

static void hb_csEmitHeaderNode( PHB_AST_NODE pNode, FILE * yyc, int iIndent )
{
   switch( pNode->type )
   {
      case HB_AST_INCLUDE:
         hb_csEmitIndent( yyc, iIndent );
         fprintf( yyc, "// #include \"%s\"\n", pNode->value.asInclude.szFile );
         break;

      case HB_AST_COMMENT:
      {
         const char * szText = pNode->value.asComment.szText;
         /* by-ref convention marker: no residual purpose */
         if( szText[ 0 ] == '/' && szText[ 1 ] == '*' && szText[ 2 ] == '@' &&
             szText[ 3 ] == '*' && szText[ 4 ] == '/' && szText[ 5 ] == '\0' )
            break;
         hb_csEmitIndent( yyc, iIndent );
         if( szText[ 0 ] == '*' && szText[ 1 ] == ' ' )
            fprintf( yyc, "//%s\n", szText + 1 );
         else if( szText[ 0 ] == '*' )
            fprintf( yyc, "// %s\n", szText + 1 );
         else if( hb_strnicmp( szText, "NOTE ", 5 ) == 0 ||
                  hb_strnicmp( szText, "NOTE\t", 5 ) == 0 )
            fprintf( yyc, "// %s\n", szText + 5 );
         else if( szText[ 0 ] == '&' && szText[ 1 ] == '&' )
            fprintf( yyc, "//%s\n", szText + 2 );
         else
            fprintf( yyc, "%s\n", szText );
         break;
      }

      case HB_AST_PPDEFINE:
      {
         const char * sz = pNode->value.asDefine.szDefine;
         const char * p = sz;
         char szName[ 256 ];
         HB_SIZE n = 0;

         while( *p && *p != ' ' && *p != '\t' && *p != '(' && n < sizeof( szName ) - 1 )
            szName[ n++ ] = *p++;
         szName[ n ] = '\0';

         if( hb_defineMapIsLocalOwned( szName ) )
            break;

         while( *p == ' ' || *p == '\t' )
            p++;

         hb_csEmitIndent( yyc, iIndent );
         if( *p == '(' || *p == '\0' )
            fprintf( yyc, "// #define %s\n", sz );
         else if( *p == '"' || *p == '\'' )
         {
            /* verbatim string: Harbour strings have no escapes */
            char cQuote = *p;
            const char * pInner = p + 1;
            HB_SIZE nInner = strlen( pInner ), i;
            if( nInner > 0 && pInner[ nInner - 1 ] == cQuote )
               nInner--;
            fprintf( yyc, "const string %s = @\"", szName );
            for( i = 0; i < nInner; i++ )
            {
               if( pInner[ i ] == '"' )
                  fprintf( yyc, "\"\"" );
               else
                  fputc( pInner[ i ], yyc );
            }
            fprintf( yyc, "\";\n" );
         }
         else if( ( *p >= '0' && *p <= '9' ) || *p == '-' || *p == '+' )
         {
            const char * pNumStart = p;
            const char * pNumEnd = p;
            const char * pTrail;
            HB_BOOL fHasDot = HB_FALSE;

            if( *pNumEnd == '+' || *pNumEnd == '-' )
               pNumEnd++;
            while( *pNumEnd >= '0' && *pNumEnd <= '9' )
               pNumEnd++;
            if( *pNumEnd == '.' )
            {
               fHasDot = HB_TRUE;
               pNumEnd++;
               while( *pNumEnd >= '0' && *pNumEnd <= '9' )
                  pNumEnd++;
            }
            pTrail = pNumEnd;
            while( *pTrail == ' ' || *pTrail == '\t' )
               pTrail++;
            if( ! ( pTrail[ 0 ] == '/' &&
                    ( pTrail[ 1 ] == '/' || pTrail[ 1 ] == '*' ) ) )
               pTrail = NULL;

            fprintf( yyc, "const decimal %s = %.*s%s;", szName,
                     ( int ) ( pNumEnd - pNumStart ), pNumStart,
                     fHasDot ? "m" : "" );
            if( pTrail )
               fprintf( yyc, " %s", pTrail );
            fprintf( yyc, "\n" );
         }
         else if( hb_stricmp( p, ".T." ) == 0 || hb_stricmp( p, ".F." ) == 0 )
            fprintf( yyc, "const bool %s = %s;\n", szName,
                     hb_stricmp( p, ".T." ) == 0 ? "true" : "false" );
         else
            fprintf( yyc, "// #define %s\n", sz );
         break;
      }

      default:
         break;
   }
}

/* #pragma BEGINCSHARP blocks — raw C#, emitted verbatim. */
static PHB_AST_NODE hb_csFileDeclFirst( void )
{
   PHB_AST_NODE pFirst = s_pCompCtx ? s_pCompCtx->ast.pFuncList : NULL;
   if( ! pFirst || pFirst->type != HB_AST_FUNCTION ||
       ! pFirst->value.asFunc.pBody ||
       pFirst->value.asFunc.pBody->type != HB_AST_BLOCK )
      return NULL;
   return pFirst->value.asFunc.pBody->value.asBlock.pFirst;
}

static void hb_csEmitCSharpBlocks( FILE * yyc )
{
   PHB_AST_NODE pStmt;
   for( pStmt = hb_csFileDeclFirst(); pStmt; pStmt = pStmt->pNext )
   {
      if( pStmt->type == HB_AST_CSHARP && pStmt->value.asCSharp.szText )
      {
         const char * szText = pStmt->value.asCSharp.szText;
         HB_SIZE nLen = strlen( szText );
         fputs( szText, yyc );
         if( nLen == 0 || szText[ nLen - 1 ] != '\n' )
            fputc( '\n', yyc );
         fputc( '\n', yyc );
      }
   }
}

static HB_BOOL hb_csClassExtendedByBlock( const char * szName )
{
   PHB_AST_NODE pStmt;
   HB_SIZE nName;

   if( ! szName )
      return HB_FALSE;
   nName = strlen( szName );
   for( pStmt = hb_csFileDeclFirst(); pStmt; pStmt = pStmt->pNext )
   {
      const char * p;
      if( pStmt->type != HB_AST_CSHARP || ! pStmt->value.asCSharp.szText )
         continue;
      p = pStmt->value.asCSharp.szText;
      while( ( p = strstr( p, "partial class " ) ) != NULL )
      {
         p += 14;
         while( *p == ' ' || *p == '\t' )
            p++;
         if( strncmp( p, szName, nName ) == 0 &&
             ! ( HB_ISALPHA( ( HB_UCHAR ) p[ nName ] ) ||
                 HB_ISDIGIT( ( HB_UCHAR ) p[ nName ] ) || p[ nName ] == '_' ) )
            return HB_TRUE;
      }
   }
   return HB_FALSE;
}

/* ---- Class and method collection ---- */

typedef struct _HB_CS_METHOD
{
   PHB_AST_NODE         pFunc;
   PHB_HFUNC            pCompFunc;
   struct _HB_CS_METHOD * pNext;
} HB_CS_METHOD;

typedef struct _HB_CS_CLASS
{
   const char *          szName;
   PHB_AST_NODE          pClassNode;
   HB_CS_METHOD *        pMethods;
   HB_CS_METHOD *        pMethodsLast;
   HB_BOOL               fDynamic;
   struct _HB_CS_CLASS * pNext;
} HB_CS_CLASS;

static HB_CS_CLASS * hb_csFindClass( HB_CS_CLASS * pList, const char * szName )
{
   for( ; pList; pList = pList->pNext )
      if( hb_stricmp( pList->szName, szName ) == 0 )
         return pList;
   return NULL;
}

static void hb_csAddMethod( HB_CS_CLASS * pClass, PHB_AST_NODE pFunc,
                            PHB_HFUNC pCompFunc )
{
   HB_CS_METHOD * pMethod = ( HB_CS_METHOD * ) hb_xgrab( sizeof( HB_CS_METHOD ) );
   pMethod->pFunc = pFunc;
   pMethod->pCompFunc = pCompFunc;
   pMethod->pNext = NULL;
   if( pClass->pMethodsLast )
      pClass->pMethodsLast->pNext = pMethod;
   else
      pClass->pMethods = pMethod;
   pClass->pMethodsLast = pMethod;
}

static void hb_csFreeClasses( HB_CS_CLASS * pList )
{
   while( pList )
   {
      HB_CS_CLASS * pNext = pList->pNext;
      HB_CS_METHOD * pMethod = pList->pMethods;
      while( pMethod )
      {
         HB_CS_METHOD * pMNext = pMethod->pNext;
         hb_xfree( pMethod );
         pMethod = pMNext;
      }
      hb_xfree( pList );
      pList = pNext;
   }
}

/* ---- Method declaration ---- */

static void hb_csEmitMethodDecl( PHB_AST_NODE pFunc, PHB_HFUNC pCompFunc,
                                 FILE * yyc, int iIndent )
{
   const char * szRetType = NULL;
   PHB_AST_NODE pFirstStmt = NULL;
   HB_BOOL fProcedure = HB_FALSE;
   const char * szClass = NULL;
   const char * szMethName;
   char szKey[ 256 ];

   /* return type inference still needs the body */
   if( pFunc->value.asFunc.pBody )
      szRetType = hb_astPropagate( pFunc->value.asFunc.pBody, s_pClassList,
                                   s_pRefTab, NULL,
                                   s_pCompCtx ? s_pCompCtx->currModule : NULL );

   if( pFunc->value.asFunc.pBody &&
       pFunc->value.asFunc.pBody->type == HB_AST_BLOCK )
      pFirstStmt = pFunc->value.asFunc.pBody->value.asBlock.pFirst;
   if( pFirstStmt && pFirstStmt->type == HB_AST_CLASSMETHOD )
   {
      fProcedure = pFirstStmt->value.asClassMethod.fProcedure;
      szClass = pFirstStmt->value.asClassMethod.szClass;
   }

   /* reftab keys methods as <Class>::<Class>__<Method> */
   hb_strncpy( szKey, hb_refTabMethodKey( szClass, pFunc->value.asFunc.szName ),
               sizeof( szKey ) - 1 );

   szMethName = ( pFirstStmt && pFirstStmt->type == HB_AST_CLASSMETHOD &&
                  pFirstStmt->value.asClassMethod.szName )
                   ? pFirstStmt->value.asClassMethod.szName
                   : pFunc->value.asFunc.szName;

   hb_csEmitIndent( yyc, iIndent );
   fprintf( yyc, "public " );
   if( ! fProcedure && pCompFunc->wParamCount == 0 &&
       hb_stricmp( szMethName, "ToString" ) == 0 )
   {
      fprintf( yyc, "override string ToString(" );
      fProcedure = HB_FALSE;
   }
   else
   {
      if( fProcedure )
         fprintf( yyc, "void" );
      else
         fprintf( yyc, "%s", szRetType ? hb_csTypeMap( szRetType ) : "dynamic" );
      fprintf( yyc, " %s(", szMethName );
   }
   hb_csEmitParams( yyc, pFunc, pCompFunc, szKey, HB_FALSE );
   fprintf( yyc, ")\n" );
   hb_csEmitStubBody( yyc, iIndent, fProcedure );
   hb_csEmitVmStackOverload( yyc, iIndent, "public", HB_FALSE, szMethName );
}

/* ---- Class declaration ---- */

static void hb_csEmitClass( HB_CS_CLASS * pClass, FILE * yyc )
{
   PHB_AST_NODE pClassNode = pClass->pClassNode;
   PHB_AST_NODE pMember;
   HB_CS_METHOD * pMethod;

   fprintf( yyc, "public %sclass %s",
            hb_csClassExtendedByBlock( pClassNode->value.asClass.szName )
               ? "partial " : "",
            pClassNode->value.asClass.szName );
   if( pClass->fDynamic && ! pClassNode->value.asClass.szParent )
      fprintf( yyc, " : HbDynamicObject" );
   else if( pClassNode->value.asClass.szParent )
      fprintf( yyc, " : %s", pClassNode->value.asClass.szParent );
   fprintf( yyc, "\n{\n" );

   /* DATA members: fields / properties, no initialisers */
   for( pMember = pClassNode->value.asClass.pMembers; pMember;
        pMember = pMember->pNext )
   {
      const char * szType, * szScope, * szName;
      int iKind;

      if( pMember->type != HB_AST_CLASSDATA )
         continue;

      iKind   = pMember->value.asClassData.iKind;
      szName  = pMember->value.asClassData.szName;
      szScope = hb_csScopeStr( pMember->value.asClassData.iScope );
      szType  = pMember->value.asClassData.szType;
      if( ! szType && iKind != HB_AST_DATA_ACCESS && iKind != HB_AST_DATA_ASSIGN )
         szType = hb_astInferTypeFromInit( szName,
                                           pMember->value.asClassData.szInit );

      if( iKind == HB_AST_DATA_ASSIGN )
      {
         /* folded into a matching ACCESS */
         PHB_AST_NODE p;
         HB_BOOL fHasAccess = HB_FALSE;
         for( p = pClassNode->value.asClass.pMembers; p; p = p->pNext )
            if( p->type == HB_AST_CLASSDATA &&
                p->value.asClassData.iKind == HB_AST_DATA_ACCESS &&
                hb_stricmp( p->value.asClassData.szName, szName ) == 0 )
            {
               fHasAccess = HB_TRUE;
               break;
            }
         if( fHasAccess )
            continue;
         hb_csEmitIndent( yyc, 1 );
         fprintf( yyc, "%s %s %s { get; set; }\n", szScope,
                  hb_csTypeMap( szType ? szType : "USUAL" ), szName );
      }
      else if( iKind == HB_AST_DATA_ACCESS )
      {
         PHB_AST_NODE p;
         HB_BOOL fAssign = HB_FALSE;
         for( p = pMember->pNext; p; p = p->pNext )
            if( p->type == HB_AST_CLASSDATA &&
                p->value.asClassData.iKind == HB_AST_DATA_ASSIGN &&
                hb_stricmp( p->value.asClassData.szName, szName ) == 0 )
            {
               fAssign = HB_TRUE;
               break;
            }
         hb_csEmitIndent( yyc, 1 );
         fprintf( yyc, "%s %s %s { get;%s }\n", szScope,
                  hb_csTypeMap( szType ? szType : "USUAL" ), szName,
                  fAssign ? " set;" : "" );
      }
      else if( iKind == HB_AST_DATA_CLASS )
      {
         hb_csEmitIndent( yyc, 1 );
         fprintf( yyc, "%s static %s %s;\n", szScope, hb_csTypeMap( szType ),
                  szName );
      }
      else if( pMember->value.asClassData.fReadOnly )
      {
         hb_csEmitIndent( yyc, 1 );
         fprintf( yyc, "%s %s %s { get; }\n", szScope, hb_csTypeMap( szType ),
                  szName );
      }
      else
      {
         hb_csEmitIndent( yyc, 1 );
         fprintf( yyc, "%s %s %s;\n", szScope, hb_csTypeMap( szType ), szName );
      }
   }

   fprintf( yyc, "\n" );

   /* INLINE methods: declaration only, body dropped */
   for( pMember = pClassNode->value.asClass.pMembers; pMember;
        pMember = pMember->pNext )
   {
      const char * szScope, * szName, * szParms;

      if( pMember->type != HB_AST_CLASSMETHOD ||
          ! pMember->value.asClassMethod.szInline )
         continue;

      szScope = hb_csScopeStr( pMember->value.asClassMethod.iScope );
      szName  = pMember->value.asClassMethod.szName;
      szParms = pMember->value.asClassMethod.szParams;

      hb_csEmitIndent( yyc, 1 );
      if( pMember->value.asClassMethod.fMessageAlias &&
          ( ! szParms || ! *szParms ) )
      {
         fprintf( yyc, "%s dynamic %s { get; set; }\n", szScope, szName );
         continue;
      }

      fprintf( yyc, "%s dynamic %s(", szScope, szName );
      if( szParms && *szParms )
      {
         const char * q = szParms;
         HB_BOOL fFirst = HB_TRUE;
         while( *q )
         {
            const char * pStart;
            while( *q == ' ' || *q == ',' )
               q++;
            if( ! *q )
               break;
            pStart = q;
            while( *q && *q != ',' && *q != ' ' )
               q++;
            if( ! fFirst )
               fprintf( yyc, ", " );
            fprintf( yyc, "dynamic %.*s = default", ( int ) ( q - pStart ), pStart );
            fFirst = HB_FALSE;
         }
      }
      fprintf( yyc, ")\n" );
      hb_csEmitStubBody( yyc, 1, HB_FALSE );
      hb_csEmitVmStackOverload( yyc, 1, szScope, HB_FALSE, szName );
   }

   /* METHOD implementations: signature + empty body */
   for( pMethod = pClass->pMethods; pMethod; pMethod = pMethod->pNext )
   {
      const char * szMethName = pMethod->pFunc->value.asFunc.szName;
      HB_BOOL fSkip = HB_FALSE;
      PHB_AST_NODE pFirst =
         ( pMethod->pFunc->value.asFunc.pBody &&
           pMethod->pFunc->value.asFunc.pBody->type == HB_AST_BLOCK )
            ? pMethod->pFunc->value.asFunc.pBody->value.asBlock.pFirst : NULL;

      if( pFirst && pFirst->type == HB_AST_CLASSMETHOD &&
          pFirst->value.asClassMethod.szName )
         szMethName = pFirst->value.asClassMethod.szName;

      /* implementation of an ACCESS / ASSIGN property is the property */
      for( pMember = pClassNode->value.asClass.pMembers; pMember && ! fSkip;
           pMember = pMember->pNext )
      {
         if( pMember->type == HB_AST_CLASSDATA &&
             ( pMember->value.asClassData.iKind == HB_AST_DATA_ACCESS ||
               pMember->value.asClassData.iKind == HB_AST_DATA_ASSIGN ) )
         {
            const char * szProp = pMember->value.asClassData.szName;
            if( hb_stricmp( szMethName, szProp ) == 0 ||
                ( szMethName[ 0 ] == '_' &&
                  hb_stricmp( szMethName + 1, szProp ) == 0 ) )
               fSkip = HB_TRUE;
         }
      }
      if( fSkip )
         continue;

      hb_csEmitMethodDecl( pMethod->pFunc, pMethod->pCompFunc, yyc, 1 );
      if( pMethod->pNext )
         fprintf( yyc, "\n" );
   }

   fprintf( yyc, "}\n" );
}

/* ---- Standalone function declaration ---- */

static void hb_csEmitFuncDecl( PHB_AST_NODE pFunc, PHB_HFUNC pCompFunc,
                               FILE * yyc, int iIndent )
{
   const char * szRetType = NULL;
   HB_BOOL fIsMain = hb_stricmp( pFunc->value.asFunc.szName, "Main" ) == 0;
   HB_BOOL fVoid;
   char szKeyBuf[ 256 ], szMangledBuf[ 256 ];
   const char * szKey;

   if( pFunc->value.asFunc.pBody )
      szRetType = hb_astPropagate( pFunc->value.asFunc.pBody, s_pClassList,
                                   s_pRefTab, NULL,
                                   s_pCompCtx ? s_pCompCtx->currModule : NULL );

   szKey = fIsMain ? "Main"
      : hb_csFuncRefKey( pFunc->value.asFunc.szName, szKeyBuf, sizeof( szKeyBuf ) );
   fVoid = pFunc->value.asFunc.fProcedure || fIsMain;

   fprintf( yyc, "\n" );
   hb_csEmitIndent( yyc, iIndent );
   fprintf( yyc, "public static %s %s(",
            fVoid ? "void" : ( szRetType ? hb_csTypeMap( szRetType ) : "dynamic" ),
            fIsMain ? "Main"
                    : hb_csMangleStaticFunc( pFunc->value.asFunc.szName,
                                             szMangledBuf, sizeof( szMangledBuf ) ) );
   hb_csEmitParams( yyc, pFunc, pCompFunc, szKey, fIsMain );
   fprintf( yyc, ")\n" );
   hb_csEmitStubBody( yyc, iIndent, fVoid );

   if( ! fIsMain )
   {
      hb_csEmitShortOverload( yyc, pFunc, pCompFunc, szKey, iIndent );
      hb_csEmitVmStackOverload( yyc, iIndent, "public", HB_TRUE,
                                hb_csMangleStaticFunc( pFunc->value.asFunc.szName,
                                                       szMangledBuf,
                                                       sizeof( szMangledBuf ) ) );
   }
}

/* ---- Main entry point ---- */

/* True when any function body of the file declares a STATIC. */
static HB_BOOL hb_csHasFileStatics( void )
{
   PHB_AST_NODE pF;
   for( pF = s_pCompCtx->ast.pFuncList; pF; pF = pF->pNext )
   {
      PHB_AST_NODE pStmt;
      if( pF->type != HB_AST_FUNCTION || ! pF->value.asFunc.pBody ||
          pF->value.asFunc.pBody->type != HB_AST_BLOCK )
         continue;
      for( pStmt = pF->value.asFunc.pBody->value.asBlock.pFirst; pStmt;
           pStmt = pStmt->pNext )
         if( pStmt->type == HB_AST_STATIC )
            return HB_TRUE;
   }
   return HB_FALSE;
}

void hb_compGenCSharp0( HB_COMP_DECL, PHB_FNAME pFileName )
{
   char szFileName[ HB_PATH_MAX ];
   PHB_AST_NODE pFunc;
   PHB_AST_NODE pFileDeclFunc;
   FILE * yyc;
   HB_CS_CLASS * pClassList = NULL;
   HB_CS_CLASS * pClassLast = NULL;
   HB_BOOL fHasStandalone = HB_FALSE;

   s_pCompCtx = HB_COMP_PARAM;
   s_iFileMemvarCount = 0;
   s_iFileStaticFuncCount = 0;
   s_szFileBase[ 0 ] = '\0';
   pFileDeclFunc = HB_COMP_PARAM->ast.pFuncList;

   /* Output file name and file-base prefix */
   {
      PHB_FNAME pOut = hb_fsFNameSplit( HB_COMP_PARAM->szFile );
      pFileName->szExtension = "0.cs";
      if( HB_COMP_PARAM->pOutPath && HB_COMP_PARAM->pOutPath->szPath )
         pFileName->szPath = HB_COMP_PARAM->pOutPath->szPath;
      else if( pOut->szPath )
         pFileName->szPath = pOut->szPath;
      hb_fsFNameMerge( szFileName, pFileName );
      if( pOut->szName )
      {
         const char * szCanon = hb_fileCaseLookup( pOut->szName );
         hb_strncpy( s_szFileBase, szCanon ? szCanon : pOut->szName,
                     sizeof( s_szFileBase ) - 1 );
         {
            char szBasename[ HB_PATH_MAX ];
            hb_snprintf( szBasename, sizeof( szBasename ), "%s%s", pOut->szName,
                         pOut->szExtension ? pOut->szExtension : ".prg" );
            hb_defineMapSetCurrentFile( szBasename );
         }
      }
      else
         hb_defineMapSetCurrentFile( NULL );
      hb_xfree( pOut );
   }

   yyc = hb_fopen( szFileName, "w" );
   if( ! yyc )
   {
      hb_compGenError( HB_COMP_PARAM, hb_comp_szErrors, 'E',
                       HB_COMP_ERR_CREATE_OUTPUT, szFileName, NULL );
      return;
   }

   /* Signature table: on-disk scan merged with this file */
   s_pRefTab = hb_refTabNew();
   hb_refTabLoad( s_pRefTab, hb_refTabGetPath() );
   hb_refTabCollect( s_pRefTab, HB_COMP_PARAM );
   hb_astSetPrefixReftab( s_pRefTab );

   /* STATIC FUNCTION / PROCEDURE names */
   for( pFunc = HB_COMP_PARAM->ast.pFuncList; pFunc; pFunc = pFunc->pNext )
      if( pFunc->type == HB_AST_FUNCTION && pFunc->value.asFunc.szName &&
          ( pFunc->value.asFunc.cScope & HB_FS_STATIC ) != 0 )
         hb_csAddFileStaticFunc( pFunc->value.asFunc.szName );

   /* CLASS nodes from the file-decl function */
   s_pClassList = hb_csFileDeclFirst();
   {
      PHB_AST_NODE pStmt;
      for( pStmt = s_pClassList; pStmt; pStmt = pStmt->pNext )
      {
         HB_CS_CLASS * pClass;
         if( pStmt->type != HB_AST_CLASS )
            continue;
         pClass = ( HB_CS_CLASS * ) hb_xgrab( sizeof( HB_CS_CLASS ) );
         pClass->szName = pStmt->value.asClass.szName;
         pClass->pClassNode = pStmt;
         pClass->pMethods = NULL;
         pClass->pMethodsLast = NULL;
         pClass->fDynamic = HB_FALSE;
         pClass->pNext = NULL;
         if( pClassLast )
            pClassLast->pNext = pClass;
         else
            pClassList = pClass;
         pClassLast = pClass;
      }
   }

   /* Sort functions into class methods and standalone functions */
   {
      PHB_HFUNC pCompFunc = HB_COMP_PARAM->functions.pFirst;
      for( pFunc = HB_COMP_PARAM->ast.pFuncList; pFunc; pFunc = pFunc->pNext )
      {
         if( pFunc->type != HB_AST_FUNCTION )
            continue;
         while( pCompFunc && ( pCompFunc->funFlags & HB_FUNF_FILE_DECL ) )
            pCompFunc = pCompFunc->pNext;

         if( pCompFunc && ! ( pCompFunc->funFlags & HB_FUNF_FILE_FIRST ) )
         {
            PHB_AST_NODE pFirstStmt = NULL;
            const char * szClassName = NULL;

            if( pFunc->value.asFunc.pBody &&
                pFunc->value.asFunc.pBody->type == HB_AST_BLOCK )
               pFirstStmt = pFunc->value.asFunc.pBody->value.asBlock.pFirst;
            if( pFirstStmt && pFirstStmt->type == HB_AST_CLASSMETHOD &&
                pFirstStmt->value.asClassMethod.szClass )
               szClassName = pFirstStmt->value.asClassMethod.szClass;

            if( szClassName )
            {
               HB_CS_CLASS * pClass = hb_csFindClass( pClassList, szClassName );
               if( pClass )
               {
                  hb_csAddMethod( pClass, pFunc, pCompFunc );
                  if( ! pClass->fDynamic &&
                      hb_csBlockHasMacroSend( pFunc->value.asFunc.pBody ) )
                     pClass->fDynamic = HB_TRUE;
               }
            }
            else if( pFunc->value.asFunc.pBody &&
                     pFunc->value.asFunc.pBody->value.asBlock.pFirst )
               fHasStandalone = HB_TRUE;
         }
         if( pCompFunc )
            pCompFunc = pCompFunc->pNext;
      }
   }

   /* ---- Emit ---- */

   fprintf( yyc, "using System;\n" );
   fprintf( yyc, "using System.Collections.Generic;\n" );
   fprintf( yyc, "using static HbRuntime;\n" );
   fprintf( yyc, "using static Program;\n" );
   fprintf( yyc, "\n" );

   {
      PHB_AST_NODE pStmt;
      for( pStmt = hb_csFileDeclFirst(); pStmt; pStmt = pStmt->pNext )
         if( pStmt->type == HB_AST_INCLUDE || pStmt->type == HB_AST_COMMENT )
            hb_csEmitHeaderNode( pStmt, yyc, 0 );
   }

   hb_csEmitCSharpBlocks( yyc );

   {
      HB_CS_CLASS * pClass;
      for( pClass = pClassList; pClass; pClass = pClass->pNext )
      {
         hb_csEmitClass( pClass, yyc );
         fprintf( yyc, "\n" );
      }
   }

   /* File-scope STATICs need the Program partial even when every function
      in the file is a class method. */
   if( hb_csHasFileStatics() )
      fHasStandalone = HB_TRUE;

   if( fHasStandalone )
   {
      PHB_HFUNC pCompFunc = HB_COMP_PARAM->functions.pFirst;
      PHB_AST_NODE pF;

      fprintf( yyc, "public static partial class Program\n{\n" );

      /* #define constants */
      {
         PHB_AST_NODE pStmt;
         for( pStmt = hb_csFileDeclFirst(); pStmt; pStmt = pStmt->pNext )
            if( pStmt->type == HB_AST_PPDEFINE )
               hb_csEmitHeaderNode( pStmt, yyc, 1 );
      }

      /* STATIC / MEMVAR / PUBLIC fields — declarations only */
      for( pF = HB_COMP_PARAM->ast.pFuncList; pF; pF = pF->pNext )
      {
         PHB_AST_NODE pStmt;
         const char * szOwner;

         if( pF->type != HB_AST_FUNCTION || ! pF->value.asFunc.pBody ||
             pF->value.asFunc.pBody->type != HB_AST_BLOCK )
            continue;
         /* file-decl head: file scope; else private to that function */
         szOwner = ( pF == pFileDeclFunc ) ? NULL : pF->value.asFunc.szName;

         for( pStmt = pF->value.asFunc.pBody->value.asBlock.pFirst; pStmt;
              pStmt = pStmt->pNext )
         {
            if( pStmt->type == HB_AST_STATIC )
            {
               const char * szName = pStmt->value.asVar.szName;
               const char * szType = pStmt->value.asVar.szAlias
                  ? pStmt->value.asVar.szAlias
                  : hb_astInferType( szName, pStmt->value.asVar.pInit );
               HB_BOOL fArrayDim = pStmt->value.asVar.fArrayDim &&
                  pStmt->value.asVar.pInit &&
                  ( pStmt->value.asVar.pInit->ExprType == HB_ET_ARGLIST ||
                    pStmt->value.asVar.pInit->ExprType == HB_ET_LIST );

               hb_csEmitIndent( yyc, 1 );
               fprintf( yyc, "public static %s ",
                        fArrayDim ? "dynamic[]" : hb_csTypeMap( szType ) );
               if( szOwner )
                  fprintf( yyc, "%s_%s_%s;\n", s_szFileBase, szOwner, szName );
               else
                  fprintf( yyc, "%s_%s;\n", s_szFileBase, szName );
            }
            else if( pStmt->type == HB_AST_MEMVAR )
            {
               const char * szName = pStmt->value.asVar.szName;
               if( ! hb_csIsFileMemvar( szName ) &&
                   ! hb_refTabIsPublic( s_pRefTab, szName ) )
               {
                  hb_csAddFileMemvar( szName );
                  hb_csEmitIndent( yyc, 1 );
                  fprintf( yyc, "public static dynamic %s_%s;\n",
                           s_szFileBase, szName );
               }
            }
            else if( pStmt->type == HB_AST_PUBLIC )
            {
               const char * szName = pStmt->value.asVar.szName;
               if( szName && ! hb_csIsFileMemvar( szName ) )
               {
                  const char * szPubOwner = hb_refTabPublicOwner( s_pRefTab, szName );
                  if( szPubOwner && hb_stricmp( szPubOwner, s_szFileBase ) == 0 )
                  {
                     hb_csAddFileMemvar( szName );
                     hb_csEmitIndent( yyc, 1 );
                     fprintf( yyc, "public static dynamic%s %s;\n",
                              pStmt->value.asVar.fArrayDim ? "[]" : "", szName );
                  }
               }
            }
         }
      }

      /* Function declarations */
      for( pFunc = HB_COMP_PARAM->ast.pFuncList; pFunc; pFunc = pFunc->pNext )
      {
         if( pFunc->type != HB_AST_FUNCTION )
            continue;
         while( pCompFunc && ( pCompFunc->funFlags & HB_FUNF_FILE_DECL ) )
            pCompFunc = pCompFunc->pNext;

         if( pCompFunc && ! ( pCompFunc->funFlags & HB_FUNF_FILE_FIRST ) )
         {
            PHB_AST_NODE pFirstStmt = NULL;
            if( pFunc->value.asFunc.pBody &&
                pFunc->value.asFunc.pBody->type == HB_AST_BLOCK )
               pFirstStmt = pFunc->value.asFunc.pBody->value.asBlock.pFirst;

            if( ! ( pFirstStmt && pFirstStmt->type == HB_AST_CLASSMETHOD &&
                    pFirstStmt->value.asClassMethod.szClass ) &&
                pFirstStmt )
               hb_csEmitFuncDecl( pFunc, pCompFunc, yyc, 1 );
         }
         if( pCompFunc )
            pCompFunc = pCompFunc->pNext;
      }

      fprintf( yyc, "}\n" );
   }

   /* Cleanup */
   hb_astSetPrefixReftab( NULL );
   hb_csFreeClasses( pClassList );
   hb_refTabFree( s_pRefTab );
   s_pRefTab = NULL;
   s_iFileMemvarCount = 0;
   s_iFileStaticFuncCount = 0;
   s_szFileBase[ 0 ] = '\0';
   fclose( yyc );

   if( ! HB_COMP_PARAM->fQuiet )
   {
      char buffer[ HB_PATH_MAX + 64 ];
      hb_snprintf( buffer, sizeof( buffer ),
                   "Generating C# declarations to '%s'... Done.\n", szFileName );
      hb_compOutStd( HB_COMP_PARAM, buffer );
   }
}
