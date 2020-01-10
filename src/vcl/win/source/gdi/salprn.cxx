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
#include "precompiled_vcl.hxx"

#include <string.h>
#include <tools/svwin.h>

#ifdef __MINGW32__
#include <excpt.h>
#endif

#ifndef _OSL_MODULE_H
#include <osl/module.h>
#endif
#include <wincomp.hxx>
#include <saldata.hxx>
#include <salinst.h>
#include <salgdi.h>
#include <salframe.h>
#include <vcl/salptype.hxx>
#include <salprn.h>
#include <vcl/print.h>
#include <vcl/jobset.h>

#include <tools/urlobj.hxx>
#include <com/sun/star/ui/dialogs/TemplateDescription.hpp>
#include <com/sun/star/ui/dialogs/ExecutableDialogResults.hpp>
#include <com/sun/star/ui/dialogs/XFilePicker.hpp>
#include <com/sun/star/ui/dialogs/XFilterManager.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <comphelper/processfactory.hxx>

#include <malloc.h>

#ifdef __MINGW32__
#define CATCH_DRIVER_EX_BEGIN                                               \
    jmp_buf jmpbuf;                                                         \
    __SEHandler han;                                                        \
    if (__builtin_setjmp(jmpbuf) == 0)                                      \
    {                                                                       \
        han.Set(jmpbuf, NULL, (__SEHandler::PF)EXCEPTION_EXECUTE_HANDLER)
        
#define CATCH_DRIVER_EX_END(mes, p)                                         \
    }                                                                       \
    han.Reset()
#define CATCH_DRIVER_EX_END_2(mes)                                            \
    }                                                                       \
    han.Reset()
#else
#define CATCH_DRIVER_EX_BEGIN                                               \
    __try                                                                   \
    {
#define CATCH_DRIVER_EX_END(mes, p)                                         \
    }                                                                       \
    __except(WinSalInstance::WorkaroundExceptionHandlingInUSER32Lib(GetExceptionCode(), GetExceptionInformation()))\
    {                                                                       \
        DBG_ERROR( mes );                                                   \
        p->markInvalid();                                                   \
    }
#define CATCH_DRIVER_EX_END_2(mes)                                         \
    }                                                                       \
    __except(WinSalInstance::WorkaroundExceptionHandlingInUSER32Lib(GetExceptionCode(), GetExceptionInformation()))\
    {                                                                       \
        DBG_ERROR( mes );                                                   \
    }
#endif


using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::ui::dialogs;
using namespace rtl;

// =======================================================================

static char aImplWindows[] = "windows";
static char aImplDevices[] = "devices";
static char aImplDevice[]  = "device";

static LPDEVMODEA SAL_DEVMODE_A( const ImplJobSetup* pSetupData )
{
    LPDEVMODEA pRet = NULL;
    SalDriverData* pDrv = (SalDriverData*)pSetupData->mpDriverData;
    if( pDrv->mnVersion == SAL_DRIVERDATA_VERSION_A &&
        pSetupData->mnDriverDataLen >= sizeof(DEVMODEA)+sizeof(SalDriverData)-1
        )
        pRet = ((LPDEVMODEA)((pSetupData->mpDriverData) + (pDrv->mnDriverOffset)));
    return pRet;
}

static LPDEVMODEW SAL_DEVMODE_W( const ImplJobSetup* pSetupData )
{
    LPDEVMODEW pRet = NULL;
    SalDriverData* pDrv = (SalDriverData*)pSetupData->mpDriverData;
    if( pDrv->mnVersion == SAL_DRIVERDATA_VERSION_W &&
        pSetupData->mnDriverDataLen >= sizeof(DEVMODEW)+sizeof(SalDriverData)-1
        )
        pRet = ((LPDEVMODEW)((pSetupData->mpDriverData) + (pDrv->mnDriverOffset)));
    return pRet;
}

// =======================================================================

static ULONG ImplWinQueueStatusToSal( DWORD nWinStatus )
{
	ULONG nStatus = 0;
	if ( nWinStatus & PRINTER_STATUS_PAUSED )
		nStatus |= QUEUE_STATUS_PAUSED;
	if ( nWinStatus & PRINTER_STATUS_ERROR )
		nStatus |= QUEUE_STATUS_ERROR;
	if ( nWinStatus & PRINTER_STATUS_PENDING_DELETION )
		nStatus |= QUEUE_STATUS_PENDING_DELETION;
	if ( nWinStatus & PRINTER_STATUS_PAPER_JAM )
		nStatus |= QUEUE_STATUS_PAPER_JAM;
	if ( nWinStatus & PRINTER_STATUS_PAPER_OUT )
		nStatus |= QUEUE_STATUS_PAPER_OUT;
	if ( nWinStatus & PRINTER_STATUS_MANUAL_FEED )
		nStatus |= QUEUE_STATUS_MANUAL_FEED;
	if ( nWinStatus & PRINTER_STATUS_PAPER_PROBLEM )
		nStatus |= QUEUE_STATUS_PAPER_PROBLEM;
	if ( nWinStatus & PRINTER_STATUS_OFFLINE )
		nStatus |= QUEUE_STATUS_OFFLINE;
	if ( nWinStatus & PRINTER_STATUS_IO_ACTIVE )
		nStatus |= QUEUE_STATUS_IO_ACTIVE;
	if ( nWinStatus & PRINTER_STATUS_BUSY )
		nStatus |= QUEUE_STATUS_BUSY;
	if ( nWinStatus & PRINTER_STATUS_PRINTING )
		nStatus |= QUEUE_STATUS_PRINTING;
	if ( nWinStatus & PRINTER_STATUS_OUTPUT_BIN_FULL )
		nStatus |= QUEUE_STATUS_OUTPUT_BIN_FULL;
	if ( nWinStatus & PRINTER_STATUS_WAITING )
		nStatus |= QUEUE_STATUS_WAITING;
	if ( nWinStatus & PRINTER_STATUS_PROCESSING )
		nStatus |= QUEUE_STATUS_PROCESSING;
	if ( nWinStatus & PRINTER_STATUS_INITIALIZING )
		nStatus |= QUEUE_STATUS_INITIALIZING;
	if ( nWinStatus & PRINTER_STATUS_WARMING_UP )
		nStatus |= QUEUE_STATUS_WARMING_UP;
	if ( nWinStatus & PRINTER_STATUS_TONER_LOW )
		nStatus |= QUEUE_STATUS_TONER_LOW;
	if ( nWinStatus & PRINTER_STATUS_NO_TONER )
		nStatus |= QUEUE_STATUS_NO_TONER;
	if ( nWinStatus & PRINTER_STATUS_PAGE_PUNT )
		nStatus |= QUEUE_STATUS_PAGE_PUNT;
	if ( nWinStatus & PRINTER_STATUS_USER_INTERVENTION )
		nStatus |= QUEUE_STATUS_USER_INTERVENTION;
	if ( nWinStatus & PRINTER_STATUS_OUT_OF_MEMORY )
		nStatus |= QUEUE_STATUS_OUT_OF_MEMORY;
	if ( nWinStatus & PRINTER_STATUS_DOOR_OPEN )
		nStatus |= QUEUE_STATUS_DOOR_OPEN;
	if ( nWinStatus & PRINTER_STATUS_SERVER_UNKNOWN )
		nStatus |= QUEUE_STATUS_SERVER_UNKNOWN;
	if ( nWinStatus & PRINTER_STATUS_POWER_SAVE )
		nStatus |= QUEUE_STATUS_POWER_SAVE;
	if ( !nStatus && !(nWinStatus & PRINTER_STATUS_NOT_AVAILABLE) )
		nStatus |= QUEUE_STATUS_READY;
	return nStatus;
}

// -----------------------------------------------------------------------

static void getPrinterQueueInfoOldStyle( ImplPrnQueueList* pList )
{
	DWORD			i;
	DWORD			n;
	DWORD			nBytes = 0;
	DWORD			nInfoPrn2;
	BOOL			bFound = FALSE;
	PRINTER_INFO_2* pWinInfo2 = NULL;
	PRINTER_INFO_2* pGetInfo2;
	EnumPrintersA( PRINTER_ENUM_LOCAL, NULL, 2, NULL, 0, &nBytes, &nInfoPrn2 );
	if ( nBytes )
	{
		pWinInfo2 = (PRINTER_INFO_2*) rtl_allocateMemory( nBytes );
		if ( EnumPrintersA( PRINTER_ENUM_LOCAL, NULL, 2, (LPBYTE)pWinInfo2, nBytes, &nBytes, &nInfoPrn2 ) )
		{
			pGetInfo2 = pWinInfo2;
			for ( i = 0; i < nInfoPrn2; i++ )
			{
				SalPrinterQueueInfo* pInfo = new SalPrinterQueueInfo;
				pInfo->maPrinterName = ImplSalGetUniString( pGetInfo2->pPrinterName );
				pInfo->maDriver 	 = ImplSalGetUniString( pGetInfo2->pDriverName );
				XubString aPortName;
				if ( pGetInfo2->pPortName )
					aPortName = ImplSalGetUniString( pGetInfo2->pPortName );
				// pLocation can be 0 (the Windows docu doesn't describe this)
				if ( pGetInfo2->pLocation && strlen( pGetInfo2->pLocation ) )
					pInfo->maLocation = ImplSalGetUniString( pGetInfo2->pLocation );
				else
					pInfo->maLocation = aPortName;
				// pComment can be 0 (the Windows docu doesn't describe this)
				if ( pGetInfo2->pComment )
					pInfo->maComment = ImplSalGetUniString( pGetInfo2->pComment );
				pInfo->mnStatus 	 = ImplWinQueueStatusToSal( pGetInfo2->Status );
				pInfo->mnJobs		 = pGetInfo2->cJobs;
				pInfo->mpSysData	 = new XubString( aPortName );
				pList->Add( pInfo );
				pGetInfo2++;
			}

			bFound = TRUE;
		}
	}

	// read printers from win.ini
	// TODO: MSDN: GetProfileString() should not be called from server
	// code because it is just there for WIN16 compatibility
	UINT	nSize = 4096;
	char*	pBuf = new char[nSize];
	UINT	nRead = GetProfileStringA( aImplDevices, NULL, "", pBuf, nSize );
	while ( nRead >= nSize-2 )
	{
		nSize += 2048;
		delete []pBuf;
		pBuf = new char[nSize];
		nRead = GetProfileStringA( aImplDevices, NULL, "", pBuf, nSize );
	}

	// extract printer names from buffer and fill list
	char* pName = pBuf;
	while ( *pName )
	{
		char*	pPortName;
		char*	pTmp;
		char	aPortBuf[256];
		GetProfileStringA( aImplDevices, pName, "", aPortBuf, sizeof( aPortBuf ) );

		pPortName = aPortBuf;

		// create name
		xub_StrLen nNameLen = sal::static_int_cast<xub_StrLen>(strlen( pName ));
		XubString aName( ImplSalGetUniString( pName, nNameLen ) );

		// get driver name
		pTmp = pPortName;
		while ( *pTmp != ',' )
			pTmp++;
		XubString aDriver( ImplSalGetUniString( pPortName, (USHORT)(pTmp-pPortName) ) );
		pPortName = pTmp;

		// get port names
		do
		{
			pPortName++;
			pTmp = pPortName;
			while ( *pTmp && (*pTmp != ',') )
				pTmp++;

			String aPortName( ImplSalGetUniString( pPortName, (USHORT)(pTmp-pPortName) ) );

			// create new entry
            // look up if printer was already found in first loop
			BOOL bAdd = TRUE;
			if ( pWinInfo2 )
			{
				pGetInfo2 = pWinInfo2;
				for ( n = 0; n < nInfoPrn2; n++ )
				{
					if ( aName.EqualsIgnoreCaseAscii( pGetInfo2->pPrinterName ) )
					{
						bAdd = FALSE;
						break;
					}
					pGetInfo2++;
				}
			}
			// if it's a new printer, add it
			if ( bAdd )
			{
				SalPrinterQueueInfo* pInfo = new SalPrinterQueueInfo;
				pInfo->maPrinterName = aName;
				pInfo->maDriver 	 = aDriver;
				pInfo->maLocation	 = aPortName;
				pInfo->mnStatus 	 = 0;
				pInfo->mnJobs		 = QUEUE_JOBS_DONTKNOW;
				pInfo->mpSysData	 = new XubString( aPortName );
				pList->Add( pInfo );
			}
		}
		while ( *pTmp == ',' );

		pName += nNameLen + 1;
	}

	delete []pBuf;
	rtl_freeMemory( pWinInfo2 );
}

