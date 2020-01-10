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
#include <vcl/svapp.hxx>
#include <com/sun/star/style/LineSpacing.hpp>
#include <com/sun/star/text/ControlCharacter.hpp>
#include <com/sun/star/text/ControlCharacter.hpp>
#ifndef _COM_SUN_STAR_TEXT_XTEXTFIELD_HDL_
#include <com/sun/star/text/XTextField.hdl>
#endif
#include <vos/mutex.hxx>
#include <svtools/itemset.hxx>

#include <svx/eeitem.hxx>
#include <svtools/itempool.hxx>
#include <fontitem.hxx>
#include <svx/tstpitem.hxx>
#include <svx/svdobj.hxx>
#include <svtools/intitem.hxx>

#include <svtools/eitem.hxx>

#include <rtl/uuid.h>
#include <rtl/memory.h>

#include <svx/unoshtxt.hxx>
#include <svx/unoprnms.hxx>
#include <svx/unotext.hxx>
#include <svx/unoedsrc.hxx>
#include <svx/unomid.hxx>
#include "unonrule.hxx"
#include "unofdesc.hxx"
#include "unoapi.hxx"
#include <svx/unofield.hxx>
#include <svx/flditem.hxx>
#include <svx/unoshprp.hxx>
#include <svx/numitem.hxx>
#include <svx/editeng.hxx>

using namespace ::rtl;
using namespace ::vos;
using namespace ::cppu;
using namespace ::com::sun::star;

#define QUERYINT( xint ) \
	if( rType == ::getCppuType((const uno::Reference< xint >*)0) ) \
        return uno::makeAny(uno::Reference< xint >(this))


extern const SfxItemPropertySet* ImplGetSvxUnoOutlinerTextCursorSfxPropertySet();
const SfxItemPropertyMapEntry* ImplGetSvxTextPortionPropertyMap()
{
	// Propertymap fuer einen Outliner Text
    static const SfxItemPropertyMapEntry aSvxTextPortionPropertyMap[] =
	{
		SVX_UNOEDIT_CHAR_PROPERTIES,
		SVX_UNOEDIT_FONT_PROPERTIES,
		SVX_UNOEDIT_OUTLINER_PROPERTIES,
		SVX_UNOEDIT_PARA_PROPERTIES,
		{MAP_CHAR_LEN("TextField"),						EE_FEATURE_FIELD,	&::getCppuType((const uno::Reference< text::XTextField >*)0),	beans::PropertyAttribute::READONLY, 0 },
		{MAP_CHAR_LEN("TextPortionType"),				WID_PORTIONTYPE,	&::getCppuType((const ::rtl::OUString*)0), beans::PropertyAttribute::READONLY, 0 },
		{MAP_CHAR_LEN("TextUserDefinedAttributes"),			EE_CHAR_XMLATTRIBS,		&::getCppuType((const ::com::sun::star::uno::Reference< ::com::sun::star::container::XNameContainer >*)0)  , 		0,     0},
		{MAP_CHAR_LEN("ParaUserDefinedAttributes"),			EE_PARA_XMLATTRIBS,		&::getCppuType((const ::com::sun::star::uno::Reference< ::com::sun::star::container::XNameContainer >*)0)  , 		0,     0},
		{0,0,0,0,0,0}
	};
	return aSvxTextPortionPropertyMap;
}
const SvxItemPropertySet* ImplGetSvxTextPortionSvxPropertySet()
{
    static SvxItemPropertySet aSvxTextPortionPropertySet( ImplGetSvxTextPortionPropertyMap() );
    return &aSvxTextPortionPropertySet;
}

const SfxItemPropertySet* ImplGetSvxTextPortionSfxPropertySet()
{
    static SfxItemPropertySet aSvxTextPortionSfxPropertySet( ImplGetSvxTextPortionPropertyMap() );
    return &aSvxTextPortionSfxPropertySet;
}

// ====================================================================
// helper fuer Item/Property Konvertierung
// ====================================================================

void GetSelection( struct ESelection& rSel, SvxTextForwarder* pForwarder ) throw()
{
	DBG_ASSERT( pForwarder, "I need a valid SvxTextForwarder!" );
	if( pForwarder )
	{
		sal_Int16 nParaCount = pForwarder->GetParagraphCount();
		if(nParaCount>0)
			nParaCount--;

		rSel = ESelection( 0,0, nParaCount, pForwarder->GetTextLen( nParaCount ));
	}
}

void CheckSelection( struct ESelection& rSel, SvxTextForwarder* pForwarder ) throw()
{
	DBG_ASSERT( pForwarder, "I need a valid SvxTextForwarder!" );
	if( pForwarder )
	{
		if( rSel.nStartPara == 0xffff )
		{
			::GetSelection( rSel, pForwarder );
		}
		else
		{
			ESelection aMaxSelection;
			GetSelection( aMaxSelection, pForwarder );

			// check start position
			if( rSel.nStartPara < aMaxSelection.nStartPara )
			{
				rSel.nStartPara = aMaxSelection.nStartPara;
				rSel.nStartPos = aMaxSelection.nStartPos;
			}
			else if( rSel.nStartPara > aMaxSelection.nEndPara )
			{
				rSel.nStartPara = aMaxSelection.nEndPara;
				rSel.nStartPos = aMaxSelection.nEndPos;
			}
			else if( rSel.nStartPos  > pForwarder->GetTextLen( rSel.nStartPara ) )
			{
				rSel.nStartPos = pForwarder->GetTextLen( rSel.nStartPara );
			}

			// check end position
			if( rSel.nEndPara < aMaxSelection.nStartPara )
			{
				rSel.nEndPara = aMaxSelection.nStartPara;
				rSel.nEndPos = aMaxSelection.nStartPos;
			}
			else if( rSel.nEndPara > aMaxSelection.nEndPara )
			{
				rSel.nEndPara = aMaxSelection.nEndPara;
				rSel.nEndPos = aMaxSelection.nEndPos;
			}
			else if( rSel.nEndPos > pForwarder->GetTextLen( rSel.nEndPara ) )
			{
				rSel.nEndPos = pForwarder->GetTextLen( rSel.nEndPara );
			}
		}
	}
}

// ====================================================================
// class SvxUnoTextRangeBase
// ====================================================================

#ifdef DEBUG
class check_me
{
public:
	check_me() : mnAllocNum(0) {};
	~check_me();

	void add( SvxUnoTextRangeBase* pRange );
	void remove( SvxUnoTextRangeBase* pRange );

	std::list< std::pair< sal_uInt32, SvxUnoTextRangeBase* > > maRanges;
	sal_uInt32 mnAllocNum;
};

void check_me::add( SvxUnoTextRangeBase* pRange )
{
	maRanges.push_back( std::pair< sal_uInt32, SvxUnoTextRangeBase* >( mnAllocNum++, pRange ) );
}

void check_me::remove( SvxUnoTextRangeBase* pRange )
{
	std::list< std::pair< sal_uInt32, SvxUnoTextRangeBase* > >::iterator aIter;
	for( aIter = maRanges.begin(); aIter != maRanges.end(); aIter++ )
	{
		if( pRange == (*aIter).second )
		{
			maRanges.erase( aIter );
			break;
		}
	}
}

check_me::~check_me()
{
	if( !maRanges.empty() )
	{
		DBG_ERROR("living text range detected!");
		std::list< std::pair< sal_uInt32, SvxUnoTextRangeBase* > >::iterator aIter;
		for( aIter = maRanges.begin(); aIter != maRanges.end(); aIter++ )
		{
			sal_Int32 nAllocNum;
			SvxUnoTextRangeBase* pRange;
            nAllocNum = (*aIter).first;
            pRange = (*aIter).second;
		}
	}
}

static check_me gNumRanges;
#endif

UNO3_GETIMPLEMENTATION_IMPL( SvxUnoTextRangeBase );

SvxUnoTextRangeBase::SvxUnoTextRangeBase( const SvxItemPropertySet* _pSet ) throw()
: mpEditSource(NULL) , mpPropSet(_pSet)
{
#ifdef DEBUG
	gNumRanges.add(this);
#endif
}

SvxUnoTextRangeBase::SvxUnoTextRangeBase( const SvxEditSource* pSource, const SvxItemPropertySet* _pSet ) throw()
: mpPropSet(_pSet)
{
	OGuard aGuard( Application::GetSolarMutex() );

	DBG_ASSERT(pSource,"SvxUnoTextRangeBase: I need a valid SvxEditSource!");

	mpEditSource = pSource->Clone();
	ESelection aSelection;
	::GetSelection( aSelection, mpEditSource->GetTextForwarder() );
	SetSelection( aSelection );

	if( mpEditSource )
		mpEditSource->addRange( this );
#ifdef DEBUG
	gNumRanges.add(this);
#endif
}

SvxUnoTextRangeBase::SvxUnoTextRangeBase( const SvxUnoTextRangeBase& rRange ) throw()
:	text::XTextRange()
,	beans::XPropertySet()
,	beans::XMultiPropertySet()
,   beans::XMultiPropertyStates()
,	beans::XPropertyState()
,	lang::XServiceInfo()
,	text::XTextRangeCompare()
,	lang::XUnoTunnel()
,	mpPropSet(rRange.getPropertySet())
{
	OGuard aGuard( Application::GetSolarMutex() );

	mpEditSource = rRange.mpEditSource ? rRange.mpEditSource->Clone() : NULL;

	SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : NULL;
	if( pForwarder )
	{
		maSelection  = rRange.maSelection;
		CheckSelection( maSelection, pForwarder );
	}

	if( mpEditSource )
		mpEditSource->addRange( this );

#ifdef DEBUG
	gNumRanges.add(this);
#endif
}

SvxUnoTextRangeBase::~SvxUnoTextRangeBase() throw()
{
#ifdef DEBUG
	gNumRanges.remove(this);
#endif

	if( mpEditSource )
		mpEditSource->removeRange( this );

	delete mpEditSource;
}

void SvxUnoTextRangeBase::SetEditSource( SvxEditSource* pSource ) throw()
{
	DBG_ASSERT(pSource,"SvxUnoTextRangeBase: I need a valid SvxEditSource!");
	DBG_ASSERT(mpEditSource==NULL,"SvxUnoTextRangeBase::SetEditSource called while SvxEditSource already set" );

	mpEditSource = pSource;

	maSelection.nStartPara = 0xffff;

	if( mpEditSource )
		mpEditSource->addRange( this );
}

/** puts a field item with a copy of the given FieldData into the itemset
    corresponding with this range */
void SvxUnoTextRangeBase::attachField( const SvxFieldData* pData ) throw()
{
	OGuard aGuard( Application::GetSolarMutex() );

	if( pData )
	{
		SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : NULL;
		if( pForwarder )
		{
            SvxFieldItem aField( *pData, EE_FEATURE_FIELD );
			pForwarder->QuickInsertField( aField, maSelection );
		}
	}
}

void SvxUnoTextRangeBase::SetSelection( const ESelection& rSelection ) throw()
{
	OGuard aGuard( Application::GetSolarMutex() );

	maSelection = rSelection;
	CheckSelection( maSelection, mpEditSource->GetTextForwarder() );
}

// Interface XTextRange ( XText )

uno::Reference< text::XTextRange > SAL_CALL SvxUnoTextRangeBase::getStart(void)
	throw( uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

	uno::Reference< text::XTextRange > xRange;

	SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : NULL;
	if( pForwarder )
	{

		CheckSelection( maSelection, pForwarder );

		SvxUnoTextBase* pText = SvxUnoTextBase::getImplementation( getText() );

		if(pText == NULL)
			throw uno::RuntimeException();

		SvxUnoTextRange* pRange = new SvxUnoTextRange( *pText );
		xRange = pRange;

		ESelection aNewSel = maSelection;
		aNewSel.nEndPara = aNewSel.nStartPara;
		aNewSel.nEndPos  = aNewSel.nStartPos;
		pRange->SetSelection( aNewSel );
	}

	return xRange;
}

