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
#include "precompiled_sot.hxx"
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <com/sun/star/embed/XStorage.hpp>
#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>

#include <rtl/digest.h>
#include <osl/file.hxx>
#include <stg.hxx>
#include <storinfo.hxx>
#include <sot/storage.hxx>
#include <sot/formats.hxx>
#include <sot/exchange.hxx>
#include <unotools/ucbstreamhelper.hxx>
#ifndef _TOOLS_FSYS_HXX
#include <tools/fsys.hxx>
#endif
#include <tools/cachestr.hxx>
#include <tools/debug.hxx>
#include <tools/urlobj.hxx>
#include <unotools/localfilehelper.hxx>
#include <unotools/ucbhelper.hxx>
#include <comphelper/processfactory.hxx>

#include "unostorageholder.hxx"

using namespace ::com::sun::star;

/************** class SotStorageStream ***********************************/
class SotStorageStreamFactory : public SotFactory
{
public:
 		TYPEINFO();
		SotStorageStreamFactory( const SvGlobalName & rName,
					  		const String & rClassName,
					  		CreateInstanceType pCreateFuncP )
			: SotFactory( rName, rClassName, pCreateFuncP )
		{}
};
TYPEINIT1(SotStorageStreamFactory,SotFactory);


SO2_IMPL_BASIC_CLASS1_DLL(SotStorageStream,SotStorageStreamFactory,SotObject,
						SvGlobalName( 0xd7deb420, 0xf902, 0x11d0,
							0xaa, 0xa1, 0x0, 0xa0, 0x24, 0x9d, 0x55, 0x90 ) )
SO2_IMPL_INVARIANT(SotStorageStream)


void SotStorageStream::TestMemberObjRef( BOOL /*bFree*/ )
{
}

#ifdef TEST_INVARIANT
void SotStorageStream::TestMemberInvariant( BOOL /*bPrint*/ )
{
}
#endif

/************************************************************************
|*    SotStorageStream::SotStorageStream()
|*
|*    Beschreibung
*************************************************************************/
SvLockBytesRef MakeLockBytes_Impl( const String & rName, StreamMode nMode )
{
	SvLockBytesRef xLB;
	if( rName.Len() )
	{
		SvStream * pFileStm = new SvFileStream( rName, nMode );
		xLB = new SvLockBytes( pFileStm, TRUE );
	}
	else
	{
		SvStream * pCacheStm = new SvCacheStream();
		xLB = new SvLockBytes( pCacheStm, TRUE );
	}
	return xLB;
}

SotStorageStream::SotStorageStream( const String & rName, StreamMode nMode,
                                  StorageMode
                                  #ifdef DBG_UTIL
                                  nStorageMode
                                  #endif
                                  )
	: SvStream( MakeLockBytes_Impl( rName, nMode ) )
    , pOwnStm( NULL )
{
	if( nMode & STREAM_WRITE )
    	bIsWritable = TRUE;
	else
    	bIsWritable = FALSE;

    DBG_ASSERT( !nStorageMode,"StorageModes ignored" );
}

SotStorageStream::SotStorageStream( BaseStorageStream * pStm )
{
	if( pStm )
	{
		if( STREAM_WRITE & pStm->GetMode() )
    		bIsWritable = TRUE;
		else
    		bIsWritable = FALSE;

		pOwnStm = pStm;
    	SetError( pStm->GetError() );
    	pStm->ResetError();
	}
	else
	{
		pOwnStm = NULL;
    	bIsWritable = TRUE;
		SetError( SVSTREAM_INVALID_PARAMETER );
	}
}

SotStorageStream::SotStorageStream()
	: pOwnStm( NULL )
{
	// ??? wenn Init virtuell ist, entsprechen setzen
    bIsWritable = TRUE;
}

/************************************************************************
|*    SotStorageStream::~SotStorageStream()
|*
|*    Beschreibung
*************************************************************************/
SotStorageStream::~SotStorageStream()
{
    Flush(); //SetBufferSize(0);
    delete pOwnStm;
}

/*************************************************************************
|*    SotStorageStream::SyncSvStream()
|*
|*    Beschreibung: Der SvStream wird auf den Zustand des Standard-Streams
|*	  				gesetzt. Der Puffer des SvStreams wird weggeworfen.
*************************************************************************/
void SotStorageStream::SyncSvStream()
{
	ULONG nPos = 0;
    if( pOwnStm )
	{
        pOwnStm->Flush();
		nPos = pOwnStm->Tell();
        SetError( pOwnStm->GetError() );
		SvStream::SyncSvStream( nPos );
    }
}

/*************************************************************************
|*    SotStorageStream::ResetError()
|*
|*    Beschreibung
*************************************************************************/
void SotStorageStream::ResetError()
{
    SvStream::ResetError();
	if( pOwnStm )
         pOwnStm->ResetError();
}

/*************************************************************************
|*    SotStorageStream::GetData()
|*
|*    Beschreibung
*************************************************************************/
ULONG SotStorageStream::GetData( void* pData, ULONG nSize )
{
    ULONG nRet = 0;

    if( pOwnStm )
	{
        nRet = pOwnStm->Read( pData, nSize );
        SetError( pOwnStm->GetError() );
    }
	else
		nRet = SvStream::GetData( (sal_Char *)pData, nSize );
    return nRet;
}

/*************************************************************************
|*    SotStorageStream::PutData()
|*
|*    Beschreibung
*************************************************************************/
ULONG SotStorageStream::PutData( const void* pData, ULONG nSize )
{
    ULONG nRet = 0;

    if( pOwnStm )
	{
        nRet = pOwnStm->Write( pData, nSize );
        SetError( pOwnStm->GetError() );
    }
	else
		nRet = SvStream::PutData( (sal_Char *)pData, nSize );
    return nRet;
}

/*************************************************************************
|*    SotStorageStream::SeekPos()
|*
|*    Beschreibung
*************************************************************************/
ULONG SotStorageStream::SeekPos( ULONG nPos )
{
    ULONG nRet = 0;

    if( pOwnStm )
	{
        nRet = pOwnStm->Seek( nPos );
        SetError( pOwnStm->GetError() );
	}
	else
		nRet = SvStream::SeekPos( nPos );
    return nRet;
}

/*************************************************************************
|*    SotStorageStream::Flush()
|*
|*    Beschreibung
*************************************************************************/
void SotStorageStream::FlushData()
{
    if( pOwnStm )
	{
        pOwnStm->Flush();
        SetError( pOwnStm->GetError() );
    }
	else
		SvStream::FlushData();
}

