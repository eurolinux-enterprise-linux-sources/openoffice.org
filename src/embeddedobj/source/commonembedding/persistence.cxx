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

#include <commonembobj.hxx>
#include <com/sun/star/embed/Aspects.hpp>
#include <com/sun/star/document/XStorageBasedDocument.hpp>
#include <com/sun/star/embed/EmbedStates.hpp>
#include <com/sun/star/embed/EmbedVerbs.hpp>
#include <com/sun/star/embed/EntryInitModes.hpp>
#include <com/sun/star/embed/XStorage.hpp>
#include <com/sun/star/embed/XOptimizedStorage.hpp>
#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/embed/EmbedUpdateModes.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/frame/XStorable.hpp>
#include <com/sun/star/frame/XLoadable.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/frame/XModule.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/util/XModifiable.hpp>

#ifndef _COM_SUN_STAR_CONTAINER_XNAMEACCEESS_HPP_
#include <com/sun/star/container/XNameAccess.hpp>
#endif
#include <com/sun/star/container/XChild.hpp>
#include <com/sun/star/util/XCloseable.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/IllegalTypeException.hpp>
#include <com/sun/star/chart2/XChartDocument.hpp>

#include <comphelper/fileformat.h>
#include <comphelper/storagehelper.hxx>
#include <comphelper/mimeconfighelper.hxx>

#include <rtl/logfile.hxx>

#define USE_STORAGEBASED_DOCUMENT

using namespace ::com::sun::star;


//------------------------------------------------------
uno::Sequence< beans::PropertyValue > GetValuableArgs_Impl( const uno::Sequence< beans::PropertyValue >& aMedDescr,
															sal_Bool bCanUseDocumentBaseURL )
{
	uno::Sequence< beans::PropertyValue > aResult;
	sal_Int32 nResLen = 0;

	for ( sal_Int32 nInd = 0; nInd < aMedDescr.getLength(); nInd++ )
	{
		if ( aMedDescr[nInd].Name.equalsAscii( "ComponentData" )
		  || aMedDescr[nInd].Name.equalsAscii( "DocumentTitle" )
		  || aMedDescr[nInd].Name.equalsAscii( "InteractionHandler" )
		  || aMedDescr[nInd].Name.equalsAscii( "JumpMark" )
		  // || aMedDescr[nInd].Name.equalsAscii( "Password" ) makes no sence for embedded objects
		  || aMedDescr[nInd].Name.equalsAscii( "Preview" )
		  || aMedDescr[nInd].Name.equalsAscii( "ReadOnly" )
		  || aMedDescr[nInd].Name.equalsAscii( "StartPresentation" )
		  || aMedDescr[nInd].Name.equalsAscii( "RepairPackage" )
		  || aMedDescr[nInd].Name.equalsAscii( "StatusIndicator" )
		  || aMedDescr[nInd].Name.equalsAscii( "ViewData" )
		  || aMedDescr[nInd].Name.equalsAscii( "ViewId" )
		  || aMedDescr[nInd].Name.equalsAscii( "MacroExecutionMode" )
		  || aMedDescr[nInd].Name.equalsAscii( "UpdateDocMode" )
		  || (aMedDescr[nInd].Name.equalsAscii( "DocumentBaseURL" ) && bCanUseDocumentBaseURL) )
		{
			aResult.realloc( ++nResLen );
			aResult[nResLen-1] = aMedDescr[nInd];
		}
	}

	return aResult;
}

//------------------------------------------------------
uno::Sequence< beans::PropertyValue > addAsTemplate( const uno::Sequence< beans::PropertyValue >& aOrig )
{
	sal_Bool bAsTemplateSet = sal_False;
	sal_Int32 nLength = aOrig.getLength();
	uno::Sequence< beans::PropertyValue > aResult( nLength );

	for ( sal_Int32 nInd = 0; nInd < nLength; nInd++ )
	{
		aResult[nInd].Name = aOrig[nInd].Name;
		if ( aResult[nInd].Name.equalsAscii( "AsTemplate" ) )
		{
			aResult[nInd].Value <<= sal_True;
			bAsTemplateSet = sal_True;
		}
		else
			aResult[nInd].Value = aOrig[nInd].Value;
	}

	if ( !bAsTemplateSet )
	{
		aResult.realloc( nLength + 1 );
		aResult[nLength].Name = ::rtl::OUString::createFromAscii( "AsTemplate" );
		aResult[nLength].Value <<= sal_True;
	}

	return aResult;
}

//------------------------------------------------------
uno::Reference< io::XInputStream > createTempInpStreamFromStor(
															const uno::Reference< embed::XStorage >& xStorage,
															const uno::Reference< lang::XMultiServiceFactory >& xFactory )
{
	OSL_ENSURE( xStorage.is(), "The storage can not be empty!" );

	uno::Reference< io::XInputStream > xResult;

	const ::rtl::OUString aServiceName ( RTL_CONSTASCII_USTRINGPARAM ( "com.sun.star.io.TempFile" ) );
	uno::Reference < io::XStream > xTempStream = uno::Reference < io::XStream > (
															xFactory->createInstance ( aServiceName ),
															uno::UNO_QUERY );
	if ( xTempStream.is() )
	{
		uno::Reference < lang::XSingleServiceFactory > xStorageFactory(
					xFactory->createInstance ( ::rtl::OUString::createFromAscii( "com.sun.star.embed.StorageFactory" ) ),
					uno::UNO_QUERY );

		uno::Sequence< uno::Any > aArgs( 2 );
		aArgs[0] <<= xTempStream;
		aArgs[1] <<= embed::ElementModes::READWRITE;
		uno::Reference< embed::XStorage > xTempStorage( xStorageFactory->createInstanceWithArguments( aArgs ),
														uno::UNO_QUERY );
		if ( !xTempStorage.is() )
			throw uno::RuntimeException(); // TODO:

		try
		{
			xStorage->copyToStorage( xTempStorage );
		} catch( uno::Exception& e )
		{
			throw embed::StorageWrappedTargetException(
						::rtl::OUString::createFromAscii( "Can't copy storage!" ),
						uno::Reference< uno::XInterface >(),
						uno::makeAny( e ) );
		}

		try {
			uno::Reference< lang::XComponent > xComponent( xTempStorage, uno::UNO_QUERY );
			OSL_ENSURE( xComponent.is(), "Wrong storage implementation!" );
			if ( xComponent.is() )
				xComponent->dispose();
		}
		catch ( uno::Exception& )
		{
		}

		try {
			uno::Reference< io::XOutputStream > xTempOut = xTempStream->getOutputStream();
			if ( xTempOut.is() )
				xTempOut->closeOutput();
		}
		catch ( uno::Exception& )
		{
		}

		xResult = xTempStream->getInputStream();
	}

	return xResult;

}

//------------------------------------------------------
static uno::Reference< util::XCloseable > CreateDocument( const uno::Reference< lang::XMultiServiceFactory >& _rxFactory,
    const ::rtl::OUString& _rDocumentServiceName, bool _bEmbeddedScriptSupport )
{
    uno::Sequence< uno::Any > aArguments(2);
    aArguments[0] <<= beans::NamedValue(
                        ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "EmbeddedObject" ) ),
                        uno::makeAny( (sal_Bool)sal_True )
                      );
    aArguments[1] <<= beans::NamedValue(
                        ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "EmbeddedScriptSupport" ) ),
                        uno::makeAny( (sal_Bool)_bEmbeddedScriptSupport )
                      );

    uno::Reference< uno::XInterface > xDocument;
    try
    {
        xDocument = _rxFactory->createInstanceWithArguments( _rDocumentServiceName, aArguments );
    }
    catch( const uno::Exception& )
    {
        // some of our embedded object implementations (in particular chart) do neither support
        // the EmbeddedObject, nor the EmbeddedScriptSupport argument. Also, they do not support
        // XInitialization, which means the default factory from cppuhelper will throw an
        // IllegalArgumentException when we try to create the instance with arguments.
        // Okay, so we fall back to creating the instance without any arguments.
        xDocument = _rxFactory->createInstance( _rDocumentServiceName );
    }

    return uno::Reference< util::XCloseable >( xDocument, uno::UNO_QUERY );
}

//------------------------------------------------------
static void SetDocToEmbedded( const uno::Reference< frame::XModel > xDocument, const ::rtl::OUString& aModuleName )
{
	if ( xDocument.is() )
	{
		uno::Sequence< beans::PropertyValue > aSeq( 1 );
		aSeq[0].Name = ::rtl::OUString::createFromAscii( "SetEmbedded" );
		aSeq[0].Value <<= sal_True;
		xDocument->attachResource( ::rtl::OUString(), aSeq );

		if ( aModuleName.getLength() )
		{
			try
			{
				uno::Reference< frame::XModule > xModule( xDocument, uno::UNO_QUERY_THROW );
				xModule->setIdentifier( aModuleName );
			}
			catch( uno::Exception& )
			{}
		}
	}
}

