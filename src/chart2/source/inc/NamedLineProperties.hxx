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
#ifndef CHART_NAMEDLINEPROPERTIES_HXX
#define CHART_NAMEDLINEPROPERTIES_HXX

#error "Deprecated, do not include this file"

#include "PropertyHelper.hxx"
#include "FastPropertyIdRanges.hxx"
#include <com/sun/star/beans/Property.hpp>

#include <vector>

namespace chart
{


// @depreated !!
class NamedLineProperties
{
public:
    // FastProperty Ids for properties
    enum
    {
        // com.sun.star.drawing.LineProperties (only named properties)
        PROP_LINE_DASH_NAME = FAST_PROPERTY_ID_START_NAMED_LINE_PROP,
        PROP_LINE_END_NAME,
        PROP_LINE_START_NAME,

        FAST_PROPERTY_ID_END_NAMED_LINE_PROP
    };

    static void AddPropertiesToVector(
        ::std::vector< ::com::sun::star::beans::Property > & rOutProperties,
        bool bIncludeLineEnds = false );

    static void AddDefaultsToMap( ::chart::tPropertyValueMap & rOutMap,
                                  bool bIncludeLineEnds = false );

    //will return e.g. "LineDashName" for PROP_LINE_DASH_NAME
    static ::rtl::OUString GetPropertyNameForHandle( sal_Int32 nHandle );

private:
    // not implemented
    NamedLineProperties();
};

} //  namespace chart

// CHART_NAMEDLINEPROPERTIES_HXX
#endif
