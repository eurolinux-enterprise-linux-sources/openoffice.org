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

#define _SVX_USE_UNOGLOBALS_
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/awt/FontSlant.hpp>
#include <com/sun/star/style/VerticalAlignment.hpp>
#include <com/sun/star/drawing/TextVerticalAdjust.hpp>
#include <com/sun/star/drawing/XEnhancedCustomShapeDefaulter.hpp>
#include <com/sun/star/awt/TextAlign.hpp>  //added by BerryJia for fixing Bug102407 2002-11-4
#include <com/sun/star/style/ParagraphAdjust.hpp>   //added by BerryJia for fixing Bug102407 2002-11-4
#include <com/sun/star/drawing/PointSequenceSequence.hpp>
#include <com/sun/star/drawing/PointSequence.hpp>
#include <com/sun/star/graphic/XGraphic.hpp>
#include <tools/urlobj.hxx>
#include <unotools/localfilehelper.hxx>
#include <vcl/svapp.hxx>
#include <vos/mutex.hxx>
#include <svtools/fltcall.hxx>
#include "impgrf.hxx"

#include <boost/scoped_ptr.hpp>

#include <rtl/uuid.h>
#include <rtl/memory.h>
#include <tools/urlobj.hxx>

#include <svx/unoprnms.hxx>
#include <svx/unoshape.hxx>
#include <svx/unopage.hxx>
#include <svx/svdobj.hxx>
#include <svx/svdpage.hxx>
#include <svx/svdmodel.hxx>
#include <svx/svdouno.hxx>
#include "shapeimpl.hxx"
#include "svx/unoshprp.hxx"
#include <svx/svdoashp.hxx>
#include "unopolyhelper.hxx"

// #i29181#
#include "svdviter.hxx"
#include <svx/svdview.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/point/b2dpoint.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>

using ::rtl::OUString;
using namespace ::osl;
using namespace ::vos;
using namespace ::cppu;
using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::container;

#define INTERFACE_TYPE( xint ) \
	::getCppuType((const Reference< xint >*)0)

#define QUERYINT( xint ) \
	if( rType == ::getCppuType((const Reference< xint >*)0) ) \
		aAny <<= Reference< xint >(this)

class GDIMetaFile;
class SvStream;
sal_Bool ConvertGDIMetaFileToWMF( const GDIMetaFile & rMTF, SvStream & rTargetStream,
							  FilterConfigItem* pFilterConfigItem = NULL, sal_Bool bPlaceable = sal_True );

/***********************************************************************
* class SvxShapeGroup                                                  *
***********************************************************************/

SvxShapeGroup::SvxShapeGroup( SdrObject* pObj, SvxDrawPage* pDrawPage  )  throw() :
	SvxShape( pObj, aSvxMapProvider.GetMap(SVXMAP_GROUP), aSvxMapProvider.GetPropertySet(SVXMAP_GROUP) ),
	mxPage( pDrawPage )
{
}

//----------------------------------------------------------------------
SvxShapeGroup::~SvxShapeGroup() throw()
{
}

//----------------------------------------------------------------------
void SvxShapeGroup::Create( SdrObject* pNewObj, SvxDrawPage* pNewPage )
{
	SvxShape::Create( pNewObj, pNewPage );
	mxPage = pNewPage;
}

//----------------------------------------------------------------------
uno::Any SAL_CALL SvxShapeGroup::queryInterface( const uno::Type & rType )
	throw(uno::RuntimeException)
{
	return SvxShape::queryInterface( rType );
}

uno::Any SAL_CALL SvxShapeGroup::queryAggregation( const uno::Type & rType ) throw(uno::RuntimeException)
{
	uno::Any aAny;

	QUERYINT( drawing::XShapeGroup );
	else QUERYINT( drawing::XShapes );
	else QUERYINT( container::XIndexAccess );
	else QUERYINT( container::XElementAccess );
	else
		return SvxShape::queryAggregation( rType );

	return aAny;
}

void SAL_CALL SvxShapeGroup::acquire() throw ( )
{
	SvxShape::acquire();
}

void SAL_CALL SvxShapeGroup::release() throw ( )
{
	SvxShape::release();
}

uno::Sequence< uno::Type > SAL_CALL SvxShapeGroup::getTypes()
	throw (uno::RuntimeException)
{
	return SvxShape::getTypes();
}

uno::Sequence< sal_Int8 > SAL_CALL SvxShapeGroup::getImplementationId()
	throw (uno::RuntimeException)
{
	static uno::Sequence< sal_Int8 > aId;
	if( aId.getLength() == 0 )
	{
		aId.realloc( 16 );
		rtl_createUuid( (sal_uInt8 *)aId.getArray(), 0, sal_True );
	}
	return aId;
}

// ::com::sun::star::drawing::XShape

//----------------------------------------------------------------------
OUString SAL_CALL SvxShapeGroup::getShapeType()
	throw( uno::RuntimeException )
{
	return SvxShape::getShapeType();
}

//------------------------------------------------------------------1----
awt::Point SAL_CALL SvxShapeGroup::getPosition() throw(uno::RuntimeException)
{
	return SvxShape::getPosition();
}

//----------------------------------------------------------------------
void SAL_CALL SvxShapeGroup::setPosition( const awt::Point& Position ) throw(uno::RuntimeException)
{
	SvxShape::setPosition(Position);
}

//----------------------------------------------------------------------

awt::Size SAL_CALL SvxShapeGroup::getSize() throw(uno::RuntimeException)
{
	return SvxShape::getSize();
}

//----------------------------------------------------------------------
void SAL_CALL SvxShapeGroup::setSize( const awt::Size& rSize )
	throw(beans::PropertyVetoException, uno::RuntimeException)
{
	SvxShape::setSize( rSize );
}

// drawing::XShapeGroup

//----------------------------------------------------------------------
void SAL_CALL SvxShapeGroup::enterGroup(  ) throw(uno::RuntimeException)
{
	// Todo
//	pDrView->EnterMarkedGroup();
}

//----------------------------------------------------------------------
void SAL_CALL SvxShapeGroup::leaveGroup(  ) throw(uno::RuntimeException)
{
	// Todo
//	pDrView->LeaveOneGroup();
}

//----------------------------------------------------------------------

// XShapes
void SAL_CALL SvxShapeGroup::add( const uno::Reference< drawing::XShape >& xShape )
	throw( uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

	SvxShape* pShape = SvxShape::getImplementation( xShape );

	if( mpObj.is()&& mxPage.is() && pShape )
	{
		SdrObject* pSdrShape = pShape->GetSdrObject();
		if( pSdrShape == NULL )
			pSdrShape = mxPage->_CreateSdrObject( xShape );

		if( pSdrShape->IsInserted() )
			pSdrShape->GetObjList()->RemoveObject( pSdrShape->GetOrdNum() );

		mpObj->GetSubList()->InsertObject( pSdrShape );
		pSdrShape->SetModel(mpObj->GetModel());

		// #85922# It makes no sense to set the layer asked
		// from the group object since these is an iteration
		// over the contained objects. In consequence, this
		// statement erases all layer information from the draw
		// objects. Layers need to be set at draw objects directly
		// and have nothing to do with grouping at all.
		// pSdrShape->SetLayer(pObject->GetLayer());

        // Establish connection between new SdrObject and its wrapper before
        // inserting the new shape into the group.  There a new wrapper
        // would be created when this connection would not already exist.
		if(pShape)
			pShape->Create( pSdrShape, mxPage.get() );

		if( mpModel )
			mpModel->SetChanged();
	}
	else
	{
		DBG_ERROR("could not add XShape to group shape!");
	}
}

//----------------------------------------------------------------------
void SAL_CALL SvxShapeGroup::remove( const uno::Reference< drawing::XShape >& xShape )
	throw( uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

	SdrObject* pSdrShape = NULL;
	SvxShape* pShape = SvxShape::getImplementation( xShape );

	if( pShape )
		pSdrShape = pShape->GetSdrObject();

	if( !mpObj.is() || pSdrShape == NULL || pSdrShape->GetObjList()->GetOwnerObj() != mpObj.get() )
		throw uno::RuntimeException();

	SdrObjList& rList = *pSdrShape->GetObjList();

	const sal_uInt32 nObjCount = rList.GetObjCount();
	sal_uInt32 nObjNum = 0;
	while( nObjNum < nObjCount )
	{
		if(rList.GetObj( nObjNum ) == pSdrShape )
			break;
		nObjNum++;
	}

	if( nObjNum < nObjCount )
	{
		// #i29181#
		// If the SdrObject which is about to be deleted is in any selection,
		// deselect it first.
        SdrViewIter aIter( pSdrShape );

		for ( SdrView* pView = aIter.FirstView(); pView; pView = aIter.NextView() )
		{
			if(CONTAINER_ENTRY_NOTFOUND != pView->TryToFindMarkedObject(pSdrShape))
			{
				pView->MarkObj(pSdrShape, pView->GetSdrPageView(), sal_True, sal_False);
			}
		}

		SdrObject* pObject = rList.NbcRemoveObject( nObjNum );
        SdrObject::Free( pObject );
	}
	else
	{
		DBG_ASSERT( 0, "Fatality! SdrObject is not belonging to its SdrObjList! [CL]" );
	}

	if( mpModel )
		mpModel->SetChanged();
}

// XIndexAccess

//----------------------------------------------------------------------
sal_Int32 SAL_CALL SvxShapeGroup::getCount() throw( uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

	sal_Int32 nRetval = 0;

	if(mpObj.is() && mpObj->GetSubList())
		nRetval = mpObj->GetSubList()->GetObjCount();
	else
		throw uno::RuntimeException();

	return nRetval;
}

