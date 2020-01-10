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
#ifndef SC_VBA_COMMANDBAR_HXX
#define SC_VBA_COMMANDBAR_HXX

#include <ooo/vba/XCommandBar.hpp>
#include <com/sun/star/ui/XUIConfigurationManager.hpp>
#include <com/sun/star/ui/XUIConfigurationPersistence.hpp>
#include <com/sun/star/container/XIndexContainer.hpp>
#include <com/sun/star/beans/PropertyValues.hpp>

#include "vbahelperinterface.hxx"
#include "vbacommandbars.hxx"

#include <map>
typedef std::map< const rtl::OUString, rtl::OUString > CommandBarNameMap;
typedef std::pair< const rtl::OUString, rtl::OUString > CommandBarNamePair;
const CommandBarNamePair namePair[] = { 
    CommandBarNamePair(  rtl::OUString::createFromAscii("standard"), rtl::OUString::createFromAscii("standardbar") ),
    CommandBarNamePair(  rtl::OUString::createFromAscii("formatting"), rtl::OUString::createFromAscii("formatobjectbar") ),
};
static const CommandBarNameMap mCommandBarNameMap( namePair, ( namePair + sizeof(namePair) / sizeof(namePair[0]) ) );


typedef InheritedHelperInterfaceImpl1< ov::XCommandBar > CommandBar_BASE;

class ScVbaCommandBar : public CommandBar_BASE
{
private:
    rtl::OUString       m_sToolBarName;
    rtl::OUString       m_sMenuModuleName;
    rtl::OUString       m_sUIName;
    sal_Bool            m_bTemporary;
    sal_Bool            m_bIsMenu;
    sal_Bool            m_bCustom;
    sal_Bool            m_bCreate;
    ScVbaCommandBars*   m_pScVbaCommandBars;
    css::beans::PropertyValues  m_aToolBar;
    // hard reference for parent
    css::uno::Reference< ov::XHelperInterface >               m_xParentHardRef;
    css::uno::Reference< css::ui::XUIConfigurationManager >         m_xUICfgManager;
    css::uno::Reference< css::ui::XUIConfigurationPersistence >     m_xUICfgPers;
    css::uno::Reference< css::container::XIndexContainer >          m_xBarSettings;
    void initCommandBar() throw( css::uno::RuntimeException );
protected:
    void getToolBarSettings( rtl::OUString sToolBarName ) throw( css::uno::RuntimeException );
    void getMenuSettings();
    void addCustomBar();
public:
    ScVbaCommandBar( const css::uno::Reference< ov::XHelperInterface > xParent, const css::uno::Reference< css::uno::XComponentContext > xContext, sal_Int32 nModuleType ) throw( css::uno::RuntimeException );
    ScVbaCommandBar( const css::uno::Reference< ov::XHelperInterface > xParent, const css::uno::Reference< css::uno::XComponentContext > xContext, rtl::OUString sToolBarName, sal_Bool bTemporary, sal_Bool bCreate ) throw( css::uno::RuntimeException );
    
    sal_Bool IsMenu() { return m_bIsMenu; };
    css::uno::Reference< css::ui::XUIConfigurationManager > GetUICfgManager() { return m_xUICfgManager; };
    css::uno::Reference< css::ui::XUIConfigurationPersistence > GetUICfgPers() { return m_xUICfgPers; };
    css::uno::Reference< css::container::XIndexContainer > GetBarSettings() { return m_xBarSettings; };
    rtl::OUString GetToolBarName() { return m_sToolBarName; };

    // Attributes
    virtual ::rtl::OUString SAL_CALL getName() throw (css::uno::RuntimeException);
    virtual void SAL_CALL setName( const ::rtl::OUString& _name ) throw (css::uno::RuntimeException);
    virtual ::sal_Bool SAL_CALL getVisible() throw (css::uno::RuntimeException);
    virtual void SAL_CALL setVisible( ::sal_Bool _visible ) throw (css::uno::RuntimeException);

    // Methods
    virtual void SAL_CALL Delete(  ) throw (css::script::BasicErrorException, css::uno::RuntimeException);
    virtual css::uno::Any SAL_CALL Controls( const css::uno::Any& aIndex ) throw (css::script::BasicErrorException, css::uno::RuntimeException);

	// XHelperInterface
	virtual rtl::OUString& getServiceImplName();
	virtual css::uno::Sequence<rtl::OUString> getServiceNames();
};
#endif//SC_VBA_COMMANDBAR_HXX
