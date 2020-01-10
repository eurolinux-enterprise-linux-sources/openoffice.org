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
#include "precompiled_toolkit.hxx"


#include <toolkit/awt/vclxprinter.hxx>
#include <toolkit/helper/macros.hxx>
#include <cppuhelper/typeprovider.hxx>
#include <rtl/memory.h>
#include <rtl/uuid.h>


#include <vcl/print.hxx>
#include <vcl/jobset.hxx>
#include <vcl/svapp.hxx>

#include <tools/debug.hxx>
#include <tools/stream.hxx>

#include <toolkit/awt/vclxdevice.hxx>


#define BINARYSETUPMARKER	0x23864691

#define PROPERTY_Orientation	0
#define PROPERTY_Horizontal		1

::com::sun::star::beans::Property* ImplGetProperties( sal_uInt16& rElementCount )
{
	static ::com::sun::star::beans::Property* pProperties = NULL;
	static sal_uInt16 nElements = 0;
	if( !pProperties )
	{
		::osl::Guard< ::osl::Mutex > aGuard( ::osl::Mutex::getGlobalMutex() );
		if( !pProperties )
		{
			static ::com::sun::star::beans::Property __FAR_DATA aPropTable[] =
			{
				::com::sun::star::beans::Property( ::rtl::OUString::createFromAscii( "Orientation" ), PROPERTY_Orientation, ::getCppuType((const sal_Int16*)0), 0 ),
				::com::sun::star::beans::Property( ::rtl::OUString::createFromAscii( "Horizontal" ), PROPERTY_Horizontal, ::getBooleanCppuType(), 0 )
			};
			pProperties = aPropTable;
			nElements = sizeof( aPropTable ) / sizeof( ::com::sun::star::beans::Property );
		}
	}
	rElementCount = nElements;
	return pProperties;
}

//	----------------------------------------------------
//	class VCLXPrinterPropertySet
//	----------------------------------------------------

// ::com::sun::star::uno::XInterface
::com::sun::star::uno::Any VCLXPrinterPropertySet::queryInterface( const ::com::sun::star::uno::Type & rType ) throw(::com::sun::star::uno::RuntimeException)
{
	::com::sun::star::uno::Any aRet = ::cppu::queryInterface( rType,
										SAL_STATIC_CAST( ::com::sun::star::beans::XMultiPropertySet*, this ),
										SAL_STATIC_CAST( ::com::sun::star::beans::XFastPropertySet*, this ),
										SAL_STATIC_CAST( ::com::sun::star::beans::XPropertySet*, (::cppu::OPropertySetHelper*) this ),
										SAL_STATIC_CAST( ::com::sun::star::awt::XPrinterPropertySet*, this ),
										SAL_STATIC_CAST( ::com::sun::star::lang::XTypeProvider*, this ) );
	return (aRet.hasValue() ? aRet : OPropertySetHelper::queryInterface( rType ));
}

// ::com::sun::star::lang::XTypeProvider
IMPL_XTYPEPROVIDER_START( VCLXPrinterPropertySet )
	getCppuType( ( ::com::sun::star::uno::Reference< ::com::sun::star::beans::XMultiPropertySet>* ) NULL ),
	getCppuType( ( ::com::sun::star::uno::Reference< ::com::sun::star::beans::XFastPropertySet>* ) NULL ),
	getCppuType( ( ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySet>* ) NULL ),
	getCppuType( ( ::com::sun::star::uno::Reference< ::com::sun::star::awt::XPrinterPropertySet>* ) NULL )
IMPL_XTYPEPROVIDER_END

VCLXPrinterPropertySet::VCLXPrinterPropertySet( const String& rPrinterName )
	: OPropertySetHelper( BrdcstHelper )
{
	osl::Guard< vos::IMutex > aSolarGuard( Application::GetSolarMutex() );

    mpPrinter = new Printer( rPrinterName );
	mnOrientation = 0;
	mbHorizontal = sal_False;
}

VCLXPrinterPropertySet::~VCLXPrinterPropertySet()
{
	osl::Guard< vos::IMutex > aSolarGuard( Application::GetSolarMutex() );

 	delete mpPrinter;
}

::com::sun::star::uno::Reference< ::com::sun::star::awt::XDevice >  VCLXPrinterPropertySet::GetDevice()
{
	if ( !mxPrnDevice.is() ) 
	{
		VCLXDevice* pDev = new VCLXDevice;
		pDev->SetOutputDevice( GetPrinter() );
		mxPrnDevice = pDev;
	}
	return mxPrnDevice;
}