//----------------------------------------------------------------------
uno::Any SAL_CALL SvxShapeGroup::getByIndex( sal_Int32 Index )
	throw( lang::IndexOutOfBoundsException, lang::WrappedTargetException, uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

	if( !mpObj.is() || mpObj->GetSubList() == NULL )
		throw uno::RuntimeException();

	if( mpObj->GetSubList()->GetObjCount() <= (sal_uInt32)Index )
		throw lang::IndexOutOfBoundsException();

	SdrObject* pDestObj = mpObj->GetSubList()->GetObj( Index );

	if(pDestObj == NULL)
		throw lang::IndexOutOfBoundsException();

	Reference< drawing::XShape > xShape( pDestObj->getUnoShape(), uno::UNO_QUERY );
	return uno::makeAny( xShape );
}

// ::com::sun::star::container::XElementAccess

//----------------------------------------------------------------------
uno::Type SAL_CALL SvxShapeGroup::getElementType() throw( uno::RuntimeException )
{
	return ::getCppuType(( const Reference< drawing::XShape >*)0);
}

//----------------------------------------------------------------------
sal_Bool SAL_CALL SvxShapeGroup::hasElements() throw( uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

	return mpObj.is() && mpObj->GetSubList() && (mpObj->GetSubList()->GetObjCount() > 0);
}

//----------------------------------------------------------------------
// ::com::sun::star::lang::XServiceInfo

uno::Sequence< OUString > SAL_CALL SvxShapeGroup::getSupportedServiceNames()
	throw(uno::RuntimeException)
{
	return SvxShape::getSupportedServiceNames();
}

/***********************************************************************
*                                                                      *
***********************************************************************/

SvxShapeConnector::SvxShapeConnector( SdrObject* pObj )  throw() :
	SvxShapeText( pObj, aSvxMapProvider.GetMap(SVXMAP_CONNECTOR), aSvxMapProvider.GetPropertySet(SVXMAP_CONNECTOR) )
{
}

//----------------------------------------------------------------------
SvxShapeConnector::~SvxShapeConnector() throw()
{
}

//----------------------------------------------------------------------

uno::Any SAL_CALL SvxShapeConnector::queryInterface( const uno::Type & rType )
	throw(uno::RuntimeException)
{
	return SvxShapeText::queryInterface( rType );
}

uno::Any SAL_CALL SvxShapeConnector::queryAggregation( const uno::Type & rType )
	throw(uno::RuntimeException)
{
	uno::Any aAny;

	QUERYINT( drawing::XConnectorShape );
	else
		return SvxShapeText::queryAggregation( rType );

	return aAny;
}

void SAL_CALL SvxShapeConnector::acquire() throw ( )
{
	SvxShapeText::acquire();
}

void SAL_CALL SvxShapeConnector::release() throw ( )
{
	SvxShapeText::release();
}
// XTypeProvider

uno::Sequence< uno::Type > SAL_CALL SvxShapeConnector::getTypes()
	throw (uno::RuntimeException)
{
	return SvxShape::getTypes();
}

uno::Sequence< sal_Int8 > SAL_CALL SvxShapeConnector::getImplementationId()
	throw (uno::RuntimeException)
{
	static uno::Sequence< sal_Int8 > aId;
	if( aId.getLength() == 0 )
	{
		aId.realloc( 16 );
		rtl_createUuid( (sal_uInt8 *)aId.getArray(), 0, sal_True );
	}
	return aId;
}

// ::com::sun::star::drawing::XShape

//----------------------------------------------------------------------
OUString SAL_CALL SvxShapeConnector::getShapeType()
	throw( uno::RuntimeException )
{
	return SvxShapeText::getShapeType();
}

//------------------------------------------------------------------1----
awt::Point SAL_CALL SvxShapeConnector::getPosition() throw(uno::RuntimeException)
{
	return SvxShapeText::getPosition();
}

//----------------------------------------------------------------------
void SAL_CALL SvxShapeConnector::setPosition( const awt::Point& Position ) throw(uno::RuntimeException)
{
	SvxShapeText::setPosition(Position);
}

//----------------------------------------------------------------------

awt::Size SAL_CALL SvxShapeConnector::getSize() throw(uno::RuntimeException)
{
	return SvxShapeText::getSize();
}

//----------------------------------------------------------------------
void SAL_CALL SvxShapeConnector::setSize( const awt::Size& rSize )
	throw(beans::PropertyVetoException, uno::RuntimeException)
{
	SvxShapeText::setSize( rSize );
}

//----------------------------------------------------------------------

// XConnectorShape

void SAL_CALL SvxShapeConnector::connectStart( const uno::Reference< drawing::XConnectableShape >& xShape, drawing::ConnectionType ) throw( uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

	Reference< drawing::XShape > xRef( xShape, UNO_QUERY );
	SvxShape* pShape = SvxShape::getImplementation( xRef );

	if( pShape )
		mpObj->ConnectToNode( sal_True, pShape->mpObj.get() );

	if( mpModel )
		mpModel->SetChanged();
}

//----------------------------------------------------------------------
void SAL_CALL SvxShapeConnector::connectEnd( const uno::Reference< drawing::XConnectableShape >& xShape, drawing::ConnectionType  )
	throw( uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

	Reference< drawing::XShape > xRef( xShape, UNO_QUERY );
	SvxShape* pShape = SvxShape::getImplementation( xRef );

	if( mpObj.is() && pShape )
		mpObj->ConnectToNode( sal_False, pShape->mpObj.get() );

	if( mpModel )
		mpModel->SetChanged();
}

//----------------------------------------------------------------------
void SAL_CALL SvxShapeConnector::disconnectBegin( const uno::Reference< drawing::XConnectableShape >&  )
	throw( uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

	if(mpObj.is())
		mpObj->DisconnectFromNode( sal_True );

	if( mpModel )
		mpModel->SetChanged();
}

//----------------------------------------------------------------------
void SAL_CALL SvxShapeConnector::disconnectEnd( const uno::Reference< drawing::XConnectableShape >& )
	throw( uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

	if(mpObj.is())
		mpObj->DisconnectFromNode( sal_False );

	if( mpModel )
		mpModel->SetChanged();
}

//----------------------------------------------------------------------
// ::com::sun::star::lang::XServiceInfo
//----------------------------------------------------------------------
uno::Sequence< OUString > SAL_CALL SvxShapeConnector::getSupportedServiceNames() throw( uno::RuntimeException )
{
	return SvxShapeText::getSupportedServiceNames();
}

/***********************************************************************
* class SvxShapeControl                                                *
***********************************************************************/
DBG_NAME(SvxShapeControl)

SvxShapeControl::SvxShapeControl( SdrObject* pObj )  throw() :
	SvxShapeText( pObj, aSvxMapProvider.GetMap(SVXMAP_CONTROL), aSvxMapProvider.GetPropertySet(SVXMAP_CONTROL) )
{
    DBG_CTOR(SvxShapeControl,NULL);
	setShapeKind( OBJ_UNO );
}

//----------------------------------------------------------------------
SvxShapeControl::~SvxShapeControl() throw()
{
    DBG_DTOR(SvxShapeControl,NULL);
}

//----------------------------------------------------------------------
uno::Any SAL_CALL SvxShapeControl::queryInterface( const uno::Type & rType )
	throw(uno::RuntimeException)
{
	return SvxShapeText::queryInterface( rType );
}

uno::Any SAL_CALL SvxShapeControl::queryAggregation( const uno::Type & rType ) throw(uno::RuntimeException)
{
	uno::Any aAny;

	QUERYINT( drawing::XControlShape );
	else
		return SvxShapeText::queryAggregation( rType );

	return aAny;
}

void SAL_CALL SvxShapeControl::acquire() throw ( )
{
	SvxShapeText::acquire();
}

void SAL_CALL SvxShapeControl::release() throw ( )
{
	SvxShapeText::release();
}
// XTypeProvider

uno::Sequence< uno::Type > SAL_CALL SvxShapeControl::getTypes()
	throw (uno::RuntimeException)
{
	return SvxShape::getTypes();
}

uno::Sequence< sal_Int8 > SAL_CALL SvxShapeControl::getImplementationId()
	throw (uno::RuntimeException)
{
	static uno::Sequence< sal_Int8 > aId;
	if( aId.getLength() == 0 )
	{
		aId.realloc( 16 );
		rtl_createUuid( (sal_uInt8 *)aId.getArray(), 0, sal_True );
	}
	return aId;
}

// ::com::sun::star::drawing::XShape

//----------------------------------------------------------------------
OUString SAL_CALL SvxShapeControl::getShapeType()
	throw( uno::RuntimeException )
{
	return SvxShapeText::getShapeType();
}

//------------------------------------------------------------------1----
awt::Point SAL_CALL SvxShapeControl::getPosition() throw(uno::RuntimeException)
{
	return SvxShapeText::getPosition();
}

//----------------------------------------------------------------------
void SAL_CALL SvxShapeControl::setPosition( const awt::Point& Position ) throw(uno::RuntimeException)
{
	SvxShapeText::setPosition(Position);
}

//----------------------------------------------------------------------

awt::Size SAL_CALL SvxShapeControl::getSize() throw(uno::RuntimeException)
{
	return SvxShapeText::getSize();
}

//----------------------------------------------------------------------
void SAL_CALL SvxShapeControl::setSize( const awt::Size& rSize )
	throw(beans::PropertyVetoException, uno::RuntimeException)
{
	SvxShapeText::setSize( rSize );
}

//----------------------------------------------------------------------
// XControlShape

Reference< awt::XControlModel > SAL_CALL SvxShapeControl::getControl()
	throw( uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

	Reference< awt::XControlModel > xModel;

	SdrUnoObj* pUnoObj = dynamic_cast< SdrUnoObj * >(mpObj.get());
	if( pUnoObj )
		xModel = pUnoObj->GetUnoControlModel();

	return xModel;
}

//----------------------------------------------------------------------
void SAL_CALL SvxShapeControl::setControl( const Reference< awt::XControlModel >& xControl )
	throw( uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

	SdrUnoObj* pUnoObj = dynamic_cast< SdrUnoObj * >(mpObj.get());
	if( pUnoObj )
		pUnoObj->SetUnoControlModel( xControl );

	if( mpModel )
		mpModel->SetChanged();
}

