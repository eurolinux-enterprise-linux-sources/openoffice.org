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
#ifndef CHART2_VIEW_LEGENDENTRYPROVIDER_HXX
#define CHART2_VIEW_LEGENDENTRYPROVIDER_HXX

#include <com/sun/star/chart2/LegendExpansion.hpp>

#ifndef _COM_SUN_STAR_CHART2_VIEWLEGENDENTRYP_HPP_
#include <com/sun/star/chart2/ViewLegendEntry.hpp>
#endif
#include <com/sun/star/uno/XComponentContext.hpp>

namespace chart
{

class LegendEntryProvider
{
public:
    virtual ::com::sun::star::uno::Sequence<
        ::com::sun::star::chart2::ViewLegendEntry > SAL_CALL createLegendEntries(
            ::com::sun::star::chart2::LegendExpansion eLegendExpansion,
            const ::com::sun::star::uno::Reference<
                ::com::sun::star::beans::XPropertySet >& xTextProperties,
            const ::com::sun::star::uno::Reference<
                ::com::sun::star::drawing::XShapes >& xTarget,
            const ::com::sun::star::uno::Reference<
                ::com::sun::star::lang::XMultiServiceFactory >& xShapeFactory,
            const ::com::sun::star::uno::Reference<
                ::com::sun::star::uno::XComponentContext >& xContext
                )
        throw (::com::sun::star::uno::RuntimeException) = 0;

private:
};

} //  namespace chart

// CHART2_VIEW_LEGENDENTRYPROVIDER_HXX
#endif
