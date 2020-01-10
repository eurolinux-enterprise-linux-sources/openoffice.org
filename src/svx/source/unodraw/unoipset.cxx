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
#include <com/sun/star/beans/XPropertySet.hpp>
#include <svtools/eitem.hxx>
#include <tools/list.hxx>

#include <hash_map>
#include <vector>
#include <svtools/itemprop.hxx>

#include <svx/unoipset.hxx>
#include <svx/svdpool.hxx>
#include <svx/svxids.hrc>
#include <svx/deflt3d.hxx>
#include <svx/unoshprp.hxx>
#include <svx/editeng.hxx>
#include "unoapi.hxx"
#include <svx/svdobj.hxx>

#include <algorithm>

using namespace ::com::sun::star;
using namespace ::rtl;

//----------------------------------------------------------------------

struct SfxItemPropertyMapEntryHash
{
    size_t operator()(const SfxItemPropertyMapEntry* pMap) const { return (size_t)pMap; }
};

//----------------------------------------------------------------------

struct SvxIDPropertyCombine
{
	sal_uInt16	nWID;
	uno::Any	aAny;
};

DECLARE_LIST( SvxIDPropertyCombineList, SvxIDPropertyCombine * )

SvxItemPropertySet::SvxItemPropertySet( const SfxItemPropertyMapEntry* pMap, sal_Bool bConvertTwips )
:   m_aPropertyMap( pMap ),
    _pMap(pMap), mbConvertTwips(bConvertTwips)
{
	pCombiList = NULL;
}

//----------------------------------------------------------------------
SvxItemPropertySet::~SvxItemPropertySet()
{
/*
	if(pItemPool)
		delete pItemPool;
	pItemPool = NULL;
*/

	if(pCombiList)
		delete pCombiList;
	pCombiList = NULL;
}

//----------------------------------------------------------------------
uno::Any* SvxItemPropertySet::GetUsrAnyForID(sal_uInt16 nWID) const
{
	if(pCombiList && pCombiList->Count())
	{
		SvxIDPropertyCombine* pActual = pCombiList->First();
		while(pActual)
		{
			if(pActual->nWID == nWID)
				return &pActual->aAny;
			pActual = pCombiList->Next();

		}
	}
	return NULL;
}

//----------------------------------------------------------------------
void SvxItemPropertySet::AddUsrAnyForID(const uno::Any& rAny, sal_uInt16 nWID)
{
	if(!pCombiList)
		pCombiList = new SvxIDPropertyCombineList();

	SvxIDPropertyCombine* pNew = new SvxIDPropertyCombine;
	pNew->nWID = nWID;
	pNew->aAny = rAny;
	pCombiList->Insert(pNew);
}

//----------------------------------------------------------------------
void SvxItemPropertySet::ObtainSettingsFromPropertySet(const SvxItemPropertySet& rPropSet,
  SfxItemSet& rSet, uno::Reference< beans::XPropertySet > xSet ) const 
{
	if(rPropSet.AreThereOwnUsrAnys())
	{
        const SfxItemPropertyMap* pSrc = rPropSet.getPropertyMap();
        PropertyEntryVector_t aSrcPropVector = pSrc->getPropertyEntries();
        PropertyEntryVector_t::const_iterator aSrcIt = aSrcPropVector.begin();
		while(aSrcIt != aSrcPropVector.end())
		{
			if(aSrcIt->nWID)
			{
				uno::Any* pUsrAny = rPropSet.GetUsrAnyForID(aSrcIt->nWID);
				if(pUsrAny)
				{
					// Aequivalenten Eintrag in pDst suchen
                    const SfxItemPropertySimpleEntry* pEntry = m_aPropertyMap.getByName( aSrcIt->sName );
					if(pEntry)
					{
						// entry found
						if(pEntry->nWID >= OWN_ATTR_VALUE_START && pEntry->nWID <= OWN_ATTR_VALUE_END)
						{
							// Special ID im PropertySet, kann nur direkt am
							// Objekt gesetzt werden+
							xSet->setPropertyValue( aSrcIt->sName, *pUsrAny);
						}
						else
						{
							if(rSet.GetPool()->IsWhich(pEntry->nWID))
								rSet.Put(rSet.GetPool()->GetDefaultItem(pEntry->nWID));

							// setzen
                            setPropertyValue(pEntry, *pUsrAny, rSet);
						}
					}
				}
			}

			// next entry
			++aSrcIt;
		}
	}
}

