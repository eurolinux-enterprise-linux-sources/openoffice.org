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

#include "dp_misc.h"
#include "com/sun/star/uno/Exception.hpp"
#include "com/sun/star/lang/XComponent.hpp"
#include "com/sun/star/uno/XComponentContext.hpp"
#include "com/sun/star/ucb/XCommandEnvironment.hpp"
#include "com/sun/star/deployment/XPackage.hpp"
#include "tools/resmgr.hxx"
#include "rtl/ustring.hxx"
#include "unotools/configmgr.hxx"


#define APP_NAME "unopkg"

namespace css = ::com::sun::star;

namespace unopkg {

    inline ::com::sun::star::lang::Locale toLocale( ::rtl::OUString const & slang )
    {
        ::com::sun::star::lang::Locale locale;
        sal_Int32 nIndex = 0;
        locale.Language = slang.getToken( 0, '-', nIndex );
        locale.Country = slang.getToken( 0, '-', nIndex );
        locale.Variant = slang.getToken( 0, '-', nIndex );
        return locale;
    }


	struct OfficeLocale :
		public rtl::StaticWithInit<const css::lang::Locale, OfficeLocale> {
			const css::lang::Locale operator () () {
				::rtl::OUString slang;
        if (! (::utl::ConfigManager::GetDirectConfigProperty(
                   ::utl::ConfigManager::LOCALE ) >>= slang))
            throw css::uno::RuntimeException( OUSTR("Cannot determine language!"), 0 );
        if (slang.getLength() == 0)
            slang = ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("en-US"));
		return toLocale(slang);
    }
};

struct DeploymentResMgr :  public rtl::StaticWithInit< ResMgr *, DeploymentResMgr > 
{
	ResMgr * operator () () {
        return ResMgr::CreateResMgr( "deployment", OfficeLocale::get());
	}
};

struct OptionInfo
{
    char const * m_name;
    sal_uInt32 m_name_length;
    sal_Unicode m_short_option;
    bool m_has_argument;
};

struct LockFileException : public css::uno::Exception
{
    LockFileException(::rtl::OUString const & sMessage) : 
        css::uno::Exception(sMessage, css::uno::Reference< css::uno::XInterface > ()) {}
};

//==============================================================================
::rtl::OUString toString( OptionInfo const * info );

//==============================================================================
OptionInfo const * getOptionInfo(
    OptionInfo const * list,
    ::rtl::OUString const & opt, sal_Unicode copt = '\0' );

//==============================================================================
bool isOption( OptionInfo const * option_info, sal_uInt32 * pIndex );

//==============================================================================
bool readArgument(
    ::rtl::OUString * pValue, OptionInfo const * option_info,
    sal_uInt32 * pIndex );

//==============================================================================
inline bool readOption(
    bool * flag, OptionInfo const * option_info, sal_uInt32 * pIndex )
{
    if (isOption( option_info, pIndex )) {
        OSL_ASSERT( flag != 0 );
        *flag = true;
        return true;
    }
    return false;
}
//==============================================================================

/** checks if an argument is a bootstrap variable. These start with -env:. For example
    -env:UNO_JAVA_JFW_USER_DATA=file:///d:/user
*/
bool isBootstrapVariable(sal_uInt32 * pIndex);
//==============================================================================
::rtl::OUString const & getExecutableDir();

//==============================================================================
::rtl::OUString const & getProcessWorkingDir();

//==============================================================================
::rtl::OUString makeAbsoluteFileUrl(
    ::rtl::OUString const & sys_path, ::rtl::OUString const & base_url,
    bool throw_exc = true );

//##############################################################################

//==============================================================================
class DisposeGuard
{
    css::uno::Reference<css::lang::XComponent> m_xComp;
    
public:
    inline DisposeGuard() {}
    inline DisposeGuard(
        css::uno::Reference<css::lang::XComponent> const & xComp )
        : m_xComp( xComp ) {}
    
    inline ~DisposeGuard()
    {
        if (m_xComp.is())
            m_xComp->dispose();
    }
    
    inline void reset(
        css::uno::Reference<css::lang::XComponent> const & xComp )
    {
        m_xComp = xComp;
    }
};

//==============================================================================
css::uno::Reference<css::ucb::XCommandEnvironment> createCmdEnv(
    css::uno::Reference<css::uno::XComponentContext> const & xContext,
    ::rtl::OUString const & logFile,
    bool option_force_overwrite,
    bool option_verbose,
    bool option_bundled,
    bool option_suppressLicense);

//==============================================================================
void printf_packages(
    css::uno::Sequence<
    css::uno::Reference<css::deployment::XPackage> > const & seq,
    css::uno::Reference<css::ucb::XCommandEnvironment> const & xCmdEnv,
    sal_Int32 level = 0 );

//##############################################################################

//==============================================================================
css::uno::Reference<css::uno::XComponentContext> getUNO(
    DisposeGuard & disposeGuard, bool verbose, bool shared, bool bGui,
    css::uno::Reference<css::uno::XComponentContext> & out_LocalComponentContext);

}

