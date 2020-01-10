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

#include <string.h>
#include <limits.h>

#define INCL_PM
#define INCL_DOS
#define INCL_DOSERRORS
#include <svpm.h>

#include <tools/debug.hxx>
#include <tools/fsys.hxx>
#include <tools/stream.hxx>

// class FileBase
#include <osl/file.hxx>

using namespace osl;

// class FileBase
#ifndef _OSL_FILE_HXX_
#include <osl/file.hxx>
#endif

using namespace osl;

// -----------------------------------------------------------------------

// --------------
// - StreamData -
// --------------

class StreamData
{
public:
    HFILE   hFile;
    BOOL    bIsEof;

            StreamData()
            {
                hFile = 0;
                bIsEof = TRUE;
            }
};

// -----------------------------------------------------------------------

ULONG GetSvError( APIRET nPMError )
{
    static struct { APIRET pm; ULONG sv; } errArr[] =
    {
        { ERROR_FILE_NOT_FOUND,         SVSTREAM_FILE_NOT_FOUND },
        { ERROR_PATH_NOT_FOUND,         SVSTREAM_PATH_NOT_FOUND },
        { ERROR_TOO_MANY_OPEN_FILES,    SVSTREAM_TOO_MANY_OPEN_FILES },
        { ERROR_ACCESS_DENIED,          SVSTREAM_ACCESS_DENIED },
        { ERROR_INVALID_ACCESS,         SVSTREAM_INVALID_ACCESS },
        { ERROR_SHARING_VIOLATION,      SVSTREAM_SHARING_VIOLATION },
        { ERROR_SHARING_BUFFER_EXCEEDED,SVSTREAM_SHARE_BUFF_EXCEEDED },
        { ERROR_CANNOT_MAKE,            SVSTREAM_CANNOT_MAKE },
        { ERROR_INVALID_PARAMETER,      SVSTREAM_INVALID_PARAMETER },
        { ERROR_DRIVE_LOCKED,           SVSTREAM_LOCKING_VIOLATION },
        { ERROR_LOCK_VIOLATION,      SVSTREAM_LOCKING_VIOLATION },
        { ERROR_FILENAME_EXCED_RANGE,   SVSTREAM_INVALID_PARAMETER },
        { ERROR_ATOMIC_LOCK_NOT_SUPPORTED, SVSTREAM_INVALID_PARAMETER },
        { ERROR_READ_LOCKS_NOT_SUPPORTED, SVSTREAM_INVALID_PARAMETER },


        { 0xFFFF, SVSTREAM_GENERALERROR }
    };

    ULONG nRetVal = SVSTREAM_GENERALERROR;    // Standardfehler
    int i=0;
    do
    {
        if( errArr[i].pm == nPMError )
        {
            nRetVal = errArr[i].sv;
            break;
        }
        i++;
    }
    while( errArr[i].pm != 0xFFFF );
    return nRetVal;
}

/*************************************************************************
|*
|*    SvFileStream::SvFileStream()
|*
|*    Beschreibung      STREAM.SDW
|*    Ersterstellung    OV 15.06.94
|*    Letzte Aenderung  OV 15.06.94
|*
*************************************************************************/

SvFileStream::SvFileStream( const String& rFileName, StreamMode nOpenMode )
{
    bIsOpen             = FALSE;
    nLockCounter        = 0;
    bIsWritable         = FALSE;
    pInstanceData       = new StreamData;

    SetBufferSize( 8192 );
	// convert URL to SystemPath, if necessary
	::rtl::OUString aFileName, aNormPath;

	if ( FileBase::getSystemPathFromFileURL( rFileName, aFileName ) != FileBase::E_None )
		aFileName = rFileName;
	Open( aFileName, nOpenMode );
}

/*************************************************************************
|*
|*    SvFileStream::SvFileStream()
|*
|*    Beschreibung      STREAM.SDW
|*    Ersterstellung    OV 22.11.94
|*    Letzte Aenderung  OV 22.11.94
|*
*************************************************************************/

SvFileStream::SvFileStream()
{
    bIsOpen             = FALSE;
    nLockCounter        = 0;
    bIsWritable         = FALSE;
    pInstanceData       = new StreamData;
    SetBufferSize( 8192 );
}

