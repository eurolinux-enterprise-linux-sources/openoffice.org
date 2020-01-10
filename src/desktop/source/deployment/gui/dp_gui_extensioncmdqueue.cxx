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
#include "precompiled_desktop.hxx"




#include "sal/config.h"

#include <cstddef>

#include "com/sun/star/beans/PropertyValue.hpp"

#include "com/sun/star/deployment/DependencyException.hpp"
#include "com/sun/star/deployment/LicenseException.hpp"
#include "com/sun/star/deployment/LicenseIndividualAgreementException.hpp"
#include "com/sun/star/deployment/VersionException.hpp"
#include "com/sun/star/deployment/InstallException.hpp"
#include "com/sun/star/deployment/PlatformException.hpp"

#include "com/sun/star/deployment/ui/LicenseDialog.hpp"
#include "com/sun/star/deployment/DeploymentException.hpp"
#include "com/sun/star/deployment/UpdateInformationProvider.hpp"
#include "com/sun/star/deployment/XPackage.hpp"
#include "com/sun/star/deployment/XPackageManager.hpp"

#include "com/sun/star/task/XAbortChannel.hpp"
#include "com/sun/star/task/XInteractionAbort.hpp"
#include "com/sun/star/task/XInteractionApprove.hpp"

#include "com/sun/star/ucb/CommandAbortedException.hpp"
#include "com/sun/star/ucb/CommandFailedException.hpp"
#include "com/sun/star/ucb/XCommandEnvironment.hpp"

#include "com/sun/star/ui/dialogs/ExecutableDialogResults.hpp"

#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/RuntimeException.hpp"
#include "com/sun/star/uno/Sequence.hxx"
#include "com/sun/star/uno/XInterface.hpp"
#include "com/sun/star/uno/TypeClass.hpp"
#include "osl/diagnose.h"
#include "osl/mutex.hxx"
#include "rtl/ref.hxx"
#include "rtl/ustring.h"
#include "rtl/ustring.hxx"
#include "sal/types.h"
#include "ucbhelper/content.hxx"
#include "cppuhelper/exc_hlp.hxx"
#include "cppuhelper/implbase3.hxx"
#include "comphelper/anytostring.hxx"
#include "vcl/msgbox.hxx"
#include "toolkit/helper/vclunohelper.hxx"

#include "dp_gui.h"
#include "dp_gui_thread.hxx"
#include "dp_gui_extensioncmdqueue.hxx"
#include "dp_gui_dependencydialog.hxx"
#include "dp_gui_dialog2.hxx"
#include "dp_gui_shared.hxx"
#include "dp_gui_theextmgr.hxx"
#include "dp_gui_updatedialog.hxx"
#include "dp_gui_updateinstalldialog.hxx"
#include "dp_dependencies.hxx"
#include "dp_identifier.hxx"
#include "dp_version.hxx"

#include <queue>
#include <boost/shared_ptr.hpp>

#if (defined(_MSC_VER) && (_MSC_VER < 1400))
#define _WIN32_WINNT 0x0400
#endif

#ifdef WNT
#include "tools/prewin.h"
#include <objbase.h>
#include "tools/postwin.h"
#endif


using namespace ::com::sun::star;
using ::rtl::OUString;

namespace {

OUString getVersion( const uno::Reference< deployment::XPackage > &rPackage )
{
    OUString sVersion( rPackage->getVersion());
    return ( sVersion.getLength() == 0 ) ? OUString( RTL_CONSTASCII_USTRINGPARAM( "0" ) ) : sVersion;
}

}


namespace dp_gui {

//==============================================================================

class ProgressCmdEnv
    : public ::cppu::WeakImplHelper3< ucb::XCommandEnvironment,
                                      task::XInteractionHandler,
                                      ucb::XProgressHandler >
{
    uno::Reference< task::XInteractionHandler> m_xHandler;
    uno::Reference< uno::XComponentContext > m_xContext;
    uno::Reference< task::XAbortChannel> m_xAbortChannel;

    DialogHelper   *m_pDialogHelper;
    OUString        m_sTitle;
    bool            m_bAborted;
    bool            m_bWarnUser;
    sal_Int32       m_nCurrentProgress;

    void updateProgress();

    void update_( uno::Any const & Status ) throw ( uno::RuntimeException );

public:
    virtual ~ProgressCmdEnv();

    /** When param bAskWhenInstalling = true, then the user is asked if he 
    agrees to install this extension. In case this extension is already installed
    then the user is also notified and asked if he wants to replace that existing 
    extension. In first case an interaction request with an InstallException 
    will be handled and in the second case a VersionException will be handled.
    */

    ProgressCmdEnv( const uno::Reference< uno::XComponentContext > rContext,
                    DialogHelper *pDialogHelper,
                    const OUString &rTitle )
        :   m_xContext( rContext ),
            m_pDialogHelper( pDialogHelper ),
            m_sTitle( rTitle ),
            m_bAborted( false ),
            m_bWarnUser( false )
    {}

    Dialog * activeDialog() { return m_pDialogHelper->getWindow(); }

    void setTitle( const OUString& rNewTitle ) { m_sTitle = rNewTitle; }
    void startProgress();
    void stopProgress();
    void progressSection( const OUString &rText,
                          const uno::Reference< task::XAbortChannel > &xAbortChannel = 0 );
    inline bool isAborted() const { return m_bAborted; }
    inline void setWarnUser( bool bNewVal ) { m_bWarnUser = bNewVal; }

    // XCommandEnvironment
    virtual uno::Reference< task::XInteractionHandler > SAL_CALL getInteractionHandler()
        throw ( uno::RuntimeException );
    virtual uno::Reference< ucb::XProgressHandler > SAL_CALL getProgressHandler()
        throw ( uno::RuntimeException );

    // XInteractionHandler
    virtual void SAL_CALL handle( uno::Reference< task::XInteractionRequest > const & xRequest )
        throw ( uno::RuntimeException );

    // XProgressHandler
    virtual void SAL_CALL push( uno::Any const & Status )
        throw ( uno::RuntimeException );
    virtual void SAL_CALL update( uno::Any const & Status )
        throw ( uno::RuntimeException );
    virtual void SAL_CALL pop() throw ( uno::RuntimeException );
};

//------------------------------------------------------------------------------
struct ExtensionCmd
{
    enum E_CMD_TYPE { ADD, ENABLE, DISABLE, REMOVE, CHECK_FOR_UPDATES };

