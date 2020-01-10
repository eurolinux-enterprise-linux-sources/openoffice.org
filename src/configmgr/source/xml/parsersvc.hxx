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

#ifndef CONFIGMGR_XML_PARSERSVC_HXX
#define CONFIGMGR_XML_PARSERSVC_HXX

#include "serviceinfohelper.hxx"
#include <cppuhelper/implbase4.hxx>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/io/XActiveDataSink.hpp>
#include <com/sun/star/xml/sax/InputSource.hpp>
#include <com/sun/star/xml/sax/XDocumentHandler.hpp>

namespace configmgr
{
// -----------------------------------------------------------------------------
    namespace xml
    {
// -----------------------------------------------------------------------------
        namespace uno   = ::com::sun::star::uno;
        namespace lang  = ::com::sun::star::lang;
        namespace io    = ::com::sun::star::io;
        namespace sax   = ::com::sun::star::xml::sax;
// -----------------------------------------------------------------------------

        template <class BackendInterface>
        class ParserService : public ::cppu::WeakImplHelper4< 
                                            lang::XInitialization,
                                            lang::XServiceInfo,  
                                            io::XActiveDataSink,
                                            BackendInterface          
                                        > 
        {
        public:
            explicit
            ParserService(uno::Reference< uno::XComponentContext > const & _xContext);

            // XInitialization
            virtual void SAL_CALL 
                initialize( const uno::Sequence< uno::Any >& aArguments ) 
                    throw (uno::Exception, uno::RuntimeException);

            // XServiceInfo
            virtual ::rtl::OUString SAL_CALL 
                getImplementationName(  ) 
                    throw (uno::RuntimeException);

            virtual sal_Bool SAL_CALL 
                supportsService( const ::rtl::OUString& ServiceName ) 
                    throw (uno::RuntimeException);

            virtual uno::Sequence< ::rtl::OUString > SAL_CALL 
                getSupportedServiceNames(  ) 
                    throw (uno::RuntimeException);
    
            // XActiveDataSink
            virtual void SAL_CALL 
                setInputStream( const uno::Reference< io::XInputStream >& aStream ) 
                    throw (uno::RuntimeException);
    
            virtual uno::Reference< io::XInputStream > SAL_CALL 
                getInputStream(  ) 
                    throw (uno::RuntimeException);

        protected:
            uno::Reference< uno::XComponentContext > getContext() const 
            { return m_xContext; }

            void parse(uno::Reference< sax::XDocumentHandler > const & _xHandler);
            // throw (backenduno::MalformedDataException, lang::WrappedTargetException, uno::RuntimeException);

        private:
            uno::Reference< uno::XComponentContext >   m_xContext;
            sax::InputSource m_aInputSource;

            static ServiceInfoHelper getServiceInfo();
        };

// -----------------------------------------------------------------------------
    } // namespace xml
// -----------------------------------------------------------------------------

} // namespace configmgr
#endif 


	
	
