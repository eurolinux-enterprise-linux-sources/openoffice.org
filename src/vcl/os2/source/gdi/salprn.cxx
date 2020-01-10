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

// use this define to disable the DJP support
// #define NO_DJP

#define INCL_DOSMODULEMGR
#define INCL_DEV
#define INCL_SPL
#define INCL_SPLERRORS
#define INCL_SPLDOSPRINT
#define INCL_DEVDJP

#define INCL_GPI
#define INCL_DOSSEMAPHORES
#define INCL_PM
#include <svpm.h>
#include <pmdjp.h>

#include <string.h>

#define _SV_SALPRN_CXX
#include <tools/debug.hxx>
#include <saldata.hxx>
#include <salinst.h>
#include <salgdi.h>
#include <salframe.h>
#include <vcl/salptype.hxx>
#include <salprn.h>
#include <vcl/print.h>
#include <vcl/jobset.h>

#ifndef __H_FT2LIB
#include <wingdi.h>
#include <ft2lib.h>
#endif

// =======================================================================

// -----------------------
// - struct ImplFormInfo -
// -----------------------

struct ImplFormInfo
{
	long					mnPaperWidth;
	long					mnPaperHeight;
#ifndef NO_DJP
	DJPT_PAPERSIZE			mnId;
#endif
};

// =======================================================================

// -----------------------
// - struct ImplTrayInfo -
// -----------------------

struct ImplTrayInfo
{
	CHAR			maName[32];
	CHAR			maDisplayName[64];
	DJPT_TRAYTYPE	mnId;

	ImplTrayInfo( const char* pTrayName,
				  const char* pTrayDisplayName ) 
	{
		strcpy( maName, pTrayName);
		strcpy( maDisplayName, pTrayDisplayName);
	}
};

// =======================================================================

struct ImplQueueSalSysData
{
	ByteString		maPrinterName;			// pszPrinters
	ByteString		maName; 				// pszName bzw. LogAddress
	ByteString		maOrgDriverName;		// pszDriverName (maDriverName.maDeviceName)
	ByteString		maDriverName;			// pszDriverName bis .
	ByteString		maDeviceName;			// pszDriverName nach .
	PDRIVDATA		mpDrivData;

					ImplQueueSalSysData( const ByteString& rPrinterName,
										 const ByteString& rName,
										 const ByteString& rDriverName,
										 const ByteString& rDeviceName,
										 const ByteString& rOrgDriverName,
										 PDRIVDATA pDrivData  );
					~ImplQueueSalSysData();
};

// -----------------------------------------------------------------------

ImplQueueSalSysData::ImplQueueSalSysData( const ByteString& rPrinterName,
										  const ByteString& rName,
										  const ByteString& rOrgDriverName,
										  const ByteString& rDriverName,
										  const ByteString& rDeviceName,
										  PDRIVDATA pDrivData ) :
	maPrinterName( rPrinterName ),
	maName( rName ),
	maOrgDriverName( rName ),
	maDriverName( rDriverName ),
	maDeviceName( rDeviceName )
{
	if ( pDrivData )
	{
		mpDrivData = (PDRIVDATA)new BYTE[pDrivData->cb];
		memcpy( mpDrivData, pDrivData, pDrivData->cb );
	}
	else
		mpDrivData = NULL;
}

// -----------------------------------------------------------------------

ImplQueueSalSysData::~ImplQueueSalSysData()
{
	delete mpDrivData;
}

// =======================================================================

static ULONG ImplPMQueueStatusToSal( USHORT nPMStatus )
{
	ULONG nStatus = 0;
	if ( nPMStatus & PRQ3_PAUSED )
		nStatus |= QUEUE_STATUS_PAUSED;
	if ( nPMStatus & PRQ3_PENDING )
		nStatus |= QUEUE_STATUS_PENDING_DELETION;
	if ( !nStatus )
		nStatus |= QUEUE_STATUS_READY;
	return nStatus;
}

// -----------------------------------------------------------------------

void Os2SalInstance::GetPrinterQueueInfo( ImplPrnQueueList* pList )
{
	APIRET rc;
	ULONG  nNeeded;
	ULONG  nReturned;
	ULONG  nTotal;

	// query needed size of the buffer for the QueueInfo
	rc = SplEnumQueue( (PSZ)NULL, 3, NULL, 0, &nReturned, &nTotal, &nNeeded, NULL );
	if( nNeeded == 0 )
		return;

	// create the buffer for the QueueInfo
	PCHAR pQueueData = new CHAR[nNeeded];

	// query QueueInfos
	rc = SplEnumQueue( (PSZ)NULL, 3, pQueueData, nNeeded, &nReturned, &nTotal, &nNeeded, NULL );

	PPRQINFO3 pPrqInfo = (PPRQINFO3)pQueueData;
	for ( int i = 0; i < nReturned; i++ )
	{
		// create entry for the QueueInfo array
		SalPrinterQueueInfo* pInfo = new SalPrinterQueueInfo;

		ByteString aOrgDriverName( pPrqInfo->pszDriverName);
		ByteString aName( pPrqInfo->pszName);
		pInfo->maDriver 	 = ::rtl::OStringToOUString (aOrgDriverName, gsl_getSystemTextEncoding());
		pInfo->maPrinterName = ::rtl::OStringToOUString (pPrqInfo->pszComment, gsl_getSystemTextEncoding());
		pInfo->maLocation	 = ::rtl::OStringToOUString (aName, gsl_getSystemTextEncoding());
		pInfo->mnStatus 	 = ImplPMQueueStatusToSal( pPrqInfo->fsStatus );
		pInfo->mnJobs		 = pPrqInfo->cJobs;
		// pInfo->maComment = !!!

		// Feststellen, ob Name doppelt
		PPRQINFO3 pTempPrqInfo = (PPRQINFO3)pQueueData;
		for ( int j = 0; j < nReturned; j++ )
		{
			// Wenn Name doppelt, erweitern wir diesen um die Location
			if ( (j != i) &&
				 (strcmp( pPrqInfo->pszComment, pTempPrqInfo->pszComment ) == 0) )
			{
				pInfo->maPrinterName += ';';
				pInfo->maPrinterName += pInfo->maLocation;
			}
			pTempPrqInfo++;
		}

		// pszDriver in DriverName (bis .) und DeviceName (nach .) aufsplitten
		PSZ pDriverName;
		PSZ pDeviceName;
		if ( (pDriverName = strchr( pPrqInfo->pszDriverName, '.' )) != 0 )
		{
		   *pDriverName = 0;
		   pDeviceName	= pDriverName + 1;
		}
		else
			pDeviceName = NULL;

		// Alle Bytes hinter dem DeviceNamen auf 0 initialisieren, damit
		// ein memcmp vom JobSetup auch funktioniert
		if ( pPrqInfo->pDriverData &&
			 (pPrqInfo->pDriverData->cb >= sizeof( pPrqInfo->pDriverData )) )
		{
			int nDeviceNameLen = strlen( pPrqInfo->pDriverData->szDeviceName );
			memset( pPrqInfo->pDriverData->szDeviceName+nDeviceNameLen,
					0,
					sizeof( pPrqInfo->pDriverData->szDeviceName )-nDeviceNameLen );
		}

		// save driver data and driver names
		ByteString aPrinterName( pPrqInfo->pszPrinters);
		ByteString aDriverName( pPrqInfo->pszDriverName);
		ByteString aDeviceName;
		if ( pDeviceName )
			aDeviceName = pDeviceName;
		pInfo->mpSysData = new ImplQueueSalSysData( aPrinterName, aName,
													aOrgDriverName,
													aDriverName, aDeviceName,
													pPrqInfo->pDriverData );

		// add queue to the list
		pList->Add( pInfo );

		// increment to next element of the QueueInfo array
		pPrqInfo++;
	}

	delete [] pQueueData;
}

// -----------------------------------------------------------------------

