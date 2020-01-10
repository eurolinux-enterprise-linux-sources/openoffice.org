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

#ifndef LAYOUT_AWT_VCLXTABCONTROLLER_HXX
#define LAYOUT_AWT_VCLXTABCONTROLLER_HXX

#include <com/sun/star/awt/XSimpleTabController.hpp>
#include <comphelper/uno3.hxx>
#include <layout/core/box-base.hxx>
#include <map>
#include <toolkit/awt/vclxwindow.hxx>

class TabControl;

namespace layoutimpl
{

typedef ::cppu::ImplHelper1 <   ::com::sun::star::awt::XSimpleTabController
                                >   VCLXTabControl_Base;

class VCLXTabControl :public VCLXWindow
                     ,public VCLXTabControl_Base
                     ,public Box_Base
{
    int mTabId;
    bool bRealized;

public:
    VCLXTabControl();

    void AddChild (css::uno::Reference <css::awt::XLayoutConstrains> const &);

protected:
    ~VCLXTabControl();

    // XInterface
    DECLARE_XINTERFACE()

    // XTypeProvider
    DECLARE_XTYPEPROVIDER()

    // XComponent
    void SAL_CALL dispose() throw(::com::sun::star::uno::RuntimeException);

    virtual void SAL_CALL draw( sal_Int32 nX, sal_Int32 nY ) throw (::com::sun::star::uno::RuntimeException);

    // XSimpleTabController
    virtual ::sal_Int32 SAL_CALL insertTab() throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL removeTab( ::sal_Int32 ID ) throw (::com::sun::star::lang::IndexOutOfBoundsException, ::com::sun::star::uno::RuntimeException);

    virtual void SAL_CALL setTabProps( ::sal_Int32 ID, const ::com::sun::star::uno::Sequence< ::com::sun::star::beans::NamedValue >& Properties ) throw (::com::sun::star::lang::IndexOutOfBoundsException, ::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Sequence< ::com::sun::star::beans::NamedValue > SAL_CALL getTabProps( ::sal_Int32 ID ) throw (::com::sun::star::lang::IndexOutOfBoundsException, ::com::sun::star::uno::RuntimeException);

    virtual void SAL_CALL activateTab( ::sal_Int32 ID ) throw (::com::sun::star::lang::IndexOutOfBoundsException, ::com::sun::star::uno::RuntimeException);
    virtual ::sal_Int32 SAL_CALL getActiveTabID() throw (::com::sun::star::uno::RuntimeException);

    virtual void SAL_CALL addTabListener( const ::com::sun::star::uno::Reference< ::com::sun::star::awt::XTabListener >& Listener ) throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL removeTabListener( const ::com::sun::star::uno::Reference< ::com::sun::star::awt::XTabListener >& Listener ) throw (::com::sun::star::uno::RuntimeException);

    // ::com::sun::star::awt::XLayoutContainer
    virtual void SAL_CALL addChild(
        const ::com::sun::star::uno::Reference< ::com::sun::star::awt::XLayoutConstrains >& Child )
        throw (::com::sun::star::uno::RuntimeException, ::com::sun::star::awt::MaxChildrenException);
    virtual void SAL_CALL removeChild( const ::com::sun::star::uno::Reference< ::com::sun::star::awt::XLayoutConstrains >& Child )
        throw (::com::sun::star::uno::RuntimeException);

    virtual void SAL_CALL allocateArea( const ::com::sun::star::awt::Rectangle &rArea )
        throw (::com::sun::star::uno::RuntimeException);

    virtual ::com::sun::star::awt::Size SAL_CALL getMinimumSize()
        throw(::com::sun::star::uno::RuntimeException);

    // unimplemented:
    virtual sal_Bool SAL_CALL hasHeightForWidth()
        throw(css::uno::RuntimeException)
    { return false; }
    virtual sal_Int32 SAL_CALL getHeightForWidth( sal_Int32 /*nWidth*/ )
    throw(css::uno::RuntimeException)
    { return maRequisition.Height; }

    // VclWindowPeer
    virtual void SAL_CALL setProperty( const ::rtl::OUString& PropertyName, const ::com::sun::star::uno::Any& Value ) throw(::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Any SAL_CALL getProperty( const ::rtl::OUString& PropertyName ) throw(::com::sun::star::uno::RuntimeException);

    // VCLXWindow
    void ProcessWindowEvent( const VclWindowEvent& _rVclWindowEvent );

public:
    // Maps page ids to child references
    struct ChildData : public Box_Base::ChildData
    {
        rtl::OUString maTitle;
        ChildData( css::uno::Reference< css::awt::XLayoutConstrains > const& xChild );
    };

    struct ChildProps : public Box_Base::ChildProps
    {
        ChildProps( VCLXTabControl::ChildData *pData );
    };

protected:
    ChildData *createChild( css::uno::Reference< css::awt::XLayoutConstrains > const& xChild );
    ChildProps *createChildProps( Box_Base::ChildData* pData );


    std::map< ::com::sun::star::uno::Reference< ::com::sun::star::awt::XLayoutConstrains >, sal_Int32 > mIdMap;
    // FIXME: we might want to use a Multiplexer
    std::list< ::com::sun::star::uno::Reference
               < ::com::sun::star::awt::XTabListener > > mxTabListeners;

    inline TabControl *getTabControl() const throw (::com::sun::star::uno::RuntimeException);

private:
    VCLXTabControl( const VCLXTabControl& );            // never implemented
    VCLXTabControl& operator=( const VCLXTabControl& ); // never implemented
};

} // namespace layoutimpl

#endif /* LAYOUT_AWT_VCLXTABCONTROLLER_HXX */