//------------------------------------------------------
void OCommonEmbeddedObject::SwitchOwnPersistence( const uno::Reference< embed::XStorage >& xNewParentStorage,
												  const uno::Reference< embed::XStorage >& xNewObjectStorage,
												  const ::rtl::OUString& aNewName )
{
	if ( xNewParentStorage == m_xParentStorage && aNewName.equals( m_aEntryName ) )
	{
		OSL_ENSURE( xNewObjectStorage == m_xObjectStorage, "The storage must be the same!\n" );
		return;
	}

    uno::Reference< lang::XComponent > xComponent( m_xObjectStorage, uno::UNO_QUERY );
    OSL_ENSURE( !m_xObjectStorage.is() || xComponent.is(), "Wrong storage implementation!" );

    m_xObjectStorage = xNewObjectStorage;
    m_xParentStorage = xNewParentStorage;
    m_aEntryName = aNewName;

#ifdef USE_STORAGEBASED_DOCUMENT
	// the linked document should not be switched
	if ( !m_bIsLink )
	{
		uno::Reference< document::XStorageBasedDocument > xDoc( m_pDocHolder->GetComponent(), uno::UNO_QUERY );
		if ( xDoc.is() )
			xDoc->switchToStorage( m_xObjectStorage );
	}
#endif

    try {
        if ( xComponent.is() )
            xComponent->dispose();
    }
    catch ( uno::Exception& )
    {
    }
}

//------------------------------------------------------
void OCommonEmbeddedObject::SwitchOwnPersistence( const uno::Reference< embed::XStorage >& xNewParentStorage,
												  const ::rtl::OUString& aNewName )
{
	if ( xNewParentStorage == m_xParentStorage && aNewName.equals( m_aEntryName ) )
		return;

	sal_Int32 nStorageMode = m_bReadOnly ? embed::ElementModes::READ : embed::ElementModes::READWRITE;

	uno::Reference< embed::XStorage > xNewOwnStorage = xNewParentStorage->openStorageElement( aNewName, nStorageMode );
	OSL_ENSURE( xNewOwnStorage.is(), "The method can not return empty reference!" );

	SwitchOwnPersistence( xNewParentStorage, xNewOwnStorage, aNewName );
}

//------------------------------------------------------
uno::Reference< util::XCloseable > OCommonEmbeddedObject::InitNewDocument_Impl()
{
    uno::Reference< util::XCloseable > xDocument( CreateDocument( m_xFactory, GetDocumentServiceName(),
                                                m_bEmbeddedScriptSupport ) );

	uno::Reference< frame::XModel > xModel( xDocument, uno::UNO_QUERY );
	uno::Reference< frame::XLoadable > xLoadable( xModel, uno::UNO_QUERY );
	if ( !xLoadable.is() )
		throw uno::RuntimeException();

	try
	{
		// set the document mode to embedded as the first action on document!!!
        SetDocToEmbedded( xModel, m_aModuleName );

        try
        {
            uno::Reference < container::XChild > xChild( xDocument, uno::UNO_QUERY );
            if ( xChild.is() )
                xChild->setParent( m_xParent );
        }
        catch( const lang::NoSupportException & )
        {
            OSL_ENSURE( false, "Cannot set parent at document" );
        }

		// init document as a new
		xLoadable->initNew();
		xModel->attachResource( xModel->getURL(),m_aDocMediaDescriptor);
	}
	catch( uno::Exception& )
	{
		uno::Reference< util::XCloseable > xCloseable( xDocument, uno::UNO_QUERY );
		if ( xCloseable.is() )
		{
			try
			{
				xCloseable->close( sal_True );
			}
			catch( uno::Exception& )
			{
			}
		}

		throw; // TODO
	}

	return xDocument;
}

//------------------------------------------------------
uno::Reference< util::XCloseable > OCommonEmbeddedObject::LoadLink_Impl()
{
    uno::Reference< util::XCloseable > xDocument( CreateDocument( m_xFactory, GetDocumentServiceName(),
                                                m_bEmbeddedScriptSupport ) );

	uno::Reference< frame::XLoadable > xLoadable( xDocument, uno::UNO_QUERY );
	if ( !xLoadable.is() )
		throw uno::RuntimeException();

	sal_Int32 nLen = 2;
	uno::Sequence< beans::PropertyValue > aArgs( nLen );
	aArgs[0].Name = ::rtl::OUString::createFromAscii( "URL" );
	aArgs[0].Value <<= m_aLinkURL;
	aArgs[1].Name = ::rtl::OUString::createFromAscii( "FilterName" );
	aArgs[1].Value <<= m_aLinkFilterName;
	if ( m_bLinkHasPassword )
	{
		aArgs.realloc( ++nLen );
		aArgs[nLen-1].Name = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "Password" ) );
		aArgs[nLen-1].Value <<= m_aLinkPassword;
	}

	aArgs.realloc( m_aDocMediaDescriptor.getLength() + nLen );
	for ( sal_Int32 nInd = 0; nInd < m_aDocMediaDescriptor.getLength(); nInd++ )
	{
		aArgs[nInd+nLen].Name = m_aDocMediaDescriptor[nInd].Name;
		aArgs[nInd+nLen].Value = m_aDocMediaDescriptor[nInd].Value;
	}

	try
	{
		// the document is not really an embedded one, it is a link
        SetDocToEmbedded( uno::Reference < frame::XModel >( xDocument, uno::UNO_QUERY ), m_aModuleName );

        try
        {
            uno::Reference < container::XChild > xChild( xDocument, uno::UNO_QUERY );
            if ( xChild.is() )
                xChild->setParent( m_xParent );
        }
        catch( const lang::NoSupportException & )
        {
            OSL_ENSURE( false, "Cannot set parent at document" );
        }

		// load the document
		xLoadable->load( aArgs );

		if ( !m_bLinkHasPassword )
		{
			// check if there is a password to cache
			uno::Reference< frame::XModel > xModel( xLoadable, uno::UNO_QUERY_THROW );
			uno::Sequence< beans::PropertyValue > aProps = xModel->getArgs();
			for ( sal_Int32 nInd = 0; nInd < aProps.getLength(); nInd++ )
				if ( aProps[nInd].Name.equals( ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "Password" ) ) )
				  && ( aProps[nInd].Value >>= m_aLinkPassword ) )
				{
					m_bLinkHasPassword = sal_True;
					break;
				}
		}
	}
	catch( uno::Exception& )
	{
		uno::Reference< util::XCloseable > xCloseable( xDocument, uno::UNO_QUERY );
		if ( xCloseable.is() )
		{
			try
			{
				xCloseable->close( sal_True );
			}
			catch( uno::Exception& )
			{
			}
		}

		throw; // TODO
	}

	return xDocument;

}

//------------------------------------------------------
::rtl::OUString OCommonEmbeddedObject::GetFilterName( sal_Int32 nVersion )
{
    ::rtl::OUString aFilterName = GetPresetFilterName();
    if ( !aFilterName.getLength() )
    {
        try {
	        ::comphelper::MimeConfigurationHelper aHelper( m_xFactory );
            aFilterName = aHelper.GetDefaultFilterFromServiceName( GetDocumentServiceName(), nVersion );
        } catch( uno::Exception& )
        {}
    }

    return aFilterName;
}