void WinSalInstance::GetPrinterQueueInfo( ImplPrnQueueList* pList )
{
    if( ! aSalShlData.mbWPrinter )
    {
        getPrinterQueueInfoOldStyle( pList );
        return;
    }
	DWORD			i;
	DWORD			nBytes = 0;
	DWORD			nInfoPrn4 = 0;
	PRINTER_INFO_4W* pWinInfo4 = NULL;
	EnumPrintersW( PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL, 4, NULL, 0, &nBytes, &nInfoPrn4 );
	if ( nBytes )
	{
		pWinInfo4 = (PRINTER_INFO_4W*) rtl_allocateMemory( nBytes );
		if ( EnumPrintersW( PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL, 4, (LPBYTE)pWinInfo4, nBytes, &nBytes, &nInfoPrn4 ) )
		{
			for ( i = 0; i < nInfoPrn4; i++ )
			{
				SalPrinterQueueInfo* pInfo = new SalPrinterQueueInfo;
				pInfo->maPrinterName = UniString( reinterpret_cast< const sal_Unicode* >(pWinInfo4[i].pPrinterName) );
				pInfo->mnStatus 	 = 0;
				pInfo->mnJobs		 = 0;
				pInfo->mpSysData	 = NULL;
				pList->Add( pInfo );
			}
		}
        rtl_freeMemory( pWinInfo4 );
	}
}

// -----------------------------------------------------------------------

static void getPrinterQueueStateOldStyle( SalPrinterQueueInfo* pInfo )
{
	DWORD				nBytes = 0;
	DWORD				nInfoRet;
	PRINTER_INFO_2* 	pWinInfo2;
	EnumPrintersA( PRINTER_ENUM_LOCAL, NULL, 2, NULL, 0, &nBytes, &nInfoRet );
	if ( nBytes )
	{
		pWinInfo2 = (PRINTER_INFO_2*) rtl_allocateMemory( nBytes );
		if ( EnumPrintersA( PRINTER_ENUM_LOCAL, NULL, 2, (LPBYTE)pWinInfo2, nBytes, &nBytes, &nInfoRet ) )
		{
			PRINTER_INFO_2* pGetInfo2 = pWinInfo2;
			for ( DWORD i = 0; i < nInfoRet; i++ )
			{
				if ( pInfo->maPrinterName.EqualsAscii( pGetInfo2->pPrinterName ) &&
					 ( pInfo->maDriver.Len() == 0 ||
                       pInfo->maDriver.EqualsAscii( pGetInfo2->pDriverName ) )
                       )
				{
                    XubString aPortName;
                    if ( pGetInfo2->pPortName )
                        aPortName = ImplSalGetUniString( pGetInfo2->pPortName );
                    // pLocation can be 0 (the Windows docu doesn't describe this)
                    if ( pGetInfo2->pLocation && strlen( pGetInfo2->pLocation ) )
                        pInfo->maLocation = ImplSalGetUniString( pGetInfo2->pLocation );
                    else
                        pInfo->maLocation = aPortName;
                    // pComment can be 0 (the Windows docu doesn't describe this)
                    if ( pGetInfo2->pComment )
                        pInfo->maComment = ImplSalGetUniString( pGetInfo2->pComment );
                    pInfo->mnStatus 	 = ImplWinQueueStatusToSal( pGetInfo2->Status );
                    pInfo->mnJobs		 = pGetInfo2->cJobs;
                    if( ! pInfo->mpSysData )
                        pInfo->mpSysData	 = new XubString( aPortName );
					break;
				}

				pGetInfo2++;
			}
		}

		rtl_freeMemory( pWinInfo2 );
	}
}

void WinSalInstance::GetPrinterQueueState( SalPrinterQueueInfo* pInfo )
{
    if( ! aSalShlData.mbWPrinter )
    {
        getPrinterQueueStateOldStyle( pInfo );
        return;
    }
    
    HANDLE hPrinter = 0;
    LPWSTR pPrnName = reinterpret_cast<LPWSTR>(const_cast<sal_Unicode*>(pInfo->maPrinterName.GetBuffer()));
    if( OpenPrinterW( pPrnName, &hPrinter, NULL ) )
    {
        DWORD				nBytes = 0;
        GetPrinterW( hPrinter, 2, NULL, 0, &nBytes );
        if( nBytes )
        {
            PRINTER_INFO_2W* pWinInfo2 = (PRINTER_INFO_2W*)rtl_allocateMemory(nBytes);
            if( GetPrinterW( hPrinter, 2, (LPBYTE)pWinInfo2, nBytes, &nBytes ) )
            {
                if( pWinInfo2->pDriverName )
                    pInfo->maDriver = String( reinterpret_cast< const sal_Unicode* >(pWinInfo2->pDriverName) );
                XubString aPortName;
                if ( pWinInfo2->pPortName )
                    aPortName = String( reinterpret_cast< const sal_Unicode* >(pWinInfo2->pPortName) );
                // pLocation can be 0 (the Windows docu doesn't describe this)
                if ( pWinInfo2->pLocation && *pWinInfo2->pLocation )
                    pInfo->maLocation = String( reinterpret_cast< const sal_Unicode* >(pWinInfo2->pLocation) );
                else
                    pInfo->maLocation = aPortName;
                // pComment can be 0 (the Windows docu doesn't describe this)
                if ( pWinInfo2->pComment )
                    pInfo->maComment = String( reinterpret_cast< const sal_Unicode* >(pWinInfo2->pComment) );
                pInfo->mnStatus 	 = ImplWinQueueStatusToSal( pWinInfo2->Status );
                pInfo->mnJobs		 = pWinInfo2->cJobs;
                if( ! pInfo->mpSysData )
                    pInfo->mpSysData	 = new XubString( aPortName );
            }
            rtl_freeMemory(pWinInfo2);
        }
        ClosePrinter( hPrinter );
    }
}

// -----------------------------------------------------------------------

void WinSalInstance::DeletePrinterQueueInfo( SalPrinterQueueInfo* pInfo )
{
	delete (String*)(pInfo->mpSysData);
	delete pInfo;
}

// -----------------------------------------------------------------------
XubString WinSalInstance::GetDefaultPrinter()
{
    static bool bGetDefPrtAPI = true;
    static BOOL(WINAPI*pGetDefaultPrinter)(LPWSTR,LPDWORD) = NULL;
    // try to use GetDefaultPrinter API (not available prior to W2000)
    if( bGetDefPrtAPI )
    {
        bGetDefPrtAPI = false;
        // check for W2k and XP
        if( aSalShlData.maVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT && aSalShlData.maVersionInfo.dwMajorVersion >= 5 )
        {
            OUString aLibraryName( RTL_CONSTASCII_USTRINGPARAM( "winspool.drv" ) );
            oslModule pLib = osl_loadModule( aLibraryName.pData, SAL_LOADMODULE_DEFAULT );
            oslGenericFunction pFunc = NULL;
            if( pLib )
            {
                OUString queryFuncName( RTL_CONSTASCII_USTRINGPARAM( "GetDefaultPrinterW" ) );
                pFunc = osl_getFunctionSymbol( pLib, queryFuncName.pData );
            }

            pGetDefaultPrinter = (BOOL(WINAPI*)(LPWSTR,LPDWORD)) pFunc;
        }
    }
    if( pGetDefaultPrinter )
    {
        DWORD   nChars = 0;
        pGetDefaultPrinter( NULL, &nChars );
        if( nChars )
        {
            LPWSTR  pStr = (LPWSTR)rtl_allocateMemory(nChars*sizeof(WCHAR));
            XubString aDefPrt;
            if( pGetDefaultPrinter( pStr, &nChars ) )
            {
                aDefPrt = reinterpret_cast<sal_Unicode* >(pStr);
            }
            rtl_freeMemory( pStr );
            if( aDefPrt.Len() )
                return aDefPrt;
        }
    }

	// get default printer from win.ini
	char szBuffer[256];
	GetProfileStringA( aImplWindows, aImplDevice, "", szBuffer, sizeof( szBuffer ) );
	if ( szBuffer[0] )
	{
		// Printername suchen
		char* pBuf = szBuffer;
		char* pTmp = pBuf;
		while ( *pTmp && (*pTmp != ',') )
			pTmp++;
		return ImplSalGetUniString( pBuf, (xub_StrLen)(pTmp-pBuf) );
	}
	else
		return XubString();
}

// =======================================================================

static DWORD ImplDeviceCaps( WinSalInfoPrinter* pPrinter, WORD nCaps,
							 BYTE* pOutput, const ImplJobSetup* pSetupData )
{
    if( aSalShlData.mbWPrinter )
    {
        DEVMODEW* pDevMode;
        if ( !pSetupData || !pSetupData->mpDriverData )
            pDevMode = NULL;
        else
            pDevMode = SAL_DEVMODE_W( pSetupData );
    
        return DeviceCapabilitiesW( reinterpret_cast<LPCWSTR>(pPrinter->maDeviceName.GetBuffer()),
                                    reinterpret_cast<LPCWSTR>(pPrinter->maPortName.GetBuffer()),
                                    nCaps, (LPWSTR)pOutput, pDevMode );
    }
    else
    {
        DEVMODEA* pDevMode;
        if ( !pSetupData || !pSetupData->mpDriverData )
            pDevMode = NULL;
        else
            pDevMode = SAL_DEVMODE_A( pSetupData );
    
        return DeviceCapabilitiesA( ImplSalGetWinAnsiString( pPrinter->maDeviceName, TRUE ).GetBuffer(),
                                ImplSalGetWinAnsiString( pPrinter->maPortName, TRUE ).GetBuffer(),
                                nCaps, (LPSTR)pOutput, pDevMode );
    }
}

// -----------------------------------------------------------------------