/** this function checks if a SFX_METRIC_ITEM realy needs to be converted.
	This check is for items that store either metric values if theire positiv
	or percentage if theire negativ.
*/
sal_Bool SvxUnoCheckForConversion( const SfxItemSet&, sal_Int32 nWID, const uno::Any& rVal )
{
	sal_Bool bConvert = sal_True; // the default is that all metric items must be converted

	switch( nWID )
	{
	case XATTR_FILLBMP_SIZEX:
	case XATTR_FILLBMP_SIZEY:
		{
			sal_Int32 nValue = 0;
			if( rVal >>= nValue )
				bConvert = nValue > 0;
			break;
		}
	}

	// the default is to always
	return bConvert;
}

//----------------------------------------------------------------------
uno::Any SvxItemPropertySet::getPropertyValue( const SfxItemPropertySimpleEntry* pMap, const SfxItemSet& rSet ) const
{
	uno::Any aVal;
	if(!pMap || !pMap->nWID)
		return aVal;

	// item holen
	const SfxPoolItem* pItem = 0;
	SfxItemPool* pPool = rSet.GetPool();

	rSet.GetItemState( pMap->nWID, pMap->nWID != SDRATTR_XMLATTRIBUTES, &pItem );

	if( NULL == pItem && pPool )
	{
		pItem = &(pPool->GetDefaultItem( pMap->nWID ));
	}

	const SfxMapUnit eMapUnit = pPool ? pPool->GetMetric((USHORT)pMap->nWID) : SFX_MAPUNIT_100TH_MM;

	BYTE nMemberId = pMap->nMemberId & (~SFX_METRIC_ITEM);
	if( eMapUnit == SFX_MAPUNIT_100TH_MM )
		nMemberId &= (~CONVERT_TWIPS);

	// item-Wert als UnoAny zurueckgeben
	if(pItem)
	{
		pItem->QueryValue( aVal, nMemberId );

		if( pMap->nMemberId & SFX_METRIC_ITEM )
		{
			// check for needed metric translation
			if(pMap->nMemberId & SFX_METRIC_ITEM && eMapUnit != SFX_MAPUNIT_100TH_MM)
			{
				if( SvxUnoCheckForConversion( rSet, pMap->nWID, aVal ) )
					SvxUnoConvertToMM( eMapUnit, aVal );
			}			
		}
		// convert typeless SfxEnumItem to enum type
		else if ( pMap->pType->getTypeClass() == uno::TypeClass_ENUM &&
			  aVal.getValueType() == ::getCppuType((const sal_Int32*)0) )
		{
			sal_Int32 nEnum;
			aVal >>= nEnum;

			aVal.setValue( &nEnum, *pMap->pType );
		}
	}
	else
	{
		DBG_ERROR( "No SfxPoolItem found for property!" );
	}

	return aVal;
}

