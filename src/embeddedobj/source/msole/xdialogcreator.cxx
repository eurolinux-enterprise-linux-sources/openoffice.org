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
#include "precompiled_embeddedobj.hxx"
#include <com/sun/star/embed/XEmbedObjectCreator.hpp>
#include <com/sun/star/embed/XEmbeddedObject.hpp>
#include <com/sun/star/embed/EntryInitModes.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/datatransfer/DataFlavor.hpp>
#include <com/sun/star/ucb/CommandAbortedException.hpp>


#include <osl/thread.h>
#include <osl/file.hxx>
#include <vos/module.hxx>
#include <comphelper/classids.hxx>

#include "platform.h"
#include <comphelper/mimeconfighelper.hxx>

#include "xdialogcreator.hxx"
#include "oleembobj.hxx"
// LLA: tip from FS
// #include <confighelper.hxx>
#include <xdialogcreator.hxx>
#include <oleembobj.hxx>


#ifdef WNT

#include <oledlg.h>

class InitializedOleGuard
{
public:
	InitializedOleGuard()
	{
		if ( !SUCCEEDED( OleInitialize( NULL ) ) )
			throw ::com::sun::star::uno::RuntimeException();
	}

	~InitializedOleGuard()
	{
		OleUninitialize();
	}
};

extern "C" {
typedef UINT STDAPICALLTYPE OleUIInsertObjectA_Type(LPOLEUIINSERTOBJECTA);
}

#endif


using namespace ::com::sun::star;
using namespace ::comphelper;
//-------------------------------------------------------------------------
uno::Sequence< sal_Int8 > GetRelatedInternalID_Impl( const uno::Sequence< sal_Int8 >& aClassID )
{
	// Writer
	if ( MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SW_OLE_EMBED_CLASSID_60 ) )
	  || MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SW_OLE_EMBED_CLASSID_8 ) ) )
		return MimeConfigurationHelper::GetSequenceClassID( SO3_SW_CLASSID_60 );

	// Calc
	if ( MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SC_OLE_EMBED_CLASSID_60 ) )
	  || MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SC_OLE_EMBED_CLASSID_8 ) ) )
		return MimeConfigurationHelper::GetSequenceClassID( SO3_SC_CLASSID_60 );

	// Impress
	if ( MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SIMPRESS_OLE_EMBED_CLASSID_60 ) )
	  || MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SIMPRESS_OLE_EMBED_CLASSID_8 ) ) )
		return MimeConfigurationHelper::GetSequenceClassID( SO3_SIMPRESS_CLASSID_60 );

	// Draw
	if ( MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SDRAW_OLE_EMBED_CLASSID_60 ) )
	  || MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SDRAW_OLE_EMBED_CLASSID_8 ) ) )
		return MimeConfigurationHelper::GetSequenceClassID( SO3_SDRAW_CLASSID_60 );

	// Chart
	if ( MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SCH_OLE_EMBED_CLASSID_60 ) )
	  || MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SCH_OLE_EMBED_CLASSID_8 ) ) )
		return MimeConfigurationHelper::GetSequenceClassID( SO3_SCH_CLASSID_60 );

	// Math
	if ( MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SM_OLE_EMBED_CLASSID_60 ) )
	  || MimeConfigurationHelper::ClassIDsEqual( aClassID, MimeConfigurationHelper::GetSequenceClassID( SO3_SM_OLE_EMBED_CLASSID_8 ) ) )
		return MimeConfigurationHelper::GetSequenceClassID( SO3_SM_CLASSID_60 );

	return aClassID;
}

//-------------------------------------------------------------------------
uno::Sequence< ::rtl::OUString > SAL_CALL MSOLEDialogObjectCreator::impl_staticGetSupportedServiceNames()
{
    uno::Sequence< ::rtl::OUString > aRet(2);
    aRet[0] = ::rtl::OUString::createFromAscii("com.sun.star.embed.MSOLEObjectSystemCreator");
    aRet[1] = ::rtl::OUString::createFromAscii("com.sun.star.comp.embed.MSOLEObjectSystemCreator");
    return aRet;
}

//-------------------------------------------------------------------------
::rtl::OUString SAL_CALL MSOLEDialogObjectCreator::impl_staticGetImplementationName()
{
    return ::rtl::OUString::createFromAscii("com.sun.star.comp.embed.MSOLEObjectSystemCreator");
}

//-------------------------------------------------------------------------
uno::Reference< uno::XInterface > SAL_CALL MSOLEDialogObjectCreator::impl_staticCreateSelfInstance(
			const uno::Reference< lang::XMultiServiceFactory >& xServiceManager )
{
	return uno::Reference< uno::XInterface >( *new MSOLEDialogObjectCreator( xServiceManager ) );
}