/*************************************************************************
|*    SotStorageStream::SetSize()
|*
|*    Beschreibung
*************************************************************************/
void SotStorageStream::SetSize( ULONG nNewSize )
{
    ULONG   nPos = Tell();
    if( pOwnStm )
    {
        pOwnStm->SetSize( nNewSize );
        SetError( pOwnStm->GetError() );
    }
    else
		SvStream::SetSize( nNewSize );

    if( nNewSize < nPos )
        // ans Ende setzen
        Seek( nNewSize );

    //return GetError() == SVSTREAM_OK;
}

/*************************************************************************
|*
|*    SotStorageStream::GetSize()
|*
|*    Beschreibung
|*
*************************************************************************/
UINT32 SotStorageStream::GetSize() const
{
    ULONG nPos = Tell();
    ((SotStorageStream *)this)->Seek( STREAM_SEEK_TO_END );
    ULONG nSize = Tell();
    ((SotStorageStream *)this)->Seek( nPos );
    return nSize;
}

/*************************************************************************
|*    SotStorageStream::CopyTo()
|*
|*    Beschreibung
*************************************************************************/
BOOL SotStorageStream::CopyTo( SotStorageStream * pDestStm )
{
    Flush(); // alle Daten schreiben
    pDestStm->ClearBuffer();
	if( !pOwnStm || !pDestStm->pOwnStm )
    { // Wenn Ole2 oder nicht nur eigene StorageStreams

        ULONG nPos = Tell();    // Position merken
		Seek( 0L );
        pDestStm->SetSize( 0 ); // Ziel-Stream leeren

        void * pMem = new BYTE[ 8192 ];
        ULONG  nRead;
        while( 0 != (nRead = Read( pMem, 8192 )) )
        {
            if( nRead != pDestStm->Write( pMem, nRead ) )
            {
                SetError( SVSTREAM_GENERALERROR );
                break;
            }
        }
        delete [] static_cast<BYTE*>(pMem);
        // Position setzen
        pDestStm->Seek( nPos );
        Seek( nPos );
    }
    else
    {
        /*
        // Kopieren
        nErr = pObjI->CopyTo( pDestStm->pObjI, uSize, NULL, &uWrite );
        if( SUCCEEDED( nErr ) )
        {
            // Ziel-Streamzeiger steht hinter den Daten
            // SvSeek abgleichen
            pDestStm->Seek( uWrite.LowPart );
        }
        else if( GetScode( nErr ) == E_NOTIMPL )
        { // Eines Tages werden alle MS... ?!#
        */
        pOwnStm->CopyTo( pDestStm->pOwnStm );
        SetError( pOwnStm->GetError() );
    }
    return GetError() == SVSTREAM_OK;
}

/*************************************************************************
|*    SotStorageStream::Commit()
|*    SotStorageStream::Revert()
|*
|*    Beschreibung
*************************************************************************/
BOOL SotStorageStream::Commit()
{
    if( pOwnStm )
	{
		pOwnStm->Flush();
		if( pOwnStm->GetError() == SVSTREAM_OK )
			pOwnStm->Commit();
        SetError( pOwnStm->GetError() );
    }
    return GetError() == SVSTREAM_OK;
}

BOOL SotStorageStream::Revert()
{
    if( !pOwnStm )
    {
        pOwnStm->Revert();
        SetError( pOwnStm->GetError() );
    }
    return GetError() == SVSTREAM_OK;
}

BOOL SotStorageStream::SetProperty( const String& rName, const ::com::sun::star::uno::Any& rValue )
{
    UCBStorageStream* pStg = PTR_CAST( UCBStorageStream, pOwnStm );
    if ( pStg )
    {
        return pStg->SetProperty( rName, rValue );
    }
    else
    {
        DBG_ERROR("Not implemented!");
        return FALSE;
    }
}

BOOL SotStorageStream::GetProperty( const String& rName, ::com::sun::star::uno::Any& rValue )
{
    UCBStorageStream* pStg = PTR_CAST( UCBStorageStream, pOwnStm );
    if ( pStg )
    {
        return pStg->GetProperty( rName, rValue );
    }
	else
    {
        DBG_ERROR("Not implemented!");
        return FALSE;
    }
}

::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream > SotStorageStream::GetXInputStream() const
{
    UCBStorageStream* pStg = PTR_CAST( UCBStorageStream, pOwnStm );
    if ( pStg )
    {
        return pStg->GetXInputStream();
    }
    else
    {
        DBG_ERROR("Not implemented!");
        return ::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream >();
    }
}



/************** class SotStorage ******************************************
*************************************************************************/
class SotStorageFactory : public SotFactory
{
public:
 		TYPEINFO();
		SotStorageFactory( const SvGlobalName & rName,
					  		const String & rClassName,
					  		CreateInstanceType pCreateFuncP )
			: SotFactory( rName, rClassName, pCreateFuncP )
		{}
};
TYPEINIT1(SotStorageFactory,SotFactory);


SO2_IMPL_BASIC_CLASS1_DLL(SotStorage,SotStorageFactory,SotObject,
						SvGlobalName( 0x980ce7e0, 0xf905, 0x11d0,
							0xaa, 0xa1, 0x0, 0xa0, 0x24, 0x9d, 0x55, 0x90 ) )
SO2_IMPL_INVARIANT(SotStorage)


/************************************************************************
|*
|*    SotStorage::Tes*()
|*
|*    Beschreibung
*************************************************************************/
void SotStorage::TestMemberObjRef( BOOL /*bFree*/ )
{
}

#ifdef TEST_INVARIANT
void SotStorage::TestMemberInvariant( BOOL /*bPrint*/ )
{
}
#endif

/************************************************************************
|*
|*    SotStorage::SotStorage()
|*
|*    Beschreibung      Es muss ein I... Objekt an SvObject uebergeben
|*                      werden, da es sonst selbst ein IUnknown anlegt und
|*                      festlegt, dass alle weiteren I... Objekte mit
|*                      delete zerstoert werden (Owner() == TRUE).
|*                      Es werden aber nur IStorage Objekte benutzt und nicht
|*                      selbst implementiert, deshalb wird so getan, als ob
|*                      das IStorage Objekt von aussen kam und es wird mit
|*                      Release() freigegeben.
|*                      Die CreateStorage Methoden werden benoetigt, um
|*                      ein IStorage Objekt vor dem Aufruf von SvObject
|*                      zu erzeugen (Own, !Own automatik).
|*                      Hat CreateStorage ein Objekt erzeugt, dann wurde
|*                      der RefCounter schon um 1 erhoet.
|*                      Die Uebergabe erfolgt in pStorageCTor. Die Variable
|*                      ist NULL, wenn es nicht geklappt hat.
|*    Ersterstellung    MM 23.06.94
|*    Letzte Aenderung  MM 23.06.94
|*
*************************************************************************/
#define INIT_SotStorage()            		\
	: m_pOwnStg( NULL )                       \
    , m_pStorStm( NULL )                      \
    , m_nError( SVSTREAM_OK )         		\
	, m_bIsRoot( FALSE )              		\
    , m_bDelStm( FALSE )						\
    , m_nVersion( SOFFICE_FILEFORMAT_CURRENT )