// XServiceInfo
uno::Sequence< OUString > SAL_CALL SvxShapeControl::getSupportedServiceNames() throw( uno::RuntimeException )
{
	return SvxShapeText::getSupportedServiceNames();
}

static struct
{
	const sal_Char* mpAPIName;
	sal_uInt16 mnAPINameLen;

	const sal_Char* mpFormName;
	sal_uInt16 mnFormNameLen;
}
SvxShapeControlPropertyMapping[] =
{
	// Warning: The first entry must be FontSlant because the any needs to be converted
	{ MAP_CHAR_LEN(UNO_NAME_EDIT_CHAR_POSTURE), MAP_CHAR_LEN("FontSlant")  }, //  const sal_Int16 => ::com::sun::star::awt::FontSlant
	{ MAP_CHAR_LEN(UNO_NAME_EDIT_CHAR_FONTNAME), MAP_CHAR_LEN("FontName") },
	{ MAP_CHAR_LEN(UNO_NAME_EDIT_CHAR_FONTSTYLENAME), MAP_CHAR_LEN("FontStyleName") },
	{ MAP_CHAR_LEN(UNO_NAME_EDIT_CHAR_FONTFAMILY), MAP_CHAR_LEN("FontFamily") },
	{ MAP_CHAR_LEN(UNO_NAME_EDIT_CHAR_FONTCHARSET), MAP_CHAR_LEN("FontCharset") },
	{ MAP_CHAR_LEN(UNO_NAME_EDIT_CHAR_HEIGHT), MAP_CHAR_LEN("FontHeight") },
	{ MAP_CHAR_LEN(UNO_NAME_EDIT_CHAR_FONTPITCH), MAP_CHAR_LEN("FontPitch" ) },
	{ MAP_CHAR_LEN(UNO_NAME_EDIT_CHAR_WEIGHT), MAP_CHAR_LEN("FontWeight" ) },
	{ MAP_CHAR_LEN(UNO_NAME_EDIT_CHAR_UNDERLINE), MAP_CHAR_LEN("FontUnderline") },
	{ MAP_CHAR_LEN(UNO_NAME_EDIT_CHAR_STRIKEOUT), MAP_CHAR_LEN("FontStrikeout") },
	{ MAP_CHAR_LEN("CharKerning"), MAP_CHAR_LEN("FontKerning") },
	{ MAP_CHAR_LEN("CharWordMode"), MAP_CHAR_LEN("FontWordLineMode" ) },
	{ MAP_CHAR_LEN(UNO_NAME_EDIT_CHAR_COLOR),	MAP_CHAR_LEN("TextColor") },
	{ MAP_CHAR_LEN("CharRelief"),	MAP_CHAR_LEN("FontRelief") },
	{ MAP_CHAR_LEN("CharUnderlineColor"),	MAP_CHAR_LEN("TextLineColor") },
	{ MAP_CHAR_LEN(UNO_NAME_EDIT_PARA_ADJUST), MAP_CHAR_LEN("Align") },
    { MAP_CHAR_LEN("TextVerticalAdjust"), MAP_CHAR_LEN("VerticalAlign") },
	{ MAP_CHAR_LEN("ControlBackground"), MAP_CHAR_LEN("BackgroundColor") },
	{ MAP_CHAR_LEN("ControlSymbolColor"), MAP_CHAR_LEN("SymbolColor") },
	{ MAP_CHAR_LEN("ControlBorder"), MAP_CHAR_LEN("Border") },
	{ MAP_CHAR_LEN("ControlBorderColor"), MAP_CHAR_LEN("BorderColor") },
	{ MAP_CHAR_LEN("ControlTextEmphasis"),	MAP_CHAR_LEN("FontEmphasisMark") },
    { MAP_CHAR_LEN("ImageScaleMode"),  MAP_CHAR_LEN("ScaleMode") },
	{ MAP_CHAR_LEN("ControlWritingMode"), MAP_CHAR_LEN("WritingMode") },
	{ NULL,0, NULL, 0 }
};

namespace
{
    static bool lcl_convertPropertyName( const OUString& rApiName, OUString& rInternalName )
    {
	    sal_uInt16 i = 0;
	    while( SvxShapeControlPropertyMapping[i].mpAPIName )
	    {
		    if( rApiName.reverseCompareToAsciiL( SvxShapeControlPropertyMapping[i].mpAPIName, SvxShapeControlPropertyMapping[i].mnAPINameLen ) == 0 )
		    {
			    rInternalName = OUString( SvxShapeControlPropertyMapping[i].mpFormName, SvxShapeControlPropertyMapping[i].mnFormNameLen, RTL_TEXTENCODING_ASCII_US );
		    }
		    ++i;
	    }
        return rInternalName.getLength() > 0;
    }

    struct EnumConversionMap
    {
        sal_Int16   nAPIValue;
        sal_Int16   nFormValue;
    };

    EnumConversionMap aMapAdjustToAlign[] =
    {
	    // note that order matters:
	    // lcl_convertTextAlignmentToParaAdjustment and lcl_convertParaAdjustmentToTextAlignment search this map from the _beginning_
	    // and use the first matching entry
	    {style::ParagraphAdjust_LEFT,			(sal_Int16)awt::TextAlign::LEFT},
	    {style::ParagraphAdjust_CENTER,			(sal_Int16)awt::TextAlign::CENTER},
	    {style::ParagraphAdjust_RIGHT,			(sal_Int16)awt::TextAlign::RIGHT},
	    {style::ParagraphAdjust_BLOCK,			(sal_Int16)awt::TextAlign::RIGHT},
	    {style::ParagraphAdjust_STRETCH,		(sal_Int16)awt::TextAlign::LEFT},
	    {-1,-1}
    };

    static void lcl_mapFormToAPIValue( Any& _rValue, const EnumConversionMap* _pMap )
    {
	    sal_Int16 nValue = sal_Int16();
	    OSL_VERIFY( _rValue >>= nValue );

        const EnumConversionMap* pEntry = _pMap;
        while ( pEntry && ( pEntry->nFormValue != -1 ) )
	    {
		    if ( nValue == pEntry->nFormValue )
		    {
			    _rValue <<= pEntry->nAPIValue;
			    return;
		    }
            ++pEntry;
	    }
    }

    static void lcl_mapAPIToFormValue( Any& _rValue, const EnumConversionMap* _pMap )
    {
	    sal_Int32 nValue = 0;
	    OSL_VERIFY( _rValue >>= nValue );

        const EnumConversionMap* pEntry = _pMap;
        while ( pEntry && ( pEntry->nAPIValue != -1 ) )
	    {
		    if ( nValue == pEntry->nAPIValue )
		    {
			    _rValue <<= pEntry->nFormValue;
			    return;
		    }
            ++pEntry;
	    }
    }

    static void lcl_convertTextAlignmentToParaAdjustment( Any& rValue )
    {
        lcl_mapFormToAPIValue( rValue, aMapAdjustToAlign );
    }

    static void lcl_convertParaAdjustmentToTextAlignment( Any& rValue )
    {
        lcl_mapAPIToFormValue( rValue, aMapAdjustToAlign );
    }

    void convertVerticalAdjustToVerticalAlign( Any& _rValue ) SAL_THROW( ( lang::IllegalArgumentException ) )
    {
        if ( !_rValue.hasValue() )
            return;

        drawing::TextVerticalAdjust eAdjust = drawing::TextVerticalAdjust_TOP;
        style::VerticalAlignment    eAlign  = style::VerticalAlignment_TOP;
        if ( !( _rValue >>= eAdjust ) )
            throw lang::IllegalArgumentException();
        switch ( eAdjust )
        {
        case drawing::TextVerticalAdjust_TOP:    eAlign = style::VerticalAlignment_TOP; break;
        case drawing::TextVerticalAdjust_BOTTOM: eAlign = style::VerticalAlignment_BOTTOM; break;
        default:                                 eAlign = style::VerticalAlignment_MIDDLE; break;
        }
        _rValue <<= eAlign;
    }

    void convertVerticalAlignToVerticalAdjust( Any& _rValue )
    {
        if ( !_rValue.hasValue() )
            return;
        style::VerticalAlignment    eAlign  = style::VerticalAlignment_TOP;
        drawing::TextVerticalAdjust eAdjust = drawing::TextVerticalAdjust_TOP;
        OSL_VERIFY( _rValue >>= eAlign );
        switch ( eAlign )
        {
        case style::VerticalAlignment_TOP:    eAdjust = drawing::TextVerticalAdjust_TOP; break;
        case style::VerticalAlignment_BOTTOM: eAdjust = drawing::TextVerticalAdjust_BOTTOM; break;
        default:                              eAdjust = drawing::TextVerticalAdjust_CENTER; break;
        }
        _rValue <<= eAdjust;
    }
}

void SAL_CALL SvxShapeControl::setPropertyValue( const OUString& aPropertyName, const uno::Any& aValue )
	throw( beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException, com::sun::star::beans::PropertyVetoException, com::sun::star::lang::IllegalArgumentException)
{
	OUString aFormsName;
	if ( lcl_convertPropertyName( aPropertyName, aFormsName ) )
	{
		uno::Reference< beans::XPropertySet > xControl( getControl(), uno::UNO_QUERY );
		if( xControl.is() )
		{
			uno::Reference< beans::XPropertySetInfo > xInfo( xControl->getPropertySetInfo() );
			if( xInfo.is() && xInfo->hasPropertyByName( aFormsName ) )
			{
                uno::Any aConvertedValue( aValue );
                if ( aFormsName.equalsAscii( "FontSlant" ) )
                {
                    awt::FontSlant nSlant;
                    if( !(aValue >>= nSlant ) )
                        throw lang::IllegalArgumentException();
                    aConvertedValue <<= (sal_Int16)nSlant;
                }
                else if ( aFormsName.equalsAscii( "Align" ) )
                {
                    lcl_convertParaAdjustmentToTextAlignment( aConvertedValue );
                }
                else if ( aFormsName.equalsAscii( "VerticalAlign" ) )
                {
                    convertVerticalAdjustToVerticalAlign( aConvertedValue );
                }

                xControl->setPropertyValue( aFormsName, aConvertedValue );
			}
		}
	}
	else
	{
		SvxShape::setPropertyValue( aPropertyName, aValue );
	}
}