/*************************************************************************
|*
|*    SvFileStream::~SvFileStream()
|*
|*    Beschreibung      STREAM.SDW
|*    Ersterstellung    OV 14.06.94
|*    Letzte Aenderung  OV 14.06.94
|*
*************************************************************************/

SvFileStream::~SvFileStream()
{
    Close();
    if( pInstanceData )
        delete pInstanceData;
}

/*************************************************************************
|*
|*    SvFileStream::GetFileHandle()
|*
|*    Beschreibung      STREAM.SDW
|*    Ersterstellung    OV 14.06.94
|*    Letzte Aenderung  OV 14.06.94
|*
*************************************************************************/

ULONG SvFileStream::GetFileHandle() const
{
    return (ULONG)pInstanceData->hFile;
}

/*************************************************************************
|*
|*    SvFileStream::IsA()
|*
|*    Beschreibung      STREAM.SDW
|*    Ersterstellung    OV 14.06.94
|*    Letzte Aenderung  OV 14.06.94
|*
*************************************************************************/

USHORT SvFileStream::IsA() const
{
    return ID_FILESTREAM;
}

/*************************************************************************
|*
|*    SvFileStream::GetData()
|*
|*    Beschreibung      STREAM.SDW, Prueft nicht Eof; IsEof danach rufbar
|*    Ersterstellung    OV 15.06.94
|*    Letzte Aenderung  OV 15.06.94
|*
*************************************************************************/

ULONG SvFileStream::GetData( void* pData, ULONG nSize )
{
#ifdef DBG_UTIL
    ByteString aTraceStr( "SvFileStream::GetData(): " );
    aTraceStr += ByteString::CreateFromInt64(nSize);
    aTraceStr += " Bytes from ";
    aTraceStr += ByteString(aFilename, osl_getThreadTextEncoding());
    DBG_TRACE( aTraceStr.GetBuffer() );
#endif

    ULONG nCount = 0L;
    if( IsOpen() )
    {
        APIRET nResult;
        nResult = DosRead( pInstanceData->hFile,(PVOID)pData,nSize,&nCount );
        if( nResult )
            SetError(::GetSvError(nResult) );
    }
    return nCount;
}

/*************************************************************************
|*
|*    SvFileStream::PutData()
|*
|*    Beschreibung      STREAM.SDW
|*    Ersterstellung    OV 15.06.94
|*    Letzte Aenderung  OV 15.06.94
|*
*************************************************************************/

ULONG SvFileStream::PutData( const void* pData, ULONG nSize )
{
#ifdef DBG_UTIL
    ByteString aTraceStr( "SvFileStrean::PutData: " );
    aTraceStr += ByteString::CreateFromInt64(nSize);
    aTraceStr += " Bytes to ";
    aTraceStr += ByteString(aFilename, osl_getThreadTextEncoding());
    DBG_TRACE( aTraceStr.GetBuffer() );
#endif

    ULONG nCount = 0L;
    if( IsOpen() )
    {
        APIRET nResult;
        nResult = DosWrite( pInstanceData->hFile,(PVOID)pData,nSize,&nCount );
        if( nResult )
            SetError(::GetSvError(nResult) );
        else if( !nCount )
            SetError( SVSTREAM_DISK_FULL );
    }
    return nCount;
}

/*************************************************************************
|*
|*    SvFileStream::SeekPos()
|*
|*    Beschreibung      STREAM.SDW
|*    Ersterstellung    OV 15.06.94
|*    Letzte Aenderung  OV 15.06.94
|*
*************************************************************************/

ULONG SvFileStream::SeekPos( ULONG nPos )
{
    ULONG nNewPos = 0L;
    if( IsOpen() )
    {
        APIRET nResult;

        if( nPos != STREAM_SEEK_TO_END )
            nResult = DosSetFilePtr( pInstanceData->hFile,(long)nPos,
                 FILE_BEGIN, &nNewPos );
        else
            nResult = DosSetFilePtr( pInstanceData->hFile,0L,
                 FILE_END, &nNewPos );

        if( nResult )
            SetError(::GetSvError(nResult) );
    }
    else
        SetError( SVSTREAM_GENERALERROR );
    return nNewPos;
}

