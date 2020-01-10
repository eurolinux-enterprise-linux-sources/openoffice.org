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
#ifndef CHART2_OBJECTHIERARCHY_HXX
#define CHART2_OBJECTHIERARCHY_HXX

#include <rtl/ustring.hxx>
#include <com/sun/star/chart2/XChartDocument.hpp>
#include <com/sun/star/awt/KeyEvent.hpp>

#include <memory>
#include <vector>

namespace chart
{

class ExplicitValueProvider;

namespace impl
{
class ImplObjectHierarchy;
}

class ObjectHierarchy
{
public:
    typedef ::rtl::OUString tCID;
    typedef ::std::vector< tCID > tChildContainer;

    /** @param bFlattenDiagram
            If <TRUE/>, the content of the diaram (data series, wall, floor,
            etc.) is treated as being at the same level as the diagram. (This is
            used for keyboard navigation).
     */
    explicit ObjectHierarchy(
        const ::com::sun::star::uno::Reference<
            ::com::sun::star::chart2::XChartDocument > & xChartDocument,
        ExplicitValueProvider * pExplicitValueProvider = 0,
        bool bFlattenDiagram = false,
        bool bOrderingForElementSelector = false );
    ~ObjectHierarchy();

    static tCID      getRootNodeCID();
    static bool      isRootNode( const tCID & rCID );

    /// equal to getChildren( getRootNodeCID())
    tChildContainer  getTopLevelChildren() const;
    bool             hasChildren( const tCID & rParent ) const;
    tChildContainer  getChildren( const tCID & rParent ) const;

    tChildContainer  getSiblings( const tCID & rNode ) const;

    /// The result is empty, if the node cannot be found in the tree
    tCID             getParent( const tCID & rNode ) const;
    /// @returns -1, if no parent can be determined
    sal_Int32        getIndexInParent( const tCID & rNode ) const;

private:
    
    ::std::auto_ptr< impl::ImplObjectHierarchy > m_apImpl;
};

class ObjectKeyNavigation
{
public:
    explicit ObjectKeyNavigation( const ObjectHierarchy::tCID & rCurrentCID,
                                  const ::com::sun::star::uno::Reference<
                                      ::com::sun::star::chart2::XChartDocument > & xChartDocument,
                                  ExplicitValueProvider * pExplicitValueProvider = 0 );

    bool handleKeyEvent( const ::com::sun::star::awt::KeyEvent & rEvent );
    ObjectHierarchy::tCID getCurrentSelection() const;

private:
    void setCurrentSelection( const ObjectHierarchy::tCID & rCID );
    bool first();
    bool last();
    bool next();
    bool previous();
    bool up();
    bool down();
    bool veryFirst();
    bool veryLast();

    ObjectHierarchy::tCID m_aCurrentCID;
    ::com::sun::star::uno::Reference<
            ::com::sun::star::chart2::XChartDocument > m_xChartDocument;
    ExplicitValueProvider * m_pExplicitValueProvider;
    bool m_bStepDownInDiagram;
};

} //  namespace chart

// CHART2_OBJECTHIERARCHY_HXX
#endif
