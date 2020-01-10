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

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_svx.hxx"
#include <svx/fmdmod.hxx>
#include "fmservs.hxx"
#include <fmobj.hxx>
#include <svx/unoshape.hxx>
#include <comphelper/processfactory.hxx>
#include <svx/fmglob.hxx>

using namespace ::svxform;

//-----------------------------------------------------------------------------
::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface >  SAL_CALL SvxFmMSFactory::createInstance(const ::rtl::OUString& ServiceSpecifier) throw( ::com::sun::star::uno::Exception, ::com::sun::star::uno::RuntimeException )
{
	::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface >  xRet;
	if ( ServiceSpecifier.indexOf( ::rtl::OUString::createFromAscii("com.sun.star.form.component.") ) == 0 )
	{
		xRet = ::comphelper::getProcessServiceFactory()->createInstance(ServiceSpecifier);
	}
	else if ( ServiceSpecifier == ::rtl::OUString( ::rtl::OUString::createFromAscii("com.sun.star.drawing.ControlShape") ) )
	{
		SdrObject* pObj = new FmFormObj(OBJ_FM_CONTROL);
		xRet = *new SvxShapeControl(pObj);
	}
	if (!xRet.is())
		xRet = SvxUnoDrawMSFactory::createInstance(ServiceSpecifier);
	return xRet;
}

//-----------------------------------------------------------------------------
::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > SAL_CALL  SvxFmMSFactory::createInstanceWithArguments(const ::rtl::OUString& ServiceSpecifier, const ::com::sun::star::uno::Sequence< ::com::sun::star::uno::Any >& Arguments) throw( ::com::sun::star::uno::Exception, ::com::sun::star::uno::RuntimeException )
{
	return SvxUnoDrawMSFactory::createInstanceWithArguments(ServiceSpecifier, Arguments );
}

//-----------------------------------------------------------------------------
::com::sun::star::uno::Sequence< ::rtl::OUString > SAL_CALL SvxFmMSFactory::getAvailableServiceNames(void) throw( ::com::sun::star::uno::RuntimeException )
{
	static const ::rtl::OUString aSvxComponentServiceNameList[] =
	{
		FM_SUN_COMPONENT_TEXTFIELD,
		FM_SUN_COMPONENT_FORM,
		FM_SUN_COMPONENT_LISTBOX,
		FM_SUN_COMPONENT_COMBOBOX,
		FM_SUN_COMPONENT_RADIOBUTTON,
		FM_SUN_COMPONENT_GROUPBOX,
		FM_SUN_COMPONENT_FIXEDTEXT,
		FM_SUN_COMPONENT_COMMANDBUTTON,
		FM_SUN_COMPONENT_CHECKBOX,
		FM_SUN_COMPONENT_GRIDCONTROL,
		FM_SUN_COMPONENT_IMAGEBUTTON,
		FM_SUN_COMPONENT_FILECONTROL,
		FM_SUN_COMPONENT_TIMEFIELD,
		FM_SUN_COMPONENT_DATEFIELD,
		FM_SUN_COMPONENT_NUMERICFIELD,
		FM_SUN_COMPONENT_CURRENCYFIELD,
		FM_SUN_COMPONENT_PATTERNFIELD,
		FM_SUN_COMPONENT_HIDDENCONTROL,
		FM_SUN_COMPONENT_IMAGECONTROL
	};

	static const sal_uInt16 nSvxComponentServiceNameListCount = sizeof(aSvxComponentServiceNameList) / sizeof ( aSvxComponentServiceNameList[0] );

	::com::sun::star::uno::Sequence< ::rtl::OUString > aSeq( nSvxComponentServiceNameListCount );
	::rtl::OUString* pStrings = aSeq.getArray();
	for( sal_uInt16 nIdx = 0; nIdx < nSvxComponentServiceNameListCount; nIdx++ )
		pStrings[nIdx] = aSvxComponentServiceNameList[nIdx];

	::com::sun::star::uno::Sequence< ::rtl::OUString > aParentSeq( SvxUnoDrawMSFactory::getAvailableServiceNames() );
	return concatServiceNames( aParentSeq, aSeq );
}

/*
// XServiceManager
::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface >  SvxFmDrawModel::createInstance(const ::rtl::OUString& ServiceName)
			const throw( ::com::sun::star::lang::ServiceNotRegisteredException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException )
{
	::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface >  xRet;
	sal_uInt16 nTokenCount = ServiceName.getTokenCount('.');
	if (nTokenCount == 5 &&
		ServiceName.getToken( 0, '.' ) == ::rtl::OUString::createFromAscii("stardiv") &&
		ServiceName.getToken( 1, '.' ) == ::rtl::OUString::createFromAscii("one") &&
		ServiceName.getToken( 2, '.' ) == ::rtl::OUString::createFromAscii("form") &&
		ServiceName.getToken( 3, '.' ) == ::rtl::OUString::createFromAscii("component"))
	{
		xRet = ::comphelper::getProcessServiceFactory()->createInstance(ServiceName);
	}
	else
	if (nTokenCount == 4 &&
		ServiceName.getToken( 0, '.' ) == ::rtl::OUString::createFromAscii("stardiv") &&
		ServiceName.getToken( 1, '.' ) == ::rtl::OUString::createFromAscii("one") &&
		ServiceName.getToken( 2, '.' ) == ::rtl::OUString::createFromAscii("drawing") &&
		ServiceName.getToken( 3, '.' ) == ::rtl::OUString::createFromAscii("ControlShape"))
	{
		SdrObject* pObj = new FmFormObj();
		xRet = *new SvxShapeControl(pObj);
	}
	if (!xRet.is())
		xRet = SvxUnoDrawModel::createInstance(ServiceName);
	return xRet;
}
*/