uno::Reference< text::XTextRange > SAL_CALL SvxUnoTextRangeBase::getEnd(void)
	throw( uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

	uno::Reference< text::XTextRange > xRet;

	SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : NULL;
	if( pForwarder )
	{
		CheckSelection( maSelection, pForwarder );

		SvxUnoTextBase* pText = SvxUnoTextBase::getImplementation( getText() );

		if(pText == NULL)
			throw uno::RuntimeException();

		SvxUnoTextRange* pNew = new SvxUnoTextRange( *pText );
		xRet = pNew;

		ESelection aNewSel = maSelection;
		aNewSel.nStartPara = aNewSel.nEndPara;
		aNewSel.nStartPos  = aNewSel.nEndPos;
		pNew->SetSelection( aNewSel );
	}
	return xRet;
}

OUString SAL_CALL SvxUnoTextRangeBase::getString(void)
	throw( uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

	SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : NULL;
	if( pForwarder )
	{
		CheckSelection( maSelection, pForwarder );

		return pForwarder->GetText( maSelection );
	}
	else
	{
		const OUString aEmpty;
		return aEmpty;
	}
}

void SAL_CALL SvxUnoTextRangeBase::setString(const OUString& aString)
	throw( uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

	SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : NULL;
	if( pForwarder )
	{
		CheckSelection( maSelection, pForwarder );

		String aConverted( aString );
		aConverted.ConvertLineEnd( LINEEND_LF );		// Zeilenenden nur einfach zaehlen

		pForwarder->QuickInsertText( aConverted, maSelection );
		mpEditSource->UpdateData();

		//	Selektion anpassen
		//!	Wenn die EditEngine bei QuickInsertText die Selektion zurueckgeben wuerde,
		//!	waer's einfacher...
		CollapseToStart();

		sal_uInt16 nLen = aConverted.Len();
		if (nLen)
			GoRight( nLen, sal_True );
	}
}

// Interface beans::XPropertySet
uno::Reference< beans::XPropertySetInfo > SAL_CALL SvxUnoTextRangeBase::getPropertySetInfo(void)
	throw( uno::RuntimeException )
{
	return mpPropSet->getPropertySetInfo();
}

void SAL_CALL SvxUnoTextRangeBase::setPropertyValue(const OUString& PropertyName, const uno::Any& aValue)
	throw( beans::UnknownPropertyException, beans::PropertyVetoException, lang::IllegalArgumentException, lang::WrappedTargetException, uno::RuntimeException )
{
	_setPropertyValue( PropertyName, aValue, -1 );
}

void SAL_CALL SvxUnoTextRangeBase::_setPropertyValue( const OUString& PropertyName, const uno::Any& aValue, sal_Int32 nPara )
	throw( beans::UnknownPropertyException, beans::PropertyVetoException, lang::IllegalArgumentException, lang::WrappedTargetException, uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

	SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : NULL;
	if( pForwarder )
	{

		CheckSelection( maSelection, pForwarder );

        const SfxItemPropertySimpleEntry* pMap = mpPropSet->getPropertyMapEntry(PropertyName );
		if ( pMap )
		{
			ESelection aSel( GetSelection() );
			sal_Bool bParaAttrib = (pMap->nWID >= EE_PARA_START) && ( pMap->nWID <= EE_PARA_END );

			if( nPara == -1 && !bParaAttrib )
			{
				SfxItemSet aOldSet( pForwarder->GetAttribs( aSel ) );
				// we have a selection and no para attribute
				SfxItemSet aNewSet( *aOldSet.GetPool(), aOldSet.GetRanges() );

				setPropertyValue( pMap, aValue, maSelection, aOldSet, aNewSet );


				pForwarder->QuickSetAttribs( aNewSet, GetSelection() );
			}
			else
			{
				sal_Int32 nEndPara;

				if( nPara == -1 )
				{
					nPara = aSel.nStartPara;
					nEndPara = aSel.nEndPara;
				}
				else
				{
					// only one paragraph
					nEndPara = nPara;
				}

				while( nPara <= nEndPara )
				{
					// we have a paragraph
					SfxItemSet aSet( pForwarder->GetParaAttribs( (USHORT)nPara ) );
					setPropertyValue( pMap, aValue, maSelection, aSet, aSet );
					pForwarder->SetParaAttribs( (USHORT)nPara, aSet );
					nPara++;
				}
			}

			GetEditSource()->UpdateData();
			return;
		}
	}

	throw beans::UnknownPropertyException();
}

void SvxUnoTextRangeBase::setPropertyValue( const SfxItemPropertySimpleEntry* pMap, const uno::Any& rValue, const ESelection& rSelection, const SfxItemSet& rOldSet, SfxItemSet& rNewSet ) throw( beans::UnknownPropertyException, lang::IllegalArgumentException )
{
	if(!SetPropertyValueHelper( rOldSet, pMap, rValue, rNewSet, &rSelection, (SvxTextEditSource*)GetEditSource() ))
	{
		//	Fuer Teile von zusammengesetzten Items mit mehreren Properties (z.B. Hintergrund)
		//	muss vorher das alte Item aus dem Dokument geholt werden
		rNewSet.Put(rOldSet.Get(pMap->nWID));			// altes Item in neuen Set
		mpPropSet->setPropertyValue(pMap, rValue, rNewSet);
	}
}

sal_Bool SvxUnoTextRangeBase::SetPropertyValueHelper( const SfxItemSet&, const SfxItemPropertySimpleEntry* pMap, const uno::Any& aValue, SfxItemSet& rNewSet, const ESelection* pSelection /* = NULL */, SvxTextEditSource* pEditSource /* = NULL*/ ) throw( uno::RuntimeException )
{
	switch( pMap->nWID )
	{
	case WID_FONTDESC:
		{
			awt::FontDescriptor aDesc;
			if(aValue >>= aDesc)
			{
				SvxUnoFontDescriptor::FillItemSet( aDesc, rNewSet );
				return sal_True;
			}
		}
		break;

	case EE_PARA_NUMBULLET:
		{
			uno::Reference< container::XIndexReplace > xRule;
			if( !aValue.hasValue() || ((aValue >>= xRule) && !xRule.is()) )
				return sal_True;

			return sal_False;
		}

	case WID_NUMLEVEL:
		{
			SvxTextForwarder* pForwarder = pEditSource? pEditSource->GetTextForwarder() : NULL;
			if(pForwarder && pSelection)
			{
				sal_Int16 nLevel = sal_Int16();
				if( aValue >>= nLevel )
				{
                    // #101004# Call interface method instead of unsafe cast
                    if(! pForwarder->SetDepth( pSelection->nStartPara, nLevel ) )
						throw lang::IllegalArgumentException();

					return sal_True;
				}
			}
		}
		break;
    case WID_NUMBERINGSTARTVALUE:
		{
			SvxTextForwarder* pForwarder = pEditSource? pEditSource->GetTextForwarder() : NULL;
			if(pForwarder && pSelection)
			{
				sal_Int16 nStartValue = -1;
				if( aValue >>= nStartValue )
                {
                    pForwarder->SetNumberingStartValue( pSelection->nStartPara, nStartValue );
	    			return sal_True;
                }
			}
		}
		break;
    case WID_PARAISNUMBERINGRESTART:
		{
			SvxTextForwarder* pForwarder = pEditSource? pEditSource->GetTextForwarder() : NULL;
			if(pForwarder && pSelection)
			{
				sal_Bool bParaIsNumberingRestart = sal_False;
				if( aValue >>= bParaIsNumberingRestart )
                {
                    pForwarder->SetParaIsNumberingRestart( pSelection->nStartPara, bParaIsNumberingRestart );
				    return sal_True;
                }
			}
		}
		break;
	case EE_PARA_BULLETSTATE:
		{
			sal_Bool bBullet = sal_True;
			if( aValue >>= bBullet )
			{
				SfxBoolItem aItem( EE_PARA_BULLETSTATE, bBullet );
				rNewSet.Put(aItem);
				return sal_True;
			}
		}
		break;

	default:
		return sal_False;
	}

	throw lang::IllegalArgumentException();
}

uno::Any SAL_CALL SvxUnoTextRangeBase::getPropertyValue(const OUString& PropertyName)
	throw( beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException )
{
	return _getPropertyValue( PropertyName, -1 );
}

uno::Any SAL_CALL SvxUnoTextRangeBase::_getPropertyValue(const OUString& PropertyName, sal_Int32 nPara )
	throw( beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

	uno::Any aAny;

	SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : NULL;
	if( pForwarder )
	{
        const SfxItemPropertySimpleEntry* pMap = mpPropSet->getPropertyMapEntry(PropertyName );
		if( pMap )
		{
			SfxItemSet* pAttribs = NULL;
			if( nPara != -1 )
				pAttribs = pForwarder->GetParaAttribs( (USHORT)nPara ).Clone();
			else
				pAttribs = pForwarder->GetAttribs( GetSelection() ).Clone();

			//	Dontcare durch Default ersetzen, damit man immer eine Reflection hat
			pAttribs->ClearInvalidItems();

			getPropertyValue( pMap, aAny, *pAttribs );

			delete pAttribs;
			return aAny;
		}
	}

	throw beans::UnknownPropertyException();
}

void SvxUnoTextRangeBase::getPropertyValue( const SfxItemPropertySimpleEntry* pMap, uno::Any& rAny, const SfxItemSet& rSet ) throw( beans::UnknownPropertyException )
{
	switch( pMap->nWID )
	{
	case EE_FEATURE_FIELD:
		if ( rSet.GetItemState( EE_FEATURE_FIELD, sal_False ) == SFX_ITEM_SET )
		{
			SvxFieldItem* pItem = (SvxFieldItem*)rSet.GetItem( EE_FEATURE_FIELD );
			const SvxFieldData* pData = pItem->GetField();
			uno::Reference< text::XTextRange > xAnchor( this );

			// get presentation string for field
			Color* pTColor = NULL;
			Color* pFColor = NULL;

			SvxTextForwarder* pForwarder = mpEditSource->GetTextForwarder();
            OUString aPresentation( pForwarder->CalcFieldValue( SvxFieldItem(*pData, EE_FEATURE_FIELD), maSelection.nStartPara, maSelection.nStartPos, pTColor, pFColor ) );

			delete pTColor;
			delete pFColor;

			uno::Reference< text::XTextField > xField( new SvxUnoTextField( xAnchor, aPresentation, pData ) );
			rAny <<= xField;
		}
		break;

	case WID_PORTIONTYPE:
		if ( rSet.GetItemState( EE_FEATURE_FIELD, sal_False ) == SFX_ITEM_SET )
		{
			OUString aType( RTL_CONSTASCII_USTRINGPARAM("TextField") );
			rAny <<= aType;
		}
		else
		{
			OUString aType( RTL_CONSTASCII_USTRINGPARAM("Text") );
			rAny <<= aType;
		}
		break;

	default:
		if(!GetPropertyValueHelper( *((SfxItemSet*)(&rSet)), pMap, rAny, &maSelection, (SvxTextEditSource*)GetEditSource() ))
			rAny = mpPropSet->getPropertyValue(pMap, rSet);
	}
}