uno::Any SAL_CALL SvxShapeControl::getPropertyValue( const OUString& aPropertyName )
	throw( beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException)
{
	OUString aFormsName;
	if ( lcl_convertPropertyName( aPropertyName, aFormsName ) )
	{
		uno::Reference< beans::XPropertySet > xControl( getControl(), uno::UNO_QUERY );

        uno::Any aValue;
		if( xControl.is() )
		{
			uno::Reference< beans::XPropertySetInfo > xInfo( xControl->getPropertySetInfo() );
			if( xInfo.is() && xInfo->hasPropertyByName( aFormsName ) )
			{
                aValue = xControl->getPropertyValue( aFormsName );
                if ( aFormsName.equalsAscii( "FontSlant" ) )
				{
                    awt::FontSlant eSlant = awt::FontSlant_NONE;
					sal_Int16 nSlant = sal_Int16();
                    if ( aValue >>= nSlant )
                    {
                        eSlant = (awt::FontSlant)nSlant;
                    }
                    else
                    {
                        OSL_VERIFY( aValue >>= eSlant );
                    }
                    aValue <<= eSlant;
				}
				else if ( aFormsName.equalsAscii( "Align" ) )
				{
                    lcl_convertTextAlignmentToParaAdjustment( aValue );
				}
                else if ( aFormsName.equalsAscii( "VerticalAlign" ) )
                {
                    convertVerticalAlignToVerticalAdjust( aValue );
                }
			}
		}

		return aValue;
	}
	else
	{
		return SvxShape::getPropertyValue( aPropertyName );
	}

}

// XPropertyState
beans::PropertyState SAL_CALL SvxShapeControl::getPropertyState( const ::rtl::OUString& PropertyName ) throw( beans::UnknownPropertyException, uno::RuntimeException )
{
	OUString aFormsName;
	if ( lcl_convertPropertyName( PropertyName, aFormsName ) )
	{
		uno::Reference< beans::XPropertyState > xControl( getControl(), uno::UNO_QUERY );
		uno::Reference< beans::XPropertySet > xPropSet( getControl(), uno::UNO_QUERY );

		if( xControl.is() && xPropSet.is() )
		{
			uno::Reference< beans::XPropertySetInfo > xInfo( xPropSet->getPropertySetInfo() );
			if( xInfo.is() && xInfo->hasPropertyByName( aFormsName ) )
			{
				return xControl->getPropertyState( aFormsName );
			}
		}

		return beans::PropertyState_DEFAULT_VALUE;
	}
	else
	{
		return SvxShape::getPropertyState( PropertyName );
	}
}

void SAL_CALL SvxShapeControl::setPropertyToDefault( const ::rtl::OUString& PropertyName ) throw( beans::UnknownPropertyException, uno::RuntimeException )
{
	OUString aFormsName;
	if ( lcl_convertPropertyName( PropertyName, aFormsName ) )
	{
		uno::Reference< beans::XPropertyState > xControl( getControl(), uno::UNO_QUERY );
		uno::Reference< beans::XPropertySet > xPropSet( getControl(), uno::UNO_QUERY );

		if( xControl.is() && xPropSet.is() )
		{
			uno::Reference< beans::XPropertySetInfo > xInfo( xPropSet->getPropertySetInfo() );
			if( xInfo.is() && xInfo->hasPropertyByName( aFormsName ) )
			{
				xControl->setPropertyToDefault( aFormsName );
			}
		}
	}
	else
	{
		SvxShape::setPropertyToDefault( PropertyName );
	}
}

uno::Any SAL_CALL SvxShapeControl::getPropertyDefault( const ::rtl::OUString& aPropertyName )
	throw( beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException )
{
	OUString aFormsName;
	if ( lcl_convertPropertyName( aPropertyName, aFormsName ) )
	{
		uno::Reference< beans::XPropertyState > xControl( getControl(), uno::UNO_QUERY );

		if( xControl.is() )
		{
            Any aDefault( xControl->getPropertyDefault( aFormsName ) );
            if ( aFormsName.equalsAscii( "FontSlant" ) )
			{
				sal_Int16 nSlant( 0 );
				aDefault >>= nSlant;
                aDefault <<= (awt::FontSlant)nSlant;
			}
            else if ( aFormsName.equalsAscii( "Align" ) )
            {
                lcl_convertTextAlignmentToParaAdjustment( aDefault );
            }
            else if ( aFormsName.equalsAscii( "VerticalAlign" ) )
            {
                convertVerticalAlignToVerticalAdjust( aDefault );
            }
            return aDefault;
		}

		throw beans::UnknownPropertyException();
	}
	else
	{
		return SvxShape::getPropertyDefault( aPropertyName );
	}
}


/***********************************************************************
* class SvxShapeDimensioning                                           *
***********************************************************************/

//----------------------------------------------------------------------
SvxShapeDimensioning::SvxShapeDimensioning( SdrObject* pObj ) throw()
:	SvxShapeText( pObj, aSvxMapProvider.GetMap(SVXMAP_DIMENSIONING), aSvxMapProvider.GetPropertySet(SVXMAP_DIMENSIONING) )
{
}

//----------------------------------------------------------------------
SvxShapeDimensioning::~SvxShapeDimensioning() throw()
{
}

// ::com::sun::star::lang::XServiceInfo
uno::Sequence< OUString > SAL_CALL SvxShapeDimensioning::getSupportedServiceNames() throw( uno::RuntimeException )
{
	return SvxShapeText::getSupportedServiceNames();
}

/***********************************************************************
*                                                                      *
***********************************************************************/

//----------------------------------------------------------------------
SvxShapeCircle::SvxShapeCircle( SdrObject* pObj ) throw()
:	SvxShapeText( pObj, aSvxMapProvider.GetMap(SVXMAP_CIRCLE), aSvxMapProvider.GetPropertySet(SVXMAP_CIRCLE) )
{
}

//----------------------------------------------------------------------
SvxShapeCircle::~SvxShapeCircle() throw()
{
}

// ::com::sun::star::lang::XServiceInfo
// XServiceInfo
uno::Sequence< OUString > SAL_CALL SvxShapeCircle::getSupportedServiceNames() throw( uno::RuntimeException )
{
	return SvxShapeText::getSupportedServiceNames();
}

/***********************************************************************
*                                                                      *
***********************************************************************/

#include <svx/svdopath.hxx>

//----------------------------------------------------------------------
SvxShapePolyPolygon::SvxShapePolyPolygon( SdrObject* pObj , drawing::PolygonKind eNew )
 throw( com::sun::star::beans::PropertyVetoException, com::sun::star::lang::IllegalArgumentException)
: SvxShapeText( pObj, aSvxMapProvider.GetMap(SVXMAP_POLYPOLYGON), aSvxMapProvider.GetPropertySet(SVXMAP_POLYPOLYGON) )
, mePolygonKind( eNew )
{
}

//----------------------------------------------------------------------
SvxShapePolyPolygon::~SvxShapePolyPolygon() throw()
{
}

basegfx::B2DPolyPolygon SAL_CALL ImplSvxPointSequenceSequenceToB2DPolyPolygon( const drawing::PointSequenceSequence* pOuterSequence) throw()
{
	basegfx::B2DPolyPolygon aRetval;

	// Zeiger auf innere sequences holen
	const drawing::PointSequence* pInnerSequence = pOuterSequence->getConstArray();
	const drawing::PointSequence* pInnerSeqEnd   = pInnerSequence + pOuterSequence->getLength();

	for(;pInnerSequence != pInnerSeqEnd; ++pInnerSequence)
	{
		// Neues Polygon vorbereiten
		basegfx::B2DPolygon aNewPolygon;

		// Zeiger auf Arrays holen
		const awt::Point* pArray	= pInnerSequence->getConstArray();
		const awt::Point* pArrayEnd = pArray + pInnerSequence->getLength();

		for(;pArray != pArrayEnd;++pArray)
		{
			aNewPolygon.append(basegfx::B2DPoint(pArray->X, pArray->Y));
		}

		// check for closed state flag
		basegfx::tools::checkClosed(aNewPolygon);

		// Neues Teilpolygon einfuegen
		aRetval.append(aNewPolygon);
	}

	return aRetval;
}

//----------------------------------------------------------------------