void Os2SalInstance::GetPrinterQueueState( SalPrinterQueueInfo* pInfo )
{
	APIRET rc;
	ULONG  nNeeded;
	ULONG  nReturned;
	ULONG  nTotal;

	// query needed size of the buffer for the QueueInfo
	rc = SplEnumQueue( (PSZ)NULL, 3, NULL, 0, &nReturned, &nTotal, &nNeeded, NULL );
	if( nNeeded == 0 )
		return;

	// create the buffer for the QueueInfo
	PCHAR pQueueData = new CHAR[nNeeded];

	// query QueueInfos
	rc = SplEnumQueue( (PSZ)NULL, 3, pQueueData, nNeeded, &nReturned, &nTotal, &nNeeded, NULL );

	PPRQINFO3 pPrqInfo = (PPRQINFO3)pQueueData;
	for ( int i = 0; i < nReturned; i++ )
	{
		ImplQueueSalSysData* pSysData = (ImplQueueSalSysData*)(pInfo->mpSysData);
		if ( pSysData->maPrinterName.Equals( pPrqInfo->pszPrinters ) &&
			 pSysData->maName.Equals( pPrqInfo->pszName ) &&
			 pSysData->maOrgDriverName.Equals( pPrqInfo->pszDriverName ) )
		{
			pInfo->mnStatus = ImplPMQueueStatusToSal( pPrqInfo->fsStatus );
			pInfo->mnJobs	= pPrqInfo->cJobs;
			break;
		}

		// increment to next element of the QueueInfo array
		pPrqInfo++;
	}

	delete [] pQueueData;
}

// -----------------------------------------------------------------------

void Os2SalInstance::DeletePrinterQueueInfo( SalPrinterQueueInfo* pInfo )
{
	delete ((ImplQueueSalSysData*)(pInfo->mpSysData));
	delete pInfo;
}

// -----------------------------------------------------------------------

XubString Os2SalInstance::GetDefaultPrinter()
{
	APIRET		rc;
	ULONG		nNeeded;
	ULONG		nReturned;
	ULONG		nTotal;
	char		szQueueName[255];
	XubString	aDefaultName;

	// query default queue
	if ( !PrfQueryProfileString( HINI_PROFILE, SPL_INI_SPOOLER, "QUEUE", 0, szQueueName, sizeof( szQueueName ) ) )
		return aDefaultName;

	// extract first queue name
	PSZ pStr;
	if ( (pStr = strchr( szQueueName, ';' )) != 0 )
		*pStr = 0;

	// query needed size of the buffer for the QueueInfo
	rc = SplEnumQueue( (PSZ)NULL, 3, NULL, 0, &nReturned, &nTotal, &nNeeded, NULL );
	if ( nNeeded == 0 )
		return aDefaultName;

	// create the buffer for the QueueInfo
	PCHAR pQueueData = new CHAR[ nNeeded ];

	// query QueueInfos
	rc = SplEnumQueue ((PSZ)NULL, 3, pQueueData, nNeeded, &nReturned, &nTotal, &nNeeded, NULL );

	// find printer name for default queue
	PPRQINFO3 pPrqInfo = (PPRQINFO3) pQueueData;
	for ( int i = 0; i < nReturned; i++ )
	{
		if ( strcmp( pPrqInfo->pszName, szQueueName ) == 0 )
		{
			aDefaultName = ::rtl::OStringToOUString (pPrqInfo->pszComment, gsl_getSystemTextEncoding());

			// Feststellen, ob Name doppelt
			PPRQINFO3 pTempPrqInfo = (PPRQINFO3)pQueueData;
			for ( int j = 0; j < nReturned; j++ )
			{
				// Wenn Name doppelt, erweitern wir diesen um die Location
				if ( (j != i) &&
					 (strcmp( pPrqInfo->pszComment, pTempPrqInfo->pszComment ) == 0) )
				{
					String pszName( ::rtl::OStringToOUString (pPrqInfo->pszName, gsl_getSystemTextEncoding()));
					aDefaultName += ';';
					aDefaultName += pszName;
				}
				pTempPrqInfo++;
			}
			break;
		}

		// increment to next element of the QueueInfo array
		pPrqInfo++;
	}

	delete [] pQueueData;

	return aDefaultName;
}

// =======================================================================

static void* ImplAllocPrnMemory( size_t n )
{
	return calloc( n, 1);
}

// -----------------------------------------------------------------------

inline void ImplFreePrnMemory( void* p )
{
	free( p );
}

// -----------------------------------------------------------------------

static PDRIVDATA ImplPrnDrivData( const ImplJobSetup* pSetupData )
{
	// Diese Funktion wird eingesetzt, damit Druckertreiber nicht auf
	// unseren Daten arbeiten, da es durch Konfigurationsprobleme
	// sein kann, das der Druckertreiber bei uns Daten ueberschreibt.
	// Durch diese vorgehensweise werden einige Abstuerze vermieden, bzw.
	// sind dadurch leichter zu finden

	if ( !pSetupData->mpDriverData )
		return NULL;

	DBG_ASSERT( ((PDRIVDATA)(pSetupData->mpDriverData))->cb == pSetupData->mnDriverDataLen,
				"ImplPrnDrivData() - SetupDataLen != DriverDataLen" );

	PDRIVDATA pDrivData = (PDRIVDATA)ImplAllocPrnMemory( pSetupData->mnDriverDataLen );
	memcpy( pDrivData, pSetupData->mpDriverData, pSetupData->mnDriverDataLen );
	return pDrivData;
}

// -----------------------------------------------------------------------

static void ImplUpdateSetupData( const PDRIVDATA pDrivData, ImplJobSetup* pSetupData )
{
	// Diese Funktion wird eingesetzt, damit Druckertreiber nicht auf
	// unseren Daten arbeiten, da es durch Konfigurationsprobleme
	// sein kann, das der Druckertreiber bei uns Daten ueberschreibt.
	// Durch diese vorgehensweise werden einige Abstuerze vermieden, bzw.
	// sind dadurch leichter zu finden

	if ( !pDrivData || !pDrivData->cb )
	{
		if ( pSetupData->mpDriverData )
			rtl_freeMemory( pSetupData->mpDriverData );
		pSetupData->mpDriverData = NULL;
		pSetupData->mnDriverDataLen = 0;
	}
	else
	{
		// Alle Bytes hinter dem DeviceNamen auf 0 initialisieren, damit
		// ein memcmp vom JobSetup auch funktioniert
		if ( pDrivData->cb >= sizeof( pDrivData ) )
		{
			int nDeviceNameLen = strlen( pDrivData->szDeviceName );
			memset( pDrivData->szDeviceName+nDeviceNameLen,
					0,
					sizeof( pDrivData->szDeviceName )-nDeviceNameLen );
		}

		if ( pSetupData->mpDriverData )
		{
			if ( pSetupData->mnDriverDataLen != pDrivData->cb )
				rtl_freeMemory( pSetupData->mpDriverData );
			pSetupData->mpDriverData = (BYTE*)rtl_allocateMemory( pDrivData->cb);
		}
		else
			pSetupData->mpDriverData = (BYTE*)rtl_allocateMemory( pDrivData->cb);
		pSetupData->mnDriverDataLen = pDrivData->cb;
		memcpy( pSetupData->mpDriverData, pDrivData, pDrivData->cb );
	}

	if ( pDrivData )
		ImplFreePrnMemory( pDrivData );
}

// -----------------------------------------------------------------------

static BOOL ImplPaperSizeEqual( long nPaperWidth1, long nPaperHeight1,
								long nPaperWidth2, long nPaperHeight2 )
{
	return (((nPaperWidth1 >= nPaperWidth2-1) && (nPaperWidth1 <= nPaperWidth2+1)) &&
			((nPaperHeight1 >= nPaperHeight2-1) && (nPaperHeight1 <= nPaperHeight2+1)));
}

// -----------------------------------------------------------------------