//------------------------------------------------------
uno::Reference< util::XCloseable > OCommonEmbeddedObject::LoadDocumentFromStorage_Impl(
															const uno::Reference< embed::XStorage >& xStorage )
{
	OSL_ENSURE( xStorage.is(), "The storage can not be empty!" );

    uno::Reference< util::XCloseable > xDocument( CreateDocument( m_xFactory, GetDocumentServiceName(),
                                                m_bEmbeddedScriptSupport ) );

    //#i103460# ODF: take the size given from the parent frame as default
    uno::Reference< chart2::XChartDocument > xChart( xDocument, uno::UNO_QUERY );
    if( xChart.is() )
    {
        uno::Reference< embed::XVisualObject > xChartVisualObject( xChart, uno::UNO_QUERY );
        if( xChartVisualObject.is() )
            xChartVisualObject->setVisualAreaSize( embed::Aspects::MSOLE_CONTENT, m_aDefaultSizeForChart_In_100TH_MM );
    }

    uno::Reference< frame::XLoadable > xLoadable( xDocument, uno::UNO_QUERY );
    uno::Reference< document::XStorageBasedDocument > xDoc
#ifdef USE_STORAGEBASED_DOCUMENT
            ( xDocument, uno::UNO_QUERY )
#endif
            ;
    if ( !xDoc.is() && !xLoadable.is() ) ///BUG: This should be || instead of && ?
		throw uno::RuntimeException();

	::rtl::OUString aFilterName = GetFilterName( ::comphelper::OStorageHelper::GetXStorageFormat( xStorage ) );

	OSL_ENSURE( aFilterName.getLength(), "Wrong document service name!" );
	if ( !aFilterName.getLength() )
		throw io::IOException();

    sal_Int32 nLen = xDoc.is() ? 4 : 6;
	uno::Sequence< beans::PropertyValue > aArgs( nLen );

    aArgs[0].Name = ::rtl::OUString::createFromAscii( "DocumentBaseURL" );
    aArgs[0].Value <<= GetBaseURL_Impl();
    aArgs[1].Name = ::rtl::OUString::createFromAscii( "HierarchicalDocumentName" );
    aArgs[1].Value <<= m_aEntryName;
	aArgs[2].Name = ::rtl::OUString::createFromAscii( "ReadOnly" );
	aArgs[2].Value <<= m_bReadOnly;
	aArgs[3].Name = ::rtl::OUString::createFromAscii( "FilterName" );
	aArgs[3].Value <<= aFilterName;

	uno::Reference< io::XInputStream > xTempInpStream;
    if ( !xDoc.is() )
    {
        xTempInpStream = createTempInpStreamFromStor( xStorage, m_xFactory );
        if ( !xTempInpStream.is() )
            throw uno::RuntimeException();

		::rtl::OUString aTempFileURL;
		try
		{
			// no need to let the file stay after the stream is removed since the embedded document
			// can not be stored directly
			uno::Reference< beans::XPropertySet > xTempStreamProps( xTempInpStream, uno::UNO_QUERY_THROW );
			xTempStreamProps->getPropertyValue( ::rtl::OUString::createFromAscii( "Uri" ) ) >>= aTempFileURL;
		}
		catch( uno::Exception& )
		{
		}

		OSL_ENSURE( aTempFileURL.getLength(), "Coudn't retrieve temporary file URL!\n" );

        aArgs[4].Name = ::rtl::OUString::createFromAscii( "URL" );
        aArgs[4].Value <<= aTempFileURL; // ::rtl::OUString::createFromAscii( "private:stream" );
        aArgs[5].Name = ::rtl::OUString::createFromAscii( "InputStream" );
        aArgs[5].Value <<= xTempInpStream;
    }

	// aArgs[4].Name = ::rtl::OUString::createFromAscii( "AsTemplate" );
	// aArgs[4].Value <<= sal_True;

	aArgs.realloc( m_aDocMediaDescriptor.getLength() + nLen );
	for ( sal_Int32 nInd = 0; nInd < m_aDocMediaDescriptor.getLength(); nInd++ )
	{
		aArgs[nInd+nLen].Name = m_aDocMediaDescriptor[nInd].Name;
		aArgs[nInd+nLen].Value = m_aDocMediaDescriptor[nInd].Value;
	}

	try
	{
		// set the document mode to embedded as the first step!!!
        SetDocToEmbedded( uno::Reference < frame::XModel >( xDocument, uno::UNO_QUERY ), m_aModuleName );

        try
        {
            uno::Reference < container::XChild > xChild( xDocument, uno::UNO_QUERY );
            if ( xChild.is() )
                xChild->setParent( m_xParent );
        }
        catch( const lang::NoSupportException & )
        {
            OSL_ENSURE( false, "Cannot set parent at document" );
        }

        if ( xDoc.is() )
            xDoc->loadFromStorage( xStorage, aArgs );
        else
            xLoadable->load( aArgs );
	}
	catch( uno::Exception& )
	{
		uno::Reference< util::XCloseable > xCloseable( xDocument, uno::UNO_QUERY );
		if ( xCloseable.is() )
		{
			try
			{
				xCloseable->close( sal_True );
			}
			catch( uno::Exception& )
			{
			}
		}

		throw; // TODO
	}

	return xDocument;
}

//------------------------------------------------------
uno::Reference< io::XInputStream > OCommonEmbeddedObject::StoreDocumentToTempStream_Impl(
																			sal_Int32 nStorageFormat,
																			const ::rtl::OUString& aBaseURL,
																			const ::rtl::OUString& aHierarchName )
{
	uno::Reference < io::XOutputStream > xTempOut(
				m_xFactory->createInstance ( ::rtl::OUString::createFromAscii( "com.sun.star.io.TempFile" ) ),
				uno::UNO_QUERY );
	uno::Reference< io::XInputStream > aResult( xTempOut, uno::UNO_QUERY );

	if ( !xTempOut.is() || !aResult.is() )
		throw uno::RuntimeException(); // TODO:

    uno::Reference< frame::XStorable > xStorable;
	{
		osl::MutexGuard aGuard( m_aMutex );
		if ( m_pDocHolder )
			xStorable = uno::Reference< frame::XStorable > ( m_pDocHolder->GetComponent(), uno::UNO_QUERY );
	}

	if( !xStorable.is() )
		throw uno::RuntimeException(); // TODO:

	::rtl::OUString aFilterName = GetFilterName( nStorageFormat );

	OSL_ENSURE( aFilterName.getLength(), "Wrong document service name!" );
	if ( !aFilterName.getLength() )
		throw io::IOException(); // TODO:

	uno::Sequence< beans::PropertyValue > aArgs( 4 );
	aArgs[0].Name = ::rtl::OUString::createFromAscii( "FilterName" );
	aArgs[0].Value <<= aFilterName;
	aArgs[1].Name = ::rtl::OUString::createFromAscii( "OutputStream" );
	aArgs[1].Value <<= xTempOut;
	aArgs[2].Name = ::rtl::OUString::createFromAscii( "DocumentBaseURL" );
	aArgs[2].Value <<= aBaseURL;
	aArgs[3].Name = ::rtl::OUString::createFromAscii( "HierarchicalDocumentName" );
	aArgs[3].Value <<= aHierarchName;

	xStorable->storeToURL( ::rtl::OUString::createFromAscii( "private:stream" ), aArgs );
	try
	{
		xTempOut->closeOutput();
	}
	catch( uno::Exception& )
	{
		OSL_ENSURE( sal_False, "Looks like stream was closed already" );
	}

	return aResult;
}

//------------------------------------------------------
void OCommonEmbeddedObject::SaveObject_Impl()
{
	if ( m_xClientSite.is() )
	{
		try
		{
			// check whether the component is modified,
			// if not there is no need for storing
    		uno::Reference< util::XModifiable > xModifiable( m_pDocHolder->GetComponent(), uno::UNO_QUERY );
			if ( xModifiable.is() && !xModifiable->isModified() )
				return;
		}
		catch( uno::Exception& )
		{}

		try {
			m_xClientSite->saveObject();
		}
		catch( uno::Exception& )
		{
			OSL_ENSURE( sal_False, "The object was not stored!\n" );
		}
	}
}

//------------------------------------------------------
::rtl::OUString OCommonEmbeddedObject::GetBaseURL_Impl()
{
	::rtl::OUString aBaseURL;
	sal_Int32 nInd = 0;

	if ( m_xClientSite.is() )
	{
		try
		{
			uno::Reference< frame::XModel > xParentModel( m_xClientSite->getComponent(), uno::UNO_QUERY_THROW );
			uno::Sequence< beans::PropertyValue > aModelProps = xParentModel->getArgs();
			for ( nInd = 0; nInd < aModelProps.getLength(); nInd++ )
				if ( aModelProps[nInd].Name.equals(
												::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "DocumentBaseURL" ) ) ) )
				{
					aModelProps[nInd].Value >>= aBaseURL;
					break;
				}


		}
		catch( uno::Exception& )
		{}
	}

	if ( !aBaseURL.getLength() )
	{
		for ( nInd = 0; nInd < m_aDocMediaDescriptor.getLength(); nInd++ )
			if ( m_aDocMediaDescriptor[nInd].Name.equals(
												::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "DocumentBaseURL" ) ) ) )
			{
				m_aDocMediaDescriptor[nInd].Value >>= aBaseURL;
				break;
			}
	}

	if ( !aBaseURL.getLength() )
		aBaseURL = m_aDefaultParentBaseURL;

	return aBaseURL;
}

//------------------------------------------------------
::rtl::OUString OCommonEmbeddedObject::GetBaseURLFrom_Impl(
					const uno::Sequence< beans::PropertyValue >& lArguments,
					const uno::Sequence< beans::PropertyValue >& lObjArgs )
{
	::rtl::OUString aBaseURL;
	sal_Int32 nInd = 0;

	for ( nInd = 0; nInd < lArguments.getLength(); nInd++ )
		if ( lArguments[nInd].Name.equals( ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "DocumentBaseURL" ) ) ) )
		{
			lArguments[nInd].Value >>= aBaseURL;
			break;
		}

	if ( !aBaseURL.getLength() )
	{
		for ( nInd = 0; nInd < lObjArgs.getLength(); nInd++ )
			if ( lObjArgs[nInd].Name.equals( ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "DefaultParentBaseURL" ) ) ) )
			{
				lObjArgs[nInd].Value >>= aBaseURL;
				break;
			}
	}

	return aBaseURL;
}


