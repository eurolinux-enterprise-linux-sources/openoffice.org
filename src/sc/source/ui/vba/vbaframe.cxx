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
#include "vbaframe.hxx"
#include <vector>

using namespace com::sun::star;
using namespace ooo::vba;


const static rtl::OUString LABEL( RTL_CONSTASCII_USTRINGPARAM("Label") );
ScVbaFrame::ScVbaFrame( const uno::Reference< XHelperInterface >& xParent, const uno::Reference< uno::XComponentContext >& xContext, const uno::Reference< uno::XInterface >& xControl, const uno::Reference< frame::XModel >& xModel, ov::AbstractGeometryAttributes* pGeomHelper ) : FrameImpl_BASE( xParent, xContext, xControl, xModel, pGeomHelper )
{
}

// Attributes
rtl::OUString SAL_CALL 
ScVbaFrame::getCaption() throw (css::uno::RuntimeException)
{
    rtl::OUString Label;
    m_xProps->getPropertyValue( LABEL ) >>= Label;
    return Label;
}

void SAL_CALL 
ScVbaFrame::setCaption( const rtl::OUString& _caption ) throw (::com::sun::star::uno::RuntimeException)
{
    m_xProps->setPropertyValue( LABEL, uno::makeAny( _caption ) );
}

uno::Any SAL_CALL 
ScVbaFrame::getValue() throw (css::uno::RuntimeException)
{
    return uno::makeAny( getCaption() );
}

void SAL_CALL 
ScVbaFrame::setValue( const uno::Any& _value ) throw (::com::sun::star::uno::RuntimeException)
{
    rtl::OUString sCaption;
    _value >>= sCaption;
    setCaption( sCaption ); 
}

rtl::OUString& 
ScVbaFrame::getServiceImplName()
{
	static rtl::OUString sImplName( RTL_CONSTASCII_USTRINGPARAM("ScVbaFrame") );
	return sImplName;
}

uno::Sequence< rtl::OUString > 
ScVbaFrame::getServiceNames()
{
	static uno::Sequence< rtl::OUString > aServiceNames;
	if ( aServiceNames.getLength() == 0 )
	{
		aServiceNames.realloc( 1 );
		aServiceNames[ 0 ] = rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("ooo.vba.msforms.Frame" ) );
	}
	return aServiceNames;
}