SotStorage::SotStorage()
    INIT_SotStorage()
{
    // ??? What's this ???
}

#define ERASEMASK  ( STREAM_TRUNC | STREAM_WRITE | STREAM_SHARE_DENYALL )
#include <com/sun/star/uno/Reference.h>
#include <com/sun/star/ucb/XCommandEnvironment.hpp>
#include <ucbhelper/content.hxx>

SotStorage::SotStorage( const ::ucbhelper::Content& rContent, const String & rName, StreamMode nMode, StorageMode nStorageMode )
    INIT_SotStorage()
{
    m_aName = rName; // Namen merken
	m_pOwnStg = new UCBStorage( rContent, m_aName, nMode, (nStorageMode & STORAGE_TRANSACTED) ? FALSE : TRUE );

	SetError( m_pOwnStg->GetError() );

    if ( IsOLEStorage() )
        m_nVersion = SOFFICE_FILEFORMAT_50;

    SignAsRoot( m_pOwnStg->IsRoot() );
}

SotStorage::SotStorage( const String & rName, StreamMode nMode, StorageMode nStorageMode )
    INIT_SotStorage()
{
    m_aName = rName; // Namen merken
    CreateStorage( TRUE, nMode, nStorageMode );
    if ( IsOLEStorage() )
        m_nVersion = SOFFICE_FILEFORMAT_50;
}

void SotStorage::CreateStorage( BOOL bForceUCBStorage, StreamMode nMode, StorageMode nStorageMode  )
{
	DBG_ASSERT( !m_pStorStm && !m_pOwnStg, "Use only in ctor!" );
    if( m_aName.Len() )
	{
        // named storage
        if( ( ( nMode & ERASEMASK ) == ERASEMASK ) )
            ::utl::UCBContentHelper::Kill( m_aName );

        INetURLObject aObj( m_aName );
        if ( aObj.GetProtocol() == INET_PROT_NOT_VALID )
        {
            String aURL;
            ::utl::LocalFileHelper::ConvertPhysicalNameToURL( m_aName, aURL );
            aObj.SetURL( aURL );
			m_aName = aObj.GetMainURL( INetURLObject::NO_DECODE );
        }

        // a new unpacked storage should be created
        if ( nStorageMode == STORAGE_CREATE_UNPACKED )
        {
            // don't open stream readwrite, content provider may not support this !
            String aURL = UCBStorage::CreateLinkFile( m_aName );
            if ( aURL.Len() )
            {
                ::ucbhelper::Content aContent( aURL, ::com::sun::star::uno::Reference < ::com::sun::star::ucb::XCommandEnvironment >() );
                m_pOwnStg = new UCBStorage( aContent, aURL, nMode, FALSE );
            }
            else
            {
                m_pOwnStg = new Storage( m_aName, nMode, (nStorageMode & STORAGE_TRANSACTED) ? FALSE : TRUE );
                SetError( ERRCODE_IO_NOTSUPPORTED );
            }
        }
        else
        {
            // check the stream
            m_pStorStm = ::utl::UcbStreamHelper::CreateStream( m_aName, nMode );
            if ( m_pStorStm && m_pStorStm->GetError() )
                DELETEZ( m_pStorStm );

            if ( m_pStorStm )
            {
                // try as UCBStorage, next try as OLEStorage
                BOOL bIsUCBStorage = UCBStorage::IsStorageFile( m_pStorStm );
                if ( !bIsUCBStorage && bForceUCBStorage )
                    // if UCBStorage has priority, it should not be used only if it is really an OLEStorage
                    bIsUCBStorage = !Storage::IsStorageFile( m_pStorStm );

                if ( bIsUCBStorage )
                {
                    if ( UCBStorage::GetLinkedFile( *m_pStorStm ).Len() )
                    {
                        // detect special unpacked storages
                        m_pOwnStg = new UCBStorage( *m_pStorStm, (nStorageMode & STORAGE_TRANSACTED) ? FALSE : TRUE );
                        m_bDelStm = TRUE;
                    }
                    else
                    {
                        // detect special disk spanned storages
                        if ( UCBStorage::IsDiskSpannedFile( m_pStorStm ) )
                            nMode |= STORAGE_DISKSPANNED_MODE;

                        // UCBStorage always works directly on the UCB content, so discard the stream first
                        DELETEZ( m_pStorStm );
                        m_pOwnStg = new UCBStorage( m_aName, nMode, (nStorageMode & STORAGE_TRANSACTED) ? FALSE : TRUE );
                    }
                }
                else
                {
                    // OLEStorage can be opened with a stream
                    m_pOwnStg = new Storage( *m_pStorStm, (nStorageMode & STORAGE_TRANSACTED) ? FALSE : TRUE );
                    m_bDelStm = TRUE;
                }
            }
            else if ( bForceUCBStorage )
            {
                m_pOwnStg = new UCBStorage( m_aName, nMode, (nStorageMode & STORAGE_TRANSACTED) ? FALSE : TRUE );
                SetError( ERRCODE_IO_NOTSUPPORTED );
            }
            else
            {
                m_pOwnStg = new Storage( m_aName, nMode, (nStorageMode & STORAGE_TRANSACTED) ? FALSE : TRUE );
                SetError( ERRCODE_IO_NOTSUPPORTED );
            }
        }
    }
    else
    {
        // temporary storage
        if ( bForceUCBStorage )
        	m_pOwnStg = new UCBStorage( m_aName, nMode, (nStorageMode & STORAGE_TRANSACTED) ? FALSE : TRUE );
        else
            m_pOwnStg = new Storage( m_aName, nMode, (nStorageMode & STORAGE_TRANSACTED) ? FALSE : TRUE );
		m_aName = m_pOwnStg->GetName();
    }

	SetError( m_pOwnStg->GetError() );

    SignAsRoot( m_pOwnStg->IsRoot() );
}

