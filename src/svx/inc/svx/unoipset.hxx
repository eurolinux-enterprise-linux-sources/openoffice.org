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

#ifndef _SVX_UNOIPSET_HXX_
#define _SVX_UNOIPSET_HXX_

#include <com/sun/star/beans/XPropertySetInfo.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include "svx/svxdllapi.h"
#include <svtools/itemprop.hxx>

class SvxIDPropertyCombineList;
class SdrItemPool;
class SfxItemSet;
class SvxShape;

class SVX_DLLPUBLIC SvxItemPropertySet
{
    SfxItemPropertyMap          m_aPropertyMap;
    mutable com::sun::star::uno::Reference<com::sun::star::beans::XPropertySetInfo> m_xInfo;
    const SfxItemPropertyMapEntry*  _pMap;
	SvxIDPropertyCombineList*	pCombiList;
	sal_Bool					mbConvertTwips;

public:
    SvxItemPropertySet( const SfxItemPropertyMapEntry *pMap, sal_Bool bConvertTwips = sal_False );
	~SvxItemPropertySet();

	// Methoden, die direkt mit dem ItemSet arbeiten
    ::com::sun::star::uno::Any getPropertyValue( const SfxItemPropertySimpleEntry* pMap, const SfxItemSet& rSet ) const;
    void setPropertyValue( const SfxItemPropertySimpleEntry* pMap, const ::com::sun::star::uno::Any& rVal, SfxItemSet& rSet ) const;

	// Methoden, die stattdessen Any benutzen
    ::com::sun::star::uno::Any getPropertyValue( const SfxItemPropertySimpleEntry* pMap ) const;
    void setPropertyValue( const SfxItemPropertySimpleEntry* pMap, const ::com::sun::star::uno::Any& rVal ) const;

	// Properties von einem anderen Set uebernehmen
	void ObtainSettingsFromPropertySet(const SvxItemPropertySet& rPropSet,  SfxItemSet& rSet, ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySet > xSet )const;
	sal_Bool AreThereOwnUsrAnys() const { return (pCombiList ? sal_True : sal_False); }
	::com::sun::star::uno::Any* GetUsrAnyForID(sal_uInt16 nWID) const;
	void AddUsrAnyForID(const ::com::sun::star::uno::Any& rAny, sal_uInt16 nWID);

	com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySetInfo > getPropertySetInfo() const;
    const SfxItemPropertyMapEntry* getPropertyMapEntries() const {return _pMap;}
    //void setPropertyMap( const SfxItemPropertyMapEntry *pMap ) { _pMap = pMap; }
    const SfxItemPropertyMap* getPropertyMap()const { return &m_aPropertyMap;}
    const SfxItemPropertySimpleEntry* getPropertyMapEntry(const ::rtl::OUString &rName) const;

    static com::sun::star::uno::Reference< com::sun::star::beans::XPropertySetInfo > getPropertySetInfo( const SfxItemPropertyMapEntry* pMap );
};

#endif // _SVX_UNOIPSET_HXX_