//----------------------------------------------------------------------
void SvxItemPropertySet::setPropertyValue( const SfxItemPropertySimpleEntry* pMap, const uno::Any& rVal, SfxItemSet& rSet ) const
{
	if(!pMap || !pMap->nWID)
		return;

	// item holen
	const SfxPoolItem* pItem = 0;
	SfxPoolItem *pNewItem = 0;
	SfxItemState eState = rSet.GetItemState( pMap->nWID, sal_True, &pItem );
	SfxItemPool* pPool = rSet.GetPool();

	// UnoAny in item-Wert stecken
	if(eState < SFX_ITEM_DEFAULT || pItem == NULL)
	{
		if( pPool == NULL )
		{
			DBG_ERROR( "No default item and no pool?" );
			return;
		}

		pItem = &pPool->GetDefaultItem( pMap->nWID );
	}

	DBG_ASSERT( pItem, "Got no default for item!" );
	if( pItem )
	{
		uno::Any aValue( rVal );

		const SfxMapUnit eMapUnit = pPool ? pPool->GetMetric((USHORT)pMap->nWID) : SFX_MAPUNIT_100TH_MM;

		if( pMap->nMemberId & SFX_METRIC_ITEM )
		{
			// check for needed metric translation
			if(pMap->nMemberId & SFX_METRIC_ITEM && eMapUnit != SFX_MAPUNIT_100TH_MM)
			{
				if( SvxUnoCheckForConversion( rSet, pMap->nWID, aValue ) )
					SvxUnoConvertFromMM( eMapUnit, aValue );
			}			
		}

		pNewItem = pItem->Clone();

		BYTE nMemberId = pMap->nMemberId & (~SFX_METRIC_ITEM);
		if( eMapUnit == SFX_MAPUNIT_100TH_MM )
			nMemberId &= (~CONVERT_TWIPS);

		if( pNewItem->PutValue( aValue, nMemberId ) )
		{
			// neues item in itemset setzen
			rSet.Put( *pNewItem, pMap->nWID );
		}
		delete pNewItem;
	}
}

//----------------------------------------------------------------------
uno::Any SvxItemPropertySet::getPropertyValue( const SfxItemPropertySimpleEntry* pMap ) const
{
	// Schon ein Wert eingetragen? Dann schnell fertig
	uno::Any* pUsrAny = GetUsrAnyForID(pMap->nWID);
	if(pUsrAny)
		return *pUsrAny;

	// Noch kein UsrAny gemerkt, generiere Default-Eintrag und gib
	// diesen zurueck

	SdrItemPool& rItemPool = SdrObject::GetGlobalDrawObjectItemPool();
	const SfxMapUnit eMapUnit = rItemPool.GetMetric((USHORT)pMap->nWID);
	BYTE nMemberId = pMap->nMemberId & (~SFX_METRIC_ITEM);
	if( eMapUnit == SFX_MAPUNIT_100TH_MM )
		nMemberId &= (~CONVERT_TWIPS);

	uno::Any aVal;
	SfxItemSet aSet( rItemPool, pMap->nWID, pMap->nWID);

	if( (pMap->nWID < OWN_ATTR_VALUE_START) && (pMap->nWID > OWN_ATTR_VALUE_END ) )
	{
		// Default aus ItemPool holen
		if(rItemPool.IsWhich(pMap->nWID))
			aSet.Put(rItemPool.GetDefaultItem(pMap->nWID));
	}

	if(aSet.Count())
	{
		const SfxPoolItem* pItem = NULL;
		SfxItemState eState = aSet.GetItemState( pMap->nWID, sal_True, &pItem );
		if(eState >= SFX_ITEM_DEFAULT && pItem)
		{
			pItem->QueryValue( aVal, nMemberId );
			((SvxItemPropertySet*)this)->AddUsrAnyForID(aVal, pMap->nWID);
		}
	}

	if( pMap->nMemberId & SFX_METRIC_ITEM )
	{
		// check for needed metric translation
		if(pMap->nMemberId & SFX_METRIC_ITEM && eMapUnit != SFX_MAPUNIT_100TH_MM)
		{
			SvxUnoConvertToMM( eMapUnit, aVal );
		}			
	}

	if ( pMap->pType->getTypeClass() == uno::TypeClass_ENUM &&
		  aVal.getValueType() == ::getCppuType((const sal_Int32*)0) )
	{
		sal_Int32 nEnum;
		aVal >>= nEnum;

		aVal.setValue( &nEnum, *pMap->pType );
	}

	return aVal;
}

//----------------------------------------------------------------------

void SvxItemPropertySet::setPropertyValue( const SfxItemPropertySimpleEntry* pMap, const uno::Any& rVal ) const
{
	uno::Any* pUsrAny = GetUsrAnyForID(pMap->nWID);
	if(!pUsrAny)
		((SvxItemPropertySet*)this)->AddUsrAnyForID(rVal, pMap->nWID);
	else
		*pUsrAny = rVal;
}