/*************************************************************************
|*
|*    SvFileStream::Tell()
|*
|*    Beschreibung      STREAM.SDW
|*    Ersterstellung    OV 15.06.94
|*    Letzte Aenderung  OV 15.06.94
|*
*************************************************************************/
/*
ULONG SvFileStream::Tell()
{
    ULONG nPos = 0L;

    if( IsOpen() )
    {
        APIRET nResult;
        nResult = DosSetFilePtr(pInstanceData->hFile,0L,FILE_CURRENT,&nPos);
        if( nResult )
            SetError(::GetSvError(nResult) );
    }
    return nPos;
}
*/

/*************************************************************************
|*
|*    SvFileStream::FlushData()
|*
|*    Beschreibung      STREAM.SDW
|*    Ersterstellung    OV 15.06.94
|*    Letzte Aenderung  OV 15.06.94
|*
*************************************************************************/

void SvFileStream::FlushData()
{
    if( IsOpen() )
    {
        APIRET nResult;
        nResult = DosResetBuffer(pInstanceData->hFile );
        if( nResult )
            SetError(::GetSvError(nResult) );
    }
}

/*************************************************************************
|*
|*    SvFileStream::LockRange()
|*
|*    Beschreibung      STREAM.SDW
|*    Ersterstellung    OV 15.06.94
|*    Letzte Aenderung  OV 15.06.94
|*
*************************************************************************/

sal_Bool SvFileStream::LockRange( ULONG nByteOffset, ULONG nBytes )
{
    sal_Bool bRetVal = FALSE;
    if( IsOpen() )
    {
        APIRET   nResult;
        FILELOCK aLockArea, aUnlockArea;
        aUnlockArea.lOffset = 0L;
        aUnlockArea.lRange      = 0L;
        aLockArea.lOffset       = (long)nByteOffset;
        aLockArea.lRange        = (long)nBytes;

        nResult = DosSetFileLocks(pInstanceData->hFile,
            &aUnlockArea, &aLockArea,
            1000UL,  // Zeit in ms bis Abbruch
            0L       // kein Atomic-Lock
        );

        if( nResult )
            SetError(::GetSvError(nResult) );
        else
            bRetVal = TRUE;
    }
    return bRetVal;
}

/*************************************************************************
|*
|*    SvFileStream::UnlockRange()
|*
|*    Beschreibung      STREAM.SDW
|*    Ersterstellung    OV 15.06.94
|*    Letzte Aenderung  OV 15.06.94
|*
*************************************************************************/

sal_Bool SvFileStream::UnlockRange( ULONG nByteOffset, ULONG nBytes )
{
    sal_Bool bRetVal = FALSE;
    if( IsOpen() )
    {
        APIRET   nResult;
        FILELOCK aLockArea, aUnlockArea;
        aLockArea.lOffset       = 0L;
        aLockArea.lRange        = 0L;
        aUnlockArea.lOffset = (long)nByteOffset;
        aUnlockArea.lRange      = (long)nBytes;

        nResult = DosSetFileLocks(pInstanceData->hFile,
            &aUnlockArea, &aLockArea,
            1000UL,  // Zeit in ms bis Abbruch
            0L       // kein Atomic-Lock
        );

        if( nResult )
            SetError(::GetSvError(nResult) );
        else
            bRetVal = TRUE;
    }
    return bRetVal;
}

/*************************************************************************
|*
|*    SvFileStream::LockFile()
|*
|*    Beschreibung      STREAM.SDW
|*    Ersterstellung    OV 15.06.94
|*    Letzte Aenderung  OV 15.06.94
|*
*************************************************************************/

sal_Bool SvFileStream::LockFile()
{
    sal_Bool bRetVal = FALSE;
    if( !nLockCounter )
    {
        if( LockRange( 0L, LONG_MAX ) )
        {
            nLockCounter = 1;
            bRetVal = TRUE;
        }
    }
    else
    {
        nLockCounter++;
        bRetVal = TRUE;
    }
    return bRetVal;
}

/*************************************************************************
|*
|*    SvFileStream::UnlockFile()
|*
|*    Beschreibung      STREAM.SDW
|*    Ersterstellung    OV 15.06.94
|*    Letzte Aenderung  OV 15.06.94
|*
*************************************************************************/

sal_Bool SvFileStream::UnlockFile()
{
    sal_Bool bRetVal = FALSE;
    if( nLockCounter > 0)
    {
        if( nLockCounter == 1)
        {
            if( UnlockRange( 0L, LONG_MAX ) )
            {
                nLockCounter = 0;
                bRetVal = TRUE;
            }
        }
        else
        {
            nLockCounter--;
            bRetVal = TRUE;
        }
    }
    return bRetVal;
}