static BOOL ImplIsDriverDJPEnabled( HDC hDC )
{
#ifdef NO_DJP
	return FALSE;
#else
	// Ueber OS2-Ini kann DJP disablte werden
	if ( !PrfQueryProfileInt( HINI_PROFILE, SAL_PROFILE_APPNAME, SAL_PROFILE_USEDJP, 1 ) )
		return FALSE;

	// Testen, ob DJP-Interface am Drucker vorhanden
	LONG   lQuery;
	APIRET rc;

	lQuery = DEVESC_QUERYSIZE;
	rc = DevEscape( hDC,
					DEVESC_QUERYESCSUPPORT,
					sizeof( lQuery ),
					(PBYTE)&lQuery,
					0,
					(PBYTE)NULL );
	if ( DEV_OK != rc )
		return FALSE;

	lQuery = DEVESC_QUERYJOBPROPERTIES;
	rc = DevEscape( hDC,
					DEVESC_QUERYESCSUPPORT,
					sizeof( lQuery ),
					(PBYTE)&lQuery,
					0,
					(PBYTE)NULL );
	if ( DEV_OK != rc )
		return FALSE;

	lQuery = DEVESC_SETJOBPROPERTIES;
	rc = DevEscape( hDC,
					DEVESC_QUERYESCSUPPORT,
					sizeof( lQuery ),
					(PBYTE)&lQuery,
					0,
					(PBYTE)NULL );
	if ( DEV_OK != rc )
		return FALSE;

	return TRUE;
#endif
}

// -----------------------------------------------------------------------

static void ImplFormatInputList( PDJP_ITEM pDJP, PQUERYTUPLE pTuple )
{
   // Loop through the query elements
   BOOL fContinue = TRUE;
   do
   {
	  pDJP->cb			  = sizeof (DJP_ITEM);
	  pDJP->ulProperty	  = pTuple->ulProperty;
	  pDJP->lType		  = pTuple->lType;
	  pDJP->ulNumReturned = 0;
	  pDJP->ulValue 	  = DJP_NONE;

	  // at EOL?
	  fContinue = DJP_NONE != pTuple->ulProperty;

	  // Move to next item structure and tuplet
	  pDJP++;
	  pTuple++;
   }
   while ( fContinue );
}

// -----------------------------------------------------------------------

static void ImplFreeFormAndTrayList( Os2SalInfoPrinter* pOs2SalInfoPrinter )
{
	if ( pOs2SalInfoPrinter->mnFormCount )
	{
		for ( USHORT i = 0; i < pOs2SalInfoPrinter->mnFormCount; i++ )
			delete pOs2SalInfoPrinter->mpFormArray[i];
		delete [] pOs2SalInfoPrinter->mpFormArray;
		pOs2SalInfoPrinter->mnFormCount = 0;
	}

	if ( pOs2SalInfoPrinter->mnTrayCount )
	{
		for ( USHORT i = 0; i < pOs2SalInfoPrinter->mnTrayCount; i++ )
			delete pOs2SalInfoPrinter->mpTrayArray[i];
		delete [] pOs2SalInfoPrinter->mpTrayArray;
		pOs2SalInfoPrinter->mnTrayCount = 0;
	}
}

// -----------------------------------------------------------------------

static void ImplGetFormAndTrayList( Os2SalInfoPrinter* pOs2SalInfoPrinter, const ImplJobSetup* pSetupData )
{
	ImplFreeFormAndTrayList( pOs2SalInfoPrinter );

	LONG alQuery[] =
	{
		0,					0,				// First two members of QUERYSIZE
		DJP_CJ_FORM,		DJP_ALL,
		DJP_CJ_TRAYNAME,	DJP_ALL,
		DJP_NONE,			DJP_NONE		// EOL marker
	};

	APIRET		rc;
	PQUERYSIZE	pQuerySize			= (PQUERYSIZE)alQuery;
	PBYTE		pBuffer 			= NULL;
	LONG		nAlloc				= 0;
	PDRIVDATA	pCopyDrivData		= ImplPrnDrivData( pSetupData );
	LONG		nDrivDataSize		= pCopyDrivData->cb;
	PBYTE		pDrivData			= (PBYTE)pCopyDrivData;

	// find out how many bytes to allocate
	pQuerySize->cb = sizeof( alQuery );
	rc = DevEscape( pOs2SalInfoPrinter->mhDC,
					DEVESC_QUERYSIZE,
					sizeof( alQuery ),
					(PBYTE)pQuerySize,
					&nDrivDataSize,
					pDrivData );
	if ( DEV_OK != rc )
	{
		ImplFreePrnMemory( pCopyDrivData );
		return;
	}

	// allocate the memory
	nAlloc = pQuerySize->ulSizeNeeded;
	pBuffer = (PBYTE)new BYTE[nAlloc];

	// set up the input
	PDJP_ITEM pDJP = (PDJP_ITEM)pBuffer;
	ImplFormatInputList( pDJP, pQuerySize->aTuples );

	// do it!
	rc = DevEscape( pOs2SalInfoPrinter->mhDC,
					DEVESC_QUERYJOBPROPERTIES,
					nAlloc,
					pBuffer,
					&nDrivDataSize,
					pDrivData );
	ImplFreePrnMemory( pCopyDrivData );

	if ( (DEV_OK == rc) || (DEV_WARNING == rc) )
	{
		// Loop through the query elements
		PQUERYTUPLE pTuple = pQuerySize->aTuples;
		while ( DJP_NONE != pTuple->ulProperty )
		{
			if ( pDJP->ulProperty == DJP_CJ_FORM )
			{
				if ( pDJP->ulNumReturned )
				{
					PDJPT_FORM pElm = DJP_ELEMENTP( *pDJP, DJPT_FORM );

					pOs2SalInfoPrinter->mnFormCount = pDJP->ulNumReturned;
					pOs2SalInfoPrinter->mpFormArray = new PIMPLFORMINFO[pOs2SalInfoPrinter->mnFormCount];
					for( int i = 0; i < pDJP->ulNumReturned; i++, pElm++ )
					{
						ImplFormInfo* pInfo 	= new ImplFormInfo;
						pInfo->mnPaperWidth 	= pElm->hcInfo.cx;
						pInfo->mnPaperHeight	= pElm->hcInfo.cy;
						pInfo->mnId 			= pElm->djppsFormID;
						pOs2SalInfoPrinter->mpFormArray[i] = pInfo;
					}
				}
			}
			else if ( pDJP->ulProperty == DJP_CJ_TRAYNAME )
			{
				if ( pDJP->ulNumReturned )
				{
					PDJPT_TRAYNAME pElm = DJP_ELEMENTP( *pDJP, DJPT_TRAYNAME );

					pOs2SalInfoPrinter->mnTrayCount = pDJP->ulNumReturned;
					pOs2SalInfoPrinter->mpTrayArray = new PIMPLTRAYINFO[pOs2SalInfoPrinter->mnTrayCount];
					for( int i = 0; i < pDJP->ulNumReturned; i++, pElm++ )
					{
						ImplTrayInfo* pInfo 	= new ImplTrayInfo( pElm->szTrayname, pElm->szDisplayTrayname );
						pInfo->mnId 			= pElm->djpttTrayID;
						pOs2SalInfoPrinter->mpTrayArray[i] = pInfo;
					}
				}
			}

			pDJP = DJP_NEXT_STRUCTP( pDJP );
			pTuple++;
		}
	}

	delete [] pBuffer;
}

// -----------------------------------------------------------------------