sal_Bool SvxUnoTextRangeBase::GetPropertyValueHelper(  SfxItemSet& rSet, const SfxItemPropertySimpleEntry* pMap, uno::Any& aAny, const ESelection* pSelection /* = NULL */, SvxTextEditSource* pEditSource /* = NULL */ )
	throw( uno::RuntimeException )
{
	switch( pMap->nWID )
	{
	case WID_FONTDESC:
		{
			awt::FontDescriptor aDesc;
			SvxUnoFontDescriptor::FillFromItemSet( rSet, aDesc );
			aAny <<= aDesc;
		}
		break;

	case EE_PARA_NUMBULLET:
		{
			if((rSet.GetItemState( EE_PARA_NUMBULLET, sal_True ) & (SFX_ITEM_SET|SFX_ITEM_DEFAULT)) == 0)
				throw uno::RuntimeException();

			SvxNumBulletItem* pBulletItem = (SvxNumBulletItem*)rSet.GetItem( EE_PARA_NUMBULLET, sal_True );

			if( pBulletItem == NULL )
				throw uno::RuntimeException();

			aAny <<= SvxCreateNumRule( pBulletItem->GetNumRule() );
		}
		break;

	case WID_NUMLEVEL:
		{
			SvxTextForwarder* pForwarder = pEditSource? pEditSource->GetTextForwarder() : NULL;
			if(pForwarder && pSelection)
			{
				sal_Int16 nLevel = pForwarder->GetDepth( pSelection->nStartPara );
				if( nLevel >= 0 )
					aAny <<= nLevel;
			}
		}
		break;
    case WID_NUMBERINGSTARTVALUE:
		{
			SvxTextForwarder* pForwarder = pEditSource? pEditSource->GetTextForwarder() : NULL;
			if(pForwarder && pSelection)
				aAny <<= pForwarder->GetNumberingStartValue( pSelection->nStartPara );
		}
		break;
    case WID_PARAISNUMBERINGRESTART:
		{
			SvxTextForwarder* pForwarder = pEditSource? pEditSource->GetTextForwarder() : NULL;
			if(pForwarder && pSelection)
				aAny <<= pForwarder->IsParaIsNumberingRestart( pSelection->nStartPara );
		}
		break;

	case EE_PARA_BULLETSTATE:
		{
			sal_Bool bState = sal_False;
			if( rSet.GetItemState( EE_PARA_BULLETSTATE, sal_True ) & (SFX_ITEM_SET|SFX_ITEM_DEFAULT))
			{
				SfxBoolItem* pItem = (SfxBoolItem*)rSet.GetItem( EE_PARA_BULLETSTATE, sal_True );
                bState = pItem->GetValue() ? sal_True : sal_False;
			}

			aAny <<= bState;
		}
		break;
	default:

		return sal_False;
	}

	return sal_True;
}

// wird (noch) nicht unterstuetzt
void SAL_CALL SvxUnoTextRangeBase::addPropertyChangeListener( const OUString& , const uno::Reference< beans::XPropertyChangeListener >& ) throw(beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException) {}
void SAL_CALL SvxUnoTextRangeBase::removePropertyChangeListener( const OUString& , const uno::Reference< beans::XPropertyChangeListener >& ) throw(beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException) {}
void SAL_CALL SvxUnoTextRangeBase::addVetoableChangeListener( const OUString& , const uno::Reference< beans::XVetoableChangeListener >& ) throw(beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException) {}
void SAL_CALL SvxUnoTextRangeBase::removeVetoableChangeListener( const OUString& , const uno::Reference< beans::XVetoableChangeListener >& ) throw(beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException) {}

// XMultiPropertySet
void SAL_CALL SvxUnoTextRangeBase::setPropertyValues( const uno::Sequence< ::rtl::OUString >& aPropertyNames, const uno::Sequence< uno::Any >& aValues ) throw (beans::PropertyVetoException, lang::IllegalArgumentException, lang::WrappedTargetException, uno::RuntimeException)
{
	_setPropertyValues( aPropertyNames, aValues, -1 );
}

void SAL_CALL SvxUnoTextRangeBase::_setPropertyValues( const uno::Sequence< ::rtl::OUString >& aPropertyNames, const uno::Sequence< uno::Any >& aValues, sal_Int32 nPara ) throw (beans::PropertyVetoException, lang::IllegalArgumentException, lang::WrappedTargetException, uno::RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );

	SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : NULL;
	if( pForwarder )
	{
		CheckSelection( maSelection, pForwarder );

		ESelection aSel( GetSelection() );

		const OUString* pPropertyNames = aPropertyNames.getConstArray();
		const uno::Any* pValues = aValues.getConstArray();
		sal_Int32 nCount = aPropertyNames.getLength();

		sal_Int32 nEndPara = nPara;
		sal_Int32 nTempPara = nPara;

		if( nTempPara == -1 )
		{
			nTempPara = aSel.nStartPara;
			nEndPara = aSel.nEndPara;
		}

		SfxItemSet* pOldAttrSet = NULL;
		SfxItemSet* pNewAttrSet = NULL;

		SfxItemSet* pOldParaSet = NULL;
		SfxItemSet* pNewParaSet = NULL;

		for( ; nCount; nCount--, pPropertyNames++, pValues++ )
		{
            const SfxItemPropertySimpleEntry* pMap = mpPropSet->getPropertyMapEntry( *pPropertyNames );

			if( pMap )
			{
				sal_Bool bParaAttrib = (pMap->nWID >= EE_PARA_START) && ( pMap->nWID <= EE_PARA_END );

				if( (nPara == -1) && !bParaAttrib )
				{
					if( NULL == pNewAttrSet )
					{
						const SfxItemSet aSet( pForwarder->GetAttribs( aSel ) );
						pOldAttrSet = new SfxItemSet( aSet );
						pNewAttrSet = new SfxItemSet( *pOldAttrSet->GetPool(), pOldAttrSet->GetRanges() );
					}

					setPropertyValue( pMap, *pValues, GetSelection(), *pOldAttrSet, *pNewAttrSet );

					if( pMap->nWID >= EE_ITEMS_START && pMap->nWID <= EE_ITEMS_END )
					{
						const SfxPoolItem* pItem;
						if( pNewAttrSet->GetItemState( pMap->nWID, sal_True, &pItem ) == SFX_ITEM_SET )
						{
							pOldAttrSet->Put( *pItem );
						}
					}
				}
				else
				{
					if( NULL == pNewParaSet )
					{
						const SfxItemSet aSet( pForwarder->GetParaAttribs( (USHORT)nTempPara ) );
						pOldParaSet = new SfxItemSet( aSet );
						pNewParaSet = new SfxItemSet( *pOldParaSet->GetPool(), pOldParaSet->GetRanges() );
					}

					setPropertyValue( pMap, *pValues, GetSelection(), *pOldParaSet, *pNewParaSet );

					if( pMap->nWID >= EE_ITEMS_START && pMap->nWID <= EE_ITEMS_END )
					{
						const SfxPoolItem* pItem;
						if( pNewParaSet->GetItemState( pMap->nWID, sal_True, &pItem ) == SFX_ITEM_SET )
						{
							pOldParaSet->Put( *pItem );
						}
					}

				}
			}
		}

		sal_Bool bNeedsUpdate = sal_False;

		if( pNewParaSet )
		{
			if( pNewParaSet->Count() )
			{
				while( nTempPara <= nEndPara )
				{
					SfxItemSet aSet( pForwarder->GetParaAttribs( (USHORT)nTempPara ) );
					aSet.Put( *pNewParaSet );
					pForwarder->SetParaAttribs( (USHORT)nTempPara, aSet );
					nTempPara++;
				}
				bNeedsUpdate = sal_True;
			}

			delete pNewParaSet;
			delete pOldParaSet;
		}

		if( pNewAttrSet )
		{
			if( pNewAttrSet->Count() )
			{
				pForwarder->QuickSetAttribs( *pNewAttrSet, GetSelection() );
				bNeedsUpdate = sal_True;
			}
			delete pNewAttrSet;
			delete pOldAttrSet;

		}

		if( bNeedsUpdate )
			GetEditSource()->UpdateData();
	}
}

uno::Sequence< uno::Any > SAL_CALL SvxUnoTextRangeBase::getPropertyValues( const uno::Sequence< ::rtl::OUString >& aPropertyNames ) throw (uno::RuntimeException)
{
	return _getPropertyValues( aPropertyNames, -1 );
}

uno::Sequence< uno::Any > SAL_CALL SvxUnoTextRangeBase::_getPropertyValues( const uno::Sequence< ::rtl::OUString >& aPropertyNames, sal_Int32 nPara ) throw (uno::RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );

	sal_Int32 nCount = aPropertyNames.getLength();


	uno::Sequence< uno::Any > aValues( nCount );

	SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : NULL;
	if( pForwarder )
	{
		SfxItemSet* pAttribs = NULL;
		if( nPara != -1 )
			pAttribs = pForwarder->GetParaAttribs( (USHORT)nPara ).Clone();
		else
			pAttribs = pForwarder->GetAttribs( GetSelection() ).Clone();

		pAttribs->ClearInvalidItems();

		const OUString* pPropertyNames = aPropertyNames.getConstArray();
		uno::Any* pValues = aValues.getArray();

		for( ; nCount; nCount--, pPropertyNames++, pValues++ )
		{
            const SfxItemPropertySimpleEntry* pMap = mpPropSet->getPropertyMapEntry( *pPropertyNames );
			if( pMap )
			{
				getPropertyValue( pMap, *pValues, *pAttribs );
			}
		}

		delete pAttribs;

	}

	return aValues;
}

void SAL_CALL SvxUnoTextRangeBase::addPropertiesChangeListener( const uno::Sequence< ::rtl::OUString >& , const uno::Reference< beans::XPropertiesChangeListener >& ) throw (uno::RuntimeException)
{
}

void SAL_CALL SvxUnoTextRangeBase::removePropertiesChangeListener( const uno::Reference< beans::XPropertiesChangeListener >& ) throw (uno::RuntimeException)
{
}

void SAL_CALL SvxUnoTextRangeBase::firePropertiesChangeEvent( const uno::Sequence< ::rtl::OUString >& , const uno::Reference< beans::XPropertiesChangeListener >& ) throw (uno::RuntimeException)
{
}

// beans::XPropertyState
beans::PropertyState SAL_CALL SvxUnoTextRangeBase::getPropertyState( const OUString& PropertyName )
	throw(beans::UnknownPropertyException, uno::RuntimeException)
{
	return _getPropertyState( PropertyName, -1 );
}

static sal_uInt16 aSvxUnoFontDescriptorWhichMap[] = { EE_CHAR_FONTINFO, EE_CHAR_FONTHEIGHT, EE_CHAR_ITALIC,
												  EE_CHAR_UNDERLINE, EE_CHAR_WEIGHT, EE_CHAR_STRIKEOUT,
												  EE_CHAR_WLM, 0 };

beans::PropertyState SAL_CALL SvxUnoTextRangeBase::_getPropertyState(const SfxItemPropertySimpleEntry* pMap, sal_Int32 nPara)
	throw( beans::UnknownPropertyException, uno::RuntimeException )
{
	if ( pMap )
	{
	    SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : NULL;
	    if( pForwarder )
	    {
		    SfxItemState eItemState = SFX_ITEM_UNKNOWN;
		    sal_uInt16 nWID = 0;

		    switch( pMap->nWID )
		    {
		    case WID_FONTDESC:
			    {
				    sal_uInt16* pWhichId = aSvxUnoFontDescriptorWhichMap;
				    SfxItemState eTempItemState;
				    while( *pWhichId )
				    {
					    if(nPara != -1)
						    eTempItemState = pForwarder->GetItemState( (USHORT)nPara, *pWhichId );
					    else
						    eTempItemState = pForwarder->GetItemState( GetSelection(), *pWhichId );

					    switch( eTempItemState )
					    {
					    case SFX_ITEM_DISABLED:
					    case SFX_ITEM_DONTCARE:
						    eItemState = SFX_ITEM_DONTCARE;
						    break;

					    case SFX_ITEM_DEFAULT:
						    if( eItemState != SFX_ITEM_DEFAULT )
						    {
							    if( eItemState == SFX_ITEM_UNKNOWN )
								    eItemState = SFX_ITEM_DEFAULT;
						    }
						    break;

					    case SFX_ITEM_READONLY:
					    case SFX_ITEM_SET:
						    if( eItemState != SFX_ITEM_SET )
						    {
							    if( eItemState == SFX_ITEM_UNKNOWN )
								    eItemState = SFX_ITEM_SET;
						    }
						    break;
					    default:
						    throw beans::UnknownPropertyException();
					    }

					    pWhichId++;
				    }
			    }
			    break;

		    case WID_NUMLEVEL:
            case WID_NUMBERINGSTARTVALUE:
            case WID_PARAISNUMBERINGRESTART:
			    eItemState = SFX_ITEM_SET;
                break;

		    default:
			    nWID = pMap->nWID;
		    }

		    if( nWID != 0 )
		    {
			    if( nPara != -1 )
				    eItemState = pForwarder->GetItemState( (USHORT)nPara, nWID );
			    else
				    eItemState = pForwarder->GetItemState( GetSelection(), nWID );
		    }

		    switch( eItemState )
		    {
		    case SFX_ITEM_DONTCARE:
		    case SFX_ITEM_DISABLED:
			    return beans::PropertyState_AMBIGUOUS_VALUE;
		    case SFX_ITEM_READONLY:
		    case SFX_ITEM_SET:
			    return beans::PropertyState_DIRECT_VALUE;
		    case SFX_ITEM_DEFAULT:
			    return beans::PropertyState_DEFAULT_VALUE;
//  			case SFX_ITEM_UNKNOWN:
		    }
        }
	}
	throw beans::UnknownPropertyException();
}