//-------------------------------------------------------------------------
embed::InsertedObjectInfo SAL_CALL MSOLEDialogObjectCreator::createInstanceByDialog(
			const uno::Reference< embed::XStorage >& xStorage,
			const ::rtl::OUString& sEntName,
			const uno::Sequence< beans::PropertyValue >& aInObjArgs )
	throw ( lang::IllegalArgumentException,
			io::IOException,
			uno::Exception,
			uno::RuntimeException )
{
	embed::InsertedObjectInfo aObjectInfo;
	uno::Sequence< beans::PropertyValue > aObjArgs( aInObjArgs );

#ifdef WNT

	if ( !xStorage.is() )
		throw lang::IllegalArgumentException( ::rtl::OUString::createFromAscii( "No parent storage is provided!\n" ),
											uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ),
											1 );

	if ( !sEntName.getLength() )
		throw lang::IllegalArgumentException( ::rtl::OUString::createFromAscii( "Empty element name is provided!\n" ),
											uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ),
											2 );

	InitializedOleGuard aGuard;

	OLEUIINSERTOBJECT io;
	char szFile[MAX_PATH];
	UINT uTemp;

	memset(&io, 0, sizeof(io));

	io.cbStruct = sizeof(io);
	io.hWndOwner = GetActiveWindow();

	szFile[0] = 0;
	io.lpszFile = szFile;
	io.cchFile = MAX_PATH;

	io.dwFlags = IOF_SELECTCREATENEW | IOF_DISABLELINK;


	::vos::OModule aOleDlgLib;
	if( !aOleDlgLib.load( ::rtl::OUString::createFromAscii( "oledlg" ) ) )
		throw uno::RuntimeException();

	OleUIInsertObjectA_Type * pInsertFct = (OleUIInsertObjectA_Type *)
								aOleDlgLib.getSymbol( ::rtl::OUString::createFromAscii( "OleUIInsertObjectA" ) );
	if( !pInsertFct )
		throw uno::RuntimeException();

	uTemp=pInsertFct(&io);

	if ( OLEUI_OK == uTemp )
	{
		if (io.dwFlags & IOF_SELECTCREATENEW)
		{
			uno::Reference< embed::XEmbedObjectCreator > xEmbCreator(
						m_xFactory->createInstance(
								::rtl::OUString::createFromAscii( "com.sun.star.embed.EmbeddedObjectCreator" ) ),
						uno::UNO_QUERY );
			if ( !xEmbCreator.is() )
				throw uno::RuntimeException();

			uno::Sequence< sal_Int8 > aClassID = MimeConfigurationHelper::GetSequenceClassID( io.clsid.Data1,
																	 io.clsid.Data2,
																	 io.clsid.Data3,
																	 io.clsid.Data4[0],
																	 io.clsid.Data4[1],
																	 io.clsid.Data4[2],
																	 io.clsid.Data4[3],
																	 io.clsid.Data4[4],
																	 io.clsid.Data4[5],
																	 io.clsid.Data4[6],
																	 io.clsid.Data4[7] );

			aClassID = GetRelatedInternalID_Impl( aClassID );

			//TODO: retrieve ClassName
			::rtl::OUString aClassName;
			aObjectInfo.Object = uno::Reference< embed::XEmbeddedObject >(
							xEmbCreator->createInstanceInitNew( aClassID, aClassName, xStorage, sEntName, aObjArgs ),
							uno::UNO_QUERY );
		}
		else
		{
			::rtl::OUString aFileName = ::rtl::OStringToOUString( ::rtl::OString( szFile ), osl_getThreadTextEncoding() );
    		rtl::OUString aFileURL;
        	if ( osl::FileBase::getFileURLFromSystemPath( aFileName, aFileURL ) != osl::FileBase::E_None )
				throw uno::RuntimeException();

			uno::Sequence< beans::PropertyValue > aMediaDescr( 1 );
			aMediaDescr[0].Name = ::rtl::OUString::createFromAscii( "URL" );
			aMediaDescr[0].Value <<= aFileURL;

			// TODO: use config helper for type detection
			uno::Reference< embed::XEmbedObjectCreator > xEmbCreator;
			::comphelper::MimeConfigurationHelper aHelper( m_xFactory );

			if ( aHelper.AddFilterNameCheckOwnFile( aMediaDescr ) )
				xEmbCreator = uno::Reference< embed::XEmbedObjectCreator >(
						m_xFactory->createInstance(
								::rtl::OUString::createFromAscii( "com.sun.star.embed.EmbeddedObjectCreator" ) ),
						uno::UNO_QUERY );
			else
				xEmbCreator = uno::Reference< embed::XEmbedObjectCreator >(
						m_xFactory->createInstance(
								::rtl::OUString::createFromAscii( "com.sun.star.embed.OLEEmbeddedObjectFactory" ) ),
						uno::UNO_QUERY );

			if ( !xEmbCreator.is() )
				throw uno::RuntimeException();

			aObjectInfo.Object = uno::Reference< embed::XEmbeddedObject >(
							xEmbCreator->createInstanceInitFromMediaDescriptor( xStorage, sEntName, aMediaDescr, aObjArgs ),
							uno::UNO_QUERY );
		}

		if ( ( io.dwFlags & IOF_CHECKDISPLAYASICON) && io.hMetaPict != NULL )
		{
			METAFILEPICT* pMF = ( METAFILEPICT* )GlobalLock( io.hMetaPict );
			if ( pMF )
			{
				sal_uInt32 nBufSize = GetMetaFileBitsEx( pMF->hMF, 0, NULL );
				uno::Sequence< sal_Int8 > aMetafile( nBufSize + 22 );
				sal_uInt8* pBuf = (sal_uInt8*)( aMetafile.getArray() );
				*( (long* )pBuf ) = 0x9ac6cdd7L;
				*( (short* )( pBuf+6 )) = ( SHORT ) 0;
				*( (short* )( pBuf+8 )) = ( SHORT ) 0;
				*( (short* )( pBuf+10 )) = ( SHORT ) pMF->xExt;
				*( (short* )( pBuf+12 )) = ( SHORT ) pMF->yExt;
				*( (short* )( pBuf+14 )) = ( USHORT ) 2540;

				if ( nBufSize && nBufSize == GetMetaFileBitsEx( pMF->hMF, nBufSize, pBuf+22 ) )
				{
					datatransfer::DataFlavor aFlavor(
                        ::rtl::OUString::createFromAscii( "application/x-openoffice-wmf;windows_formatname=\"Image WMF\"" ),
						::rtl::OUString::createFromAscii( "Image WMF" ),
						getCppuType( ( const uno::Sequence< sal_Int8 >* ) 0 ) );

					aObjectInfo.Options.realloc( 2 );
					aObjectInfo.Options[0].Name = ::rtl::OUString::createFromAscii( "Icon" );
					aObjectInfo.Options[0].Value <<= aMetafile;
					aObjectInfo.Options[1].Name = ::rtl::OUString::createFromAscii( "IconFormat" );
					aObjectInfo.Options[1].Value <<= aFlavor;
				}

				GlobalUnlock( io.hMetaPict );
			}
		}
	}
	else
		throw ucb::CommandAbortedException();