static BOOL ImplGetCurrentSettings( Os2SalInfoPrinter* pOs2SalInfoPrinter, ImplJobSetup* pSetupData )
{
	// Um den aktuellen Tray zu ermitteln, brauchen wir auch die Listen dazu
	if ( !pOs2SalInfoPrinter->mnFormCount )
		ImplGetFormAndTrayList( pOs2SalInfoPrinter, pSetupData );

	LONG alQuery[] =
	{
		0,						0,				// First two members of QUERYSIZE
		DJP_SJ_ORIENTATION, 	DJP_CURRENT,
		DJP_CJ_FORM,			DJP_CURRENT,
		DJP_NONE,				DJP_NONE		// EOL marker
	};

	APIRET		rc;
	PQUERYSIZE	pQuerySize			= (PQUERYSIZE)alQuery;
	PBYTE		pBuffer 			= NULL;
	LONG		nAlloc				= 0;
	PDRIVDATA	pCopyDrivData		= ImplPrnDrivData( pSetupData );
	LONG		nDrivDataSize		= pCopyDrivData->cb;
	PBYTE		pDrivData			= (PBYTE)pCopyDrivData;
	BOOL		bResult;

	// find out how many bytes to allocate
	pQuerySize->cb = sizeof( alQuery );
	rc = DevEscape( pOs2SalInfoPrinter->mhDC,
					DEVESC_QUERYSIZE,
					sizeof( alQuery ),
					(PBYTE)pQuerySize,
					&nDrivDataSize,
					pDrivData );
	if ( DEV_OK != rc )
	{
		ImplFreePrnMemory( pCopyDrivData );
		return FALSE;
	}

	// allocate the memory
	nAlloc = pQuerySize->ulSizeNeeded;
	pBuffer = (PBYTE)new BYTE[nAlloc];

	// set up the input
	PDJP_ITEM pDJP = (PDJP_ITEM)pBuffer;
	ImplFormatInputList( pDJP, pQuerySize->aTuples );

	rc = DevEscape( pOs2SalInfoPrinter->mhDC,
					DEVESC_QUERYJOBPROPERTIES,
					nAlloc,
					pBuffer,
					&nDrivDataSize,
					pDrivData );
	if ( (DEV_OK == rc) || (DEV_WARNING == rc) )
	{
		// aktuelle Setup-Daten uebernehmen
		ImplUpdateSetupData( pCopyDrivData, pSetupData );

		// Loop through the query elements
		PQUERYTUPLE pTuple = pQuerySize->aTuples;
		while ( DJP_NONE != pTuple->ulProperty )
		{
			if ( pDJP->ulProperty == DJP_SJ_ORIENTATION )
			{
				if ( pDJP->ulNumReturned )
				{
					PDJPT_ORIENTATION pElm = DJP_ELEMENTP( *pDJP, DJPT_ORIENTATION );
					if ( (DJP_ORI_PORTRAIT == *pElm) || (DJP_ORI_REV_PORTRAIT == *pElm) )
						pSetupData->meOrientation = ORIENTATION_PORTRAIT;
					else
						pSetupData->meOrientation = ORIENTATION_LANDSCAPE;
				}
			}
			else if ( pDJP->ulProperty == DJP_CJ_FORM )
			{
				if ( pDJP->ulNumReturned )
				{
					PDJPT_FORM pElm = DJP_ELEMENTP( *pDJP, DJPT_FORM );

					pSetupData->mnPaperWidth  = pElm->hcInfo.cx*100;
					pSetupData->mnPaperHeight = pElm->hcInfo.cy*100;
					switch( pElm->djppsFormID )
					{
						case DJP_PSI_A3:
							pSetupData->mePaperFormat = PAPER_A3;
							break;

						case DJP_PSI_A4:
							pSetupData->mePaperFormat = PAPER_A4;
							break;

						case DJP_PSI_A5:
							pSetupData->mePaperFormat = PAPER_A5;
							break;

						case DJP_PSI_B4:
							pSetupData->mePaperFormat = PAPER_B4;
							break;

						case DJP_PSI_B5:
							pSetupData->mePaperFormat = PAPER_B5;
							break;

						case DJP_PSI_LETTER:
							pSetupData->mePaperFormat = PAPER_LETTER;
							break;

						case DJP_PSI_LEGAL:
							pSetupData->mePaperFormat = PAPER_LEGAL;
							break;

						case DJP_PSI_TABLOID:
							pSetupData->mePaperFormat = PAPER_TABLOID;
							break;

						default:
							pSetupData->mePaperFormat = PAPER_USER;
							break;
					}

					// Wir suchen zuerst ueber den Namen/Id und dann ueber die Id
					BOOL	bTrayFound = FALSE;
					USHORT	j;
					for ( j = 0; j < pOs2SalInfoPrinter->mnTrayCount; j++ )
					{
						if ( (pOs2SalInfoPrinter->mpTrayArray[j]->mnId == pElm->djpttTrayID) &&
							 (pOs2SalInfoPrinter->mpTrayArray[j]->maName == pElm->szTrayname) )
						{
							pSetupData->mnPaperBin = j;
							bTrayFound = TRUE;
							break;
						}
					}
					if ( !bTrayFound )
					{
						for ( j = 0; j < pOs2SalInfoPrinter->mnTrayCount; j++ )
						{
							if ( pOs2SalInfoPrinter->mpTrayArray[j]->mnId == pElm->djpttTrayID )
							{
								pSetupData->mnPaperBin = j;
								bTrayFound = TRUE;
								break;
							}
						}
					}
					// Wenn wir Ihn immer noch nicht gefunden haben, setzen
					// wir ihn auf DontKnow
					if ( !bTrayFound )
						pSetupData->mnPaperBin = 0xFFFF;
				}
			}

			pDJP = DJP_NEXT_STRUCTP( pDJP );
			pTuple++;
		}

		bResult = TRUE;
	}
	else
	{
		ImplFreePrnMemory( pCopyDrivData );
		bResult = FALSE;
	}

	delete [] pBuffer;

	return bResult;
}

// -----------------------------------------------------------------------

static BOOL ImplSetOrientation( HDC hPrinterDC, PDRIVDATA pDriverData,
								Orientation eOrientation )
{
	LONG alQuery[] =
	{
		0,						0,				// First two members of QUERYSIZE
		DJP_SJ_ORIENTATION, 	DJP_CURRENT,
		DJP_NONE,				DJP_NONE		// EOL marker
	};

	APIRET		rc;
	PQUERYSIZE	pQuerySize		= (PQUERYSIZE)alQuery;
	PBYTE		pBuffer 		= NULL;
	LONG		nAlloc			= 0;
	LONG		nDrivDataSize	= pDriverData->cb;

	// find out how many bytes to allocate
	pQuerySize->cb = sizeof( alQuery );
	rc = DevEscape( hPrinterDC,
					DEVESC_QUERYSIZE,
					sizeof( alQuery ),
					(PBYTE)pQuerySize,
					&nDrivDataSize,
					(PBYTE)pDriverData );
	if ( DEV_OK != rc )
		return FALSE;

	// allocate the memory
	nAlloc = pQuerySize->ulSizeNeeded;
	pBuffer = (PBYTE)new BYTE[nAlloc];

	// set up the input
	PDJP_ITEM pDJP = (PDJP_ITEM)pBuffer;
	ImplFormatInputList( pDJP, pQuerySize->aTuples );

	pDJP->cb		 = sizeof( DJP_ITEM );
	pDJP->ulProperty = DJP_SJ_ORIENTATION;
	pDJP->lType 	 = DJP_CURRENT;
	pDJP->ulValue	 = (eOrientation == ORIENTATION_PORTRAIT)
						   ? DJP_ORI_PORTRAIT
						   : DJP_ORI_LANDSCAPE;

	// do it!
	rc = DevEscape( hPrinterDC,
					DEVESC_SETJOBPROPERTIES,
					nAlloc,
					pBuffer,
					&nDrivDataSize,
					(PBYTE)pDriverData );

	delete [] pBuffer;

	return ((DEV_OK == rc) || (DEV_WARNING == rc));
}

// -----------------------------------------------------------------------