SotStorage::SotStorage( BOOL bUCBStorage, const String & rName, StreamMode nMode, StorageMode nStorageMode )
    INIT_SotStorage()
{
    m_aName = rName;
	CreateStorage( bUCBStorage, nMode, nStorageMode );
    if ( IsOLEStorage() )
        m_nVersion = SOFFICE_FILEFORMAT_50;
}

SotStorage::SotStorage( BaseStorage * pStor )
    INIT_SotStorage()
{
    if ( pStor )
    {
        m_aName = pStor->GetName(); // Namen merken
        SignAsRoot( pStor->IsRoot() );
        SetError( pStor->GetError() );
    }

    m_pOwnStg = pStor;
    ULONG nErr = m_pOwnStg ? m_pOwnStg->GetError() : SVSTREAM_CANNOT_MAKE;
	SetError( nErr );
    if ( IsOLEStorage() )
        m_nVersion = SOFFICE_FILEFORMAT_50;
}

SotStorage::SotStorage( BOOL bUCBStorage, SvStream & rStm )
    INIT_SotStorage()
{
    SetError( rStm.GetError() );

    // try as UCBStorage, next try as OLEStorage
    if ( UCBStorage::IsStorageFile( &rStm ) || bUCBStorage )
        m_pOwnStg = new UCBStorage( rStm, FALSE );
    else
        m_pOwnStg = new Storage( rStm, FALSE );

	SetError( m_pOwnStg->GetError() );

    if ( IsOLEStorage() )
        m_nVersion = SOFFICE_FILEFORMAT_50;

    SignAsRoot( m_pOwnStg->IsRoot() );
}

SotStorage::SotStorage( SvStream & rStm )
    INIT_SotStorage()
{
    SetError( rStm.GetError() );

    // try as UCBStorage, next try as OLEStorage
    if ( UCBStorage::IsStorageFile( &rStm ) )
        m_pOwnStg = new UCBStorage( rStm, FALSE );
    else
        m_pOwnStg = new Storage( rStm, FALSE );

	SetError( m_pOwnStg->GetError() );

    if ( IsOLEStorage() )
        m_nVersion = SOFFICE_FILEFORMAT_50;

    SignAsRoot( m_pOwnStg->IsRoot() );
}

SotStorage::SotStorage( SvStream * pStm, BOOL bDelete )
    INIT_SotStorage()
{
    SetError( pStm->GetError() );

    // try as UCBStorage, next try as OLEStorage
    if ( UCBStorage::IsStorageFile( pStm ) )
        m_pOwnStg = new UCBStorage( *pStm, FALSE );
    else
        m_pOwnStg = new Storage( *pStm, FALSE );

	SetError( m_pOwnStg->GetError() );

	m_pStorStm = pStm;
	m_bDelStm = bDelete;
    if ( IsOLEStorage() )
        m_nVersion = SOFFICE_FILEFORMAT_50;

    SignAsRoot( m_pOwnStg->IsRoot() );
}

/*************************************************************************
|*    SotStorage::~SotStorage()
|*
|*    Beschreibung
*************************************************************************/
SotStorage::~SotStorage()
{
    delete m_pOwnStg;
    if( m_bDelStm )
        delete m_pStorStm;
}

/*************************************************************************
|*    SotStorage::RemoveUNOStorageHolder()
|*
|*    Beschreibung
*************************************************************************/
void SotStorage::RemoveUNOStorageHolder( UNOStorageHolder* pHolder )
{
    UCBStorage* pStg = PTR_CAST( UCBStorage, m_pOwnStg );
    if ( pStg )
    {
        pStg->GetUNOStorageHolderList()->remove( pHolder );
		pHolder->release();
    }
    else
    {
        DBG_ERROR("Not implemented!");
    }
}

/*************************************************************************
|*    SotStorage::GetUNOAPIDuplicate()
|*
|*    Beschreibung
*************************************************************************/
uno::Reference< embed::XStorage > SotStorage::GetUNOAPIDuplicate( const String& rEleName, sal_Int32 nUNOStorageMode )
{
	// after we create a duplicate we will register wrapper
	// for storage messages, the wrapper will control the real storage
	// the real storage will be able to ask the duplicate to dispose if it's parent is disposed

	uno::Reference< embed::XStorage > xResult;

    UCBStorage* pStg = PTR_CAST( UCBStorage, m_pOwnStg );
    if ( !pStg )
		return xResult;

	UNOStorageHolderList* pUNOStorageHolderList = pStg->GetUNOStorageHolderList();
	if ( !pUNOStorageHolderList )
		return xResult;

	for ( UNOStorageHolderList::iterator aIter = pUNOStorageHolderList->begin();
		  aIter != pUNOStorageHolderList->end(); aIter++ )
		if ( (*aIter) && (*aIter)->GetStorageName().Equals( rEleName ) )
		{
			// the storage is already in use
			return xResult;
		}

	if ( IsStream( rEleName ) )
		return xResult;

	if ( GetError() == ERRCODE_NONE )
	{
		StreamMode nMode = ( ( nUNOStorageMode & embed::ElementModes::WRITE ) == embed::ElementModes::WRITE ) ?
									STREAM_WRITE : ( STREAM_READ | STREAM_NOCREATE );
		if ( nUNOStorageMode & embed::ElementModes::NOCREATE )
			nMode |= STREAM_NOCREATE;

		sal_Bool bStorageReady = !IsStorage( rEleName );
		SotStorageRef pChildStorage = OpenUCBStorage( rEleName, nMode, STORAGE_TRANSACTED );
		if ( pChildStorage->GetError() == ERRCODE_NONE && pChildStorage->m_pOwnStg )
		{
			::utl::TempFile* pTempFile = new ::utl::TempFile();
			if ( pTempFile->GetURL().Len() )
			{
					if ( !bStorageReady )
					{
   						UCBStorage* pChildUCBStg = PTR_CAST( UCBStorage, pChildStorage->m_pOwnStg );
						if ( pChildUCBStg )
						{
							UCBStorage* pTempStorage = new UCBStorage( pTempFile->GetURL(), STREAM_WRITE, FALSE, TRUE );
							if ( pTempStorage )
							{
								pChildUCBStg->CopyTo( pTempStorage );

								// CopyTo does not transport unknown media type
								// just workaround it
								uno::Any aMediaType;

								if ( pChildUCBStg->GetProperty(
													::rtl::OUString::createFromAscii( "MediaType" ), aMediaType ) )
									pTempStorage->SetProperty( ::rtl::OUString::createFromAscii( "MediaType" ), aMediaType );

								bStorageReady = !pChildUCBStg->GetError() && !pTempStorage->GetError()
											&& pTempStorage->Commit();

								delete ((BaseStorage*)pTempStorage);
								pTempStorage = NULL;
							}
						}

						OSL_ENSURE( bStorageReady, "Problem on storage copy!\n" );
					}

					if ( bStorageReady )
					{
						try {
							uno::Reference< lang::XSingleServiceFactory > xStorageFactory(
									::comphelper::getProcessServiceFactory()->createInstance(
                                		::rtl::OUString::createFromAscii( "com.sun.star.embed.StorageFactory" ) ),
									uno::UNO_QUERY );

							OSL_ENSURE( xStorageFactory.is(), "Can't create storage factory!\n" );
							if ( xStorageFactory.is() )
							{
								uno::Sequence< uno::Any > aArg( 2 );
								aArg[0] <<= ::rtl::OUString( pTempFile->GetURL() );
								aArg[1] <<= nUNOStorageMode;
								uno::Reference< embed::XStorage > xDuplStorage(
													xStorageFactory->createInstanceWithArguments( aArg ),
													uno::UNO_QUERY );

								OSL_ENSURE( xDuplStorage.is(), "Can't open storage!\n" );
								if ( xDuplStorage.is() )
								{
									UNOStorageHolder* pHolder =
											new UNOStorageHolder( *this, *pChildStorage, xDuplStorage, pTempFile );
									pHolder->acquire();
                                    pTempFile = NULL;
									pUNOStorageHolderList->push_back( pHolder );
									xResult = xDuplStorage;
								}
							}
						}
						catch( uno::Exception& e )
						{
                                                    (void)e;
                                                    OSL_ENSURE( sal_False, ::rtl::OUStringToOString( e.Message, RTL_TEXTENCODING_ASCII_US ) );
						}
					}
			}

            if ( pTempFile != NULL )
                delete pTempFile;
		}
		else
			SetError( pChildStorage->GetError() );
	}

	return xResult;
}

