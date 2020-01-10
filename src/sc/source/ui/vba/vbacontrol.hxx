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
#ifndef SC_VBA_CONTROL_HXX
#define SC_VBA_CONTROL_HXX

#include <cppuhelper/implbase1.hxx>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/script/XDefaultProperty.hpp>
#include <com/sun/star/drawing/XControlShape.hpp>
#include <com/sun/star/awt/XControl.hpp>
#include <com/sun/star/awt/XWindowPeer.hpp>
#include <ooo/vba/msforms/XControl.hpp>

#include "vbahelper.hxx"
#include "vbahelperinterface.hxx"

//typedef ::cppu::WeakImplHelper1< ov::msforms::XControl > ControlImpl_BASE;
typedef InheritedHelperInterfaceImpl1< ov::msforms::XControl > ControlImpl_BASE;

class ScVbaControl : public ControlImpl_BASE
{
private:
    com::sun::star::uno::Reference< com::sun::star::lang::XEventListener > m_xEventListener;
protected:
    std::auto_ptr< ov::AbstractGeometryAttributes > mpGeometryHelper;
    css::uno::Reference< css::beans::XPropertySet > m_xProps;
    css::uno::Reference< css::uno::XInterface > m_xControl;
    css::uno::Reference< css::frame::XModel > m_xModel;

    virtual css::uno::Reference< css::awt::XWindowPeer > getWindowPeer() throw (css::uno::RuntimeException);
public:
    ScVbaControl( const css::uno::Reference< ov::XHelperInterface >& xParent, const css::uno::Reference< css::uno::XComponentContext >& xContext, 
                    const css::uno::Reference< css::uno::XInterface >& xControl, const css::uno::Reference< css::frame::XModel >& xModel, ov::AbstractGeometryAttributes* pHelper );
    virtual ~ScVbaControl();
    // This class will own the helper, so make sure it is allocated from 
    // the heap
    void setGeometryHelper( ov::AbstractGeometryAttributes* pHelper );
    // XControl
    virtual sal_Bool SAL_CALL getEnabled() throw (css::uno::RuntimeException);
    virtual void SAL_CALL setEnabled( sal_Bool _enabled ) throw (css::uno::RuntimeException);
    virtual sal_Bool SAL_CALL getVisible() throw (css::uno::RuntimeException);
    virtual void SAL_CALL setVisible( sal_Bool _visible ) throw (css::uno::RuntimeException);
    virtual double SAL_CALL getHeight() throw (css::uno::RuntimeException);
    virtual void SAL_CALL setHeight( double _height ) throw (css::uno::RuntimeException);
    virtual double SAL_CALL getWidth() throw (css::uno::RuntimeException);
    virtual void SAL_CALL setWidth( double _width ) throw (css::uno::RuntimeException);
    virtual double SAL_CALL getLeft() throw (css::uno::RuntimeException);
    virtual void SAL_CALL setLeft( double _left ) throw (css::uno::RuntimeException);
    virtual double SAL_CALL getTop() throw (css::uno::RuntimeException);
    virtual void SAL_CALL setTop( double _top ) throw (css::uno::RuntimeException);
    virtual void SAL_CALL SetFocus(  ) throw (css::uno::RuntimeException);

    virtual css::uno::Reference< css::uno::XInterface > SAL_CALL getObject() throw (css::uno::RuntimeException);
    virtual rtl::OUString SAL_CALL getControlSource() throw (css::uno::RuntimeException);
    virtual void SAL_CALL setControlSource( const rtl::OUString& _controlsource ) throw (css::uno::RuntimeException);
    virtual rtl::OUString SAL_CALL getRowSource() throw (css::uno::RuntimeException);
    virtual void SAL_CALL setRowSource( const rtl::OUString& _rowsource ) throw (css::uno::RuntimeException);
    virtual rtl::OUString SAL_CALL getName() throw (css::uno::RuntimeException);
    virtual void SAL_CALL setName( const rtl::OUString& _name ) throw (css::uno::RuntimeException);
    //remove resouce because ooo.vba.excel.XControl is a wrapper of com.sun.star.drawing.XControlShape
    virtual void removeResouce() throw( css::uno::RuntimeException );
    //XHelperInterface
    virtual rtl::OUString& getServiceImplName();
    virtual css::uno::Sequence<rtl::OUString> getServiceNames();
};


class ScVbaControlFactory
{
public:
    ScVbaControlFactory( const css::uno::Reference< css::uno::XComponentContext >& xContext, 
                    const css::uno::Reference< css::uno::XInterface >& xControl, const css::uno::Reference< css::frame::XModel >& xModel );
    ScVbaControl* createControl()  throw ( css::uno::RuntimeException );
    ScVbaControl* createControl( const css::uno::Reference< css::uno::XInterface >& xParent )  throw ( css::uno::RuntimeException );
private:
    ScVbaControl* createControl( const css::uno::Reference< css::awt::XControl >&, const css::uno::Reference< css::uno::XInterface >&  )  throw ( css::uno::RuntimeException );
    ScVbaControl* createControl( const css::uno::Reference< css::drawing::XControlShape >&, const css::uno::Reference< css::uno::XInterface >& )  throw ( css::uno::RuntimeException );
    css::uno::Reference< css::uno::XComponentContext > m_xContext;
    css::uno::Reference< css::uno::XInterface > m_xControl;
    css::uno::Reference< css::frame::XModel > m_xModel;
};

#endif//SC_VBA_CONTROL_HXX