bool SvxShapePolyPolygon::setPropertyValueImpl( const ::rtl::OUString& rName, const SfxItemPropertySimpleEntry* pProperty, const ::com::sun::star::uno::Any& rValue ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::beans::PropertyVetoException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException)
{
	switch( pProperty->nWID )
	{
	case OWN_ATTR_VALUE_POLYPOLYGON:
	{
		if( rValue.getValue() && (rValue.getValueType() == ::getCppuType(( const drawing::PointSequenceSequence*)0) ) )
		{
			basegfx::B2DPolyPolygon aNewPolyPolygon(ImplSvxPointSequenceSequenceToB2DPolyPolygon( (drawing::PointSequenceSequence*)rValue.getValue()));
			SetPolygon(aNewPolyPolygon);
			return true;
		}
		break;
	}
	case OWN_ATTR_BASE_GEOMETRY:
	{
		if( rValue.getValue() && (rValue.getValueType() == ::getCppuType(( const drawing::PointSequenceSequence*)0)))
		{
			if( mpObj.is() )
			{
				basegfx::B2DPolyPolygon aNewPolyPolygon;
				basegfx::B2DHomMatrix aNewHomogenMatrix;

				mpObj->TRGetBaseGeometry(aNewHomogenMatrix, aNewPolyPolygon);
				aNewPolyPolygon = ImplSvxPointSequenceSequenceToB2DPolyPolygon((drawing::PointSequenceSequence*)rValue.getValue());
				mpObj->TRSetBaseGeometry(aNewHomogenMatrix, aNewPolyPolygon);
			}
			return true;
		}
		break;
	}
	case OWN_ATTR_VALUE_POLYGON:
	{
		if( rValue.getValue() && (rValue.getValueType() == ::getCppuType(( const drawing::PointSequenceSequence*)0) ))
		{
			drawing::PointSequence* pSequence = (drawing::PointSequence*)rValue.getValue();

			// Neues Polygon vorbereiten
			basegfx::B2DPolygon aNewPolygon;

			// Zeiger auf Arrays holen
			// Zeiger auf Arrays holen
			const awt::Point* pArray	= pSequence->getConstArray();
			const awt::Point* pArrayEnd = pArray + pSequence->getLength();

			for(;pArray != pArrayEnd;++pArray)
			{
				aNewPolygon.append(basegfx::B2DPoint(pArray->X, pArray->Y));
			}

			// check for closed state flag
			basegfx::tools::checkClosed(aNewPolygon);

			// Polygon setzen
			SetPolygon(basegfx::B2DPolyPolygon(aNewPolygon));
			return true;
		}
		break;
	}
	default:
        return SvxShapeText::setPropertyValueImpl( rName, pProperty, rValue );
	}

	throw lang::IllegalArgumentException();
}

void SAL_CALL B2DPolyPolygonToSvxPointSequenceSequence( const basegfx::B2DPolyPolygon& rPolyPoly, drawing::PointSequenceSequence& rRetval )
{
	if( (sal_uInt32)rRetval.getLength() != rPolyPoly.count() )
		rRetval.realloc( rPolyPoly.count() );

	// Zeiger auf aeussere Arrays holen
	drawing::PointSequence* pOuterSequence = rRetval.getArray();

	for(sal_uInt32 a(0L); a < rPolyPoly.count(); a++)
	{
		// Einzelpolygon holen
		const basegfx::B2DPolygon aPoly(rPolyPoly.getB2DPolygon(a));

		// #i75974# take closed stae into account, the API polygon still uses the old closed definition
		// with last/first point are identical (cannot hold information about open polygons with identical
		// first and last point, though)
		const sal_uInt32 nPointCount(aPoly.count());
		const bool bIsClosed(aPoly.isClosed());

		// Platz in Arrays schaffen
		pOuterSequence->realloc(bIsClosed ? nPointCount + 1 : nPointCount);

		// Pointer auf arrays holen
		awt::Point* pInnerSequence = pOuterSequence->getArray();

		for(sal_uInt32 b(0L); b < nPointCount; b++)
		{
			const basegfx::B2DPoint aPoint(aPoly.getB2DPoint(b));
			*pInnerSequence = awt::Point( basegfx::fround(aPoint.getX()), basegfx::fround(aPoint.getY()) );
			pInnerSequence++;
		}

		// #i75974# copy first point
		if(bIsClosed)
		{
			*pInnerSequence = *pOuterSequence->getArray();
		}

		pOuterSequence++;
	}
}

//----------------------------------------------------------------------

bool SvxShapePolyPolygon::getPropertyValueImpl( const ::rtl::OUString& rName, const SfxItemPropertySimpleEntry* pProperty, ::com::sun::star::uno::Any& rValue ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException)
{
	switch( pProperty->nWID )
	{
	case OWN_ATTR_VALUE_POLYPOLYGON:
	{
		// PolyPolygon in eine struct PolyPolygon packen
		const basegfx::B2DPolyPolygon& rPolyPoly = GetPolygon();
		drawing::PointSequenceSequence aRetval( rPolyPoly.count() );

		B2DPolyPolygonToSvxPointSequenceSequence( rPolyPoly, aRetval );

		rValue <<= aRetval;
		break;
	}
	case OWN_ATTR_BASE_GEOMETRY:
	{
		// pack a PolyPolygon in struct PolyPolygon
		basegfx::B2DPolyPolygon aNewPolyPolygon;
		basegfx::B2DHomMatrix aNewHomogenMatrix;

		if(mpObj.is())
			mpObj->TRGetBaseGeometry(aNewHomogenMatrix, aNewPolyPolygon);

		drawing::PointSequenceSequence aRetval(aNewPolyPolygon.count());
		B2DPolyPolygonToSvxPointSequenceSequence(aNewPolyPolygon, aRetval);
		rValue <<= aRetval;
		break;
	}
	case OWN_ATTR_VALUE_POLYGON:
	{
		// PolyPolygon in eine struct PolyPolygon packen
		const basegfx::B2DPolyPolygon& rPolyPoly = GetPolygon();

		sal_Int32 nCount = 0;
		if( rPolyPoly.count() > 0 )
			nCount = rPolyPoly.getB2DPolygon(0L).count();

		drawing::PointSequence aRetval( nCount );

		if( nCount > 0 )
		{
			// Einzelpolygon holen
			const basegfx::B2DPolygon aPoly(rPolyPoly.getB2DPolygon(0L));

			// Pointer auf arrays holen
			awt::Point* pSequence = aRetval.getArray();

			for(sal_Int32 b=0;b<nCount;b++)
			{
				const basegfx::B2DPoint aPoint(aPoly.getB2DPoint(b));
				*pSequence++ = awt::Point( basegfx::fround(aPoint.getX()), basegfx::fround(aPoint.getY()) );
			}
		}

		rValue <<= aRetval;
		break;
	}
	case OWN_ATTR_VALUE_POLYGONKIND:
	{
		rValue <<= GetPolygonKind();
		break;
	}
	default:
        return SvxShapeText::getPropertyValueImpl( rName, pProperty, rValue );
	}

	return true;
}

//----------------------------------------------------------------------
drawing::PolygonKind SvxShapePolyPolygon::GetPolygonKind() const throw()
{
	return mePolygonKind;
}

//----------------------------------------------------------------------
void SvxShapePolyPolygon::SetPolygon(const basegfx::B2DPolyPolygon& rNew) throw()
{
	OGuard aGuard( Application::GetSolarMutex() );

	if(mpObj.is())
		((SdrPathObj*)mpObj.get())->SetPathPoly(rNew);
}

//----------------------------------------------------------------------
basegfx::B2DPolyPolygon SvxShapePolyPolygon::GetPolygon() const throw()
{
	OGuard aGuard( Application::GetSolarMutex() );

	if(mpObj.is())
	{
		return ((SdrPathObj*)mpObj.get())->GetPathPoly();
	}
	else
	{
		return basegfx::B2DPolyPolygon();
	}
}

// ::com::sun::star::lang::XServiceInfo
uno::Sequence< OUString > SAL_CALL SvxShapePolyPolygon::getSupportedServiceNames() throw( uno::RuntimeException )
{
	return SvxShapeText::getSupportedServiceNames();
}

/***********************************************************************
* class SvxShapePolyPolygonBezier                                      *
***********************************************************************/
#include <com/sun/star/drawing/PolyPolygonBezierCoords.hpp>
#include <com/sun/star/drawing/FlagSequence.hpp>
//----------------------------------------------------------------------
SvxShapePolyPolygonBezier::SvxShapePolyPolygonBezier( SdrObject* pObj , drawing::PolygonKind eNew ) throw()
:	SvxShapeText( pObj, aSvxMapProvider.GetMap(SVXMAP_POLYPOLYGONBEZIER), aSvxMapProvider.GetPropertySet(SVXMAP_POLYPOLYGONBEZIER) )
,	mePolygonKind( eNew )
{
}

//----------------------------------------------------------------------
SvxShapePolyPolygonBezier::~SvxShapePolyPolygonBezier() throw()
{
}