/*************************************************************************
|*    SotStorage::CreateMemoryStream()
|*
|*    Beschreibung
*************************************************************************/
SvMemoryStream * SotStorage::CreateMemoryStream()
{
    SvMemoryStream * pStm = NULL;
	pStm = new SvMemoryStream( 0x8000, 0x8000 );
    SotStorageRef aStg = new SotStorage( *pStm );
    if( CopyTo( aStg ) )
        aStg->Commit();
    else
    {
        aStg.Clear(); // Storage vorher freigeben
        delete pStm;
		pStm = NULL;
    }
    return pStm;
}

/*************************************************************************
|*    SotStorage::GetStorage()
|*
|*    Beschreibung
*************************************************************************/
BOOL SotStorage::IsStorageFile( const String & rFileName )
{
	String aName( rFileName );
    INetURLObject aObj( aName );
    if ( aObj.GetProtocol() == INET_PROT_NOT_VALID )
    {
        String aURL;
        ::utl::LocalFileHelper::ConvertPhysicalNameToURL( aName, aURL );
        aObj.SetURL( aURL );
		aName = aObj.GetMainURL( INetURLObject::NO_DECODE );
    }

	SvStream * pStm = ::utl::UcbStreamHelper::CreateStream( aName, STREAM_STD_READ );
    BOOL bRet = SotStorage::IsStorageFile( pStm );
	delete pStm;
	return bRet;
}

BOOL SotStorage::IsStorageFile( SvStream* pStream )
{
    /** code for new storages must come first! **/
    if ( pStream )
    {
        long nPos = pStream->Tell();
        BOOL bRet = UCBStorage::IsStorageFile( pStream );
		if ( !bRet )
			bRet = Storage::IsStorageFile( pStream );
        pStream->Seek( nPos );
        return bRet;
    }
    else
        return FALSE;
}
/*************************************************************************
|*    SotStorage::GetStorage()
|*
|*    Beschreibung
*************************************************************************/
const String & SotStorage::GetName() const
{
	if( !m_aName.Len() )
	{
    	DBG_ASSERT( Owner(), "must be owner" );
		if( m_pOwnStg )
    		((SotStorage *)this)->m_aName = m_pOwnStg->GetName();
	}
	return m_aName;
}

void SotStorage::SetName( const String& rName )
{
    // This method is necessary because most storages will not be opened with a FileName, but an external stream instead
    // This stream is a stream opened by a UCP and so aName is only used as a transport for all client code of the SotStorage
    // class that depends on the fact that a root storage has a name
    DBG_ASSERT( !GetName().Len(), "SetName() must not be called when the storage already has a name!" );
    m_aName = rName;
}

/*************************************************************************
|*    SotStorage::ResetError()
|*
|*    Beschreibung
*************************************************************************/
void SotStorage::ResetError()
{
    m_nError = SVSTREAM_OK;
    if( m_pOwnStg )
		m_pOwnStg->ResetError();
}

/*************************************************************************
|*    SotStorage::SetClass()
|*    SotStorage::SetConvertClass()
|*
|*    Beschreibung
*************************************************************************/
void SotStorage::SetClass( const SvGlobalName & rName,
                          ULONG nOriginalClipFormat,
                          const String & rUserTypeName )
{
    DBG_ASSERT( Owner(), "must be owner" );
	if( m_pOwnStg )
	    m_pOwnStg->SetClass( rName, nOriginalClipFormat, rUserTypeName );
	else
		SetError( SVSTREAM_GENERALERROR );
}

void SotStorage::SetConvertClass( const SvGlobalName & rName,
                                 ULONG nOriginalClipFormat,
                                 const String & rUserTypeName )
{
    DBG_ASSERT( Owner(), "must be owner" );
	if( m_pOwnStg )
	    m_pOwnStg->SetConvertClass( rName, nOriginalClipFormat, rUserTypeName );
	else
		SetError( SVSTREAM_GENERALERROR );
}

/*************************************************************************
|*    SotStorage::GetClassName()
|*    SotStorage::GetFormat()
|*    SotStorage::GetUserName()
|*    SotStorage::ShouldConvert()
|*
|*    Beschreibung
*************************************************************************/
SvGlobalName SotStorage::GetClassName()
{
    SvGlobalName aGN;
    DBG_ASSERT( Owner(), "must be owner" );
	if( m_pOwnStg )
		aGN = m_pOwnStg->GetClassName();
	else
		SetError( SVSTREAM_GENERALERROR );
    return aGN;
}