::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySetInfo > VCLXPrinterPropertySet::getPropertySetInfo(  ) throw(::com::sun::star::uno::RuntimeException)
{
	static ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySetInfo >  xInfo( createPropertySetInfo( getInfoHelper() ) );
	return xInfo;
}

::cppu::IPropertyArrayHelper& VCLXPrinterPropertySet::getInfoHelper()
{
	static ::cppu::OPropertyArrayHelper* pPropertyArrayHelper = NULL;
	if ( !pPropertyArrayHelper )
	{
		::osl::Guard< ::osl::Mutex > aGuard( ::osl::Mutex::getGlobalMutex() );
		if( !pPropertyArrayHelper )
		{
			sal_uInt16 nElements;
			::com::sun::star::beans::Property* pProps = ImplGetProperties( nElements );
			pPropertyArrayHelper = new ::cppu::OPropertyArrayHelper( pProps, nElements, sal_False );
		}
	}
	return *pPropertyArrayHelper ;
}

sal_Bool VCLXPrinterPropertySet::convertFastPropertyValue( ::com::sun::star::uno::Any & rConvertedValue, ::com::sun::star::uno::Any & rOldValue, sal_Int32 nHandle, const ::com::sun::star::uno::Any& rValue ) throw (::com::sun::star::lang::IllegalArgumentException)
{
	::osl::Guard< ::osl::Mutex > aGuard( Mutex );
	
	sal_Bool bDifferent = sal_False;
	switch ( nHandle ) 
	{
		case PROPERTY_Orientation:
		{
			sal_Int16 n;
			if( ( rValue >>= n ) && ( n != mnOrientation ) )
			{
				rConvertedValue <<= n;
				rOldValue <<= mnOrientation;
				bDifferent = sal_True;
			}
		}
		break;
		case PROPERTY_Horizontal:
		{
			sal_Bool b;
			if( ( rValue >>= b ) && ( b != mbHorizontal ) )
			{
				rConvertedValue <<= b;
				rOldValue <<= mbHorizontal;
				bDifferent = sal_True;
			}
		}
		break;
		default:
		{
			DBG_ERROR( "VCLXPrinterPropertySet_Impl::convertFastPropertyValue - invalid Handle" );
		}
	}
	return bDifferent;
}

void VCLXPrinterPropertySet::setFastPropertyValue_NoBroadcast( sal_Int32 nHandle, const ::com::sun::star::uno::Any& rValue ) throw (::com::sun::star::uno::Exception)
{
	::osl::Guard< ::osl::Mutex > aGuard( Mutex );
	
	switch( nHandle )
	{
		case PROPERTY_Orientation:
		{
			rValue >>= mnOrientation;
		}
		break;
		case PROPERTY_Horizontal:
		{
			rValue >>= mbHorizontal;
		}
		break;
		default:
		{
			DBG_ERROR( "VCLXPrinterPropertySet_Impl::convertFastPropertyValue - invalid Handle" );
		}
	}
}

void VCLXPrinterPropertySet::getFastPropertyValue( ::com::sun::star::uno::Any& rValue, sal_Int32 nHandle ) const
{
	::osl::Guard< ::osl::Mutex > aGuard( ((VCLXPrinterPropertySet*)this)->Mutex );
	
	switch( nHandle )
	{
		case PROPERTY_Orientation:
			rValue <<= mnOrientation;
		break;
		case PROPERTY_Horizontal:
			rValue <<= mbHorizontal;
		break;
		default:
		{
			DBG_ERROR( "VCLXPrinterPropertySet_Impl::convertFastPropertyValue - invalid Handle" );
		}
	}
}

// ::com::sun::star::awt::XPrinterPropertySet
void VCLXPrinterPropertySet::setHorizontal( sal_Bool bHorizontal ) throw(::com::sun::star::beans::PropertyVetoException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( Mutex );

	::com::sun::star::uno::Any aValue;
	aValue <<= bHorizontal;
	setFastPropertyValue( PROPERTY_Horizontal, aValue );
}