static BOOL ImplSetPaperSize( HDC hPrinterDC, PDRIVDATA pDriverData,
							  DJPT_PAPERSIZE nOS2PaperFormat )
{
	LONG alQuery[] =
	{
		0,						0,				// First two members of QUERYSIZE
		DJP_SJ_PAPERSIZE,		DJP_CURRENT,
		DJP_NONE,				DJP_NONE		// EOL marker
	};

	APIRET		rc;
	PQUERYSIZE	pQuerySize		= (PQUERYSIZE)alQuery;
	PBYTE		pBuffer 		= NULL;
	LONG		nAlloc			= 0;
	LONG		nDrivDataSize	= pDriverData->cb;

	// find out how many bytes to allocate
	pQuerySize->cb = sizeof( alQuery );
	rc = DevEscape( hPrinterDC,
					DEVESC_QUERYSIZE,
					sizeof( alQuery ),
					(PBYTE)pQuerySize,
					&nDrivDataSize,
					(PBYTE)pDriverData );
	if ( DEV_OK != rc )
		return FALSE;

	// allocate the memory
	nAlloc = pQuerySize->ulSizeNeeded;
	pBuffer = (PBYTE)new BYTE[nAlloc];

	// set up the input
	PDJP_ITEM pDJP = (PDJP_ITEM)pBuffer;
	PDJP_ITEM pStartDJP = pDJP;
	ImplFormatInputList( pDJP, pQuerySize->aTuples );

	// Neue Daten zuweisen
	pDJP->cb		 = sizeof( DJP_ITEM );
	pDJP->ulProperty = DJP_SJ_PAPERSIZE;
	pDJP->lType 	 = DJP_CURRENT;
	pDJP->ulValue	 = nOS2PaperFormat;

	// und setzen
	rc = DevEscape( hPrinterDC,
					DEVESC_SETJOBPROPERTIES,
					nAlloc,
					pBuffer,
					&nDrivDataSize,
					(PBYTE)pDriverData );

	delete [] pBuffer;

	return ((DEV_OK == rc) || (DEV_WARNING == rc));
}

// -----------------------------------------------------------------------

static BOOL ImplSetPaperBin( HDC hPrinterDC, PDRIVDATA pDriverData,
							 ImplTrayInfo* pTrayInfo )
{
	LONG alQuery[] =
	{
		0,						0,				// First two members of QUERYSIZE
		DJP_SJ_TRAYTYPE,		DJP_CURRENT,
		DJP_NONE,				DJP_NONE		// EOL marker
	};

	APIRET		rc;
	PQUERYSIZE	pQuerySize		= (PQUERYSIZE)alQuery;
	PBYTE		pBuffer 		= NULL;
	LONG		nAlloc			= 0;
	LONG		nDrivDataSize	= pDriverData->cb;

	// find out how many bytes to allocate
	pQuerySize->cb = sizeof( alQuery );
	rc = DevEscape( hPrinterDC,
					DEVESC_QUERYSIZE,
					sizeof( alQuery ),
					(PBYTE)pQuerySize,
					&nDrivDataSize,
					(PBYTE)pDriverData );
	if ( DEV_OK != rc )
		return FALSE;

	// allocate the memory
	nAlloc = pQuerySize->ulSizeNeeded;
	pBuffer = (PBYTE)new BYTE[nAlloc];

	// set up the input
	PDJP_ITEM pDJP = (PDJP_ITEM)pBuffer;
	ImplFormatInputList( pDJP, pQuerySize->aTuples );

	// Neue Daten zuweisen
	pDJP->cb		 = sizeof( DJP_ITEM );
	pDJP->ulProperty = DJP_SJ_TRAYTYPE;
	pDJP->lType 	 = DJP_CURRENT;
	pDJP->ulValue	 = pTrayInfo->mnId;

	// und setzen
	rc = DevEscape( hPrinterDC,
					DEVESC_SETJOBPROPERTIES,
					nAlloc,
					pBuffer,
					&nDrivDataSize,
					(PBYTE)pDriverData );

	delete [] pBuffer;

	return ((DEV_OK == rc) || (DEV_WARNING == rc));
}

// =======================================================================

static BOOL ImplSalCreateInfoPrn( Os2SalInfoPrinter* pPrinter, PDRIVDATA pDriverData,
								  HDC& rDC, HPS& rPS )
{
	SalData* pSalData = GetSalData();

	// create info context
	DEVOPENSTRUC  devOpenStruc;
	memset( &devOpenStruc, 0, sizeof( devOpenStruc ) );
	devOpenStruc.pszLogAddress		= (char*)pPrinter->maName.GetBuffer();
	devOpenStruc.pszDriverName		= (char*)pPrinter->maDriverName.GetBuffer();
	devOpenStruc.pdriv				= pDriverData;
	devOpenStruc.pszDataType		= "PM_Q_STD";

	HDC hDC = DevOpenDC( pSalData->mhAB, OD_INFO, "*",
						 4, (PDEVOPENDATA)&devOpenStruc, (HDC)NULL);
	if ( !hDC )
		return FALSE;

	// create presentation space
	SIZEL sizel;
	sizel.cx = 0;
	sizel.cy = 0;
	HPS hPS = Ft2CreatePS( pSalData->mhAB, hDC, &sizel, GPIA_ASSOC | GPIT_MICRO | PU_PELS );
	if ( !hPS )
	{
		DevCloseDC( hDC );
		return FALSE;
	}

	rDC = hDC;
	rPS = hPS;
	return TRUE;
}

// -----------------------------------------------------------------------

static void ImplSalDestroyInfoPrn( Os2SalInfoPrinter* pPrinter )
{
	ImplSalDeInitGraphics( pPrinter->mpGraphics);
	Ft2Associate( pPrinter->mhPS, 0 );
	Ft2DestroyPS( pPrinter->mhPS );
	DevCloseDC( pPrinter->mhDC );
}

// =======================================================================

SalInfoPrinter* Os2SalInstance::CreateInfoPrinter( SalPrinterQueueInfo* pQueueInfo,
												ImplJobSetup* pSetupData )
{
	ImplQueueSalSysData* pSysQueueData = (ImplQueueSalSysData*)(pQueueInfo->mpSysData);
	Os2SalInfoPrinter* pPrinter = new Os2SalInfoPrinter;
	pPrinter->maPrinterName	= pSysQueueData->maPrinterName;
	pPrinter->maName			= pSysQueueData->maName;
	pPrinter->maDriverName	= pSysQueueData->maDriverName;
	pPrinter->maDeviceName	= pSysQueueData->maDeviceName;

	// Nur Setup-Daten uebernehmen, wenn Treiber und Laenge der Treiberdaten
	// uebereinstimmt
	PDRIVDATA	pDriverData;
	BOOL		bUpdateDriverData;
	if ( pSetupData->mpDriverData && pSysQueueData->mpDrivData &&
		 (pSetupData->mnSystem == JOBSETUP_SYSTEM_OS2) &&
		 (pSetupData->mnDriverDataLen == pSysQueueData->mpDrivData->cb) &&
		 (strcmp( ((PDRIVDATA)pSetupData->mpDriverData)->szDeviceName,
				  pSysQueueData->mpDrivData->szDeviceName ) == 0) )
	{
		pDriverData = PDRIVDATA( pSetupData->mpDriverData );
		bUpdateDriverData = FALSE;
	}
	else
	{
		pDriverData = pSysQueueData->mpDrivData;
		bUpdateDriverData = TRUE;
	}
	if ( pDriverData )
		pPrinter->maJobSetupDeviceName = pDriverData->szDeviceName;

	if ( !ImplSalCreateInfoPrn( pPrinter, pDriverData,
								pPrinter->mhDC,
								pPrinter->mhPS ) )
	{
		delete pPrinter;
		return NULL;
	}

	// create graphics object for output
	Os2SalGraphics* pGraphics = new Os2SalGraphics;
	pGraphics->mhDC				= pPrinter->mhDC;
	pGraphics->mhPS				= pPrinter->mhPS;
	pGraphics->mhWnd 			= 0;
	pGraphics->mbPrinter 		= TRUE;
	pGraphics->mbVirDev			= FALSE;
	pGraphics->mbWindow			= FALSE;
	pGraphics->mbScreen			= FALSE;

	ImplSalInitGraphics( pGraphics );
	pPrinter->mpGraphics			= pGraphics;

	// check printer driver for DJP support
	pPrinter->mbDJPSupported = ImplIsDriverDJPEnabled( pPrinter->mhDC );

	if ( bUpdateDriverData )
	{
		if ( pSetupData->mpDriverData )
			rtl_freeMemory( pSetupData->mpDriverData);
		pSetupData->mpDriverData = (BYTE*)rtl_allocateMemory( pDriverData->cb);
		memcpy( pSetupData->mpDriverData, pDriverData, pDriverData->cb );
		pSetupData->mnDriverDataLen = pDriverData->cb;
	}

	// retrieve current settings from printer driver and store them to system independend data!
	if ( pPrinter->mbDJPSupported )
		ImplGetCurrentSettings( pPrinter, pSetupData );
	pSetupData->mnSystem = JOBSETUP_SYSTEM_OS2;

	return pPrinter;
}

