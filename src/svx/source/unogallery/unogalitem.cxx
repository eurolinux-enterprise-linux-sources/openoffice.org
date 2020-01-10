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
#include "precompiled_svx.hxx"

#include "unogalitem.hxx"	
#include "unogaltheme.hxx"	
#include "galtheme.hxx"	
#include "galmisc.hxx"	
#include <svx/fmmodel.hxx>
#include <rtl/uuid.h>
#include <vos/mutex.hxx>
#ifndef _SV_SVAPP_HXX_ 
#include <vcl/svapp.hxx>
#endif
#ifndef _SV_GRAPH_HXX_ 
#include <vcl/graph.hxx>
#endif
#include <svtools/itemprop.hxx>
#include <svtools/itempool.hxx>


#ifndef _COM_SUN_STAR_BEANS_PROPERTYSTATE_HDL_ 
#include <com/sun/star/beans/PropertyState.hpp>
#endif
#ifndef _COM_SUN_STAR_BEANS_PROPERTYATTRIBUTE_HDL_ 
#include <com/sun/star/beans/PropertyAttribute.hpp>
#endif
#include <com/sun/star/gallery/GalleryItemType.hpp>

#define UNOGALLERY_GALLERYITEMTYPE	1
#define UNOGALLERY_URL				2
#define UNOGALLERY_TITLE			3
#define UNOGALLERY_THUMBNAIL		4
#define UNOGALLERY_GRAPHIC			5
#define UNOGALLERY_DRAWING			6

using namespace ::com::sun::star;