::com::sun::star::uno::Sequence< ::rtl::OUString > VCLXPrinterPropertySet::getFormDescriptions(  ) throw(::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( Mutex );
	
	sal_uInt16 nPaperBinCount = GetPrinter()->GetPaperBinCount();
	::com::sun::star::uno::Sequence< ::rtl::OUString >	aDescriptions( nPaperBinCount );
	for ( sal_uInt16 n = 0; n < nPaperBinCount; n++ ) 
	{
		// Format: <DisplayFormName;FormNameId;DisplayPaperBinName;PaperBinNameId;DisplayPaperName;PaperNameId>
		String aDescr( RTL_CONSTASCII_USTRINGPARAM( "*;*;" ) );
		aDescr += GetPrinter()->GetPaperBinName( n );
		aDescr += ';';
		aDescr += n;
		aDescr.AppendAscii( ";*;*", 4 );

		aDescriptions.getArray()[n] = aDescr;
	}
	return aDescriptions;
}

void VCLXPrinterPropertySet::selectForm( const ::rtl::OUString& rFormDescription ) throw(::com::sun::star::beans::PropertyVetoException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( Mutex );
	
    sal_Int32 nIndex = 0;
	sal_uInt16 nPaperBin = sal::static_int_cast< sal_uInt16 >(
        rFormDescription.getToken( 3, ';', nIndex ).toInt32());
	GetPrinter()->SetPaperBin( nPaperBin );
}

::com::sun::star::uno::Sequence< sal_Int8 > VCLXPrinterPropertySet::getBinarySetup(  ) throw(::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( Mutex );
	
	SvMemoryStream aMem;
	aMem << BINARYSETUPMARKER;
	aMem << GetPrinter()->GetJobSetup();
	return ::com::sun::star::uno::Sequence<sal_Int8>( (sal_Int8*) aMem.GetData(), aMem.Tell() );
}

void VCLXPrinterPropertySet::setBinarySetup( const ::com::sun::star::uno::Sequence< sal_Int8 >& data ) throw(::com::sun::star::beans::PropertyVetoException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( Mutex );
	
	SvMemoryStream aMem( (char*) data.getConstArray(), data.getLength(), STREAM_READ );
	sal_uInt32 nMarker;
	aMem >> nMarker;
	DBG_ASSERT( nMarker == BINARYSETUPMARKER, "setBinarySetup - invalid!" );
	if ( nMarker == BINARYSETUPMARKER ) 
	{
		JobSetup aSetup;
		aMem >> aSetup;
		GetPrinter()->SetJobSetup( aSetup );
	}
}


//	----------------------------------------------------
//	class VCLXPrinter
//	----------------------------------------------------
VCLXPrinter::VCLXPrinter( const String& rPrinterName ) 
	: VCLXPrinterPropertySet( rPrinterName )
{
}

VCLXPrinter::~VCLXPrinter()
{
}

// ::com::sun::star::uno::XInterface
::com::sun::star::uno::Any VCLXPrinter::queryInterface( const ::com::sun::star::uno::Type & rType ) throw(::com::sun::star::uno::RuntimeException)
{
	::com::sun::star::uno::Any aRet = ::cppu::queryInterface( rType,
										SAL_STATIC_CAST( ::com::sun::star::awt::XPrinter*, this ) );

    if ( !aRet.hasValue() )
        aRet = VCLXPrinterPropertySet::queryInterface( rType );

	return (aRet.hasValue() ? aRet : OWeakObject::queryInterface( rType ));
}

// ::com::sun::star::lang::XTypeProvider
IMPL_XTYPEPROVIDER_START( VCLXPrinter )
	getCppuType( ( ::com::sun::star::uno::Reference< ::com::sun::star::awt::XPrinter>* ) NULL ),
	VCLXPrinterPropertySet::getTypes()
IMPL_XTYPEPROVIDER_END

sal_Bool VCLXPrinter::start( const ::rtl::OUString& rJobName, sal_Int16 /*nCopies*/, sal_Bool /*bCollate*/ ) throw(::com::sun::star::awt::PrinterException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( Mutex );
	
	sal_Bool bDone = sal_True;
	if ( GetPrinter() )
		bDone = GetPrinter()->StartJob( rJobName );
	
	return bDone;
}

void VCLXPrinter::end(  ) throw(::com::sun::star::awt::PrinterException, ::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( Mutex );
	
	if ( GetPrinter() ) 
		GetPrinter()->EndJob();
}

void VCLXPrinter::terminate(  ) throw(::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( Mutex );
	
	if ( GetPrinter() ) 
		GetPrinter()->AbortJob();
}

