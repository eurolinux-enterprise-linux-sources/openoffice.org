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

#ifndef __FRAMEWORK_UIELEMENT_HEADERMENUCONTROLLER_HXX_
#define __FRAMEWORK_UIELEMENT_HEADERMENUCONTROLLER_HXX_

//_________________________________________________________________________________________________________________
//	my own includes
//_________________________________________________________________________________________________________________

#include <helper/popupmenucontrollerbase.hxx>
#include <stdtypes.h>

//_________________________________________________________________________________________________________________
//	interface includes
//_________________________________________________________________________________________________________________
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XTypeProvider.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/frame/XDispatch.hpp>
#include <com/sun/star/frame/XStatusListener.hpp>
#include <com/sun/star/frame/XPopupMenuController.hpp>
#include <com/sun/star/frame/status/Verb.hpp>
#include <com/sun/star/frame/XModel.hpp>

//_________________________________________________________________________________________________________________
//	includes of other projects
//_________________________________________________________________________________________________________________
#include <toolkit/awt/vclxmenu.hxx>
#include <cppuhelper/weak.hxx>
#include <rtl/ustring.hxx>

namespace framework
{
    class HeaderMenuController :  public PopupMenuControllerBase
    {
		public:
		    HeaderMenuController( const ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >& xServiceManager,bool _bFooter = sal_False );
		    virtual ~HeaderMenuController();
		    
            // XServiceInfo
		    DECLARE_XSERVICEINFO

            // XPopupMenuController
            virtual void SAL_CALL updatePopupMenu() throw (::com::sun::star::uno::RuntimeException);
		    
		    // XStatusListener
		    virtual void SAL_CALL statusChanged( const ::com::sun::star::frame::FeatureStateEvent& Event ) throw ( ::com::sun::star::uno::RuntimeException );

		    // XEventListener
		    virtual void SAL_CALL disposing( const com::sun::star::lang::EventObject& Source ) throw ( ::com::sun::star::uno::RuntimeException );

		private:
            virtual void impl_select(const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XDispatch >& _xDispatch,const ::com::sun::star::util::URL& aURL);
            void fillPopupMenu( const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XModel >& rModel, com::sun::star::uno::Reference< com::sun::star::awt::XPopupMenu >& rPopupMenu );

            ::com::sun::star::uno::Reference< ::com::sun::star::frame::XModel > m_xModel;
            bool m_bFooter;
    };
}

#endif // __FRAMEWORK_UIELEMENT_HEADERMENUCONTROLLER_HXX_