// -----------------------------------------------------------------------

void Os2SalInstance::DestroyInfoPrinter( SalInfoPrinter* pPrinter )
{
	delete pPrinter;
}

// =======================================================================

Os2SalInfoPrinter::Os2SalInfoPrinter()
{
	mhDC					= 0;
	mhPS					= 0;
	mpGraphics			= NULL;
	mbGraphics			= FALSE;
	mbDJPSupported		= FALSE;
	mnFormCount			= 0;
	mpFormArray			= NULL;
	mnTrayCount			= 0;
	mpTrayArray			= NULL;
}

// -----------------------------------------------------------------------

Os2SalInfoPrinter::~Os2SalInfoPrinter()
{
	if ( mpGraphics )
	{
		ImplSalDestroyInfoPrn( this );
		delete mpGraphics;
	}

	ImplFreeFormAndTrayList( this );
}

// -----------------------------------------------------------------------

SalGraphics* Os2SalInfoPrinter::GetGraphics()
{
	if ( mbGraphics )
		return NULL;

	if ( mpGraphics )
		mbGraphics = TRUE;

	return mpGraphics;
}

// -----------------------------------------------------------------------

void Os2SalInfoPrinter::ReleaseGraphics( SalGraphics* )
{
	mbGraphics = FALSE;
}

// -----------------------------------------------------------------------

BOOL Os2SalInfoPrinter::Setup( SalFrame* pFrame, ImplJobSetup* pSetupData )
{
	PDRIVDATA pDrivData = ImplPrnDrivData( pSetupData );
	if ( !pDrivData )
		return FALSE;

	APIRET rc = DevPostDeviceModes( GetSalData()->mhAB, pDrivData,
									maDriverName.GetBuffer(),
									maDeviceName.GetBuffer(),
									maPrinterName.GetBuffer(),
									DPDM_POSTJOBPROP );
	if ( rc == DEV_OK )
	{
		ImplUpdateSetupData( pDrivData, pSetupData );

		// update DC and PS
		HDC hDC;
		HPS hPS;
		if ( !ImplSalCreateInfoPrn( this, (PDRIVDATA)(pSetupData->mpDriverData), hDC, hPS ) )
			return FALSE;

		// Alten Printer DC/PS zerstoeren
		ImplSalDestroyInfoPrn( this );

		// Neue Daten setzen und initialisieren
		mhDC = hDC;
		mhPS = hPS;
		mpGraphics->mhDC = mhDC;
		mpGraphics->mhPS = mhPS;
		ImplSalInitGraphics( mpGraphics );

		// retrieve current settings from printer driver and store them to system independend data!
		ImplFreeFormAndTrayList( this );
		if ( mbDJPSupported )
			ImplGetCurrentSettings( this, pSetupData );

		return TRUE;
	}
	else
	{
		ImplFreePrnMemory( pDrivData );
		return FALSE;
	}
}

// -----------------------------------------------------------------------

BOOL Os2SalInfoPrinter::SetPrinterData( ImplJobSetup* pSetupData )
{
	// Wir koennen nur Treiberdaten von OS2 setzen
	if ( pSetupData->mnSystem != JOBSETUP_SYSTEM_OS2 )
		return FALSE;

	PDRIVDATA pNewDrivData = (PDRIVDATA)(pSetupData->mpDriverData);
	if ( !pNewDrivData )
		return FALSE;

	// Testen, ob Printerdaten fuer den gleichen Printer uebergeben werden,
	// da einige Treiber zu Abstuerzen neigen, wenn Daten von einem anderen
	// Printer gesetzt werden
	if ( !maJobSetupDeviceName.Equals( pNewDrivData->szDeviceName ))
		return FALSE;

	// update DC and PS
	HDC hDC;
	HPS hPS;
	if ( !ImplSalCreateInfoPrn( this, pNewDrivData, hDC, hPS ) )
		return FALSE;

	// Alten Printer DC/PS zerstoeren
	ImplSalDestroyInfoPrn( this );

	// Neue Daten setzen und initialisieren
	mhDC = hDC;
	mhPS = hPS;
	mpGraphics->mhDC = mhDC;
	mpGraphics->mhPS = mhPS;
	ImplSalInitGraphics( mpGraphics );

	// retrieve current settings from printer driver and store them to system independend data!
	ImplFreeFormAndTrayList( this );
	if ( mbDJPSupported )
		ImplGetCurrentSettings( this, pSetupData );

	return TRUE;
}

// -----------------------------------------------------------------------

BOOL Os2SalInfoPrinter::SetData( ULONG nFlags, ImplJobSetup* pSetupData )
{
	// needs DJP support
	if ( !mbDJPSupported )
		return FALSE;

	PDRIVDATA pDrivData = ImplPrnDrivData( pSetupData );

	if ( !pDrivData )
		return FALSE;

	BOOL bOK = FALSE;

	// set orientation
	if ( nFlags & SAL_JOBSET_ORIENTATION )
	{
		if ( ImplSetOrientation( mhDC, pDrivData, pSetupData->meOrientation ) )
			bOK = TRUE;
	}

	// set paper size
	if ( nFlags & SAL_JOBSET_PAPERSIZE )
	{
		// Papierformat ermitteln
		DJPT_PAPERSIZE nOS2PaperFormat;
		switch ( pSetupData->mePaperFormat )
		{
			case PAPER_A3:
				nOS2PaperFormat = DJP_PSI_A3;
				break;

			case PAPER_A4:
				nOS2PaperFormat = DJP_PSI_A4;
				break;

			case PAPER_A5:
				nOS2PaperFormat = DJP_PSI_A5;
				break;

			case PAPER_B4:
				nOS2PaperFormat = DJP_PSI_B4;
				break;

			case PAPER_B5:
				nOS2PaperFormat = DJP_PSI_B5;
				break;

			case PAPER_LETTER:
				nOS2PaperFormat = DJP_PSI_LETTER;
				break;

			case PAPER_LEGAL:
				nOS2PaperFormat = DJP_PSI_LEGAL;
				break;

			case PAPER_TABLOID:
				nOS2PaperFormat = DJP_PSI_TABLOID;
				break;

			default:
				{
				nOS2PaperFormat = DJP_PSI_NONE;
				// OS2 rechnet in Millimetern
				long nPaperWidth	 = pSetupData->mnPaperWidth  / 100;
				long nPaperHeight	 = pSetupData->mnPaperHeight / 100;
				// Ansonsten ueber die Papiergroesse suchen
				for( int i = 0; i < mnFormCount; i++ )
				{
					ImplFormInfo* pFormInfo = mpFormArray[i];
					if ( ImplPaperSizeEqual( nPaperWidth, nPaperHeight,
											 pFormInfo->mnPaperWidth, pFormInfo->mnPaperHeight ) )
					{
						nOS2PaperFormat = pFormInfo->mnId;
						break;
					}
				}
				}
				break;
		}

		if ( nOS2PaperFormat != DJP_PSI_NONE )
		{
			if ( ImplSetPaperSize( mhDC, pDrivData, nOS2PaperFormat ) )
				bOK = TRUE;
		}
	}

	// set paper tray
	if ( (nFlags & SAL_JOBSET_PAPERBIN) && (pSetupData->mnPaperBin < mnTrayCount) )
	{
		if ( ImplSetPaperBin( mhDC, pDrivData,
							  mpTrayArray[pSetupData->mnPaperBin] ) )
			bOK = TRUE;
	}

	if ( bOK )
	{
		ImplUpdateSetupData( pDrivData, pSetupData );

		// query current driver settings
		ImplFreeFormAndTrayList( this );
		if ( ImplGetCurrentSettings( this, pSetupData ) )
		{
			// update DC and PS
			HDC hDC;
			HPS hPS;
			if ( ImplSalCreateInfoPrn( this, (PDRIVDATA)(pSetupData->mpDriverData), hDC, hPS ) )
			{
				// Alten Printer DC/PS zerstoeren
				ImplSalDestroyInfoPrn( this );

				// Neue Daten setzen und initialisieren
				mhDC = hDC;
				mhPS = hPS;
				mpGraphics->mhDC = mhDC;
				mpGraphics->mhPS = mhPS;
				ImplSalInitGraphics( mpGraphics );
			}
			else
				bOK = FALSE;
		}
		else
			bOK = FALSE;
	}

	return bOK;
}

