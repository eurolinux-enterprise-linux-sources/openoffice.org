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

#include "dp_misc.h"
#include "unopkg_main.h"
#include "unopkg_shared.h"
#include "dp_identifier.hxx"
#include "sal/main.h"
#include "tools/extendapplicationenvironment.hxx"
#include "rtl/ustrbuf.hxx"
#include "rtl/uri.hxx"
#include "osl/thread.h"
#include "osl/process.h"
#include "osl/conditn.hxx"
#include "cppuhelper/implbase1.hxx"
#include "cppuhelper/exc_hlp.hxx"
#include "comphelper/anytostring.hxx"
#include "com/sun/star/deployment/thePackageManagerFactory.hpp"
#include "com/sun/star/deployment/ui/PackageManagerDialog.hpp"
#include "com/sun/star/ui/dialogs/XExecutableDialog.hpp"
#include "com/sun/star/lang/DisposedException.hpp"
#include "boost/scoped_array.hpp"
#include "com/sun/star/ui/dialogs/XDialogClosedListener.hpp"
#include "com/sun/star/bridge/XBridgeFactory.hpp"
#include <stdio.h>
#include <vector>


using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::unopkg;
using ::rtl::OUString;
namespace css = ::com::sun::star;
namespace {

//------------------------------------------------------------------------------
const char s_usingText [] =
"\n"
"using: " APP_NAME " add <options> extension-path...\n"
"       " APP_NAME " remove <options> extension-identifier...\n"
"       " APP_NAME " list <options> extension-identifier...\n"
"       " APP_NAME " reinstall <options>\n"
"       " APP_NAME " gui\n"
"       " APP_NAME " -V\n"
"       " APP_NAME " -h\n"
"\n"
"sub-commands:\n"
" add                     add extension\n"
" remove                  remove extensions by identifier\n"
" reinstall               expert feature: reinstall all deployed extensions\n"
" list                    list information about deployed extensions\n"
" gui                     raise Extension Manager Graphical User Interface (GUI)\n"
"\n"
"options:\n"
" -h, --help              this help\n"
" -V, --version           version information\n"
" -v, --verbose           verbose output to stdout\n"
" -f, --force             force overwriting existing extensions\n"
" -s, --suppress-license  prevents showing the license provided that\n"
"                         the extension allows it\n"
" --log-file <file>       custom log file; default: <cache-dir>/log.txt\n"
" --shared                expert feature: operate on shared installation\n"
"                                         deployment context;\n"
"                                         run only when no concurrent Office\n"
"                                         process(es) are running!\n"
" --deployment-context    expert feature: explicit deployment context\n"
"     <context>\n"
"\n"
"To learn more about the Extension Manager and extensions, see:\n"
"http://wiki.services.openoffice.org/wiki/Documentation/DevGuide/Extensions/Extensions\n\n";

//------------------------------------------------------------------------------
const OptionInfo s_option_infos [] = {
    { RTL_CONSTASCII_STRINGPARAM("help"), 'h', false },
    { RTL_CONSTASCII_STRINGPARAM("version"), 'V', false },
    { RTL_CONSTASCII_STRINGPARAM("verbose"), 'v', false },
    { RTL_CONSTASCII_STRINGPARAM("force"), 'f', false },
    { RTL_CONSTASCII_STRINGPARAM("log-file"), '\0', true },
    { RTL_CONSTASCII_STRINGPARAM("shared"), '\0', false },
    { RTL_CONSTASCII_STRINGPARAM("deployment-context"), '\0', true },
    { RTL_CONSTASCII_STRINGPARAM("bundled"), '\0', false},
    { RTL_CONSTASCII_STRINGPARAM("suppress-license"), 's', false},

    { 0, 0, '\0', false }
};

class DialogClosedListenerImpl :
    public ::cppu::WeakImplHelper1< ui::dialogs::XDialogClosedListener >
{
    osl::Condition & m_rDialogClosedCondition;

public:
    DialogClosedListenerImpl( osl::Condition & rDialogClosedCondition )
        : m_rDialogClosedCondition( rDialogClosedCondition ) {}

    // XEventListener (base of XDialogClosedListener)
    virtual void SAL_CALL disposing( lang::EventObject const & Source )
        throw (RuntimeException);

    // XDialogClosedListener
    virtual void SAL_CALL dialogClosed(
        ui::dialogs::DialogClosedEvent const & aEvent )
        throw (RuntimeException);
};

// XEventListener (base of XDialogClosedListener)
void DialogClosedListenerImpl::disposing( lang::EventObject const & )
    throw (RuntimeException)
{
    // nothing to do
}

// XDialogClosedListener
void DialogClosedListenerImpl::dialogClosed(
    ui::dialogs::DialogClosedEvent const & )
    throw (RuntimeException)
{
    m_rDialogClosedCondition.set();
}

// If a package had been installed with a pre OOo 2.2, it could not normally be
// found via its identifier; similarly (and for ease of use), a package
// installed with OOo 2.2 or later could not normally be found via its file
// name.
Reference<deployment::XPackage> findPackage(
    Reference<deployment::XPackageManager> const & manager,
    Reference<ucb::XCommandEnvironment > const & environment,
    OUString const & idOrFileName )
{
    Sequence< Reference<deployment::XPackage> > ps(
        manager->getDeployedPackages(
            Reference<task::XAbortChannel>(), environment ) );
    for ( sal_Int32 i = 0; i < ps.getLength(); ++i )
        if ( dp_misc::getIdentifier( ps[i] ) == idOrFileName )
            return ps[i];
    for ( sal_Int32 i = 0; i < ps.getLength(); ++i )
        if ( ps[i]->getName() == idOrFileName )
            return ps[i];
    return Reference<deployment::XPackage>();
}

} // anon namespace