//------------------------------------------------------
void OCommonEmbeddedObject::StoreDocToStorage_Impl( const uno::Reference< embed::XStorage >& xStorage,
													sal_Int32 nStorageFormat,
													const ::rtl::OUString& aBaseURL,
													const ::rtl::OUString& aHierarchName,
													sal_Bool bAttachToTheStorage )
{
	OSL_ENSURE( xStorage.is(), "No storage is provided for storing!" );

	if ( !xStorage.is() )
		throw uno::RuntimeException(); // TODO:

#ifdef USE_STORAGEBASED_DOCUMENT
    uno::Reference< document::XStorageBasedDocument > xDoc;
	{
		osl::MutexGuard aGuard( m_aMutex );
		if ( m_pDocHolder )
			xDoc = uno::Reference< document::XStorageBasedDocument >( m_pDocHolder->GetComponent(), uno::UNO_QUERY );
	}

    if ( xDoc.is() )
    {
	    ::rtl::OUString aFilterName = GetFilterName( nStorageFormat );

        OSL_ENSURE( aFilterName.getLength(), "Wrong document service name!" );
        if ( !aFilterName.getLength() )
            throw io::IOException(); // TODO:

        uno::Sequence< beans::PropertyValue > aArgs( 3 );
        aArgs[0].Name = ::rtl::OUString::createFromAscii( "FilterName" );
        aArgs[0].Value <<= aFilterName;
        aArgs[2].Name = ::rtl::OUString::createFromAscii( "DocumentBaseURL" );
        aArgs[2].Value <<= aBaseURL;
        aArgs[1].Name = ::rtl::OUString::createFromAscii( "HierarchicalDocumentName" );
        aArgs[1].Value <<= aHierarchName;

        xDoc->storeToStorage( xStorage, aArgs );
		if ( bAttachToTheStorage )
		{
			xDoc->switchToStorage( xStorage );
			uno::Reference< util::XModifiable > xModif( m_pDocHolder->GetComponent(), uno::UNO_QUERY );
			if ( xModif.is() )
				xModif->setModified( sal_False );
		}
    }
    else
#endif
    {
        // store document to temporary stream based on temporary file
        uno::Reference < io::XInputStream > xTempIn = StoreDocumentToTempStream_Impl( nStorageFormat, aBaseURL, aHierarchName );

        OSL_ENSURE( xTempIn.is(), "The stream reference can not be empty!\n" );

        // open storage based on document temporary file for reading
        uno::Reference < lang::XSingleServiceFactory > xStorageFactory(
                    m_xFactory->createInstance ( ::rtl::OUString::createFromAscii( "com.sun.star.embed.StorageFactory" ) ),
                    uno::UNO_QUERY );

        uno::Sequence< uno::Any > aArgs(1);
        aArgs[0] <<= xTempIn;
        uno::Reference< embed::XStorage > xTempStorage( xStorageFactory->createInstanceWithArguments( aArgs ),
                                                            uno::UNO_QUERY );
        if ( !xTempStorage.is() )
            throw uno::RuntimeException(); // TODO:

        // object storage must be commited automatically
        xTempStorage->copyToStorage( xStorage );
    }
}

//------------------------------------------------------
uno::Reference< util::XCloseable > OCommonEmbeddedObject::CreateDocFromMediaDescr_Impl(
										const uno::Sequence< beans::PropertyValue >& aMedDescr )
{
    uno::Reference< util::XCloseable > xDocument( CreateDocument( m_xFactory, GetDocumentServiceName(),
                                                m_bEmbeddedScriptSupport ) );

	uno::Reference< frame::XLoadable > xLoadable( xDocument, uno::UNO_QUERY );
	if ( !xLoadable.is() )
		throw uno::RuntimeException();

	try
	{
		// set the document mode to embedded as the first action on the document!!!
        SetDocToEmbedded( uno::Reference < frame::XModel >( xDocument, uno::UNO_QUERY ), m_aModuleName );

        try
        {
            uno::Reference < container::XChild > xChild( xDocument, uno::UNO_QUERY );
            if ( xChild.is() )
                xChild->setParent( m_xParent );
        }
        catch( const lang::NoSupportException & )
        {
            OSL_ENSURE( false, "Cannot set parent at document" );
        }

		xLoadable->load( addAsTemplate( aMedDescr ) );
	}
	catch( uno::Exception& )
	{
		uno::Reference< util::XCloseable > xCloseable( xDocument, uno::UNO_QUERY );
		if ( xCloseable.is() )
		{
			try
			{
				xCloseable->close( sal_True );
			}
			catch( uno::Exception& )
			{
			}
		}

		throw; // TODO
	}

	return xDocument;
}

//------------------------------------------------------
uno::Reference< util::XCloseable > OCommonEmbeddedObject::CreateTempDocFromLink_Impl()
{
    uno::Reference< util::XCloseable > xResult;

	OSL_ENSURE( m_bIsLink, "The object is not a linked one!\n" );

	uno::Sequence< beans::PropertyValue > aTempMediaDescr;

	sal_Int32 nStorageFormat = SOFFICE_FILEFORMAT_CURRENT;
	try {
		nStorageFormat = ::comphelper::OStorageHelper::GetXStorageFormat( m_xParentStorage );
	}
	catch ( beans::IllegalTypeException& )
	{
		// the container just has an unknown type, use current file format
	}
	catch ( uno::Exception& )
	{
		OSL_ENSURE( sal_False, "Can not retrieve storage media type!\n" );
	}

    if ( m_pDocHolder->GetComponent().is() )
	{
		aTempMediaDescr.realloc( 4 );

		// TODO/LATER: may be private:stream should be used as target URL
		::rtl::OUString aTempFileURL;
		uno::Reference< io::XInputStream > xTempStream = StoreDocumentToTempStream_Impl( SOFFICE_FILEFORMAT_CURRENT,
																						 ::rtl::OUString(),
																						 ::rtl::OUString() );
		try
		{
			// no need to let the file stay after the stream is removed since the embedded document
			// can not be stored directly
			uno::Reference< beans::XPropertySet > xTempStreamProps( xTempStream, uno::UNO_QUERY_THROW );
			xTempStreamProps->getPropertyValue( ::rtl::OUString::createFromAscii( "Uri" ) ) >>= aTempFileURL;
		}
		catch( uno::Exception& )
		{
		}

		OSL_ENSURE( aTempFileURL.getLength(), "Coudn't retrieve temporary file URL!\n" );

		aTempMediaDescr[0].Name = ::rtl::OUString::createFromAscii( "URL" );
		aTempMediaDescr[0].Value <<= aTempFileURL;
		aTempMediaDescr[1].Name = ::rtl::OUString::createFromAscii( "InputStream" );
		aTempMediaDescr[1].Value <<= xTempStream;
		aTempMediaDescr[2].Name = ::rtl::OUString::createFromAscii( "FilterName" );
		aTempMediaDescr[2].Value <<= GetFilterName( nStorageFormat );
		aTempMediaDescr[3].Name = ::rtl::OUString::createFromAscii( "AsTemplate" );
		aTempMediaDescr[3].Value <<= sal_True;
	}
	else
	{
		aTempMediaDescr.realloc( 2 );
		aTempMediaDescr[0].Name = ::rtl::OUString::createFromAscii( "URL" );
		aTempMediaDescr[0].Value <<= m_aLinkURL;
		aTempMediaDescr[1].Name = ::rtl::OUString::createFromAscii( "FilterName" );
		aTempMediaDescr[1].Value <<= m_aLinkFilterName;
		// aTempMediaDescr[2].Name = ::rtl::OUString::createFromAscii( "AsTemplate" );
		// aTempMediaDescr[2].Value <<= sal_True;
	}

	xResult = CreateDocFromMediaDescr_Impl( aTempMediaDescr );

	return xResult;
}