    E_CMD_TYPE  m_eCmdType;
    bool        m_bWarnUser;
    OUString    m_sExtensionURL;
    uno::Reference< deployment::XPackageManager > m_xPackageManager;
    uno::Reference< deployment::XPackage >        m_xPackage;
    std::vector< TUpdateListEntry >               m_vExtensionList;

    ExtensionCmd( const E_CMD_TYPE eCommand,
                  const uno::Reference< deployment::XPackageManager > &rPackageManager,
                  const OUString &rExtensionURL,
                  const bool bWarnUser )
        : m_eCmdType( eCommand ),
          m_bWarnUser( bWarnUser ),
          m_sExtensionURL( rExtensionURL ),
          m_xPackageManager( rPackageManager ) {};
    ExtensionCmd( const E_CMD_TYPE eCommand,
                  const uno::Reference< deployment::XPackageManager > &rPackageManager,
                  const uno::Reference< deployment::XPackage > &rPackage )
        : m_eCmdType( eCommand ),
          m_bWarnUser( false ),
          m_xPackageManager( rPackageManager ),
          m_xPackage( rPackage ) {};
    ExtensionCmd( const E_CMD_TYPE eCommand,
                  const uno::Reference< deployment::XPackage > &rPackage )
        : m_eCmdType( eCommand ),
          m_bWarnUser( false ),
          m_xPackage( rPackage ) {};
    ExtensionCmd( const E_CMD_TYPE eCommand,
                  const std::vector< TUpdateListEntry > &vExtensionList )
        : m_eCmdType( eCommand ),
          m_bWarnUser( false ),
          m_vExtensionList( vExtensionList ) {};
};

typedef ::boost::shared_ptr< ExtensionCmd > TExtensionCmd;

//------------------------------------------------------------------------------
class ExtensionCmdQueue::Thread: public dp_gui::Thread
{
public:
    Thread( DialogHelper *pDialogHelper,
            TheExtensionManager *pManager,
            const uno::Reference< uno::XComponentContext > & rContext );

    void addExtension( const uno::Reference< deployment::XPackageManager > &rPackageManager,
                       const OUString &rExtensionURL,
                       const bool bWarnUser );
    void removeExtension( const uno::Reference< deployment::XPackageManager > &rPackageManager,
                          const uno::Reference< deployment::XPackage > &rPackage );
    void enableExtension( const uno::Reference< deployment::XPackage > &rPackage,
                          const bool bEnable );
    void checkForUpdates( const std::vector< TUpdateListEntry > &vExtensionList );
    void stop();
    bool hasTerminated();
    bool isBusy();

    static OUString searchAndReplaceAll( const OUString &rSource,
                                         const OUString &rWhat,
                                         const OUString &rWith );
private:
    Thread( Thread & ); // not defined
    void operator =( Thread & ); // not defined

    virtual ~Thread();

    virtual void execute();
    virtual void SAL_CALL onTerminated();

    void _addExtension( ::rtl::Reference< ProgressCmdEnv > &rCmdEnv,
                        const uno::Reference< deployment::XPackageManager > &xPackageManager,
                        const OUString &rPackageURL,
                        const bool bWarnUser );
    void _removeExtension( ::rtl::Reference< ProgressCmdEnv > &rCmdEnv,
                           const uno::Reference< deployment::XPackageManager > &xPackageManager,
                           const uno::Reference< deployment::XPackage > &xPackage );
    void _enableExtension( ::rtl::Reference< ProgressCmdEnv > &rCmdEnv,
                           const uno::Reference< deployment::XPackage > &xPackage );
    void _disableExtension( ::rtl::Reference< ProgressCmdEnv > &rCmdEnv,
                            const uno::Reference< deployment::XPackage > &xPackage );
    void _checkForUpdates( const std::vector< TUpdateListEntry > &vExtensionList );

    enum Input { NONE, START, STOP };

    uno::Reference< uno::XComponentContext > m_xContext;
    std::queue< TExtensionCmd >              m_queue;

    DialogHelper *m_pDialogHelper;
    TheExtensionManager *m_pManager;