ULONG SotStorage::GetFormat()
{
    ULONG nFormat = 0;
    DBG_ASSERT( Owner(), "must be owner" );
	if( m_pOwnStg )
		nFormat = m_pOwnStg->GetFormat();
	else
		SetError( SVSTREAM_GENERALERROR );
    return nFormat;
}

String SotStorage::GetUserName()
{
    String aName;
    DBG_ASSERT( Owner(), "must be owner" );
	if( m_pOwnStg )
		aName = m_pOwnStg->GetUserName();
	else
		SetError( SVSTREAM_GENERALERROR );
    return aName;
}

BOOL SotStorage::ShouldConvert()
{
    DBG_ASSERT( Owner(), "must be owner" );
	if( m_pOwnStg )
		return m_pOwnStg->ShouldConvert();
	else
		SetError( SVSTREAM_GENERALERROR );
	return FALSE;
}

/*************************************************************************
|*    SotStorage::FillInfoList()
|*
|*    Beschreibung
*************************************************************************/
void SotStorage::FillInfoList( SvStorageInfoList * pFillList ) const
{
    DBG_ASSERT( Owner(), "must be owner" );
	if( m_pOwnStg )
		m_pOwnStg->FillInfoList( pFillList );
}

/*************************************************************************
|*    SotStorage::CopyTo()
|*
|*    Beschreibung
*************************************************************************/
BOOL SotStorage::CopyTo( SotStorage * pDestStg )
{
    DBG_ASSERT( Owner(), "must be owner" );
    DBG_ASSERT( pDestStg->Owner(), "must be owner" );
	if( m_pOwnStg && pDestStg->m_pOwnStg )
	{
		m_pOwnStg->CopyTo( pDestStg->m_pOwnStg );
		SetError( m_pOwnStg->GetError() );
		pDestStg->m_aKey = m_aKey;
		pDestStg->m_nVersion = m_nVersion;
	}
	else
		SetError( SVSTREAM_GENERALERROR );
    return SVSTREAM_OK == GetError();
}

/*************************************************************************
|*    SotStorage::Commit()
|*
|*    Beschreibung
*************************************************************************/
BOOL SotStorage::Commit()
{
    DBG_ASSERT( Owner(), "must be owner" );
	if( m_pOwnStg )
	{
		if( !m_pOwnStg->Commit() )
			SetError( m_pOwnStg->GetError() );
	}
	else
		SetError( SVSTREAM_GENERALERROR );
    return SVSTREAM_OK == GetError();
}

/*************************************************************************
|*    SotStorage::Revert()
|*
|*    Beschreibung
*************************************************************************/
BOOL SotStorage::Revert()
{
    DBG_ASSERT( Owner(), "must be owner" );
	if( m_pOwnStg )
	{
		if( !m_pOwnStg->Revert() )
			SetError( m_pOwnStg->GetError() );
	}
	else
		SetError( SVSTREAM_GENERALERROR );
    return SVSTREAM_OK == GetError();
}

/*************************************************************************
|*    SotStorage::OpenStream()
|*
|*    Beschreibung
*************************************************************************/
SotStorageStream * SotStorage::OpenEncryptedSotStream( const String & rEleName, const ByteString& rKey,
										     StreamMode nMode,
											 StorageMode nStorageMode )
{
    DBG_ASSERT( !nStorageMode, "StorageModes ignored" );
    SotStorageStream * pStm = NULL;
    DBG_ASSERT( Owner(), "must be owner" );
	if( m_pOwnStg )
	{
		// volle Ole-Patches einschalten
		// egal was kommt, nur exclusiv gestattet
		nMode |= STREAM_SHARE_DENYALL;
		ErrCode nE = m_pOwnStg->GetError();
        BaseStorageStream* p = m_pOwnStg->OpenStream( rEleName, nMode,
                            (nStorageMode & STORAGE_TRANSACTED) ? FALSE : TRUE, &rKey );
		pStm = new SotStorageStream( p );

		if( !nE )
			m_pOwnStg->ResetError(); // kein Fehler setzen
		if( nMode & STREAM_TRUNC )
			pStm->SetSize( 0 );
	}
	else
		SetError( SVSTREAM_GENERALERROR );
    return pStm;
}

SotStorageStream * SotStorage::OpenSotStream( const String & rEleName,
										     StreamMode nMode,
											 StorageMode nStorageMode )
{
    DBG_ASSERT( !nStorageMode, "StorageModes ignored" );
    SotStorageStream * pStm = NULL;
    DBG_ASSERT( Owner(), "must be owner" );
	if( m_pOwnStg )
	{
		// volle Ole-Patches einschalten
		// egal was kommt, nur exclusiv gestattet
		nMode |= STREAM_SHARE_DENYALL;
		ErrCode nE = m_pOwnStg->GetError();
        BaseStorageStream * p = m_pOwnStg->OpenStream( rEleName, nMode,
							(nStorageMode & STORAGE_TRANSACTED) ? FALSE : TRUE );
		pStm = new SotStorageStream( p );

		if( !nE )
			m_pOwnStg->ResetError(); // kein Fehler setzen
		if( nMode & STREAM_TRUNC )
			pStm->SetSize( 0 );
	}
	else
		SetError( SVSTREAM_GENERALERROR );
    return pStm;
}

/*************************************************************************
|*    SotStorage::OpenStorage()
|*
|*    Beschreibung
*************************************************************************/
SotStorage * SotStorage::OpenSotStorage( const String & rEleName,
										StreamMode nMode,
										StorageMode nStorageMode )
{
    SotStorage * pStor = NULL;
    DBG_ASSERT( Owner(), "must be owner" );
	if( m_pOwnStg )
	{
		nMode |= STREAM_SHARE_DENYALL;
		ErrCode nE = m_pOwnStg->GetError();
        BaseStorage * p = m_pOwnStg->OpenStorage( rEleName, nMode,
						(nStorageMode & STORAGE_TRANSACTED) ? FALSE : TRUE );
		if( p )
		{
			pStor = new SotStorage( p );
			if( !nE )
				m_pOwnStg->ResetError(); // kein Fehler setzen

		    return pStor;
		}
	}

	SetError( SVSTREAM_GENERALERROR );

    return NULL;
}