static BOOL ImplTestSalJobSetup( WinSalInfoPrinter* pPrinter,
								 ImplJobSetup* pSetupData, BOOL bDelete )
{
	if ( pSetupData && pSetupData->mpDriverData )
	{
        // signature and size must fit to avoid using
        // JobSetups from a wrong system

        // initialize versions from jobsetup
        // those will be overwritten with driver's version
        DEVMODEA* pDevModeA = NULL;
        DEVMODEW* pDevModeW = NULL;
        LONG dmSpecVersion = -1;
        LONG dmDriverVersion = -1;
        SalDriverData* pSalDriverData = (SalDriverData*)pSetupData->mpDriverData;
        BYTE* pDriverData = ((BYTE*)pSalDriverData) + pSalDriverData->mnDriverOffset;
        if( pSalDriverData->mnVersion == SAL_DRIVERDATA_VERSION_W )
        {
            if( aSalShlData.mbWPrinter )
                pDevModeW = (DEVMODEW*)pDriverData;
        }
        else if( pSalDriverData->mnVersion == SAL_DRIVERDATA_VERSION_A )
        {
            if( ! aSalShlData.mbWPrinter )
                pDevModeA = (DEVMODEA*)pDriverData;
        }

        long nSysJobSize = -1;
        if( pPrinter && ( pDevModeA || pDevModeW ) )
        {
            // just too many driver crashes in that area -> check the dmSpecVersion and dmDriverVersion fields always !!!
            // this prevents using the jobsetup between different Windows versions (eg from XP to 9x) but we
            // can avoid potential driver crashes as their jobsetups are often not compatible
            // #110800#, #111151#, #112381#, #i16580#, #i14173# and perhaps #112375#
            ByteString aPrinterNameA= ImplSalGetWinAnsiString( pPrinter->maDeviceName, TRUE );
            HANDLE hPrn;
            LPWSTR pPrinterNameW = reinterpret_cast<LPWSTR>(const_cast<sal_Unicode*>(pPrinter->maDeviceName.GetBuffer()));
            if ( ! aSalShlData.mbWPrinter )
            {
                if ( !OpenPrinterA( (LPSTR)aPrinterNameA.GetBuffer(), &hPrn, NULL ) )
                    return FALSE;
            }
            else
                if ( !OpenPrinterW( pPrinterNameW, &hPrn, NULL ) )
                    return FALSE;

            // #131642# hPrn==HGDI_ERROR even though OpenPrinter() succeeded!
            if( hPrn == HGDI_ERROR )
                return FALSE;

            if( aSalShlData.mbWPrinter )
            {
                nSysJobSize = DocumentPropertiesW( 0, hPrn,
                                                   pPrinterNameW,
                                                   NULL, NULL, 0 );
            }
            else
            {
                nSysJobSize = DocumentPropertiesA( 0, hPrn,
                                                   (LPSTR)aPrinterNameA.GetBuffer(),
                                                   NULL, NULL, 0 );
            }
            
            if( nSysJobSize < 0 )
            {
                ClosePrinter( hPrn );
                return FALSE;
            }
            BYTE *pBuffer = (BYTE*)_alloca( nSysJobSize );
            LONG nRet = -1;
            if( aSalShlData.mbWPrinter )
            {
                nRet = DocumentPropertiesW( 0, hPrn,
                                            pPrinterNameW,
                                            (LPDEVMODEW)pBuffer, NULL, DM_OUT_BUFFER );
            }
            else
            {
                nRet = DocumentPropertiesA( 0, hPrn,
                                            (LPSTR)aPrinterNameA.GetBuffer(),
                                            (LPDEVMODEA)pBuffer, NULL, DM_OUT_BUFFER );
            }
            if( nRet < 0 )
            {
                ClosePrinter( hPrn );
                return FALSE;
            }

            // the spec version differs between the windows platforms, ie 98,NT,2000/XP
            // this allows us to throw away printer settings from other platforms that might crash a buggy driver
            // we check the driver version as well
            dmSpecVersion = aSalShlData.mbWPrinter ? ((DEVMODEW*)pBuffer)->dmSpecVersion : ((DEVMODEA*)pBuffer)->dmSpecVersion;
            dmDriverVersion = aSalShlData.mbWPrinter ? ((DEVMODEW*)pBuffer)->dmDriverVersion : ((DEVMODEA*)pBuffer)->dmDriverVersion;

            ClosePrinter( hPrn );
        }
        SalDriverData* pSetupDriverData = (SalDriverData*)(pSetupData->mpDriverData);
		if ( (pSetupData->mnSystem == JOBSETUP_SYSTEM_WINDOWS) &&
             (pPrinter->maDriverName == pSetupData->maDriver) &&
			 (pSetupData->mnDriverDataLen > sizeof( SalDriverData )) &&
             (long)(pSetupData->mnDriverDataLen - pSetupDriverData->mnDriverOffset) == nSysJobSize &&
			 pSetupDriverData->mnSysSignature == SAL_DRIVERDATA_SYSSIGN )
        {
            if( pDevModeA &&
                (dmSpecVersion == pDevModeA->dmSpecVersion) &&
                (dmDriverVersion == pDevModeA->dmDriverVersion) )
                return TRUE;
            if( pDevModeW &&
                (dmSpecVersion == pDevModeW->dmSpecVersion) &&
                (dmDriverVersion == pDevModeW->dmDriverVersion) )
                return TRUE;
        }
		if ( bDelete )
		{
			rtl_freeMemory( pSetupData->mpDriverData );
			pSetupData->mpDriverData = NULL;
			pSetupData->mnDriverDataLen = 0;
		}
	}

	return FALSE;
}

// -----------------------------------------------------------------------

static BOOL ImplUpdateSalJobSetup( WinSalInfoPrinter* pPrinter, ImplJobSetup* pSetupData,
								   BOOL bIn, WinSalFrame* pVisibleDlgParent )
{
    ByteString aPrinterNameA = ImplSalGetWinAnsiString( pPrinter->maDeviceName, TRUE );
    HANDLE hPrn;
    LPWSTR pPrinterNameW = reinterpret_cast<LPWSTR>(const_cast<sal_Unicode*>(pPrinter->maDeviceName.GetBuffer()));
    if( aSalShlData.mbWPrinter )
    {
        if ( !OpenPrinterW( pPrinterNameW, &hPrn, NULL ) )
            return FALSE;
    }
    else
    {
        if ( !OpenPrinterA( (LPSTR)aPrinterNameA.GetBuffer(), &hPrn, NULL ) )
            return FALSE;
    }
    // #131642# hPrn==HGDI_ERROR even though OpenPrinter() succeeded!
    if( hPrn == HGDI_ERROR )
        return FALSE;

	LONG			nRet;
	LONG			nSysJobSize = -1;
	HWND			hWnd = 0;
	DWORD			nMode = DM_OUT_BUFFER;
	ULONG			nDriverDataLen = 0;
	SalDriverData*	pOutBuffer = NULL;
    BYTE*           pInBuffer = NULL;

    if( aSalShlData.mbWPrinter )
    {
        nSysJobSize = DocumentPropertiesW( hWnd, hPrn,
                                           pPrinterNameW,
                                           NULL, NULL, 0 );
    }
    else
        nSysJobSize = DocumentPropertiesA( hWnd, hPrn,
                                           (LPSTR)ImplSalGetWinAnsiString( pPrinter->maDeviceName, TRUE ).GetBuffer(),
                                           NULL, NULL, 0 );
	if ( nSysJobSize < 0 )
	{
		ClosePrinter( hPrn );
		return FALSE;
	}

	// Outputbuffer anlegen
	nDriverDataLen				= sizeof(SalDriverData) + nSysJobSize-1;
	pOutBuffer					= (SalDriverData*)rtl_allocateZeroMemory( nDriverDataLen );
	pOutBuffer->mnSysSignature	= SAL_DRIVERDATA_SYSSIGN;
	pOutBuffer->mnVersion		= aSalShlData.mbWPrinter ? SAL_DRIVERDATA_VERSION_W : SAL_DRIVERDATA_VERSION_A;
    // calculate driver data offset including structure padding
	pOutBuffer->mnDriverOffset	= sal::static_int_cast<USHORT>(
                                    (char*)pOutBuffer->maDriverData -
                                    (char*)pOutBuffer );

	// Testen, ob wir einen geeigneten Inputbuffer haben
	if ( bIn && ImplTestSalJobSetup( pPrinter, pSetupData, FALSE ) )
	{
		pInBuffer = (BYTE*)pSetupData->mpDriverData + ((SalDriverData*)pSetupData->mpDriverData)->mnDriverOffset;
		nMode |= DM_IN_BUFFER;
	}

	// Testen, ob Dialog angezeigt werden soll
	if ( pVisibleDlgParent )
	{
		hWnd = pVisibleDlgParent->mhWnd;
		nMode |= DM_IN_PROMPT;
	}

	// Release mutex, in the other case we don't get paints and so on
    ULONG nMutexCount=0;
    if ( pVisibleDlgParent )
        nMutexCount = ImplSalReleaseYieldMutex();
    
    BYTE* pOutDevMode = (((BYTE*)pOutBuffer) + pOutBuffer->mnDriverOffset);
    if( aSalShlData.mbWPrinter )
    {
        nRet = DocumentPropertiesW( hWnd, hPrn,
                                    pPrinterNameW,
                                    (LPDEVMODEW)pOutDevMode, (LPDEVMODEW)pInBuffer, nMode );
    }
    else
    {
        nRet = DocumentPropertiesA( hWnd, hPrn,
                                    (LPSTR)ImplSalGetWinAnsiString( pPrinter->maDeviceName, TRUE ).GetBuffer(),
                                    (LPDEVMODEA)pOutDevMode, (LPDEVMODEA)pInBuffer, nMode );
    }
    if ( pVisibleDlgParent )
        ImplSalAcquireYieldMutex( nMutexCount );
	ClosePrinter( hPrn );

	if( (nRet < 0) || (pVisibleDlgParent && (nRet == IDCANCEL)) )
	{
		rtl_freeMemory( pOutBuffer );
		return FALSE;
	}

    // fill up string buffers with 0 so they do not influence a JobSetup's memcmp
    if( aSalShlData.mbWPrinter )
    {
        if( ((LPDEVMODEW)pOutDevMode)->dmSize >= 64 )
        {
            sal_Int32 nLen = rtl_ustr_getLength( (const sal_Unicode*)((LPDEVMODEW)pOutDevMode)->dmDeviceName );
            if ( nLen < sizeof( ((LPDEVMODEW)pOutDevMode)->dmDeviceName )/sizeof(sal_Unicode) )
                memset( ((LPDEVMODEW)pOutDevMode)->dmDeviceName+nLen, 0, sizeof( ((LPDEVMODEW)pOutDevMode)->dmDeviceName )-(nLen*sizeof(sal_Unicode)) );
        }
        if( ((LPDEVMODEW)pOutDevMode)->dmSize >= 166 )
        {
            sal_Int32 nLen = rtl_ustr_getLength( (const sal_Unicode*)((LPDEVMODEW)pOutDevMode)->dmFormName );
            if ( nLen < sizeof( ((LPDEVMODEW)pOutDevMode)->dmFormName )/sizeof(sal_Unicode) )
                memset( ((LPDEVMODEW)pOutDevMode)->dmFormName+nLen, 0, sizeof( ((LPDEVMODEW)pOutDevMode)->dmFormName )-(nLen*sizeof(sal_Unicode)) );
        }
    }
    else
    {
        if( ((LPDEVMODEA)pOutDevMode)->dmSize >= 32 )
        {
            sal_Int32 nLen = strlen( (const char*)((LPDEVMODEA)pOutDevMode)->dmDeviceName );
            if ( nLen < sizeof( ((LPDEVMODEA)pOutDevMode)->dmDeviceName ) )
                memset( ((LPDEVMODEA)pOutDevMode)->dmDeviceName+nLen, 0, sizeof( ((LPDEVMODEA)pOutDevMode)->dmDeviceName )-nLen );
        }
        if( ((LPDEVMODEA)pOutDevMode)->dmSize >= 102 )
        {
            sal_Int32 nLen = strlen( (const char*)((LPDEVMODEA)pOutDevMode)->dmFormName );
            if ( nLen < sizeof( ((LPDEVMODEA)pOutDevMode)->dmFormName ) )
                memset( ((LPDEVMODEA)pOutDevMode)->dmFormName+nLen, 0, sizeof( ((LPDEVMODEA)pOutDevMode)->dmFormName )-nLen );
        }
    }

	// update data
	if ( pSetupData->mpDriverData )
		rtl_freeMemory( pSetupData->mpDriverData );
	pSetupData->mnDriverDataLen = nDriverDataLen;
	pSetupData->mpDriverData	= (BYTE*)pOutBuffer;
	pSetupData->mnSystem		= JOBSETUP_SYSTEM_WINDOWS;

	return TRUE;
}

