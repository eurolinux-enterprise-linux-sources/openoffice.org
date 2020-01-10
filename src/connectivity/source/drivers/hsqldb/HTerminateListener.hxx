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
#ifndef CONNECTIVITY_HSQLDB_TERMINATELISTENER_HXX
#define CONNECTIVITY_HSQLDB_TERMINATELISTENER_HXX

#include <cppuhelper/compbase1.hxx>
#ifndef _COM_SUN_STAR_LANG_XSERVICEINFO_HPP_
#include <com/sun/star/frame/XTerminateListener.hpp>
#endif

//........................................................................
namespace connectivity
{
//........................................................................

	namespace hsqldb
	{
        class ODriverDelegator;
        class OConnectionController : public ::cppu::WeakImplHelper1< 
											        ::com::sun::star::frame::XTerminateListener >
        {
            ODriverDelegator* m_pDriver;
            protected:
                virtual ~OConnectionController() {m_pDriver = NULL;}
	        public:
                OConnectionController(ODriverDelegator* _pDriver) : m_pDriver(_pDriver){}

		        // XEventListener
		        virtual void SAL_CALL disposing( const ::com::sun::star::lang::EventObject& Source )
			        throw( ::com::sun::star::uno::RuntimeException );

		        // XTerminateListener
		        virtual void SAL_CALL queryTermination( const ::com::sun::star::lang::EventObject& aEvent )
			        throw( ::com::sun::star::frame::TerminationVetoException, ::com::sun::star::uno::RuntimeException );
		        virtual void SAL_CALL notifyTermination( const ::com::sun::star::lang::EventObject& aEvent )
			        throw( ::com::sun::star::uno::RuntimeException );
        };
    }
//........................................................................
}	// namespace connectivity
//........................................................................
#endif // CONNECTIVITY_HSQLDB_TERMINATELISTENER_HXX