/*************************************************************************
|*
|*    SvFileStream::Open()
|*
|*    Beschreibung      STREAM.SDW
|*    Ersterstellung    OV 15.06.94
|*    Letzte Aenderung  OV 15.06.94
|*
*************************************************************************/

#if 0
BOOL createLongNameEA   ( const PCSZ pszPath, ULONG ulAttributes, const String& aLongName );
#endif

void SvFileStream::Open( const String& rFilename, StreamMode nOpenMode )
{
        String aParsedFilename;

#if 0
        if      ( Folder::IsAvailable() && (rFilename.Search('{') < 9) )
        {
                String          aVirtualPart;
                String          aRealPart;
                String          aVirtualPath;
                ItemIDPath      aVirtualURL;
                ULONG           nDivider = 0;

                String          aVirtualString(rFilename);

                for (int x=aVirtualString.Len(); x>0; x--)
                {
                        if (aVirtualString.Copy(x,1).Compare("}")==COMPARE_EQUAL)
                        {
                                nDivider = x;
                                break;
                        }
                }

                aVirtualPart = aVirtualString.Copy(0,nDivider+1);
                aRealPart = aVirtualString.Copy(nDivider+2);

                aVirtualURL  = aVirtualPart;
                aVirtualPath = aVirtualURL.GetHostNotationPath();

                DirEntry aTempDirEntry(aVirtualPath);

                aTempDirEntry += aRealPart;

                aParsedFilename = aTempDirEntry.GetFull();
        }
        else
#endif // 0
        {
                aParsedFilename = rFilename;
        }

    Close();
    SvStream::ClearBuffer();

    ULONG   nActionTaken;
    ULONG   nOpenAction     = 0L;
    ULONG   nShareBits      = 0L;
    ULONG   nReadWriteBits  = 0L;

    eStreamMode = nOpenMode;
    eStreamMode &= ~STREAM_TRUNC; // beim ReOpen nicht cutten

    nOpenMode |= STREAM_SHARE_DENYNONE;  // definierten Zustand garantieren

    // ********* Zugriffsflags ***********
    if( nOpenMode & STREAM_SHARE_DENYNONE)
        nShareBits = OPEN_SHARE_DENYNONE;

    if( nOpenMode & STREAM_SHARE_DENYREAD)
        nShareBits = OPEN_SHARE_DENYREAD;

    if( nOpenMode & STREAM_SHARE_DENYWRITE)
        nShareBits = OPEN_SHARE_DENYWRITE;

    if( nOpenMode & STREAM_SHARE_DENYALL)
        nShareBits = OPEN_SHARE_DENYREADWRITE;

        if( (nOpenMode & STREAM_READ) )
    {
        if( nOpenMode & STREAM_WRITE )
            nReadWriteBits |= OPEN_ACCESS_READWRITE;
        else
        {
            nReadWriteBits |= OPEN_ACCESS_READONLY;
            nOpenMode |= STREAM_NOCREATE;
        }
    }
    else
        nReadWriteBits |= OPEN_ACCESS_WRITEONLY;


    if( nOpenMode & STREAM_NOCREATE )
    {
        // Datei nicht erzeugen
        nOpenAction = OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS;
    }
    else
    {
        // Datei erzeugen, wenn nicht vorhanden
        nOpenAction = OPEN_ACTION_CREATE_IF_NEW;
        if( nOpenMode & STREAM_TRUNC )
            // Auf Nullaenge kuerzen, wenn existiert
            nOpenAction |= OPEN_ACTION_REPLACE_IF_EXISTS;
        else
            // Inhalt der Datei nicht wegwerfen
            nOpenAction |= OPEN_ACTION_OPEN_IF_EXISTS;
    }

#if 0 // YD
	//
	// resolves long FAT names used by OS2
	//
	BOOL bIsLongOS2=FALSE;
	if (Folder::IsAvailable())
	{
		DirEntry aDirEntry(rFilename);
		if  (aDirEntry.IsLongNameOnFAT())
		{
			// in kurzen Pfad wandeln
			ItemIDPath      aItemIDPath(rFilename);
			aParsedFilename = aItemIDPath.GetHostNotationPath();
			bIsLongOS2 = TRUE;
		}
	}
#endif

	aFilename = aParsedFilename;
	ByteString aFileNameA( aFilename, gsl_getSystemTextEncoding());
	FSysRedirector::DoRedirect( aFilename );

#ifdef DBG_UTIL
	ByteString aTraceStr( "SvFileStream::Open(): " );
	aTraceStr +=  aFileNameA;
	DBG_TRACE( aTraceStr.GetBuffer() );
#endif

    APIRET nRet = DosOpen( aFileNameA.GetBuffer(), &pInstanceData->hFile,
    &nActionTaken, 0L, FILE_NORMAL, nOpenAction,
    nReadWriteBits | nShareBits | OPEN_FLAGS_NOINHERIT, 0L);

    if( nRet == ERROR_TOO_MANY_OPEN_FILES )
    {
		long nToAdd = 10;
		ULONG nCurMaxFH;
		nRet = DosSetRelMaxFH( &nToAdd, &nCurMaxFH );
		nRet = DosOpen( aFileNameA.GetBuffer(), &pInstanceData->hFile,
		&nActionTaken, 0L, FILE_NORMAL, nOpenAction,
		nReadWriteBits | nShareBits | OPEN_FLAGS_NOINHERIT, 0L);
    }

    // Bei Fehler pruefen, ob wir lesen duerfen
    if( nRet==ERROR_ACCESS_DENIED || nRet==ERROR_SHARING_VIOLATION )
    {
        nReadWriteBits = OPEN_ACCESS_READONLY;
        nRet = DosOpen( aFileNameA.GetBuffer(), &pInstanceData->hFile,
            &nActionTaken, 0L, FILE_NORMAL, nOpenAction,
            nReadWriteBits | nShareBits | OPEN_FLAGS_NOINHERIT, 0L);
    }

        if( nRet )
    {
        bIsOpen = FALSE;
        SetError(::GetSvError(nRet) );
    }
    else
    {
        bIsOpen     = TRUE;
        pInstanceData->bIsEof = FALSE;
        if( nReadWriteBits != OPEN_ACCESS_READONLY )
            bIsWritable = TRUE;
    }

#if 0
	if (bIsOpen && bIsLongOS2)
	{
		//file schlie�en, da sonst createLongName u.U. nicht m�glich
		Close();

		// erzeugtem File langen Namen geben
		DirEntry aDirEntry(rFilename);
		createLongNameEA(aFileNameA.GetBuffer(),  FILE_NORMAL, aDirEntry.GetName());

		// und wieder oeffnen
		ReOpen();		
	}
#endif

}

