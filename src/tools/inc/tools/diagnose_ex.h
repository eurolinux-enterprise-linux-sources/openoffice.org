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

#ifndef TOOLS_DIAGNOSE_EX_H
#define TOOLS_DIAGNOSE_EX_H

#include <osl/diagnose.h>
#include <rtl/ustring.hxx>

#include <com/sun/star/uno/RuntimeException.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>

#include <boost/current_function.hpp>


#define OSL_UNUSED( expression ) \
    (void)(expression)

#if OSL_DEBUG_LEVEL > 0

    #include <cppuhelper/exc_hlp.hxx>
    #include <osl/diagnose.h>
    #include <osl/thread.h>
    #include <boost/current_function.hpp>
    #include <typeinfo>

    /** reports a caught UNO exception via OSL diagnostics

        Note that whenever you use this, it might be an indicator that your error
        handling is not correct ....
    */
    #define DBG_UNHANDLED_EXCEPTION()   \
        ::com::sun::star::uno::Any caught( ::cppu::getCaughtException() ); \
	    ::rtl::OString sMessage( "caught an exception!" ); \
        sMessage += "\nin function:"; \
        sMessage += BOOST_CURRENT_FUNCTION; \
	    sMessage += "\ntype: "; \
	    sMessage += ::rtl::OString( caught.getValueTypeName().getStr(), caught.getValueTypeName().getLength(), osl_getThreadTextEncoding() ); \
        ::com::sun::star::uno::Exception exception; \
        caught >>= exception; \
        if ( exception.Message.getLength() ) \
        { \
	        sMessage += "\nmessage: "; \
	        sMessage += ::rtl::OString( exception.Message.getStr(), exception.Message.getLength(), osl_getThreadTextEncoding() ); \
        } \
        if ( exception.Context.is() ) \
        { \
            const char* pContext = typeid( *exception.Context.get() ).name(); \
            sMessage += "\ncontext: "; \
            sMessage += pContext; \
        } \
        sMessage += "\n"; \
	    OSL_ENSURE( false, sMessage )

#else   // OSL_DEBUG_LEVEL

    #define DBG_UNHANDLED_EXCEPTION()

#endif  // OSL_DEBUG_LEVEL


/** This macro asserts the given condition (in debug mode), and throws
    an IllegalArgumentException afterwards.
 */
#define ENSURE_ARG_OR_THROW(c, m) if( !(c) ) { \
                                     OSL_ENSURE(c, m); \
                                     throw ::com::sun::star::lang::IllegalArgumentException( \
                                     ::rtl::OUString::createFromAscii(BOOST_CURRENT_FUNCTION) + \
                                     ::rtl::OUString::createFromAscii(",\n"m), \
                                     ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface >(), \
                                     0 ); }
#define ENSURE_ARG_OR_THROW2(c, m, ifc, arg) if( !(c) ) { \
                                               OSL_ENSURE(c, m); \
                                               throw ::com::sun::star::lang::IllegalArgumentException( \
                                               ::rtl::OUString::createFromAscii(BOOST_CURRENT_FUNCTION) + \
                                               ::rtl::OUString::createFromAscii(",\n"m), \
                                               ifc, \
                                               arg ); }

/** This macro asserts the given condition (in debug mode), and throws
    an RuntimeException afterwards.
 */
#define ENSURE_OR_THROW(c, m) if( !(c) ) { \
                                     OSL_ENSURE(c, m); \
                                     throw ::com::sun::star::uno::RuntimeException( \
                                     ::rtl::OUString::createFromAscii(BOOST_CURRENT_FUNCTION) + \
                                     ::rtl::OUString::createFromAscii(",\n"m), \
                                     ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface >() ); }
#define ENSURE_OR_THROW2(c, m, ifc) if( !(c) ) { \
                                          OSL_ENSURE(c, m); \
                                          throw ::com::sun::star::uno::RuntimeException( \
                                          ::rtl::OUString::createFromAscii(BOOST_CURRENT_FUNCTION) + \
                                          ::rtl::OUString::createFromAscii(",\n"m), \
                                          ifc ); }

/** This macro asserts the given condition (in debug mode), and
    returns false afterwards.
 */
#define ENSURE_OR_RETURN(c, m) if( !(c) ) { \
                                     OSL_ENSURE(c, m); \
                                     return false; }



#endif // TOOLS_DIAGNOSE_EX_H