SotStorage * SotStorage::OpenUCBStorage( const String & rEleName,
										StreamMode nMode,
										StorageMode nStorageMode )
{
    SotStorage * pStor = NULL;
    DBG_ASSERT( Owner(), "must be owner" );
	if( m_pOwnStg )
	{
		nMode |= STREAM_SHARE_DENYALL;
		ErrCode nE = m_pOwnStg->GetError();
        BaseStorage * p = m_pOwnStg->OpenUCBStorage( rEleName, nMode,
						(nStorageMode & STORAGE_TRANSACTED) ? FALSE : TRUE );
		pStor = new SotStorage( p );
		if( !nE )
			m_pOwnStg->ResetError(); // kein Fehler setzen
	}
	else
		SetError( SVSTREAM_GENERALERROR );
    return pStor;
}

SotStorage * SotStorage::OpenOLEStorage( const String & rEleName,
										StreamMode nMode,
										StorageMode nStorageMode )
{
    SotStorage * pStor = NULL;
    DBG_ASSERT( Owner(), "must be owner" );
	if( m_pOwnStg )
	{
		nMode |= STREAM_SHARE_DENYALL;
		ErrCode nE = m_pOwnStg->GetError();
        BaseStorage * p = m_pOwnStg->OpenOLEStorage( rEleName, nMode,
						(nStorageMode & STORAGE_TRANSACTED) ? FALSE : TRUE );
		pStor = new SotStorage( p );
		if( !nE )
			m_pOwnStg->ResetError(); // kein Fehler setzen
	}
	else
		SetError( SVSTREAM_GENERALERROR );
    return pStor;
}

/*************************************************************************
|*    SotStorage::IsStream()
|*    SotStorage::IsStorage()
|*    SotStorage::IsContained()
|*
|*    Beschreibung
*************************************************************************/
BOOL SotStorage::IsStorage( const String & rEleName ) const
{
    DBG_ASSERT( Owner(), "must be owner" );
    // ein bisschen schneller
	if( m_pOwnStg )
		return m_pOwnStg->IsStorage( rEleName );
	return FALSE;
}

BOOL SotStorage::IsStream( const String & rEleName ) const
{
    DBG_ASSERT( Owner(), "must be owner" );
    // ein bisschen schneller
	if( m_pOwnStg )
		return m_pOwnStg->IsStream( rEleName );
	return FALSE;
}

BOOL SotStorage::IsContained( const String & rEleName ) const
{
    DBG_ASSERT( Owner(), "must be owner" );
    // ein bisschen schneller
	if( m_pOwnStg )
		return m_pOwnStg->IsContained( rEleName );
	return FALSE;
}

/*************************************************************************
|*    SotStorage::Remove()
|*
|*    Beschreibung
*************************************************************************/
BOOL SotStorage::Remove( const String & rEleName )
{
    DBG_ASSERT( Owner(), "must be owner" );
	if( m_pOwnStg )
	{
		m_pOwnStg->Remove( rEleName );
		SetError( m_pOwnStg->GetError() );
	}
	else
		SetError( SVSTREAM_GENERALERROR );
    return SVSTREAM_OK == GetError();
}

/*************************************************************************
|*    SotStorage::Rename()
|*
|*    Beschreibung
*************************************************************************/
BOOL SotStorage::Rename( const String & rEleName, const String & rNewName )
{
    DBG_ASSERT( Owner(), "must be owner" );
	if( m_pOwnStg )
	{
		m_pOwnStg->Rename( rEleName, rNewName );
		SetError( m_pOwnStg->GetError() );
	}
	else
		SetError( SVSTREAM_GENERALERROR );
    return SVSTREAM_OK == GetError();
}

/*************************************************************************
|*    SotStorage::CopyTo()
|*
|*    Beschreibung
*************************************************************************/
BOOL SotStorage::CopyTo( const String & rEleName,
                        SotStorage * pNewSt, const String & rNewName )
{
    DBG_ASSERT( Owner(), "must be owner" );
    DBG_ASSERT( pNewSt->Owner(), "must be owner" );
	if( m_pOwnStg )
	{
		m_pOwnStg->CopyTo( rEleName, pNewSt->m_pOwnStg, rNewName );
		SetError( m_pOwnStg->GetError() );
		SetError( pNewSt->GetError() );
	}
	else
		SetError( SVSTREAM_GENERALERROR );
    return SVSTREAM_OK == GetError();
}

/*************************************************************************
|*    SotStorage::MoveTo()
|*
|*    Beschreibung
*************************************************************************/
BOOL SotStorage::MoveTo( const String & rEleName,
                        SotStorage * pNewSt, const String & rNewName )
{
    DBG_ASSERT( Owner(), "must be owner" );
    DBG_ASSERT( pNewSt->Owner(), "must be owner" );
	if( m_pOwnStg )
	{
		m_pOwnStg->MoveTo( rEleName, pNewSt->m_pOwnStg, rNewName );
		SetError( m_pOwnStg->GetError() );
		SetError( pNewSt->GetError() );
	}
	else
		SetError( SVSTREAM_GENERALERROR );
    return SVSTREAM_OK == GetError();
}

const SvStream* SotStorage::GetSvStream()
{
	const SvStream* pResult = 0;
    DBG_ASSERT( Owner(), "must be owner" );
    if( m_pOwnStg )
		pResult = m_pOwnStg->GetSvStream();
	return pResult;
}

SvStream* SotStorage::GetTargetSvStream() const
{
	SvStream* pResult = 0;
    DBG_ASSERT( Owner(), "must be owner" );
    if( m_pOwnStg )
		pResult = (SvStream*)(m_pOwnStg->GetSvStream());
	return pResult;
}


BOOL SotStorage::Validate()
{
	DBG_ASSERT( m_bIsRoot, "Validate nur an Rootstorage" );
	if( m_pOwnStg )
		return m_pOwnStg->ValidateFAT();
	else
		return TRUE;
}

BOOL SotStorage::SetProperty( const String& rName, const ::com::sun::star::uno::Any& rValue )
{
    UCBStorage* pStg = PTR_CAST( UCBStorage, m_pOwnStg );
    if ( pStg )
    {
        return pStg->SetProperty( rName, rValue );
    }
    else
    {
        DBG_WARNING("W1:Not implemented!");
        return FALSE;
    }
}