/*************************************************************************
|*
|*    SvFileStream::ReOpen()
|*
|*    Beschreibung      STREAM.SDW
|*    Ersterstellung    OV 15.06.94
|*    Letzte Aenderung  OV 15.06.94
|*
*************************************************************************/

void SvFileStream::ReOpen()
{
    if( !bIsOpen && aFilename.Len() )
        Open( aFilename, eStreamMode );
}

/*************************************************************************
|*
|*    SvFileStream::Close()
|*
|*    Beschreibung      STREAM.SDW
|*    Ersterstellung    OV 15.06.94
|*    Letzte Aenderung  OV 15.06.94
|*
*************************************************************************/

void SvFileStream::Close()
{
    if( IsOpen() )
    {
#ifdef DBG_UTIL
        ByteString aTraceStr( "SvFileStream::Close(): " );
        aTraceStr += ByteString(aFilename, osl_getThreadTextEncoding());
        DBG_TRACE( aTraceStr.GetBuffer() );
#endif

        if( nLockCounter )
        {
            nLockCounter = 1;
            UnlockFile();
        }
        Flush();
        DosClose( pInstanceData->hFile );
    }

    bIsOpen     = FALSE;
    nLockCounter= 0;
    bIsWritable = FALSE;
    pInstanceData->bIsEof = TRUE;
    SvStream::ClearBuffer();
    SvStream::ClearError();
}

/*************************************************************************
|*
|*    SvFileStream::ResetError()
|*
|*    Beschreibung      STREAM.SDW; Setzt Filepointer auf Dateianfang
|*    Ersterstellung    OV 15.06.94
|*    Letzte Aenderung  OV 15.06.94
|*
*************************************************************************/