//------------------------------------------------------
void SAL_CALL OCommonEmbeddedObject::setPersistentEntry(
					const uno::Reference< embed::XStorage >& xStorage,
					const ::rtl::OUString& sEntName,
					sal_Int32 nEntryConnectionMode,
					const uno::Sequence< beans::PropertyValue >& lArguments,
					const uno::Sequence< beans::PropertyValue >& lObjArgs )
		throw ( lang::IllegalArgumentException,
				embed::WrongStateException,
				io::IOException,
				uno::Exception,
				uno::RuntimeException )
{
	RTL_LOGFILE_CONTEXT( aLog, "embeddedobj (mv76033) OCommonEmbeddedObject::setPersistentEntry" );

	// the type of the object must be already set
	// a kind of typedetection should be done in the factory

	::osl::MutexGuard aGuard( m_aMutex );
	if ( m_bDisposed )
		throw lang::DisposedException(); // TODO

	if ( !xStorage.is() )
		throw lang::IllegalArgumentException( ::rtl::OUString::createFromAscii( "No parent storage is provided!\n" ),
											uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ),
											1 );

	if ( !sEntName.getLength() )
		throw lang::IllegalArgumentException( ::rtl::OUString::createFromAscii( "Empty element name is provided!\n" ),
											uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ),
											2 );

	// May be LOADED should be forbidden here ???
	if ( ( m_nObjectState != -1 || nEntryConnectionMode == embed::EntryInitModes::NO_INIT )
	  && ( m_nObjectState == -1 || nEntryConnectionMode != embed::EntryInitModes::NO_INIT ) )
	{
		// if the object is not loaded
		// it can not get persistant representation without initialization

		// if the object is loaded
		// it can switch persistant representation only without initialization

		throw embed::WrongStateException(
					::rtl::OUString::createFromAscii( "Can't change persistant representation of activated object!\n" ),
					uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );
	}

    if ( m_bWaitSaveCompleted )
	{
		if ( nEntryConnectionMode == embed::EntryInitModes::NO_INIT )
			saveCompleted( ( m_xParentStorage != xStorage || !m_aEntryName.equals( sEntName ) ) );
		else
			throw embed::WrongStateException(
						::rtl::OUString::createFromAscii( "The object waits for saveCompleted() call!\n" ),
						uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );
	}

	// for now support of this interface is required to allow breaking of links and converting them to normal embedded
	// objects, so the persist name must be handled correctly ( althowgh no real persist entry is used )
	// OSL_ENSURE( !m_bIsLink, "This method implementation must not be used for links!\n" );
	if ( m_bIsLink )
	{
		m_aEntryName = sEntName;
		return;
	}

	uno::Reference< container::XNameAccess > xNameAccess( xStorage, uno::UNO_QUERY );
	if ( !xNameAccess.is() )
		throw uno::RuntimeException(); //TODO

	// detect entry existence
	sal_Bool bElExists = xNameAccess->hasByName( sEntName );

	m_aDocMediaDescriptor = GetValuableArgs_Impl( lArguments,
												  nEntryConnectionMode != embed::EntryInitModes::MEDIA_DESCRIPTOR_INIT );

	m_bReadOnly = sal_False;
	for ( sal_Int32 nInd = 0; nInd < lArguments.getLength(); nInd++ )
		if ( lArguments[nInd].Name.equalsAscii( "ReadOnly" ) )
			lArguments[nInd].Value >>= m_bReadOnly;

	// TODO: use lObjArgs for StoreVisualReplacement
	for ( sal_Int32 nObjInd = 0; nObjInd < lObjArgs.getLength(); nObjInd++ )
		if ( lObjArgs[nObjInd].Name.equalsAscii( "OutplaceDispatchInterceptor" ) )
		{
			uno::Reference< frame::XDispatchProviderInterceptor > xDispatchInterceptor;
			if ( lObjArgs[nObjInd].Value >>= xDispatchInterceptor )
				m_pDocHolder->SetOutplaceDispatchInterceptor( xDispatchInterceptor );
		}
		else if ( lObjArgs[nObjInd].Name.equalsAscii( "DefaultParentBaseURL" ) )
		{
			lObjArgs[nObjInd].Value >>= m_aDefaultParentBaseURL;
		}
        else if ( lObjArgs[nObjInd].Name.equalsAscii( "Parent" ) )
		{
            lObjArgs[nObjInd].Value >>= m_xParent;
		}
        else if ( lObjArgs[nObjInd].Name.equalsAscii( "IndividualMiscStatus" ) )
		{
            sal_Int64 nMiscStatus=0;
            lObjArgs[nObjInd].Value >>= nMiscStatus;
            m_nMiscStatus |= nMiscStatus;
		}
        else if ( lObjArgs[nObjInd].Name.equalsAscii( "CloneFrom" ) )
        {
            uno::Reference < embed::XEmbeddedObject > xObj;
            lObjArgs[nObjInd].Value >>= xObj;
            if ( xObj.is() )
            {
                m_bHasClonedSize = sal_True;
                m_aClonedSize = xObj->getVisualAreaSize( embed::Aspects::MSOLE_CONTENT );
                m_nClonedMapUnit = xObj->getMapUnit( embed::Aspects::MSOLE_CONTENT );
            }
        }
        else if ( lObjArgs[nObjInd].Name.equalsAscii( "OutplaceFrameProperties" ) )
		{
			uno::Sequence< uno::Any > aOutFrameProps;
            uno::Sequence< beans::NamedValue > aOutFramePropsTyped;
			if ( lObjArgs[nObjInd].Value >>= aOutFrameProps )
            {
				m_pDocHolder->SetOutplaceFrameProperties( aOutFrameProps );
            }
            else if ( lObjArgs[nObjInd].Value >>= aOutFramePropsTyped )
            {
                aOutFrameProps.realloc( aOutFramePropsTyped.getLength() );
                uno::Any* pProp = aOutFrameProps.getArray();
                for (   const beans::NamedValue* pTypedProp = aOutFramePropsTyped.getConstArray();
                        pTypedProp != aOutFramePropsTyped.getConstArray() + aOutFramePropsTyped.getLength();
                        ++pTypedProp, ++pProp
                    )
                {
                    *pProp <<= *pTypedProp;
                }
				m_pDocHolder->SetOutplaceFrameProperties( aOutFrameProps );
            }
            else
                OSL_ENSURE( false, "OCommonEmbeddedObject::setPersistentEntry: illegal type for argument 'OutplaceFrameProperties'!" );
		}
		else if ( lObjArgs[nObjInd].Name.equalsAscii( "ModuleName" ) )
		{
			lObjArgs[nObjInd].Value >>= m_aModuleName;
		}
		else if ( lObjArgs[nObjInd].Name.equalsAscii( "EmbeddedScriptSupport" ) )
        {
			OSL_VERIFY( lObjArgs[nObjInd].Value >>= m_bEmbeddedScriptSupport );
        }


	sal_Int32 nStorageMode = m_bReadOnly ? embed::ElementModes::READ : embed::ElementModes::READWRITE;

	SwitchOwnPersistence( xStorage, sEntName );

	if ( nEntryConnectionMode == embed::EntryInitModes::DEFAULT_INIT )
	{
		if ( bElExists )
		{
			// the initialization from existing storage allows to leave object in loaded state
			m_nObjectState = embed::EmbedStates::LOADED;
		}
		else
		{
            m_pDocHolder->SetComponent( InitNewDocument_Impl(), m_bReadOnly );
            if ( !m_pDocHolder->GetComponent().is() )
				throw io::IOException(); // TODO: can not create document

			m_nObjectState = embed::EmbedStates::RUNNING;
		}
	}
	else
	{
		if ( ( nStorageMode & embed::ElementModes::READWRITE ) != embed::ElementModes::READWRITE )
			throw io::IOException();

		if ( nEntryConnectionMode == embed::EntryInitModes::NO_INIT )
		{
			// the document just already changed its storage to store to
			// the links to OOo documents for now ignore this call
			// TODO: OOo links will have persistence so it will be switched here
		}
		else if ( nEntryConnectionMode == embed::EntryInitModes::TRUNCATE_INIT )
		{
			// TODO:
            m_pDocHolder->SetComponent( InitNewDocument_Impl(), m_bReadOnly );

            if ( !m_pDocHolder->GetComponent().is() )
				throw io::IOException(); // TODO: can not create document

			m_nObjectState = embed::EmbedStates::RUNNING;
		}
		else if ( nEntryConnectionMode == embed::EntryInitModes::MEDIA_DESCRIPTOR_INIT )
		{
            m_pDocHolder->SetComponent( CreateDocFromMediaDescr_Impl( lArguments ), m_bReadOnly );
			m_nObjectState = embed::EmbedStates::RUNNING;
		}
		//else if ( nEntryConnectionMode == embed::EntryInitModes::TRANSFERABLE_INIT )
		//{
			//TODO:
		//}
		else
			throw lang::IllegalArgumentException( ::rtl::OUString::createFromAscii( "Wrong connection mode is provided!\n" ),
										uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ),
										3 );
	}
}