::com::sun::star::uno::Reference< ::com::sun::star::awt::XDevice > VCLXPrinter::startPage(  ) throw(::com::sun::star::awt::PrinterException, ::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( Mutex );
	
	if ( GetPrinter() ) 
		GetPrinter()->StartPage();
	return GetDevice();
}

void VCLXPrinter::endPage(  ) throw(::com::sun::star::awt::PrinterException, ::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( Mutex );
	
	if ( GetPrinter() ) 
		GetPrinter()->EndPage();
}


//	----------------------------------------------------
//	class VCLXInfoPrinter
//	----------------------------------------------------

VCLXInfoPrinter::VCLXInfoPrinter( const String& rPrinterName ) 
	: VCLXPrinterPropertySet( rPrinterName )
{
}

VCLXInfoPrinter::~VCLXInfoPrinter()
{
}

// ::com::sun::star::uno::XInterface
::com::sun::star::uno::Any VCLXInfoPrinter::queryInterface( const ::com::sun::star::uno::Type & rType ) throw(::com::sun::star::uno::RuntimeException)
{
	::com::sun::star::uno::Any aRet = ::cppu::queryInterface( rType,
										SAL_STATIC_CAST( ::com::sun::star::awt::XInfoPrinter*, this ) );

    if ( !aRet.hasValue() )
        aRet = VCLXPrinterPropertySet::queryInterface( rType );

	return (aRet.hasValue() ? aRet : OWeakObject::queryInterface( rType ));
}

// ::com::sun::star::lang::XTypeProvider
IMPL_XTYPEPROVIDER_START( VCLXInfoPrinter )
	getCppuType( ( ::com::sun::star::uno::Reference< ::com::sun::star::awt::XInfoPrinter>* ) NULL ),
	VCLXPrinterPropertySet::getTypes()
IMPL_XTYPEPROVIDER_END

// ::com::sun::star::awt::XInfoPrinter
::com::sun::star::uno::Reference< ::com::sun::star::awt::XDevice > VCLXInfoPrinter::createDevice(  ) throw(::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( Mutex );
	
	return GetDevice();
}

//	----------------------------------------------------
//	class VCLXPrinterServer
//	----------------------------------------------------

// ::com::sun::star::uno::XInterface
::com::sun::star::uno::Any VCLXPrinterServer::queryInterface( const ::com::sun::star::uno::Type & rType ) throw(::com::sun::star::uno::RuntimeException)
{
	::com::sun::star::uno::Any aRet = ::cppu::queryInterface( rType,
										SAL_STATIC_CAST( ::com::sun::star::awt::XPrinterServer*, this ) );
	return (aRet.hasValue() ? aRet : OWeakObject::queryInterface( rType ));
}

// ::com::sun::star::lang::XTypeProvider
IMPL_XTYPEPROVIDER_START( VCLXPrinterServer )
	getCppuType( ( ::com::sun::star::uno::Reference< ::com::sun::star::awt::XPrinterServer>* ) NULL )
IMPL_XTYPEPROVIDER_END

// ::com::sun::star::awt::XPrinterServer
::com::sun::star::uno::Sequence< ::rtl::OUString > VCLXPrinterServer::getPrinterNames(  ) throw(::com::sun::star::uno::RuntimeException)
{
    const std::vector<rtl::OUString>& rQueues = Printer::GetPrinterQueues();
	sal_uInt32 nPrinters = rQueues.size();

	::com::sun::star::uno::Sequence< ::rtl::OUString >	aNames( nPrinters );
	for ( sal_uInt32 n = 0; n < nPrinters; n++ ) 
		aNames.getArray()[n] = rQueues[n];
	
	return aNames;
}

::com::sun::star::uno::Reference< ::com::sun::star::awt::XPrinter > VCLXPrinterServer::createPrinter( const ::rtl::OUString& rPrinterName ) throw(::com::sun::star::uno::RuntimeException)
{
	::com::sun::star::uno::Reference< ::com::sun::star::awt::XPrinter > xP;
	xP = new VCLXPrinter( rPrinterName );
	return xP;
}

::com::sun::star::uno::Reference< ::com::sun::star::awt::XInfoPrinter > VCLXPrinterServer::createInfoPrinter( const ::rtl::OUString& rPrinterName ) throw(::com::sun::star::uno::RuntimeException)
{
	::com::sun::star::uno::Reference< ::com::sun::star::awt::XInfoPrinter > xP;
	xP = new VCLXInfoPrinter( rPrinterName );
	return xP;
}