beans::PropertyState SAL_CALL SvxUnoTextRangeBase::_getPropertyState(const OUString& PropertyName, sal_Int32 nPara /* = -1 */)
	throw( beans::UnknownPropertyException, uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

    return _getPropertyState( mpPropSet->getPropertyMapEntry( PropertyName ), nPara);
}

uno::Sequence< beans::PropertyState > SAL_CALL SvxUnoTextRangeBase::getPropertyStates( const uno::Sequence< OUString >& aPropertyName )
	throw(beans::UnknownPropertyException, uno::RuntimeException)
{
	return _getPropertyStates( aPropertyName, -1 );
}

uno::Sequence< beans::PropertyState > SvxUnoTextRangeBase::_getPropertyStates(const uno::Sequence< OUString >& PropertyName, sal_Int32 nPara /* = -1 */)
	throw( beans::UnknownPropertyException, uno::RuntimeException )
{
	const sal_Int32 nCount = PropertyName.getLength();
	const OUString* pNames = PropertyName.getConstArray();

	uno::Sequence< beans::PropertyState > aRet( nCount );
	beans::PropertyState* pState = aRet.getArray();

	SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : NULL;
	if( pForwarder )
	{
		SfxItemSet* pSet = NULL;
		if( nPara != -1 )
		{
			pSet = new SfxItemSet( pForwarder->GetParaAttribs( (USHORT)nPara ) );
		}
		else
		{
			ESelection aSel( GetSelection() );
			CheckSelection( aSel, pForwarder );
			pSet = new SfxItemSet( pForwarder->GetAttribs( aSel, EditEngineAttribs_OnlyHard ) );
		}

		sal_Bool bUnknownPropertyFound = sal_False;
		for( sal_Int32 nIdx = 0; nIdx < nCount; nIdx++ )
		{
            const SfxItemPropertySimpleEntry* pMap = mpPropSet->getPropertyMapEntry( *pNames++ );
			if( NULL == pMap )
			{
				bUnknownPropertyFound = sal_True;
				break;
			}
            bUnknownPropertyFound = !_getOnePropertyStates(pSet, pMap, *pState++);
		}

		delete pSet;

		if( bUnknownPropertyFound )
			throw beans::UnknownPropertyException();
	}

	return aRet;
}

sal_Bool SvxUnoTextRangeBase::_getOnePropertyStates(const SfxItemSet* pSet, const SfxItemPropertySimpleEntry* pMap, beans::PropertyState& rState)
{
    sal_Bool bUnknownPropertyFound = sal_False;
    if(pSet && pMap)
    {
		SfxItemState eItemState = SFX_ITEM_UNKNOWN;
		sal_uInt16 nWID = 0;

		switch( pMap->nWID )
		{
			case WID_FONTDESC:
				{
					sal_uInt16* pWhichId = aSvxUnoFontDescriptorWhichMap;
					SfxItemState eTempItemState;
					while( *pWhichId )
					{
						eTempItemState = pSet->GetItemState( *pWhichId );

						switch( eTempItemState )
						{
						case SFX_ITEM_DISABLED:
						case SFX_ITEM_DONTCARE:
							eItemState = SFX_ITEM_DONTCARE;
							break;

						case SFX_ITEM_DEFAULT:
							if( eItemState != SFX_ITEM_DEFAULT )
							{
								if( eItemState == SFX_ITEM_UNKNOWN )
									eItemState = SFX_ITEM_DEFAULT;
							}
							break;

						case SFX_ITEM_READONLY:
						case SFX_ITEM_SET:
							if( eItemState != SFX_ITEM_SET )
							{
								if( eItemState == SFX_ITEM_UNKNOWN )
									eItemState = SFX_ITEM_SET;
							}
							break;
						default:
							bUnknownPropertyFound = sal_True;
							break;
						}

						pWhichId++;
					}
				}
				break;

			case WID_NUMLEVEL:
            case WID_NUMBERINGSTARTVALUE:
            case WID_PARAISNUMBERINGRESTART:
				eItemState = SFX_ITEM_SET;
				break;

			default:
				nWID = pMap->nWID;
		}

		if( bUnknownPropertyFound )
			return !bUnknownPropertyFound;

		if( nWID != 0 )
			eItemState = pSet->GetItemState( nWID, sal_False );

		switch( eItemState )
		{
				case SFX_ITEM_READONLY:
				case SFX_ITEM_SET:
					rState = beans::PropertyState_DIRECT_VALUE;
					break;
				case SFX_ITEM_DEFAULT:
					rState = beans::PropertyState_DEFAULT_VALUE;
					break;
//					case SFX_ITEM_UNKNOWN:
//					case SFX_ITEM_DONTCARE:
//					case SFX_ITEM_DISABLED:
				default:
					rState = beans::PropertyState_AMBIGUOUS_VALUE;
		}
    }
    return !bUnknownPropertyFound;
}

void SAL_CALL SvxUnoTextRangeBase::setPropertyToDefault( const OUString& PropertyName )
	throw(beans::UnknownPropertyException, uno::RuntimeException)
{
	_setPropertyToDefault( PropertyName, -1 );
}

void SvxUnoTextRangeBase::_setPropertyToDefault(const OUString& PropertyName, sal_Int32 nPara /* = -1 */)
	throw( beans::UnknownPropertyException, uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

	SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : NULL;

    if( pForwarder )
    {
        const SfxItemPropertySimpleEntry* pMap = mpPropSet->getPropertyMapEntry( PropertyName );
		if ( pMap )
        {
    		CheckSelection( maSelection, mpEditSource->GetTextForwarder() );
            _setPropertyToDefault( pForwarder, pMap, nPara );
            return;
        }
    }

	throw beans::UnknownPropertyException();
}

void SvxUnoTextRangeBase::_setPropertyToDefault(SvxTextForwarder* pForwarder, const SfxItemPropertySimpleEntry* pMap, sal_Int32 nPara )
	throw( beans::UnknownPropertyException, uno::RuntimeException )
{
	do
	{
		SfxItemSet aSet( *pForwarder->GetPool(), TRUE );

		if( pMap->nWID == WID_FONTDESC )
		{
			SvxUnoFontDescriptor::setPropertyToDefault( aSet );
		}
		else if( pMap->nWID == WID_NUMLEVEL )
		{
            // #101004# Call interface method instead of unsafe cast
            pForwarder->SetDepth( maSelection.nStartPara, -1 );
            return;
		}
        else if( pMap->nWID == WID_NUMBERINGSTARTVALUE )
        {
            pForwarder->SetNumberingStartValue( maSelection.nStartPara, -1 );
        }
        else if( pMap->nWID == WID_PARAISNUMBERINGRESTART )
        {
            pForwarder->SetParaIsNumberingRestart( maSelection.nStartPara, sal_False );
        }
		else
		{
			aSet.InvalidateItem( pMap->nWID );
		}

		if(nPara != -1)
			pForwarder->SetParaAttribs( (USHORT)nPara, aSet );
		else
			pForwarder->QuickSetAttribs( aSet, GetSelection() );

		GetEditSource()->UpdateData();

		return;
	}
	while(0);
}

uno::Any SAL_CALL SvxUnoTextRangeBase::getPropertyDefault( const OUString& aPropertyName )
	throw(beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );

	SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : NULL;
	if( pForwarder )
	{
        const SfxItemPropertySimpleEntry* pMap = mpPropSet->getPropertyMapEntry( aPropertyName );
		if( pMap )
		{
			SfxItemPool* pPool = pForwarder->GetPool();

			switch( pMap->nWID )
			{
			case WID_FONTDESC:
				return SvxUnoFontDescriptor::getPropertyDefault( pPool );

			case WID_NUMLEVEL:
                {
                    uno::Any aAny;
				    return aAny;
                }

            case WID_NUMBERINGSTARTVALUE:
                return uno::Any( (sal_Int16)-1 );

            case WID_PARAISNUMBERINGRESTART:
                return uno::Any( (sal_Bool)sal_False );

			default:
				{
					// Default aus ItemPool holen
					if(pPool->IsWhich(pMap->nWID))
					{
						SfxItemSet aSet( *pPool,	pMap->nWID, pMap->nWID);
						aSet.Put(pPool->GetDefaultItem(pMap->nWID));
						return mpPropSet->getPropertyValue(pMap, aSet);
					}
				}
			}
		}
	}
	throw beans::UnknownPropertyException();
}

// beans::XMultiPropertyStates
void SAL_CALL SvxUnoTextRangeBase::setAllPropertiesToDefault(  ) throw (uno::RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );

	SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : NULL;

    if( pForwarder )
    {
        PropertyEntryVector_t aEntries = mpPropSet->getPropertyMap()->getPropertyEntries();
        PropertyEntryVector_t::const_iterator aIt = aEntries.begin();
        while( aIt != aEntries.end() )
        {
            _setPropertyToDefault( pForwarder, &(*aIt), -1 ); 
            ++aIt;
        }
    }
}

void SAL_CALL SvxUnoTextRangeBase::setPropertiesToDefault( const uno::Sequence< OUString >& aPropertyNames ) throw (beans::UnknownPropertyException, uno::RuntimeException)
{
    sal_Int32 nCount = aPropertyNames.getLength();
    for( const OUString* pName = aPropertyNames.getConstArray(); nCount; pName++, nCount-- )
    {
        setPropertyToDefault( *pName );
    }
}

uno::Sequence< uno::Any > SAL_CALL SvxUnoTextRangeBase::getPropertyDefaults( const uno::Sequence< OUString >& aPropertyNames ) throw (beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException)
{
    sal_Int32 nCount = aPropertyNames.getLength();
    uno::Sequence< uno::Any > ret( nCount );
    uno::Any* pDefaults = ret.getArray();

    for( const OUString* pName = aPropertyNames.getConstArray(); nCount; pName++, nCount--, pDefaults++ )
    {
        *pDefaults = getPropertyDefault( *pName );
    }

    return ret;
}

// internal
void SvxUnoTextRangeBase::CollapseToStart(void) throw()
{
	CheckSelection( maSelection, mpEditSource->GetTextForwarder() );

	maSelection.nEndPara = maSelection.nStartPara;
	maSelection.nEndPos  = maSelection.nStartPos;
}

void SvxUnoTextRangeBase::CollapseToEnd(void) throw()
{
	CheckSelection( maSelection, mpEditSource->GetTextForwarder() );

	maSelection.nStartPara = maSelection.nEndPara;
	maSelection.nStartPos  = maSelection.nEndPos;
}

