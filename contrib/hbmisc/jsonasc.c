/*
 * JavaScript Object Notation (JSON)
 *
 * Copyright 2010 Mindaugas Kavaliauskas <dbtopas / at / dbtopas.lt>
 * Copyright 2015 Przemyslaw Czerpak <druzus / at / priv.onet.pl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.txt.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA (or visit https://www.gnu.org/licenses/).
 *
 * As a special exception, the Harbour Project gives permission for
 * additional uses of the text contained in its release of Harbour.
 *
 * The exception is that, if you link the Harbour libraries with other
 * files to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the Harbour library code into it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 *
 * This exception applies only to the code released by the Harbour
 * Project under the name Harbour.  If you copy code from other
 * Harbour Project or Free Software Foundation releases into a copy of
 * Harbour, as the General Public License permits, the exception does
 * not apply to the code that you add in this way.  To avoid misleading
 * anyone as to the status of such modified files, you must delete
 * this exception notice from them.
 *
 * If you write modifications of your own for Harbour, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.
 *
 */

#include "hbapi.h"
#include "hbapiitm.h"
#include "hbapistr.h"
#include "hbset.h"

/*
   7-bit (JSON) serializer, special use cases only
   see /src/rtl/hbjson.c for regular human-readable emitter

      C level functions:
        char * hb_jsonEncode7Bit( PHB_ITEM pValue, HB_SIZE * pnLen, int iIndent );
           pValue  - value to encode;
           pnLen   - if pnLen is not NULL, length of returned buffer is
                     stored to *pnLen;
           iIndent - indenting to be human readable;
           returns pointer to encoded JSON buffer. buffer must be fried
              by the caller.

      Harbour level functions:
        hb_jsonEncode7Bit( xValue [, lHuman = .F. | nIndent = 0 ]
                         [, cTargetCP = NIL ] --> cJSON

 */

typedef struct
{
   char *  pBuffer;
   char *  pHead;
   HB_SIZE nAlloc;
   void ** pId;
   HB_SIZE nAllocId;
   int     iIndent;
   int     iEolLen;
   const char * szEol;
} HB_JSON_ENCODE_CTX, * PHB_JSON_ENCODE_CTX;

#define INDENT_SIZE  2

static void _hb_jsonCtxAdd( PHB_JSON_ENCODE_CTX pCtx, const char * szString, HB_SIZE nLen )
{
   if( pCtx->pHead + nLen >= pCtx->pBuffer + pCtx->nAlloc )
   {
      HB_SIZE nSize = pCtx->pHead - pCtx->pBuffer;

      pCtx->nAlloc += ( pCtx->nAlloc << 1 ) + nLen;
      pCtx->pBuffer = ( char * ) hb_xrealloc( pCtx->pBuffer, pCtx->nAlloc );
      pCtx->pHead = pCtx->pBuffer + nSize;
   }
   if( szString )
   {
      hb_xmemcpy( pCtx->pHead, szString, nLen );
      pCtx->pHead += nLen;
   }
}
static void _hb_jsonCtxAddIndent( PHB_JSON_ENCODE_CTX pCtx, HB_SIZE nLevel )
{
   if( nLevel > 0 )
   {
      HB_SIZE nCount = nLevel * ( pCtx->iIndent > 0 ? pCtx->iIndent : 1 );
      if( pCtx->pHead + nCount >= pCtx->pBuffer + pCtx->nAlloc )
      {
         HB_SIZE nSize = pCtx->pHead - pCtx->pBuffer;

         pCtx->nAlloc += ( pCtx->nAlloc << 1 ) + nCount;
         pCtx->pBuffer = ( char * ) hb_xrealloc( pCtx->pBuffer, pCtx->nAlloc );
         pCtx->pHead = pCtx->pBuffer + nSize;
      }
      hb_xmemset( pCtx->pHead, pCtx->iIndent > 0 ? ' ' : '\t', nCount );
      pCtx->pHead += nCount;
   }
}