namespace unogallery {

// -----------------
// - GalleryItem -
// -----------------

GalleryItem::GalleryItem( ::unogallery::GalleryTheme& rTheme, const GalleryObject& rObject ) :
	::comphelper::PropertySetHelper( createPropertySetInfo() ),
	mpTheme( &rTheme ),
	mpGalleryObject( &rObject )
{
	mpTheme->implRegisterGalleryItem( *this );
}

// ------------------------------------------------------------------------------

GalleryItem::~GalleryItem()
	throw()
{
	if( mpTheme )
		mpTheme->implDeregisterGalleryItem( *this );
}

// ------------------------------------------------------------------------------

bool GalleryItem::isValid() const
{
	return( mpTheme != NULL );
}

// ------------------------------------------------------------------------------

uno::Any SAL_CALL GalleryItem::queryAggregation( const uno::Type & rType ) 
	throw( uno::RuntimeException )
{
	uno::Any aAny;

	if( rType == ::getCppuType((const uno::Reference< lang::XServiceInfo >*)0) )
		aAny <<= uno::Reference< lang::XServiceInfo >(this);
	else if( rType == ::getCppuType((const uno::Reference< lang::XTypeProvider >*)0) )
		aAny <<= uno::Reference< lang::XTypeProvider >(this);
	else if( rType == ::getCppuType((const uno::Reference< gallery::XGalleryItem >*)0) )
		aAny <<= uno::Reference< gallery::XGalleryItem >(this);
	else if( rType == ::getCppuType((const uno::Reference< beans::XPropertySet >*)0) )
		aAny <<= uno::Reference< beans::XPropertySet >(this);
	else if( rType == ::getCppuType((const uno::Reference< beans::XPropertyState >*)0) )
		aAny <<= uno::Reference< beans::XPropertyState >(this);
	else if( rType == ::getCppuType((const uno::Reference< beans::XMultiPropertySet >*)0) )
		aAny <<= uno::Reference< beans::XMultiPropertySet >(this);
	else
		aAny <<= OWeakAggObject::queryAggregation( rType );

	return aAny;
}

// ------------------------------------------------------------------------------

uno::Any SAL_CALL GalleryItem::queryInterface( const uno::Type & rType ) 
	throw( uno::RuntimeException )
{
	return OWeakAggObject::queryInterface( rType );
}

// ------------------------------------------------------------------------------

void SAL_CALL GalleryItem::acquire() 
	throw()
{
	OWeakAggObject::acquire();
}

// ------------------------------------------------------------------------------

void SAL_CALL GalleryItem::release()
	throw()
{
	OWeakAggObject::release();
}

// ------------------------------------------------------------------------------
	
::rtl::OUString GalleryItem::getImplementationName_Static()
	throw()
{
	return ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.comp.gallery.GalleryItem" ) );
}

// ------------------------------------------------------------------------------

uno::Sequence< ::rtl::OUString > GalleryItem::getSupportedServiceNames_Static()
	throw()
{
	uno::Sequence< ::rtl::OUString > aSeq( 1 );
	
	aSeq.getArray()[ 0 ] = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.gallery.GalleryItem" ) );
	
	return aSeq;
}

// ------------------------------------------------------------------------------
	
::rtl::OUString SAL_CALL GalleryItem::getImplementationName() 
	throw( uno::RuntimeException )
{
	return getImplementationName_Static();
}

// ------------------------------------------------------------------------------

sal_Bool SAL_CALL GalleryItem::supportsService( const ::rtl::OUString& ServiceName ) 
	throw( uno::RuntimeException )
{
    uno::Sequence< ::rtl::OUString >	aSNL( getSupportedServiceNames() );
    const ::rtl::OUString*				pArray = aSNL.getConstArray();

    for( int i = 0; i < aSNL.getLength(); i++ )
        if( pArray[i] == ServiceName )
            return true;

    return false;
}

// ------------------------------------------------------------------------------

uno::Sequence< ::rtl::OUString > SAL_CALL GalleryItem::getSupportedServiceNames() 
	throw( uno::RuntimeException )
{
	return getSupportedServiceNames_Static();
}

// ------------------------------------------------------------------------------

uno::Sequence< uno::Type > SAL_CALL GalleryItem::getTypes() 
	throw(uno::RuntimeException)
{
	uno::Sequence< uno::Type >	aTypes( 6 );
	uno::Type* 					pTypes = aTypes.getArray();

	*pTypes++ = ::getCppuType((const uno::Reference< lang::XServiceInfo>*)0);
	*pTypes++ = ::getCppuType((const uno::Reference< lang::XTypeProvider>*)0);
	*pTypes++ = ::getCppuType((const uno::Reference< gallery::XGalleryItem>*)0);
	*pTypes++ = ::getCppuType((const uno::Reference< beans::XPropertySet>*)0);
	*pTypes++ = ::getCppuType((const uno::Reference< beans::XPropertyState>*)0);
	*pTypes++ = ::getCppuType((const uno::Reference< beans::XMultiPropertySet>*)0);

	return aTypes;
}

// ------------------------------------------------------------------------------

uno::Sequence< sal_Int8 > SAL_CALL GalleryItem::getImplementationId() 
	throw(uno::RuntimeException)
{
	const vos::OGuard 					aGuard( Application::GetSolarMutex() );
	static uno::Sequence< sal_Int8 >	aId;
	
	if( aId.getLength() == 0 )
	{
		aId.realloc( 16 );
		rtl_createUuid( reinterpret_cast< sal_uInt8* >( aId.getArray() ), 0, sal_True );
	}
	
	return aId;
}

// ------------------------------------------------------------------------------

sal_Int8 SAL_CALL GalleryItem::getType()
	throw (uno::RuntimeException)
{
	const ::vos::OGuard aGuard( Application::GetSolarMutex() );
	sal_Int8 			nRet = gallery::GalleryItemType::EMPTY;
	
	if( isValid() )
	{
		switch( implGetObject()->eObjKind )
		{
			case( SGA_OBJ_SOUND ):
			case( SGA_OBJ_VIDEO ):
				nRet = gallery::GalleryItemType::MEDIA;
			break;
			
			case( SGA_OBJ_SVDRAW ):
				nRet = gallery::GalleryItemType::DRAWING;
			break;
			
			default:
				nRet = gallery::GalleryItemType::GRAPHIC;
			break;
		}
	}
	
	return nRet;
}

// ------------------------------------------------------------------------------

::comphelper::PropertySetInfo* GalleryItem::createPropertySetInfo()
{
	vos::OGuard 					aGuard( Application::GetSolarMutex() );
	::comphelper::PropertySetInfo*	pRet = new ::comphelper::PropertySetInfo();

	static ::comphelper::PropertyMapEntry aEntries[] =
	{
		{ MAP_CHAR_LEN( "GalleryItemType" ), UNOGALLERY_GALLERYITEMTYPE, &::getCppuType( (const sal_Int8*)(0)),
		  beans::PropertyAttribute::READONLY, 0 },
		
		{ MAP_CHAR_LEN( "URL" ), UNOGALLERY_URL, &::getCppuType( (const ::rtl::OUString*)(0)), 
		  beans::PropertyAttribute::READONLY, 0 },
		  
		{ MAP_CHAR_LEN( "Title" ), UNOGALLERY_TITLE, &::getCppuType( (const ::rtl::OUString*)(0)),
		  0, 0 },
		
		{ MAP_CHAR_LEN( "Thumbnail" ), UNOGALLERY_THUMBNAIL, &::getCppuType( (const uno::Reference< graphic::XGraphic >*)(0)),
		  beans::PropertyAttribute::READONLY, 0 },

		{ MAP_CHAR_LEN( "Graphic" ), UNOGALLERY_GRAPHIC, &::getCppuType( (const uno::Reference< graphic::XGraphic >*)(0)),
		  beans::PropertyAttribute::READONLY, 0 },
		  		  
		{ MAP_CHAR_LEN( "Drawing" ), UNOGALLERY_DRAWING, &::getCppuType( (const uno::Reference< lang::XComponent >*)(0) ),
		  beans::PropertyAttribute::READONLY, 0 },
		
		{ 0,0,0,0,0,0}
	};

	pRet->acquire();
	pRet->add( aEntries );

	return pRet;
}

// ------------------------------------------------------------------------------

void GalleryItem::_setPropertyValues( const comphelper::PropertyMapEntry** ppEntries, const uno::Any* pValues ) 						
	throw( beans::UnknownPropertyException, 
		   beans::PropertyVetoException, 
		   lang::IllegalArgumentException, 
		   lang::WrappedTargetException )
{
	const ::vos::OGuard aGuard( Application::GetSolarMutex() );
	
	while( *ppEntries )
	{
		if( UNOGALLERY_TITLE == (*ppEntries)->mnHandle )
		{
			::rtl::OUString aNewTitle; 
		
			if( *pValues >>= aNewTitle )
			{
				::GalleryTheme*	pGalTheme = ( isValid() ? mpTheme->implGetTheme() : NULL );
			
				if( pGalTheme )
				{
					SgaObject* pObj = pGalTheme->ImplReadSgaObject( const_cast< GalleryObject* >( implGetObject() ) );
					
					if( pObj )
					{
						if( ::rtl::OUString( pObj->GetTitle() ) != aNewTitle )
						{
							pObj->SetTitle( aNewTitle );
							pGalTheme->InsertObject( *pObj );
						}
						
						delete pObj;
					}
				}
			}
			else
			{
				throw lang::IllegalArgumentException();
			}
		}
		
		++ppEntries;
		++pValues;
	}
}

// ------------------------------------------------------------------------------
    
void GalleryItem::_getPropertyValues( const comphelper::PropertyMapEntry** ppEntries, uno::Any* pValue )
	throw( beans::UnknownPropertyException, 
		   lang::WrappedTargetException )
{
	const ::vos::OGuard aGuard( Application::GetSolarMutex() );

	while( *ppEntries )
	{
		switch( (*ppEntries)->mnHandle )
		{
			case( UNOGALLERY_GALLERYITEMTYPE ):
			{
				*pValue <<= sal_Int8( getType() );
			}
			break;

			case( UNOGALLERY_URL ):
			{
				::GalleryTheme*	pGalTheme = ( isValid() ? mpTheme->implGetTheme() : NULL );
			
				if( pGalTheme )
					*pValue <<= ::rtl::OUString( implGetObject()->aURL.GetMainURL( INetURLObject::NO_DECODE ) );
			}
			break;
			
			case( UNOGALLERY_TITLE ):
			{
				::GalleryTheme*	pGalTheme = ( isValid() ? mpTheme->implGetTheme() : NULL );

				if( pGalTheme )
				{
					SgaObject* pObj = pGalTheme->AcquireObject( pGalTheme->ImplGetGalleryObjectPos( implGetObject() ) );
					
					if( pObj )
					{
						*pValue <<= ::rtl::OUString( pObj->GetTitle() );
						pGalTheme->ReleaseObject( pObj );
					}
				}
			}
			break;
		
			case( UNOGALLERY_THUMBNAIL ):
			{
				::GalleryTheme*	pGalTheme = ( isValid() ? mpTheme->implGetTheme() : NULL );

				if( pGalTheme )
				{
					SgaObject* pObj = pGalTheme->AcquireObject( pGalTheme->ImplGetGalleryObjectPos( implGetObject() ) );
					
					if( pObj )
					{
						Graphic aThumbnail;
						
						if( pObj->IsThumbBitmap() )
							aThumbnail = pObj->GetThumbBmp();
						else
							aThumbnail = pObj->GetThumbMtf();
					
						*pValue <<= aThumbnail.GetXGraphic();
						pGalTheme->ReleaseObject( pObj );
					}
				}
			}
			break;

			case( UNOGALLERY_GRAPHIC ):
			{
				::GalleryTheme*	pGalTheme = ( isValid() ? mpTheme->implGetTheme() : NULL );
				Graphic			aGraphic;

				if( pGalTheme && pGalTheme->GetGraphic( pGalTheme->ImplGetGalleryObjectPos( implGetObject() ), aGraphic ) )
					*pValue <<= aGraphic.GetXGraphic();
			}
			break;
			
			case( UNOGALLERY_DRAWING ):
			{
				if( gallery::GalleryItemType::DRAWING == getType() )
				{
					::GalleryTheme*	pGalTheme = ( isValid() ? mpTheme->implGetTheme() : NULL );
					FmFormModel*	pModel = new FmFormModel;

					pModel->GetItemPool().FreezeIdRanges();
	
					if( pGalTheme && pGalTheme->GetModel( pGalTheme->ImplGetGalleryObjectPos( implGetObject() ), *pModel ) )
					{
						uno::Reference< lang::XComponent > xDrawing( new GalleryDrawingModel( pModel ) );
						
						pModel->setUnoModel( uno::Reference< uno::XInterface >::query( xDrawing ) );
						*pValue <<= xDrawing;
					}
					else
						delete pModel;
				}
			}
			break;
		}
		
		++ppEntries;
		++pValue;
	}
}

// ------------------------------------------------------------------------------

const ::GalleryObject* GalleryItem::implGetObject() const
{
	return mpGalleryObject;
}

// ------------------------------------------------------------------------------

void GalleryItem::implSetInvalid()
{
	if( mpTheme )
	{
		mpTheme = NULL;
		mpGalleryObject = NULL;
	}
}

// -----------------------
// - GalleryDrawingModel -
// -----------------------

GalleryDrawingModel::GalleryDrawingModel( SdrModel* pDoc ) 
	throw() :
	SvxUnoDrawingModel( pDoc )
{
}

// -----------------------------------------------------------------------------

GalleryDrawingModel::~GalleryDrawingModel() 
	throw()
{
	delete GetDoc();
}

// -----------------------------------------------------------------------------

UNO3_GETIMPLEMENTATION_IMPL( GalleryDrawingModel );

}