sal_Bool SvxUnoTextRangeBase::IsCollapsed(void) throw()
{
	CheckSelection( maSelection, mpEditSource->GetTextForwarder() );

	return ( maSelection.nStartPara == maSelection.nEndPara &&
			 maSelection.nStartPos  == maSelection.nEndPos );
}

sal_Bool SvxUnoTextRangeBase::GoLeft(sal_Int16 nCount, sal_Bool Expand) throw()
{
	CheckSelection( maSelection, mpEditSource->GetTextForwarder() );

	//	#75098# use end position, as in Writer (start is anchor, end is cursor)
	sal_uInt16 nNewPos = maSelection.nEndPos;
	sal_uInt16 nNewPar = maSelection.nEndPara;

	sal_Bool bOk = sal_True;
	SvxTextForwarder* pForwarder = NULL;
	while ( nCount > nNewPos && bOk )
	{
		if ( nNewPar == 0 )
			bOk = sal_False;
		else
		{
			if ( !pForwarder )
				pForwarder = mpEditSource->GetTextForwarder();	// erst hier, wenn's noetig ist...

			--nNewPar;
			nCount -= nNewPos + 1;
			nNewPos = pForwarder->GetTextLen( nNewPar );
		}
	}

	if ( bOk )
	{
		nNewPos = nNewPos - nCount;
		maSelection.nStartPara = nNewPar;
		maSelection.nStartPos  = nNewPos;
	}

	if (!Expand)
		CollapseToStart();

	return bOk;
}

sal_Bool SvxUnoTextRangeBase::GoRight(sal_Int16 nCount, sal_Bool Expand)  throw()
{
	SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : NULL;
	if( pForwarder )
	{
		CheckSelection( maSelection, pForwarder );


		sal_uInt16 nNewPos = maSelection.nEndPos + nCount;			//! Ueberlauf ???
		sal_uInt16 nNewPar = maSelection.nEndPara;

		sal_Bool bOk = sal_True;
		sal_uInt16 nParCount = pForwarder->GetParagraphCount();
		sal_uInt16 nThisLen = pForwarder->GetTextLen( nNewPar );
		while ( nNewPos > nThisLen && bOk )
		{
			if ( nNewPar + 1 >= nParCount )
				bOk = sal_False;
			else
			{
				nNewPos -= nThisLen+1;
				++nNewPar;
				nThisLen = pForwarder->GetTextLen( nNewPar );
			}
		}

		if (bOk)
		{
			maSelection.nEndPara = nNewPar;
			maSelection.nEndPos  = nNewPos;
		}

		if (!Expand)
			CollapseToEnd();

		return bOk;
	}
	return sal_False;
}

void SvxUnoTextRangeBase::GotoStart(sal_Bool Expand) throw()
{
	maSelection.nStartPara = 0;
	maSelection.nStartPos  = 0;

	if (!Expand)
		CollapseToStart();
}

void SvxUnoTextRangeBase::GotoEnd(sal_Bool Expand) throw()
{
	CheckSelection( maSelection, mpEditSource->GetTextForwarder() );

	SvxTextForwarder* pForwarder = mpEditSource ? mpEditSource->GetTextForwarder() : NULL;
	if( pForwarder )
	{

		sal_uInt16 nPar = pForwarder->GetParagraphCount();
		if (nPar)
			--nPar;

		maSelection.nEndPara = nPar;
		maSelection.nEndPos  = pForwarder->GetTextLen( nPar );

		if (!Expand)
			CollapseToEnd();
	}
}

// lang::XServiceInfo
sal_Bool SAL_CALL SvxUnoTextRangeBase::supportsService( const OUString& ServiceName )
	throw(uno::RuntimeException)
{
	return SvxServiceInfoHelper::supportsService( ServiceName, getSupportedServiceNames() );
}

uno::Sequence< OUString > SAL_CALL SvxUnoTextRangeBase::getSupportedServiceNames()
	throw(uno::RuntimeException)
{
    return getSupportedServiceNames_Static();
}

uno::Sequence< OUString > SAL_CALL SvxUnoTextRangeBase::getSupportedServiceNames_Static()
	SAL_THROW(())
{
	uno::Sequence< OUString >	aSeq;
	SvxServiceInfoHelper::addToSequence( aSeq, 3, "com.sun.star.style.CharacterProperties",
												  "com.sun.star.style.CharacterPropertiesComplex",
												  "com.sun.star.style.CharacterPropertiesAsian");
	return aSeq;
}

// XTextRangeCompare
sal_Int16 SAL_CALL SvxUnoTextRangeBase::compareRegionStarts( const uno::Reference< text::XTextRange >& xR1, const uno::Reference< text::XTextRange >& xR2 ) throw (lang::IllegalArgumentException, uno::RuntimeException)
{
	SvxUnoTextRangeBase* pR1 = SvxUnoTextRangeBase::getImplementation( xR1 );
	SvxUnoTextRangeBase* pR2 = SvxUnoTextRangeBase::getImplementation( xR2 );

	if( (pR1 == 0) || (pR2 == 0) )
		throw lang::IllegalArgumentException();

	const ESelection& r1 = pR1->maSelection;
	const ESelection& r2 = pR2->maSelection;

	if( r1.nStartPara == r2.nStartPara )
	{
		if( r1.nStartPos == r2.nStartPos )
			return 0;
		else
			return r1.nStartPos < r2.nStartPos ? 1 : -1;
	}
	else
	{
		return r1.nStartPara < r2.nStartPara ? 1 : -1;
	}
}

sal_Int16 SAL_CALL SvxUnoTextRangeBase::compareRegionEnds( const uno::Reference< text::XTextRange >& xR1, const uno::Reference< text::XTextRange >& xR2 ) throw (lang::IllegalArgumentException, uno::RuntimeException)
{
	SvxUnoTextRangeBase* pR1 = SvxUnoTextRangeBase::getImplementation( xR1 );
	SvxUnoTextRangeBase* pR2 = SvxUnoTextRangeBase::getImplementation( xR2 );

	if( (pR1 == 0) || (pR2 == 0) )
		throw lang::IllegalArgumentException();

	const ESelection& r1 = pR1->maSelection;
	const ESelection& r2 = pR2->maSelection;

	if( r1.nEndPara == r2.nEndPara )
	{
		if( r1.nEndPos == r2.nEndPos )
			return 0;
		else
			return r1.nEndPos < r2.nEndPos ? 1 : -1;
	}
	else
	{
		return r1.nEndPara < r2.nEndPara ? 1 : -1;
	}
}

// ====================================================================
// class SvxUnoTextRange
// ====================================================================

uno::Sequence< uno::Type > SvxUnoTextRange::maTypeSequence;

uno::Reference< uno::XInterface > SvxUnoTextRange_NewInstance()
{
	SvxUnoText aText;
	uno::Reference< text::XTextRange > xRange( new SvxUnoTextRange( aText ) );
#if (_MSC_VER < 1300)
	return xRange;
#else
	return (uno::Reference< uno::XInterface >)xRange;
#endif
}

SvxUnoTextRange::SvxUnoTextRange( const SvxUnoTextBase& rParent, sal_Bool bPortion /* = sal_False */ ) throw()
:SvxUnoTextRangeBase( rParent.GetEditSource(), bPortion ? ImplGetSvxTextPortionSvxPropertySet() : rParent.getPropertySet() ),
 mbPortion( bPortion )
{
	xParentText =  (text::XText*)&rParent;
}

SvxUnoTextRange::~SvxUnoTextRange() throw()
{
}

uno::Any SAL_CALL SvxUnoTextRange::queryAggregation( const uno::Type & rType )
	throw(uno::RuntimeException)
{
	QUERYINT( text::XTextRange );
	else if( rType == ::getCppuType((const uno::Reference< beans::XMultiPropertyStates >*)0) )
		return uno::makeAny(uno::Reference< beans::XMultiPropertyStates >(this));
	else if( rType == ::getCppuType((const uno::Reference< beans::XPropertySet >*)0) )
		return uno::makeAny(uno::Reference< beans::XPropertySet >(this));
	else QUERYINT( beans::XPropertyState );
	else QUERYINT( text::XTextRangeCompare );
	else if( rType == ::getCppuType((const uno::Reference< beans::XMultiPropertySet >*)0) )
		return uno::makeAny(uno::Reference< beans::XMultiPropertySet >(this));
	else QUERYINT( lang::XServiceInfo );
	else QUERYINT( lang::XTypeProvider );
	else QUERYINT( lang::XUnoTunnel );
	else
		return OWeakAggObject::queryAggregation( rType );
}

uno::Any SAL_CALL SvxUnoTextRange::queryInterface( const uno::Type & rType )
	throw(uno::RuntimeException)
{
	return OWeakAggObject::queryInterface(rType);
}

void SAL_CALL SvxUnoTextRange::acquire()
	throw( )
{
	OWeakAggObject::acquire();
}

void SAL_CALL SvxUnoTextRange::release()
	throw( )
{
	OWeakAggObject::release();
}

// XTypeProvider

uno::Sequence< uno::Type > SAL_CALL SvxUnoTextRange::getTypes()
	throw (uno::RuntimeException)
{
	if( maTypeSequence.getLength() == 0 )
	{
		maTypeSequence.realloc( 9 ); // !DANGER! keep this updated
		uno::Type* pTypes = maTypeSequence.getArray();

		*pTypes++ = ::getCppuType(( const uno::Reference< text::XTextRange >*)0);
		*pTypes++ = ::getCppuType(( const uno::Reference< beans::XPropertySet >*)0);
		*pTypes++ = ::getCppuType(( const uno::Reference< beans::XMultiPropertySet >*)0);
		*pTypes++ = ::getCppuType(( const uno::Reference< beans::XMultiPropertyStates >*)0);
		*pTypes++ = ::getCppuType(( const uno::Reference< beans::XPropertyState >*)0);
		*pTypes++ = ::getCppuType(( const uno::Reference< lang::XServiceInfo >*)0);
		*pTypes++ = ::getCppuType(( const uno::Reference< lang::XTypeProvider >*)0);
		*pTypes++ = ::getCppuType(( const uno::Reference< lang::XUnoTunnel >*)0);
		*pTypes++ = ::getCppuType(( const uno::Reference< text::XTextRangeCompare >*)0);
	}
	return maTypeSequence;
}

uno::Sequence< sal_Int8 > SAL_CALL SvxUnoTextRange::getImplementationId()
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

// XTextRange
uno::Reference< text::XText > SAL_CALL SvxUnoTextRange::getText()
	throw(uno::RuntimeException)
{
	return xParentText;
}

// lang::XServiceInfo
OUString SAL_CALL SvxUnoTextRange::getImplementationName()
	throw(uno::RuntimeException)
{
	return OUString(RTL_CONSTASCII_USTRINGPARAM("SvxUnoTextRange"));
}

// ====================================================================
// class SvxUnoText
// ====================================================================

// UNO3_GETIMPLEMENTATION2_IMPL( SvxUnoText, SvxUnoTextRangeBase );

uno::Sequence< uno::Type > SvxUnoTextBase::maTypeSequence;

SvxUnoTextBase::SvxUnoTextBase() throw()
: SvxUnoTextRangeBase( NULL )
{

}

SvxUnoTextBase::SvxUnoTextBase( const SvxItemPropertySet* _pSet  ) throw()
: SvxUnoTextRangeBase( _pSet )
{
}

SvxUnoTextBase::SvxUnoTextBase( const SvxEditSource* pSource, const SvxItemPropertySet* _pSet  ) throw()
: SvxUnoTextRangeBase( pSource, _pSet )
{
	ESelection aSelection;
	::GetSelection( aSelection, GetEditSource()->GetTextForwarder() );
	SetSelection( aSelection );
}

