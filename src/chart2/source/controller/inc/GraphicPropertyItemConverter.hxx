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
#ifndef CHART_GRAPHICPROPERTYITEMCONVERTER_HXX
#define CHART_GRAPHICPROPERTYITEMCONVERTER_HXX

#include "ItemConverter.hxx"
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/beans/PropertyState.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>

class SdrModel;

namespace chart
{
namespace wrapper
{

class GraphicPropertyItemConverter :
        public ::comphelper::ItemConverter
{
public:
    enum eGraphicObjectType
    {
        FILLED_DATA_POINT,
        LINE_DATA_POINT,
        LINE_PROPERTIES,
        FILL_PROPERTIES,
        LINE_AND_FILL_PROPERTIES
    };

    GraphicPropertyItemConverter(
        const ::com::sun::star::uno::Reference<
        ::com::sun::star::beans::XPropertySet > & rPropertySet,
        SfxItemPool& rItemPool,
        SdrModel& rDrawModel,
        const ::com::sun::star::uno::Reference<
            ::com::sun::star::lang::XMultiServiceFactory > & xNamedPropertyContainerFactory,
        eGraphicObjectType eObjectType = FILLED_DATA_POINT );
    virtual ~GraphicPropertyItemConverter();

protected:
    virtual const USHORT * GetWhichPairs() const;
    virtual bool GetItemProperty( tWhichIdType nWhichId, tPropertyNameWithMemberId & rOutProperty ) const;

    virtual void FillSpecialItem( USHORT nWhichId, SfxItemSet & rOutItemSet ) const
        throw( ::com::sun::star::uno::Exception );
    virtual bool ApplySpecialItem( USHORT nWhichId, const SfxItemSet & rItemSet )
        throw( ::com::sun::star::uno::Exception );

private:
    eGraphicObjectType              m_eGraphicObjectType;
    SdrModel &                      m_rDrawModel;
    ::com::sun::star::uno::Reference<
            ::com::sun::star::lang::XMultiServiceFactory >  m_xNamedPropertyTableFactory;
};

} //  namespace wrapper
} //  namespace chart

// CHART_GRAPHICPROPERTYITEMCONVERTER_HXX
#endif