// -----------------------------------------------------------------------

ULONG Os2SalInfoPrinter::GetPaperBinCount( const ImplJobSetup* pJobSetup )
{
	if ( !mbDJPSupported )
		return 1;

	// init paperbinlist if empty
	if ( !mnTrayCount )
		ImplGetFormAndTrayList( this, pJobSetup );

	// Wir haben immer einen PaperTray und wenn, das eben einen ohne
	// Namen
	if ( !mnTrayCount )
		return 1;
	else
		return mnTrayCount;
}

// -----------------------------------------------------------------------

XubString Os2SalInfoPrinter::GetPaperBinName( const ImplJobSetup* pJobSetup,
										  ULONG nPaperBin )
{
	XubString aPaperBinName;

	if ( mbDJPSupported )
	{
		// init paperbinlist if empty
		if ( !mnTrayCount )
			ImplGetFormAndTrayList( this, pJobSetup );

		if ( nPaperBin < mnTrayCount )
			aPaperBinName = ::rtl::OStringToOUString (mpTrayArray[nPaperBin]->maDisplayName, gsl_getSystemTextEncoding());
	}

	return aPaperBinName;
}

// -----------------------------------------------------------------------

ULONG Os2SalInfoPrinter::GetCapabilities( const ImplJobSetup*, USHORT nType )
{
	switch ( nType )
	{
		case PRINTER_CAPABILITIES_SUPPORTDIALOG:
			return TRUE;
		case PRINTER_CAPABILITIES_COPIES:
			return 0xFFFF;
		case PRINTER_CAPABILITIES_COLLATECOPIES:
			return 0;
		case PRINTER_CAPABILITIES_SETORIENTATION:
		case PRINTER_CAPABILITIES_SETPAPERBIN:
		case PRINTER_CAPABILITIES_SETPAPERSIZE:
		case PRINTER_CAPABILITIES_SETPAPER:
			return mbDJPSupported;
	}

	return 0;
}

// -----------------------------------------------------------------------

void Os2SalInfoPrinter::GetPageInfo( const ImplJobSetup*,
								  long& rOutWidth, long& rOutHeight,
								  long& rPageOffX, long& rPageOffY,
								  long& rPageWidth, long& rPageHeight )
{
	HDC hDC = mhDC;

	// search current form
	HCINFO	aInfo;
	int nForms = DevQueryHardcopyCaps( hDC, 0, 0, &aInfo );
	for( int i = 0; i < nForms; i++ )
	{
		if ( DevQueryHardcopyCaps( hDC, i, 1, &aInfo ) >= 0 )
		{
			if ( aInfo.flAttributes & HCAPS_CURRENT )
			{
				// query resolution
				long nXResolution;
				long nYResolution;
				DevQueryCaps( hDC, CAPS_HORIZONTAL_RESOLUTION, 1, &nXResolution );
				DevQueryCaps( hDC, CAPS_VERTICAL_RESOLUTION, 1, &nYResolution );
				rPageOffX	= aInfo.xLeftClip * nXResolution / 1000;
				rPageOffY	= (aInfo.cy-aInfo.yTopClip) * nYResolution / 1000;
				rPageWidth	= aInfo.cx * nXResolution / 1000;
				rPageHeight = aInfo.cy * nYResolution / 1000;
				rOutWidth	= aInfo.xPels;
				rOutHeight	= aInfo.yPels;
				return;
			}
		}
	}

	// use device caps if no form selected/found
	long lCapsWidth = 0;
	long lCapsHeight = 0;
	DevQueryCaps( hDC, CAPS_WIDTH, 1L, &lCapsWidth );
	DevQueryCaps( hDC, CAPS_HEIGHT, 1L, &lCapsHeight );
	rPageOffX	 = 0;
	rPageOffY	 = 0;
	rOutWidth	 = lCapsWidth;
	rOutHeight	 = lCapsHeight;
	rPageWidth	 = rOutWidth;
	rPageHeight  = rOutHeight;
}

// =======================================================================

static BOOL ImplIsDriverPrintDJPEnabled( HDC hDC )
{
#ifdef NO_DJP
	return FALSE;
#else
	// Ueber OS2-Ini kann DJP disablte werden
	if ( !PrfQueryProfileInt( HINI_PROFILE, SAL_PROFILE_APPNAME, SAL_PROFILE_PRINTDJP, 1 ) )
		return FALSE;

	// Testen, ob DJP-Interface am Drucker vorhanden
	LONG   lQuery;
	APIRET rc;

	lQuery = DEVESC_QUERYSIZE;
	rc = DevEscape( hDC,
					DEVESC_QUERYESCSUPPORT,
					sizeof( lQuery ),
					(PBYTE)&lQuery,
					0,
					(PBYTE)NULL );
	if ( DEV_OK != rc )
		return FALSE;

	return TRUE;
#endif
}

// =======================================================================

SalPrinter* Os2SalInstance::CreatePrinter( SalInfoPrinter* pInfoPrinter )
{
	Os2SalPrinter* pPrinter = new Os2SalPrinter;
	pPrinter->mpInfoPrinter = static_cast<Os2SalInfoPrinter*>(pInfoPrinter);
	return pPrinter;
}

// -----------------------------------------------------------------------

void Os2SalInstance::DestroyPrinter( SalPrinter* pPrinter )
{
	delete pPrinter;
}

// =======================================================================

Os2SalPrinter::Os2SalPrinter()
{
	mhDC					= 0;
	mhPS					= 0;
	mpGraphics			= NULL;
	mbAbort				= FALSE;
	mbPrintDJPSupported	= FALSE;
}

// -----------------------------------------------------------------------

Os2SalPrinter::~Os2SalPrinter()
{
}

// -----------------------------------------------------------------------