BOOL SotStorage::GetProperty( const String& rName, ::com::sun::star::uno::Any& rValue )
{
    UCBStorage* pStg = PTR_CAST( UCBStorage, m_pOwnStg );
    if ( pStg )
    {
        return pStg->GetProperty( rName, rValue );
    }
    else if ( rName.CompareToAscii("MediaType") == COMPARE_EQUAL )
	{
		String aStr = SotExchange::GetFormatMimeType( GetFormat() );
		USHORT nPos = aStr.Search(';');
		if ( nPos != STRING_NOTFOUND )
			aStr = aStr.Copy( 0, nPos );
		rValue <<= (::rtl::OUString) aStr;
		return TRUE;
	}
    else
    {
        DBG_WARNING("W1:Not implemented!");
        return FALSE;
    }
}

BOOL SotStorage::GetProperty( const String& rEleName, const String& rName, ::com::sun::star::uno::Any& rValue )
{
    UCBStorage* pStg = PTR_CAST( UCBStorage, m_pOwnStg );
    if ( pStg )
    {
        return pStg->GetProperty( rEleName, rName, rValue );
    }
    else
    {
        DBG_WARNING("W1:Not implemented!");
        return FALSE;
    }
}

BOOL SotStorage::IsOLEStorage() const
{
    UCBStorage* pStg = PTR_CAST( UCBStorage, m_pOwnStg );
	return !pStg;
}

BOOL SotStorage::IsOLEStorage( const String & rFileName )
{
    return Storage::IsStorageFile( rFileName );
}

BOOL SotStorage::IsOLEStorage( SvStream* pStream )
{
    return Storage::IsStorageFile( pStream );
}

void SotStorage::SetKey( const ByteString& rKey )
{
    m_aKey = rKey;
    if ( !IsOLEStorage() )
    {
        sal_uInt8 aBuffer[RTL_DIGEST_LENGTH_SHA1];
        rtlDigestError nError = rtl_digest_SHA1( m_aKey.GetBuffer(), m_aKey.Len(), aBuffer, RTL_DIGEST_LENGTH_SHA1 );
        if ( nError == rtl_Digest_E_None )
        {
            sal_uInt8* pBuffer = aBuffer;
            ::com::sun::star::uno::Sequence < sal_Int8 > aSequ( (sal_Int8*) pBuffer, RTL_DIGEST_LENGTH_SHA1 );
            ::com::sun::star::uno::Any aAny;
            aAny <<= aSequ;
            SetProperty( ::rtl::OUString::createFromAscii("EncryptionKey"), aAny );
        }
    }
}

SotStorage* SotStorage::OpenOLEStorage( const com::sun::star::uno::Reference < com::sun::star::embed::XStorage >& xStorage,
                                    const String& rEleName, StreamMode nMode )
{
    sal_Int32 nEleMode = embed::ElementModes::SEEKABLEREAD;
    if ( nMode & STREAM_WRITE )
        nEleMode |= embed::ElementModes::WRITE;
    if ( nMode & STREAM_TRUNC )
        nEleMode |= embed::ElementModes::TRUNCATE;
    if ( nMode & STREAM_NOCREATE )
        nEleMode |= embed::ElementModes::NOCREATE;

    SvStream* pStream = NULL;
    try
    {
        uno::Reference < io::XStream > xStream = xStorage->openStreamElement( rEleName, nEleMode );

		// TODO/LATER: should it be done this way?
    	if ( nMode & STREAM_WRITE )
		{
			uno::Reference < beans::XPropertySet > xStreamProps( xStream, uno::UNO_QUERY_THROW );
			xStreamProps->setPropertyValue(
						::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "MediaType" ) ),
						uno::makeAny( ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "application/vnd.sun.star.oleobject" ) ) ) );
		}

       	pStream = utl::UcbStreamHelper::CreateStream( xStream );
    }
    catch ( uno::Exception& )
    {
        //TODO/LATER: ErrorHandling
        pStream = new SvMemoryStream;
        pStream->SetError( ERRCODE_IO_GENERAL );
    }

    return new SotStorage( pStream, TRUE );
}

sal_Int32 SotStorage::GetFormatID( const com::sun::star::uno::Reference < com::sun::star::embed::XStorage >& xStorage )
{
    uno::Reference< beans::XPropertySet > xProps( xStorage, uno::UNO_QUERY );
    if ( !xProps.is() )
        return 0;

    ::rtl::OUString aMediaType;
    xProps->getPropertyValue( ::rtl::OUString::createFromAscii( "MediaType" ) ) >>= aMediaType;
    if ( aMediaType.getLength() )
    {
        ::com::sun::star::datatransfer::DataFlavor aDataFlavor;
        aDataFlavor.MimeType = aMediaType;
        return SotExchange::GetFormat( aDataFlavor );
    }

    return 0;
}

sal_Int32 SotStorage::GetVersion( const com::sun::star::uno::Reference < com::sun::star::embed::XStorage >& xStorage )
{
	sal_Int32 nSotFormatID = SotStorage::GetFormatID( xStorage );
	switch( nSotFormatID )
	{
		case SOT_FORMATSTR_ID_STARWRITER_8:
		case SOT_FORMATSTR_ID_STARWRITER_8_TEMPLATE:
		case SOT_FORMATSTR_ID_STARWRITERWEB_8:
		case SOT_FORMATSTR_ID_STARWRITERGLOB_8:
		case SOT_FORMATSTR_ID_STARDRAW_8:
		case SOT_FORMATSTR_ID_STARDRAW_8_TEMPLATE:
		case SOT_FORMATSTR_ID_STARIMPRESS_8:
		case SOT_FORMATSTR_ID_STARIMPRESS_8_TEMPLATE:
		case SOT_FORMATSTR_ID_STARCALC_8:
		case SOT_FORMATSTR_ID_STARCALC_8_TEMPLATE:
		case SOT_FORMATSTR_ID_STARCHART_8:
		case SOT_FORMATSTR_ID_STARCHART_8_TEMPLATE:
		case SOT_FORMATSTR_ID_STARMATH_8:
		case SOT_FORMATSTR_ID_STARMATH_8_TEMPLATE:
			return SOFFICE_FILEFORMAT_8;
		case SOT_FORMATSTR_ID_STARWRITER_60:
		case SOT_FORMATSTR_ID_STARWRITERWEB_60:
		case SOT_FORMATSTR_ID_STARWRITERGLOB_60:
		case SOT_FORMATSTR_ID_STARDRAW_60:
		case SOT_FORMATSTR_ID_STARIMPRESS_60:
		case SOT_FORMATSTR_ID_STARCALC_60:
		case SOT_FORMATSTR_ID_STARCHART_60:
		case SOT_FORMATSTR_ID_STARMATH_60:
			return SOFFICE_FILEFORMAT_60;
	}

	return 0;
}