static void _hb_jsonEncode7Bit( PHB_ITEM pValue, PHB_JSON_ENCODE_CTX pCtx,
                            HB_SIZE nLevel, HB_BOOL fEOL, PHB_CODEPAGE cdp )
{
   /* Protection against recursive structures */
   if( ( HB_IS_ARRAY( pValue ) || HB_IS_HASH( pValue ) ) && hb_itemSize( pValue ) > 0 )
   {
      void * id = HB_IS_HASH( pValue ) ? hb_hashId( pValue ) : hb_arrayId( pValue );
      HB_SIZE nIndex;

      for( nIndex = 0; nIndex < nLevel; nIndex++ )
      {
         if( pCtx->pId[ nIndex ] == id )
         {
            if( ! fEOL && pCtx->iIndent )
               _hb_jsonCtxAddIndent( pCtx, nLevel );
            _hb_jsonCtxAdd( pCtx, "null", 4 );
            return;
         }
      }
      if( nLevel >= pCtx->nAllocId )
      {
         pCtx->nAllocId += 8;
         pCtx->pId = ( void ** ) hb_xrealloc( pCtx->pId, sizeof( void * ) * pCtx->nAllocId );
      }
      pCtx->pId[ nLevel ] = id;
   }

   if( fEOL )
   {
      --pCtx->pHead;
      _hb_jsonCtxAdd( pCtx, pCtx->szEol, pCtx->iEolLen );
   }

   if( HB_IS_STRING( pValue ) )
   {
      const char * szString = hb_itemGetCPtr( pValue );
      HB_SIZE nPos, nLen = hb_itemGetCLen( pValue );

      char buf[ 8 ];

      _hb_jsonCtxAdd( pCtx, "\"", 1 );

      if( cdp )
      {
         HB_WCHAR wc;

         nPos = 0;
         while( HB_CDPCHAR_GET( cdp, szString, nLen, &nPos, &wc ) )
         {
            if( wc >= ' ' && wc < 0x7F && wc != '\\' && wc != '\"' )
            {
               buf[ 0 ] = ( char ) wc;
               _hb_jsonCtxAdd( pCtx, buf, 1 );
               continue;
            }
            switch( wc )
            {
               case '\\':
                  _hb_jsonCtxAdd( pCtx, "\\\\", 2 );
                  break;
               case '\"':
                  _hb_jsonCtxAdd( pCtx, "\\\"", 2 );
                  break;
               case '\b':
                  _hb_jsonCtxAdd( pCtx, "\\b", 2 );
                  break;
               case '\f':
                  _hb_jsonCtxAdd( pCtx, "\\f", 2 );
                  break;
               case '\n':
                  _hb_jsonCtxAdd( pCtx, "\\n", 2 );
                  break;
               case '\r':
                  _hb_jsonCtxAdd( pCtx, "\\r", 2 );
                  break;
               case '\t':
                  _hb_jsonCtxAdd( pCtx, "\\t", 2 );
                  break;
               default:
                  hb_snprintf( buf, sizeof( buf ), "\\u%04X", wc );
                  _hb_jsonCtxAdd( pCtx, buf, 6 );
                  break;
            }
         }
      }
      _hb_jsonCtxAdd( pCtx, "\"", 1 );
   }
   else if( HB_IS_NUMINT( pValue ) )
   {
      char buf[ 24 ];
      HB_MAXINT nVal = hb_itemGetNInt( pValue );
      HB_BOOL fNeg = nVal < 0;
      int i = 0;

      if( fNeg )
         nVal = -nVal;
      do
         buf[ sizeof( buf ) - ++i ] = ( nVal % 10 ) + '0';
      while( ( nVal /= 10 ) != 0 );
      if( fNeg )
         buf[ sizeof( buf ) - ++i ] = '-';
      _hb_jsonCtxAdd( pCtx, &buf[ sizeof( buf ) - i ], i );
   }
   else if( HB_IS_NUMERIC( pValue ) )
   {
      char buf[ 64 ];
      int iDec;
      double dblValue = hb_itemGetNDDec( pValue, &iDec );

      hb_snprintf( buf, sizeof( buf ), "%.*f", iDec, dblValue );
      _hb_jsonCtxAdd( pCtx, buf, strlen( buf ) );
   }
   else if( HB_IS_NIL( pValue ) )
   {
      _hb_jsonCtxAdd( pCtx, "null", 4 );
   }
   else if( HB_IS_LOGICAL( pValue ) )
   {
      if( hb_itemGetL( pValue ) )
         _hb_jsonCtxAdd( pCtx, "true", 4 );
      else
         _hb_jsonCtxAdd( pCtx, "false", 5 );

   }
   else if( HB_IS_DATE( pValue ) )
   {
      char szBuffer[ 10 ];

      hb_itemGetDS( pValue, szBuffer + 1 );
      szBuffer[ 0 ] = '\"';
      szBuffer[ 9 ] = '\"';
      _hb_jsonCtxAdd( pCtx, szBuffer, 10 );
   }
   else if( HB_IS_TIMESTAMP( pValue ) )
   {
      char szBuffer[ 19 ];
      hb_itemGetTS( pValue, szBuffer + 1 );
      szBuffer[ 0 ] = '\"';
      szBuffer[ 18 ] = '\"';
      _hb_jsonCtxAdd( pCtx, szBuffer, 19 );
   }
   else if( HB_IS_ARRAY( pValue ) )
   {
      HB_SIZE nLen = hb_itemSize( pValue );

      if( nLen )
      {
         HB_SIZE nIndex;

         if( pCtx->iIndent )
            _hb_jsonCtxAddIndent( pCtx, nLevel );

         _hb_jsonCtxAdd( pCtx, "[", 1 );

         for( nIndex = 1; nIndex <= nLen; nIndex++ )
         {
            PHB_ITEM pItem = hb_arrayGetItemPtr( pValue, nIndex );

            if( nIndex > 1 )
               _hb_jsonCtxAdd( pCtx, ",", 1 );

            if( pCtx->iIndent )
               _hb_jsonCtxAdd( pCtx, pCtx->szEol, pCtx->iEolLen );

            if( pCtx->iIndent &&
                ! ( ( HB_IS_ARRAY( pItem ) || HB_IS_HASH( pItem ) ) &&
                    hb_itemSize( pItem ) > 0 ) )
               _hb_jsonCtxAddIndent( pCtx, ( nLevel + 1 ) );

            _hb_jsonEncode7Bit( pItem, pCtx, nLevel + 1, HB_FALSE, cdp );
         }
         if( pCtx->iIndent )
         {
            _hb_jsonCtxAdd( pCtx, pCtx->szEol, pCtx->iEolLen );
            _hb_jsonCtxAddIndent( pCtx, nLevel );
         }
         _hb_jsonCtxAdd( pCtx, "]", 1 );
      }
      else
         _hb_jsonCtxAdd( pCtx, "[]", 2 );
   }
   else if( HB_IS_HASH( pValue ) )
   {
      HB_SIZE nLen = hb_hashLen( pValue );

      if( nLen )
      {
         HB_SIZE nIndex;

         if( pCtx->iIndent )
            _hb_jsonCtxAddIndent( pCtx, nLevel );

         _hb_jsonCtxAdd( pCtx, "{", 1 );

         for( nIndex = 1; nIndex <= nLen; nIndex++ )
         {
            PHB_ITEM pKey = hb_hashGetKeyAt( pValue, nIndex );

            if( HB_IS_STRING( pKey ) )
            {
               PHB_ITEM pItem = hb_hashGetValueAt( pValue, nIndex );

               if( nIndex > 1 )
                  _hb_jsonCtxAdd( pCtx, ",", 1 );

               if( pCtx->iIndent )
               {
                  _hb_jsonCtxAdd( pCtx, pCtx->szEol, pCtx->iEolLen );
                  _hb_jsonCtxAddIndent( pCtx, ( nLevel + 1 ) );
               }
               _hb_jsonEncode7Bit( pKey, pCtx, nLevel + 1, HB_FALSE, cdp );

               if( pCtx->iIndent )
               {
                  _hb_jsonCtxAdd( pCtx, ": ", 2 );
                  fEOL = ( HB_IS_ARRAY( pItem ) || HB_IS_HASH( pItem ) ) && hb_itemSize( pItem ) > 0;
               }
               else
               {
                  _hb_jsonCtxAdd( pCtx, ":", 1 );
                  fEOL = HB_FALSE;
               }

               _hb_jsonEncode7Bit( pItem, pCtx, nLevel + 1, fEOL, cdp );
            }
         }
         if( pCtx->iIndent )
         {
            _hb_jsonCtxAdd( pCtx, pCtx->szEol, pCtx->iEolLen );
            _hb_jsonCtxAddIndent( pCtx, nLevel );
         }
         _hb_jsonCtxAdd( pCtx, "}", 1 );
      }
      else
         _hb_jsonCtxAdd( pCtx, "{}", 2 );
   }
   else
   {
      /* All unsupported types are replaced by null */
      _hb_jsonCtxAdd( pCtx, "null", 4 );
   }
}