void SvFileStream::ResetError()
{
    SvStream::ClearError();
}

/*************************************************************************
|*
|*    SvFileStream::SetSize()
|*
|*    Beschreibung
|*    Ersterstellung    OV 19.10.95
|*    Letzte Aenderung  OV 19.10.95
|*
*************************************************************************/

void SvFileStream::SetSize( ULONG nSize )
{
    if( IsOpen() )
    {
        APIRET nRet = DosSetFileSize( pInstanceData->hFile, nSize );
        if( nRet )
            SetError( ::GetSvError( nRet ) );
    }
}

#if 0
/*************************************************************************
|*
|*    SvSharedMemoryStream::AllocateMemory()
|*
|*    Beschreibung      STREAM.SDW
|*    Ersterstellung    CL 05.05.95
|*    Letzte Aenderung  CL 05.05.95
|*
*************************************************************************/

sal_Bool SvSharedMemoryStream::AllocateMemory( ULONG nNewSize )
{
        DBG_ASSERT(aHandle==0,"Keine Handles unter OS/2");
        DBG_ASSERT(nNewSize,"Cannot allocate zero Bytes");
        APIRET nRet = DosAllocSharedMem( (void**)&pBuf, (PSZ)NULL, nNewSize,
                                     PAG_READ | PAG_WRITE | PAG_COMMIT |
                                     OBJ_GIVEABLE | OBJ_GETTABLE | OBJ_ANY);
    return( nRet == 0 );
}

/*************************************************************************
|*
|*    SvSharedMemoryStream::ReAllocateMemory()   (Bozo-Algorithmus)
|*
|*    Beschreibung      STREAM.SDW
|*    Ersterstellung    CL 05.05.95
|*    Letzte Aenderung  CL 05.05.95
|*
*************************************************************************/

sal_Bool SvSharedMemoryStream::ReAllocateMemory( long nDiff )
{
	DBG_ASSERT(aHandle==0,"Keine Handles unter OS/2");
	sal_Bool bRetVal    = FALSE;
    ULONG nNewSize  = nSize + nDiff;
        if( nNewSize )
        {
                // neuen Speicher nicht ueber AllocateMemory holen, da wir den
                // alten Speicher behalten wollen, falls nicht genuegend Platz
                // fuer den neuen Block da ist
                char* pNewBuf;
            APIRET nRet = DosAllocSharedMem( (void**)&pNewBuf,(PSZ)NULL,nNewSize,
                                PAG_READ | PAG_WRITE | PAG_COMMIT |
                                OBJ_GIVEABLE | OBJ_GETTABLE | OBJ_ANY);
            DBG_ASSERT(!nRet,"DosAllocSharedMem failed");

            if( !nRet )
            {
                bRetVal = TRUE; // Success!
                if( nNewSize < nSize )      // Verkleinern ?
                {
                    memcpy( pNewBuf, pBuf, (size_t)nNewSize );
                    if( nPos > nNewSize )
                        nPos = 0L;
                    if( nEndOfData >= nNewSize )
                        nEndOfData = nNewSize-1L;
                }
                else
                    memcpy( pNewBuf, pBuf, (size_t)nSize );

                FreeMemory(); // den alten Block loeschen ...

				// und den neuen Block in Dienst stellen
				pBuf  = (sal_uInt8*)pNewBuf;
                nSize = nNewSize;
            }
        }
        else
        {
                bRetVal = TRUE;
                FreeMemory();
                pBuf = 0;
                nSize = 0;
                nEndOfData = 0;
        }
    return bRetVal;
}

void SvSharedMemoryStream::FreeMemory()
{
	DBG_ASSERT(aHandle==0,"Keine Handles unter OS/2");
    DosFreeMem( pBuf );
}

/*************************************************************************
|*
|*    SvSharedMemoryStream::SetHandle()
|*
|*    Beschreibung      STREAM.SDW
|*    Ersterstellung    OV 05.10.95
|*    Letzte Aenderung  OV 05.10.95
|*
*************************************************************************/

void* SvSharedMemoryStream::SetHandle( void* aNewHandle, sal_Size nSize,
                                       sal_Bool bOwnsData, sal_Size nEOF )
{
	DBG_ERROR("OS/2 does not support memory handles");
    // return SetBuffer(aNewHandle, nSize, bOwnsData, nEOF );
	return 0;
}


#endif // 0
