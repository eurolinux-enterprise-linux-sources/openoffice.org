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

#ifndef _TOOLKIT_AWT_VCLXTOPWINDOW_HXX_
#define _TOOLKIT_AWT_VCLXTOPWINDOW_HXX_

#include <com/sun/star/awt/XSystemDependentWindowPeer.hpp>
#include <com/sun/star/awt/XTopWindow.hpp>
#include <com/sun/star/awt/XMenuBar.hpp>
#include <cppuhelper/weak.hxx>
#include <osl/mutex.hxx>

#include <cppuhelper/implbase2.hxx>

#include <toolkit/awt/vclxcontainer.hxx>

typedef ::cppu::ImplHelper2 < ::com::sun::star::awt::XTopWindow,
                              ::com::sun::star::awt::XSystemDependentWindowPeer
                              > VCLXTopWindow_XBase;

class TOOLKIT_DLLPUBLIC VCLXTopWindow_Base: public VCLXTopWindow_XBase
{
protected:
  	::com::sun::star::uno::Reference< ::com::sun::star::awt::XMenuBar> mxMenuBar;

	virtual ::vos::IMutex& GetMutexImpl() = 0;
    virtual Window* GetWindowImpl() = 0;
	virtual ::cppu::OInterfaceContainerHelper& GetTopWindowListenersImpl() = 0;

public:
    virtual ~VCLXTopWindow_Base();

    // ::com::sun::star::awt::XSystemDependentWindowPeer
    ::com::sun::star::uno::Any SAL_CALL getWindowHandle( const ::com::sun::star::uno::Sequence< sal_Int8 >& ProcessId, sal_Int16 SystemType ) throw(::com::sun::star::uno::RuntimeException);

    // ::com::sun::star::awt::XTopWindow
    void SAL_CALL addTopWindowListener( const ::com::sun::star::uno::Reference< ::com::sun::star::awt::XTopWindowListener >& rxListener ) throw(::com::sun::star::uno::RuntimeException);
    void SAL_CALL removeTopWindowListener( const ::com::sun::star::uno::Reference< ::com::sun::star::awt::XTopWindowListener >& rxListener ) throw(::com::sun::star::uno::RuntimeException);
    void SAL_CALL toFront() throw(::com::sun::star::uno::RuntimeException);
    void SAL_CALL toBack() throw(::com::sun::star::uno::RuntimeException);
    void SAL_CALL setMenuBar( const ::com::sun::star::uno::Reference< ::com::sun::star::awt::XMenuBar >& xMenu ) throw(::com::sun::star::uno::RuntimeException);
};

//	----------------------------------------------------
//	class VCLXTopWindow
//	----------------------------------------------------

class VCLXTopWindow: public VCLXTopWindow_Base,
                     public VCLXContainer
{
private:
    bool m_bWHWND;

protected:
    virtual vos::IMutex& GetMutexImpl();
    virtual Window* GetWindowImpl();
    virtual ::cppu::OInterfaceContainerHelper& GetTopWindowListenersImpl();

public:	
	VCLXTopWindow(bool bWHWND = false);
	~VCLXTopWindow();
	
	// ::com::sun::star::uno::XInterface
    ::com::sun::star::uno::Any	SAL_CALL queryInterface( const ::com::sun::star::uno::Type & rType ) throw(::com::sun::star::uno::RuntimeException);
	void						SAL_CALL acquire() throw()	{ OWeakObject::acquire(); }
	void						SAL_CALL release() throw()	{ OWeakObject::release(); }

	// ::com::sun::star::lang::XTypeProvider
	::com::sun::star::uno::Sequence< ::com::sun::star::uno::Type >	SAL_CALL getTypes() throw(::com::sun::star::uno::RuntimeException);
	::com::sun::star::uno::Sequence< sal_Int8 >						SAL_CALL getImplementationId() throw(::com::sun::star::uno::RuntimeException);

    static void     ImplGetPropertyIds( std::list< sal_uInt16 > &aIds );
    virtual void    GetPropertyIds( std::list< sal_uInt16 > &aIds ) { return ImplGetPropertyIds( aIds ); }
};




#endif // _TOOLKIT_AWT_VCLXTOPWINDOW_HXX_

