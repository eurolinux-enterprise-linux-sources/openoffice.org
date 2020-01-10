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
#ifndef SC_VBA_INTERIOR_HXX
#define SC_VBA_INTERIOR_HXX

#include <ooo/vba/excel/XInterior.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XIndexAccess.hpp>

#include <com/sun/star/script/XInvocation.hpp>
#include "vbahelperinterface.hxx"

class ScDocument;

typedef InheritedHelperInterfaceImpl1< ov::excel::XInterior > ScVbaInterior_BASE;

class ScVbaInterior :  public ScVbaInterior_BASE
{
	css::uno::Reference< css::beans::XPropertySet > m_xProps;
	ScDocument* m_pScDoc;
    Color m_aPattColor;
    sal_Int32 m_nPattern; 

        css::uno::Reference< css::container::XIndexAccess > getPalette(); 
    css::uno::Reference< css::container::XNameContainer > GetAttributeContainer();
    css::uno::Any SetAttributeData( sal_Int32 nValue );
    sal_Int32 GetAttributeData( css::uno::Any aValue );
    Color GetBackColor();
protected:
    Color GetPatternColor( const Color& rPattColor, const Color& rBackColor, sal_uInt32 nXclPattern );
    Color GetMixedColor( const Color& rFore, const Color& rBack, sal_uInt8 nTrans );
    sal_uInt8 GetMixedColorComp( sal_uInt8 nFore, sal_uInt8 nBack, sal_uInt8 nTrans );
    css::uno::Any GetIndexColor( const sal_Int32& nColorIndex );
    sal_Int32 GetColorIndex( const sal_Int32 nColor );
    css::uno::Any GetUserDefinedAttributes( const rtl::OUString& sName );
    void SetUserDefinedAttributes( const rtl::OUString& sName, const css::uno::Any& aValue );
    void SetMixedColor();
public:
        ScVbaInterior( const css::uno::Reference< ov::XHelperInterface >& xParent,  const css::uno::Reference< css::uno::XComponentContext >& xContext,
                 const css::uno::Reference< css::beans::XPropertySet >& xProps, ScDocument* pScDoc = NULL) throw ( css::lang::IllegalArgumentException);

        virtual ~ScVbaInterior(){}

	virtual css::uno::Any SAL_CALL getColor() throw (css::uno::RuntimeException) ;
	virtual void SAL_CALL setColor( const css::uno::Any& _color ) throw (css::uno::RuntimeException) ;

	virtual css::uno::Any SAL_CALL getColorIndex() throw ( css::uno::RuntimeException);
	virtual void SAL_CALL setColorIndex( const css::uno::Any& _colorindex ) throw ( css::uno::RuntimeException );
    virtual css::uno::Any SAL_CALL getPattern() throw (css::uno::RuntimeException);
    virtual void SAL_CALL setPattern( const css::uno::Any& _pattern ) throw (css::uno::RuntimeException);
    virtual css::uno::Any SAL_CALL getPatternColor() throw (css::uno::RuntimeException);
    virtual void SAL_CALL setPatternColor( const css::uno::Any& _patterncolor ) throw (css::uno::RuntimeException);
    virtual css::uno::Any SAL_CALL getPatternColorIndex() throw (css::uno::RuntimeException);
    virtual void SAL_CALL setPatternColorIndex( const css::uno::Any& _patterncolorindex ) throw (css::uno::RuntimeException); 
	//XHelperInterface
	virtual rtl::OUString& getServiceImplName();
	virtual css::uno::Sequence<rtl::OUString> getServiceNames();
};
#endif