SvxUnoTextBase::SvxUnoTextBase( const SvxEditSource* pSource, const SvxItemPropertySet* _pSet, uno::Reference < text::XText > xParent ) throw()
: SvxUnoTextRangeBase( pSource, _pSet )
{
	xParentText = xParent;
	ESelection aSelection;
	::GetSelection( aSelection, GetEditSource()->GetTextForwarder() );
	SetSelection( aSelection );
}

SvxUnoTextBase::SvxUnoTextBase( const SvxUnoTextBase& rText ) throw()
:	SvxUnoTextRangeBase( rText )
, text::XTextAppend()
,   text::XTextCopy()
,	container::XEnumerationAccess()
,	text::XTextRangeMover()
,	lang::XTypeProvider()
{
	xParentText = rText.xParentText;
}

SvxUnoTextBase::~SvxUnoTextBase() throw()
{
}

// Internal
ESelection SvxUnoTextBase::InsertField( const SvxFieldItem& rField ) throw()
{
	SvxTextForwarder* pForwarder = GetEditSource() ? GetEditSource()->GetTextForwarder() : NULL;
	if( pForwarder )
	{
		pForwarder->QuickInsertField( rField, GetSelection() );
		GetEditSource()->UpdateData();

		//	Selektion anpassen
		//!	Wenn die EditEngine bei QuickInsertText die Selektion zurueckgeben wuerde,
		//!	waer's einfacher...

		CollapseToStart();
		GoRight( 1, sal_True );		// Feld ist immer 1 Zeichen
	}

	return GetSelection();	// Selektion mit dem Feld
}

// XInterface
uno::Any SAL_CALL SvxUnoTextBase::queryAggregation( const uno::Type & rType )
	throw(uno::RuntimeException)
{
	QUERYINT( text::XText );
	QUERYINT( text::XSimpleText );
	if( rType == ::getCppuType((const uno::Reference< text::XTextRange >*)0) )
        return uno::makeAny(uno::Reference< text::XTextRange >((text::XText*)(this)));
	QUERYINT(container::XEnumerationAccess );
	QUERYINT( container::XElementAccess );
	QUERYINT( beans::XMultiPropertyStates );
	QUERYINT( beans::XPropertySet );
	QUERYINT( beans::XMultiPropertySet );
	QUERYINT( beans::XPropertyState );
	QUERYINT( text::XTextRangeCompare );
	QUERYINT( lang::XServiceInfo );
	QUERYINT( text::XTextRangeMover );
    QUERYINT( text::XTextCopy );
    QUERYINT( text::XTextAppend );
    QUERYINT( text::XParagraphAppend );
    QUERYINT( text::XTextPortionAppend );
	QUERYINT( lang::XTypeProvider );
	QUERYINT( lang::XUnoTunnel );

    return uno::Any();
}

// XTypeProvider

uno::Sequence< uno::Type > SAL_CALL SvxUnoTextBase::getStaticTypes() throw()
{
	if( maTypeSequence.getLength() == 0 )
	{
        maTypeSequence.realloc( 15 ); // !DANGER! keep this updated
		uno::Type* pTypes = maTypeSequence.getArray();

		*pTypes++ = ::getCppuType(( const uno::Reference< text::XText >*)0);
		*pTypes++ = ::getCppuType(( const uno::Reference< container::XEnumerationAccess >*)0);
		*pTypes++ = ::getCppuType(( const uno::Reference< beans::XPropertySet >*)0);
		*pTypes++ = ::getCppuType(( const uno::Reference< beans::XMultiPropertySet >*)0);
		*pTypes++ = ::getCppuType(( const uno::Reference< beans::XMultiPropertyStates >*)0);
		*pTypes++ = ::getCppuType(( const uno::Reference< beans::XPropertyState >*)0);
		*pTypes++ = ::getCppuType(( const uno::Reference< text::XTextRangeMover >*)0);
        *pTypes++ = ::getCppuType(( const uno::Reference< text::XTextAppend >*)0);
        *pTypes++ = ::getCppuType(( const uno::Reference< text::XTextCopy >*)0);
        *pTypes++ = ::getCppuType(( const uno::Reference< text::XParagraphAppend >*)0);
        *pTypes++ = ::getCppuType(( const uno::Reference< text::XTextPortionAppend >*)0);
		*pTypes++ = ::getCppuType(( const uno::Reference< lang::XServiceInfo >*)0);
		*pTypes++ = ::getCppuType(( const uno::Reference< lang::XTypeProvider >*)0);
		*pTypes++ = ::getCppuType(( const uno::Reference< lang::XUnoTunnel >*)0);
		*pTypes++ = ::getCppuType(( const uno::Reference< text::XTextRangeCompare >*)0);
	}
	return maTypeSequence;
}

uno::Sequence< uno::Type > SAL_CALL SvxUnoTextBase::getTypes()
	throw (uno::RuntimeException)
{
	return getStaticTypes();
}

uno::Sequence< sal_Int8 > SAL_CALL SvxUnoTextBase::getImplementationId()
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

uno::Reference< text::XTextCursor > SvxUnoTextBase::createTextCursorBySelection( const ESelection& rSel )
{
	SvxUnoTextCursor* pCursor = new SvxUnoTextCursor( *this );
	uno::Reference< text::XTextCursor >  xCursor( pCursor );
	pCursor->SetSelection( rSel );
	return xCursor;
}

// XSimpleText

uno::Reference< text::XTextCursor > SAL_CALL SvxUnoTextBase::createTextCursor()
	throw(uno::RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );
	return new SvxUnoTextCursor( *this );
}

uno::Reference< text::XTextCursor > SAL_CALL SvxUnoTextBase::createTextCursorByRange( const uno::Reference< text::XTextRange >& aTextPosition )
	throw(uno::RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );

	uno::Reference< text::XTextCursor >  xCursor;

	if( aTextPosition.is() )
	{
		SvxUnoTextRangeBase* pRange = SvxUnoTextRangeBase::getImplementation( aTextPosition );
		if(pRange)
			xCursor = createTextCursorBySelection( pRange->GetSelection() );
	}

	return xCursor;
}

void SAL_CALL SvxUnoTextBase::insertString( const uno::Reference< text::XTextRange >& xRange, const OUString& aString, sal_Bool bAbsorb )
	throw(uno::RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );

	if( !xRange.is() )
		return;

	ESelection aSelection;
	::GetSelection( aSelection, GetEditSource()->GetTextForwarder() );
	SetSelection( aSelection );

	SvxUnoTextRangeBase* pRange = SvxUnoTextRange::getImplementation( xRange );
	if(pRange)
	{
		//	setString am SvxUnoTextRangeBase statt selber QuickInsertText und UpdateData,
		//	damit die Selektion am SvxUnoTextRangeBase angepasst wird.
		//!	Eigentlich muessten alle Cursor-Objekte dieses Textes angepasst werden!

		if (!bAbsorb)					// nicht ersetzen -> hinten anhaengen
			pRange->CollapseToEnd();

		pRange->setString( aString );

		pRange->CollapseToEnd();
	}
}

void SAL_CALL SvxUnoTextBase::insertControlCharacter( const uno::Reference< text::XTextRange >& xRange, sal_Int16 nControlCharacter, sal_Bool bAbsorb )
	throw(lang::IllegalArgumentException, uno::RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );

	SvxTextForwarder* pForwarder = GetEditSource() ? GetEditSource()->GetTextForwarder() : NULL;

	if( pForwarder )
	{
		ESelection aSelection;
		::GetSelection( aSelection, pForwarder );
		SetSelection( aSelection );

		switch( nControlCharacter )
		{
		case text::ControlCharacter::PARAGRAPH_BREAK:
		{
			const String aText( (sal_Unicode)13 );	// '\r' geht auf'm Mac nicht
			insertString( xRange, aText, bAbsorb );

			return;
		}
		case text::ControlCharacter::LINE_BREAK:
		{
			SvxUnoTextRangeBase* pRange = SvxUnoTextRange::getImplementation( xRange );
			if(pRange)
			{
				ESelection aRange = pRange->GetSelection();

				if( bAbsorb )
				{
					const String aEmpty;
					pForwarder->QuickInsertText( aEmpty, aRange );

					aRange.nEndPos = aRange.nStartPos;
					aRange.nEndPara = aRange.nStartPara;
				}
				else
				{
					aRange.nStartPos = aRange.nEndPos;
					aRange.nStartPara = aRange.nStartPara;
				}

				pForwarder->QuickInsertLineBreak( aRange );
				GetEditSource()->UpdateData();

				aRange.nEndPos += 1;
				if( !bAbsorb )
					aRange.nStartPos += 1;

				pRange->SetSelection( aRange );
			}
			return;
		}
		case text::ControlCharacter::APPEND_PARAGRAPH:
		{
			SvxUnoTextRangeBase* pRange = SvxUnoTextRange::getImplementation( xRange );
			if(pRange)
			{
				ESelection aRange = pRange->GetSelection();
//				ESelection aOldSelection = aRange;

				aRange.nStartPos  = pForwarder->GetTextLen( aRange.nStartPara );

				aRange.nEndPara = aRange.nStartPara;
				aRange.nEndPos  = aRange.nStartPos;

				pRange->SetSelection( aRange );
				const String aText( (sal_Unicode)13 );	// '\r' geht auf'm Mac nicht
				pRange->setString( aText );

				aRange.nStartPos = 0;
				aRange.nStartPara += 1;
				aRange.nEndPos = 0;
				aRange.nEndPara += 1;

				pRange->SetSelection( aRange );

				return;
			}
		}
		}
	}

	throw lang::IllegalArgumentException();
}

// XText
void SAL_CALL SvxUnoTextBase::insertTextContent( const uno::Reference< text::XTextRange >& xRange, const uno::Reference< text::XTextContent >& xContent, sal_Bool bAbsorb )
	throw(lang::IllegalArgumentException, uno::RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );

	SvxTextForwarder* pForwarder = GetEditSource() ? GetEditSource()->GetTextForwarder() : NULL;
	if( pForwarder )
	{

		SvxUnoTextRangeBase* pRange = SvxUnoTextRange::getImplementation( xRange );
		SvxUnoTextField* pField = SvxUnoTextField::getImplementation( xContent );

		if( pRange == NULL || pField == NULL )
			throw lang::IllegalArgumentException();

		ESelection aSelection = pRange->GetSelection();
		if( !bAbsorb )
		{
			aSelection.nStartPara = aSelection.nEndPara;
			aSelection.nStartPos  = aSelection.nEndPos;
		}

		SvxFieldData* pFieldData = pField->CreateFieldData();
		if( pFieldData == NULL )
			throw lang::IllegalArgumentException();

        SvxFieldItem aField( *pFieldData, EE_FEATURE_FIELD );
		pForwarder->QuickInsertField( aField, aSelection );
		GetEditSource()->UpdateData();

		pField->SetAnchor( uno::Reference< text::XTextRange >::query( (cppu::OWeakObject*)this ) );

        aSelection.nEndPos += 1;
        aSelection.nStartPos = aSelection.nEndPos;
        //maSelection = aSelection; //???
        pRange->SetSelection( aSelection );

		delete pFieldData;
	}
}

void SAL_CALL SvxUnoTextBase::removeTextContent( const uno::Reference< text::XTextContent >& ) throw(container::NoSuchElementException, uno::RuntimeException)
{
}

// XTextRange

uno::Reference< text::XText > SAL_CALL SvxUnoTextBase::getText()
	throw(uno::RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );

	if (GetEditSource())
	{
		ESelection aSelection;
		::GetSelection( aSelection, GetEditSource()->GetTextForwarder() );
		((SvxUnoTextBase*)this)->SetSelection( aSelection );
	}

	return (text::XText*)this;
}

uno::Reference< text::XTextRange > SAL_CALL SvxUnoTextBase::getStart()
	throw(uno::RuntimeException)
{
	return SvxUnoTextRangeBase::getStart();
}

uno::Reference< text::XTextRange > SAL_CALL SvxUnoTextBase::getEnd()
	throw(uno::RuntimeException)
{
	return SvxUnoTextRangeBase::getEnd();
}