// -----------------------------------------------------------------------

#define DECLARE_DEVMODE( i )\
    DEVMODEA* pDevModeA = SAL_DEVMODE_A(i);\
    DEVMODEW* pDevModeW = SAL_DEVMODE_W(i);\
    if( pDevModeA == NULL && pDevModeW == NULL )\
        return

#define CHOOSE_DEVMODE(i)\
    (pDevModeW ? pDevModeW->i : pDevModeA->i)

static void ImplDevModeToJobSetup( WinSalInfoPrinter* pPrinter, ImplJobSetup* pSetupData, ULONG nFlags )
{
	if ( !pSetupData || !pSetupData->mpDriverData )
		return;

    DECLARE_DEVMODE( pSetupData );

	// Orientation
	if ( nFlags & SAL_JOBSET_ORIENTATION )
	{
		if ( CHOOSE_DEVMODE(dmOrientation) == DMORIENT_PORTRAIT )
			pSetupData->meOrientation = ORIENTATION_PORTRAIT;
		else if ( CHOOSE_DEVMODE(dmOrientation) == DMORIENT_LANDSCAPE )
			pSetupData->meOrientation = ORIENTATION_LANDSCAPE;
	}

	// PaperBin
	if ( nFlags & SAL_JOBSET_PAPERBIN )
	{
		ULONG nCount = ImplDeviceCaps( pPrinter, DC_BINS, NULL, pSetupData );

		if ( nCount && (nCount != GDI_ERROR) )
		{
			WORD* pBins = (WORD*)rtl_allocateZeroMemory( nCount*sizeof(WORD) );
			ImplDeviceCaps( pPrinter, DC_BINS, (BYTE*)pBins, pSetupData );
			pSetupData->mnPaperBin = 0;

			// search the right bin and assign index to mnPaperBin
			for( ULONG i = 0; i < nCount; i++ )
			{
				if( CHOOSE_DEVMODE(dmDefaultSource) == pBins[ i ] )
				{
					pSetupData->mnPaperBin = (USHORT)i;
					break;
				}
			}

			rtl_freeMemory( pBins );
		}
	}

	// PaperSize
	if ( nFlags & SAL_JOBSET_PAPERSIZE )
	{
		if( (CHOOSE_DEVMODE(dmFields) & (DM_PAPERWIDTH|DM_PAPERLENGTH)) == (DM_PAPERWIDTH|DM_PAPERLENGTH) )
		{
		    pSetupData->mnPaperWidth  = CHOOSE_DEVMODE(dmPaperWidth)*10;
		    pSetupData->mnPaperHeight = CHOOSE_DEVMODE(dmPaperLength)*10;
		}
		else
		{
			ULONG	nPaperCount = ImplDeviceCaps( pPrinter, DC_PAPERS, NULL, pSetupData );
			WORD*	pPapers = NULL;
			ULONG	nPaperSizeCount = ImplDeviceCaps( pPrinter, DC_PAPERSIZE, NULL, pSetupData );
			POINT*	pPaperSizes = NULL;
			if ( nPaperCount && (nPaperCount != GDI_ERROR) )
			{
				pPapers = (WORD*)rtl_allocateZeroMemory(nPaperCount*sizeof(WORD));
				ImplDeviceCaps( pPrinter, DC_PAPERS, (BYTE*)pPapers, pSetupData );
			}
			if ( nPaperSizeCount && (nPaperSizeCount != GDI_ERROR) )
			{
				pPaperSizes = (POINT*)rtl_allocateZeroMemory(nPaperSizeCount*sizeof(POINT));
				ImplDeviceCaps( pPrinter, DC_PAPERSIZE, (BYTE*)pPaperSizes, pSetupData );
			}
			if( nPaperSizeCount == nPaperCount && pPaperSizes && pPapers )
			{
				for( ULONG i = 0; i < nPaperCount; i++ )
				{
					if( pPapers[ i ] == CHOOSE_DEVMODE(dmPaperSize) )
					{
						pSetupData->mnPaperWidth  = pPaperSizes[ i ].x*10;
						pSetupData->mnPaperHeight = pPaperSizes[ i ].y*10;
						break;
					}
				}
			}
			if( pPapers )
				rtl_freeMemory( pPapers );
			if( pPaperSizes )
				rtl_freeMemory( pPaperSizes );
		}
		switch( CHOOSE_DEVMODE(dmPaperSize) )
		{
			case( DMPAPER_LETTER ):
				pSetupData->mePaperFormat = PAPER_LETTER;
				break;
			case( DMPAPER_TABLOID ):
				pSetupData->mePaperFormat = PAPER_TABLOID;
				break;
			case( DMPAPER_LEDGER ):
				pSetupData->mePaperFormat = PAPER_LEDGER;
				break;
			case( DMPAPER_LEGAL ):
				pSetupData->mePaperFormat = PAPER_LEGAL;
				break;
			case( DMPAPER_STATEMENT ):
				pSetupData->mePaperFormat = PAPER_STATEMENT;
				break;
			case( DMPAPER_EXECUTIVE ):
				pSetupData->mePaperFormat = PAPER_EXECUTIVE;
				break;
			case( DMPAPER_A3 ):
				pSetupData->mePaperFormat = PAPER_A3;
				break;
			case( DMPAPER_A4 ):
				pSetupData->mePaperFormat = PAPER_A4;
				break;
			case( DMPAPER_A5 ):
				pSetupData->mePaperFormat = PAPER_A5;
				break;
			//See http://wiki.services.openoffice.org/wiki/DefaultPaperSize
			//i.e.
			//http://msdn.microsoft.com/en-us/library/dd319099(VS.85).aspx
			//DMPAPER_B4	12	B4 (JIS) 257 x 364 mm
			//http://partners.adobe.com/public/developer/en/ps/5003.PPD_Spec_v4.3.pdf
			//also says that the MS DMPAPER_B4 is JIS, which makes most sense. And
			//matches our Excel filter's belief about the matching XlPaperSize
			//enumeration.
			//
			//http://msdn.microsoft.com/en-us/library/ms776398(VS.85).aspx said
			////"DMPAPER_B4 	12 	B4 (JIS) 250 x 354"
			//which is bogus as it's either JIS 257 × 364 or ISO 250 × 353
			//(cmc)
			case( DMPAPER_B4 ):
				pSetupData->mePaperFormat = PAPER_B4_JIS;
				break;
			case( DMPAPER_B5 ):
				pSetupData->mePaperFormat = PAPER_B5_JIS;
				break;
			case( DMPAPER_QUARTO ):
				pSetupData->mePaperFormat = PAPER_QUARTO;
				break;
			case( DMPAPER_10X14 ):
				pSetupData->mePaperFormat = PAPER_10x14;
				break;
			case( DMPAPER_NOTE ):
				pSetupData->mePaperFormat = PAPER_LETTER;
				break;
			case( DMPAPER_ENV_9 ):
				pSetupData->mePaperFormat = PAPER_ENV_9;
				break;
			case( DMPAPER_ENV_10 ):
				pSetupData->mePaperFormat = PAPER_ENV_10;
				break;
			case( DMPAPER_ENV_11 ):
				pSetupData->mePaperFormat = PAPER_ENV_11;
				break;
			case( DMPAPER_ENV_12 ):
				pSetupData->mePaperFormat = PAPER_ENV_12;
				break;
			case( DMPAPER_ENV_14 ):
				pSetupData->mePaperFormat = PAPER_ENV_14;
				break;
			case( DMPAPER_CSHEET ):
				pSetupData->mePaperFormat = PAPER_C;
				break;
			case( DMPAPER_DSHEET ):
				pSetupData->mePaperFormat = PAPER_D;
				break;
			case( DMPAPER_ESHEET ):
				pSetupData->mePaperFormat = PAPER_E;
				break;
			case( DMPAPER_ENV_DL):
				pSetupData->mePaperFormat = PAPER_ENV_DL;
				break;
			case( DMPAPER_ENV_C5):
				pSetupData->mePaperFormat = PAPER_ENV_C5;
				break;
			case( DMPAPER_ENV_C3):
				pSetupData->mePaperFormat = PAPER_ENV_C3;
				break;
			case( DMPAPER_ENV_C4):
				pSetupData->mePaperFormat = PAPER_ENV_C4;
				break;
			case( DMPAPER_ENV_C6):
				pSetupData->mePaperFormat = PAPER_ENV_C6;
				break;
			case( DMPAPER_ENV_C65):
				pSetupData->mePaperFormat = PAPER_ENV_C65;
				break;
			case( DMPAPER_ENV_ITALY ):
				pSetupData->mePaperFormat = PAPER_ENV_ITALY;
				break;
			case( DMPAPER_ENV_MONARCH ):
				pSetupData->mePaperFormat = PAPER_ENV_MONARCH;
				break;
			case( DMPAPER_ENV_PERSONAL ):
				pSetupData->mePaperFormat = PAPER_ENV_PERSONAL;
				break;
			case( DMPAPER_FANFOLD_US ):
				pSetupData->mePaperFormat = PAPER_FANFOLD_US;
				break;
			case( DMPAPER_FANFOLD_STD_GERMAN ):
				pSetupData->mePaperFormat = PAPER_FANFOLD_DE;
				break;
			case( DMPAPER_FANFOLD_LGL_GERMAN ):
				pSetupData->mePaperFormat = PAPER_FANFOLD_LEGAL_DE;
				break;
			case( DMPAPER_ISO_B4 ):
				pSetupData->mePaperFormat = PAPER_B4_ISO;
				break;
			case( DMPAPER_JAPANESE_POSTCARD ):
				pSetupData->mePaperFormat = PAPER_POSTCARD_JP;
				break;
			case( DMPAPER_9X11 ):
				pSetupData->mePaperFormat = PAPER_9x11;
				break;
			case( DMPAPER_10X11 ):
				pSetupData->mePaperFormat = PAPER_10x11;
				break;
			case( DMPAPER_15X11 ):
				pSetupData->mePaperFormat = PAPER_15x11;
				break;
			case( DMPAPER_ENV_INVITE ):
				pSetupData->mePaperFormat = PAPER_ENV_INVITE;
				break;
			case( DMPAPER_A_PLUS ):
				pSetupData->mePaperFormat = PAPER_A_PLUS;
				break;
			case( DMPAPER_B_PLUS ):
				pSetupData->mePaperFormat = PAPER_B_PLUS;
				break;
			case( DMPAPER_LETTER_PLUS ):
				pSetupData->mePaperFormat = PAPER_LETTER_PLUS;
				break;
			case( DMPAPER_A4_PLUS ):
				pSetupData->mePaperFormat = PAPER_A4_PLUS;
				break;
			case( DMPAPER_A2 ):
				pSetupData->mePaperFormat = PAPER_A2;
				break;
			case( DMPAPER_DBL_JAPANESE_POSTCARD ):
				pSetupData->mePaperFormat = PAPER_DOUBLEPOSTCARD_JP;
				break;
			case( DMPAPER_A6 ):
				pSetupData->mePaperFormat = PAPER_A6;
				break;
			case( DMPAPER_B6_JIS ):
				pSetupData->mePaperFormat = PAPER_B6_JIS;
				break;
			case( DMPAPER_12X11 ):
				pSetupData->mePaperFormat = PAPER_12x11;
				break;
			default:
				pSetupData->mePaperFormat = PAPER_USER;
				break;
		}
	}
}

// -----------------------------------------------------------------------