basegfx::B2DPolyPolygon SvxConvertPolyPolygonBezierToB2DPolyPolygon(const drawing::PolyPolygonBezierCoords* pSourcePolyPolygon)
	throw( IllegalArgumentException )
{
	const sal_Int32 nOuterSequenceCount(pSourcePolyPolygon->Coordinates.getLength());
	basegfx::B2DPolyPolygon aNewPolyPolygon;

	if(pSourcePolyPolygon->Flags.getLength() != nOuterSequenceCount)
	{
		throw IllegalArgumentException();
	}

	// get pointers to inner sequence
	const drawing::PointSequence* pInnerSequence = pSourcePolyPolygon->Coordinates.getConstArray();
	const drawing::FlagSequence* pInnerSequenceFlags = pSourcePolyPolygon->Flags.getConstArray();

	for(sal_Int32 a(0); a < nOuterSequenceCount; a++)
	{
		const sal_Int32 nInnerSequenceCount(pInnerSequence->getLength());

		if(pInnerSequenceFlags->getLength() != nInnerSequenceCount)
		{
			throw IllegalArgumentException();
		}

		// prepare new polygon
		basegfx::B2DPolygon aNewPolygon;
		const awt::Point* pArray = pInnerSequence->getConstArray();
		const drawing::PolygonFlags* pArrayFlags = pInnerSequenceFlags->getConstArray();

		// get first point and flag
		basegfx::B2DPoint aNewCoordinatePair(pArray->X, pArray->Y); pArray++;
		XPolyFlags ePolyFlag((XPolyFlags)((sal_uInt16)*pArrayFlags)); pArrayFlags++;
		basegfx::B2DPoint aControlA;
		basegfx::B2DPoint aControlB;

		// first point is not allowed to be a control point
		if(XPOLY_CONTROL == ePolyFlag)
		{
			throw IllegalArgumentException();
		}

		// add first point as start point
		aNewPolygon.append(aNewCoordinatePair);

		for(sal_Int32 b(1); b < nInnerSequenceCount;)
		{
			// prepare loop
			bool bControlA(false);
			bool bControlB(false);

			// get next point and flag
			aNewCoordinatePair = basegfx::B2DPoint(pArray->X, pArray->Y);
			ePolyFlag = XPolyFlags((XPolyFlags)((sal_uInt16)*pArrayFlags));
			pArray++; pArrayFlags++; b++;

			if(b < nInnerSequenceCount && XPOLY_CONTROL == ePolyFlag)
			{
				aControlA = aNewCoordinatePair;
				bControlA = true;

				// get next point and flag
				aNewCoordinatePair = basegfx::B2DPoint(pArray->X, pArray->Y);
				ePolyFlag = XPolyFlags((XPolyFlags)((sal_uInt16)*pArrayFlags));
				pArray++; pArrayFlags++; b++;
			}

			if(b < nInnerSequenceCount && XPOLY_CONTROL == ePolyFlag)
			{
				aControlB = aNewCoordinatePair;
				bControlB = true;

				// get next point and flag
				aNewCoordinatePair = basegfx::B2DPoint(pArray->X, pArray->Y);
				ePolyFlag = XPolyFlags((XPolyFlags)((sal_uInt16)*pArrayFlags));
				pArray++; pArrayFlags++; b++;
			}

			// two or no control points are consumed, another one would be an error.
			// It's also an error if only one control point was read
			if(XPOLY_CONTROL == ePolyFlag || bControlA != bControlB)
			{
				throw IllegalArgumentException();
			}

			// the previous writes used the B2DPolyPoygon -> PolyPolygon converter
			// which did not create minimal PolyPolygons, but created all control points
			// as null vectors (identical points). Because of the former P(CA)(CB)-norm of
			// B2DPolygon and it's unused sign of being the zero-vector and CA and CB being
			// relative to P, an empty edge was exported as P == CA == CB. Luckily, the new
			// export format can be read without errors by the old OOo-versions, so we need only
			// to correct here at read and do not need to export a wrong but compatible version
			// for the future.
			if(bControlA
				&& aControlA.equal(aControlB)
				&& aControlA.equal(aNewPolygon.getB2DPoint(aNewPolygon.count() - 1)))
			{
				bControlA = bControlB = false;
			}

			if(bControlA)
			{
				// add bezier edge
				aNewPolygon.appendBezierSegment(aControlA, aControlB, aNewCoordinatePair);
			}
			else
			{
				// add edge
				aNewPolygon.append(aNewCoordinatePair);
			}
		}

		// next sequence
		pInnerSequence++;
		pInnerSequenceFlags++;

		// #i72807# API import uses old line start/end-equal definition for closed,
		// so we need to correct this to closed state here
		basegfx::tools::checkClosed(aNewPolygon);

		// add new subpolygon
		aNewPolyPolygon.append(aNewPolygon);
	}

	return aNewPolyPolygon;
}

//----------------------------------------------------------------------

bool SvxShapePolyPolygonBezier::setPropertyValueImpl( const ::rtl::OUString& rName, const SfxItemPropertySimpleEntry* pProperty, const ::com::sun::star::uno::Any& rValue ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::beans::PropertyVetoException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException)
{
	switch( pProperty->nWID )
	{
	case OWN_ATTR_VALUE_POLYPOLYGONBEZIER:
	{
		if( rValue.getValue() && (rValue.getValueType() == ::getCppuType(( const drawing::PolyPolygonBezierCoords*)0) ) )
		{
			basegfx::B2DPolyPolygon aNewPolyPolygon(SvxConvertPolyPolygonBezierToB2DPolyPolygon( (drawing::PolyPolygonBezierCoords*)rValue.getValue()));
			SetPolygon(aNewPolyPolygon);
			return true;
		}
		break;
	}
	case OWN_ATTR_BASE_GEOMETRY:
	{
		if( rValue.getValue() && (rValue.getValueType() == ::getCppuType(( const drawing::PolyPolygonBezierCoords*)0)) )
		{
			if( mpObj.is() )
			{
				basegfx::B2DPolyPolygon aNewPolyPolygon;
				basegfx::B2DHomMatrix aNewHomogenMatrix;

				mpObj->TRGetBaseGeometry(aNewHomogenMatrix, aNewPolyPolygon);
				aNewPolyPolygon = SvxConvertPolyPolygonBezierToB2DPolyPolygon((drawing::PolyPolygonBezierCoords*)rValue.getValue());
				mpObj->TRSetBaseGeometry(aNewHomogenMatrix, aNewPolyPolygon);
			}
			return true;
		}
		break;
	}
	default:
        return SvxShapeText::setPropertyValueImpl( rName, pProperty, rValue );
	}

	throw IllegalArgumentException();
}

void SvxConvertB2DPolyPolygonToPolyPolygonBezier( const basegfx::B2DPolyPolygon& rPolyPoly, drawing::PolyPolygonBezierCoords& rRetval )
{
	// use PolyPolygon converter as base. Since PolyPolygonBezierCoords uses
	// integer coordinates, this is no precision loss at all.
	const PolyPolygon aPolyPoly(rPolyPoly);

	// Polygone innerhalb vrobereiten
	rRetval.Coordinates.realloc((sal_Int32)aPolyPoly.Count());
	rRetval.Flags.realloc((sal_Int32)aPolyPoly.Count());

	// Zeiger auf aeussere Arrays holen
	drawing::PointSequence* pOuterSequence = rRetval.Coordinates.getArray();
	drawing::FlagSequence*  pOuterFlags = rRetval.Flags.getArray();

	for(sal_uInt16 a=0;a<aPolyPoly.Count();a++)
	{
		// Einzelpolygon holen
		const Polygon& rPoly = aPolyPoly[a];

		// Platz in Arrays schaffen
		pOuterSequence->realloc((sal_Int32)rPoly.GetSize());
		pOuterFlags->realloc((sal_Int32)rPoly.GetSize());

		// Pointer auf arrays holen
		awt::Point* pInnerSequence = pOuterSequence->getArray();
		drawing::PolygonFlags* pInnerFlags = pOuterFlags->getArray();

		for(sal_uInt16 b=0;b<rPoly.GetSize();b++)
		{
			*pInnerSequence++ = awt::Point( rPoly[b].X(), rPoly[b].Y() );
			*pInnerFlags++ = (drawing::PolygonFlags)((sal_uInt16)rPoly.GetFlags(b));
		}

		pOuterSequence++;
		pOuterFlags++;
	}
}

//----------------------------------------------------------------------

bool SvxShapePolyPolygonBezier::getPropertyValueImpl( const ::rtl::OUString& rName, const SfxItemPropertySimpleEntry* pProperty, ::com::sun::star::uno::Any& rValue ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException)
{
	switch( pProperty->nWID )
	{
	case OWN_ATTR_VALUE_POLYPOLYGONBEZIER:
	{
		// PolyPolygon in eine struct PolyPolygon packen
		const basegfx::B2DPolyPolygon& rPolyPoly = GetPolygon();
		drawing::PolyPolygonBezierCoords aRetval;
		SvxConvertB2DPolyPolygonToPolyPolygonBezier(rPolyPoly, aRetval );

		rValue <<= aRetval;
		break;
	}
	case OWN_ATTR_BASE_GEOMETRY:
	{
		// PolyPolygon in eine struct PolyPolygon packen
		basegfx::B2DPolyPolygon aNewPolyPolygon;
		basegfx::B2DHomMatrix aNewHomogenMatrix;
		mpObj.get()->TRGetBaseGeometry(aNewHomogenMatrix, aNewPolyPolygon);
		drawing::PolyPolygonBezierCoords aRetval;
		SvxConvertB2DPolyPolygonToPolyPolygonBezier(aNewPolyPolygon, aRetval);

		rValue <<= aRetval;
		break;
	}
	case OWN_ATTR_VALUE_POLYGONKIND:
	{
		rValue <<= mePolygonKind;
		break;
	}
	default:
        return SvxShapeText::getPropertyValueImpl( rName, pProperty, rValue );
	}
	return true;
}

//----------------------------------------------------------------------
drawing::PolygonKind SvxShapePolyPolygonBezier::GetPolygonKind() const throw()
{
	return mePolygonKind;
}

//----------------------------------------------------------------------
void SvxShapePolyPolygonBezier::SetPolygon(const basegfx::B2DPolyPolygon& rNew) throw()
{
	OGuard aGuard( Application::GetSolarMutex() );

	if(mpObj.is())
		static_cast<SdrPathObj*>(mpObj.get())->SetPathPoly(rNew);
}

//----------------------------------------------------------------------
basegfx::B2DPolyPolygon SvxShapePolyPolygonBezier::GetPolygon() const throw()
{
	OGuard aGuard( Application::GetSolarMutex() );

	if(mpObj.is())
	{
		return static_cast<SdrPathObj*>(mpObj.get())->GetPathPoly();
	}
	else
	{
		return basegfx::B2DPolyPolygon();
	}
}


// ::com::sun::star::lang::XServiceInfo
uno::Sequence< OUString > SAL_CALL SvxShapePolyPolygonBezier::getSupportedServiceNames() throw( uno::RuntimeException )
{
	return SvxShapeText::getSupportedServiceNames();
}

/***********************************************************************
* class SvxGraphicObject                                               *
***********************************************************************/
#include <com/sun/star/awt/XBitmap.hpp>
#include <vcl/cvtgrf.hxx>
#include <svx/svdograf.hxx>
#ifndef SVX_LIGHT
#ifndef _SFXDOCFILE_HXX
#include <sfx2/docfile.hxx>
#endif
#include <sfx2/app.hxx>
#include <sfx2/fcontnr.hxx>
#endif

#include "toolkit/unohlp.hxx"

//----------------------------------------------------------------------
SvxGraphicObject::SvxGraphicObject( SdrObject* pObj ) throw()
:	SvxShapeText( pObj, aSvxMapProvider.GetMap(SVXMAP_GRAPHICOBJECT), aSvxMapProvider.GetPropertySet(SVXMAP_GRAPHICOBJECT) )
{
}