OUString SAL_CALL SvxUnoTextBase::getString() throw( uno::RuntimeException )
{
	return SvxUnoTextRangeBase::getString();
}

void SAL_CALL SvxUnoTextBase::setString( const OUString& aString ) throw(uno::RuntimeException)
{
	SvxUnoTextRangeBase::setString(aString);
}


// XEnumerationAccess
uno::Reference< container::XEnumeration > SAL_CALL SvxUnoTextBase::createEnumeration()
	throw(uno::RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );

	ESelection aSelection;
	::GetSelection( aSelection, GetEditSource()->GetTextForwarder() );
	SetSelection( aSelection );

	uno::Reference< container::XEnumeration > xEnum( (container::XEnumeration*) new SvxUnoTextContentEnumeration( *this ) );
	return xEnum;
}

// XElementAccess ( container::XEnumerationAccess )
uno::Type SAL_CALL SvxUnoTextBase::getElementType(  ) throw(uno::RuntimeException)
{
	return ::getCppuType((const uno::Reference< text::XTextRange >*)0 );
}

sal_Bool SAL_CALL SvxUnoTextBase::hasElements(  ) throw(uno::RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );

	if(GetEditSource())
	{
		SvxTextForwarder* pForwarder = GetEditSource()->GetTextForwarder();
		if(pForwarder)
			return pForwarder->GetParagraphCount() != 0;
	}

	return sal_False;
}

// text::XTextRangeMover
void SAL_CALL SvxUnoTextBase::moveTextRange( const uno::Reference< text::XTextRange >&, sal_Int16 )
	throw(uno::RuntimeException)
{
}

void SvxPropertyValuesToItemSet(
		SfxItemSet &rItemSet,
		const uno::Sequence< beans::PropertyValue > rPropertyVaules,
        const SfxItemPropertySet *pPropSet,
        SvxTextForwarder *pForwarder /*needed for WID_NUMLEVEL*/,
        USHORT nPara /*needed for WID_NUMLEVEL*/)
	throw(lang::IllegalArgumentException, beans::UnknownPropertyException, uno::RuntimeException)
{
    sal_Int32 nProps = rPropertyVaules.getLength();
    const beans::PropertyValue *pProps = rPropertyVaules.getConstArray();
    for (sal_Int32 i = 0;  i < nProps;  ++i)
    {
        const SfxItemPropertySimpleEntry *pEntry = pPropSet->getPropertyMap()->getByName( pProps[i].Name );
        if (pEntry)
        {
            // Note: there is no need to take special care of the properties
            //      TextField (EE_FEATURE_FIELD) and
            //      TextPortionType (WID_PORTIONTYPE)
            //  since they are read-only and thus are already taken care of below.

            if (pEntry->nFlags & beans::PropertyAttribute::READONLY)
                // should be PropertyVetoException which is not yet defined for the new import API's functions
                throw uno::RuntimeException( OUString ( RTL_CONSTASCII_USTRINGPARAM ( "Property is read-only: " ) ) + pProps[i].Name, static_cast < cppu::OWeakObject * > ( 0 ) );
                //throw PropertyVetoException ( OUString ( RTL_CONSTASCII_USTRINGPARAM ( "Property is read-only: " ) ) + pProps[i].Name, static_cast < cppu::OWeakObject * > ( 0 ) );

            if (pEntry->nWID == WID_FONTDESC)
            {
                awt::FontDescriptor aDesc;
                if (pProps[i].Value >>= aDesc)
                    SvxUnoFontDescriptor::FillItemSet( aDesc, rItemSet );
            }
            else if (pEntry->nWID == WID_NUMLEVEL)
            {
                if (pForwarder)
                {
                    sal_Int16 nLevel = -1;
                    pProps[i].Value >>= nLevel;

					// #101004# Call interface method instead of unsafe cast
					if (!pForwarder->SetDepth( nPara, nLevel ))
						throw lang::IllegalArgumentException();
                }
            }
            else if (pEntry->nWID == WID_NUMBERINGSTARTVALUE )
            {
                if( pForwarder )
                {
                    sal_Int16 nStartValue = -1;
                    if( !(pProps[i].Value >>= nStartValue) )
						throw lang::IllegalArgumentException();

                    pForwarder->SetNumberingStartValue( nPara, nStartValue );
                }
            }
            else if (pEntry->nWID == WID_PARAISNUMBERINGRESTART )
            {
                if( pForwarder )
                {
                    sal_Bool bParaIsNumberingRestart = sal_False;
                    if( !(pProps[i].Value >>= bParaIsNumberingRestart) )
						throw lang::IllegalArgumentException();

                    pForwarder->SetParaIsNumberingRestart( nPara, bParaIsNumberingRestart );
                }
            }
            else
                pPropSet->setPropertyValue( pProps[i].Name, pProps[i].Value, rItemSet );
        }
        else
            throw beans::UnknownPropertyException(OUString ( RTL_CONSTASCII_USTRINGPARAM ( "Unknown property: " ) ) + pProps[i].Name, static_cast < cppu::OWeakObject * > ( 0 ) );
    }
}

// com::sun::star::text::XParagraphAppend (new import API)
uno::Reference< text::XTextRange > SAL_CALL SvxUnoTextBase::appendParagraph(
        const uno::Sequence< beans::PropertyValue >& rCharAndParaProps )
    throw (lang::IllegalArgumentException, beans::UnknownPropertyException, uno::RuntimeException)
{
    OGuard aGuard( Application::GetSolarMutex() );
	uno::Reference< text::XTextRange > xRet;
    SvxEditSource *pEditSource = GetEditSource();
    SvxTextForwarder *pTextForwarder = pEditSource ? pEditSource->GetTextForwarder() : 0;
    if (pTextForwarder)
    {
        USHORT nParaCount = pTextForwarder->GetParagraphCount();
        DBG_ASSERT( nParaCount > 0, "paragraph count is 0 or negative" );
		pTextForwarder->AppendParagraph();

        // set properties for new appended (now last) paragraph
        ESelection aSel( nParaCount, 0, nParaCount, 0 );
        SfxItemSet aItemSet( *pTextForwarder->GetEmptyItemSetPtr() );
        SvxPropertyValuesToItemSet( aItemSet, rCharAndParaProps, 
                            ImplGetSvxUnoOutlinerTextCursorSfxPropertySet(), 
                            pTextForwarder, 
                            nParaCount );
        pTextForwarder->QuickSetAttribs( aItemSet, aSel );
        pEditSource->UpdateData();
		SvxUnoTextRange* pRange = new SvxUnoTextRange( *this );
		xRet = pRange;
		pRange->SetSelection( aSel );
    }
	return xRet;
}

uno::Reference< text::XTextRange > SAL_CALL SvxUnoTextBase::finishParagraph(
        const uno::Sequence< beans::PropertyValue >& rCharAndParaProps )
    throw (lang::IllegalArgumentException, beans::UnknownPropertyException, uno::RuntimeException)
{
    OGuard aGuard( Application::GetSolarMutex() );

	uno::Reference< text::XTextRange > xRet;
    SvxEditSource *pEditSource = GetEditSource();
    SvxTextForwarder *pTextForwarder = pEditSource ? pEditSource->GetTextForwarder() : 0;
    if (pTextForwarder)
    {
        USHORT nParaCount = pTextForwarder->GetParagraphCount();
        DBG_ASSERT( nParaCount > 0, "paragraph count is 0 or negative" );
        pTextForwarder->AppendParagraph();

        // set properties for the previously last paragraph
        USHORT nPara = nParaCount - 1;
        ESelection aSel( nPara, 0, nPara, 0 );
        SfxItemSet aItemSet( *pTextForwarder->GetEmptyItemSetPtr() );
        SvxPropertyValuesToItemSet( aItemSet, rCharAndParaProps, 
                ImplGetSvxUnoOutlinerTextCursorSfxPropertySet(), pTextForwarder, nPara );
		pTextForwarder->QuickSetAttribs( aItemSet, aSel );
        pEditSource->UpdateData();
		SvxUnoTextRange* pRange = new SvxUnoTextRange( *this );
		xRet = pRange;
		pRange->SetSelection( aSel );
	}
	return xRet;
}

// com::sun::star::text::XTextPortionAppend (new import API)
uno::Reference< text::XTextRange > SAL_CALL SvxUnoTextBase::appendTextPortion(
        const ::rtl::OUString& rText,
        const uno::Sequence< beans::PropertyValue >& rCharAndParaProps )
    throw (lang::IllegalArgumentException, beans::UnknownPropertyException, uno::RuntimeException)
{
    OGuard aGuard( Application::GetSolarMutex() );

    SvxEditSource *pEditSource = GetEditSource();
    SvxTextForwarder *pTextForwarder = pEditSource ? pEditSource->GetTextForwarder() : 0;
	uno::Reference< text::XTextRange > xRet;
    if (pTextForwarder)
    {
        USHORT nParaCount = pTextForwarder->GetParagraphCount();
        DBG_ASSERT( nParaCount > 0, "paragraph count is 0 or negative" );
		USHORT nPara = nParaCount - 1;
        SfxItemSet aSet( pTextForwarder->GetParaAttribs( nPara ) );
		xub_StrLen nStart = pTextForwarder->AppendTextPortion( nPara, rText, aSet );
        pEditSource->UpdateData();
		xub_StrLen nEnd   = pTextForwarder->GetTextLen( nPara );

        // set properties for the new text portion
        ESelection aSel( nPara, nStart, nPara, nEnd );
        pTextForwarder->RemoveAttribs( aSel, sal_False, 0 );
        pEditSource->UpdateData();

        SfxItemSet aItemSet( *pTextForwarder->GetEmptyItemSetPtr() );
        SvxPropertyValuesToItemSet( aItemSet, rCharAndParaProps, 
                ImplGetSvxTextPortionSfxPropertySet(), pTextForwarder, nPara );
		pTextForwarder->QuickSetAttribs( aItemSet, aSel );
		SvxUnoTextRange* pRange = new SvxUnoTextRange( *this );
		xRet = pRange;
		pRange->SetSelection( aSel );
        const beans::PropertyValue* pProps = rCharAndParaProps.getConstArray();
        for( sal_Int32 nProp = 0; nProp < rCharAndParaProps.getLength(); ++nProp )
            pRange->setPropertyValue( pProps[nProp].Name, pProps[nProp].Value );
	}
	return xRet;
}
/*-- 25.03.2008 08:16:09---------------------------------------------------

  -----------------------------------------------------------------------*/
void SvxUnoTextBase::copyText(
    const uno::Reference< text::XTextCopy >& xSource ) throw ( uno::RuntimeException )
{
    OGuard aGuard( Application::GetSolarMutex() );
    uno::Reference< lang::XUnoTunnel > xUT( xSource, uno::UNO_QUERY );
    SvxEditSource *pEditSource = GetEditSource();
    SvxTextForwarder *pTextForwarder = pEditSource ? pEditSource->GetTextForwarder() : 0;
    if( !pTextForwarder )
        return;
    if( xUT.is() )
    {
        SvxUnoTextBase* pSource = reinterpret_cast<SvxUnoTextBase*>(sal::static_int_cast<sal_uIntPtr>(
                                                                    xUT->getSomething( SvxUnoTextBase::getUnoTunnelId())));
        SvxEditSource *pSourceEditSource = pSource->GetEditSource();
        SvxTextForwarder *pSourceTextForwarder = pSourceEditSource ? pSourceEditSource->GetTextForwarder() : 0;
        if( pSourceTextForwarder )
        {
            pTextForwarder->CopyText( *pSourceTextForwarder );
            pEditSource->UpdateData();
        }
    }
    else
    {
        uno::Reference< text::XText > xSourceText( xSource, uno::UNO_QUERY );
        if( xSourceText.is() )
        {
            setString( xSourceText->getString() );
        }
    }
}