//----------------------------------------------------------------------

const SfxItemPropertySimpleEntry* SvxItemPropertySet::getPropertyMapEntry(const OUString &rName) const
{
    return m_aPropertyMap.getByName( rName );
 }

//----------------------------------------------------------------------

uno::Reference< beans::XPropertySetInfo >  SvxItemPropertySet::getPropertySetInfo() const
{
    if( !m_xInfo.is() )
        m_xInfo = new SfxItemPropertySetInfo( &m_aPropertyMap );
    return m_xInfo;
}

//----------------------------------------------------------------------

#ifndef TWIPS_TO_MM
#define	TWIPS_TO_MM(val) ((val * 127 + 36) / 72)
#endif
#ifndef MM_TO_TWIPS
#define	MM_TO_TWIPS(val) ((val * 72 + 63) / 127)
#endif

/** converts the given any with a metric to 100th/mm if needed */
void SvxUnoConvertToMM( const SfxMapUnit eSourceMapUnit, uno::Any & rMetric ) throw()
{
	// map the metric of the itempool to 100th mm
	switch(eSourceMapUnit)
	{
		case SFX_MAPUNIT_TWIP :
		{
			switch( rMetric.getValueTypeClass() )
			{
			case uno::TypeClass_BYTE:
				rMetric <<= (sal_Int8)(TWIPS_TO_MM(*(sal_Int8*)rMetric.getValue()));
				break;
			case uno::TypeClass_SHORT:
				rMetric <<= (sal_Int16)(TWIPS_TO_MM(*(sal_Int16*)rMetric.getValue()));
				break;
			case uno::TypeClass_UNSIGNED_SHORT:
				rMetric <<= (sal_uInt16)(TWIPS_TO_MM(*(sal_uInt16*)rMetric.getValue()));
				break;
			case uno::TypeClass_LONG:
				rMetric <<= (sal_Int32)(TWIPS_TO_MM(*(sal_Int32*)rMetric.getValue()));
				break;
			case uno::TypeClass_UNSIGNED_LONG:
				rMetric <<= (sal_uInt32)(TWIPS_TO_MM(*(sal_uInt32*)rMetric.getValue()));
				break;
			default:
				DBG_ERROR("AW: Missing unit translation to 100th mm!");
			}
			break;
		}
		default:
		{
			DBG_ERROR("AW: Missing unit translation to 100th mm!");
		}
	}
}

//----------------------------------------------------------------------

/** converts the given any with a metric from 100th/mm to the given metric if needed */
void SvxUnoConvertFromMM( const SfxMapUnit eDestinationMapUnit, uno::Any & rMetric ) throw()
{
	switch(eDestinationMapUnit)
	{
		case SFX_MAPUNIT_TWIP :
		{
			switch( rMetric.getValueTypeClass() )
			{
				case uno::TypeClass_BYTE:
					rMetric <<= (sal_Int8)(MM_TO_TWIPS(*(sal_Int8*)rMetric.getValue()));
					break;
				case uno::TypeClass_SHORT:
					rMetric <<= (sal_Int16)(MM_TO_TWIPS(*(sal_Int16*)rMetric.getValue()));
					break;
				case uno::TypeClass_UNSIGNED_SHORT:
					rMetric <<= (sal_uInt16)(MM_TO_TWIPS(*(sal_uInt16*)rMetric.getValue()));
					break;
				case uno::TypeClass_LONG:
					rMetric <<= (sal_Int32)(MM_TO_TWIPS(*(sal_Int32*)rMetric.getValue()));
					break;
				case uno::TypeClass_UNSIGNED_LONG:
					rMetric <<= (sal_uInt32)(MM_TO_TWIPS(*(sal_uInt32*)rMetric.getValue()));
					break;
				default:
					DBG_ERROR("AW: Missing unit translation to 100th mm!");
			}
			break;
		}
		default:
		{
			DBG_ERROR("AW: Missing unit translation to PoolMetrics!");
		}
	}
}