//----------------------------------------------------------------------
SvxGraphicObject::~SvxGraphicObject() throw()
{
}

//----------------------------------------------------------------------

bool SvxGraphicObject::setPropertyValueImpl( const ::rtl::OUString& rName, const SfxItemPropertySimpleEntry* pProperty, const ::com::sun::star::uno::Any& rValue ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::beans::PropertyVetoException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException)
{
	bool bOk = false;
	switch( pProperty->nWID )
	{
	case OWN_ATTR_VALUE_FILLBITMAP:
	{
		if( rValue.getValue() )
		{
			if( rValue.getValueType() == ::getCppuType(( const uno::Sequence< sal_Int8 >*)0) )
			{
				uno::Sequence<sal_Int8>* pSeq( (uno::Sequence<sal_Int8>*)rValue.getValue() );
				SvMemoryStream	aMemStm;
				Graphic			aGraphic;

				aMemStm.SetBuffer( (char*)pSeq->getConstArray(), pSeq->getLength(), sal_False, pSeq->getLength() );

				if( GraphicConverter::Import( aMemStm, aGraphic ) == ERRCODE_NONE )
				{
					static_cast<SdrGrafObj*>(mpObj.get())->SetGraphic(aGraphic);
					bOk = true;
				}
			}
		}
		else if( (rValue.getValueType() == awt::XBitmap::static_type()) || (rValue.getValueType() == graphic::XGraphic::static_type()))
		{
			Reference< graphic::XGraphic> xGraphic( rValue, UNO_QUERY );
			if( xGraphic.is() )
			{
				((SdrGrafObj*)mpObj.get())->SetGraphic(Graphic(xGraphic));
				bOk = true;
			}
			else
			{
				// Bitmap in das Objekt packen
				Reference< awt::XBitmap > xBmp( rValue, UNO_QUERY );
				if( xBmp.is() )
				{
					// Bitmap einsetzen
					Graphic aGraphic(VCLUnoHelper::GetBitmap( xBmp ));
					((SdrGrafObj*)mpObj.get())->SetGraphic(aGraphic);
					bOk = true;
				}
			}
		}
		break;
	}
	case OWN_ATTR_GRAFURL:
	{
		OUString aURL;
		if( rValue >>= aURL )
		{
			if( aURL.compareToAscii( UNO_NAME_GRAPHOBJ_URLPREFIX, RTL_CONSTASCII_LENGTH( UNO_NAME_GRAPHOBJ_URLPREFIX ) ) == 0 )
			{
				// graphic manager url
				aURL = aURL.copy( sizeof( UNO_NAME_GRAPHOBJ_URLPREFIX ) - 1 );
				String aTmpStr(aURL);
				ByteString aUniqueID( aTmpStr, RTL_TEXTENCODING_UTF8 );
				GraphicObject aGrafObj( aUniqueID );

				// #101808# since loading a graphic can cause a reschedule of the office
				//			it is possible that our shape is removed while where in this
				//			method.
				if( mpObj.is() )
				{
					static_cast<SdrGrafObj*>(mpObj.get())->ReleaseGraphicLink();
					static_cast<SdrGrafObj*>(mpObj.get())->SetGraphicObject( aGrafObj );
				}
			}
			else if( aURL.compareToAscii( UNO_NAME_GRAPHOBJ_URLPKGPREFIX, RTL_CONSTASCII_LENGTH( UNO_NAME_GRAPHOBJ_URLPKGPREFIX ) ) != 0 )
			{
				// normal link
				String				aFilterName;
				const SfxFilter*	pSfxFilter = NULL;
				SfxMedium			aSfxMedium( aURL, STREAM_READ | STREAM_SHARE_DENYNONE, FALSE );

				SFX_APP()->GetFilterMatcher().GuessFilter( aSfxMedium, &pSfxFilter, SFX_FILTER_IMPORT, SFX_FILTER_NOTINSTALLED | SFX_FILTER_EXECUTABLE );

				if( !pSfxFilter )
				{
					INetURLObject aURLObj( aURL );

					if( aURLObj.GetProtocol() == INET_PROT_NOT_VALID )
					{
						String aValidURL;

						if( ::utl::LocalFileHelper::ConvertPhysicalNameToURL( aURL, aValidURL ) )
							aURLObj = INetURLObject( aValidURL );
					}

					if( aURLObj.GetProtocol() != INET_PROT_NOT_VALID )
					{
						GraphicFilter* pGrfFilter = GetGrfFilter();
						aFilterName = pGrfFilter->GetImportFormatName( pGrfFilter->GetImportFormatNumberForShortName( aURLObj.getExtension() ) );
					}
				}
				else
					aFilterName = pSfxFilter->GetFilterName();

				// #101808# since loading a graphic can cause a reschedule of the office
				//			it is possible that our shape is removed while where in this
				//			method.
				if( mpObj.is() )
					static_cast<SdrGrafObj*>(mpObj.get())->SetGraphicLink( aURL, aFilterName );

			}
			bOk = true;
		}
		break;
	}

	case OWN_ATTR_GRAFSTREAMURL:
	{
		OUString aStreamURL;

		if( rValue >>= aStreamURL )
		{
			if( aStreamURL.compareToAscii( UNO_NAME_GRAPHOBJ_URLPKGPREFIX, RTL_CONSTASCII_LENGTH( UNO_NAME_GRAPHOBJ_URLPKGPREFIX ) ) != 0 )
				aStreamURL = OUString();

			if( mpObj.is() )
			{
				static_cast<SdrGrafObj*>(mpObj.get())->SetGrafStreamURL( aStreamURL );
				static_cast<SdrGrafObj*>(mpObj.get())->ForceSwapOut();
			}
			bOk = true;
		}
		break;
	}

	case OWN_ATTR_VALUE_GRAPHIC:
    {
		Reference< graphic::XGraphic > xGraphic( rValue, uno::UNO_QUERY );
		if( xGraphic.is() )
		{
	    	static_cast< SdrGrafObj*>( mpObj.get() )->SetGraphic( xGraphic );
			bOk = true;
		}
		break;
    }
	default:
        return SvxShapeText::setPropertyValueImpl( rName, pProperty, rValue );
	}

	if( !bOk )
		throw lang::IllegalArgumentException();

	if( mpModel )
		mpModel->SetChanged();

	return true;
}

//----------------------------------------------------------------------

bool SvxGraphicObject::getPropertyValueImpl( const ::rtl::OUString& rName, const SfxItemPropertySimpleEntry* pProperty, ::com::sun::star::uno::Any& rValue ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException)
{
	switch( pProperty->nWID )
	{
	case OWN_ATTR_VALUE_FILLBITMAP:
	{
		const Graphic& rGraphic = static_cast< SdrGrafObj*>( mpObj.get() )->GetGraphic();

		if(rGraphic.GetType() != GRAPHIC_GDIMETAFILE)
		{
			// Objekt in eine Bitmap packen
			Reference< ::com::sun::star::awt::XBitmap >  xBitmap( VCLUnoHelper::CreateBitmap(static_cast< SdrGrafObj*>( mpObj.get() )->GetGraphic().GetBitmapEx()) );
			rValue <<= xBitmap;
		}
		else
		{
			SvMemoryStream aDestStrm( 65535, 65535 );

			ConvertGDIMetaFileToWMF( rGraphic.GetGDIMetaFile(), aDestStrm, NULL, sal_False );
			uno::Sequence<sal_Int8> aSeq((sal_Int8*)aDestStrm.GetData(), aDestStrm.GetSize());
			rValue <<= aSeq;
		}
		break;
	}

	case OWN_ATTR_GRAFURL:
	{
		if( static_cast< SdrGrafObj*>( mpObj.get() )->IsLinkedGraphic() )
		{
			rValue <<= OUString( static_cast< SdrGrafObj*>( mpObj.get() )->GetFileName() );
		}
		else
		{
			const GraphicObject& rGrafObj = static_cast< SdrGrafObj*>( mpObj.get() )->GetGraphicObject(true);
			OUString aURL( RTL_CONSTASCII_USTRINGPARAM(UNO_NAME_GRAPHOBJ_URLPREFIX));
			aURL += OUString::createFromAscii( rGrafObj.GetUniqueID().GetBuffer() );
			rValue <<= aURL;
		}
		break;
	}

	case OWN_ATTR_GRAFSTREAMURL:
	{
		const OUString	aStreamURL( ( (SdrGrafObj*) mpObj.get() )->GetGrafStreamURL() );
		if( aStreamURL.getLength() )
			rValue <<= aStreamURL;
		break;
	}

	case OWN_ATTR_VALUE_GRAPHIC:
    {
        Reference< graphic::XGraphic > xGraphic( static_cast< SdrGrafObj* >( mpObj.get() )->GetGraphic().GetXGraphic() ); 
		rValue <<= xGraphic;
		break;
    }

	case OWN_ATTR_GRAPHIC_STREAM:
	{
		rValue <<= static_cast< SdrGrafObj* >( mpObj.get() )->getInputStream();
		break;
	}
	default:
        return SvxShapeText::getPropertyValueImpl(rName, pProperty,rValue);
	}

	return true;
}

///////////////////////////////////////////////////////////////////////

SvxShapeCaption::SvxShapeCaption( SdrObject* pObj ) throw()
: SvxShapeText( pObj, aSvxMapProvider.GetMap(SVXMAP_CAPTION), aSvxMapProvider.GetPropertySet(SVXMAP_CAPTION) )
{
}

SvxShapeCaption::~SvxShapeCaption() throw()
{
}

/***********************************************************************
* class SvxCustomShape                                                   *
***********************************************************************/

SvxCustomShape::SvxCustomShape( SdrObject* pObj )  throw() :
	SvxShapeText( pObj, aSvxMapProvider.GetMap( SVXMAP_CUSTOMSHAPE ), aSvxMapProvider.GetPropertySet(SVXMAP_CUSTOMSHAPE) )
{
}