#else
	throw lang::NoSupportException(); // TODO:
#endif

	OSL_ENSURE( aObjectInfo.Object.is(), "No object was created!\n" );
	if ( !aObjectInfo.Object.is() )
		throw uno::RuntimeException();

	return aObjectInfo;
}

//-------------------------------------------------------------------------
embed::InsertedObjectInfo SAL_CALL MSOLEDialogObjectCreator::createInstanceInitFromClipboard(
				const uno::Reference< embed::XStorage >& xStorage,
				const ::rtl::OUString& sEntryName,
				const uno::Sequence< beans::PropertyValue >& aObjectArgs )
		throw ( lang::IllegalArgumentException,
				io::IOException,
				uno::Exception,
				uno::RuntimeException )
{
	embed::InsertedObjectInfo aObjectInfo;

#ifdef WNT

	if ( !xStorage.is() )
		throw lang::IllegalArgumentException( ::rtl::OUString::createFromAscii( "No parent storage is provided!\n" ),
											uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ),
											1 );

	if ( !sEntryName.getLength() )
		throw lang::IllegalArgumentException( ::rtl::OUString::createFromAscii( "Empty element name is provided!\n" ),
											uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ),
											2 );

	uno::Reference< embed::XEmbeddedObject > xResult(
					static_cast< ::cppu::OWeakObject* > ( new OleEmbeddedObject( m_xFactory ) ),
					uno::UNO_QUERY );

	uno::Reference< embed::XEmbedPersist > xPersist( xResult, uno::UNO_QUERY );

	if ( !xPersist.is() )
		throw uno::RuntimeException(); // TODO: the interface must be supported by own document objects

	xPersist->setPersistentEntry( xStorage,
									sEntryName,
									embed::EntryInitModes::DEFAULT_INIT,
									uno::Sequence< beans::PropertyValue >(),
									aObjectArgs );

	aObjectInfo.Object = xResult;

	// TODO/LATER: in case of iconifie object the icon should be stored in aObjectInfo
#else
	throw lang::NoSupportException(); // TODO:
#endif

	OSL_ENSURE( aObjectInfo.Object.is(), "No object was created!\n" );
	if ( !aObjectInfo.Object.is() )
		throw uno::RuntimeException();

	return aObjectInfo;
}

//-------------------------------------------------------------------------
::rtl::OUString SAL_CALL MSOLEDialogObjectCreator::getImplementationName()
	throw ( uno::RuntimeException )
{
	return impl_staticGetImplementationName();
}

//-------------------------------------------------------------------------
sal_Bool SAL_CALL MSOLEDialogObjectCreator::supportsService( const ::rtl::OUString& ServiceName )
	throw ( uno::RuntimeException )
{
	uno::Sequence< ::rtl::OUString > aSeq = impl_staticGetSupportedServiceNames();

	for ( sal_Int32 nInd = 0; nInd < aSeq.getLength(); nInd++ )
    	if ( ServiceName.compareTo( aSeq[nInd] ) == 0 )
        	return sal_True;

	return sal_False;
}

//-------------------------------------------------------------------------
uno::Sequence< ::rtl::OUString > SAL_CALL MSOLEDialogObjectCreator::getSupportedServiceNames()
	throw ( uno::RuntimeException )
{
	return impl_staticGetSupportedServiceNames();
}