//------------------------------------------------------
void SAL_CALL OCommonEmbeddedObject::storeToEntry( const uno::Reference< embed::XStorage >& xStorage,
							const ::rtl::OUString& sEntName,
							const uno::Sequence< beans::PropertyValue >& lArguments,
							const uno::Sequence< beans::PropertyValue >& lObjArgs )
		throw ( lang::IllegalArgumentException,
				embed::WrongStateException,
				io::IOException,
				uno::Exception,
				uno::RuntimeException )
{
	RTL_LOGFILE_CONTEXT( aLog, "embeddedobj (mv76033) OCommonEmbeddedObject::storeToEntry" );

	::osl::ResettableMutexGuard aGuard( m_aMutex );
	if ( m_bDisposed )
		throw lang::DisposedException(); // TODO

	if ( m_nObjectState == -1 )
	{
		// the object is still not loaded
		throw embed::WrongStateException( ::rtl::OUString::createFromAscii( "Can't store object without persistence!\n" ),
										uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );
	}

	if ( m_bWaitSaveCompleted )
		throw embed::WrongStateException(
					::rtl::OUString::createFromAscii( "The object waits for saveCompleted() call!\n" ),
					uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );

	// for now support of this interface is required to allow breaking of links and converting them to normal embedded
	// objects, so the persist name must be handled correctly ( althowgh no real persist entry is used )
	// OSL_ENSURE( !m_bIsLink, "This method implementation must not be used for links!\n" );
	if ( m_bIsLink )
		return;

	OSL_ENSURE( m_xParentStorage.is() && m_xObjectStorage.is(), "The object has no valid persistence!\n" );

	sal_Int32 nTargetStorageFormat = SOFFICE_FILEFORMAT_CURRENT;
	sal_Int32 nOriginalStorageFormat = SOFFICE_FILEFORMAT_CURRENT;
	try {
		nTargetStorageFormat = ::comphelper::OStorageHelper::GetXStorageFormat( xStorage );
	}
	catch ( beans::IllegalTypeException& )
	{
		// the container just has an unknown type, use current file format
	}
	catch ( uno::Exception& )
	{
		OSL_ENSURE( sal_False, "Can not retrieve target storage media type!\n" );
	}

	try
	{
		nOriginalStorageFormat = ::comphelper::OStorageHelper::GetXStorageFormat( m_xParentStorage );
	}
	catch ( beans::IllegalTypeException& )
	{
		// the container just has an unknown type, use current file format
	}
	catch ( uno::Exception& )
	{
		OSL_ENSURE( sal_False, "Can not retrieve own storage media type!\n" );
	}

	sal_Bool bTryOptimization = sal_False;
	for ( sal_Int32 nInd = 0; nInd < lObjArgs.getLength(); nInd++ )
	{
		// StoreVisualReplacement and VisualReplacement args have no sence here
		if ( lObjArgs[nInd].Name.equalsAscii( "CanTryOptimization" ) )
			lObjArgs[nInd].Value >>= bTryOptimization;
	}

	sal_Bool bSwitchBackToLoaded = sal_False;

	// Storing to different format can be done only in running state.
	if ( m_nObjectState == embed::EmbedStates::LOADED )
	{
		// TODO/LATER: copying is not legal for documents with relative links.
		if ( nTargetStorageFormat == nOriginalStorageFormat )
		{
			sal_Bool bOptimizationWorks = sal_False;
			if ( bTryOptimization )
			{
				try
				{
					// try to use optimized copying
					uno::Reference< embed::XOptimizedStorage > xSource( m_xParentStorage, uno::UNO_QUERY_THROW );
					uno::Reference< embed::XOptimizedStorage > xTarget( xStorage, uno::UNO_QUERY_THROW );
					xSource->copyElementDirectlyTo( m_aEntryName, xTarget, sEntName );
					bOptimizationWorks = sal_True;
				}
				catch( uno::Exception& )
				{
				}
			}

			if ( !bOptimizationWorks )
				m_xParentStorage->copyElementTo( m_aEntryName, xStorage, sEntName );
		}
		else
		{
			changeState( embed::EmbedStates::RUNNING );
			bSwitchBackToLoaded = sal_True;
		}
	}

	if ( m_nObjectState != embed::EmbedStates::LOADED )
	{
		uno::Reference< embed::XStorage > xSubStorage =
					xStorage->openStorageElement( sEntName, embed::ElementModes::READWRITE );

		if ( !xSubStorage.is() )
			throw uno::RuntimeException(); //TODO

		aGuard.clear();
		// TODO/LATER: support hierarchical name for embedded objects in embedded objects
		StoreDocToStorage_Impl( xSubStorage, nTargetStorageFormat, GetBaseURLFrom_Impl( lArguments, lObjArgs ), sEntName, sal_False );
		aGuard.reset();

		if ( bSwitchBackToLoaded )
			changeState( embed::EmbedStates::LOADED );
	}

	// TODO: should the listener notification be done?
}

//------------------------------------------------------
void SAL_CALL OCommonEmbeddedObject::storeAsEntry( const uno::Reference< embed::XStorage >& xStorage,
							const ::rtl::OUString& sEntName,
							const uno::Sequence< beans::PropertyValue >& lArguments,
							const uno::Sequence< beans::PropertyValue >& lObjArgs )
		throw ( lang::IllegalArgumentException,
				embed::WrongStateException,
				io::IOException,
				uno::Exception,
				uno::RuntimeException )
{
	RTL_LOGFILE_CONTEXT( aLog, "embeddedobj (mv76033) OCommonEmbeddedObject::storeAsEntry" );

	// TODO: use lObjArgs

	::osl::ResettableMutexGuard aGuard( m_aMutex );
	if ( m_bDisposed )
		throw lang::DisposedException(); // TODO

	if ( m_nObjectState == -1 )
	{
		// the object is still not loaded
		throw embed::WrongStateException( ::rtl::OUString::createFromAscii( "Can't store object without persistence!\n" ),
										uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );
	}

	if ( m_bWaitSaveCompleted )
		throw embed::WrongStateException(
					::rtl::OUString::createFromAscii( "The object waits for saveCompleted() call!\n" ),
					uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );

	// for now support of this interface is required to allow breaking of links and converting them to normal embedded
	// objects, so the persist name must be handled correctly ( althowgh no real persist entry is used )
	// OSL_ENSURE( !m_bIsLink, "This method implementation must not be used for links!\n" );
	if ( m_bIsLink )
	{
    	m_aNewEntryName = sEntName;
		return;
	}

	OSL_ENSURE( m_xParentStorage.is() && m_xObjectStorage.is(), "The object has no valid persistence!\n" );

	sal_Int32 nTargetStorageFormat = SOFFICE_FILEFORMAT_CURRENT;
	sal_Int32 nOriginalStorageFormat = SOFFICE_FILEFORMAT_CURRENT;
	try {
		nTargetStorageFormat = ::comphelper::OStorageHelper::GetXStorageFormat( xStorage );
	}
	catch ( beans::IllegalTypeException& )
	{
		// the container just has an unknown type, use current file format
	}
	catch ( uno::Exception& )
	{
		OSL_ENSURE( sal_False, "Can not retrieve target storage media type!\n" );
	}

	try
	{
		nOriginalStorageFormat = ::comphelper::OStorageHelper::GetXStorageFormat( m_xParentStorage );
	}
	catch ( beans::IllegalTypeException& )
	{
		// the container just has an unknown type, use current file format
	}
	catch ( uno::Exception& )
	{
		OSL_ENSURE( sal_False, "Can not retrieve own storage media type!\n" );
	}

	PostEvent_Impl( ::rtl::OUString::createFromAscii( "OnSaveAs" ) );

	sal_Bool bTryOptimization = sal_False;
	for ( sal_Int32 nInd = 0; nInd < lObjArgs.getLength(); nInd++ )
	{
		// StoreVisualReplacement and VisualReplacement args have no sence here
		if ( lObjArgs[nInd].Name.equalsAscii( "CanTryOptimization" ) )
			lObjArgs[nInd].Value >>= bTryOptimization;
	}

	sal_Bool bSwitchBackToLoaded = sal_False;

	// Storing to different format can be done only in running state.
	if ( m_nObjectState == embed::EmbedStates::LOADED )
	{
		// TODO/LATER: copying is not legal for documents with relative links.
		if ( nTargetStorageFormat == nOriginalStorageFormat )
		{
			sal_Bool bOptimizationWorks = sal_False;
			if ( bTryOptimization )
			{
				try
				{
					// try to use optimized copying
					uno::Reference< embed::XOptimizedStorage > xSource( m_xParentStorage, uno::UNO_QUERY_THROW );
					uno::Reference< embed::XOptimizedStorage > xTarget( xStorage, uno::UNO_QUERY_THROW );
					xSource->copyElementDirectlyTo( m_aEntryName, xTarget, sEntName );
					bOptimizationWorks = sal_True;
				}
				catch( uno::Exception& )
				{
				}
			}

			if ( !bOptimizationWorks )
				m_xParentStorage->copyElementTo( m_aEntryName, xStorage, sEntName );
		}
		else
		{
			changeState( embed::EmbedStates::RUNNING );
			bSwitchBackToLoaded = sal_True;
		}
	}

	uno::Reference< embed::XStorage > xSubStorage =
				xStorage->openStorageElement( sEntName, embed::ElementModes::READWRITE );

	if ( !xSubStorage.is() )
		throw uno::RuntimeException(); //TODO

	if ( m_nObjectState != embed::EmbedStates::LOADED )
	{
		aGuard.clear();
		// TODO/LATER: support hierarchical name for embedded objects in embedded objects
		StoreDocToStorage_Impl( xSubStorage, nTargetStorageFormat, GetBaseURLFrom_Impl( lArguments, lObjArgs ), sEntName, sal_False );
		aGuard.reset();

		if ( bSwitchBackToLoaded )
			changeState( embed::EmbedStates::LOADED );
	}

	m_bWaitSaveCompleted = sal_True;
	m_xNewObjectStorage = xSubStorage;
	m_xNewParentStorage = xStorage;
    m_aNewEntryName = sEntName;
	m_aNewDocMediaDescriptor = GetValuableArgs_Impl( lArguments, sal_True );

	// TODO: register listeners for storages above, in case thay are disposed
	// 		 an exception will be thrown on saveCompleted( true )

	// TODO: should the listener notification be done here or in saveCompleted?
}