//----------------------------------------------------------------------
SvxCustomShape::~SvxCustomShape() throw()
{
}

//----------------------------------------------------------------------

void SvxCustomShape::Create( SdrObject* pNewObj, SvxDrawPage* pNewPage )
{
	SvxShapeText::Create( pNewObj, pNewPage );
}

//----------------------------------------------------------------------

uno::Any SAL_CALL SvxCustomShape::queryInterface( const uno::Type & rType )
	throw(uno::RuntimeException)
{
    return SvxShapeText::queryInterface( rType );
}

uno::Any SAL_CALL SvxCustomShape::queryAggregation( const uno::Type & rType )
	throw(uno::RuntimeException)
{
	::com::sun::star::uno::Any aReturn = SvxShapeText::queryAggregation( rType );
    if ( !aReturn.hasValue() )
		aReturn = ::cppu::queryInterface(rType, static_cast<drawing::XEnhancedCustomShapeDefaulter*>(this) );
	return aReturn;
}

void SAL_CALL SvxCustomShape::acquire() throw ( )
{
	SvxShapeText::acquire();
}

void SAL_CALL SvxCustomShape::release() throw ( )
{
	SvxShapeText::release();
}

//----------------------------------------------------------------------

uno::Sequence< uno::Type > SAL_CALL SvxCustomShape::getTypes()
	throw (uno::RuntimeException)
{
	return SvxShapeText::getTypes();
}

uno::Sequence< sal_Int8 > SAL_CALL SvxCustomShape::getImplementationId()
	throw (uno::RuntimeException)
{
	static uno::Sequence< sal_Int8 > aId;
	if( aId.getLength() == 0 )
	{
		aId.realloc( 16 );
		rtl_createUuid( (sal_uInt8 *)aId.getArray(), 0, sal_True );
	}
	return aId;
}

// ::com::sun::star::drawing::XShape

//----------------------------------------------------------------------
OUString SAL_CALL SvxCustomShape::getShapeType()
	throw( uno::RuntimeException )
{
	return SvxShape::getShapeType();
}

//------------------------------------------------------------------1----
awt::Point SAL_CALL SvxCustomShape::getPosition() throw(uno::RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );
	if ( mpModel && mpObj.is() )
	{
		SdrAShapeObjGeoData aCustomShapeGeoData;
		((SdrObjCustomShape*)mpObj.get())->SaveGeoData( aCustomShapeGeoData );

		sal_Bool bMirroredX = sal_False;
		sal_Bool bMirroredY = sal_False;

		if ( mpObj.is() )
		{
			bMirroredX = ( ((SdrObjCustomShape*)mpObj.get())->IsMirroredX() );
			bMirroredY = ( ((SdrObjCustomShape*)mpObj.get())->IsMirroredY() );
		}
		// get aRect, this is the unrotated snaprect
		Rectangle aRect(((SdrObjCustomShape*)mpObj.get())->GetLogicRect());
		Rectangle aRectangle( aRect );

		if ( bMirroredX || bMirroredY )
		{	// we have to retrieve the unmirrored rect

			GeoStat aNewGeo( aCustomShapeGeoData.aGeo );
			if ( bMirroredX )
			{
				Polygon aPol( Rect2Poly( aRect, aNewGeo ) );
				Rectangle aBoundRect( aPol.GetBoundRect() );

				Point aRef1( ( aBoundRect.Left() + aBoundRect.Right() ) >> 1, aBoundRect.Top() );
				Point aRef2( aRef1.X(), aRef1.Y() + 1000 );
				USHORT i;
				USHORT nPntAnz=aPol.GetSize();
				for (i=0; i<nPntAnz; i++)
				{
					MirrorPoint(aPol[i],aRef1,aRef2);
				}
				// Polygon wenden und etwas schieben
				Polygon aPol0(aPol);
				aPol[0]=aPol0[1];
				aPol[1]=aPol0[0];
				aPol[2]=aPol0[3];
				aPol[3]=aPol0[2];
				aPol[4]=aPol0[1];
				Poly2Rect(aPol,aRectangle,aNewGeo);
			}
			if ( bMirroredY )
			{
				Polygon aPol( Rect2Poly( aRectangle, aNewGeo ) );
				Rectangle aBoundRect( aPol.GetBoundRect() );

				Point aRef1( aBoundRect.Left(), ( aBoundRect.Top() + aBoundRect.Bottom() ) >> 1 );
				Point aRef2( aRef1.X() + 1000, aRef1.Y() );
				USHORT i;
				USHORT nPntAnz=aPol.GetSize();
				for (i=0; i<nPntAnz; i++)
				{
					MirrorPoint(aPol[i],aRef1,aRef2);
				}
				// Polygon wenden und etwas schieben
				Polygon aPol0(aPol);
				aPol[0]=aPol0[1];
				aPol[1]=aPol0[0];
				aPol[2]=aPol0[3];
				aPol[3]=aPol0[2];
				aPol[4]=aPol0[1];
				Poly2Rect( aPol, aRectangle, aNewGeo );
			}
		}
		Point aPt( aRectangle.TopLeft() );

		if( mpModel->IsWriter() )
			aPt -= mpObj->GetAnchorPos();

		ForceMetricTo100th_mm(aPt);
		return ::com::sun::star::awt::Point( aPt.X(), aPt.Y() );
	}
	else
		return SvxShape::getPosition();
}

//----------------------------------------------------------------------
void SAL_CALL SvxCustomShape::setPosition( const awt::Point& Position ) throw(uno::RuntimeException)
{
	SvxShapeText::setPosition(Position);
}

//----------------------------------------------------------------------

awt::Size SAL_CALL SvxCustomShape::getSize() throw(uno::RuntimeException)
{
	return SvxShapeText::getSize();
}

//----------------------------------------------------------------------
void SAL_CALL SvxCustomShape::setSize( const awt::Size& rSize )
	throw(beans::PropertyVetoException, uno::RuntimeException)
{
	SvxShapeText::setSize( rSize );
}

//----------------------------------------------------------------------

//----------------------------------------------------------------------
void SAL_CALL SvxCustomShape::setPropertyValue( const OUString& aPropertyName, const uno::Any& aValue )
	throw( beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException, com::sun::star::beans::PropertyVetoException, com::sun::star::lang::IllegalArgumentException)
{
	OGuard aGuard( Application::GetSolarMutex() );
    SdrObject* pObject = mpObj.get();

    sal_Bool bCustomShapeGeometry = pObject && aPropertyName.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM( "CustomShapeGeometry" ) );

	sal_Bool bMirroredX = sal_False;
	sal_Bool bMirroredY = sal_False;

	if ( bCustomShapeGeometry )
	{
		bMirroredX = ( ((SdrObjCustomShape*)pObject)->IsMirroredX() );
		bMirroredY = ( ((SdrObjCustomShape*)pObject)->IsMirroredY() );
	}

	SvxShape::setPropertyValue( aPropertyName, aValue );

	if ( bCustomShapeGeometry )
	{
		((SdrObjCustomShape*)pObject)->MergeDefaultAttributes(0);
		Rectangle aRect( pObject->GetSnapRect() );

		// #i38892#
		bool bNeedsMirrorX = ((SdrObjCustomShape*)pObject)->IsMirroredX() != bMirroredX;
		bool bNeedsMirrorY = ((SdrObjCustomShape*)pObject)->IsMirroredY() != bMirroredY;

		boost::scoped_ptr< SdrGluePointList > pListCopy;
		if( bNeedsMirrorX || bNeedsMirrorY )
		{
			const SdrGluePointList* pList = pObject->GetGluePointList();
			if( pList )
				pListCopy.reset( new SdrGluePointList(*pList) );
		}

        if ( bNeedsMirrorX )
		{
			Point aTop( ( aRect.Left() + aRect.Right() ) >> 1, aRect.Top() );
			Point aBottom( aTop.X(), aTop.Y() + 1000 );
			pObject->NbcMirror( aTop, aBottom );
			// NbcMirroring is flipping the current mirror state,
			// so we have to set the correct state again
			((SdrObjCustomShape*)pObject)->SetMirroredX( bMirroredX ? sal_False : sal_True );
		}
		if ( bNeedsMirrorY )
		{
			Point aLeft( aRect.Left(), ( aRect.Top() + aRect.Bottom() ) >> 1 );
			Point aRight( aLeft.X() + 1000, aLeft.Y() );
			pObject->NbcMirror( aLeft, aRight );
			// NbcMirroring is flipping the current mirror state,
			// so we have to set the correct state again
			((SdrObjCustomShape*)pObject)->SetMirroredY( bMirroredY ? sal_False : sal_True );
		}

		if( pListCopy )
		{
			SdrGluePointList* pNewList = const_cast< SdrGluePointList* >( pObject->GetGluePointList() );
			if(pNewList)
				*pNewList = *pListCopy;
		}
	}
}

bool SvxCustomShape::getPropertyValueImpl( const ::rtl::OUString& rName, const SfxItemPropertySimpleEntry* pProperty, ::com::sun::star::uno::Any& rValue ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException)
{
	switch( pProperty->nWID )
	{
	case SDRATTR_ROTATEANGLE:
	{
		double fAngle = static_cast<SdrObjCustomShape*>(mpObj.get())->GetObjectRotation();
		fAngle *= 100;
		rValue <<= (sal_Int32)fAngle;
		return true;
	}
	default:
        return SvxShape::getPropertyValueImpl( rName, pProperty, rValue );
	}
}
//----------------------------------------------------------------------

void SvxCustomShape::createCustomShapeDefaults( const rtl::OUString& rValueType ) throw (::com::sun::star::uno::RuntimeException)
{
	((SdrObjCustomShape*)mpObj.get())->MergeDefaultAttributes( &rValueType );
}