BOOL Os2SalPrinter::StartJob( const XubString* pFileName,
						   const XubString& rJobName,
						   const XubString& rAppName,
						   ULONG nCopies, BOOL bCollate,
						   ImplJobSetup* pSetupData )
{
	DEVOPENSTRUC	aDevOpenStruc;
	LONG			lType;
	APIRET			rc;

	// prepare queue information
	memset( &aDevOpenStruc, 0, sizeof( aDevOpenStruc ) );
	aDevOpenStruc.pszDriverName = (PSZ)(mpInfoPrinter->maDriverName.GetBuffer());

	// print into file?
	if ( pFileName )
	{
		aDevOpenStruc.pszLogAddress = (PSZ)pFileName->GetBuffer();
		aDevOpenStruc.pszDataType = "PM_Q_RAW";
		lType = OD_DIRECT;
	}
	else
	{
		aDevOpenStruc.pszLogAddress = (PSZ)(mpInfoPrinter->maName.GetBuffer());
		if ( PrfQueryProfileInt( HINI_PROFILE, SAL_PROFILE_APPNAME, SAL_PROFILE_PRINTRAW, 0 ) )
			aDevOpenStruc.pszDataType = "PM_Q_RAW";
		else
			aDevOpenStruc.pszDataType = "PM_Q_STD";
		lType = OD_QUEUED;
	}

#if 0 // YD FIXME
	// Set comment (AppName nur bis zum 1. Space-Zeichen nehmen)
	const xub_Unicode*	pComment = rAppName;
	USHORT			nCommentLen = 0;
	memset( maCommentBuf, 0, sizeof( maCommentBuf ) );
	while ( (nCommentLen < 32) &&
			(((*pComment >= 'a') && (*pComment <= 'z')) ||
			 ((*pComment >= 'A') && (*pComment <= 'Z')) ||
			 ((*pComment >= '0') && (*pComment <= '9')) ||
			 (*pComment == '-')))
	{
		maCommentBuf[nCommentLen] = (char)(*pComment);
		nCommentLen++;
		pComment++;
	}
	aDevOpenStruc.pszComment = (PSZ)maCommentBuf;
#endif
	ByteString jobName( rJobName, gsl_getSystemTextEncoding());
	aDevOpenStruc.pszComment = (PSZ)jobName.GetBuffer();

	// Kopien
	if ( nCopies > 1 )
	{
		// OS2 kann maximal 999 Kopien
		if ( nCopies > 999 )
			nCopies = 999;
		sprintf( maCopyBuf, "COP=%d", nCopies);
		aDevOpenStruc.pszQueueProcParams = (PSZ)maCopyBuf;
	}

	// open device context
	SalData*	pSalData = GetSalData();
	HAB 		hAB = pSalData->mhAB;
	aDevOpenStruc.pdriv = (PDRIVDATA)pSetupData->mpDriverData;
	mhDC = DevOpenDC( hAB,
									lType,
									"*",
									7,
									(PDEVOPENDATA)&aDevOpenStruc,
									0 );
	if ( mhDC == 0 )
	{
		ERRORID nLastError = WinGetLastError( hAB );
		if ( (nLastError & 0xFFFF) == PMERR_SPL_PRINT_ABORT )
			mnError = SAL_PRINTER_ERROR_ABORT;
		else
			mnError = SAL_PRINTER_ERROR_GENERALERROR;
		return FALSE;
	}

	// open presentation space
	SIZEL sizel;
	sizel.cx = 0;
	sizel.cy = 0;
	mhPS = Ft2CreatePS( hAB, mhDC, &sizel, GPIA_ASSOC | GPIT_MICRO | PU_PELS );
	if ( !mhPS )
	{
		DevCloseDC( mhDC );
		mnError = SAL_PRINTER_ERROR_GENERALERROR;
		return NULL;
	}

	// Can we print with DJP
	mbPrintDJPSupported = ImplIsDriverPrintDJPEnabled( mhDC );

	// JobName ermitteln und Job starten
	PSZ pszJobName = NULL;
	int nJobNameLen = 0;
	if ( jobName.Len() > 0 )
	{
		pszJobName = (PSZ)jobName.GetBuffer();
		nJobNameLen = jobName.Len();
	}
	rc = DevEscape( mhDC,
					DEVESC_STARTDOC,
					nJobNameLen, (PBYTE)pszJobName,
					0, (PBYTE)NULL );

	if ( rc != DEV_OK )
	{
		ERRORID nLastError = WinGetLastError( hAB );
		if ( (nLastError & 0xFFFF) == PMERR_SPL_PRINT_ABORT )
			mnError = SAL_PRINTER_ERROR_ABORT;
		else
			mnError = SAL_PRINTER_ERROR_GENERALERROR;
		Ft2Associate( mhPS, NULL );
		Ft2DestroyPS( mhPS );
		DevCloseDC( mhDC );
		return FALSE;
	}

	// init for first page
	mbFirstPage = TRUE;
	mnError = 0;

	return TRUE;
}

// -----------------------------------------------------------------------

BOOL Os2SalPrinter::EndJob()
{
	APIRET rc;
	rc = DevEscape( mhDC,
					DEVESC_ENDDOC,
					0, NULL,
					0, NULL);

	// destroy presentation space and device context
	Ft2Associate( mhPS, NULL );
	Ft2DestroyPS( mhPS );
	DevCloseDC( mhDC );
	return TRUE;
}

// -----------------------------------------------------------------------

BOOL Os2SalPrinter::AbortJob()
{
	APIRET rc;

	rc = DevEscape( mhDC,
					DEVESC_ABORTDOC,
					0, NULL,
					0, NULL );

	// destroy SalGraphics
	if ( mpGraphics )
	{
		ImplSalDeInitGraphics( mpGraphics );
		delete mpGraphics;
		mpGraphics = NULL;
	}

	// destroy presentation space and device context
	Ft2Associate( mhPS, NULL );
	Ft2DestroyPS( mhPS );
	DevCloseDC( mhDC );
	return TRUE;
}

// -----------------------------------------------------------------------

SalGraphics* Os2SalPrinter::StartPage( ImplJobSetup* pSetupData, BOOL bNewJobSetup )
{
	APIRET rc;

	if ( mbFirstPage )
		mbFirstPage = FALSE;
	else
	{
		PBYTE	pJobData;
		LONG	nJobDataSize;
		LONG	nEscape;
		if ( mbPrintDJPSupported && bNewJobSetup )
		{
			nEscape 		= DEVESC_NEWFRAME_WPROP;
			nJobDataSize	= ((PDRIVDATA)(pSetupData->mpDriverData))->cb;
			pJobData		= (PBYTE)(pSetupData->mpDriverData);
		}
		else
		{
			nEscape 		= DEVESC_NEWFRAME;
			nJobDataSize	= 0;
			pJobData		= NULL;
		}
		rc = DevEscape( mhDC,
						nEscape,
						0, NULL,
						&nJobDataSize, pJobData );

		if ( rc != DEV_OK )
		{
			DevEscape( mhDC, DEVESC_ENDDOC, 0, NULL, 0, NULL);
			Ft2Associate( mhPS, NULL );
			Ft2DestroyPS( mhPS );
			DevCloseDC( mhDC );
			mnError = SAL_PRINTER_ERROR_GENERALERROR;
			return NULL;
		}
	}

	// create SalGraphics with copy of hPS
	Os2SalGraphics* pGraphics = new Os2SalGraphics;
	pGraphics->mhDC				= mhDC;
	pGraphics->mhPS				= mhPS;
	pGraphics->mhWnd 			= 0;
	pGraphics->mbPrinter 		= TRUE;
	pGraphics->mbVirDev			= FALSE;
	pGraphics->mbWindow			= FALSE;
	pGraphics->mbScreen			= FALSE;
	pGraphics->mnHeight			= 0;
	// search current form for actual page height
	HCINFO	aInfo;
	int 	nForms = DevQueryHardcopyCaps( mhDC, 0, 0, &aInfo );
	for( int i = 0; i < nForms; i++ )
	{
		if ( DevQueryHardcopyCaps( mhDC, i, 1, &aInfo ) >= 0 )
		{
			if ( aInfo.flAttributes & HCAPS_CURRENT )
				pGraphics->mnHeight	= aInfo.yPels;
		}
	}
	// use device caps if no form selected/found
	if ( !pGraphics->mnHeight )
		DevQueryCaps( mhDC, CAPS_HEIGHT, 1L, &pGraphics->mnHeight );

	ImplSalInitGraphics( pGraphics );
	mpGraphics = pGraphics;

	return pGraphics;
}

// -----------------------------------------------------------------------

BOOL Os2SalPrinter::EndPage()
{
	if ( mpGraphics )
	{
		// destroy SalGraphics
		ImplSalDeInitGraphics( mpGraphics );
		delete mpGraphics;
		mpGraphics = NULL;
	}

	return TRUE;
}

// -----------------------------------------------------------------------

ULONG Os2SalPrinter::GetErrorCode()
{
	return mnError;
}

void Os2SalInfoPrinter::InitPaperFormats( const ImplJobSetup* pSetupData )
{
	printf("Os2SalInfoPrinter::InitPaperFormats\n");
}
int Os2SalInfoPrinter::GetLandscapeAngle( const ImplJobSetup* pSetupData )
{
	printf("Os2SalInfoPrinter::GetLandscapeAngle\n");
	return 0;
}
DuplexMode Os2SalInfoPrinter::GetDuplexMode( const ImplJobSetup* pSetupData )
{
    DuplexMode nRet = DUPLEX_UNKNOWN;
	printf("Os2SalInfoPrinter::GetDuplexMode\n");
    return nRet;
}
