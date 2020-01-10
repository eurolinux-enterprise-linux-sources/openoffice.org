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
#ifndef SC_VBA_COMMANDBARCONTROLS_HXX
#define SC_VBA_COMMANDBARCONTROLS_HXX

#include <ooo/vba/XCommandBarControls.hpp>

#include "vbahelperinterface.hxx"
#include "vbacommandbar.hxx"
#include "vbacollectionimpl.hxx"

class ScVbaCommandBarControl;

typedef CollTestImplHelper< ov::XCommandBarControls > CommandBarControls_BASE;

class ScVbaCommandBarControls : public CommandBarControls_BASE
{
private:
    sal_Bool                    m_bIsMenu;
    sal_Bool                    m_bHasElements;
    ScVbaCommandBar*            m_pCommandBar; 
    ScVbaCommandBarControl*     m_pCommandBarControl; 
    css::uno::Reference< ov::XHelperInterface >               m_xParentHardRef;
    css::uno::Reference< css::ui::XUIConfigurationManager >         m_xUICfgManager;
    css::uno::Reference< css::ui::XUIConfigurationPersistence >     m_xUICfgPers;
    css::uno::Reference< css::container::XIndexContainer >          m_xBarSettings;

public:
    ScVbaCommandBarControls( const css::uno::Reference< ov::XHelperInterface > xParent, const css::uno::Reference< css::uno::XComponentContext > xContext, const css::uno::Reference< css::container::XIndexAccess > xIndexAccess ) throw( css::uno::RuntimeException );
    css::uno::Reference< css::ui::XUIConfigurationManager > GetUICfgManager() { return m_xUICfgManager; };
    css::uno::Reference< css::ui::XUIConfigurationPersistence > GetUICfgPers() { return m_xUICfgPers; };
    css::uno::Reference< css::container::XIndexContainer >  GetBarSettings() { return m_xBarSettings; };
    sal_Bool IsMenu() { return m_bIsMenu; };
    ScVbaCommandBar* GetParentCommandBar() { return m_pCommandBar; };
    ScVbaCommandBarControl* GetParentCommandBarControl() { return m_pCommandBarControl; };
    rtl::OUString GetParentToolBarName() 
    { 
        if( m_pCommandBar ) return m_pCommandBar->GetToolBarName();
        else return rtl::OUString();
    }
    rtl::OUString GetControlNameByIndex( const sal_Int32 nIndex ) throw ( css::uno::RuntimeException );

    // Attributes
    virtual ::sal_Int32 SAL_CALL getCount() throw (css::uno::RuntimeException);
    // XEnumerationAccess
    virtual css::uno::Type SAL_CALL getElementType() throw (css::uno::RuntimeException);
    virtual css::uno::Reference< css::container::XEnumeration > SAL_CALL createEnumeration() throw (css::uno::RuntimeException);
    virtual css::uno::Any createCollectionObject( const css::uno::Any& aSource );

    // Methods     
    virtual css::uno::Any SAL_CALL Item( const css::uno::Any& Index, const css::uno::Any& /*Index2*/ ) throw (css::uno::RuntimeException); 
    virtual css::uno::Reference< ov::XCommandBarControl > SAL_CALL Add( const css::uno::Any& Type, const css::uno::Any& Id, const css::uno::Any& Parameter, const css::uno::Any& Before, const css::uno::Any& Temporary ) throw (css::script::BasicErrorException, css::uno::RuntimeException);
    // XHelperInterface
    virtual rtl::OUString& getServiceImplName();
    virtual css::uno::Sequence<rtl::OUString> getServiceNames();
};

#endif//SC_VBA_COMMANDBARCONTROLS_HXX
