/*************************************************************************
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * Copyright 2000, 2010 Oracle and/or its affiliates.
 *
 * OpenOffice.org - a multi-platform office productivity suite
 *
 * This file is part of OpenOffice.org.
 *
 * OpenOffice.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * OpenOffice.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU Lesser General Public License
 * version 3 along with OpenOffice.org.  If not, see
 * <http://www.openoffice.org/license.html>
 * for a copy of the LGPLv3 License.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_tools.hxx"

#define _TOOLS_DEBUG_CXX

#if defined (UNX) || defined (GCC)
#include <unistd.h>
#else
#include <direct.h>
#endif

#include <time.h>
#include <cstdarg>  // combinations
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef OS2
#define INCL_DOSSEMAPHORES
#define INCL_DOSMISC
#define INCL_WINDIALOGS
#define INCL_WINSHELLDATA
#include <svpm.h>
#endif

#if defined ( WNT )
#ifdef _MSC_VER
#pragma warning (push,1)
#endif
#include <tools/svwin.h>
#ifdef _MSC_VER
#pragma warning (pop)
#endif
#endif

#include <tools/debug.hxx>
#include <rtl/string.h>

#include <vector>

#include <osl/diagnose.h>

// =======================================================================

#ifdef DBG_UTIL

// --- DbgErrors ---

static sal_Char const DbgError_ProfEnd1[]   = "DBG_PROF...() without DBG_PROFSTART(): ";
static sal_Char const DbgError_Xtor1[]      = "DBG_DTOR() or DBG_CHKTHIS() without DBG_CTOR(): ";

static sal_Char const DbgError_CtorDtor1[]  = "this == NULL in class ";
static sal_Char const DbgError_CtorDtor2[]  = "invalid this-Pointer %p in class ";
static sal_Char const DbgError_CtorDtor3[]  = "Error-Msg from Object %p in class ";

static sal_Char const DbgTrace_EnterCtor[]  = "Enter Ctor from class ";
static sal_Char const DbgTrace_LeaveCtor[]  = "Leave Ctor from class ";
static sal_Char const DbgTrace_EnterDtor[]  = "Enter Dtor from class ";
static sal_Char const DbgTrace_LeaveDtor[]  = "Leave Dtor from class ";
static sal_Char const DbgTrace_EnterMeth[]  = "Enter method from class ";
static sal_Char const DbgTrace_LeaveMeth[]  = "Leave method from class ";

// --- PointerList ---

#define PBLOCKCOUNT     1024

struct PBlock
{
    void*       aData[PBLOCKCOUNT];
    USHORT      nCount;
    PBlock*     pPrev;
    PBlock*     pNext;
};

class PointerList
{
private:
    PBlock*     pFirst;
    PBlock*     pLast;
    ULONG       nCount;

public:
                PointerList() { pFirst = NULL; pLast = NULL; nCount = 0; }
                ~PointerList();

    void        Add( const void* p );
    BOOL        Remove( const void* p );

    const void* Get( ULONG nPos ) const;
    BOOL        IsIn( const void* p ) const;
    ULONG       Count() const { return nCount; }
};

// --- Datentypen ---

#define DBG_MAXNAME     28

struct ProfType
{
    ULONG                   nCount;
    ULONG                   nTime;
    ULONG                   nMinTime;
    ULONG                   nMaxTime;
    ULONG                   nStart;
    ULONG                   nContinueTime;
    ULONG                   nContinueStart;
    sal_Char                aName[DBG_MAXNAME+1];
};

struct XtorType
{
    ULONG                   nCtorCalls;
    ULONG                   nDtorCalls;
    ULONG                   nMaxCount;
    ULONG                   nStatics;
    sal_Char                aName[DBG_MAXNAME+1];
    BOOL                    bTest;
    PointerList             aThisList;
};

struct DebugData
{
    DbgData                 aDbgData;
    USHORT                  bInit;
    DbgPrintLine            pDbgPrintMsgBox;
    DbgPrintLine            pDbgPrintWindow;
    DbgPrintLine            pDbgPrintShell;
    DbgPrintLine            pDbgPrintTestTool;
    ::std::vector< DbgPrintLine >
                            aDbgPrintUserChannels;
    PointerList*            pProfList;
    PointerList*            pXtorList;
    DbgTestSolarMutexProc   pDbgTestSolarMutex;
    pfunc_osl_printDetailedDebugMessage
                            pOldDebugMessageFunc;
    bool                    bOslIsHooked;

    DebugData()
        :bInit( FALSE )
        ,pDbgPrintMsgBox( FALSE )
        ,pDbgPrintWindow( NULL )
        ,pDbgPrintShell( NULL )
        ,pDbgPrintTestTool( NULL )
        ,pProfList( NULL )
        ,pXtorList( NULL )
        ,pDbgTestSolarMutex( NULL )
        ,pOldDebugMessageFunc( NULL )
        ,bOslIsHooked( false )
    {
        aDbgData.nTestFlags = DBG_TEST_RESOURCE | DBG_TEST_MEM_INIT;
        aDbgData.bOverwrite = TRUE;
        aDbgData.nTraceOut = DBG_OUT_NULL;
        aDbgData.nWarningOut = DBG_OUT_NULL;
        aDbgData.nErrorOut = DBG_OUT_MSGBOX;
        aDbgData.bMemInit = 0x77;
        aDbgData.bMemBound = 0x55;
        aDbgData.bMemFree = 0x33;
        aDbgData.bHookOSLAssert = TRUE;
        aDbgData.aDebugName[0] = 0;
        aDbgData.aInclFilter[0] = 0;
        aDbgData.aExclFilter[0] = 0;
        aDbgData.aInclClassFilter[0] = 0;
        aDbgData.aExclClassFilter[0] = 0;
        aDbgData.aDbgWinState[0] = 0;
    }
};

#define DBG_TEST_XTOR_EXTRA (DBG_TEST_XTOR_THIS |  DBG_TEST_XTOR_FUNC |               \
                             DBG_TEST_XTOR_EXIT |  DBG_TEST_XTOR_REPORT )

// ------------------------------
// - statische Verwaltungsdaten -
// ------------------------------

static DebugData aDebugData;

static sal_Char aCurPath[260];

static int bDbgImplInMain = FALSE;

// =======================================================================

#if defined( WNT )
static CRITICAL_SECTION aImplCritDbgSection;
#elif defined( OS2 )
static HMTX             hImplCritDbgSection = 0;
#endif
static BOOL             bImplCritDbgSectionInit = FALSE;

// -----------------------------------------------------------------------

void ImplDbgInitLock()
{
#if defined( WNT )
    InitializeCriticalSection( &aImplCritDbgSection );
#elif defined( OS2 )
    DosCreateMutexSem( NULL, &hImplCritDbgSection, 0, FALSE );
#endif
    bImplCritDbgSectionInit = TRUE;
}

// -----------------------------------------------------------------------

void ImplDbgDeInitLock()
{
#if defined( WNT )
    DeleteCriticalSection( &aImplCritDbgSection );
#elif defined( OS2 )
    DosCloseMutexSem( hImplCritDbgSection );
#endif
    bImplCritDbgSectionInit = FALSE;
}

// -----------------------------------------------------------------------

void ImplDbgLock()
{
    if ( !bImplCritDbgSectionInit )
        return;

#if defined( WNT )
    EnterCriticalSection( &aImplCritDbgSection );
#elif defined( OS2 )
    DosRequestMutexSem( hImplCritDbgSection, SEM_INDEFINITE_WAIT );
#endif
}

// -----------------------------------------------------------------------

void ImplDbgUnlock()
{
    if ( !bImplCritDbgSectionInit )
        return;

#if defined( WNT )
    LeaveCriticalSection( &aImplCritDbgSection );
#elif defined( OS2 )
    DosReleaseMutexSem( hImplCritDbgSection );
#endif
}

// =======================================================================

#if (defined WNT || defined OS2) && !defined SVX_LIGHT
//#define SV_MEMMGR //
#endif
#ifdef SV_MEMMGR
void DbgImpCheckMemory( void* p = NULL );
void DbgImpCheckMemoryDeInit();
void DbgImpMemoryInfo( sal_Char* pBuf );
#endif

#define FILE_LINEEND    "\n"

// =======================================================================

static BOOL ImplActivateDebugger( const sal_Char* pMsg )
{
#if defined( WNT )
    static sal_Char aImplDbgOutBuf[DBG_BUF_MAXLEN];
    strcpy( aImplDbgOutBuf, pMsg );
    strcat( aImplDbgOutBuf, "\r\n" );
    OutputDebugString( aImplDbgOutBuf );
    DebugBreak();
    return TRUE;
#else
    (void) pMsg; // avoid warning about unused parameter
    return FALSE;
#endif
}

// -----------------------------------------------------------------------

static BOOL ImplCoreDump()
{
#if defined( WNT )
    DebugBreak();
#else
    long* pTemp = 0;
    *pTemp = 0xCCCC;
#endif
    return TRUE;
}

// =======================================================================

static ULONG ImplGetPerfTime()
{
#if defined( WNT )
    return (ULONG)GetTickCount();
#elif defined( OS2 )
    ULONG nClock;
    DosQuerySysInfo( QSV_MS_COUNT, QSV_MS_COUNT, &nClock, sizeof( nClock ) );
    return (ULONG)nClock;
#else
    static ULONG    nImplTicksPerSecond = 0;
    static double   dImplTicksPerSecond;
    ULONG           nTicks = (ULONG)clock();

    if ( !nImplTicksPerSecond )
    {
        nImplTicksPerSecond = CLOCKS_PER_SEC;
        dImplTicksPerSecond = nImplTicksPerSecond;
    }

    double fTicks = nTicks;
    fTicks *= 1000;
    fTicks /= dImplTicksPerSecond;
    return (ULONG)fTicks;
#endif
}

// -----------------------------------------------------------------------

typedef FILE*       FILETYPE;
#define FileOpen    fopen
#define FileRead    fread
#define FileWrite   fwrite
#define FilePrintF  fprintf
#define FileClose   fclose

// =======================================================================

namespace
{
    enum ConfigSection
    {
        eOutput,
        eMemory,
        eGUI,
        eObjects,
        eTest,

        eUnknown
    };

    void lcl_lineFeed( FILETYPE _pFile )
    {
        FilePrintF( _pFile, "%s", FILE_LINEEND );
    }

    const sal_Char* lcl_getSectionName( ConfigSection _eSection )
    {
        const sal_Char* pSectionName = NULL;
        switch ( _eSection )
        {
            case eOutput    : pSectionName = "output";  break;
            case eMemory    : pSectionName = "memory";  break;
            case eGUI       : pSectionName = "gui";     break;
            case eObjects   : pSectionName = "objects"; break;
            case eTest      : pSectionName = "test";    break;
            case eUnknown:
                OSL_ASSERT(false);
                break;
        }
        return pSectionName;
    }

    ConfigSection lcl_getSectionFromName( const sal_Char* _pSectionName, size_t _nSectionNameLength )
    {
        if ( strncmp( _pSectionName, "output",  _nSectionNameLength < 6 ? _nSectionNameLength : 6 ) == 0 )
            return eOutput;
        if ( strncmp( _pSectionName, "memory",  _nSectionNameLength < 6 ? _nSectionNameLength : 6 ) == 0 )
            return eMemory;
        if ( strncmp( _pSectionName, "gui",     _nSectionNameLength < 3 ? _nSectionNameLength : 3 ) == 0 )
            return eGUI;
        if ( strncmp( _pSectionName, "objects", _nSectionNameLength < 7 ? _nSectionNameLength : 7 ) == 0 )
            return eObjects;
        if ( strncmp( _pSectionName, "test",    _nSectionNameLength < 4 ? _nSectionNameLength : 4 ) == 0 )
            return eTest;
        return eUnknown;
    }

    void lcl_startSection( FILETYPE _pFile, ConfigSection _eSection )
    {
        FilePrintF( _pFile, "[%s]%s", lcl_getSectionName( _eSection ), FILE_LINEEND );
    }

    void lcl_writeConfigString( FILETYPE _pFile, const sal_Char* _pKeyName, const sal_Char* _pValue )
    {
        FilePrintF( _pFile, "%s=%s%s", _pKeyName, _pValue, FILE_LINEEND );
    }

    void lcl_writeConfigBoolean( FILETYPE _pFile, const sal_Char* _pKeyName, bool _bValue )
    {
        lcl_writeConfigString( _pFile, _pKeyName, _bValue ? "1" : "0" );
    }

    void lcl_writeConfigFlag( FILETYPE _pFile, const sal_Char* _pKeyName, ULONG _nAllFlags, ULONG _nCheckFlag )
    {
        lcl_writeConfigBoolean( _pFile, _pKeyName, ( _nAllFlags & _nCheckFlag ) != 0 );
    }

    void lcl_writeConfigOutChannel( FILETYPE _pFile, const sal_Char* _pKeyName, ULONG _nValue )
    {
        const sal_Char* names[ DBG_OUT_COUNT ] =
        {
            "dev/null", "file", "window", "shell", "messagebox", "testtool", "debugger", "coredump"
        };
        lcl_writeConfigString( _pFile, _pKeyName, names[ _nValue ] );
    }
    void lcl_writeHexByte( FILETYPE _pFile, const sal_Char* _pKeyName, BYTE _nValue )
    {
        sal_Char buf[RTL_STR_MAX_VALUEOFINT32];
        rtl_String* stringData = NULL;
        rtl_string_newFromStr_WithLength( &stringData, buf, rtl_str_valueOfInt32( buf, _nValue, 16 ) );

        lcl_writeConfigString( _pFile, _pKeyName, stringData->buffer );

        rtl_string_release( stringData );
    }
    bool lcl_isConfigSection( const sal_Char* _pLine, size_t _nLineLen )
    {
        if ( _nLineLen < 2 )
            // not even enough space for '[' and ']'
            return false;
        if ( ( _pLine[0] == '[' ) && ( _pLine[ _nLineLen - 1 ] == ']' ) )
            return true;
        return false;
    }
    bool lcl_isConfigKey( const sal_Char* _pLine, size_t _nLineLen, const sal_Char* _pKeyName )
    {
        size_t nKeyLength = strlen( _pKeyName );
        if ( nKeyLength + 1 >= _nLineLen )
            // not even long enough for the key name plus "=" plus a one-character value
            return false;
        if ( ( strncmp( _pLine, _pKeyName, nKeyLength ) == 0 ) && ( _pLine[ nKeyLength ] == '=' ) )
            return true;
        return false;
    }
    sal_Int32 lcl_tryReadConfigString( const sal_Char* _pLine, size_t _nLineLen, const sal_Char* _pKeyName, sal_Char* _pValue, size_t _nValueLen )
    {
        if ( !lcl_isConfigKey( _pLine, _nLineLen, _pKeyName ) )
            return 0;
        size_t nValuePos = strlen( _pKeyName ) + 1;
        size_t nValueLen = _nLineLen - nValuePos;
        const sal_Char* pValue = _pLine + nValuePos;
        strncpy( _pValue, pValue, ( _nValueLen > nValueLen ) ? nValueLen : _nValueLen );
        _pValue[ ( _nValueLen > nValueLen ) ? nValueLen : _nValueLen - 1 ] = 0;
        return strlen( _pValue );
    }
    void lcl_tryReadConfigBoolean( const sal_Char* _pLine, size_t _nLineLen, const sal_Char* _pKeyName, ULONG* _out_pnValue )
    {
        sal_Char aBuf[2];
        size_t nValueLen = lcl_tryReadConfigString( _pLine, _nLineLen, _pKeyName, aBuf, sizeof( aBuf ) );
        if ( nValueLen )
            *_out_pnValue = strcmp( aBuf, "1" ) == 0 ? TRUE : FALSE;
    }
    void lcl_tryReadOutputChannel( const sal_Char* _pLine, size_t _nLineLen, const sal_Char* _pKeyName, ULONG* _out_pnValue )
    {
        const sal_Char* names[ DBG_OUT_COUNT ] =
        {
            "dev/null", "file", "window", "shell", "messagebox", "testtool", "debugger", "coredump"
        };
        sal_Char aBuf[20];
        size_t nValueLen = lcl_tryReadConfigString( _pLine, _nLineLen, _pKeyName, aBuf, sizeof( aBuf ) );
        if ( nValueLen )
        {
            for ( ULONG name = 0; name < sizeof( names ) / sizeof( names[0] ); ++name )
            {
                if ( strcmp( aBuf, names[ name ] ) == 0 )
                {
                    *_out_pnValue = name;
                    return;
                }
            }
        }
    }
    void lcl_tryReadConfigFlag( const sal_Char* _pLine, size_t _nLineLen, const sal_Char* _pKeyName, ULONG* _out_pnAllFlags, ULONG _nCheckFlag )
    {
        sal_Char aBuf[2];
        size_t nValueLen = lcl_tryReadConfigString( _pLine, _nLineLen, _pKeyName, aBuf, sizeof( aBuf ) );
        if ( nValueLen )
            if ( strcmp( aBuf, "1" ) == 0 )
                *_out_pnAllFlags |= _nCheckFlag;
            else
                *_out_pnAllFlags &= ~_nCheckFlag;
    }
    void lcl_tryReadHexByte( const sal_Char* _pLine, size_t _nLineLen, const sal_Char* _pKeyName, BYTE* _out_pnValue )
    {
        sal_Char aBuf[3];
        size_t nValueLen = lcl_tryReadConfigString( _pLine, _nLineLen, _pKeyName, aBuf, sizeof( aBuf ) );
        if ( nValueLen )
            *_out_pnValue = (BYTE)rtl_str_toInt32( aBuf, 16 );
    }
}

// =======================================================================

PointerList::~PointerList()
{
    PBlock* pBlock = pFirst;
    while ( pBlock )
    {
        PBlock* pNextBlock = pBlock->pNext;
        delete pBlock;
        pBlock = pNextBlock;
    }
}

// -----------------------------------------------------------------------

void PointerList::Add( const void* p )
{
    if ( !pFirst )
    {
        pFirst = new PBlock;
        memset( pFirst->aData, 0, PBLOCKCOUNT * sizeof( void* ) );
        pFirst->nCount = 0;
        pFirst->pPrev  = NULL;
        pFirst->pNext  = NULL;
        pLast = pFirst;
    }

    PBlock* pBlock = pFirst;
    while ( pBlock && (pBlock->nCount == PBLOCKCOUNT) )
        pBlock = pBlock->pNext;

    if ( !pBlock )
    {
        pBlock = new PBlock;
        memset( pBlock->aData, 0, PBLOCKCOUNT * sizeof( void* ) );
        pBlock->nCount = 0;
        pBlock->pPrev  = pLast;
        pBlock->pNext  = NULL;
        pLast->pNext   = pBlock;
        pLast          = pBlock;
    }

    USHORT i = 0;
    while ( pBlock->aData[i] )
        i++;

    pBlock->aData[i] = (void*)p;
    pBlock->nCount++;
    nCount++;
}

// -----------------------------------------------------------------------

BOOL PointerList::Remove( const void* p )
{
    if ( !p )
       return FALSE;

    PBlock* pBlock = pFirst;
    while ( pBlock )
    {
        USHORT i = 0;
        while ( i < PBLOCKCOUNT )
        {
            if ( ((ULONG)p) == ((ULONG)pBlock->aData[i]) )
            {
                pBlock->aData[i] = NULL;
                pBlock->nCount--;
                nCount--;

                if ( !pBlock->nCount )
                {
                    if ( pBlock->pPrev )
                        pBlock->pPrev->pNext = pBlock->pNext;
                    if ( pBlock->pNext )
                        pBlock->pNext->pPrev = pBlock->pPrev;
                    if ( pBlock == pFirst )
                        pFirst = pBlock->pNext;
                    if ( pBlock == pLast )
                        pLast = pBlock->pPrev;
                    delete pBlock;
                }

                return TRUE;
            }
            i++;
        }

        pBlock = pBlock->pNext;
    }

    return FALSE;
}

// -----------------------------------------------------------------------

const void* PointerList::Get( ULONG nPos ) const
{
    if ( nCount <= nPos )
        return NULL;

    PBlock* pBlock = pFirst;
    ULONG   nStart = 0;
    while ( pBlock )
    {
        USHORT i = 0;
        while ( i < PBLOCKCOUNT )
        {
            if ( pBlock->aData[i] )
            {
                nStart++;
                if ( (nStart-1) == nPos )
                    return pBlock->aData[i];
            }

            i++;
        }

        pBlock = pBlock->pNext;
    }

    return NULL;
}

// -----------------------------------------------------------------------

BOOL PointerList::IsIn( const void* p ) const
{
    if ( !p )
       return FALSE;

    PBlock* pBlock = pFirst;
    while ( pBlock )
    {
        USHORT i = 0;
        while ( i < PBLOCKCOUNT )
        {
            if ( ((ULONG)p) == ((ULONG)pBlock->aData[i]) )
                return TRUE;
            i++;
        }

        pBlock = pBlock->pNext;
    }

    return FALSE;
}


// =======================================================================

static void DbgGetDbgFileName( sal_Char* pStr, sal_Int32 nMaxLen )
{
#if defined( UNX )
    const sal_Char* pName = getenv("DBGSV_INIT");
    if ( !pName )
        pName = ".dbgsv.init";
    strncpy( pStr, pName, nMaxLen );
#elif defined( WNT )
    const sal_Char* pName = getenv("DBGSV_INIT");
    if ( pName )
        strncpy( pStr, pName, nMaxLen );
    else
        GetProfileStringA( "sv", "dbgsv", "dbgsv.ini", pStr, nMaxLen );
#elif defined( OS2 )
    PrfQueryProfileString( HINI_PROFILE, (PSZ)"SV", (PSZ)"DBGSV",
                           "dbgsv.ini", (PSZ)pStr, nMaxLen );
#else
    strncpy( pStr, "dbgsv.ini", nMaxLen );
#endif
    pStr[ nMaxLen - 1 ] = 0;
}

// -----------------------------------------------------------------------

static void DbgGetLogFileName( sal_Char* pStr )
{
#if defined( UNX )
    const sal_Char* pName = getenv("DBGSV_LOG");
    if ( !pName )
        pName = "dbgsv.log";
    strcpy( pStr, pName );
#elif defined( WNT )
    const sal_Char* pName = getenv("DBGSV_LOG");
    if ( pName )
        strcpy( pStr, pName );
    else
        GetProfileStringA( "sv", "dbgsvlog", "dbgsv.log", pStr, 200 );
#elif defined( OS2 )
    PrfQueryProfileString( HINI_PROFILE, (PSZ)"SV", (PSZ)"DBGSVLOG",
                           "dbgsv.log", (PSZ)pStr, 200 );
#else
    strcpy( pStr, "dbgsv.log" );
#endif
}

// -----------------------------------------------------------------------

static void DbgDebugBeep()
{
#if defined( WNT )
    MessageBeep( MB_ICONHAND );
#elif defined( OS2 )
    WinAlarm( HWND_DESKTOP, WA_ERROR );
#endif
}

// -----------------------------------------------------------------------

static DebugData* GetDebugData()
{
    if ( !aDebugData.bInit )
    {
        aDebugData.bInit = TRUE;

        // Default Debug-Namen setzen
        DbgGetLogFileName( aDebugData.aDbgData.aDebugName );

        // DEBUG.INI-File
        sal_Char aBuf[ 4096 ];
        DbgGetDbgFileName( aBuf, sizeof( aBuf ) );
        FILETYPE pIniFile = FileOpen( aBuf, "r" );
        if ( pIniFile != NULL )
        {
            ConfigSection eCurrentSection = eUnknown;

            // no sophisticated algorithm here, assume that the whole file fits into aBuf ...
            ULONG nReallyRead = FileRead( aBuf, 1, sizeof( aBuf ) / sizeof( sal_Char ) - 1, pIniFile );
            aBuf[ nReallyRead ] = 0;
            const sal_Char* pLine = aBuf;
            while ( const sal_Char* pNextLine = strstr( pLine, FILE_LINEEND ) )
            {
                size_t nLineLength = pNextLine - pLine;

                if ( lcl_isConfigSection( pLine, nLineLength ) )
                    eCurrentSection = lcl_getSectionFromName( pLine + 1, nLineLength - 2 );

                // elements of the [output] section
                if ( eCurrentSection == eOutput )
                {
                    lcl_tryReadConfigString( pLine, nLineLength, "log_file", aDebugData.aDbgData.aDebugName, sizeof( aDebugData.aDbgData.aDebugName ) );
                    lcl_tryReadConfigBoolean( pLine, nLineLength, "overwrite", &aDebugData.aDbgData.bOverwrite );
                    lcl_tryReadConfigString( pLine, nLineLength, "include", aDebugData.aDbgData.aInclFilter, sizeof( aDebugData.aDbgData.aInclFilter ) );
                    lcl_tryReadConfigString( pLine, nLineLength, "exclude", aDebugData.aDbgData.aExclFilter, sizeof( aDebugData.aDbgData.aExclFilter ) );
                    lcl_tryReadConfigString( pLine, nLineLength, "include_class", aDebugData.aDbgData.aInclClassFilter, sizeof( aDebugData.aDbgData.aInclClassFilter ) );
                    lcl_tryReadConfigString( pLine, nLineLength, "exclude_class", aDebugData.aDbgData.aExclClassFilter, sizeof( aDebugData.aDbgData.aExclClassFilter ) );
                    lcl_tryReadOutputChannel( pLine, nLineLength, "trace", &aDebugData.aDbgData.nTraceOut );
                    lcl_tryReadOutputChannel( pLine, nLineLength, "warning", &aDebugData.aDbgData.nWarningOut );
                    lcl_tryReadOutputChannel( pLine, nLineLength, "error", &aDebugData.aDbgData.nErrorOut );
                    lcl_tryReadConfigBoolean( pLine, nLineLength, "oslhook", &aDebugData.aDbgData.bHookOSLAssert );
                }

                // elements of the [memory] section
                if ( eCurrentSection == eMemory )
                {
                    lcl_tryReadConfigFlag( pLine, nLineLength, "initialize", &aDebugData.aDbgData.nTestFlags, DBG_TEST_MEM_INIT );
                    lcl_tryReadConfigFlag( pLine, nLineLength, "overwrite", &aDebugData.aDbgData.nTestFlags, DBG_TEST_MEM_OVERWRITE );
                    lcl_tryReadConfigFlag( pLine, nLineLength, "overwrite_free", &aDebugData.aDbgData.nTestFlags, DBG_TEST_MEM_OVERWRITEFREE );
                    lcl_tryReadConfigFlag( pLine, nLineLength, "pointer", &aDebugData.aDbgData.nTestFlags, DBG_TEST_MEM_POINTER );
                    lcl_tryReadConfigFlag( pLine, nLineLength, "report", &aDebugData.aDbgData.nTestFlags, DBG_TEST_MEM_REPORT );
                    lcl_tryReadConfigFlag( pLine, nLineLength, "trace", &aDebugData.aDbgData.nTestFlags, DBG_TEST_MEM_TRACE );
                    lcl_tryReadConfigFlag( pLine, nLineLength, "new_and_delete", &aDebugData.aDbgData.nTestFlags, DBG_TEST_MEM_NEWDEL );
                    lcl_tryReadConfigFlag( pLine, nLineLength, "object_test", &aDebugData.aDbgData.nTestFlags, DBG_TEST_MEM_XTOR );
                    lcl_tryReadConfigFlag( pLine, nLineLength, "sys_alloc", &aDebugData.aDbgData.nTestFlags, DBG_TEST_MEM_SYSALLOC );
                    lcl_tryReadConfigFlag( pLine, nLineLength, "leak_report", &aDebugData.aDbgData.nTestFlags, DBG_TEST_MEM_LEAKREPORT );

                    lcl_tryReadHexByte( pLine, nLineLength, "init_byte", &aDebugData.aDbgData.bMemInit );
                    lcl_tryReadHexByte( pLine, nLineLength, "bound_byte", &aDebugData.aDbgData.bMemBound );
                    lcl_tryReadHexByte( pLine, nLineLength, "free_byte", &aDebugData.aDbgData.bMemFree );
                }

                // elements of the [gui] section
                if ( eCurrentSection == eGUI )
                {
                    lcl_tryReadConfigString( pLine, nLineLength, "debug_window_state", aDebugData.aDbgData.aDbgWinState, sizeof( aDebugData.aDbgData.aDbgWinState ) );
                }

                // elements of the [objects] section
                if ( eCurrentSection == eObjects )
                {
                    lcl_tryReadConfigFlag( pLine, nLineLength, "check_this", &aDebugData.aDbgData.nTestFlags, DBG_TEST_XTOR_THIS );
                    lcl_tryReadConfigFlag( pLine, nLineLength, "check_function", &aDebugData.aDbgData.nTestFlags, DBG_TEST_XTOR_FUNC );
                    lcl_tryReadConfigFlag( pLine, nLineLength, "check_exit", &aDebugData.aDbgData.nTestFlags, DBG_TEST_XTOR_EXIT );
                    lcl_tryReadConfigFlag( pLine, nLineLength, "generate_report", &aDebugData.aDbgData.nTestFlags, DBG_TEST_XTOR_REPORT );
                    lcl_tryReadConfigFlag( pLine, nLineLength, "trace", &aDebugData.aDbgData.nTestFlags, DBG_TEST_XTOR_TRACE );
                }

                // elements of the [test] section
                if ( eCurrentSection == eTest )
                {
                    lcl_tryReadConfigFlag( pLine, nLineLength, "profiling", &aDebugData.aDbgData.nTestFlags, DBG_TEST_PROFILING );
                    lcl_tryReadConfigFlag( pLine, nLineLength, "resources", &aDebugData.aDbgData.nTestFlags, DBG_TEST_RESOURCE );
                    lcl_tryReadConfigFlag( pLine, nLineLength, "dialog", &aDebugData.aDbgData.nTestFlags, DBG_TEST_DIALOG );
                    lcl_tryReadConfigFlag( pLine, nLineLength, "bold_app_font", &aDebugData.aDbgData.nTestFlags, DBG_TEST_BOLDAPPFONT );
                }

                pLine = pNextLine + strlen( FILE_LINEEND );
            }

            FileClose( pIniFile );
        }

        getcwd( aCurPath, sizeof( aCurPath ) );

        // Daten initialisieren
        if ( aDebugData.aDbgData.nTestFlags & DBG_TEST_XTOR )
            aDebugData.pXtorList = new PointerList;
        if ( aDebugData.aDbgData.nTestFlags & DBG_TEST_PROFILING )
            aDebugData.pProfList = new PointerList;
    }

    return &aDebugData;
}

// -----------------------------------------------------------------------

inline DebugData* ImplGetDebugData()
{
    if ( !aDebugData.bInit )
        return GetDebugData();
    else
        return &aDebugData;
}

// -----------------------------------------------------------------------

static FILETYPE ImplDbgInitFile()
{
    static BOOL bFileInit = FALSE;

    sal_Char aBuf[4096];
    getcwd( aBuf, sizeof( aBuf ) );
    chdir( aCurPath );

    DebugData*  pData = GetDebugData();
    FILETYPE    pDebugFile;

    if ( !bFileInit )
    {
        bFileInit = TRUE;

        if ( pData->aDbgData.bOverwrite )
            pDebugFile = FileOpen( pData->aDbgData.aDebugName, "w" );
        else
            pDebugFile = FileOpen( pData->aDbgData.aDebugName, "a" );

        if ( pDebugFile )
        {
            time_t  nTime = time( 0 );
            tm*     pTime;
#ifdef UNX
            tm      aTime;
            pTime = localtime_r( &nTime, &aTime );
#else
            pTime = localtime( &nTime );
#endif

            // Header ausgeben
            FilePrintF( pDebugFile, "******************************************************************************%s", FILE_LINEEND );
            FilePrintF( pDebugFile, "%s%s", pData->aDbgData.aDebugName, FILE_LINEEND );
            if ( pTime )
                FilePrintF( pDebugFile, "%s%s", asctime( pTime ), FILE_LINEEND );
        }
    }
    else
        pDebugFile = FileOpen( pData->aDbgData.aDebugName, "a" );

    chdir( aBuf );

    return pDebugFile;
}

// -----------------------------------------------------------------------

static void ImplDbgPrintFile( const sal_Char* pLine )
{
    FILETYPE pDebugFile = ImplDbgInitFile();

    if ( pDebugFile )
    {
        FilePrintF( pDebugFile, "%s%s", pLine, FILE_LINEEND );
        FileClose( pDebugFile );
    }
}

// -----------------------------------------------------------------------

static int ImplStrSearch( const sal_Char* pSearchStr, int nSearchLen,
                          const sal_Char* pStr, int nLen )
{
    int nPos = 0;
    while ( nPos+nSearchLen <= nLen )
    {
        if ( strncmp( pStr+nPos, pSearchStr, nSearchLen ) == 0 )
            return 1;
        nPos++;
    }

    return 0;
}

// -----------------------------------------------------------------------

static int ImplDbgFilter( const sal_Char* pFilter, const sal_Char* pMsg,
                          int bEmpty )
{
    int nStrLen = strlen( pFilter );
    if ( !nStrLen )
        return bEmpty;

    int nMsgLen = strlen( pMsg );
    const sal_Char* pTok = pFilter;
    int         nTok = 0;
    while ( pTok[nTok] )
    {
        if ( pTok[nTok] == ';' )
        {
            if ( nTok && ImplStrSearch( pTok, nTok, pMsg, nMsgLen ) )
                return TRUE;

            pTok += nTok+1;
            nTok = 0;
        }

        nTok++;
    }

    if ( nTok && ImplStrSearch( pTok, nTok, pMsg, nMsgLen ) )
        return TRUE;
    else
        return FALSE;
}

// -----------------------------------------------------------------------

extern "C"
void SAL_CALL dbg_printOslDebugMessage( const sal_Char * pszFileName, sal_Int32 nLine, const sal_Char * pszMessage )
{
    DbgOut( pszMessage ? pszMessage : "assertion failed!", DBG_OUT_ERROR, pszFileName, (USHORT)nLine );
}

// -----------------------------------------------------------------------

static void DebugInit()
{
    bDbgImplInMain = TRUE;
    ImplDbgInitLock();

    DebugData* pData = GetDebugData();
    if( pData->aDbgData.bHookOSLAssert && ! pData->bOslIsHooked )
    {
        pData->pOldDebugMessageFunc = osl_setDetailedDebugMessageFunc( &dbg_printOslDebugMessage );
        pData->bOslIsHooked = true;
    }
}

// -----------------------------------------------------------------------

static void DebugDeInit()
{
    DebugData*  pData = GetDebugData();
    ULONG       i;
    ULONG       nCount;
    ULONG       nOldOut;

    if( pData->bOslIsHooked )
    {
        osl_setDetailedDebugMessageFunc( pData->pOldDebugMessageFunc );
        pData->bOslIsHooked = FALSE;
    }

    // Statistik-Ausgaben immer in File
    nOldOut = pData->aDbgData.nTraceOut;
    pData->aDbgData.nTraceOut = DBG_OUT_FILE;

    // Xtor-Liste ausgeben
    if ( pData->pXtorList && pData->pXtorList->Count() &&
         (pData->aDbgData.nTestFlags & DBG_TEST_XTOR_REPORT) )
    {
        DbgOutf( "------------------------------------------------------------------------------" );
        DbgOutf( "Object Report" );
        DbgOutf( "------------------------------------------------------------------------------" );
        DbgOutf( "%-27s : %-9s : %-9s : %-7s : %-3s : %-6s :",
                 "XTor-List", "Ctor", "Dtor", "MaxInst", "St.", "Diff." );
        DbgOutf( "----------------------------:-----------:-----------:---------:----:---------:" );
        for( i = 0, nCount = pData->pXtorList->Count(); i < nCount; i++ )
        {
            XtorType* pXtorData = (XtorType*)pData->pXtorList->Get( i );
            if ( pXtorData->bTest )
            {
                // Static-Objekte dazurechnen
                pXtorData->nDtorCalls += pXtorData->nStatics;
                if ( pXtorData->nStatics && (pXtorData->nDtorCalls > pXtorData->nCtorCalls) )
                    pXtorData->nDtorCalls = pXtorData->nCtorCalls;
                DbgOutf( "%-27s : %9lu : %9lu : %7lu : %3lu : %4lu %-1s :",
                         pXtorData->aName, pXtorData->nCtorCalls, pXtorData->nDtorCalls,
                         pXtorData->nMaxCount, pXtorData->nStatics,
                         pXtorData->nCtorCalls - pXtorData->nDtorCalls,
                         (pXtorData->nCtorCalls - pXtorData->nDtorCalls) ? "!" : " " );
            }
        }
        DbgOutf( "==============================================================================" );
    }

    // Aufraeumen
    if ( pData->pXtorList )
    {
        for( i = 0, nCount = pData->pXtorList->Count(); i < nCount; i++ )
        {
            XtorType* pXtorData = (XtorType*)pData->pXtorList->Get( i );
            delete pXtorData;
        }
        delete pData->pXtorList;
        pData->pXtorList = NULL;
    }

    // Alles auf FALSE setzen, damit globale Variablen nicht das
    // System zum Abstuerzen bringt. Dabei muessen aber die
    // Memory-Flags erhalten bleiben, da sonst new/delete in globalen
    // Variablen abstuerzen, da die Pointeranpassung dann nicht mehr richtig
    // funktioniert
    pData->aDbgData.nTraceOut   = nOldOut;
    pData->aDbgData.nTestFlags &= (DBG_TEST_MEM | DBG_TEST_PROFILING);
    pData->aDbgPrintUserChannels.clear();
    pData->pDbgPrintTestTool    = NULL;
    pData->pDbgPrintShell       = NULL;
    pData->pDbgPrintWindow      = NULL;
    pData->pDbgPrintShell       = NULL;
    pData->pOldDebugMessageFunc = NULL;
    ImplDbgDeInitLock();
}

// -----------------------------------------------------------------------

static void DebugGlobalDeInit()
{
    DebugData*  pData = GetDebugData();
    ULONG       i;
    ULONG       nCount;
    ULONG       nOldOut;

    // Statistik-Ausgaben immer in File
    nOldOut = pData->aDbgData.nTraceOut;
    pData->aDbgData.nTraceOut = DBG_OUT_FILE;

    // Profileliste ausgeben
    if ( pData->pProfList && pData->pProfList->Count() )
    {
        DbgOutf( "------------------------------------------------------------------------------" );
        DbgOutf( "Profiling Report" );
        DbgOutf( "------------------------------------------------------------------------------" );
        DbgOutf( "%-25s : %-9s : %-6s : %-6s : %-6s : %-9s :",
                 "Prof-List (ms)", "Time", "Min", "Max", "Ave", "Count" );
        DbgOutf( "--------------------------:-----------:--------:--------:--------:-----------:" );
        for( i = 0, nCount = pData->pProfList->Count(); i < nCount; i++ )
        {
            ProfType* pProfData = (ProfType*)pData->pProfList->Get( i );
            ULONG nAve = pProfData->nTime / pProfData->nCount;
            DbgOutf( "%-25s : %9lu : %6lu : %6lu : %6lu : %9lu :",
                     pProfData->aName, pProfData->nTime,
                     pProfData->nMinTime, pProfData->nMaxTime, nAve,
                     pProfData->nCount );
        }
        DbgOutf( "==============================================================================" );
    }

    // Aufraeumen
    if ( pData->pProfList )
    {
        for( i = 0, nCount = pData->pProfList->Count(); i < nCount; i++ )
        {
            ProfType* pProfData = (ProfType*)pData->pProfList->Get( i );
            delete pProfData;
        }
        delete pData->pProfList;
        pData->pProfList = NULL;
    }

#ifdef SV_MEMMGR
    DbgImpCheckMemoryDeInit();
#endif

    // Profiling-Flags ausschalten
    pData->aDbgData.nTraceOut   = nOldOut;
    pData->aDbgData.nTestFlags &= ~DBG_TEST_PROFILING;
}

// -----------------------------------------------------------------------

void ImpDbgOutfBuf( sal_Char* pBuf, const sal_Char* pFStr, ... )
{
    va_list pList;

    va_start( pList, pFStr );
    sal_Char aBuf[DBG_BUF_MAXLEN];
    vsprintf( aBuf, pFStr, pList );
    va_end( pList );

    strcat( pBuf, aBuf );
    strcat( pBuf, "\n" );
}

// -----------------------------------------------------------------------

static void DebugXTorInfo( sal_Char* pBuf )
{
    DebugData*  pData = GetDebugData();
    ULONG       i;
    ULONG       nCount;

    // Xtor-Liste ausgeben
    if ( pData->pXtorList && pData->pXtorList->Count() &&
         (pData->aDbgData.nTestFlags & DBG_TEST_XTOR_REPORT) )
    {
        ImpDbgOutfBuf( pBuf, "------------------------------------------------------------------------------" );
        ImpDbgOutfBuf( pBuf, "Object Report" );
        ImpDbgOutfBuf( pBuf, "------------------------------------------------------------------------------" );
        ImpDbgOutfBuf( pBuf, "%-27s : %-9s : %-9s : %-7s : %-3s : %-6s :",
                       "XTor-List", "Ctor", "Dtor", "MaxInst", "St.", "Diff." );
        ImpDbgOutfBuf( pBuf, "----------------------------:-----------:-----------:---------:----:---------:" );
        for( i = 0, nCount = pData->pXtorList->Count(); i < nCount; i++ )
        {
            XtorType* pXtorData = (XtorType*)pData->pXtorList->Get( i );
            if ( pXtorData->bTest )
            {
                ImpDbgOutfBuf( pBuf, "%-27s : %9lu : %9lu : %7lu : %3lu : %6lu :",
                               pXtorData->aName, pXtorData->nCtorCalls, pXtorData->nDtorCalls,
                               pXtorData->nMaxCount, pXtorData->nStatics,
                               pXtorData->nCtorCalls - pXtorData->nDtorCalls );
            }
        }
        ImpDbgOutfBuf( pBuf, "==============================================================================" );
        ImpDbgOutfBuf( pBuf, "" );
    }
}

// -----------------------------------------------------------------------
BOOL ImplDbgFilterMessage( const sal_Char* pMsg )
{
    DebugData*  pData = GetDebugData();
    if ( !ImplDbgFilter( pData->aDbgData.aInclFilter, pMsg, TRUE ) )
        return TRUE;
    if ( ImplDbgFilter( pData->aDbgData.aExclFilter, pMsg, FALSE ) )
        return TRUE;
    return FALSE;
}

// -----------------------------------------------------------------------

void* DbgFunc( USHORT nAction, void* pParam )
{
    DebugData* pDebugData = ImplGetDebugData();

    if ( nAction == DBG_FUNC_GETDATA )
        return (void*)&(pDebugData->aDbgData);
    else if ( nAction == DBG_FUNC_GETPRINTMSGBOX )
        return (void*)(long)(pDebugData->pDbgPrintMsgBox);
    else if ( nAction == DBG_FUNC_FILTERMESSAGE )
        if ( ImplDbgFilterMessage( (const sal_Char*) pParam ) )
            return (void*) -1;
        else
            return (void*) 0;   // aka NULL
    else

    {
        switch ( nAction )
        {
            case DBG_FUNC_DEBUGSTART:
                DebugInit();
                break;

            case DBG_FUNC_DEBUGEND:
                DebugDeInit();
                break;

            case DBG_FUNC_GLOBALDEBUGEND:
                DebugGlobalDeInit();
                break;

            case DBG_FUNC_SETPRINTMSGBOX:
                pDebugData->pDbgPrintMsgBox = (DbgPrintLine)(long)pParam;
                break;

            case DBG_FUNC_SETPRINTWINDOW:
                pDebugData->pDbgPrintWindow = (DbgPrintLine)(long)pParam;
                break;

            case DBG_FUNC_SETPRINTSHELL:
                pDebugData->pDbgPrintShell = (DbgPrintLine)(long)pParam;
                break;

            case DBG_FUNC_SETPRINTTESTTOOL:
                pDebugData->pDbgPrintTestTool = (DbgPrintLine)(long)pParam;
                break;

            case DBG_FUNC_SAVEDATA:
                {
                const DbgData* pData = static_cast< const DbgData* >( pParam );
                
                sal_Char aBuf[ 4096 ];
                DbgGetDbgFileName( aBuf, sizeof( aBuf ) );
                FILETYPE pIniFile = FileOpen( aBuf, "w" );
                if ( pIniFile == NULL )
                    break;

                lcl_startSection( pIniFile, eOutput );
                lcl_writeConfigString( pIniFile, "log_file", pData->aDebugName );
                lcl_writeConfigBoolean( pIniFile, "overwrite", pData->bOverwrite );
                lcl_writeConfigString( pIniFile, "include", pData->aInclFilter );
                lcl_writeConfigString( pIniFile, "exclude", pData->aExclFilter );
                lcl_writeConfigString( pIniFile, "include_class", pData->aInclClassFilter );
                lcl_writeConfigString( pIniFile, "exclude_class", pData->aExclClassFilter );
                lcl_writeConfigOutChannel( pIniFile, "trace", pData->nTraceOut );
                lcl_writeConfigOutChannel( pIniFile, "warning", pData->nWarningOut );
                lcl_writeConfigOutChannel( pIniFile, "error", pData->nErrorOut );
                lcl_writeConfigBoolean( pIniFile, "oslhook", pData->bHookOSLAssert );

                lcl_lineFeed( pIniFile );
                lcl_startSection( pIniFile, eMemory );
                lcl_writeConfigFlag( pIniFile, "initialize", pData->nTestFlags, DBG_TEST_MEM_INIT );
                lcl_writeConfigFlag( pIniFile, "overwrite", pData->nTestFlags, DBG_TEST_MEM_OVERWRITE );
                lcl_writeConfigFlag( pIniFile, "overwrite_free", pData->nTestFlags, DBG_TEST_MEM_OVERWRITEFREE );
                lcl_writeConfigFlag( pIniFile, "pointer", pData->nTestFlags, DBG_TEST_MEM_POINTER );
                lcl_writeConfigFlag( pIniFile, "report", pData->nTestFlags, DBG_TEST_MEM_REPORT );
                lcl_writeConfigFlag( pIniFile, "trace", pData->nTestFlags, DBG_TEST_MEM_TRACE );
                lcl_writeConfigFlag( pIniFile, "new_and_delete", pData->nTestFlags, DBG_TEST_MEM_NEWDEL );
                lcl_writeConfigFlag( pIniFile, "object_test", pData->nTestFlags, DBG_TEST_MEM_XTOR );
                lcl_writeConfigFlag( pIniFile, "sys_alloc", pData->nTestFlags, DBG_TEST_MEM_SYSALLOC );
                lcl_writeConfigFlag( pIniFile, "leak_report", pData->nTestFlags, DBG_TEST_MEM_LEAKREPORT );

                lcl_lineFeed( pIniFile );
                lcl_writeHexByte( pIniFile, "init_byte", pData->bMemInit );
                lcl_writeHexByte( pIniFile, "bound_byte", pData->bMemBound );
                lcl_writeHexByte( pIniFile, "free_byte", pData->bMemFree );

                lcl_lineFeed( pIniFile );
                lcl_startSection( pIniFile, eGUI );
                lcl_writeConfigString( pIniFile, "debug_window_state", pData->aDbgWinState );

                lcl_lineFeed( pIniFile );
                lcl_startSection( pIniFile, eObjects );
                lcl_writeConfigFlag( pIniFile, "check_this", pData->nTestFlags, DBG_TEST_XTOR_THIS );
                lcl_writeConfigFlag( pIniFile, "check_function", pData->nTestFlags, DBG_TEST_XTOR_FUNC );
                lcl_writeConfigFlag( pIniFile, "check_exit", pData->nTestFlags, DBG_TEST_XTOR_EXIT );
                lcl_writeConfigFlag( pIniFile, "generate_report", pData->nTestFlags, DBG_TEST_XTOR_REPORT );
                lcl_writeConfigFlag( pIniFile, "trace", pData->nTestFlags, DBG_TEST_XTOR_TRACE );

                lcl_lineFeed( pIniFile );
                lcl_startSection( pIniFile, eTest );
                lcl_writeConfigFlag( pIniFile, "profiling", pData->nTestFlags, DBG_TEST_PROFILING );
                lcl_writeConfigFlag( pIniFile, "resources", pData->nTestFlags, DBG_TEST_RESOURCE );
                lcl_writeConfigFlag( pIniFile, "dialog", pData->nTestFlags, DBG_TEST_DIALOG );
                lcl_writeConfigFlag( pIniFile, "bold_app_font", pData->nTestFlags, DBG_TEST_BOLDAPPFONT );

                FileClose( pIniFile );
                }
                break;

            case DBG_FUNC_MEMTEST:
#ifdef SV_MEMMGR
                DbgImpCheckMemory( pParam );
#endif
                break;

            case DBG_FUNC_XTORINFO:
                DebugXTorInfo( (sal_Char*)pParam );
                break;

            case DBG_FUNC_MEMINFO:
#ifdef SV_MEMMGR
                DbgImpMemoryInfo( (sal_Char*)pParam );
#endif
                break;

            case DBG_FUNC_COREDUMP:
                ImplCoreDump();
                break;

            case DBG_FUNC_ALLERROROUT:
                return (void*)(ULONG)TRUE;

            case DBG_FUNC_SETTESTSOLARMUTEX:
                pDebugData->pDbgTestSolarMutex = (DbgTestSolarMutexProc)(long)pParam;
                break;

            case DBG_FUNC_TESTSOLARMUTEX:
                if ( pDebugData->pDbgTestSolarMutex )
                    pDebugData->pDbgTestSolarMutex();
                break;

            case DBG_FUNC_PRINTFILE:
                ImplDbgPrintFile( (const sal_Char*)pParam );
                break;
            case DBG_FUNC_UPDATEOSLHOOK:
            {
                const DbgData* pData = static_cast< const DbgData* >( pParam );
                pDebugData->aDbgData.bHookOSLAssert = pData->bHookOSLAssert;
                if( pDebugData->bOslIsHooked && ! pData->bHookOSLAssert )
                {
                    osl_setDetailedDebugMessageFunc( pDebugData->pOldDebugMessageFunc );
                    pDebugData->bOslIsHooked = FALSE;
                }
                else if( ! pDebugData->bOslIsHooked && pData->bHookOSLAssert )
                {
                    pDebugData->pOldDebugMessageFunc = osl_setDetailedDebugMessageFunc( &dbg_printOslDebugMessage );
                    pDebugData->bOslIsHooked = TRUE;
                }
            }
            break;
       }

        return NULL;
    }
}

// -----------------------------------------------------------------------

DbgChannelId DbgRegisterUserChannel( DbgPrintLine pProc )
{
    DebugData* pData = ImplGetDebugData();
    pData->aDbgPrintUserChannels.push_back( pProc );
    return (DbgChannelId)( pData->aDbgPrintUserChannels.size() - 1 + DBG_OUT_USER_CHANNEL_0 );
}

// -----------------------------------------------------------------------

void DbgProf( USHORT nAction, DbgDataType* pDbgData )
{
    // Ueberhaupt Profiling-Test an
    DebugData* pData = ImplGetDebugData();

    if ( !(pData->aDbgData.nTestFlags & DBG_TEST_PROFILING) )
        return;

    sal_Char    aBuf[DBG_BUF_MAXLEN];
    ProfType*   pProfData = (ProfType*)pDbgData->pData;
    ULONG       nTime;
    if ( (nAction != DBG_PROF_START) && !pProfData )
    {
        strcpy( aBuf, DbgError_ProfEnd1 );
        strcat( aBuf, pDbgData->pName );
        DbgError( aBuf );
        return;
    }

    switch ( nAction )
    {
        case DBG_PROF_START:
            if ( !pDbgData->pData )
            {
                pDbgData->pData = (void*)new ProfType;
                pProfData = (ProfType*)pDbgData->pData;
                strncpy( pProfData->aName, pDbgData->pName, DBG_MAXNAME );
                pProfData->aName[DBG_MAXNAME] = '\0';
                pProfData->nCount           = 0;
                pProfData->nTime            = 0;
                pProfData->nMinTime         = 0xFFFFFFFF;
                pProfData->nMaxTime         = 0;
                pProfData->nStart           = 0xFFFFFFFF;
                pProfData->nContinueTime    = 0;
                pProfData->nContinueStart   = 0xFFFFFFFF;
                pData->pProfList->Add( (void*)pProfData );
            }

            if ( pProfData->nStart == 0xFFFFFFFF )
            {
                pProfData->nStart = ImplGetPerfTime();
                pProfData->nCount++;
            }
            break;

        case DBG_PROF_STOP:
            nTime = ImplGetPerfTime();

            if ( pProfData->nStart == 0xFFFFFFFF )
            {
                DbgError( DbgError_ProfEnd1 );
                return;
            }

            if ( pProfData->nContinueStart != 0xFFFFFFFF )
            {
                pProfData->nContinueTime += ImplGetPerfTime() - pProfData->nContinueStart;
                pProfData->nContinueStart = 0xFFFFFFFF;
            }

            nTime -= pProfData->nStart;
            nTime -= pProfData->nContinueTime;

            if ( nTime < pProfData->nMinTime )
                pProfData->nMinTime = nTime;

            if ( nTime > pProfData->nMaxTime )
                pProfData->nMaxTime = nTime;

            pProfData->nTime += nTime;

            pProfData->nStart         = 0xFFFFFFFF;
            pProfData->nContinueTime  = 0;
            pProfData->nContinueStart = 0xFFFFFFFF;
            break;

        case DBG_PROF_CONTINUE:
            if ( pProfData->nContinueStart != 0xFFFFFFFF )
            {
                pProfData->nContinueTime += ImplGetPerfTime() - pProfData->nContinueStart;
                pProfData->nContinueStart = 0xFFFFFFFF;
            }
            break;

        case DBG_PROF_PAUSE:
            if ( pProfData->nContinueStart == 0xFFFFFFFF )
                pProfData->nContinueStart = ImplGetPerfTime();
            break;
    }
}

// -----------------------------------------------------------------------

void DbgXtor( DbgDataType* pDbgData, USHORT nAction, const void* pThis,
              DbgUsr fDbgUsr )
{
    DebugData* pData = ImplGetDebugData();

    // Verbindung zu Debug-Memory-Manager testen
#ifdef SV_MEMMGR
    if ( pData->aDbgData.nTestFlags & DBG_TEST_MEM_XTOR )
        DbgImpCheckMemory();
#endif

    // Schnell-Test
    if ( !(pData->aDbgData.nTestFlags & DBG_TEST_XTOR) )
        return;

    XtorType* pXtorData = (XtorType*)pDbgData->pData;
    if ( !pXtorData )
    {
        pDbgData->pData = (void*)new XtorType;
        pXtorData = (XtorType*)pDbgData->pData;
        strncpy( pXtorData->aName, pDbgData->pName, DBG_MAXNAME );
        pXtorData->aName[DBG_MAXNAME] = '\0';
        pXtorData->nCtorCalls   = 0;
        pXtorData->nDtorCalls   = 0;
        pXtorData->nMaxCount    = 0;
        pXtorData->nStatics     = 0;
        pXtorData->bTest        = TRUE;
        pData->pXtorList->Add( (void*)pXtorData );

        if ( !ImplDbgFilter( pData->aDbgData.aInclClassFilter, pXtorData->aName, TRUE ) )
            pXtorData->bTest = FALSE;
        if ( ImplDbgFilter( pData->aDbgData.aExclClassFilter, pXtorData->aName, FALSE ) )
            pXtorData->bTest = FALSE;
    }
    if ( !pXtorData->bTest )
        return;

    sal_Char    aBuf[DBG_BUF_MAXLEN];
    USHORT      nAct = nAction & ~DBG_XTOR_DTOROBJ;

    // Trace (Enter)
    if ( (pData->aDbgData.nTestFlags & DBG_TEST_XTOR_TRACE) &&
         !(nAction & DBG_XTOR_DTOROBJ) )
    {
        if ( nAct != DBG_XTOR_CHKOBJ )
        {
            if ( nAct == DBG_XTOR_CTOR )
                strcpy( aBuf, DbgTrace_EnterCtor );
            else if ( nAct == DBG_XTOR_DTOR )
                strcpy( aBuf, DbgTrace_EnterDtor );
            else
                strcpy( aBuf, DbgTrace_EnterMeth );
            strcat( aBuf, pDbgData->pName );
            DbgTrace( aBuf );
        }
    }

    // Sind noch Xtor-Tests als Trace an
    if ( pData->aDbgData.nTestFlags & DBG_TEST_XTOR_EXTRA )
    {
        // DBG_CTOR-Aufruf vor allen anderen DBG_XTOR-Aufrufen
        if ( ((nAction & ~DBG_XTOR_DTOROBJ) != DBG_XTOR_CTOR) && !pDbgData->pData )
        {
            strcpy( aBuf, DbgError_Xtor1 );
            strcat( aBuf, pDbgData->pName );
            DbgError( aBuf );
            return;
        }

        // Testen, ob This-Pointer gueltig
        if ( pData->aDbgData.nTestFlags & DBG_TEST_XTOR_THIS )
        {
            if ( (pData->aDbgData.nTestFlags & DBG_TEST_XTOR_EXIT) ||
                 !(nAction & DBG_XTOR_DTOROBJ) )
            {
                // This-Pointer == NULL
                if ( !pThis )
                {
                    strcpy( aBuf, DbgError_CtorDtor1 );
                    strcat( aBuf, pDbgData->pName );
                    DbgError( aBuf );
                    return;
                }

                if ( (nAction & ~DBG_XTOR_DTOROBJ) != DBG_XTOR_CTOR )
                {
                    if ( !pXtorData->aThisList.IsIn( pThis ) )
                    {
                        sprintf( aBuf, DbgError_CtorDtor2, pThis );
                        strcat( aBuf, pDbgData->pName );
                        DbgError( aBuf );
                    }
                }
            }
        }

        // Function-Test durchfuehren und Verwaltungsdaten updaten
        const sal_Char* pMsg = NULL;
        switch ( nAction & ~DBG_XTOR_DTOROBJ )
        {
            case DBG_XTOR_CTOR:
                if ( nAction & DBG_XTOR_DTOROBJ )
                {
                    if ( fDbgUsr &&
                         (pData->aDbgData.nTestFlags & DBG_TEST_XTOR_EXIT) &&
                         (pData->aDbgData.nTestFlags & DBG_TEST_XTOR_FUNC) )
                        pMsg = fDbgUsr( pThis );
                }
                else
                {
                    pXtorData->nCtorCalls++;
                    if ( !bDbgImplInMain )
                        pXtorData->nStatics++;
                    if ( (pXtorData->nCtorCalls-pXtorData->nDtorCalls) > pXtorData->nMaxCount )
                        pXtorData->nMaxCount = pXtorData->nCtorCalls - pXtorData->nDtorCalls;

                    if ( pData->aDbgData.nTestFlags & DBG_TEST_XTOR_THIS )
                        pXtorData->aThisList.Add( pThis );
                }
                break;

            case DBG_XTOR_DTOR:
                if ( nAction & DBG_XTOR_DTOROBJ )
                {
                    pXtorData->nDtorCalls++;
                    if ( pData->aDbgData.nTestFlags & DBG_TEST_XTOR_THIS )
                        pXtorData->aThisList.Remove( pThis );
                }
                else
                {
                    if ( fDbgUsr &&
                         (pData->aDbgData.nTestFlags & DBG_TEST_XTOR_FUNC) )
                        pMsg = fDbgUsr( pThis );
                }
                break;

            case DBG_XTOR_CHKTHIS:
            case DBG_XTOR_CHKOBJ:
                if ( nAction & DBG_XTOR_DTOROBJ )
                {
                    if ( fDbgUsr &&
                         (pData->aDbgData.nTestFlags & DBG_TEST_XTOR_EXIT) &&
                         (pData->aDbgData.nTestFlags & DBG_TEST_XTOR_FUNC) )
                        pMsg = fDbgUsr( pThis );
                }
                else
                {
                    if ( fDbgUsr &&
                         (pData->aDbgData.nTestFlags & DBG_TEST_XTOR_FUNC) )
                        pMsg = fDbgUsr( pThis );
                }
                break;
        }

        // Gegebenenfalls Fehlermeldung ausgeben
        if ( pMsg )
        {
            sprintf( aBuf, DbgError_CtorDtor3, pThis );
            strcat( aBuf, pDbgData->pName );
            strcat( aBuf, ": \n" );
            strcat( aBuf, pMsg );
            DbgError( aBuf );
        }
    }

    // Trace (Leave)
    if ( (pData->aDbgData.nTestFlags & DBG_TEST_XTOR_TRACE) &&
         (nAction & DBG_XTOR_DTOROBJ) )
    {
        if ( nAct != DBG_XTOR_CHKOBJ )
        {
            if ( nAct == DBG_XTOR_CTOR )
                strcpy( aBuf, DbgTrace_LeaveCtor );
            else if ( nAct == DBG_XTOR_DTOR )
                strcpy( aBuf, DbgTrace_LeaveDtor );
            else
                strcpy( aBuf, DbgTrace_LeaveMeth );
            strcat( aBuf, pDbgData->pName );
            DbgTrace( aBuf );
        }
    }
}

// -----------------------------------------------------------------------

void DbgOut( const sal_Char* pMsg, USHORT nDbgOut, const sal_Char* pFile, USHORT nLine )
{
    static BOOL bIn = FALSE;
    if ( bIn )
        return;
    bIn = TRUE;

    DebugData*  pData = GetDebugData();
    sal_Char const *   pStr;
    ULONG       nOut;
    int         nBufLen = 0;

    if ( nDbgOut == DBG_OUT_ERROR )
    {
        nOut = pData->aDbgData.nErrorOut;
        pStr = "Error: ";
        if ( pData->aDbgData.nErrorOut == DBG_OUT_FILE )
            DbgDebugBeep();
    }
    else if ( nDbgOut == DBG_OUT_WARNING )
    {
        nOut = pData->aDbgData.nWarningOut;
        pStr = "Warning: ";
    }
    else
    {
        nOut = pData->aDbgData.nTraceOut;
        pStr = NULL;
    }

    if ( nOut == DBG_OUT_NULL )
    {
        bIn = FALSE;
        return;
    }

    if ( ImplDbgFilterMessage( pMsg ) )
    {
        bIn = FALSE;
        return;
    }

    ImplDbgLock();

    sal_Char aBufOut[DBG_BUF_MAXLEN];
    if ( pStr )
    {
        strcpy( aBufOut, pStr );
        nBufLen = strlen( pStr );
    }
    else
        aBufOut[0] = '\0';

    int nMsgLen = strlen( pMsg );
    if ( nBufLen+nMsgLen > DBG_BUF_MAXLEN )
    {
        int nCopyLen = DBG_BUF_MAXLEN-nBufLen-3;
        strncpy( &(aBufOut[nBufLen]), pMsg, nCopyLen );
        strcpy( &(aBufOut[nBufLen+nCopyLen]), "..." );
    }
    else
        strcpy( &(aBufOut[nBufLen]), pMsg );

    if ( pFile && nLine && (nBufLen+nMsgLen < DBG_BUF_MAXLEN) )
    {
        if ( nOut == DBG_OUT_MSGBOX )
            strcat( aBufOut, "\n" );
        else
            strcat( aBufOut, " " );
        strcat( aBufOut, "From File " );
        strcat( aBufOut, pFile );
        strcat( aBufOut, " at Line " );

        // Line in String umwandeln und dranhaengen
        sal_Char    aLine[9];
        sal_Char*   pLine = &aLine[7];
        USHORT      i;
        memset( aLine, 0, sizeof( aLine ) );
        do
        {
            i = nLine % 10;
            pLine--;
            *(pLine) = (sal_Char)i + 48;
            nLine /= 10;
        }
        while ( nLine );
        strcat( aBufOut, pLine );
    }

    if ( ( nOut >= DBG_OUT_USER_CHANNEL_0 ) && ( nOut - DBG_OUT_USER_CHANNEL_0 < pData->aDbgPrintUserChannels.size() ) )
    {
        DbgPrintLine pPrinter = pData->aDbgPrintUserChannels[ nOut - DBG_OUT_USER_CHANNEL_0 ];
        if ( pPrinter )
            pPrinter( aBufOut );
        else
            nOut = DBG_OUT_DEBUGGER;
    }

    if ( nOut == DBG_OUT_COREDUMP )
    {
        if ( !ImplCoreDump() )
            nOut = DBG_OUT_DEBUGGER;
    }

    if ( nOut == DBG_OUT_DEBUGGER )
    {
        if ( !ImplActivateDebugger( aBufOut ) )
            nOut = DBG_OUT_TESTTOOL;
    }

    if ( nOut == DBG_OUT_TESTTOOL )
    {
        if ( pData->pDbgPrintTestTool )
            pData->pDbgPrintTestTool( aBufOut );
        else
            nOut = DBG_OUT_MSGBOX;
    }

    if ( nOut == DBG_OUT_MSGBOX )
    {
        if ( pData->pDbgPrintMsgBox )
            pData->pDbgPrintMsgBox( aBufOut );
        else
            nOut = DBG_OUT_SHELL;
    }

    if ( nOut == DBG_OUT_SHELL )
    {
        if ( pData->pDbgPrintShell )
            pData->pDbgPrintShell( aBufOut );
        else
            nOut = DBG_OUT_WINDOW;
    }

    if ( nOut == DBG_OUT_WINDOW )
    {
        if ( pData->pDbgPrintWindow )
            pData->pDbgPrintWindow( aBufOut );
        else
            nOut = DBG_OUT_FILE;
    }

    if ( nOut == DBG_OUT_FILE )
        ImplDbgPrintFile( aBufOut );

    ImplDbgUnlock();

    bIn = FALSE;
}

// -----------------------------------------------------------------------

void DbgOutTypef( USHORT nDbgOut, const sal_Char* pFStr, ... )
{
    va_list pList;

    va_start( pList, pFStr );
    sal_Char aBuf[DBG_BUF_MAXLEN];
    vsprintf( aBuf, pFStr, pList );
    va_end( pList );

    DbgOut( aBuf, nDbgOut );
}

// -----------------------------------------------------------------------

void DbgOutf( const sal_Char* pFStr, ... )
{
    va_list pList;

    va_start( pList, pFStr );
    sal_Char aBuf[DBG_BUF_MAXLEN];
    vsprintf( aBuf, pFStr, pList );
    va_end( pList );

    DbgOut( aBuf );
}

// =======================================================================

#else

void* DbgFunc( USHORT, void* ) { return NULL; }

void DbgProf( USHORT, DbgDataType* ) {}
void DbgXtor( DbgDataType*, USHORT, const void*, DbgUsr ) {}

void DbgOut( const sal_Char*, USHORT, const sal_Char*, USHORT ) {}
void DbgOutTypef( USHORT, const sal_Char*, ... ) {}
void DbgOutf( const sal_Char*, ... ) {}

#endif