// lang::XServiceInfo
OUString SAL_CALL SvxUnoTextBase::getImplementationName()
	throw(uno::RuntimeException)
{
	return OUString(RTL_CONSTASCII_USTRINGPARAM("SvxUnoTextBase"));
}

uno::Sequence< OUString > SAL_CALL SvxUnoTextBase::getSupportedServiceNames(  )
	throw(uno::RuntimeException)
{
    return getSupportedServiceNames_Static();
}

uno::Sequence< OUString > SAL_CALL SvxUnoTextBase::getSupportedServiceNames_Static(  )
	SAL_THROW(())
{
	uno::Sequence< OUString > aSeq( SvxUnoTextRangeBase::getSupportedServiceNames_Static() );
	SvxServiceInfoHelper::addToSequence( aSeq, 1, "com.sun.star.text.Text" );
	return aSeq;
}

const uno::Sequence< sal_Int8 > & SvxUnoTextBase::getUnoTunnelId() throw()
{
	static uno::Sequence< sal_Int8 > * pSeq = 0;
	if( !pSeq )
	{
		::osl::Guard< ::osl::Mutex > aGuard( ::osl::Mutex::getGlobalMutex() );
		if( !pSeq )
		{
			static uno::Sequence< sal_Int8 > aSeq( 16 );
			rtl_createUuid( (sal_uInt8*)aSeq.getArray(), 0, sal_True );
			pSeq = &aSeq;
		}
	}
	return *pSeq;
}

SvxUnoTextBase* SvxUnoTextBase::getImplementation( const uno::Reference< uno::XInterface >& xInt )
{
	uno::Reference< lang::XUnoTunnel > xUT( xInt, uno::UNO_QUERY );
	if( xUT.is() )
		return reinterpret_cast<SvxUnoTextBase*>(sal::static_int_cast<sal_uIntPtr>(xUT->getSomething( SvxUnoTextBase::getUnoTunnelId())));
	else
		return NULL;
}

sal_Int64 SAL_CALL SvxUnoTextBase::getSomething( const uno::Sequence< sal_Int8 >& rId ) throw(uno::RuntimeException) \
{
	if( rId.getLength() == 16 && 0 == rtl_compareMemory( getUnoTunnelId().getConstArray(),
														 rId.getConstArray(), 16 ) )
	{
		return sal::static_int_cast<sal_Int64>(reinterpret_cast<sal_uIntPtr>(this));
	}
	else
	{
		return SvxUnoTextRangeBase::getSomething( rId );
	}
}

// --------------------------------------------------------------------

SvxUnoText::SvxUnoText( ) throw()
{
}

SvxUnoText::SvxUnoText( const SvxItemPropertySet* _pSet ) throw()
: SvxUnoTextBase( _pSet )
{
}

SvxUnoText::SvxUnoText( const SvxEditSource* pSource, const SvxItemPropertySet* _pSet, uno::Reference < text::XText > xParent ) throw()
: SvxUnoTextBase( pSource, _pSet, xParent )
{
}

SvxUnoText::SvxUnoText( const SvxUnoText& rText ) throw()
: SvxUnoTextBase( rText )
, cppu::OWeakAggObject()
{
}

SvxUnoText::~SvxUnoText() throw()
{
}

uno::Sequence< uno::Type > SAL_CALL getStaticTypes() throw()
{
	return SvxUnoTextBase::getStaticTypes();
}

// uno::XInterface
uno::Any SAL_CALL SvxUnoText::queryAggregation( const uno::Type & rType ) throw( uno::RuntimeException )
{
	uno::Any aAny( SvxUnoTextBase::queryAggregation( rType ) );
	if( !aAny.hasValue() )
		aAny = OWeakAggObject::queryAggregation( rType );

	return aAny;
}

uno::Any SAL_CALL SvxUnoText::queryInterface( const uno::Type & rType ) throw( uno::RuntimeException )
{
	return OWeakAggObject::queryInterface( rType );
}

void SAL_CALL SvxUnoText::acquire() throw( )
{
	OWeakAggObject::acquire();
}

void SAL_CALL SvxUnoText::release() throw( )
{
	OWeakAggObject::release();
}

// lang::XTypeProvider
uno::Sequence< uno::Type > SAL_CALL SvxUnoText::getTypes(  ) throw( uno::RuntimeException )
{
	return SvxUnoTextBase::getTypes();
}

uno::Sequence< sal_Int8 > SAL_CALL SvxUnoText::getImplementationId(  ) throw( uno::RuntimeException )
{
	static uno::Sequence< sal_Int8 > aId;
	if( aId.getLength() == 0 )
	{
		aId.realloc( 16 );
		rtl_createUuid( (sal_uInt8 *)aId.getArray(), 0, sal_True );
	}
	return aId;
}

SvxUnoText* SvxUnoText::getImplementation( const uno::Reference< uno::XInterface >& xInt )
{
	uno::Reference< lang::XUnoTunnel > xUT( xInt, uno::UNO_QUERY );
	if( xUT.is() )
		return reinterpret_cast<SvxUnoText*>(sal::static_int_cast<sal_uIntPtr>(xUT->getSomething( SvxUnoText::getUnoTunnelId())));
	else
		return NULL;
}

const uno::Sequence< sal_Int8 > & SvxUnoText::getUnoTunnelId() throw()
{
	static uno::Sequence< sal_Int8 > * pSeq = 0;
	if( !pSeq )
	{
		::osl::Guard< ::osl::Mutex > aGuard( ::osl::Mutex::getGlobalMutex() );
		if( !pSeq )
		{
			static uno::Sequence< sal_Int8 > aSeq( 16 );
			rtl_createUuid( (sal_uInt8*)aSeq.getArray(), 0, sal_True );
			pSeq = &aSeq;
		}
	}
	return *pSeq;
}

sal_Int64 SAL_CALL SvxUnoText::getSomething( const uno::Sequence< sal_Int8 >& rId ) throw(uno::RuntimeException) \
{
	if( rId.getLength() == 16 && 0 == rtl_compareMemory( getUnoTunnelId().getConstArray(),
														 rId.getConstArray(), 16 ) )
	{
		return sal::static_int_cast<sal_Int64>(reinterpret_cast<sal_uIntPtr>(this));
	}
	else
	{
		return SvxUnoTextBase::getSomething( rId );
	}
}


// --------------------------------------------------------------------

SvxDummyTextSource::~SvxDummyTextSource()
{
};

SvxEditSource* SvxDummyTextSource::Clone() const
{
	return new SvxDummyTextSource();
}

SvxTextForwarder* SvxDummyTextSource::GetTextForwarder()
{
	return this;
}

void SvxDummyTextSource::UpdateData()
{
}

sal_uInt16 SvxDummyTextSource::GetParagraphCount() const
{
	return 0;
}

sal_uInt16 SvxDummyTextSource::GetTextLen( sal_uInt16 ) const
{
	return 0;
}

String SvxDummyTextSource::GetText( const ESelection& ) const
{
	return String();
}

SfxItemSet SvxDummyTextSource::GetAttribs( const ESelection&, BOOL ) const
{
    // AW: Very dangerous: The former implementation used a SfxItemPool created on the
    // fly which of course was deleted again ASAP. Thus, the returned SfxItemSet was using
    // a deleted Pool by design.
    return SfxItemSet(SdrObject::GetGlobalDrawObjectItemPool());
}

SfxItemSet SvxDummyTextSource::GetParaAttribs( sal_uInt16 ) const
{
	return GetAttribs(ESelection());
}

void SvxDummyTextSource::SetParaAttribs( sal_uInt16, const SfxItemSet& )
{
}

void SvxDummyTextSource::RemoveAttribs( const ESelection& , sal_Bool , sal_uInt16 )
{
}

void SvxDummyTextSource::GetPortions( sal_uInt16, SvUShorts& ) const
{
}

sal_uInt16 SvxDummyTextSource::GetItemState( const ESelection&, sal_uInt16 ) const
{
	return 0;
}

sal_uInt16 SvxDummyTextSource::GetItemState( sal_uInt16, sal_uInt16 ) const
{
	return 0;
}

SfxItemPool* SvxDummyTextSource::GetPool() const
{
	return NULL;
}

void SvxDummyTextSource::QuickInsertText( const String&, const ESelection& )
{
}

void SvxDummyTextSource::QuickInsertField( const SvxFieldItem&, const ESelection& )
{
}

void SvxDummyTextSource::QuickSetAttribs( const SfxItemSet&, const ESelection& )
{
}

void SvxDummyTextSource::QuickInsertLineBreak( const ESelection& )
{
};

XubString SvxDummyTextSource::CalcFieldValue( const SvxFieldItem&, sal_uInt16, sal_uInt16, Color*&, Color*& )
{
	return XubString();
}

sal_Bool SvxDummyTextSource::IsValid() const
{
	return sal_False;
}

void SvxDummyTextSource::SetNotifyHdl( const Link& )
{
}

LanguageType SvxDummyTextSource::GetLanguage( USHORT, USHORT ) const
{
    return LANGUAGE_DONTKNOW;
}

USHORT SvxDummyTextSource::GetFieldCount( USHORT ) const
{
    return 0;
}

EFieldInfo SvxDummyTextSource::GetFieldInfo( USHORT, USHORT ) const
{
    return EFieldInfo();
}

EBulletInfo SvxDummyTextSource::GetBulletInfo( USHORT ) const
{
    return EBulletInfo();
}

Rectangle SvxDummyTextSource::GetCharBounds( USHORT, USHORT ) const
{
    return Rectangle();
}

Rectangle SvxDummyTextSource::GetParaBounds( USHORT ) const
{
    return Rectangle();
}

MapMode SvxDummyTextSource::GetMapMode() const
{
    return MapMode();
}

OutputDevice* SvxDummyTextSource::GetRefDevice() const
{
    return NULL;
}

sal_Bool SvxDummyTextSource::GetIndexAtPoint( const Point&, USHORT&, USHORT& ) const
{
    return sal_False;
}

sal_Bool SvxDummyTextSource::GetWordIndices( USHORT, USHORT, USHORT&, USHORT& ) const
{
    return sal_False;
}

sal_Bool SvxDummyTextSource::GetAttributeRun( USHORT&, USHORT&, USHORT, USHORT ) const
{
    return sal_False;
}

USHORT SvxDummyTextSource::GetLineCount( USHORT ) const
{
    return 0;
}

USHORT SvxDummyTextSource::GetLineLen( USHORT, USHORT ) const
{
    return 0;
}

void SvxDummyTextSource::GetLineBoundaries( /*out*/USHORT &rStart, /*out*/USHORT &rEnd, USHORT /*nParagraph*/, USHORT /*nLine*/ ) const
{
    rStart = rEnd = 0;
}
    
USHORT SvxDummyTextSource::GetLineNumberAtIndex( USHORT /*nPara*/, USHORT /*nIndex*/ ) const
{
    return 0;
}    

sal_Bool SvxDummyTextSource::QuickFormatDoc( BOOL )
{
    return sal_False;
}

sal_Int16 SvxDummyTextSource::GetDepth( USHORT ) const
{
    return -1;
}

sal_Bool SvxDummyTextSource::SetDepth( USHORT, sal_Int16 nNewDepth )
{
    return nNewDepth == 0 ? sal_True : sal_False;
}

sal_Bool SvxDummyTextSource::Delete( const ESelection& )
{
    return sal_False;
}

sal_Bool SvxDummyTextSource::InsertText( const String&, const ESelection& )
{
    return sal_False;
}

const SfxItemSet * SvxDummyTextSource::GetEmptyItemSetPtr()
{
    return 0;
}

void SvxDummyTextSource::AppendParagraph()
{
}

xub_StrLen SvxDummyTextSource::AppendTextPortion( USHORT, const String &, const SfxItemSet & )
{
    return 0;
}

void  SvxDummyTextSource::CopyText(const SvxTextForwarder& )
{
}