static void ImplJobSetupToDevMode( WinSalInfoPrinter* pPrinter, ImplJobSetup* pSetupData, ULONG nFlags )
{
	if ( !pSetupData || !pSetupData->mpDriverData )
		return;

    DECLARE_DEVMODE( pSetupData );

	// Orientation
	if ( nFlags & SAL_JOBSET_ORIENTATION )
	{
		CHOOSE_DEVMODE(dmFields) |= DM_ORIENTATION;
		if ( pSetupData->meOrientation == ORIENTATION_PORTRAIT )
			CHOOSE_DEVMODE(dmOrientation) = DMORIENT_PORTRAIT;
		else
			CHOOSE_DEVMODE(dmOrientation) = DMORIENT_LANDSCAPE;
	}

	// PaperBin
	if ( nFlags & SAL_JOBSET_PAPERBIN )
	{
		ULONG nCount = ImplDeviceCaps( pPrinter, DC_BINS, NULL, pSetupData );

		if ( nCount && (nCount != GDI_ERROR) )
		{
			WORD* pBins = (WORD*)rtl_allocateZeroMemory(nCount*sizeof(WORD));
			ImplDeviceCaps( pPrinter, DC_BINS, (BYTE*)pBins, pSetupData );
			CHOOSE_DEVMODE(dmFields) |= DM_DEFAULTSOURCE;
			CHOOSE_DEVMODE(dmDefaultSource) = pBins[ pSetupData->mnPaperBin ];
			rtl_freeMemory( pBins );
		}
	}

	// PaperSize
	if ( nFlags & SAL_JOBSET_PAPERSIZE )
	{
		CHOOSE_DEVMODE(dmFields)		|= DM_PAPERSIZE;
		CHOOSE_DEVMODE(dmPaperWidth)	 = 0;
		CHOOSE_DEVMODE(dmPaperLength)    = 0;

		switch( pSetupData->mePaperFormat )
		{
			case( PAPER_A2 ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_A2;
				break;
			case( PAPER_A3 ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_A3;
				break;
			case( PAPER_A4 ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_A4;
				break;
			case( PAPER_A5 ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_A5;
				break;
			case( PAPER_B4_ISO):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_ISO_B4;
				break;
			case( PAPER_LETTER ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_LETTER;
				break;
			case( PAPER_LEGAL ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_LEGAL;
				break;
			case( PAPER_TABLOID ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_TABLOID;
				break;
#if 0
			//http://msdn.microsoft.com/en-us/library/ms776398(VS.85).aspx
			//DMPAPER_ENV_B6 is documented as:
			//"DMPAPER_ENV_B6 	35 	Envelope B6 176 x 125 mm"
			//which is the wrong way around, it is surely 125 x 176, i.e.
			//compare DMPAPER_ENV_B4 and DMPAPER_ENV_B4 as
			//DMPAPER_ENV_B4 	33 	Envelope B4 250 x 353 mm
			//DMPAPER_ENV_B5 	34 	Envelope B5 176 x 250 mm
			case( PAPER_B6_ISO ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_ENV_B6;
				break;
#endif
			case( PAPER_ENV_C4 ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_ENV_C4;
				break;
			case( PAPER_ENV_C5 ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_ENV_C5;
				break;
			case( PAPER_ENV_C6 ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_ENV_C6;
				break;
			case( PAPER_ENV_C65 ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_ENV_C65;
				break;
			case( PAPER_ENV_DL ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_ENV_DL;
				break;
			case( PAPER_C ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_CSHEET;
				break;
			case( PAPER_D ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_DSHEET;
				break;
			case( PAPER_E ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_ESHEET;
				break;
			case( PAPER_EXECUTIVE ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_EXECUTIVE;
				break;
			case( PAPER_FANFOLD_LEGAL_DE ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_FANFOLD_LGL_GERMAN;
				break;
			case( PAPER_ENV_MONARCH ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_ENV_MONARCH;
				break;
			case( PAPER_ENV_PERSONAL ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_ENV_PERSONAL;
				break;
			case( PAPER_ENV_9 ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_ENV_9;
				break;
			case( PAPER_ENV_10 ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_ENV_10;
				break;
			case( PAPER_ENV_11 ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_ENV_11;
				break;
			case( PAPER_ENV_12 ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_ENV_12;
				break;
			//See the comments on DMPAPER_B4 above
			case( PAPER_B4_JIS ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_B4;
				break;
			case( PAPER_B5_JIS ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_B5;
				break;
			case( PAPER_B6_JIS ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_B6_JIS;
				break;
			case( PAPER_LEDGER ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_LEDGER;
				break;
			case( PAPER_STATEMENT ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_STATEMENT;
				break;
			case( PAPER_10x14 ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_10X14;
				break;
			case( PAPER_ENV_14 ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_ENV_14;
				break;
			case( PAPER_ENV_C3 ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_ENV_C3;
				break;
			case( PAPER_ENV_ITALY ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_ENV_ITALY;
				break;
			case( PAPER_FANFOLD_US ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_FANFOLD_US;
				break;
			case( PAPER_FANFOLD_DE ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_FANFOLD_STD_GERMAN;
				break;
			case( PAPER_POSTCARD_JP ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_JAPANESE_POSTCARD;
				break;
			case( PAPER_9x11 ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_9X11;
				break;
			case( PAPER_10x11 ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_10X11;
				break;
			case( PAPER_15x11 ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_15X11;
				break;
			case( PAPER_ENV_INVITE ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_ENV_INVITE;
				break;
			case( PAPER_A_PLUS ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_A_PLUS;
				break;
			case( PAPER_B_PLUS ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_B_PLUS;
				break;
			case( PAPER_LETTER_PLUS ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_LETTER_PLUS;
				break;
			case( PAPER_A4_PLUS ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_A4_PLUS;
				break;
			case( PAPER_DOUBLEPOSTCARD_JP ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_DBL_JAPANESE_POSTCARD;
				break;
			case( PAPER_A6 ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_A6;
				break;
			case( PAPER_12x11 ):
				CHOOSE_DEVMODE(dmPaperSize) = DMPAPER_12X11;
				break;
			default:
			{
				short	nPaper = 0;
				ULONG	nPaperCount = ImplDeviceCaps( pPrinter, DC_PAPERS, NULL, pSetupData );
				WORD*	pPapers = NULL;
				ULONG	nPaperSizeCount = ImplDeviceCaps( pPrinter, DC_PAPERSIZE, NULL, pSetupData );
				POINT*	pPaperSizes = NULL;
				DWORD	nLandscapeAngle = ImplDeviceCaps( pPrinter, DC_ORIENTATION, NULL, pSetupData );
				if ( nPaperCount && (nPaperCount != GDI_ERROR) )
				{
					pPapers = (WORD*)rtl_allocateZeroMemory(nPaperCount*sizeof(WORD));
					ImplDeviceCaps( pPrinter, DC_PAPERS, (BYTE*)pPapers, pSetupData );
				}
				if ( nPaperSizeCount && (nPaperSizeCount != GDI_ERROR) )
				{
					pPaperSizes = (POINT*)rtl_allocateZeroMemory(nPaperSizeCount*sizeof(POINT));
					ImplDeviceCaps( pPrinter, DC_PAPERSIZE, (BYTE*)pPaperSizes, pSetupData );
				}
				if ( (nPaperSizeCount == nPaperCount) && pPapers && pPaperSizes )
				{
                    PaperInfo aInfo(pSetupData->mnPaperWidth, pSetupData->mnPaperHeight);
					// compare paper formats and select a good match
					for ( ULONG i = 0; i < nPaperCount; i++ )
					{
						if ( aInfo.sloppyEqual(PaperInfo(pPaperSizes[i].x*10, pPaperSizes[i].y*10)))
						{
							nPaper = pPapers[i];
							break;
						}
					}

					// If the printer supports landscape orientation, check paper sizes again
					// with landscape orientation. This is necessary as a printer driver provides
					// all paper sizes with portrait orientation only!!
					if ( !nPaper && nLandscapeAngle != 0 )
					{
                        PaperInfo aRotatedInfo(pSetupData->mnPaperHeight, pSetupData->mnPaperWidth);
						for ( ULONG i = 0; i < nPaperCount; i++ )
						{
							if ( aRotatedInfo.sloppyEqual(PaperInfo(pPaperSizes[i].x*10, pPaperSizes[i].y*10)) )
							{
								nPaper = pPapers[i];
								break;
							}
						}
					}

					if ( nPaper )
						CHOOSE_DEVMODE(dmPaperSize) = nPaper;
				}

				if ( !nPaper )
				{
					CHOOSE_DEVMODE(dmFields)	   |= DM_PAPERLENGTH | DM_PAPERWIDTH;
					CHOOSE_DEVMODE(dmPaperSize)    	= DMPAPER_USER;
					CHOOSE_DEVMODE(dmPaperWidth)	= (short)(pSetupData->mnPaperWidth/10);
					CHOOSE_DEVMODE(dmPaperLength)   = (short)(pSetupData->mnPaperHeight/10);
				}

				if ( pPapers )
					rtl_freeMemory(pPapers);
				if ( pPaperSizes )
					rtl_freeMemory(pPaperSizes);

				break;
			}
		}
	}
}

// -----------------------------------------------------------------------

static HDC ImplCreateICW_WithCatch( LPWSTR pDriver,
	                                LPCWSTR pDevice,
                                    LPDEVMODEW pDevMode )
{
    HDC hDC = 0;
	CATCH_DRIVER_EX_BEGIN;
    hDC = CreateICW( pDriver, pDevice, 0, pDevMode );
	CATCH_DRIVER_EX_END_2( "exception in CreateICW" );
    return hDC;
}

static HDC ImplCreateICA_WithCatch( char* pDriver,
	                                char* pDevice,
                                    LPDEVMODEA pDevMode )
{
    HDC hDC = 0;
	CATCH_DRIVER_EX_BEGIN;
    hDC = CreateICA( pDriver, pDevice, 0, pDevMode );
	CATCH_DRIVER_EX_END_2( "exception in CreateICW" );
    return hDC;
}


static HDC ImplCreateSalPrnIC( WinSalInfoPrinter* pPrinter, ImplJobSetup* pSetupData )
{
    HDC hDC = 0;
    if( aSalShlData.mbWPrinter )
    {
        LPDEVMODEW pDevMode;
        if ( pSetupData && pSetupData->mpDriverData )
            pDevMode = SAL_DEVMODE_W( pSetupData );
        else
            pDevMode = NULL;
        // #95347 some buggy drivers (eg, OKI) write to those buffers in CreateIC, although declared const - so provide some space
        // pl: does this hold true for Unicode functions ?
        if( pPrinter->maDriverName.Len() > 2048 || pPrinter->maDeviceName.Len() > 2048 )
            return 0;
        sal_Unicode pDriverName[ 4096 ];
        sal_Unicode pDeviceName[ 4096 ];
        rtl_copyMemory( pDriverName, pPrinter->maDriverName.GetBuffer(), pPrinter->maDriverName.Len()*sizeof(sal_Unicode));
        memset( pDriverName+pPrinter->maDriverName.Len(), 0, 32 );
        rtl_copyMemory( pDeviceName, pPrinter->maDeviceName.GetBuffer(), pPrinter->maDeviceName.Len()*sizeof(sal_Unicode));
        memset( pDeviceName+pPrinter->maDeviceName.Len(), 0, 32 );
        hDC = ImplCreateICW_WithCatch( reinterpret_cast< LPWSTR >(pDriverName),
                                       reinterpret_cast< LPCWSTR >(pDeviceName),
                                       pDevMode );
    }
    else
    {
        LPDEVMODEA pDevMode;
        if ( pSetupData && pSetupData->mpDriverData )
            pDevMode = SAL_DEVMODE_A( pSetupData );
        else
            pDevMode = NULL;
        // #95347 some buggy drivers (eg, OKI) write to those buffers in CreateIC, although declared const - so provide some space
        ByteString aDriver ( ImplSalGetWinAnsiString( pPrinter->maDriverName, TRUE ) );
        ByteString aDevice ( ImplSalGetWinAnsiString( pPrinter->maDeviceName, TRUE ) );
        int n = aDriver.Len() > aDevice.Len() ? aDriver.Len() : aDevice.Len();
            // #125813# under some circumstances many printer drivers really
        // seem to have a problem with the names and their conversions.
        // We need to get on to of this, but haven't been able to reproduce
        // the problem yet. Put the names on the stack so we get them
        // with an eventual crash report.
        if( n >= 2048 )
            return 0;
        n += 2048;
        char lpszDriverName[ 4096 ];
        char lpszDeviceName[ 4096 ];
        strncpy( lpszDriverName, aDriver.GetBuffer(), n );
        strncpy( lpszDeviceName, aDevice.GetBuffer(), n );
        // HDU: the crashes usually happen in a MBCS to unicode conversion,
        // so I suspect the MBCS string's end is not properly recognized.
        // The longest MBCS encoding I'm aware of has six bytes per code
        // => add a couple of zeroes...
        memset( lpszDriverName+aDriver.Len(), 0, 16 );
        memset( lpszDeviceName+aDevice.Len(), 0, 16 );
        hDC = ImplCreateICA_WithCatch( lpszDriverName,
                                       lpszDeviceName,
                                       pDevMode );
    }
    return hDC;
}

// -----------------------------------------------------------------------

static WinSalGraphics* ImplCreateSalPrnGraphics( HDC hDC )
{
	WinSalGraphics* pGraphics = new WinSalGraphics;
    pGraphics->SetLayout( 0 );
	pGraphics->mhDC		= hDC;
	pGraphics->mhWnd 	= 0;
	pGraphics->mbPrinter = TRUE;
	pGraphics->mbVirDev	= FALSE;
	pGraphics->mbWindow	= FALSE;
	pGraphics->mbScreen	= FALSE;
	ImplSalInitGraphics( pGraphics );
	return pGraphics;
}

// -----------------------------------------------------------------------

static BOOL ImplUpdateSalPrnIC( WinSalInfoPrinter* pPrinter, ImplJobSetup* pSetupData )
{
	HDC hNewDC = ImplCreateSalPrnIC( pPrinter, pSetupData );
	if ( !hNewDC )
		return FALSE;

	if ( pPrinter->mpGraphics )
	{
		ImplSalDeInitGraphics( pPrinter->mpGraphics );
		DeleteDC( pPrinter->mpGraphics->mhDC );
		delete pPrinter->mpGraphics;
	}

	pPrinter->mpGraphics = ImplCreateSalPrnGraphics( hNewDC );
	pPrinter->mhDC		= hNewDC;

	return TRUE;
}

// =======================================================================

SalInfoPrinter* WinSalInstance::CreateInfoPrinter( SalPrinterQueueInfo* pQueueInfo,
                                                   ImplJobSetup* pSetupData )
{
	WinSalInfoPrinter* pPrinter = new WinSalInfoPrinter;
    if( ! pQueueInfo->mpSysData )
        GetPrinterQueueState( pQueueInfo );
	pPrinter->maDriverName	= pQueueInfo->maDriver;
	pPrinter->maDeviceName	= pQueueInfo->maPrinterName;
	pPrinter->maPortName	= pQueueInfo->mpSysData ? 
                                *(String*)(pQueueInfo->mpSysData)
                              : String();

    // check if the provided setup data match the actual printer
	ImplTestSalJobSetup( pPrinter, pSetupData, TRUE );

	HDC hDC = ImplCreateSalPrnIC( pPrinter, pSetupData );
	if ( !hDC )
	{
		delete pPrinter;
		return NULL;
	}

    pPrinter->mpGraphics = ImplCreateSalPrnGraphics( hDC );
	pPrinter->mhDC		= hDC;
	if ( !pSetupData->mpDriverData )
		ImplUpdateSalJobSetup( pPrinter, pSetupData, FALSE, NULL );
	ImplDevModeToJobSetup( pPrinter, pSetupData, SAL_JOBSET_ALL );
	pSetupData->mnSystem = JOBSETUP_SYSTEM_WINDOWS;

	return pPrinter;
}

// -----------------------------------------------------------------------

void WinSalInstance::DestroyInfoPrinter( SalInfoPrinter* pPrinter )
{
	delete pPrinter;
}

// =======================================================================

WinSalInfoPrinter::WinSalInfoPrinter() :
    mpGraphics( NULL ),
    mhDC( 0 ),
    mbGraphics( FALSE )
{
    m_bPapersInit = FALSE;
}

// -----------------------------------------------------------------------

WinSalInfoPrinter::~WinSalInfoPrinter()
{
	if ( mpGraphics )
	{
		ImplSalDeInitGraphics( mpGraphics );
		DeleteDC( mpGraphics->mhDC );
		delete mpGraphics;
	}
}

// -----------------------------------------------------------------------

void WinSalInfoPrinter::InitPaperFormats( const ImplJobSetup* pSetupData )
{
    m_aPaperFormats.clear();

    DWORD nCount = ImplDeviceCaps( this, DC_PAPERSIZE, NULL, pSetupData );
    if( nCount == GDI_ERROR )
        nCount = 0;

    POINT* pPaperSizes = NULL;
    if( nCount )
	{
		pPaperSizes = (POINT*)rtl_allocateZeroMemory(nCount*sizeof(POINT));
		ImplDeviceCaps( this, DC_PAPERSIZE, (BYTE*)pPaperSizes, pSetupData );

        if( aSalShlData.mbWPrinter )
        {
            sal_Unicode* pNamesBuffer = (sal_Unicode*)rtl_allocateMemory(nCount*64*sizeof(sal_Unicode));
            ImplDeviceCaps( this, DC_PAPERNAMES, (BYTE*)pNamesBuffer, pSetupData );
            for( DWORD i = 0; i < nCount; ++i )
            {
                PaperInfo aInfo(pPaperSizes[i].x * 10, pPaperSizes[i].y * 10);
                m_aPaperFormats.push_back( aInfo );
            }
            rtl_freeMemory( pNamesBuffer );
        }
        else
        {
            char* pNamesBuffer = (char*)rtl_allocateMemory(nCount*64);
            ImplDeviceCaps( this, DC_PAPERNAMES, (BYTE*)pNamesBuffer, pSetupData );
            for( DWORD i = 0; i < nCount; ++i )
            {
                PaperInfo aInfo(pPaperSizes[i].x * 10, pPaperSizes[i].y * 10);
                m_aPaperFormats.push_back( aInfo );
            }
            rtl_freeMemory( pNamesBuffer );
        }
        rtl_freeMemory( pPaperSizes );
	}

    m_bPapersInit = true;
}

// -----------------------------------------------------------------------

DuplexMode WinSalInfoPrinter::GetDuplexMode( const ImplJobSetup* pSetupData )
{
    DuplexMode nRet = DUPLEX_UNKNOWN;
	if ( pSetupData &&pSetupData->mpDriverData )
    {
        if( aSalShlData.mbWPrinter )
        {
            DEVMODEW* pDevMode = SAL_DEVMODE_W( pSetupData );
            if ( pDevMode && (pDevMode->dmFields & DM_DUPLEX ))
            {
                if ( pDevMode->dmDuplex == DMDUP_SIMPLEX )
                    nRet = DUPLEX_OFF;
                else
                    nRet = DUPLEX_ON;
            }
        }
        else
        {
            DEVMODEA* pDevMode = SAL_DEVMODE_A( pSetupData );
            if ( pDevMode && (pDevMode->dmFields & DM_DUPLEX ))
            {
                if ( pDevMode->dmDuplex == DMDUP_SIMPLEX )
                    nRet = DUPLEX_OFF;
                else
                    nRet = DUPLEX_ON;
            }
        }
    }
    return nRet;
}

// -----------------------------------------------------------------------

int WinSalInfoPrinter::GetLandscapeAngle( const ImplJobSetup* pSetupData )
{
    int nRet = ImplDeviceCaps( this, DC_ORIENTATION, NULL, pSetupData );

    if( nRet != GDI_ERROR )
        return nRet * 10;
    else
        return 900; // guess
}

// -----------------------------------------------------------------------

SalGraphics* WinSalInfoPrinter::GetGraphics()
{
	if ( mbGraphics )
		return NULL;

	if ( mpGraphics )
		mbGraphics = TRUE;

	return mpGraphics;
}

// -----------------------------------------------------------------------

void WinSalInfoPrinter::ReleaseGraphics( SalGraphics* )
{
	mbGraphics = FALSE;
}

// -----------------------------------------------------------------------

BOOL WinSalInfoPrinter::Setup( SalFrame* pFrame, ImplJobSetup* pSetupData )
{
	if ( ImplUpdateSalJobSetup( this, pSetupData, TRUE, static_cast<WinSalFrame*>(pFrame) ) )
	{
		ImplDevModeToJobSetup( this, pSetupData, SAL_JOBSET_ALL );
		return ImplUpdateSalPrnIC( this, pSetupData );
	}

	return FALSE;
}

// -----------------------------------------------------------------------

BOOL WinSalInfoPrinter::SetPrinterData( ImplJobSetup* pSetupData )
{
	if ( !ImplTestSalJobSetup( this, pSetupData, FALSE ) )
		return FALSE;
	return ImplUpdateSalPrnIC( this, pSetupData );
}

// -----------------------------------------------------------------------

BOOL WinSalInfoPrinter::SetData( ULONG nFlags, ImplJobSetup* pSetupData )
{
	ImplJobSetupToDevMode( this, pSetupData, nFlags );
	if ( ImplUpdateSalJobSetup( this, pSetupData, TRUE, NULL ) )
	{
		ImplDevModeToJobSetup( this, pSetupData, nFlags );
		return ImplUpdateSalPrnIC( this, pSetupData );
	}

	return FALSE;
}

// -----------------------------------------------------------------------

ULONG WinSalInfoPrinter::GetPaperBinCount( const ImplJobSetup* pSetupData )
{
	DWORD nRet = ImplDeviceCaps( this, DC_BINS, NULL, pSetupData );
	if ( nRet && (nRet != GDI_ERROR) )
		return nRet;
	else
		return 0;
}

// -----------------------------------------------------------------------

XubString WinSalInfoPrinter::GetPaperBinName( const ImplJobSetup* pSetupData, ULONG nPaperBin )
{
	XubString aPaperBinName;

	DWORD nBins = ImplDeviceCaps( this, DC_BINNAMES, NULL, pSetupData );
	if ( (nPaperBin < nBins) && (nBins != GDI_ERROR) )
	{
        if( aSalShlData.mbWPrinter )
        {
            sal_Unicode* pBuffer = new sal_Unicode[nBins*24];
            DWORD nRet = ImplDeviceCaps( this, DC_BINNAMES, (BYTE*)pBuffer, pSetupData );
            if ( nRet && (nRet != GDI_ERROR) )
                aPaperBinName = pBuffer + (nPaperBin*24);
            delete [] pBuffer;
        }
        else
        {
            char* pBuffer = new char[nBins*24];
            DWORD nRet = ImplDeviceCaps( this, DC_BINNAMES, (BYTE*)pBuffer, pSetupData );
            if ( nRet && (nRet != GDI_ERROR) )
                aPaperBinName = ImplSalGetUniString( (const char*)(pBuffer + (nPaperBin*24)) );
            delete [] pBuffer;
        }
	}

	return aPaperBinName;
}

// -----------------------------------------------------------------------

ULONG WinSalInfoPrinter::GetCapabilities( const ImplJobSetup* pSetupData, USHORT nType )
{
	DWORD nRet;

	switch ( nType )
	{
		case PRINTER_CAPABILITIES_SUPPORTDIALOG:
			return TRUE;
		case PRINTER_CAPABILITIES_COPIES:
			nRet = ImplDeviceCaps( this, DC_COPIES, NULL, pSetupData );
			if ( nRet && (nRet != GDI_ERROR) )
				return nRet;
			return 0;
		case PRINTER_CAPABILITIES_COLLATECOPIES:
			if ( aSalShlData.mbW40 )
			{
				nRet = ImplDeviceCaps( this, DC_COLLATE, NULL, pSetupData );
				if ( nRet && (nRet != GDI_ERROR) )
				{
					nRet = ImplDeviceCaps( this, DC_COPIES, NULL, pSetupData );
					if ( nRet && (nRet != GDI_ERROR) )
						 return nRet;
				}
			}
			return 0;

		case PRINTER_CAPABILITIES_SETORIENTATION:
			nRet = ImplDeviceCaps( this, DC_ORIENTATION, NULL, pSetupData );
			if ( nRet && (nRet != GDI_ERROR) )
				return TRUE;
			return FALSE;

		case PRINTER_CAPABILITIES_SETPAPERBIN:
			nRet = ImplDeviceCaps( this, DC_BINS, NULL, pSetupData );
			if ( nRet && (nRet != GDI_ERROR) )
				return TRUE;
			return FALSE;

		case PRINTER_CAPABILITIES_SETPAPERSIZE:
		case PRINTER_CAPABILITIES_SETPAPER:
			nRet = ImplDeviceCaps( this, DC_PAPERS, NULL, pSetupData );
			if ( nRet && (nRet != GDI_ERROR) )
				return TRUE;
			return FALSE;
	}

	return 0;
}

// -----------------------------------------------------------------------

void WinSalInfoPrinter::GetPageInfo( const ImplJobSetup*,
								  long& rOutWidth, long& rOutHeight,
								  long& rPageOffX, long& rPageOffY,
								  long& rPageWidth, long& rPageHeight )
{
	HDC hDC = mhDC;

	rOutWidth	= GetDeviceCaps( hDC, HORZRES );
	rOutHeight	= GetDeviceCaps( hDC, VERTRES );

	rPageOffX	= GetDeviceCaps( hDC, PHYSICALOFFSETX );
	rPageOffY	= GetDeviceCaps( hDC, PHYSICALOFFSETY );
	rPageWidth	= GetDeviceCaps( hDC, PHYSICALWIDTH );
	rPageHeight = GetDeviceCaps( hDC, PHYSICALHEIGHT );
}

// =======================================================================

SalPrinter* WinSalInstance::CreatePrinter( SalInfoPrinter* pInfoPrinter )
{
	WinSalPrinter* pPrinter = new WinSalPrinter;
	pPrinter->mpInfoPrinter = static_cast<WinSalInfoPrinter*>(pInfoPrinter);
	return pPrinter;
}

// -----------------------------------------------------------------------

void WinSalInstance::DestroyPrinter( SalPrinter* pPrinter )
{
	delete pPrinter;
}

// =======================================================================

WIN_BOOL CALLBACK SalPrintAbortProc( HDC hPrnDC, int /* nError */ )
{
	SalData*	pSalData = GetSalData();
	WinSalPrinter* pPrinter;
	BOOL		bWhile = TRUE;
	int 		i = 0;

	do
	{
		// Messages verarbeiten
		MSG aMsg;
		if ( ImplPeekMessage( &aMsg, 0, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &aMsg );
			ImplDispatchMessage( &aMsg );
			i++;
			if ( i > 15 )
				bWhile = FALSE;
		}
		else
			bWhile = FALSE;

		pPrinter = pSalData->mpFirstPrinter;
		while ( pPrinter )
		{
			if( pPrinter->mhDC == hPrnDC )
				break;

			pPrinter = pPrinter->mpNextPrinter;
		}

		if ( !pPrinter || pPrinter->mbAbort )
			return FALSE;
	}
	while ( bWhile );

	return TRUE;
}

// -----------------------------------------------------------------------

static LPDEVMODEA ImplSalSetCopies( LPDEVMODEA pDevMode, ULONG nCopies, BOOL bCollate )
{
	LPDEVMODEA pNewDevMode = pDevMode;
	if ( pDevMode && (nCopies > 1) )
	{
		if ( nCopies > 32765 )
			nCopies = 32765;
		ULONG nDevSize = pDevMode->dmSize+pDevMode->dmDriverExtra;
		pNewDevMode = (LPDEVMODEA)rtl_allocateMemory( nDevSize );
		memcpy( pNewDevMode, pDevMode, nDevSize );
		pDevMode = pNewDevMode;
		pDevMode->dmFields |= DM_COPIES;
		pDevMode->dmCopies	= (short)(USHORT)nCopies;
		if ( aSalShlData.mbW40 )
		{
			pDevMode->dmFields |= DM_COLLATE;
			if ( bCollate )
				pDevMode->dmCollate = DMCOLLATE_TRUE;
			else
				pDevMode->dmCollate = DMCOLLATE_FALSE;
		}
	}

	return pNewDevMode;
}

static LPDEVMODEW ImplSalSetCopies( LPDEVMODEW pDevMode, ULONG nCopies, BOOL bCollate )
{
	LPDEVMODEW pNewDevMode = pDevMode;
	if ( pDevMode && (nCopies > 1) )
	{
		if ( nCopies > 32765 )
			nCopies = 32765;
		ULONG nDevSize = pDevMode->dmSize+pDevMode->dmDriverExtra;
		pNewDevMode = (LPDEVMODEW)rtl_allocateMemory( nDevSize );
		memcpy( pNewDevMode, pDevMode, nDevSize );
		pDevMode = pNewDevMode;
		pDevMode->dmFields |= DM_COPIES;
		pDevMode->dmCopies	= (short)(USHORT)nCopies;
		if ( aSalShlData.mbW40 )
		{
			pDevMode->dmFields |= DM_COLLATE;
			if ( bCollate )
				pDevMode->dmCollate = DMCOLLATE_TRUE;
			else
				pDevMode->dmCollate = DMCOLLATE_FALSE;
		}
	}

	return pNewDevMode;
}

// -----------------------------------------------------------------------

WinSalPrinter::WinSalPrinter() :
    mpGraphics( NULL ),
    mpInfoPrinter( NULL ),
    mpNextPrinter( NULL ),
    mhDC( 0 ),
    mnError( 0 ),
    mnCopies( 0 ),
    mbCollate( FALSE ),
    mbAbort( FALSE ),
    mbValid( true )
{
	SalData* pSalData = GetSalData();
	// insert printer in printerlist
	mpNextPrinter = pSalData->mpFirstPrinter;
	pSalData->mpFirstPrinter = this;
}

// -----------------------------------------------------------------------

WinSalPrinter::~WinSalPrinter()
{
	SalData* pSalData = GetSalData();

    // release DC if there is one still around because of AbortJob
	HDC hDC = mhDC;
	if ( hDC )
	{
		if ( mpGraphics )
		{
			ImplSalDeInitGraphics( mpGraphics );
			delete mpGraphics;
		}

		DeleteDC( hDC );
	}

	// remove printer from printerlist
	if ( this == pSalData->mpFirstPrinter )
		pSalData->mpFirstPrinter = mpNextPrinter;
	else
	{
		WinSalPrinter* pTempPrinter = pSalData->mpFirstPrinter;

		while( pTempPrinter->mpNextPrinter != this )
			pTempPrinter = pTempPrinter->mpNextPrinter;

		pTempPrinter->mpNextPrinter = mpNextPrinter;
	}
    mbValid = false;
}

// -----------------------------------------------------------------------

void WinSalPrinter::markInvalid()
{
    mbValid = false;
}

// -----------------------------------------------------------------------

// need wrappers for StarTocW/A to use structured exception handling
// since SEH does not mix with standard exception handling's cleanup
static int lcl_StartDocW( HDC hDC, DOCINFOW* pInfo, WinSalPrinter* pPrt )
{
    int nRet = 0;
    CATCH_DRIVER_EX_BEGIN;
    nRet = ::StartDocW( hDC, pInfo );
    CATCH_DRIVER_EX_END( "exception in StartDocW", pPrt );
    return nRet;
}

static int lcl_StartDocA( HDC hDC, DOCINFOA* pInfo, WinSalPrinter* pPrt )
{
    int nRet = 0;
    CATCH_DRIVER_EX_BEGIN;
    nRet = ::StartDocA( hDC, pInfo );
    CATCH_DRIVER_EX_END( "exception in StartDocW", pPrt );
    return nRet;
}

BOOL WinSalPrinter::StartJob( const XubString* pFileName,
						   const XubString& rJobName,
						   const XubString&,
						   ULONG nCopies, BOOL bCollate,
						   ImplJobSetup* pSetupData )
{
	mnError		= 0;
	mbAbort		= FALSE;
	mnCopies		= nCopies;
	mbCollate 	= bCollate;

	LPDEVMODEA	pOrgDevModeA = NULL;
	LPDEVMODEA	pDevModeA = NULL;
	LPDEVMODEW	pOrgDevModeW = NULL;
	LPDEVMODEW	pDevModeW = NULL;
    HDC hDC = 0;
    if( aSalShlData.mbWPrinter )
    {
        if ( pSetupData && pSetupData->mpDriverData )
        {
            pOrgDevModeW = SAL_DEVMODE_W( pSetupData );
            pDevModeW = ImplSalSetCopies( pOrgDevModeW, nCopies, bCollate );
        }
        else
            pDevModeW = NULL;
    
        // #95347 some buggy drivers (eg, OKI) write to those buffers in CreateDC, although declared const - so provide some space
        sal_Unicode aDrvBuf[4096];
        sal_Unicode aDevBuf[4096];
        rtl_copyMemory( aDrvBuf, mpInfoPrinter->maDriverName.GetBuffer(), (mpInfoPrinter->maDriverName.Len()+1)*sizeof(sal_Unicode));
        rtl_copyMemory( aDevBuf, mpInfoPrinter->maDeviceName.GetBuffer(), (mpInfoPrinter->maDeviceName.Len()+1)*sizeof(sal_Unicode));
        hDC = CreateDCW( reinterpret_cast<LPCWSTR>(aDrvBuf),
                         reinterpret_cast<LPCWSTR>(aDevBuf),
                         NULL,
                         pDevModeW );
    
        if ( pDevModeW != pOrgDevModeW )
            rtl_freeMemory( pDevModeW );
    }
    else
    {
        if ( pSetupData && pSetupData->mpDriverData )
        {
            pOrgDevModeA = SAL_DEVMODE_A( pSetupData );
            pDevModeA = ImplSalSetCopies( pOrgDevModeA, nCopies, bCollate );
        }
        else
            pDevModeA = NULL;
    
        // #95347 some buggy drivers (eg, OKI) write to those buffers in CreateDC, although declared const - so provide some space
        ByteString aDriver ( ImplSalGetWinAnsiString( mpInfoPrinter->maDriverName, TRUE ) );
        ByteString aDevice ( ImplSalGetWinAnsiString( mpInfoPrinter->maDeviceName, TRUE ) );
        int n = aDriver.Len() > aDevice.Len() ? aDriver.Len() : aDevice.Len();
        n += 2048;
        char *lpszDriverName = new char[n];
        char *lpszDeviceName = new char[n];
        strncpy( lpszDriverName, aDriver.GetBuffer(), n );
        strncpy( lpszDeviceName, aDevice.GetBuffer(), n );
        hDC = CreateDCA( lpszDriverName,
                         lpszDeviceName,
                         NULL,
                         pDevModeA );
    
        delete [] lpszDriverName;
        delete [] lpszDeviceName;
    
        if ( pDevModeA != pOrgDevModeA )
            rtl_freeMemory( pDevModeA );
    }

	if ( !hDC )
	{
		mnError = SAL_PRINTER_ERROR_GENERALERROR;
		return FALSE;
	}

    // make sure mhDC is set before the printer driver may call our abortproc
    mhDC = hDC;
	if ( SetAbortProc( hDC, SalPrintAbortProc ) <= 0 )
	{
		mnError = SAL_PRINTER_ERROR_GENERALERROR;
		return FALSE;
	}

	mnError	= 0;
	mbAbort	= FALSE;

	// Wegen Telocom Balloon Fax-Treiber, der uns unsere Messages
	// ansonsten oefters schickt, versuchen wir vorher alle
	// zu verarbeiten und dann eine Dummy-Message reinstellen
	BOOL bWhile = TRUE;
	int  i = 0;
	do
	{
		// Messages verarbeiten
		MSG aMsg;
		if ( ImplPeekMessage( &aMsg, 0, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &aMsg );
			ImplDispatchMessage( &aMsg );
			i++;
			if ( i > 15 )
				bWhile = FALSE;
		}
		else
			bWhile = FALSE;
	}
	while ( bWhile );
	ImplPostMessage( GetSalData()->mpFirstInstance->mhComWnd, SAL_MSG_DUMMY, 0, 0 );

    // bring up a file choser if printing to file port but no file name given
    OUString aOutFileName;
    if( mpInfoPrinter->maPortName.EqualsIgnoreCaseAscii( "FILE:" ) && !(pFileName && pFileName->Len()) )
    {

        Reference< XMultiServiceFactory > xFactory( ::comphelper::getProcessServiceFactory() );
        if( xFactory.is() )
        {
            Reference< XFilePicker > xFilePicker( xFactory->createInstance(
                OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.ui.dialogs.FilePicker" ) ) ),
                UNO_QUERY );
            DBG_ASSERT( xFilePicker.is(), "could not get FilePicker service" );

            Reference< XInitialization > xInit( xFilePicker, UNO_QUERY );
            Reference< XFilterManager > xFilterMgr( xFilePicker, UNO_QUERY );
            if( xInit.is() && xFilePicker.is() && xFilterMgr.is() )
            {
                Sequence< Any > aServiceType( 1 );
                aServiceType[0] <<= TemplateDescription::FILESAVE_SIMPLE;
                xInit->initialize( aServiceType );
                if( xFilePicker->execute() == ExecutableDialogResults::OK )
                {
                    Sequence< OUString > aPathSeq( xFilePicker->getFiles() );
				    INetURLObject aObj( aPathSeq[0] );
                    // we're using ansi calls (StartDocA) so convert the string
                    aOutFileName = aObj.PathToFileName();
                }
                else
                {
                    mnError = SAL_PRINTER_ERROR_ABORT;
                    return FALSE;
                }
            }
        }
    }

    if( aSalShlData.mbWPrinter )
    {
        DOCINFOW aInfo;
        memset( &aInfo, 0, sizeof( DOCINFOW ) );
        aInfo.cbSize = sizeof( aInfo );
        aInfo.lpszDocName = (LPWSTR)rJobName.GetBuffer();
        if ( pFileName || aOutFileName.getLength() )
        {
            if ( (pFileName && pFileName->Len()) || aOutFileName.getLength() )
            {
                aInfo.lpszOutput = (LPWSTR)( (pFileName && pFileName->Len()) ? pFileName->GetBuffer() : aOutFileName.getStr());
            }
            else
                aInfo.lpszOutput = L"FILE:";
        }
        else
            aInfo.lpszOutput = NULL;
    
        // start Job
        int nRet = lcl_StartDocW( hDC, &aInfo, this );
        
        if ( nRet <= 0 )
        {
            long nError = GetLastError();
            if ( (nRet == SP_USERABORT) || (nRet == SP_APPABORT) || (nError == ERROR_PRINT_CANCELLED) || (nError == ERROR_CANCELLED) )
                mnError = SAL_PRINTER_ERROR_ABORT;
            else
                mnError = SAL_PRINTER_ERROR_GENERALERROR;
            return FALSE;            
        }
    }
    else
    {
        // Both strings must exist, if StartJob() is called
        ByteString aJobName( ImplSalGetWinAnsiString( rJobName, TRUE ) );
        ByteString aFileName;
    
        DOCINFOA aInfo;
        memset( &aInfo, 0, sizeof( DOCINFOA ) );
        aInfo.cbSize = sizeof( aInfo );
        aInfo.lpszDocName = (LPCSTR)aJobName.GetBuffer();
        if ( pFileName || aOutFileName.getLength() )
        {
            if ( pFileName->Len() || aOutFileName.getLength() )
            {
                aFileName = ImplSalGetWinAnsiString( pFileName ? *pFileName : static_cast<const XubString>(aOutFileName), TRUE );
                aInfo.lpszOutput = (LPCSTR)aFileName.GetBuffer();
            }
            else
                aInfo.lpszOutput = "FILE:";
        }
        else
            aInfo.lpszOutput = NULL;
    
        // start Job
        int nRet = lcl_StartDocA( hDC, &aInfo, this );
        if ( nRet <= 0 )
        {
            long nError = GetLastError();
            if ( (nRet == SP_USERABORT) || (nRet == SP_APPABORT) || (nError == ERROR_PRINT_CANCELLED) || (nError == ERROR_CANCELLED) )
                mnError = SAL_PRINTER_ERROR_ABORT;
            else
                mnError = SAL_PRINTER_ERROR_GENERALERROR;
            return FALSE;
        }
    }

	return TRUE;
}

// -----------------------------------------------------------------------

BOOL WinSalPrinter::EndJob()
{
	DWORD err = 0;
	HDC hDC = mhDC;
	if ( isValid() && hDC )
	{
		if ( mpGraphics )
		{
			ImplSalDeInitGraphics( mpGraphics );
			delete mpGraphics;
			mpGraphics = NULL;
		}

        // #i54419# Windows fax printer brings up a dialog in EndDoc
        // which text previously copied in soffice process can be
        // pasted to -> deadlock due to mutex not released.
        // it should be safe to release the yield mutex over the EndDoc
        // call, however the real solution is supposed to be the threading
        // framework yet to come.
        SalData* pSalData = GetSalData();
		ULONG nAcquire = pSalData->mpFirstInstance->ReleaseYieldMutex();
        CATCH_DRIVER_EX_BEGIN;
        if( ::EndDoc( hDC ) <= 0 )
            err = GetLastError();
        CATCH_DRIVER_EX_END( "exception in EndDoc", this );
        
		pSalData->mpFirstInstance->AcquireYieldMutex( nAcquire );
		DeleteDC( hDC );
        mhDC = 0;
	}

	return TRUE;
}

// -----------------------------------------------------------------------

BOOL WinSalPrinter::AbortJob()
{
	mbAbort = TRUE;

	// Abort asyncron ausloesen
	HDC hDC = mhDC;
	if ( hDC )
	{
		SalData* pSalData = GetSalData();
		ImplPostMessage( pSalData->mpFirstInstance->mhComWnd,
						 SAL_MSG_PRINTABORTJOB, (WPARAM)hDC, 0 );
	}

	return TRUE;
}

// -----------------------------------------------------------------------

void ImplSalPrinterAbortJobAsync( HDC hPrnDC )
{
	SalData*	pSalData = GetSalData();
	WinSalPrinter* pPrinter = pSalData->mpFirstPrinter;

	// Feststellen, ob Printer noch existiert
	while ( pPrinter )
	{
		if ( pPrinter->mhDC == hPrnDC )
			break;

		pPrinter = pPrinter->mpNextPrinter;
	}

	// Wenn Printer noch existiert, dann den Job abbrechen
	if ( pPrinter )
	{
		HDC hDC = pPrinter->mhDC;
		if ( hDC )
		{
			if ( pPrinter->mpGraphics )
			{
				ImplSalDeInitGraphics( pPrinter->mpGraphics );
				delete pPrinter->mpGraphics;
				pPrinter->mpGraphics = NULL;
			}

            CATCH_DRIVER_EX_BEGIN;
            ::AbortDoc( hDC );
            CATCH_DRIVER_EX_END( "exception in AbortDoc", pPrinter );

			DeleteDC( hDC );
            pPrinter->mhDC = 0;
		}
	}
}

// -----------------------------------------------------------------------

SalGraphics* WinSalPrinter::StartPage( ImplJobSetup* pSetupData, BOOL bNewJobData )
{
    if( ! isValid() || mhDC == 0 )
        return NULL;
    
	HDC hDC = mhDC;
	if ( pSetupData && pSetupData->mpDriverData && bNewJobData )
	{
        if( aSalShlData.mbWPrinter )
        {
            LPDEVMODEW	pOrgDevModeW;
            LPDEVMODEW	pDevModeW;
            pOrgDevModeW = SAL_DEVMODE_W( pSetupData );
            pDevModeW = ImplSalSetCopies( pOrgDevModeW, mnCopies, mbCollate );
            ResetDCW( hDC, pDevModeW );
            if ( pDevModeW != pOrgDevModeW )
                rtl_freeMemory( pDevModeW );
        }
        else
        {
            LPDEVMODEA	pOrgDevModeA;
            LPDEVMODEA	pDevModeA;
            pOrgDevModeA = SAL_DEVMODE_A( pSetupData );
            pDevModeA = ImplSalSetCopies( pOrgDevModeA, mnCopies, mbCollate );
            ResetDCA( hDC, pDevModeA );
            if ( pDevModeA != pOrgDevModeA )
                rtl_freeMemory( pDevModeA );
        }
	}
	int nRet = 0;
    CATCH_DRIVER_EX_BEGIN;
    nRet = ::StartPage( hDC );
    CATCH_DRIVER_EX_END( "exception in StartPage", this );
    
	if ( nRet <= 0 )
	{
		GetLastError();
		mnError = SAL_PRINTER_ERROR_GENERALERROR;
		return NULL;
	}

	// Hack to work around old PostScript printer drivers optimizing away empty pages
    // TODO: move into ImplCreateSalPrnGraphics()?
	HPEN	hTempPen = SelectPen( hDC, GetStockPen( NULL_PEN ) );
	HBRUSH	hTempBrush = SelectBrush( hDC, GetStockBrush( NULL_BRUSH ) );
	WIN_Rectangle( hDC, -8000, -8000, -7999, -7999 );
	SelectPen( hDC, hTempPen );
	SelectBrush( hDC, hTempBrush );

    mpGraphics = ImplCreateSalPrnGraphics( hDC );
    return mpGraphics;
}

// -----------------------------------------------------------------------

BOOL WinSalPrinter::EndPage()
{
	HDC hDC = mhDC;
	if ( hDC && mpGraphics )
	{
		ImplSalDeInitGraphics( mpGraphics );
		delete mpGraphics;
		mpGraphics = NULL;
	}

    if( ! isValid() )
        return FALSE;
    
	int nRet = 0;
    CATCH_DRIVER_EX_BEGIN;
    nRet = ::EndPage( hDC );
    CATCH_DRIVER_EX_END( "exception in EndPage", this );

	if ( nRet > 0 )
		return TRUE;
	else
	{
		GetLastError();
		mnError = SAL_PRINTER_ERROR_GENERALERROR;
		return FALSE;
	}
}

// -----------------------------------------------------------------------

ULONG WinSalPrinter::GetErrorCode()
{
	return mnError;
}