    const OUString   m_sEnablingPackages;
    const OUString   m_sDisablingPackages;
    const OUString   m_sAddingPackages;
    const OUString   m_sRemovingPackages;
    const OUString   m_sDefaultCmd;
    osl::Condition   m_wakeup;
    osl::Mutex       m_mutex;
    Input            m_eInput;
    bool             m_bTerminated;
    bool             m_bStopped;
    bool             m_bWorking;
};

//------------------------------------------------------------------------------
void ProgressCmdEnv::startProgress()
{
    m_nCurrentProgress = 0;

    if ( m_pDialogHelper )
        m_pDialogHelper->showProgress( true );
}

//------------------------------------------------------------------------------
void ProgressCmdEnv::stopProgress()
{
    if ( m_pDialogHelper )
        m_pDialogHelper->showProgress( false );
}

//------------------------------------------------------------------------------
void ProgressCmdEnv::progressSection( const OUString &rText,
                                      const uno::Reference< task::XAbortChannel > &xAbortChannel )
{
    m_xAbortChannel = xAbortChannel;
    if (! m_bAborted)
    {
        m_nCurrentProgress = 0;
        if ( m_pDialogHelper )
        {
            m_pDialogHelper->updateProgress( rText, xAbortChannel );
            m_pDialogHelper->updateProgress( 5 );
        }
    }
}

//------------------------------------------------------------------------------
void ProgressCmdEnv::updateProgress()
{
    if ( ! m_bAborted )
    {
        long nProgress = ((m_nCurrentProgress*5) % 100) + 5;
        if ( m_pDialogHelper )
            m_pDialogHelper->updateProgress( nProgress );
    }
}

//------------------------------------------------------------------------------
ProgressCmdEnv::~ProgressCmdEnv()
{
    // TODO: stop all threads and wait
}


//------------------------------------------------------------------------------
// XCommandEnvironment
//------------------------------------------------------------------------------
uno::Reference< task::XInteractionHandler > ProgressCmdEnv::getInteractionHandler()
    throw ( uno::RuntimeException )
{
    return this;
}

//------------------------------------------------------------------------------
uno::Reference< ucb::XProgressHandler > ProgressCmdEnv::getProgressHandler()
    throw ( uno::RuntimeException )
{
    return this;
}

//------------------------------------------------------------------------------
// XInteractionHandler
//------------------------------------------------------------------------------
void ProgressCmdEnv::handle( uno::Reference< task::XInteractionRequest > const & xRequest )
    throw ( uno::RuntimeException )
{
    uno::Any request( xRequest->getRequest() );
    OSL_ASSERT( request.getValueTypeClass() == uno::TypeClass_EXCEPTION );
    dp_misc::TRACE( OUSTR("[dp_gui_cmdenv.cxx] incoming request:\n")
        + ::comphelper::anyToString(request) + OUSTR("\n"));
    
    lang::WrappedTargetException wtExc;
    deployment::DependencyException depExc;
	deployment::LicenseException licExc;
    deployment::LicenseIndividualAgreementException licAgreementExc;
    deployment::VersionException verExc;
	deployment::InstallException instExc;
    deployment::PlatformException platExc;

    // selections:
    bool approve = false;
    bool abort = false;

    if (request >>= wtExc) {
        // handable deployment error signalled, e.g.
        // bundle item registration failed, notify cause only:
        uno::Any cause;
        deployment::DeploymentException dpExc;
        if (wtExc.TargetException >>= dpExc)
            cause = dpExc.Cause;
        else {
            ucb::CommandFailedException cfExc;
            if (wtExc.TargetException >>= cfExc)
                cause = cfExc.Reason;
            else
                cause = wtExc.TargetException;
        }
        update_( cause );
        
        // ignore intermediate errors of legacy packages, i.e.
        // former pkgchk behaviour:
        const uno::Reference< deployment::XPackage > xPackage( wtExc.Context, uno::UNO_QUERY );
        OSL_ASSERT( xPackage.is() );
        if ( xPackage.is() )
        {
            const uno::Reference< deployment::XPackageTypeInfo > xPackageType( xPackage->getPackageType() );
            OSL_ASSERT( xPackageType.is() );
            if (xPackageType.is())
            {
                approve = ( xPackage->isBundle() &&
                            xPackageType->getMediaType().matchAsciiL(
                                RTL_CONSTASCII_STRINGPARAM(
                                    "application/"
                                    "vnd.sun.star.legacy-package-bundle") ));
            }
        }
        abort = !approve;        
    }
    else if (request >>= depExc)
    {
        std::vector< rtl::OUString > deps;
        for (sal_Int32 i = 0; i < depExc.UnsatisfiedDependencies.getLength();
             ++i)
        {
            deps.push_back(
                dp_misc::Dependencies::getErrorText( depExc.UnsatisfiedDependencies[i]) );
        }
        {
            vos::OGuard guard(Application::GetSolarMutex());
            short n = DependencyDialog( m_pDialogHelper? m_pDialogHelper->getWindow() : NULL, deps ).Execute();
            // Distinguish between closing the dialog and programatically
            // canceling the dialog (headless VCL):
            approve = n == RET_OK
                || (n == RET_CANCEL && !Application::IsDialogCancelEnabled());
        }
    }
    else if (request >>= licAgreementExc)
    {   
        vos::OGuard aSolarGuard( Application::GetSolarMutex() );
        ResId warnId(WARNINGBOX_NOSHAREDALLOWED, *DeploymentGuiResMgr::get());
        WarningBox warn( m_pDialogHelper? m_pDialogHelper->getWindow() : NULL, warnId);
        String msgText = warn.GetMessText();
        msgText.SearchAndReplaceAllAscii( "%PRODUCTNAME", BrandName::get() );
        msgText.SearchAndReplaceAllAscii("%NAME", licAgreementExc.ExtensionName);
        warn.SetMessText(msgText);
        warn.Execute();			
          abort = true;
    }
	else if (request >>= licExc)
    {
        uno::Reference< ui::dialogs::XExecutableDialog > xDialog(
            deployment::ui::LicenseDialog::create(
            m_xContext, VCLUnoHelper::GetInterface( m_pDialogHelper? m_pDialogHelper->getWindow() : NULL ), licExc.Text ) );
        sal_Int16 res = xDialog->execute();
        if ( res == ui::dialogs::ExecutableDialogResults::CANCEL )
            abort = true;
        else if ( res == ui::dialogs::ExecutableDialogResults::OK )
            approve = true;
        else
        {
            OSL_ASSERT(0);
        }
	}
    else if (request >>= verExc)
    {
        sal_uInt32 id;
        switch (dp_misc::comparePackageVersions( verExc.New, verExc.Deployed ))
        {
        case dp_misc::LESS:
            id = RID_WARNINGBOX_VERSION_LESS;
            break;
        case dp_misc::EQUAL:
            id = RID_WARNINGBOX_VERSION_EQUAL;
            break;
        default: // dp_misc::GREATER
            id = RID_WARNINGBOX_VERSION_GREATER;
            break;
        }
        OSL_ASSERT( verExc.New.is() && verExc.Deployed.is() );
        bool bEqualNames = verExc.New->getDisplayName().equals(
            verExc.Deployed->getDisplayName());
        {
            vos::OGuard guard(Application::GetSolarMutex());
            WarningBox box( m_pDialogHelper? m_pDialogHelper->getWindow() : NULL, ResId(id, *DeploymentGuiResMgr::get()));
            String s;
            if (bEqualNames)
            {
                s = box.GetMessText();
            }
            else if (id == RID_WARNINGBOX_VERSION_EQUAL) 
            {
                //hypothetical: requires two instances of an extension with the same 
                //version to have different display names. Probably the developer forgot 
                //to change the version.
                s = String(ResId(RID_STR_WARNINGBOX_VERSION_EQUAL_DIFFERENT_NAMES, *DeploymentGuiResMgr::get()));
            }
            else if (id == RID_WARNINGBOX_VERSION_LESS)
            {
                s = String(ResId(RID_STR_WARNINGBOX_VERSION_LESS_DIFFERENT_NAMES, *DeploymentGuiResMgr::get()));
            }
            else if (id == RID_WARNINGBOX_VERSION_GREATER)
            {
               s = String(ResId(RID_STR_WARNINGBOX_VERSION_GREATER_DIFFERENT_NAMES, *DeploymentGuiResMgr::get()));
            }
            s.SearchAndReplaceAllAscii( "$NAME", verExc.New->getDisplayName());
            s.SearchAndReplaceAllAscii( "$OLDNAME", verExc.Deployed->getDisplayName());
            s.SearchAndReplaceAllAscii( "$NEW", getVersion(verExc.New) );
            s.SearchAndReplaceAllAscii( "$DEPLOYED", getVersion(verExc.Deployed) );
            box.SetMessText(s);
            approve = box.Execute() == RET_OK;
            abort = !approve;
        }
    }
	else if (request >>= instExc)
	{
        if ( ! m_bWarnUser )
        {
            approve = true;
        }
        else
        {
            if ( m_pDialogHelper )
            {
                vos::OGuard guard(Application::GetSolarMutex());
    
                approve = m_pDialogHelper->installExtensionWarn( instExc.New->getDisplayName() );
            }
            else
                approve = false;
            abort = !approve;
        }
	}
    else if (request >>= platExc)
    {
        vos::OGuard guard( Application::GetSolarMutex() );
        String sMsg( ResId( RID_STR_UNSUPPORTED_PLATFORM, *DeploymentGuiResMgr::get() ) );
        sMsg.SearchAndReplaceAllAscii( "%Name", platExc.package->getDisplayName() );
        ErrorBox box( m_pDialogHelper? m_pDialogHelper->getWindow() : NULL, WB_OK, sMsg );
        box.Execute();
        approve = true;
    }

	if (approve == false && abort == false)
    {
        // forward to UUI handler:
        if (! m_xHandler.is()) {
            // late init:
            uno::Sequence< uno::Any > handlerArgs( 1 );
            handlerArgs[ 0 ] <<= beans::PropertyValue(
                OUSTR("Context"), -1, uno::Any( m_sTitle ),
                beans::PropertyState_DIRECT_VALUE );
             m_xHandler.set( m_xContext->getServiceManager()
                            ->createInstanceWithArgumentsAndContext(
                                OUSTR("com.sun.star.uui.InteractionHandler"),
                                handlerArgs, m_xContext ), uno::UNO_QUERY_THROW );
        }
        m_xHandler->handle( xRequest );
    }
	else
	{
        // select:
        uno::Sequence< uno::Reference< task::XInteractionContinuation > > conts(
            xRequest->getContinuations() );
        uno::Reference< task::XInteractionContinuation > const * pConts = conts.getConstArray();
        sal_Int32 len = conts.getLength();
        for ( sal_Int32 pos = 0; pos < len; ++pos )
        {
            if (approve) {
                uno::Reference< task::XInteractionApprove > xInteractionApprove( pConts[ pos ], uno::UNO_QUERY );
                if (xInteractionApprove.is()) {
                    xInteractionApprove->select();
                    // don't query again for ongoing continuations:
                    approve = false;
                }
            }
            else if (abort) {
                uno::Reference< task::XInteractionAbort > xInteractionAbort( pConts[ pos ], uno::UNO_QUERY );
                if (xInteractionAbort.is()) {           
                    xInteractionAbort->select();
                    // don't query again for ongoing continuations:
                    abort = false;
                }
            }
        }
	}
}

//------------------------------------------------------------------------------
// XProgressHandler
//------------------------------------------------------------------------------
void ProgressCmdEnv::push( uno::Any const & rStatus )
    throw( uno::RuntimeException )
{
    update_( rStatus );
}

//------------------------------------------------------------------------------
void ProgressCmdEnv::update_( uno::Any const & rStatus )
    throw( uno::RuntimeException )
{
    OUString text;
    if ( rStatus.hasValue() && !( rStatus >>= text) )
    {
        if ( rStatus.getValueTypeClass() == uno::TypeClass_EXCEPTION )
            text = static_cast< uno::Exception const *>( rStatus.getValue() )->Message;
        if ( text.getLength() == 0 )
            text = ::comphelper::anyToString( rStatus ); // fallback

        const ::vos::OGuard aGuard( Application::GetSolarMutex() );
        const ::std::auto_ptr< ErrorBox > aBox( new ErrorBox( m_pDialogHelper? m_pDialogHelper->getWindow() : NULL, WB_OK, text ) );
        aBox->Execute();
    }
    ++m_nCurrentProgress;
    updateProgress();
}

//------------------------------------------------------------------------------
void ProgressCmdEnv::update( uno::Any const & rStatus )
    throw( uno::RuntimeException )
{
    update_( rStatus );
}

//------------------------------------------------------------------------------
void ProgressCmdEnv::pop()
    throw( uno::RuntimeException )
{
    update_( uno::Any() ); // no message
}

//------------------------------------------------------------------------------
ExtensionCmdQueue::Thread::Thread( DialogHelper *pDialogHelper,
                                   TheExtensionManager *pManager,
                                   const uno::Reference< uno::XComponentContext > & rContext ) :
    m_xContext( rContext ),
    m_pDialogHelper( pDialogHelper ),
    m_pManager( pManager ),
    m_sEnablingPackages( DialogHelper::getResourceString( RID_STR_ENABLING_PACKAGES ) ),
    m_sDisablingPackages( DialogHelper::getResourceString( RID_STR_DISABLING_PACKAGES ) ),
    m_sAddingPackages( DialogHelper::getResourceString( RID_STR_ADDING_PACKAGES ) ),
    m_sRemovingPackages( DialogHelper::getResourceString( RID_STR_REMOVING_PACKAGES ) ),
    m_sDefaultCmd( DialogHelper::getResourceString( RID_STR_ADD_PACKAGES ) ),
    m_eInput( NONE ),
    m_bTerminated( false ),
    m_bStopped( false ),
    m_bWorking( false )
{
    OSL_ASSERT( pDialogHelper );
}

//------------------------------------------------------------------------------
void ExtensionCmdQueue::Thread::addExtension( const uno::Reference< deployment::XPackageManager > &rPackageManager,
                                              const ::rtl::OUString &rExtensionURL,
                                              const bool bWarnUser )
{ 
    ::osl::MutexGuard aGuard( m_mutex );

    //If someone called stop then we do not add the extension -> game over!
    if ( m_bStopped )
        return;

    if ( rExtensionURL.getLength() )
    {
        TExtensionCmd pEntry( new ExtensionCmd( ExtensionCmd::ADD, rPackageManager, rExtensionURL, bWarnUser ) );

        m_queue.push( pEntry );
        m_eInput = START;
        m_wakeup.set();
    }
}

//------------------------------------------------------------------------------
void ExtensionCmdQueue::Thread::removeExtension( const uno::Reference< deployment::XPackageManager > &rPackageManager,
                                                 const uno::Reference< deployment::XPackage > &rPackage )
{ 
    ::osl::MutexGuard aGuard( m_mutex );

    //If someone called stop then we do not remove the extension -> game over!
    if ( m_bStopped )
        return;

    if ( rPackageManager.is() && rPackage.is() )
    {
        TExtensionCmd pEntry( new ExtensionCmd( ExtensionCmd::REMOVE, rPackageManager, rPackage ) );

        m_queue.push( pEntry );
        m_eInput = START;
        m_wakeup.set();
    }
}

//------------------------------------------------------------------------------
void ExtensionCmdQueue::Thread::enableExtension( const uno::Reference< deployment::XPackage > &rPackage,
                                                 const bool bEnable )
{ 
    ::osl::MutexGuard aGuard( m_mutex );

    //If someone called stop then we do not remove the extension -> game over!
    if ( m_bStopped )
        return;

    if ( rPackage.is() )
    {
        TExtensionCmd pEntry( new ExtensionCmd( bEnable ? ExtensionCmd::ENABLE :
                                                          ExtensionCmd::DISABLE,
                                                rPackage ) );
        m_queue.push( pEntry );
        m_eInput = START;
        m_wakeup.set();
    }
}

//------------------------------------------------------------------------------
void ExtensionCmdQueue::Thread::checkForUpdates( const std::vector< TUpdateListEntry > &vExtensionList )
{ 
    ::osl::MutexGuard aGuard( m_mutex );

    //If someone called stop then we do not update the extension -> game over!
    if ( m_bStopped )
        return;

    TExtensionCmd pEntry( new ExtensionCmd( ExtensionCmd::CHECK_FOR_UPDATES, vExtensionList ) );
    m_queue.push( pEntry );
    m_eInput = START;
    m_wakeup.set();
}

//------------------------------------------------------------------------------
//Stopping this thread will not abort the installation of extensions.
void ExtensionCmdQueue::Thread::stop() 
{
    osl::MutexGuard aGuard( m_mutex );
    m_bStopped = true;
    m_eInput = STOP;
    m_wakeup.set();
}

//------------------------------------------------------------------------------
bool ExtensionCmdQueue::Thread::hasTerminated() 
{
    osl::MutexGuard aGuard( m_mutex );
    return m_bTerminated;
}

//------------------------------------------------------------------------------
bool ExtensionCmdQueue::Thread::isBusy() 
{
    osl::MutexGuard aGuard( m_mutex );
    return m_bWorking;
}

//------------------------------------------------------------------------------
ExtensionCmdQueue::Thread::~Thread() {}

//------------------------------------------------------------------------------
void ExtensionCmdQueue::Thread::execute() 
{
#ifdef WNT
    //Needed for use of the service "com.sun.star.system.SystemShellExecute" in 
    //DialogHelper::openWebBrowser
    CoUninitialize();
    HRESULT r = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
#endif
    for (;;) 
    {
        if ( m_wakeup.wait() != osl::Condition::result_ok )
        {
            dp_misc::TRACE( "dp_gui::ExtensionCmdQueue::Thread::run: ignored "
                       "osl::Condition::wait failure\n" );
        }
        m_wakeup.reset();

        int nSize;
        Input eInput;
        {
            osl::MutexGuard aGuard( m_mutex );
            eInput = m_eInput;
            m_eInput = NONE;
            nSize = m_queue.size();
            m_bWorking = false;
        }

        // If this thread has been woken up by anything else except start, stop
        // then input is NONE and we wait again.
        // We only install the extension which are currently in the queue.
        // The progressbar will be set to show the progress of the current number
        // of extensions. If we allowed to add extensions now then the progressbar may 
        // have reached the end while we still install newly added extensions.
        if ( ( eInput == NONE ) || ( nSize == 0 ) )
            continue;
        if ( eInput == STOP )
            break;
 
        ::rtl::Reference< ProgressCmdEnv > currentCmdEnv( new ProgressCmdEnv( m_xContext, m_pDialogHelper, m_sDefaultCmd ) );

        // Do not lock the following part with addExtension. addExtension may be called in the main thread.
        // If the message box "Do you want to install the extension (or similar)" is shown and then
        // addExtension is called, which then blocks the main thread, then we deadlock.
        bool bStartProgress = true;

        while ( !currentCmdEnv->isAborted() && --nSize >= 0 )
        {
            {
                osl::MutexGuard aGuard( m_mutex );
                m_bWorking = true;
            }

            try
            {
                TExtensionCmd pEntry;
                {
                    ::osl::MutexGuard queueGuard( m_mutex );
                    pEntry = m_queue.front();
                    m_queue.pop();
                }

                if ( bStartProgress && ( pEntry->m_eCmdType != ExtensionCmd::CHECK_FOR_UPDATES ) )
                {
                    currentCmdEnv->startProgress();
                    bStartProgress = false;
                }

                switch ( pEntry->m_eCmdType ) {
                case ExtensionCmd::ADD :
                    _addExtension( currentCmdEnv, pEntry->m_xPackageManager, pEntry->m_sExtensionURL, pEntry->m_bWarnUser );
                    break;
                case ExtensionCmd::REMOVE :
                    _removeExtension( currentCmdEnv, pEntry->m_xPackageManager, pEntry->m_xPackage );
                    break;
                case ExtensionCmd::ENABLE :
                    _enableExtension( currentCmdEnv, pEntry->m_xPackage );
                    break;
                case ExtensionCmd::DISABLE :
                    _disableExtension( currentCmdEnv, pEntry->m_xPackage );
                    break;
                case ExtensionCmd::CHECK_FOR_UPDATES :
                    _checkForUpdates( pEntry->m_vExtensionList );
                    break;
                }
            } 
            //catch ( deployment::DeploymentException &) 
            //{   
            //}
            //catch ( lang::IllegalArgumentException &) 
            //{
            //}
            catch ( ucb::CommandAbortedException & ) 
            {
                //This exception is thrown when the user clicks cancel on the progressbar.
                //Then we cancel the installation of all extensions and remove them from 
                //the queue.
                {
                    ::osl::MutexGuard queueGuard2(m_mutex);
                    while ( --nSize >= 0 )
                        m_queue.pop();
                }
                break;
            }
            catch ( ucb::CommandFailedException & ) 
            {
                //This exception is thrown when a user clicked cancel in the messagebox which was 
                //startet by the interaction handler. For example the user will be asked if he/she
                //really wants to install the extension.
                //These interaction are run for exectly one extension at a time. Therefore we continue
                //with installing the remaining extensions.
                continue;
            } 
            catch ( uno::Exception & ) 
            {
                //Todo display the user an error
                //see also DialogImpl::SyncPushButton::Click()
                uno::Any exc( ::cppu::getCaughtException() );
                OUString msg;
                deployment::DeploymentException dpExc;
                if ((exc >>= dpExc) &&
                    dpExc.Cause.getValueTypeClass() == uno::TypeClass_EXCEPTION) 
                {
                    // notify error cause only:
                    msg = reinterpret_cast< uno::Exception const * >( dpExc.Cause.getValue() )->Message;
                }
                if (msg.getLength() == 0) // fallback for debugging purposes
                    msg = ::comphelper::anyToString(exc);

                const ::vos::OGuard guard( Application::GetSolarMutex() );
                ::std::auto_ptr<ErrorBox> box( 
                    new ErrorBox( currentCmdEnv->activeDialog(), WB_OK, msg ) );
                if ( m_pDialogHelper )
                    box->SetText( m_pDialogHelper->getWindow()->GetText() );
                box->Execute();
                    //Continue with installation of the remaining extensions
            }
            {
                osl::MutexGuard aGuard( m_mutex );
                m_bWorking = false;
            }
        }

        {
            // when leaving the while loop with break, we should set working to false, too
			osl::MutexGuard aGuard( m_mutex );
            m_bWorking = false;
        }

		if ( !bStartProgress )
            currentCmdEnv->stopProgress();
    }
    //end for
    //enable all buttons
//     m_pDialog->m_bAddingExtensions = false;
//     m_pDialog->updateButtonStates();
#ifdef WNT
    CoUninitialize();
#endif
}

//------------------------------------------------------------------------------
void ExtensionCmdQueue::Thread::_addExtension( ::rtl::Reference< ProgressCmdEnv > &rCmdEnv,
                                               const uno::Reference< deployment::XPackageManager > &xPackageManager,
                                               const OUString &rPackageURL,
                                               const bool bWarnUser )
{
    //check if we have a string in anyTitle. For example "unopkg gui \" caused anyTitle to be void
    //and anyTitle.get<OUString> throws as RuntimeException.
    uno::Any anyTitle;
	try
	{
		anyTitle = ::ucbhelper::Content( rPackageURL, rCmdEnv.get() ).getPropertyValue( OUSTR("Title") );
	}
	catch ( uno::Exception & )
	{
		return;
	}

    OUString sName;
    if ( ! (anyTitle >>= sName) )
    {
        OSL_ENSURE(0, "Could not get file name for extension.");
        return;
    }

    rCmdEnv->setWarnUser( bWarnUser );
    uno::Reference< task::XAbortChannel > xAbortChannel( xPackageManager->createAbortChannel() );
    OUString sTitle = searchAndReplaceAll( m_sAddingPackages, OUSTR("%EXTENSION_NAME"), sName );
    rCmdEnv->progressSection( sTitle, xAbortChannel );

    try
    {
        uno::Reference< deployment::XPackage > xPackage( xPackageManager->addPackage(
                                                            rPackageURL, OUString() /* detect media-type */,
                                                            xAbortChannel, rCmdEnv.get() ) );
        OSL_ASSERT( xPackage.is() );
   }
    catch ( ucb::CommandFailedException & )
    {
        // When the extension is already installed we'll get a dialog asking if we want to overwrite. If we then press
        // cancel this exception is thrown.
    }
    catch ( ucb::CommandAbortedException & )
    {
        // User clicked the cancel button
        // TODO: handle cancel
    }
    rCmdEnv->setWarnUser( false );
}

//------------------------------------------------------------------------------
void ExtensionCmdQueue::Thread::_removeExtension( ::rtl::Reference< ProgressCmdEnv > &rCmdEnv,
                                                  const uno::Reference< deployment::XPackageManager > &xPackageManager,
                                                  const uno::Reference< deployment::XPackage > &xPackage )
{
    uno::Reference< task::XAbortChannel > xAbortChannel( xPackageManager->createAbortChannel() );
    OUString sTitle = searchAndReplaceAll( m_sRemovingPackages, OUSTR("%EXTENSION_NAME"), xPackage->getDisplayName() );
    rCmdEnv->progressSection( sTitle, xAbortChannel );

    OUString id( dp_misc::getIdentifier( xPackage ) );
    try
    {
        xPackageManager->removePackage( id, xPackage->getName(), xAbortChannel, rCmdEnv.get() );
    }
    catch ( ucb::CommandAbortedException & )
    {}

    // Check, if there are still updates to be notified via menu bar icon
    uno::Sequence< uno::Sequence< rtl::OUString > > aItemList;
    UpdateDialog::createNotifyJob( false, aItemList );
}

//------------------------------------------------------------------------------
void ExtensionCmdQueue::Thread::_checkForUpdates( const std::vector< TUpdateListEntry > &vExtensionList )
{
    UpdateDialog* pUpdateDialog;
    std::vector< UpdateData > vData;

    const ::vos::OGuard guard( Application::GetSolarMutex() );

    pUpdateDialog = new UpdateDialog( m_xContext, m_pDialogHelper? m_pDialogHelper->getWindow() : NULL, vExtensionList, &vData );
        
    pUpdateDialog->notifyMenubar( true, false ); // prepare the checking, if there updates to be notified via menu bar icon

    if ( ( pUpdateDialog->Execute() == RET_OK ) && !vData.empty() )
    {
        // If there is at least one directly downloadable dialog then we
        // open the install dialog.
        ::std::vector< UpdateData > dataDownload;
        int countWebsiteDownload = 0;
        typedef std::vector< dp_gui::UpdateData >::const_iterator cit;

        for ( cit i = vData.begin(); i < vData.end(); i++ )
        {
            if ( i->sWebsiteURL.getLength() > 0 )
                countWebsiteDownload ++;
            else
                dataDownload.push_back( *i );
        }

        short nDialogResult = RET_OK; 
        if ( !dataDownload.empty() )
        {
            nDialogResult = UpdateInstallDialog( m_pDialogHelper? m_pDialogHelper->getWindow() : NULL, dataDownload, m_xContext ).Execute();
            pUpdateDialog->notifyMenubar( false, true ); // Check, if there are still pending updates to be notified via menu bar icon
        }
        else
            pUpdateDialog->notifyMenubar( false, false ); // Check, if there are pending updates to be notified via menu bar icon

        //Now start the webbrowser and navigate to the websites where we get the updates
        if ( RET_OK == nDialogResult )
        {
            for ( cit i = vData.begin(); i < vData.end(); i++ )
            {
                if ( m_pDialogHelper && ( i->sWebsiteURL.getLength() > 0 ) )
                    m_pDialogHelper->openWebBrowser( i->sWebsiteURL, m_pDialogHelper->getWindow()->GetText() );
            }
        }   
    }
    else
        pUpdateDialog->notifyMenubar( false, false ); // check if there updates to be notified via menu bar icon

    delete pUpdateDialog;
}

//------------------------------------------------------------------------------
void ExtensionCmdQueue::Thread::_enableExtension( ::rtl::Reference< ProgressCmdEnv > &rCmdEnv,
                                                  const uno::Reference< deployment::XPackage > &xPackage )
{
    if ( !xPackage.is() )
        return;

    uno::Reference< task::XAbortChannel > xAbortChannel( xPackage->createAbortChannel() );
    OUString sTitle = searchAndReplaceAll( m_sEnablingPackages, OUSTR("%EXTENSION_NAME"), xPackage->getDisplayName() );
    rCmdEnv->progressSection( sTitle, xAbortChannel );

    try
    {
        xPackage->registerPackage( xAbortChannel, rCmdEnv.get() );
        if ( m_pDialogHelper )
            m_pDialogHelper->updatePackageInfo( xPackage );
    }
    catch ( ::ucb::CommandAbortedException & )
    {}
}

//------------------------------------------------------------------------------
void ExtensionCmdQueue::Thread::_disableExtension( ::rtl::Reference< ProgressCmdEnv > &rCmdEnv,
                                                   const uno::Reference< deployment::XPackage > &xPackage )
{
    if ( !xPackage.is() )
        return;

    uno::Reference< task::XAbortChannel > xAbortChannel( xPackage->createAbortChannel() );
    OUString sTitle = searchAndReplaceAll( m_sDisablingPackages, OUSTR("%EXTENSION_NAME"), xPackage->getDisplayName() );
    rCmdEnv->progressSection( sTitle, xAbortChannel );

    try
    {
        xPackage->revokePackage( xAbortChannel, rCmdEnv.get() );
        if ( m_pDialogHelper )
            m_pDialogHelper->updatePackageInfo( xPackage );
    }
    catch ( ::ucb::CommandAbortedException & )
    {}
}

//------------------------------------------------------------------------------
void ExtensionCmdQueue::Thread::onTerminated()
{
    ::osl::MutexGuard g(m_mutex); 
    m_bTerminated = true;
}

//------------------------------------------------------------------------------
OUString ExtensionCmdQueue::Thread::searchAndReplaceAll( const OUString &rSource,
                                                         const OUString &rWhat,
                                                         const OUString &rWith )
{
    OUString aRet( rSource );
    sal_Int32 nLen = rWhat.getLength();

    if ( !nLen )
        return aRet;

    sal_Int32 nIndex = rSource.indexOf( rWhat );
    while ( nIndex != -1 )
    {
        aRet = aRet.replaceAt( nIndex, nLen, rWith );
        nIndex = aRet.indexOf( rWhat, nIndex + rWith.getLength() );
    }
    return aRet;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ExtensionCmdQueue::ExtensionCmdQueue( DialogHelper * pDialogHelper,
                                      TheExtensionManager *pManager,
                                      const uno::Reference< uno::XComponentContext > &rContext )
  : m_thread( new Thread( pDialogHelper, pManager, rContext ) )
{
    m_thread->launch();
}

ExtensionCmdQueue::~ExtensionCmdQueue() {
    stop();
}

void ExtensionCmdQueue::addExtension( const uno::Reference< deployment::XPackageManager > &rPackageManager,
                                      const ::rtl::OUString & extensionURL,
                                      const bool bWarnUser )
{
    m_thread->addExtension( rPackageManager, extensionURL, bWarnUser );
}

void ExtensionCmdQueue::removeExtension( const uno::Reference< deployment::XPackageManager > &rPackageManager,
                                         const uno::Reference< deployment::XPackage > &rPackage )
{
    m_thread->removeExtension( rPackageManager, rPackage );
}

void ExtensionCmdQueue::enableExtension( const uno::Reference< deployment::XPackage > &rPackage,
                                         const bool bEnable )
{
    m_thread->enableExtension( rPackage, bEnable );
}

void ExtensionCmdQueue::checkForUpdates( const std::vector< TUpdateListEntry > &vExtensionList )
{
    m_thread->checkForUpdates( vExtensionList );
}

void ExtensionCmdQueue::stop() 
{
    m_thread->stop();
}

void ExtensionCmdQueue::stopAndWait() 
{
    m_thread->stop();
    m_thread->join();
}
 
bool ExtensionCmdQueue::hasTerminated()
{
    return m_thread->hasTerminated();
}

bool ExtensionCmdQueue::isBusy()
{
    return m_thread->isBusy();
}

void handleInteractionRequest( const uno::Reference< uno::XComponentContext > & xContext,
                               const uno::Reference< task::XInteractionRequest > & xRequest )
{
    ::rtl::Reference< ProgressCmdEnv > xCmdEnv( new ProgressCmdEnv( xContext, NULL, OUSTR("Extension Manager") ) );
    xCmdEnv->handle( xRequest );
}

} //namespace dp_gui