/* C level API functions */

static char * hb_jsonEncodeCP7Bit( PHB_ITEM pValue, HB_SIZE * pnLen, int iIndent, PHB_CODEPAGE cdp )
{
   PHB_JSON_ENCODE_CTX pCtx;
   char * szRet;
   HB_SIZE nLen;

   pCtx = ( PHB_JSON_ENCODE_CTX ) hb_xgrab( sizeof( HB_JSON_ENCODE_CTX ) );
   pCtx->nAlloc = 16;
   pCtx->pHead = pCtx->pBuffer = ( char * ) hb_xgrab( pCtx->nAlloc );
   pCtx->nAllocId = 8;
   pCtx->pId = ( void ** ) hb_xgrab( sizeof( void * ) * pCtx->nAllocId );
   pCtx->iIndent = iIndent;
   pCtx->szEol = hb_setGetEOL();
   if( ! pCtx->szEol || ! pCtx->szEol[ 0 ] )
      pCtx->szEol = hb_conNewLine();
   pCtx->iEolLen = ( int ) strlen( pCtx->szEol );

   _hb_jsonEncode7Bit( pValue, pCtx, 0, HB_FALSE, cdp );
   if( iIndent )
      _hb_jsonCtxAdd( pCtx, pCtx->szEol, pCtx->iEolLen );

   nLen = pCtx->pHead - pCtx->pBuffer;
   szRet = ( char * ) hb_xrealloc( pCtx->pBuffer, nLen + 1 );
   szRet[ nLen ] = '\0';
   hb_xfree( pCtx->pId );
   hb_xfree( pCtx );
   if( pnLen )
      *pnLen = nLen;
   return szRet;
}

#if 0
static char * hb_jsonEncode7Bit( PHB_ITEM pValue, HB_SIZE * pnLen, int iIndent )
{
   return hb_jsonEncodeCP7Bit( pValue, pnLen, iIndent, NULL );
}
#endif

/* Harbour level API functions */

static PHB_CODEPAGE _hb_jsonSrcCdpPar( int iParam, HB_BOOL lVmCp )
{
   if( hb_pcount() >= iParam )
   {
      const char * szCdp = hb_parc( iParam );

      if( szCdp )
         return hb_cdpFindExt( szCdp );
   }

   /* use VM cdp if codepage was not specified or invalid */

   if( lVmCp )
      return hb_vmCDP();

   return NULL;
}

HB_FUNC( HB_JSONENCODE7BIT )
{
   PHB_ITEM pItem = hb_param( 1, HB_IT_ANY );

   if( pItem )
   {
      HB_SIZE nLen;
      int iIndent = hb_parl( 2 ) ? INDENT_SIZE : hb_parni( 2 );
      char * szRet = hb_jsonEncodeCP7Bit( pItem, &nLen, iIndent, _hb_jsonSrcCdpPar( 3, HB_TRUE ) );
      hb_retclen_buffer( szRet, nLen );
   }
}