//------------------------------------------------------
void SAL_CALL OCommonEmbeddedObject::saveCompleted( sal_Bool bUseNew )
		throw ( embed::WrongStateException,
				uno::Exception,
				uno::RuntimeException )
{
	RTL_LOGFILE_CONTEXT( aLog, "embeddedobj (mv76033) OCommonEmbeddedObject::saveCompleted" );

	::osl::MutexGuard aGuard( m_aMutex );
	if ( m_bDisposed )
		throw lang::DisposedException(); // TODO

	if ( m_nObjectState == -1 )
	{
		// the object is still not loaded
		throw embed::WrongStateException( ::rtl::OUString::createFromAscii( "Can't store object without persistence!\n" ),
										uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );
	}

	// for now support of this interface is required to allow breaking of links and converting them to normal embedded
	// objects, so the persist name must be handled correctly ( althowgh no real persist entry is used )
	// OSL_ENSURE( !m_bIsLink, "This method implementation must not be used for links!\n" );
	if ( m_bIsLink )
	{
		if ( bUseNew )
			m_aEntryName = m_aNewEntryName;
		m_aNewEntryName = ::rtl::OUString();
		return;
	}

	// it is allowed to call saveCompleted( false ) for nonstored objects
	if ( !m_bWaitSaveCompleted && !bUseNew )
		return;

	OSL_ENSURE( m_bWaitSaveCompleted, "Unexpected saveCompleted() call!\n" );
	if ( !m_bWaitSaveCompleted )
		throw io::IOException(); // TODO: illegal call

	OSL_ENSURE( m_xNewObjectStorage.is() && m_xNewParentStorage.is() , "Internal object information is broken!\n" );
	if ( !m_xNewObjectStorage.is() || !m_xNewParentStorage.is() )
		throw uno::RuntimeException(); // TODO: broken internal information

	if ( bUseNew )
	{
		SwitchOwnPersistence( m_xNewParentStorage, m_xNewObjectStorage, m_aNewEntryName );
		m_aDocMediaDescriptor = m_aNewDocMediaDescriptor;

		uno::Reference< util::XModifiable > xModif( m_pDocHolder->GetComponent(), uno::UNO_QUERY );
		if ( xModif.is() )
			xModif->setModified( sal_False );

		PostEvent_Impl( ::rtl::OUString::createFromAscii( "OnSaveAsDone" ) );
	}
	else
	{
		try {
			uno::Reference< lang::XComponent > xComponent( m_xNewObjectStorage, uno::UNO_QUERY );
			OSL_ENSURE( xComponent.is(), "Wrong storage implementation!" );
			if ( xComponent.is() )
				xComponent->dispose();
		}
		catch ( uno::Exception& )
		{
		}
	}

	m_xNewObjectStorage = uno::Reference< embed::XStorage >();
	m_xNewParentStorage = uno::Reference< embed::XStorage >();
	m_aNewEntryName = ::rtl::OUString();
	m_aNewDocMediaDescriptor.realloc( 0 );
	m_bWaitSaveCompleted = sal_False;

	if ( bUseNew )
	{
		// TODO: notify listeners

		if ( m_nUpdateMode == embed::EmbedUpdateModes::ALWAYS_UPDATE )
		{
			// TODO: update visual representation
		}
	}
}

//------------------------------------------------------
sal_Bool SAL_CALL OCommonEmbeddedObject::hasEntry()
		throw ( embed::WrongStateException,
				uno::RuntimeException )
{
	::osl::MutexGuard aGuard( m_aMutex );
	if ( m_bDisposed )
		throw lang::DisposedException(); // TODO

	if ( m_bWaitSaveCompleted )
		throw embed::WrongStateException(
					::rtl::OUString::createFromAscii( "The object waits for saveCompleted() call!\n" ),
					uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );

	if ( m_xObjectStorage.is() )
		return sal_True;

	return sal_False;
}

//------------------------------------------------------
::rtl::OUString SAL_CALL OCommonEmbeddedObject::getEntryName()
		throw ( embed::WrongStateException,
				uno::RuntimeException )
{
	::osl::MutexGuard aGuard( m_aMutex );
	if ( m_bDisposed )
		throw lang::DisposedException(); // TODO

	if ( m_nObjectState == -1 )
	{
		// the object is still not loaded
		throw embed::WrongStateException( ::rtl::OUString::createFromAscii( "The object persistence is not initialized!\n" ),
										uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );
	}

	if ( m_bWaitSaveCompleted )
		throw embed::WrongStateException(
					::rtl::OUString::createFromAscii( "The object waits for saveCompleted() call!\n" ),
					uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );

	return m_aEntryName;
}

//------------------------------------------------------
void SAL_CALL OCommonEmbeddedObject::storeOwn()
		throw ( embed::WrongStateException,
				io::IOException,
				uno::Exception,
				uno::RuntimeException )
{
	RTL_LOGFILE_CONTEXT( aLog, "embeddedobj (mv76033) OCommonEmbeddedObject::storeOwn" );

	// during switching from Activated to Running and from Running to Loaded states the object will
	// ask container to store the object, the container has to make decision
	// to do so or not

	::osl::ResettableMutexGuard aGuard( m_aMutex );
	if ( m_bDisposed )
		throw lang::DisposedException(); // TODO

	if ( m_nObjectState == -1 )
	{
		// the object is still not loaded
		throw embed::WrongStateException( ::rtl::OUString::createFromAscii( "Can't store object without persistence!\n" ),
									uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );
	}

	if ( m_bWaitSaveCompleted )
		throw embed::WrongStateException(
					::rtl::OUString::createFromAscii( "The object waits for saveCompleted() call!\n" ),
					uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );

	if ( m_bReadOnly )
		throw io::IOException(); // TODO: access denied

	// nothing to do, if the object is in loaded state
	if ( m_nObjectState == embed::EmbedStates::LOADED )
		return;

	PostEvent_Impl( ::rtl::OUString::createFromAscii( "OnSave" ) );

    OSL_ENSURE( m_pDocHolder->GetComponent().is(), "If an object is activated or in running state it must have a document!\n" );
    if ( !m_pDocHolder->GetComponent().is() )
		throw uno::RuntimeException();

	if ( m_bIsLink )
	{
		// TODO: just store the document to it's location
		uno::Reference< frame::XStorable > xStorable( m_pDocHolder->GetComponent(), uno::UNO_QUERY );
		if ( !xStorable.is() )
			throw uno::RuntimeException(); // TODO

		// free the main mutex for the storing time
		aGuard.clear();

		xStorable->store();

		aGuard.reset();
	}
	else
	{
		OSL_ENSURE( m_xParentStorage.is() && m_xObjectStorage.is(), "The object has no valid persistence!\n" );

		if ( !m_xObjectStorage.is() )
			throw io::IOException(); //TODO: access denied

		sal_Int32 nStorageFormat = SOFFICE_FILEFORMAT_CURRENT;
		try {
			nStorageFormat = ::comphelper::OStorageHelper::GetXStorageFormat( m_xParentStorage );
		}
		catch ( beans::IllegalTypeException& )
		{
			// the container just has an unknown type, use current file format
		}
		catch ( uno::Exception& )
		{
			OSL_ENSURE( sal_False, "Can not retrieve storage media type!\n" );
		}

		aGuard.clear();
		StoreDocToStorage_Impl( m_xObjectStorage, nStorageFormat, GetBaseURL_Impl(), m_aEntryName, sal_True );
		aGuard.reset();
	}

	uno::Reference< util::XModifiable > xModif( m_pDocHolder->GetComponent(), uno::UNO_QUERY );
	if ( xModif.is() )
		xModif->setModified( sal_False );

	PostEvent_Impl( ::rtl::OUString::createFromAscii( "OnSaveDone" ) );
}

//------------------------------------------------------
sal_Bool SAL_CALL OCommonEmbeddedObject::isReadonly()
		throw ( embed::WrongStateException,
				uno::RuntimeException )
{
	::osl::MutexGuard aGuard( m_aMutex );
	if ( m_bDisposed )
		throw lang::DisposedException(); // TODO

	if ( m_nObjectState == -1 )
	{
		// the object is still not loaded
		throw embed::WrongStateException( ::rtl::OUString::createFromAscii( "The object persistence is not initialized!\n" ),
										uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );
	}

	if ( m_bWaitSaveCompleted )
		throw embed::WrongStateException(
					::rtl::OUString::createFromAscii( "The object waits for saveCompleted() call!\n" ),
					uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );

	return m_bReadOnly;
}