//workaround for some reason the bridge threads which communicate with the uno.exe
//process are not releases on time
void disposeBridges(Reference<css::uno::XComponentContext> ctx)
{
    if (!ctx.is())
        return;

    Reference<css::bridge::XBridgeFactory> bridgeFac(
        ctx->getServiceManager()->createInstanceWithContext(
            OUSTR("com.sun.star.bridge.BridgeFactory"), ctx),
        UNO_QUERY);

    if (bridgeFac.is())
    {
        const Sequence< Reference<css::bridge::XBridge> >seqBridges = bridgeFac->getExistingBridges();
        for (sal_Int32 i = 0; i < seqBridges.getLength(); i++)
        {        
            Reference<css::lang::XComponent> comp(seqBridges[i], UNO_QUERY);
            if (comp.is())
            {
                try {
                    comp->dispose();
                }
                catch (css::lang::DisposedException& )
                {
                }
            }
        }
    }
}

//##############################################################################
extern "C" int unopkg_main()
{
    tools::extendApplicationEnvironment();
    DisposeGuard disposeGuard;
    bool bNoOtherErrorMsg = false;
    OUString subCommand;
    bool option_shared = false;
    bool option_force = false;
    bool option_verbose = false;
    bool option_bundled = false;
    bool option_suppressLicense = false;
    bool subcmd_add = false;
	bool subcmd_gui = false;
    OUString logFile;
    OUString deploymentContext;
    OUString cmdArg;
    ::std::vector<OUString> cmdPackages;
    
    OptionInfo const * info_shared = getOptionInfo(
        s_option_infos, OUSTR("shared") );
    OptionInfo const * info_force = getOptionInfo(
        s_option_infos, OUSTR("force") );
    OptionInfo const * info_verbose = getOptionInfo(
        s_option_infos, OUSTR("verbose") );
    OptionInfo const * info_log = getOptionInfo(
        s_option_infos, OUSTR("log-file") );
    OptionInfo const * info_context = getOptionInfo(
        s_option_infos, OUSTR("deployment-context") );
    OptionInfo const * info_help = getOptionInfo(
        s_option_infos, OUSTR("help") );
    OptionInfo const * info_version = getOptionInfo(
        s_option_infos, OUSTR("version") );
    OptionInfo const * info_bundled = getOptionInfo(
        s_option_infos, OUSTR("bundled") );
    OptionInfo const * info_suppressLicense = getOptionInfo(
        s_option_infos, OUSTR("suppress-license") );

    
    Reference<XComponentContext> xComponentContext;
    Reference<XComponentContext> xLocalComponentContext;
    
    try {
        sal_uInt32 nPos = 0;
        sal_uInt32 nCount = osl_getCommandArgCount();
        if (nCount == 0 || isOption( info_help, &nPos ))
        {
            dp_misc::writeConsole(s_usingText);
            return 0;
        }
        else if (isOption( info_version, &nPos )) {
            dp_misc::writeConsole("\n"APP_NAME" Version 3.0\n");
            return 0;
        }
        //consume all bootstrap variables which may occur before the subcommannd
        while(isBootstrapVariable(&nPos));
       
        if(nPos >= nCount)
            return 0;
        //get the sub command
        osl_getCommandArg( nPos, &subCommand.pData );
        ++nPos;
        subCommand = subCommand.trim();
        subcmd_add = subCommand.equalsAsciiL(
            RTL_CONSTASCII_STRINGPARAM("add") );
		subcmd_gui = subCommand.equalsAsciiL(
            RTL_CONSTASCII_STRINGPARAM("gui") );
        
        // sun-command options and packages:
        while (nPos < nCount)
        {
            if (readArgument( &cmdArg, info_log, &nPos )) {
                logFile = makeAbsoluteFileUrl(
                    cmdArg.trim(), getProcessWorkingDir() );
            }
            else if (!readOption( &option_verbose, info_verbose, &nPos ) &&
                     !readOption( &option_shared, info_shared, &nPos ) &&
                     !readOption( &option_force, info_force, &nPos ) &&
                     !readOption( &option_bundled, info_bundled, &nPos ) &&
                     !readOption( &option_suppressLicense, info_suppressLicense, &nPos ) &&
                     !readArgument( &deploymentContext, info_context, &nPos ) &&
                     !isBootstrapVariable(&nPos))
            {
                osl_getCommandArg( nPos, &cmdArg.pData );
                ++nPos;
                cmdArg = cmdArg.trim();
                if (cmdArg.getLength() > 0)
                {
                    if (cmdArg[ 0 ] == '-')
                    {
                        // is option:
                        dp_misc::writeConsoleError(
                                 OUSTR("\nERROR: unexpected option ") +
                                 cmdArg +
                                 OUSTR("!\n") +
                                 OUSTR("       Use " APP_NAME " ") + 
                                 toString(info_help) +
                                 OUSTR(" to print all options.\n"));
                        return 1;
                    }
                    else
                    {
                        // is package:
                        cmdPackages.push_back(
                            subcmd_add || subcmd_gui
                            ? makeAbsoluteFileUrl(
                                cmdArg, getProcessWorkingDir() )
                            : cmdArg );
                    }
                }
            }
        }

        //make sure the bundled option was provided together with shared
        if (option_bundled && !option_shared)
        {
            dp_misc::writeConsoleError(
                "\nERROR: option --bundled can only be used together with --shared!");
            return 1;
        }

        
        xComponentContext = getUNO(
            disposeGuard, option_verbose, option_shared, subcmd_gui,
            xLocalComponentContext );
        
        if (deploymentContext.getLength() == 0) {
            deploymentContext = option_shared ? OUSTR("shared") : OUSTR("user");
        }
        else
        {
            if (deploymentContext.equalsAsciiL(
                    RTL_CONSTASCII_STRINGPARAM("shared") )) {
                option_shared = true;
            }
            else if (option_shared) {
                dp_misc::writeConsoleError(
                    OUSTR("WARNING: explicit context given!  ") +
                    OUSTR("Ignoring option ") +
                    toString( info_shared ) +
                    OUSTR("!\n") );
            }
        }
        
        Reference<deployment::XPackageManagerFactory> xPackageManagerFactory(
            deployment::thePackageManagerFactory::get( xComponentContext ) );
        Reference<deployment::XPackageManager> xPackageManager(
            xPackageManagerFactory->getPackageManager( deploymentContext ) );
        
        Reference< ::com::sun::star::ucb::XCommandEnvironment > xCmdEnv(
            createCmdEnv( xComponentContext, logFile,
                          option_force, option_verbose, option_bundled, 
                          option_suppressLicense) );
        
        if (subcmd_add ||
            subCommand.equalsAsciiL(
                RTL_CONSTASCII_STRINGPARAM("remove") ))
        {
            for ( ::std::size_t pos = 0; pos < cmdPackages.size(); ++pos )
            {
                OUString const & cmdPackage = cmdPackages[ pos ];
                if (subcmd_add)
                {
                    Reference<deployment::XPackage> xPackage(
                        xPackageManager->addPackage(
                            cmdPackage, OUString() /* to be detected */,
                            Reference<task::XAbortChannel>(), xCmdEnv ) );
                    OSL_ASSERT( xPackage.is() );
                }
                else
                {
                    try
                    {
                        xPackageManager->removePackage(
                            cmdPackage, cmdPackage,
                            Reference<task::XAbortChannel>(), xCmdEnv );
                    }
                    catch (lang::IllegalArgumentException &)
                    {
                        Reference<deployment::XPackage> p(
                            findPackage(
                                xPackageManager, xCmdEnv, cmdPackage ) );
                        //Todo. temporary preventing exception in bundled case.
                        //In case of a bundled extension, remove would be called as a result of 
                        //uninstalling a rpm. Then we do not want to show an error when the 
                        //extension does not exist, because the package will be uninstalled anyway
                        //and the error would only confuse people.
                        if ( !p.is() && !option_bundled)
                            throw;
                        else if (p.is())
                            xPackageManager->removePackage(
                                ::dp_misc::getIdentifier(p), p->getName(),
                                Reference<task::XAbortChannel>(), xCmdEnv );
                    }
                }
            }
        }
        else if (subCommand.equalsAsciiL(
                     RTL_CONSTASCII_STRINGPARAM("reinstall") ))
        {
            xPackageManager->reinstallDeployedPackages(
                Reference<task::XAbortChannel>(), xCmdEnv );
        }
        else if (subCommand.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM("list") ))
        {
            Sequence< Reference<deployment::XPackage> > packages;
            if (cmdPackages.empty())
            {
                packages = xPackageManager->getDeployedPackages(
                    Reference<task::XAbortChannel>(), xCmdEnv );
                dp_misc::writeConsole(
                    OUSTR("all deployed ") + deploymentContext + OUSTR(" packages:\n"));
            }
            else
            {
                packages.realloc( cmdPackages.size() );
                for ( ::std::size_t pos = 0; pos < cmdPackages.size(); ++pos )
                    try
                    {
                        packages[ pos ] = xPackageManager->getDeployedPackage(
                            cmdPackages[ pos ], cmdPackages[ pos ], xCmdEnv );
                    }
                    catch (lang::IllegalArgumentException &)
                    {
                        packages[ pos ] = findPackage(
                            xPackageManager, xCmdEnv, cmdPackages[ pos ] );
                        if ( !packages[ pos ].is() )
                            throw;
                    }
            }
            printf_packages( packages, xCmdEnv );
        }
        else if (subCommand.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM("gui") ))
        {
            Reference<ui::dialogs::XAsynchronousExecutableDialog> xDialog(
                deployment::ui::PackageManagerDialog::createAndInstall(
                    xComponentContext, 
                    cmdPackages.size() > 0 ? cmdPackages[0] : OUString() ));

            osl::Condition dialogEnded;
            dialogEnded.reset();

            Reference< ui::dialogs::XDialogClosedListener > xListener(
                new DialogClosedListenerImpl( dialogEnded ) );

            xDialog->startExecuteModal(xListener);
            dialogEnded.wait();
        }
        else
        {
            dp_misc::writeConsoleError(
                OUSTR("\nERROR: unknown sub-command ") + 
                subCommand +
                OUSTR("!\n") + 
                OUSTR("       Use " APP_NAME " ") + 
                toString(info_help) +
                OUSTR(" to print all options.\n"));
            return 1;
        }
        
        if (option_verbose)
            dp_misc::writeConsole(OUSTR("\n"APP_NAME" done.\n"));
        //Force to release all bridges which connect us to the child processes
        disposeBridges(xLocalComponentContext);
        return 0;
    }
    catch (ucb::CommandFailedException &e) 
    {
        dp_misc::writeConsoleError(e.Message + OUSTR("\n"));
        bNoOtherErrorMsg = true;    
    }
    catch (ucb::CommandAbortedException &) 
    {
        dp_misc::writeConsoleError("\n"APP_NAME" aborted!\n");
    }
    catch (deployment::DeploymentException & exc) 
    {
		OUString cause;
		if (option_verbose)
		{
			cause = ::comphelper::anyToString(exc.Cause);
		}
		else
		{
			css::uno::Exception e;
			if (exc.Cause >>= e)
				cause = e.Message; 
		}

		dp_misc::writeConsoleError(
            OUSTR("\nERROR: ") + exc.Message + OUSTR("\n"));
		if (cause.getLength())
			dp_misc::writeConsoleError(
				OUSTR("       Cause: ") + cause + OUSTR("\n"));
    }
    catch (LockFileException & e) 
    {
        if (!subcmd_gui)
            dp_misc::writeConsoleError(e.Message);
        bNoOtherErrorMsg = true;
    }
    catch (::com::sun::star::uno::Exception & e ) {
        Any exc( ::cppu::getCaughtException() );
 
        dp_misc::writeConsoleError(
            OUSTR("\nERROR: ") +  
            OUString(option_verbose  ? e.Message + OUSTR("\nException details: \n") + 
            ::comphelper::anyToString(exc) : e.Message) +
            OUSTR("\n"));
    }
    if (!bNoOtherErrorMsg)
        dp_misc::writeConsoleError("\n"APP_NAME" failed.\n");
    disposeBridges(xLocalComponentContext);
    return 1;
}


