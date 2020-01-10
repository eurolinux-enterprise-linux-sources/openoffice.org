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
#ifndef SC_VBA_WORKBOOK_HXX
#define SC_VBA_WORKBOOK_HXX

#include <com/sun/star/frame/XModel.hpp>
#include <ooo/vba/excel/XWorkbook.hpp>
#include "vbahelperinterface.hxx"

class ScModelObj;

typedef InheritedHelperInterfaceImpl1< ov::excel::XWorkbook > ScVbaWorkbook_BASE;

class ScVbaWorkbook : public ScVbaWorkbook_BASE
{
	css::uno::Reference< css::frame::XModel > mxModel;
	static css::uno::Sequence< sal_Int32 > ColorData;
	void initColorData( const css::uno::Sequence< sal_Int32 >& sColors );
	void init();
protected:

	virtual css::uno::Reference< css::frame::XModel >  getModel() { return mxModel; }
	ScVbaWorkbook( 	const css::uno::Reference< ov::XHelperInterface >& xParent, const css::uno::Reference< css::uno::XComponentContext >& xContext);
public:
	ScVbaWorkbook( 	const css::uno::Reference< ov::XHelperInterface >& xParent, const css::uno::Reference< css::uno::XComponentContext >& xContext,
			css::uno::Reference< css::frame::XModel > xModel );
	ScVbaWorkbook( 	css::uno::Sequence< css::uno::Any > const& aArgs, css::uno::Reference< css::uno::XComponentContext >const& xContext );
	virtual ~ScVbaWorkbook() {}

    // Attributes
	virtual ::rtl::OUString SAL_CALL getName() throw (css::uno::RuntimeException);
	virtual ::rtl::OUString SAL_CALL getPath() throw (css::uno::RuntimeException);
	virtual ::rtl::OUString SAL_CALL getFullName() throw (css::uno::RuntimeException);
	virtual ::sal_Bool SAL_CALL getProtectStructure() throw (css::uno::RuntimeException);
	virtual css::uno::Reference< ov::excel::XWorksheet > SAL_CALL getActiveSheet() throw (css::uno::RuntimeException);
	virtual sal_Bool SAL_CALL getSaved() throw (css::uno::RuntimeException);
	virtual void SAL_CALL setSaved( sal_Bool bSave ) throw (css::uno::RuntimeException);

	// Methods
	virtual css::uno::Any SAL_CALL Worksheets( const css::uno::Any& aIndex ) throw (css::uno::RuntimeException);
	virtual css::uno::Any SAL_CALL Sheets( const css::uno::Any& aIndex ) throw (css::uno::RuntimeException);
	virtual css::uno::Any SAL_CALL Windows( const css::uno::Any& aIndex ) throw (css::uno::RuntimeException);
	virtual void SAL_CALL Close( const css::uno::Any &bSaveChanges,
								 const css::uno::Any &aFileName,
								 const css::uno::Any &bRouteWorkbook ) throw (css::uno::RuntimeException);
	virtual void SAL_CALL Protect( const css::uno::Any & aPassword ) throw (css::uno::RuntimeException);
	virtual void SAL_CALL Unprotect( const css::uno::Any &aPassword ) throw (css::uno::RuntimeException);
	virtual void SAL_CALL Save() throw (css::uno::RuntimeException);
	virtual void SAL_CALL Activate() throw (css::uno::RuntimeException);
    // Amelia Wang
    virtual css::uno::Any SAL_CALL Names( const css::uno::Any& aIndex ) throw (css::uno::RuntimeException);

	virtual css::uno::Any SAL_CALL Styles( const css::uno::Any& Item ) throw (css::uno::RuntimeException);
	virtual void SAL_CALL ResetColors(  ) throw (css::script::BasicErrorException, css::uno::RuntimeException);
	virtual css::uno::Any SAL_CALL Colors( const css::uno::Any& Index ) throw (css::script::BasicErrorException, css::uno::RuntimeException);
	virtual ::sal_Int32 SAL_CALL FileFormat(  ) throw (css::script::BasicErrorException, css::uno::RuntimeException);
	virtual void SAL_CALL SaveCopyAs( const rtl::OUString& Filename ) throw ( css::uno::RuntimeException);
    // code name
    virtual ::rtl::OUString SAL_CALL getCodeName() throw ( css::uno::RuntimeException);
    virtual void SAL_CALL setCodeName( const ::rtl::OUString& sGlobCodeName ) throw (css::uno::RuntimeException);
    
	// XHelperInterface
	virtual rtl::OUString& getServiceImplName();
	virtual css::uno::Sequence<rtl::OUString> getServiceNames();
};

#endif /* SC_VBA_WORKBOOK_HXX */