//------------------------------------------------------
void SAL_CALL OCommonEmbeddedObject::reload(
				const uno::Sequence< beans::PropertyValue >& lArguments,
				const uno::Sequence< beans::PropertyValue >& lObjArgs )
		throw ( lang::IllegalArgumentException,
				embed::WrongStateException,
				io::IOException,
				uno::Exception,
				uno::RuntimeException )
{
	// TODO: use lObjArgs
	// for now this method is used only to switch readonly state

	::osl::MutexGuard aGuard( m_aMutex );
	if ( m_bDisposed )
		throw lang::DisposedException(); // TODO

	if ( m_nObjectState == -1 )
	{
		// the object is still not loaded
		throw embed::WrongStateException( ::rtl::OUString::createFromAscii( "The object persistence is not initialized!\n" ),
										uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );
	}

	if ( m_nObjectState != embed::EmbedStates::LOADED )
	{
		// the object is still not loaded
		throw embed::WrongStateException(
								::rtl::OUString::createFromAscii( "The object must be in loaded state to be reloaded!\n" ),
								uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );
	}

	if ( m_bWaitSaveCompleted )
		throw embed::WrongStateException(
					::rtl::OUString::createFromAscii( "The object waits for saveCompleted() call!\n" ),
					uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );

	if ( m_bIsLink )
	{
		// reload of the link
		::rtl::OUString aOldLinkFilter = m_aLinkFilterName;

		::rtl::OUString aNewLinkFilter;
		for ( sal_Int32 nInd = 0; nInd < lArguments.getLength(); nInd++ )
		{
			if ( lArguments[nInd].Name.equalsAscii( "URL" ) )
			{
				// the new URL
				lArguments[nInd].Value >>= m_aLinkURL;
				m_aLinkFilterName = ::rtl::OUString();
			}
			else if ( lArguments[nInd].Name.equalsAscii( "FilterName" ) )
			{
				lArguments[nInd].Value >>= aNewLinkFilter;
				m_aLinkFilterName = ::rtl::OUString();
			}
		}

		::comphelper::MimeConfigurationHelper aHelper( m_xFactory );
		if ( !m_aLinkFilterName.getLength() )
		{
			if ( aNewLinkFilter.getLength() )
				m_aLinkFilterName = aNewLinkFilter;
			else
			{
				uno::Sequence< beans::PropertyValue > aArgs( 1 );
				aArgs[0].Name = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "URL" ) );
				aArgs[0].Value <<= m_aLinkURL;
				m_aLinkFilterName = aHelper.UpdateMediaDescriptorWithFilterName( aArgs, sal_False );
			}
		}

		if ( !aOldLinkFilter.equals( m_aLinkFilterName ) )
		{
			uno::Sequence< beans::NamedValue > aObject = aHelper.GetObjectPropsByFilter( m_aLinkFilterName );

			// TODO/LATER: probably the document holder could be cleaned explicitly as in the destructor
			m_pDocHolder->release();
			m_pDocHolder = NULL;

			LinkInit_Impl( aObject, lArguments, lObjArgs );
		}
	}
	
	m_aDocMediaDescriptor = GetValuableArgs_Impl( lArguments, sal_True );

	// TODO: use lObjArgs for StoreVisualReplacement
	for ( sal_Int32 nObjInd = 0; nObjInd < lObjArgs.getLength(); nObjInd++ )
		if ( lObjArgs[nObjInd].Name.equalsAscii( "OutplaceDispatchInterceptor" ) )
		{
			uno::Reference< frame::XDispatchProviderInterceptor > xDispatchInterceptor;
			if ( lObjArgs[nObjInd].Value >>= xDispatchInterceptor )
				m_pDocHolder->SetOutplaceDispatchInterceptor( xDispatchInterceptor );

			break;
		}

	// TODO:
	// when document allows reloading through API the object can be reloaded not only in loaded state

	sal_Bool bOldReadOnlyValue = m_bReadOnly;

	m_bReadOnly = sal_False;
	for ( sal_Int32 nInd = 0; nInd < lArguments.getLength(); nInd++ )
		if ( lArguments[nInd].Name.equalsAscii( "ReadOnly" ) )
			lArguments[nInd].Value >>= m_bReadOnly;

	if ( bOldReadOnlyValue != m_bReadOnly && !m_bIsLink )
	{
		// close own storage
		try {
			uno::Reference< lang::XComponent > xComponent( m_xObjectStorage, uno::UNO_QUERY );
			OSL_ENSURE( !m_xObjectStorage.is() || xComponent.is(), "Wrong storage implementation!" );
			if ( xComponent.is() )
				xComponent->dispose();
		}
		catch ( uno::Exception& )
		{
		}

		sal_Int32 nStorageMode = m_bReadOnly ? embed::ElementModes::READ : embed::ElementModes::READWRITE;
		m_xObjectStorage = m_xParentStorage->openStorageElement( m_aEntryName, nStorageMode );
	}
}

//------------------------------------------------------
void SAL_CALL OCommonEmbeddedObject::breakLink( const uno::Reference< embed::XStorage >& xStorage,
												const ::rtl::OUString& sEntName )
		throw ( lang::IllegalArgumentException,
				embed::WrongStateException,
				io::IOException,
				uno::Exception,
				uno::RuntimeException )
{
	::osl::ResettableMutexGuard aGuard( m_aMutex );
	if ( m_bDisposed )
		throw lang::DisposedException(); // TODO

	if ( !m_bIsLink )
	{
		// it must be a linked initialized object
		throw embed::WrongStateException(
					::rtl::OUString::createFromAscii( "The object is not a valid linked object!\n" ),
					uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );
	}
#if 0
	else
	{
		// the current implementation of OOo links does not implement this method since it does not implement
		// all the set of interfaces required for OOo embedded object ( XEmbedPersist is not supported ).
		throw io::IOException(); // TODO:
	}
#endif

	if ( !xStorage.is() )
		throw lang::IllegalArgumentException( ::rtl::OUString::createFromAscii( "No parent storage is provided!\n" ),
											uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ),
											1 );

	if ( !sEntName.getLength() )
		throw lang::IllegalArgumentException( ::rtl::OUString::createFromAscii( "Empty element name is provided!\n" ),
											uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ),
											2 );

	if ( !m_bIsLink || m_nObjectState == -1 )
	{
		// it must be a linked initialized object
		throw embed::WrongStateException(
					::rtl::OUString::createFromAscii( "The object is not a valid linked object!\n" ),
					uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );
	}

	if ( m_bWaitSaveCompleted )
		throw embed::WrongStateException(
					::rtl::OUString::createFromAscii( "The object waits for saveCompleted() call!\n" ),
					uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );

	uno::Reference< container::XNameAccess > xNameAccess( xStorage, uno::UNO_QUERY );
	if ( !xNameAccess.is() )
		throw uno::RuntimeException(); //TODO

	// detect entry existence
	/*sal_Bool bElExists =*/ xNameAccess->hasByName( sEntName );

	m_bReadOnly = sal_False;
//	sal_Int32 nStorageMode = embed::ElementModes::READWRITE;

	if ( m_xParentStorage != xStorage || !m_aEntryName.equals( sEntName ) )
		SwitchOwnPersistence( xStorage, sEntName );

	// for linked object it means that it becomes embedded object
	// the document must switch it's persistence also

	// TODO/LATER: handle the case when temp doc can not be created
	// the document is a new embedded object so it must be marked as modified
    uno::Reference< util::XCloseable > xDocument = CreateTempDocFromLink_Impl();
    uno::Reference< util::XModifiable > xModif( m_pDocHolder->GetComponent(), uno::UNO_QUERY );
	if ( !xModif.is() )
		throw uno::RuntimeException();
	try
	{
		xModif->setModified( sal_True );
	}
	catch( uno::Exception& )
	{}

    m_pDocHolder->SetComponent( xDocument, m_bReadOnly );
    OSL_ENSURE( m_pDocHolder->GetComponent().is(), "If document cant be created, an exception must be thrown!\n" );

	if ( m_nObjectState == embed::EmbedStates::LOADED )
	{
		// the state is changed and can not be switched to loaded state back without saving
		m_nObjectState = embed::EmbedStates::RUNNING;
		StateChangeNotification_Impl( sal_False, embed::EmbedStates::LOADED, m_nObjectState, aGuard );
	}
	else if ( m_nObjectState == embed::EmbedStates::ACTIVE )
		m_pDocHolder->Show();

	m_bIsLink = sal_False;
	m_aLinkFilterName = ::rtl::OUString();
	m_aLinkURL = ::rtl::OUString();
}

//------------------------------------------------------
sal_Bool SAL_CALL  OCommonEmbeddedObject::isLink()
		throw ( embed::WrongStateException,
				uno::RuntimeException )
{
	::osl::MutexGuard aGuard( m_aMutex );
	if ( m_bDisposed )
		throw lang::DisposedException(); // TODO

	// Actually this information is clear even in case object is wayting for saveCompleted
	// if ( m_bWaitSaveCompleted )
	//	throw embed::WrongStateException(
	//				::rtl::OUString::createFromAscii( "The object waits for saveCompleted() call!\n" ),
	//				uno::Reference< uno::XInterface >( reinterpret_cast< ::cppu::OWeakObject* >(this) ) );

	return m_bIsLink;
}

//------------------------------------------------------
::rtl::OUString SAL_CALL OCommonEmbeddedObject::getLinkURL()
		throw ( embed::WrongStateException,
				uno::Exception,
				uno::RuntimeException )
{
	::osl::MutexGuard aGuard( m_aMutex );
	if ( m_bDisposed )
		throw lang::DisposedException(); // TODO

	// Actually this information is clear even in case object is wayting for saveCompleted
	// if ( m_bWaitSaveCompleted )
	// 	throw embed::WrongStateException(
	// 				::rtl::OUString::createFromAscii( "The object waits for saveCompleted() call!\n" ),
	// 				uno::Reference< uno::XInterface >( reinterpret_cast< ::cppu::OWeakObject* >(this) ) );

	if ( !m_bIsLink )
		throw embed::WrongStateException(
					::rtl::OUString::createFromAscii( "The object is not a link object!\n" ),
					uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );

	return m_aLinkURL;
}

