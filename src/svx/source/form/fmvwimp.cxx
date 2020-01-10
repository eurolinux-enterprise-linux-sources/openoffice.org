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

#include "fmctrler.hxx"
#include "fmdocumentclassification.hxx"
#include "fmobj.hxx"
#include "fmpgeimp.hxx"
#include "fmprop.hrc"
#include "fmresids.hrc"
#include "fmservs.hxx"
#include "fmshimp.hxx"
#include "fmtools.hxx"
#include "fmundo.hxx"
#include "fmvwimp.hxx"
#include "formcontrolfactory.hxx"
#include "sdrpaintwindow.hxx"
#include "svditer.hxx"
#include "svx/dataaccessdescriptor.hxx"
#include "svx/dialmgr.hxx"
#include "svx/fmglob.hxx"
#include "svx/fmmodel.hxx"
#include "svx/fmpage.hxx"
#include "svx/fmshell.hxx"
#include "svx/fmview.hxx"
#include "svx/sdrpagewindow.hxx"
#include "svx/svdogrp.hxx"
#include "svx/svdpagv.hxx"
#include "xmlexchg.hxx"

/** === begin UNO includes === **/
#include <com/sun/star/ui/dialogs/XExecutableDialog.hpp>
#include <com/sun/star/style/VerticalAlignment.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/sdbc/XRowSet.hpp>
#include <com/sun/star/form/XLoadable.hpp>
#include <com/sun/star/awt/VisualEffect.hpp>
#include <com/sun/star/util/XNumberFormatsSupplier.hpp>
#include <com/sun/star/util/XNumberFormats.hpp>
#include <com/sun/star/sdb/CommandType.hpp>
#include <com/sun/star/sdbc/DataType.hpp>
#include <com/sun/star/sdbc/ColumnValue.hpp>
#include <com/sun/star/form/FormComponentType.hpp>
#include <com/sun/star/form/FormButtonType.hpp>
#include <com/sun/star/form/XReset.hpp>
#include <com/sun/star/form/binding/XBindableValue.hpp>
#include <com/sun/star/form/binding/XValueBinding.hpp>
#include <com/sun/star/form/submission/XSubmissionSupplier.hpp>
#include <com/sun/star/awt/XTabControllerModel.hpp>
#include <com/sun/star/awt/XControlContainer.hpp>
#include <com/sun/star/awt/XTabController.hpp>
#include <com/sun/star/container/XIndexAccess.hpp>
#include <com/sun/star/awt/XControl.hpp>
#include <com/sun/star/lang/XUnoTunnel.hpp>
#include <com/sun/star/sdbcx/XTablesSupplier.hpp>
#include <com/sun/star/sdbc/XPreparedStatement.hpp>
#include <com/sun/star/sdb/XQueriesSupplier.hpp>
/** === end UNO includes === **/

#include <comphelper/enumhelper.hxx>
#include <comphelper/extract.hxx>
#include <comphelper/numbers.hxx>
#include <comphelper/property.hxx>
#include <svtools/moduleoptions.hxx>
#include <tools/diagnose_ex.h>
#include <vcl/msgbox.hxx>
#include <vcl/stdtext.hxx>
#include <rtl/logfile.hxx>

#include <algorithm>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::sdbcx;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::sdb;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::form;
using namespace ::com::sun::star::awt;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::script;
using namespace ::com::sun::star::style;
using namespace ::com::sun::star::task;
using namespace ::com::sun::star::ui::dialogs;
using namespace ::comphelper;
using namespace ::svxform;
using namespace ::svx;
using com::sun::star::style::VerticalAlignment_MIDDLE;
using ::com::sun::star::form::binding::XValueBinding;
using ::com::sun::star::form::binding::XBindableValue;

namespace svxform
{
	//========================================================================
	class OAutoDispose
	{
	protected:
		Reference< XComponent >	m_xComp;

	public:
		OAutoDispose( const Reference< XInterface > _rxObject );
		~OAutoDispose();
	};

	//------------------------------------------------------------------------
	OAutoDispose::OAutoDispose( const Reference< XInterface > _rxObject )
		:m_xComp(_rxObject, UNO_QUERY)
	{
	}

	//------------------------------------------------------------------------
	OAutoDispose::~OAutoDispose()
	{
		if (m_xComp.is())
			m_xComp->dispose();
	}
}

//------------------------------------------------------------------------------
class FmXFormView::ObjectRemoveListener : public SfxListener
{
	FmXFormView* m_pParent;
public:
	ObjectRemoveListener( FmXFormView* pParent );
	virtual void Notify( SfxBroadcaster& rBC, const SfxHint& rHint );
};

//========================================================================
DBG_NAME(FmXPageViewWinRec)
//------------------------------------------------------------------------
FmXPageViewWinRec::FmXPageViewWinRec( const ::comphelper::ComponentContext& _rContext, const SdrPageWindow& _rWindow, FmXFormView* _pViewImpl )
:	m_xControlContainer( _rWindow.GetControlContainer() ),
	m_aContext( _rContext ),
	m_pViewImpl( _pViewImpl ),
    m_pWindow( dynamic_cast< Window* >( &_rWindow.GetPaintWindow().GetOutputDevice() ) )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXPageViewWinRec::FmXPageViewWinRec" );
	DBG_CTOR(FmXPageViewWinRec,NULL);

    // create an XFormController for every form
    FmFormPage* pFormPage = dynamic_cast< FmFormPage* >( _rWindow.GetPageView().GetPage() );
    DBG_ASSERT( pFormPage, "FmXPageViewWinRec::FmXPageViewWinRec: no FmFormPage found!" );
    if ( pFormPage )
	{
        try
        {
            Reference< XIndexAccess > xForms( pFormPage->GetForms(), UNO_QUERY_THROW );
		    sal_uInt32 nLength = xForms->getCount();
		    for (sal_uInt32 i = 0; i < nLength; i++)
		    {
                Reference< XForm > xForm( xForms->getByIndex(i), UNO_QUERY );
                if ( xForm.is() )
			        setController( xForm );
		    }
        }
        catch( const Exception& )
        {
        	DBG_UNHANDLED_EXCEPTION();
        }
	}
}
// -----------------------------------------------------------------------------
FmXPageViewWinRec::~FmXPageViewWinRec()
{
	DBG_DTOR(FmXPageViewWinRec,NULL);
}

//------------------------------------------------------------------
void FmXPageViewWinRec::dispose()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXPageViewWinRec::dispose" );
    for (   ::std::vector< Reference< XFormController > >::const_iterator i = m_aControllerList.begin();
            i != m_aControllerList.end();
            ++i
        )
    {
        try
        {
            Reference< XFormController > xController( *i, UNO_SET_THROW );

            // detaching the events
            Reference< XChild > xControllerModel( xController->getModel(), UNO_QUERY );
            if ( xControllerModel.is() )
            {
                Reference< XEventAttacherManager >  xEventManager( xControllerModel->getParent(), UNO_QUERY_THROW );
                Reference< XInterface > xControllerNormalized( xController, UNO_QUERY_THROW );
                xEventManager->detach( i - m_aControllerList.begin(), xControllerNormalized );
            }

            // dispose the formcontroller
            Reference< XComponent > xComp( xController, UNO_QUERY_THROW );
            xComp->dispose();
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION();
        }
    }

    m_aControllerList.clear();
}


//------------------------------------------------------------------------------
sal_Bool SAL_CALL FmXPageViewWinRec::hasElements(void) throw( RuntimeException )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXPageViewWinRec::hasElements" );
	return getCount() != 0;
}

//------------------------------------------------------------------------------
Type SAL_CALL  FmXPageViewWinRec::getElementType(void) throw( RuntimeException )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXPageViewWinRec::getElementType" );
	return ::getCppuType((const Reference< XFormController>*)0);
}

// XEnumerationAccess
//------------------------------------------------------------------------------
Reference< XEnumeration >  SAL_CALL FmXPageViewWinRec::createEnumeration(void) throw( RuntimeException )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXPageViewWinRec::createEnumeration" );
	return new ::comphelper::OEnumerationByIndex(this);
}

// XIndexAccess
//------------------------------------------------------------------------------
sal_Int32 SAL_CALL FmXPageViewWinRec::getCount(void) throw( RuntimeException )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXPageViewWinRec::getCount" );
	return m_aControllerList.size();
}

//------------------------------------------------------------------------------
Any SAL_CALL FmXPageViewWinRec::getByIndex(sal_Int32 nIndex) throw( IndexOutOfBoundsException, WrappedTargetException, RuntimeException )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXPageViewWinRec::getByIndex" );
	if (nIndex < 0 ||
		nIndex >= getCount())
		throw IndexOutOfBoundsException();

	Any aElement;
	aElement <<= m_aControllerList[nIndex];
	return aElement;
}

//------------------------------------------------------------------------
Reference< XFormController >  getControllerSearchChilds( const Reference< XIndexAccess > & xIndex, const Reference< XTabControllerModel > & xModel)
{
	if (xIndex.is() && xIndex->getCount())
	{
		Reference< XFormController >  xController;

		for (sal_Int32 n = xIndex->getCount(); n-- && !xController.is(); )
		{
			xIndex->getByIndex(n) >>= xController;
			if ((XTabControllerModel*)xModel.get() == (XTabControllerModel*)xController->getModel().get())
				return xController;
			else
			{
				xController = getControllerSearchChilds(Reference< XIndexAccess > (xController, UNO_QUERY), xModel);
				if ( xController.is() )
					return xController;
			}
		}
	}
	return Reference< XFormController > ();
}

// Search the according controller
//------------------------------------------------------------------------
Reference< XFormController >  FmXPageViewWinRec::getController( const Reference< XForm > & xForm ) const
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXPageViewWinRec::getController" );
	Reference< XTabControllerModel >  xModel(xForm, UNO_QUERY);
	for (::std::vector< Reference< XFormController > >::const_iterator i = m_aControllerList.begin();
		 i != m_aControllerList.end(); i++)
	{
		if ((XTabControllerModel*)(*i)->getModel().get() == (XTabControllerModel*)xModel.get())
			return *i;

		// the current-round controller isn't the right one. perhaps one of it's children ?
		Reference< XFormController >  xChildSearch = getControllerSearchChilds(Reference< XIndexAccess > (*i, UNO_QUERY), xModel);
		if (xChildSearch.is())
			return xChildSearch;
	}
	return Reference< XFormController > ();
}

//------------------------------------------------------------------------
void FmXPageViewWinRec::setController(const Reference< XForm > & xForm,  FmXFormController* _pParent )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXPageViewWinRec::setController" );
    DBG_ASSERT( xForm.is(), "FmXPageViewWinRec::setController: there should be a form!" );
	Reference< XIndexAccess >  xFormCps(xForm, UNO_QUERY);
	if (!xFormCps.is())
		return;

	Reference< XTabControllerModel >  xTabOrder(xForm, UNO_QUERY);

	// create a form controller
	FmXFormController* pController = new FmXFormController( m_aContext.getLegacyServiceFactory(), m_pViewImpl->getView(), m_pWindow );
	Reference< XFormController > xController( pController );

    Reference< XInteractionHandler > xHandler;
    if ( _pParent )
        xHandler = _pParent->getInteractionHandler();
    else
    {
        // TODO: should we create a default handler? Not really necessary, since the
        // FormController itself has a default fallback
    }
    if ( xHandler.is() )
    {
        Reference< XInitialization > xInitController( xController, UNO_QUERY );
        DBG_ASSERT( xInitController.is(), "FmXPageViewWinRec::setController: can't initialize the controller!" );
        if ( xInitController.is() )
        {
            Sequence< Any > aInitArgs( 1 );
            aInitArgs[ 0 ] <<= NamedValue( ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "InteractionHandler" ) ), makeAny( xHandler ) );
            xInitController->initialize( aInitArgs );
        }
    }

	pController->setModel(xTabOrder);
	pController->setContainer( m_xControlContainer );
	pController->activateTabOrder();
	pController->addActivateListener(m_pViewImpl);

	if ( _pParent )
		_pParent->addChild(pController);
	else
	{
		//	Reference< XFormController >  xController(pController);
		m_aControllerList.push_back(xController);

		pController->setParent(*this);

		// attaching the events
		Reference< XEventAttacherManager >  xEventManager(xForm->getParent(), UNO_QUERY);
		Reference< XInterface >  xIfc(xController, UNO_QUERY);
		xEventManager->attach(m_aControllerList.size() - 1, xIfc, makeAny(xController) );
	}



	// jetzt die Subforms durchgehen
	sal_uInt32 nLength = xFormCps->getCount();
	Reference< XForm >  xSubForm;
	for (sal_uInt32 i = 0; i < nLength; i++)
	{
		if ( xFormCps->getByIndex(i) >>= xSubForm )
			setController(xSubForm, pController);
	}
}

//------------------------------------------------------------------------
void FmXPageViewWinRec::updateTabOrder( const Reference< XForm >& _rxForm )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXPageViewWinRec::updateTabOrder" );
    OSL_PRECOND( _rxForm.is(), "FmXPageViewWinRec::updateTabOrder: illegal argument!" );
    if ( !_rxForm.is() )
        return;

    try
    {
        Reference< XTabController > xTabCtrl( getController( _rxForm ).get() );
        if ( xTabCtrl.is() )
        {   // if there already is a TabController for this form, then delegate the "updateTabOrder" request
            xTabCtrl->activateTabOrder();
        }
        else
        {   // otherwise, create a TabController

            // if it's a sub form, then we must ensure there exist TabControllers
            // for all its ancestors, too
            Reference< XForm > xParentForm( _rxForm->getParent(), UNO_QUERY );
            FmXFormController* pFormController = NULL;
            // there is a parent form -> look for the respective controller
            if ( xParentForm.is() )
                xTabCtrl = Reference< XTabController >( getController( xParentForm ), UNO_QUERY );

            if ( xTabCtrl.is() )
            {
                Reference< XUnoTunnel > xTunnel( xTabCtrl, UNO_QUERY_THROW );
                pFormController = reinterpret_cast< FmXFormController* >( xTunnel->getSomething( FmXFormController::getUnoTunnelImplementationId() ) );
            }

            setController( _rxForm, pFormController );
        }
    }
    catch( const Exception& )
    {
    	DBG_UNHANDLED_EXCEPTION();
    }
}

//------------------------------------------------------------------------
FmXFormView::FmXFormView(const ::comphelper::ComponentContext& _rContext, FmFormView* _pView )
    :m_aContext( _rContext )
	,m_pMarkedGrid(NULL)
	,m_pView(_pView)
	,m_nActivationEvent(0)
	,m_nErrorMessageEvent( 0 )
	,m_nAutoFocusEvent( 0 )
    ,m_nControlWizardEvent( 0 )
	,m_pWatchStoredList( NULL )
	,m_bFirstActivation( true )
    ,m_isTabOrderUpdateSuspended( false )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::FmXFormView" );
}

//------------------------------------------------------------------------
void FmXFormView::cancelEvents()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::cancelEvents" );
	if ( m_nActivationEvent )
	{
		Application::RemoveUserEvent( m_nActivationEvent );
		m_nActivationEvent = 0;
	}

	if ( m_nErrorMessageEvent )
	{
		Application::RemoveUserEvent( m_nErrorMessageEvent );
		m_nErrorMessageEvent = 0;
	}

	if ( m_nAutoFocusEvent )
	{
        Application::RemoveUserEvent( m_nAutoFocusEvent );
		m_nAutoFocusEvent = 0;
	}

    if ( m_nControlWizardEvent )
	{
        Application::RemoveUserEvent( m_nControlWizardEvent );
		m_nControlWizardEvent = 0;
	}
}

//------------------------------------------------------------------------
void FmXFormView::notifyViewDying( )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::notifyViewDying" );
	DBG_ASSERT( m_pView, "FmXFormView::notifyViewDying: my view already died!" );
	m_pView = NULL;
	cancelEvents();
}

//------------------------------------------------------------------------
FmXFormView::~FmXFormView()
{
    DBG_ASSERT(m_aWinList.size() == 0, "FmXFormView::~FmXFormView: Window list not empty!");

	cancelEvents();

	delete m_pWatchStoredList;
	m_pWatchStoredList = NULL;
}

//      EventListener
//------------------------------------------------------------------------------
void SAL_CALL FmXFormView::disposing(const EventObject& Source) throw( RuntimeException )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::disposing" );
	if ( m_xWindow.is() && Source.Source == m_xWindow )
		removeGridWindowListening();
}

// XFormControllerListener
//------------------------------------------------------------------------------
void SAL_CALL FmXFormView::formActivated(const EventObject& rEvent) throw( RuntimeException )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::formActivated" );
	if ( m_pView && m_pView->GetFormShell() && m_pView->GetFormShell()->GetImpl() )
		m_pView->GetFormShell()->GetImpl()->formActivated( rEvent );
}

//------------------------------------------------------------------------------
void SAL_CALL FmXFormView::formDeactivated(const EventObject& rEvent) throw( RuntimeException )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::formDeactivated" );
	if ( m_pView && m_pView->GetFormShell() && m_pView->GetFormShell()->GetImpl() )
		m_pView->GetFormShell()->GetImpl()->formDeactivated( rEvent );
}

// XContainerListener
//------------------------------------------------------------------------------
void SAL_CALL FmXFormView::elementInserted(const ContainerEvent& evt) throw( RuntimeException )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::elementInserted" );
    try
    {
	    Reference< XControlContainer > xControlContainer( evt.Source, UNO_QUERY_THROW );
		Reference< XControl > xControl( evt.Element, UNO_QUERY_THROW );
        Reference< XFormComponent > xControlModel( xControl->getModel(), UNO_QUERY_THROW );
        Reference< XForm > xForm( xControlModel->getParent(), UNO_QUERY_THROW );

        if ( m_isTabOrderUpdateSuspended )
        {
            // remember the container and the control, so we can update the tab order on resumeTabOrderUpdate
            m_aNeedTabOrderUpdate[ xControlContainer ].insert( xForm );
        }
        else
        {
            FmWinRecList::iterator pos = findWindow( xControlContainer );
		    if ( pos != m_aWinList.end() )
		    {
			    (*pos)->updateTabOrder( xForm );
		    }
        }
    }
    catch( const Exception& )
    {
    	DBG_UNHANDLED_EXCEPTION();
    }
}

//------------------------------------------------------------------------------
void SAL_CALL FmXFormView::elementReplaced(const ContainerEvent& evt) throw( RuntimeException )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::elementReplaced" );
	elementInserted(evt);
}

//------------------------------------------------------------------------------
void SAL_CALL FmXFormView::elementRemoved(const ContainerEvent& /*evt*/) throw( RuntimeException )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::elementRemoved" );
}

//------------------------------------------------------------------------------
FmWinRecList::const_iterator FmXFormView::findWindow( const Reference< XControlContainer >& _rxCC )  const
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::findWindow" );
	for (FmWinRecList::const_iterator i = m_aWinList.begin();
			i != m_aWinList.end(); i++)
	{
		if ( _rxCC == (*i)->getControlContainer() )
			return i;
	}
	return m_aWinList.end();
}

//------------------------------------------------------------------------------
FmWinRecList::iterator FmXFormView::findWindow( const Reference< XControlContainer >& _rxCC )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::findWindow" );
	for (FmWinRecList::iterator i = m_aWinList.begin();
			i != m_aWinList.end(); i++)
	{
		if ( _rxCC == (*i)->getControlContainer() )
			return i;
	}
	return m_aWinList.end();
}

//------------------------------------------------------------------------------
void FmXFormView::addWindow(const SdrPageWindow& rWindow)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::addWindow" );
    FmFormPage* pFormPage = PTR_CAST( FmFormPage, rWindow.GetPageView().GetPage() );
    if ( !pFormPage )
        return;

    Reference< XControlContainer > xCC = rWindow.GetControlContainer();
    if ( xCC.is() && findWindow( xCC ) == m_aWinList.end())
    {
        FmXPageViewWinRec *pFmRec = new FmXPageViewWinRec( m_aContext, rWindow, this );
        pFmRec->acquire();

        m_aWinList.push_back(pFmRec);

        // Am ControlContainer horchen um Aenderungen mitzbekommen
        Reference< XContainer >  xContainer( xCC, UNO_QUERY );
        if (xContainer.is())
            xContainer->addContainerListener(this);
	}
}

//------------------------------------------------------------------------------
void FmXFormView::removeWindow( const Reference< XControlContainer >& _rxCC )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::removeWindow" );
	// Wird gerufen, wenn
	// - in den Design-Modus geschaltet wird
	// - ein Window geloescht wird, waehrend man im Design-Modus ist
	// - der Control-Container fuer ein Window entfernt wird, waehrend
	//   der aktive Modus eingeschaltet ist.
	FmWinRecList::iterator i = findWindow( _rxCC );
	if (i != m_aWinList.end())
	{
		// Am ControlContainer horchen um Aenderungen mitzbekommen
		Reference< XContainer >  xContainer( _rxCC, UNO_QUERY );
		if (xContainer.is())
			xContainer->removeContainerListener(this);

		(*i)->dispose();
		(*i)->release();
		m_aWinList.erase(i);
	}
}

//------------------------------------------------------------------------------
void FmXFormView::displayAsyncErrorMessage( const SQLErrorEvent& _rEvent )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::displayAsyncErrorMessage" );
	DBG_ASSERT( 0 == m_nErrorMessageEvent, "FmXFormView::displayAsyncErrorMessage: not too fast, please!" );
		// This should not happen - usually, the PostUserEvent is faster than any possible user
		// interaction which could trigger a new error. If it happens, we need a queue for the events.
	m_aAsyncError = _rEvent;
	m_nErrorMessageEvent = Application::PostUserEvent( LINK( this, FmXFormView, OnDelayedErrorMessage ) );
}

//------------------------------------------------------------------------------
IMPL_LINK(FmXFormView, OnDelayedErrorMessage, void*, /*EMPTYTAG*/)
{
	m_nErrorMessageEvent = 0;
	displayException( m_aAsyncError );
	return 0L;
}

//------------------------------------------------------------------------------
void FmXFormView::onFirstViewActivation( const FmFormModel* _pDocModel )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::onFirstViewActivation" );
	if ( _pDocModel && _pDocModel->GetAutoControlFocus() )
		m_nAutoFocusEvent = Application::PostUserEvent( LINK( this, FmXFormView, OnAutoFocus ) );
}

//------------------------------------------------------------------------------
void FmXFormView::suspendTabOrderUpdate()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::suspendTabOrderUpdate" );
    OSL_ENSURE( !m_isTabOrderUpdateSuspended, "FmXFormView::suspendTabOrderUpdate: nesting not allowed!" );
    m_isTabOrderUpdateSuspended = true;
}

//------------------------------------------------------------------------------
void FmXFormView::resumeTabOrderUpdate()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::resumeTabOrderUpdate" );
    OSL_ENSURE( m_isTabOrderUpdateSuspended, "FmXFormView::resumeTabOrderUpdate: not suspended!" );
    m_isTabOrderUpdateSuspended = false;

    // update the tab orders for all components which were collected since the suspendTabOrderUpdate call.
    for (   MapControlContainerToSetOfForms::const_iterator container = m_aNeedTabOrderUpdate.begin();
            container != m_aNeedTabOrderUpdate.end();
            ++container
        )
    {
        FmWinRecList::iterator pos = findWindow( container->first );
        if ( pos == m_aWinList.end() )
            continue;

        for (   SetOfForms::const_iterator form = container->second.begin();
                form != container->second.end();
                ++form
            )
        {
	        (*pos)->updateTabOrder( *form );
        }
    }
    m_aNeedTabOrderUpdate.clear();
}

//------------------------------------------------------------------------------
IMPL_LINK(FmXFormView, OnActivate, void*, /*EMPTYTAG*/)
{
	m_nActivationEvent = 0;

	if ( !m_pView )
	{
		DBG_ERROR( "FmXFormView::OnActivate: well .... seems we have a timing problem (the view already died)!" );
		return 0;
	}

	// setting the controller to activate
	if (m_pView->GetFormShell() && m_pView->GetActualOutDev() && m_pView->GetActualOutDev()->GetOutDevType() == OUTDEV_WINDOW)
	{
		Window* pWindow = const_cast<Window*>(static_cast<const Window*>(m_pView->GetActualOutDev()));
		FmXPageViewWinRec* pFmRec = m_aWinList.size() ? m_aWinList[0] : NULL;
		for (FmWinRecList::const_iterator i = m_aWinList.begin();
			i != m_aWinList.end(); i++)
		{
			if (pWindow == (*i)->getWindow())
				pFmRec =*i;
		}

		if (pFmRec)
		{
			for (::std::vector< Reference< XFormController > >::const_iterator i = pFmRec->GetList().begin();
				i != pFmRec->GetList().end(); i++)
			{
				const Reference< XFormController > & xController = *i;
				if (xController.is())
				{
					// Nur bei Datenbankformularen erfolgt eine aktivierung
					Reference< XRowSet >  xForm(xController->getModel(), UNO_QUERY);
					if (xForm.is() && OStaticDataAccessTools().getRowSetConnection(xForm).is())
					{
						Reference< XPropertySet >  xFormSet(xForm, UNO_QUERY);
						if (xFormSet.is())
						{
							// wenn es eine Datenquelle gibt, dann als aktive ::com::sun::star::form setzen
							::rtl::OUString aSource = ::comphelper::getString(xFormSet->getPropertyValue(FM_PROP_COMMAND));
							if (aSource.getLength())
							{
								// benachrichtigung der Shell
								FmXFormShell* pShImpl =  m_pView->GetFormShell()->GetImpl();
								if (pShImpl)
									pShImpl->setActiveController(xController);
								break;
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

//------------------------------------------------------------------------------
void FmXFormView::Activate(sal_Bool bSync)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::Activate" );
	if (m_nActivationEvent)
	{
		Application::RemoveUserEvent(m_nActivationEvent);
		m_nActivationEvent = 0;
	}

	if (bSync)
	{
		LINK(this,FmXFormView,OnActivate).Call(NULL);
	}
	else
		m_nActivationEvent = Application::PostUserEvent(LINK(this,FmXFormView,OnActivate));
}

//------------------------------------------------------------------------------
void FmXFormView::Deactivate(BOOL bDeactivateController)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::Deactivate" );
	if (m_nActivationEvent)
	{
		Application::RemoveUserEvent(m_nActivationEvent);
		m_nActivationEvent = 0;
	}

	FmXFormShell* pShImpl =  m_pView->GetFormShell() ? m_pView->GetFormShell()->GetImpl() : NULL;
	if (pShImpl && bDeactivateController)
		pShImpl->setActiveController( NULL );
}

//------------------------------------------------------------------------------
FmFormShell* FmXFormView::GetFormShell() const
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::GetFormShell" );
	return m_pView ? m_pView->GetFormShell() : NULL;
}
// -----------------------------------------------------------------------------
void FmXFormView::AutoFocus( sal_Bool _bSync )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::AutoFocus" );
	if (m_nAutoFocusEvent)
        Application::RemoveUserEvent(m_nAutoFocusEvent);

	if ( _bSync )
		OnAutoFocus( NULL );
	else
		m_nAutoFocusEvent = Application::PostUserEvent(LINK(this, FmXFormView, OnAutoFocus));
}
// -----------------------------------------------------------------------------
static Reference< XControl > lcl_firstFocussableControl( const Sequence< Reference< XControl > >& _rControls )
{
	Reference< XControl > xReturn;

	// loop through all the controls
	const Reference< XControl >* pControls = _rControls.getConstArray();
	const Reference< XControl >* pControlsEnd = _rControls.getConstArray() + _rControls.getLength();
	for ( ; pControls != pControlsEnd; ++pControls )
	{
		try
		{
			if ( !pControls->is() )
                continue;

			Reference< XPropertySet > xModelProps( (*pControls)->getModel(), UNO_QUERY_THROW );

            // only enabled controls are allowed to participate
            sal_Bool bEnabled = sal_False;
            OSL_VERIFY( xModelProps->getPropertyValue( FM_PROP_ENABLED ) >>= bEnabled );
            if ( !bEnabled )
                continue;

            // check the class id of the control model
			sal_Int16 nClassId = FormComponentType::CONTROL;
			OSL_VERIFY( xModelProps->getPropertyValue( FM_PROP_CLASSID ) >>= nClassId );

			// controls which are not focussable
			if	(	( FormComponentType::CONTROL != nClassId )
				&&	( FormComponentType::IMAGEBUTTON != nClassId )
				&&	( FormComponentType::GROUPBOX != nClassId )
				&&	( FormComponentType::FIXEDTEXT != nClassId )
				&&	( FormComponentType::HIDDENCONTROL != nClassId )
				&&	( FormComponentType::IMAGECONTROL != nClassId )
				&&	( FormComponentType::SCROLLBAR != nClassId )
				&&	( FormComponentType::SPINBUTTON!= nClassId )
				)
			{
				xReturn = *pControls;
				break;
			}
		}
		catch( const Exception& e )
		{
			(void)e;	// make compiler happy
		}

		if ( !xReturn.is() && _rControls.getLength() )
			xReturn = _rControls[0];
	}

	return xReturn;
}

// -----------------------------------------------------------------------------
namespace
{
    // .........................................................................
    void lcl_ensureControlsOfFormExist_nothrow( const SdrPage& _rPage, const SdrView& _rView, const Window& _rWindow, const Reference< XForm >& _rxForm )
    {
        try
        {
            Reference< XInterface > xNormalizedForm( _rxForm, UNO_QUERY_THROW );

            SdrObjListIter aSdrObjectLoop( _rPage, IM_DEEPNOGROUPS );
            while ( aSdrObjectLoop.IsMore() )
            {
                FmFormObj* pFormObject = FmFormObj::GetFormObject( aSdrObjectLoop.Next() );
                if ( !pFormObject )
                    continue;

                Reference< XChild > xModel( pFormObject->GetUnoControlModel(), UNO_QUERY_THROW );
                Reference< XInterface > xModelParent( xModel->getParent(), UNO_QUERY_THROW );

                if ( xNormalizedForm.get() != xModelParent.get() )
                    continue;

                pFormObject->GetUnoControl( _rView, _rWindow );
            }
        }
        catch( const Exception& )
        {
        	DBG_UNHANDLED_EXCEPTION();
        }
    }
}

// -----------------------------------------------------------------------------
Reference< XFormController > FmXFormView::getFormController( const Reference< XForm >& _rxForm, const OutputDevice& _rDevice ) const
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::getFormController" );
    Reference< XFormController > xController;

    for ( FmWinRecList::const_iterator rec = m_aWinList.begin(); rec != m_aWinList.end(); ++rec )
    {
        const FmXPageViewWinRec* pViewWinRec( *rec );
        OSL_ENSURE( pViewWinRec, "FmXFormView::getFormController: invalid PageViewWinRec!" );
        if ( !pViewWinRec || ( pViewWinRec->getWindow() != &_rDevice ) )
            // wrong device
            continue;

        xController = pViewWinRec->getController( _rxForm );
        if ( xController.is() )
            break;
    }
    return xController;
}

// -----------------------------------------------------------------------------
IMPL_LINK(FmXFormView, OnAutoFocus, void*, /*EMPTYTAG*/)
{
	m_nAutoFocusEvent = 0;

	// go to the first form of our page, examine it's TabController, go to it's first (in terms of the tab order)
	// control, give it the focus

    do
    {

	// get the forms collection of the page we belong to
    FmFormPage* pPage = m_pView ? PTR_CAST( FmFormPage, m_pView->GetSdrPageView()->GetPage() ) : NULL;
    Reference< XIndexAccess > xForms( pPage ? Reference< XIndexAccess >( pPage->GetForms(), UNO_QUERY ) : Reference< XIndexAccess >() );

    const FmXPageViewWinRec* pViewWinRec = m_aWinList.size() ? m_aWinList[0] : NULL;
    const Window* pWindow = pViewWinRec ? pViewWinRec->getWindow() : NULL;

    OSL_ENSURE( xForms.is() && pWindow, "FmXFormView::OnAutoFocus: could not collect all essentials!" );
    if ( !xForms.is() || !pWindow )
        return 0L;

    try
	{
		// go for the tab controller of the first form
        if ( !xForms->getCount() )
            break;
        Reference< XForm > xForm( xForms->getByIndex( 0 ), UNO_QUERY_THROW );
		Reference< XTabController > xTabController( pViewWinRec->getController( xForm ), UNO_QUERY_THROW );

        // go for the first control of the controller
		Sequence< Reference< XControl > > aControls( xTabController->getControls() );
        if ( aControls.getLength() == 0 )
        {
            Reference< XElementAccess > xFormElementAccess( xForm, UNO_QUERY_THROW );
            if ( xFormElementAccess->hasElements() )
            {
                // there are control models in the form, but no controls, yet.
                // Well, since some time controls are created on demand only. In particular,
                // they're normally created when they're first painted.
                // Unfortunately, the FormController does not have any way to
                // trigger the creation itself, so we must hack this ...
                lcl_ensureControlsOfFormExist_nothrow( *pPage, *m_pView, *pWindow, xForm );
                aControls = xTabController->getControls();
                OSL_ENSURE( aControls.getLength(), "FmXFormView::OnAutoFocus: no controls at all!" );
            }
        }

		// set the focus to this first control
		Reference< XWindow > xControlWindow( lcl_firstFocussableControl( aControls ), UNO_QUERY );
		if ( !xControlWindow.is() )
            break;

        xControlWindow->setFocus();

		// ensure that the control is visible
		// 80210 - 12/07/00 - FS
        const Window* pCurrentWindow = dynamic_cast< const Window* >( m_pView->GetActualOutDev() );
        if ( pCurrentWindow )
		{
            awt::Rectangle aRect = xControlWindow->getPosSize();
			::Rectangle aNonUnoRect( aRect.X, aRect.Y, aRect.X + aRect.Width, aRect.Y + aRect.Height );
			m_pView->MakeVisible( pCurrentWindow->PixelToLogic( aNonUnoRect ), *const_cast< Window* >( pCurrentWindow ) );
		}
	}
    catch( const Exception& )
    {
    	DBG_UNHANDLED_EXCEPTION();
    }

    }   // do
    while ( false );

	return 1L;
}

// -----------------------------------------------------------------------------
namespace
{
}

// -----------------------------------------------------------------------------
void FmXFormView::onCreatedFormObject( FmFormObj& _rFormObject )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::onCreatedFormObject" );
    FmFormShell* pShell = m_pView ? m_pView->GetFormShell() : NULL;
    FmXFormShell* pShellImpl = pShell ? pShell->GetImpl() : NULL;
    OSL_ENSURE( pShellImpl, "FmXFormView::onCreatedFormObject: no form shell!" );
    if ( !pShellImpl )
        return;

    // it is valid that the form shell's forms collection is not initialized, yet
    pShellImpl->UpdateForms( sal_True );
    
    m_xLastCreatedControlModel.set( _rFormObject.GetUnoControlModel(), UNO_QUERY );
    if ( !m_xLastCreatedControlModel.is() )
        return;

    // some initial property defaults
    FormControlFactory aControlFactory( m_aContext );
    aControlFactory.initializeControlModel( pShellImpl->getDocumentType(), _rFormObject );

    if ( !pShellImpl->GetWizardUsing() )
        return;

    // #i31958# don't call wizards in XForms mode
    if ( pShellImpl->isEnhancedForm() )
        return;

    // #i46898# no wizards if there is no Base installed - currently, all wizards are
    // database related
    if ( !SvtModuleOptions().IsModuleInstalled( SvtModuleOptions::E_SDATABASE ) )
        return;

    if ( m_nControlWizardEvent )
        Application::RemoveUserEvent( m_nControlWizardEvent );
    m_nControlWizardEvent = Application::PostUserEvent( LINK( this, FmXFormView, OnStartControlWizard ) );
}

// -----------------------------------------------------------------------------
IMPL_LINK( FmXFormView, OnStartControlWizard, void*, /**/ )
{
    m_nControlWizardEvent = 0;
    OSL_PRECOND( m_xLastCreatedControlModel.is(), "FmXFormView::OnStartControlWizard: illegal call!" );
    if ( !m_xLastCreatedControlModel.is() )
        return 0L;

    sal_Int16 nClassId = FormComponentType::CONTROL;
    try
    {
        OSL_VERIFY( m_xLastCreatedControlModel->getPropertyValue( FM_PROP_CLASSID ) >>= nClassId );
    }
    catch( const Exception& )
    {
    	DBG_UNHANDLED_EXCEPTION();
    }

    const sal_Char* pWizardAsciiName = NULL;
    switch ( nClassId )
    {
        case FormComponentType::GRIDCONTROL:
            pWizardAsciiName = "com.sun.star.sdb.GridControlAutoPilot";
            break;
        case FormComponentType::LISTBOX:
        case FormComponentType::COMBOBOX:
            pWizardAsciiName = "com.sun.star.sdb.ListComboBoxAutoPilot";
            break;
        case FormComponentType::GROUPBOX:
            pWizardAsciiName = "com.sun.star.sdb.GroupBoxAutoPilot";
            break;
    }

    if ( pWizardAsciiName )
    {
        // build the argument list
        Sequence< Any > aWizardArgs(1);
        // the object affected
        aWizardArgs[0] = makeAny( PropertyValue(
            ::rtl::OUString::createFromAscii("ObjectModel"),
            0,
            makeAny( m_xLastCreatedControlModel ),
            PropertyState_DIRECT_VALUE
        ) );

        // create the wizard object
        Reference< XExecutableDialog > xWizard;
        try
        {
            m_aContext.createComponentWithArguments( pWizardAsciiName, aWizardArgs, xWizard );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION();
        }

        if ( !xWizard.is() )
        {
            ShowServiceNotAvailableError( NULL, String::CreateFromAscii( pWizardAsciiName ), sal_True );
        }
        else
        {
            // execute the wizard
            try
            {
                xWizard->execute();
            }
            catch( const Exception& )
            {
                DBG_UNHANDLED_EXCEPTION();
            }
        }
    }

    m_xLastCreatedControlModel.clear();
    return 1L;
}

// -----------------------------------------------------------------------------
namespace
{
    void lcl_insertIntoFormComponentHierarchy_throw( const FmFormView& _rView, const SdrUnoObj& _rSdrObj,
        const Reference< XDataSource >& _rxDataSource = NULL, const ::rtl::OUString& _rDataSourceName = ::rtl::OUString(),
        const ::rtl::OUString& _rCommand = ::rtl::OUString(), const sal_Int32 _nCommandType = -1 )
    {
		FmFormPage& rPage = static_cast< FmFormPage& >( *_rView.GetSdrPageView()->GetPage() );

	    Reference< XFormComponent > xFormComponent( _rSdrObj.GetUnoControlModel(), UNO_QUERY_THROW );
        Reference< XForm > xTargetForm(
            rPage.GetImpl().findPlaceInFormComponentHierarchy( xFormComponent, _rxDataSource, _rDataSourceName, _rCommand, _nCommandType ),
            UNO_SET_THROW );

        rPage.GetImpl().setUniqueName( xFormComponent, xTargetForm );

        Reference< XIndexContainer > xFormAsContainer( xTargetForm, UNO_QUERY_THROW );
	    xFormAsContainer->insertByIndex( xFormAsContainer->getCount(), makeAny( xFormComponent ) );
    }
}

// -----------------------------------------------------------------------------
SdrObject* FmXFormView::implCreateFieldControl( const ::svx::ODataAccessDescriptor& _rColumnDescriptor )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::implCreateFieldControl" );
	// not if we're in design mode
	if ( !m_pView->IsDesignMode() )
		return NULL;

	::rtl::OUString sCommand, sFieldName;
	sal_Int32 nCommandType = CommandType::COMMAND;
    SharedConnection xConnection;

	::rtl::OUString sDataSource = _rColumnDescriptor.getDataSource();
	_rColumnDescriptor[ daCommand ]		>>= sCommand;
	_rColumnDescriptor[ daColumnName ]	>>= sFieldName;
	_rColumnDescriptor[ daCommandType ]	>>= nCommandType;
    {
	    Reference< XConnection > xExternalConnection;
	    _rColumnDescriptor[ daConnection ]  >>= xExternalConnection;
        xConnection.reset( xExternalConnection, SharedConnection::NoTakeOwnership );
    }

	if  (   !sCommand.getLength()
        ||  !sFieldName.getLength()
        ||  (   !sDataSource.getLength()
            &&  !xConnection.is()
            )
        )
    {
        DBG_ERROR( "FmXFormView::implCreateFieldControl: nonsense!" );
    }

	Reference< XDataSource > xDataSource;
	SQLErrorEvent aError;
	try
	{
        if ( xConnection.is() && !xDataSource.is() && !sDataSource.getLength() )
        {
            Reference< XChild > xChild( xConnection, UNO_QUERY );
            if ( xChild.is() )
                xDataSource = xDataSource.query( xChild->getParent() );
        }

	    // obtain the data source
        if ( !xDataSource.is() )
		    xDataSource = OStaticDataAccessTools().getDataSource( sDataSource, m_aContext.getLegacyServiceFactory() );

        // and the connection, if necessary
		if ( !xConnection.is() )
			xConnection.reset( OStaticDataAccessTools().getConnection_withFeedback(
                sDataSource,
                ::rtl::OUString(),
                ::rtl::OUString(),
                m_aContext.getLegacyServiceFactory()
            ) );
	}
	catch(const SQLContext& e) { aError.Reason <<= e; }
	catch(const SQLWarning& e) { aError.Reason <<= e; }
	catch(const SQLException& e) { aError.Reason <<= e; }
    catch( const Exception& ) { /* will be asserted below */ }
	if (aError.Reason.hasValue())
	{
		displayAsyncErrorMessage( aError );
		return NULL;
	}

	// need a data source and a connection here
	if (!xDataSource.is() || !xConnection.is())
	{
		DBG_ERROR("FmXFormView::implCreateFieldControl : could not retrieve the data source or the connection!");
		return NULL;
	}

	OStaticDataAccessTools aDBATools;
	Reference< XComponent > xKeepFieldsAlive;
	// go
	try
	{
		// determine the table/query field which we should create a control for
		Reference< XPropertySet >	xField;

		Reference< XNameAccess >	xFields = aDBATools.getFieldsByCommandDescriptor(
			xConnection, nCommandType, sCommand, xKeepFieldsAlive );

		if (xFields.is() && xFields->hasByName(sFieldName))
			xFields->getByName(sFieldName) >>= xField;

		Reference< XNumberFormatsSupplier >  xSupplier = aDBATools.getNumberFormats(xConnection, sal_False);
		if (!xSupplier.is() || !xField.is())
			return NULL;

		Reference< XNumberFormats >  xNumberFormats(xSupplier->getNumberFormats());
		if (!xNumberFormats.is())
			return NULL;

		::rtl::OUString sLabelPostfix;

		////////////////////////////////////////////////////////////////
		// nur fuer Textgroesse
		OutputDevice* pOutDev = NULL;
		if (m_pView->GetActualOutDev() && m_pView->GetActualOutDev()->GetOutDevType() == OUTDEV_WINDOW)
			pOutDev = const_cast<OutputDevice*>(m_pView->GetActualOutDev());
		else
		{// OutDev suchen
			SdrPageView* pPageView = m_pView->GetSdrPageView();
			if( pPageView && !pOutDev )
			{
				// const SdrPageViewWinList& rWinList = pPageView->GetWinList();
				// const SdrPageViewWindows& rPageViewWindows = pPageView->GetPageViewWindows();

				for( sal_uInt32 i = 0L; i < pPageView->PageWindowCount(); i++ )
				{
					const SdrPageWindow& rPageWindow = *pPageView->GetPageWindow(i);

					if( rPageWindow.GetPaintWindow().OutputToWindow())
					{
						pOutDev = &rPageWindow.GetPaintWindow().GetOutputDevice();
						break;
					}
				}
			}
		}

		if ( !pOutDev )
			return NULL;

		sal_Int32 nDataType = ::comphelper::getINT32(xField->getPropertyValue(FM_PROP_FIELDTYPE));
		if ((DataType::BINARY == nDataType) || (DataType::VARBINARY == nDataType))
			return NULL;

		//////////////////////////////////////////////////////////////////////
		// determine the control type by examining the data type of the bound column
		sal_uInt16 nOBJID = 0;
		sal_Bool bDateNTimeField = sal_False;

		sal_Bool bIsCurrency = sal_False;
		if (::comphelper::hasProperty(FM_PROP_ISCURRENCY, xField))
			bIsCurrency = ::comphelper::getBOOL(xField->getPropertyValue(FM_PROP_ISCURRENCY));

		if (bIsCurrency)
			nOBJID = OBJ_FM_CURRENCYFIELD;
		else
			switch (nDataType)
			{
				case DataType::LONGVARBINARY:
					nOBJID = OBJ_FM_IMAGECONTROL;
					break;
				case DataType::LONGVARCHAR:
					nOBJID = OBJ_FM_EDIT;
					break;
				case DataType::BINARY:
				case DataType::VARBINARY:
					return NULL;
				case DataType::BIT:
				case DataType::BOOLEAN:
					nOBJID = OBJ_FM_CHECKBOX;
					break;
				case DataType::TINYINT:
				case DataType::SMALLINT:
				case DataType::INTEGER:
					nOBJID = OBJ_FM_NUMERICFIELD;
					break;
				case DataType::REAL:
				case DataType::DOUBLE:
				case DataType::NUMERIC:
				case DataType::DECIMAL:
					nOBJID = OBJ_FM_FORMATTEDFIELD;
					break;
				case DataType::TIMESTAMP:
					bDateNTimeField = sal_True;
					sLabelPostfix = String( SVX_RES( RID_STR_POSTFIX_DATE ) );
					// DON'T break !
				case DataType::DATE:
					nOBJID = OBJ_FM_DATEFIELD;
					break;
				case DataType::TIME:
					nOBJID = OBJ_FM_TIMEFIELD;
					break;
				case DataType::CHAR:
				case DataType::VARCHAR:
				default:
					nOBJID = OBJ_FM_EDIT;
					break;
			}
		if (!nOBJID)
			return NULL;

		SdrUnoObj* pLabel( NULL );
		SdrUnoObj* pControl( NULL );
		if  (   !createControlLabelPair( *pOutDev, 0, 0, xField, xNumberFormats, nOBJID, sLabelPostfix,
                    pLabel, pControl, xDataSource, sDataSource, sCommand, nCommandType )
            )
		{
			return NULL;
		}

		//////////////////////////////////////////////////////////////////////
		// group objects
		bool bCheckbox = ( OBJ_FM_CHECKBOX == nOBJID );
        OSL_ENSURE( !bCheckbox || !pLabel, "FmXFormView::implCreateFieldControl: why was there a label created for a check box?" );
        if ( bCheckbox )
            return pControl;

		SdrObjGroup* pGroup  = new SdrObjGroup();
		SdrObjList* pObjList = pGroup->GetSubList();
		pObjList->InsertObject( pLabel );
		pObjList->InsertObject( pControl );

		if ( bDateNTimeField )
		{	// so far we created a date field only, but we also need a time field
			pLabel = pControl = NULL;
			if  (   createControlLabelPair( *pOutDev, 0, 1000, xField, xNumberFormats, OBJ_FM_TIMEFIELD,
                        String( SVX_RES( RID_STR_POSTFIX_TIME ) ), pLabel, pControl,
                        xDataSource, sDataSource, sCommand, nCommandType )
                )
			{
				pObjList->InsertObject( pLabel );
				pObjList->InsertObject( pControl );
			}
		}

		return pGroup; // und fertig
	}
	catch(const Exception&)
	{
        DBG_UNHANDLED_EXCEPTION();
	}


	return NULL;
}

// -----------------------------------------------------------------------------
SdrObject* FmXFormView::implCreateXFormsControl( const ::svx::OXFormsDescriptor &_rDesc )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::implCreateXFormsControl" );
	// not if we're in design mode
	if ( !m_pView->IsDesignMode() )
		return NULL;

	Reference< XComponent > xKeepFieldsAlive;

	// go
	try
	{
		// determine the table/query field which we should create a control for
		Reference< XNumberFormats >	xNumberFormats;
		::rtl::OUString sLabelPostfix = _rDesc.szName;

		////////////////////////////////////////////////////////////////
		// nur fuer Textgroesse
		OutputDevice* pOutDev = NULL;
		if (m_pView->GetActualOutDev() && m_pView->GetActualOutDev()->GetOutDevType() == OUTDEV_WINDOW)
			pOutDev = const_cast<OutputDevice*>(m_pView->GetActualOutDev());
		else
		{// OutDev suchen
			SdrPageView* pPageView = m_pView->GetSdrPageView();
			if( pPageView && !pOutDev )
			{
				// const SdrPageViewWinList& rWinList = pPageView->GetWinList();
				// const SdrPageViewWindows& rPageViewWindows = pPageView->GetPageViewWindows();

				for( sal_uInt32 i = 0L; i < pPageView->PageWindowCount(); i++ )
				{
					const SdrPageWindow& rPageWindow = *pPageView->GetPageWindow(i);

					if( rPageWindow.GetPaintWindow().GetOutputDevice().GetOutDevType() == OUTDEV_WINDOW)
					{
						pOutDev = &rPageWindow.GetPaintWindow().GetOutputDevice();
						break;
					}
				}
			}
		}

		if ( !pOutDev )
			return NULL;

		//////////////////////////////////////////////////////////////////////
		// The service name decides which control should be created
		sal_uInt16 nOBJID = OBJ_FM_EDIT;
		if(::rtl::OUString(_rDesc.szServiceName).equals((::rtl::OUString)FM_SUN_COMPONENT_NUMERICFIELD))
			nOBJID = OBJ_FM_NUMERICFIELD;
		if(::rtl::OUString(_rDesc.szServiceName).equals((::rtl::OUString)FM_SUN_COMPONENT_CHECKBOX))
			nOBJID = OBJ_FM_CHECKBOX;
		if(::rtl::OUString(_rDesc.szServiceName).equals((::rtl::OUString)FM_COMPONENT_COMMANDBUTTON))
			nOBJID = OBJ_FM_BUTTON;

		typedef ::com::sun::star::form::submission::XSubmission XSubmission_t;
		Reference< XSubmission_t > xSubmission(_rDesc.xPropSet, UNO_QUERY);

		// xform control or submission button?
		if ( !xSubmission.is() )
        {
			SdrUnoObj* pLabel( NULL );
			SdrUnoObj* pControl( NULL );
			if  (   !createControlLabelPair( *pOutDev, 0, 0, NULL, xNumberFormats, nOBJID, sLabelPostfix,
                        pLabel, pControl )
                )
            {
				return NULL;
			}

			//////////////////////////////////////////////////////////////////////
			// Now build the connection between the control and the data item.
			Reference< XValueBinding > xValueBinding(_rDesc.xPropSet,UNO_QUERY);
			Reference< XBindableValue > xBindableValue(pControl->GetUnoControlModel(),UNO_QUERY);

            DBG_ASSERT( xBindableValue.is(), "FmXFormView::implCreateXFormsControl: control's not bindable!" );
            if ( xBindableValue.is() )
			    xBindableValue->setValueBinding(xValueBinding);

		    bool bCheckbox = ( OBJ_FM_CHECKBOX == nOBJID );
            OSL_ENSURE( !bCheckbox || !pLabel, "FmXFormView::implCreateXFormsControl: why was there a label created for a check box?" );
            if ( bCheckbox )
                return pControl;

			//////////////////////////////////////////////////////////////////////
			// group objects
			SdrObjGroup* pGroup  = new SdrObjGroup();
			SdrObjList* pObjList = pGroup->GetSubList();
			pObjList->InsertObject(pLabel);
			pObjList->InsertObject(pControl);

			return pGroup;
		}
		else {

			// create a button control
			const MapMode eTargetMode( pOutDev->GetMapMode() );
			const MapMode eSourceMode(MAP_100TH_MM);
			const sal_uInt16 nObjID = OBJ_FM_BUTTON;
			::Size controlSize(4000, 500);
			FmFormObj *pControl = static_cast<FmFormObj*>(SdrObjFactory::MakeNewObject( FmFormInventor, nObjID, NULL, NULL ));
			controlSize.Width() = Fraction(controlSize.Width(), 1) * eTargetMode.GetScaleX();
			controlSize.Height() = Fraction(controlSize.Height(), 1) * eTargetMode.GetScaleY();
			::Point controlPos( pOutDev->LogicToLogic( ::Point( controlSize.Width(), 0 ), eSourceMode, eTargetMode ) );
			::Rectangle controlRect( controlPos, pOutDev->LogicToLogic( controlSize, eSourceMode, eTargetMode ) );
			pControl->SetLogicRect(controlRect);

			// set the button label
			Reference< XPropertySet > xControlSet(pControl->GetUnoControlModel(), UNO_QUERY);
			xControlSet->setPropertyValue(FM_PROP_LABEL, makeAny(::rtl::OUString(_rDesc.szName)));

			// connect the submission with the submission supplier (aka the button)
            xControlSet->setPropertyValue( FM_PROP_BUTTON_TYPE,
                                           makeAny( FormButtonType_SUBMIT ) );
			typedef ::com::sun::star::form::submission::XSubmissionSupplier XSubmissionSupplier_t;
			Reference< XSubmissionSupplier_t > xSubmissionSupplier(pControl->GetUnoControlModel(), UNO_QUERY);
			xSubmissionSupplier->setSubmission(xSubmission);

			return pControl;
		}
	}
	catch(const Exception&)
	{
        DBG_ERROR("FmXFormView::implCreateXFormsControl: caught an exception while creating the control !");
	}


	return NULL;
}

//------------------------------------------------------------------------
bool FmXFormView::createControlLabelPair( OutputDevice& _rOutDev, sal_Int32 _nXOffsetMM, sal_Int32 _nYOffsetMM,
        const Reference< XPropertySet >& _rxField, const Reference< XNumberFormats >& _rxNumberFormats,
        sal_uInt16 _nControlObjectID, const ::rtl::OUString& _rFieldPostfix,
        SdrUnoObj*& _rpLabel, SdrUnoObj*& _rpControl,
        const Reference< XDataSource >& _rxDataSource, const ::rtl::OUString& _rDataSourceName,
        const ::rtl::OUString& _rCommand, const sal_Int32 _nCommandType )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::createControlLabelPair" );
    if  (   !createControlLabelPair( m_aContext, _rOutDev, _nXOffsetMM, _nYOffsetMM,
                _rxField, _rxNumberFormats, _nControlObjectID, _rFieldPostfix, FmFormInventor, OBJ_FM_FIXEDTEXT,
                NULL, NULL, NULL, _rpLabel, _rpControl )
        )
        return false;

    // insert the control model(s) into the form component hierachy
    if ( _rpLabel )
        lcl_insertIntoFormComponentHierarchy_throw( *m_pView, *_rpLabel, _rxDataSource, _rDataSourceName, _rCommand, _nCommandType );
    lcl_insertIntoFormComponentHierarchy_throw( *m_pView, *_rpControl, _rxDataSource, _rDataSourceName, _rCommand, _nCommandType );

    // some context-dependent initializations
    FormControlFactory aControlFactory( m_aContext );
    if ( _rpLabel )
        aControlFactory.initializeControlModel( impl_getDocumentType(), *_rpLabel );
    aControlFactory.initializeControlModel( impl_getDocumentType(), *_rpControl );

    return true;
}

//------------------------------------------------------------------------
bool FmXFormView::createControlLabelPair( const ::comphelper::ComponentContext& _rContext,
    OutputDevice& _rOutDev, sal_Int32 _nXOffsetMM, sal_Int32 _nYOffsetMM, const Reference< XPropertySet >& _rxField,
    const Reference< XNumberFormats >& _rxNumberFormats, sal_uInt16 _nControlObjectID,
    const ::rtl::OUString& _rFieldPostfix, UINT32 _nInventor, UINT16 _nLabelObjectID,
    SdrPage* _pLabelPage, SdrPage* _pControlPage, SdrModel* _pModel, SdrUnoObj*& _rpLabel, SdrUnoObj*& _rpControl)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::createControlLabelPair" );
	sal_Int32 nDataType = 0;
	::rtl::OUString sFieldName;
	Any aFieldName;
	if ( _rxField.is() )
	{
		nDataType = ::comphelper::getINT32(_rxField->getPropertyValue(FM_PROP_FIELDTYPE));
		aFieldName = Any(_rxField->getPropertyValue(FM_PROP_NAME));
		aFieldName >>= sFieldName;
	}

	// calculate the positions, respecting the settings of the target device
	::Size aTextSize( _rOutDev.GetTextWidth(sFieldName + _rFieldPostfix), _rOutDev.GetTextHeight() );

	MapMode   eTargetMode( _rOutDev.GetMapMode() ),
			  eSourceMode( MAP_100TH_MM );

	// Textbreite ist mindestens 4cm
	// Texthoehe immer halber cm
	::Size aDefTxtSize(4000, 500);
	::Size aDefSize(4000, 500);
	::Size aDefImageSize(4000, 4000);

	::Size aRealSize = _rOutDev.LogicToLogic(aTextSize, eTargetMode, eSourceMode);
	aRealSize.Width() = std::max(aRealSize.Width(), aDefTxtSize.Width());
	aRealSize.Height()= aDefSize.Height();

	// adjust to scaling of the target device (#53523#)
	aRealSize.Width() = long(Fraction(aRealSize.Width(), 1) * eTargetMode.GetScaleX());
	aRealSize.Height() = long(Fraction(aRealSize.Height(), 1) * eTargetMode.GetScaleY());

    // for boolean fields, we do not create a label, but just a checkbox
    bool bNeedLabel = ( _nControlObjectID != OBJ_FM_CHECKBOX );

	// the label
    ::std::auto_ptr< SdrUnoObj > pLabel;
    Reference< XPropertySet > xLabelModel;
    if ( bNeedLabel )
    {
        pLabel.reset( dynamic_cast< SdrUnoObj* >(
            SdrObjFactory::MakeNewObject( _nInventor, _nLabelObjectID, _pLabelPage, _pModel ) ) );
        OSL_ENSURE( pLabel.get(), "FmXFormView::createControlLabelPair: could not create the label!" );
        if ( !pLabel.get() )
            return false;

		xLabelModel.set( pLabel->GetUnoControlModel(), UNO_QUERY );
		if ( xLabelModel.is() )
        {
		    xLabelModel->setPropertyValue( FM_PROP_LABEL, makeAny( sFieldName + _rFieldPostfix ) );
            String sObjectLabel( SVX_RES( RID_STR_OBJECT_LABEL ) );
            sObjectLabel.SearchAndReplaceAllAscii( "#object#", sFieldName );
            xLabelModel->setPropertyValue( FM_PROP_NAME, makeAny( ::rtl::OUString( sObjectLabel ) ) );
        }

	    pLabel->SetLogicRect( ::Rectangle(
            _rOutDev.LogicToLogic( ::Point( _nXOffsetMM, _nYOffsetMM ), eSourceMode, eTargetMode ),
            _rOutDev.LogicToLogic( aRealSize, eSourceMode, eTargetMode )
        ) );
    }

	// the control
    ::std::auto_ptr< SdrUnoObj > pControl( dynamic_cast< SdrUnoObj* >(
	    SdrObjFactory::MakeNewObject( _nInventor, _nControlObjectID, _pControlPage, _pModel ) ) );
    OSL_ENSURE( pControl.get(), "FmXFormView::createControlLabelPair: could not create the control!" );
    if ( !pControl.get() )
        return false;

    Reference< XPropertySet > xControlSet( pControl->GetUnoControlModel(), UNO_QUERY );
    if ( !xControlSet.is() )
        return false;

	// size of the control
	::Size aControlSize( aDefSize );
    switch ( nDataType )
    {
    case DataType::BIT:
    case DataType::BOOLEAN:
		aControlSize = aDefSize;
        break;
    case DataType::LONGVARCHAR:
    case DataType::LONGVARBINARY:
		aControlSize = aDefImageSize;
        break;
    }

    if ( OBJ_FM_IMAGECONTROL == _nControlObjectID )
		aControlSize = aDefImageSize;

	aControlSize.Width() = long(Fraction(aControlSize.Width(), 1) * eTargetMode.GetScaleX());
	aControlSize.Height() = long(Fraction(aControlSize.Height(), 1) * eTargetMode.GetScaleY());

    pControl->SetLogicRect( ::Rectangle(
        _rOutDev.LogicToLogic( ::Point( aRealSize.Width() + _nXOffsetMM, _nYOffsetMM ), eSourceMode, eTargetMode ),
        _rOutDev.LogicToLogic( aControlSize, eSourceMode, eTargetMode )
    ) );

    // some initializations
    Reference< XPropertySetInfo > xControlPropInfo = xControlSet->getPropertySetInfo();

	if ( aFieldName.hasValue() )
	{
		xControlSet->setPropertyValue( FM_PROP_CONTROLSOURCE, aFieldName );
		xControlSet->setPropertyValue( FM_PROP_NAME, aFieldName );
        if ( !bNeedLabel )
        {
            // no dedicated label control => use the label property
            if ( xControlPropInfo->hasPropertyByName( FM_PROP_LABEL ) )
		        xControlSet->setPropertyValue( FM_PROP_LABEL, makeAny( sFieldName + _rFieldPostfix ) );
            else
                OSL_ENSURE( false, "FmXFormView::createControlLabelPair: can't set a label for the control!" );
        }
	}

	if ( nDataType == DataType::LONGVARCHAR && xControlPropInfo->hasPropertyByName( FM_PROP_MULTILINE ) )
	{
		xControlSet->setPropertyValue( FM_PROP_MULTILINE, makeAny( sal_Bool( sal_True ) ) );
	}

    // announce the label to the control
    if ( xControlPropInfo->hasPropertyByName( FM_PROP_CONTROLLABEL ) && xLabelModel.is() )
    {
	    try
	    {
		    xControlSet->setPropertyValue( FM_PROP_CONTROLLABEL, makeAny( xLabelModel ) );
	    }
	    catch( const Exception& )
	    {
            DBG_UNHANDLED_EXCEPTION();
	    }
    }

	if ( _rxField.is() )
    {
    	FormControlFactory aControlFactory( _rContext );
	    aControlFactory.initializeFieldDependentProperties( _rxField, xControlSet, _rxNumberFormats );
	}

    _rpLabel = pLabel.release();
    _rpControl = pControl.release();
    return true;
}

//------------------------------------------------------------------------------
FmXFormView::ObjectRemoveListener::ObjectRemoveListener( FmXFormView* pParent )
	:m_pParent( pParent )
{
}

//------------------------------------------------------------------------------
void FmXFormView::ObjectRemoveListener::Notify( SfxBroadcaster& /*rBC*/, const SfxHint& rHint )
{
	if (rHint.ISA(SdrHint) && (((SdrHint&)rHint).GetKind() == HINT_OBJREMOVED))
		m_pParent->ObjectRemovedInAliveMode(((SdrHint&)rHint).GetObject());
}

//------------------------------------------------------------------------------
void FmXFormView::ObjectRemovedInAliveMode( const SdrObject* pObject )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::ObjectRemovedInAliveMode" );
	// wenn das entfernte Objekt in meiner MarkList, die ich mir beim Umschalten in den Alive-Mode gemerkt habe, steht,
	// muss ich es jetzt da rausnehmen, da ich sonst beim Zurueckschalten versuche, die Markierung wieder zu setzen
	// (interesanterweise geht das nur bei gruppierten Objekten schief (beim Zugriff auf deren ObjList GPF), nicht bei einzelnen)

	ULONG nCount = m_aMark.GetMarkCount();
	for (ULONG i = 0; i < nCount; ++i)
	{
		SdrMark* pMark = m_aMark.GetMark(i);
		SdrObject* pCurrent = pMark->GetMarkedSdrObj();
		if (pObject == pCurrent)
		{
			m_aMark.DeleteMark(i);
			return;
		}
		// ich brauche nicht in GroupObjects absteigen : wenn dort unten ein Objekt geloescht wird, dann bleibt der
		// Zeiger auf das GroupObject, den ich habe, trotzdem weiter gueltig bleibt ...
	}
}

//------------------------------------------------------------------------------
void FmXFormView::stopMarkListWatching()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::stopMarkListWatching" );
	if ( m_pWatchStoredList )
	{
		m_pWatchStoredList->EndListeningAll();
		delete m_pWatchStoredList;
		m_pWatchStoredList = NULL;
	}
}

//------------------------------------------------------------------------------
void FmXFormView::startMarkListWatching()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::startMarkListWatching" );
	if ( !m_pWatchStoredList )
	{
		m_pWatchStoredList = new ObjectRemoveListener( this );
		FmFormModel* pModel = GetFormShell() ? GetFormShell()->GetFormModel() : NULL;
		DBG_ASSERT( pModel != NULL, "FmXFormView::startMarkListWatching: shell has no model!" );
		m_pWatchStoredList->StartListening( *static_cast< SfxBroadcaster* >( pModel ) );
	}
	else
	{
		DBG_ERROR( "FmXFormView::startMarkListWatching: already listening!" );
	}
}

//------------------------------------------------------------------------------
void FmXFormView::saveMarkList( sal_Bool _bSmartUnmark )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::saveMarkList" );
	if ( m_pView )
	{
		m_aMark = m_pView->GetMarkedObjectList();
		if ( _bSmartUnmark )
		{
			ULONG nCount = m_aMark.GetMarkCount( );
			for ( ULONG i = 0; i < nCount; ++i )
			{
				SdrMark*   pMark = m_aMark.GetMark(i);
				SdrObject* pObj  = pMark->GetMarkedSdrObj();

				if ( m_pView->IsObjMarked( pObj ) )
				{
					if ( pObj->IsGroupObject() )
					{
						SdrObjListIter aIter( *pObj->GetSubList() );
						sal_Bool bMixed = sal_False;
						while ( aIter.IsMore() && !bMixed )
							bMixed = ( aIter.Next()->GetObjInventor() != FmFormInventor );

						if ( !bMixed )
						{
							// all objects in the group are form objects
							m_pView->MarkObj( pMark->GetMarkedSdrObj(), pMark->GetPageView(), sal_True /* unmark! */ );
						}
					}
					else
					{
						if ( pObj->GetObjInventor() == FmFormInventor )
						{	// this is a form layer object
							m_pView->MarkObj( pMark->GetMarkedSdrObj(), pMark->GetPageView(), sal_True /* unmark! */ );
						}
					}
				}
			}
		}
	}
	else
	{
		DBG_ERROR( "FmXFormView::saveMarkList: invalid view!" );
		m_aMark = SdrMarkList();
	}
}

//--------------------------------------------------------------------------
static sal_Bool lcl_hasObject( SdrObjListIter& rIter, SdrObject* pObj )
{
	sal_Bool bFound = sal_False;
	while (rIter.IsMore() && !bFound)
		bFound = pObj == rIter.Next();

	rIter.Reset();
	return bFound;
}

//------------------------------------------------------------------------------
void FmXFormView::restoreMarkList( SdrMarkList& _rRestoredMarkList )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::restoreMarkList" );
	if ( !m_pView )
		return;

	_rRestoredMarkList.Clear();

	const SdrMarkList& rCurrentList = m_pView->GetMarkedObjectList();
	FmFormPage* pPage = GetFormShell() ? GetFormShell()->GetCurPage() : NULL;
	if (pPage)
	{
		if (rCurrentList.GetMarkCount())
		{	// there is a current mark ... hmm. Is it a subset of the mark we remembered in saveMarkList?
			sal_Bool bMisMatch = sal_False;

			// loop through all current marks
			ULONG nCurrentCount = rCurrentList.GetMarkCount();
			for ( ULONG i=0; i<nCurrentCount&& !bMisMatch; ++i )
			{
				const SdrObject* pCurrentMarked = rCurrentList.GetMark( i )->GetMarkedSdrObj();

				// loop through all saved marks, check for equality
				sal_Bool bFound = sal_False;
				ULONG nSavedCount = m_aMark.GetMarkCount();
				for ( ULONG j=0; j<nSavedCount && !bFound; ++j )
				{
					if ( m_aMark.GetMark( j )->GetMarkedSdrObj() == pCurrentMarked )
						bFound = sal_True;
				}

				// did not find a current mark in the saved marks
				if ( !bFound )
					bMisMatch = sal_True;
			}

			if ( bMisMatch )
			{
				m_aMark.Clear();
				_rRestoredMarkList = rCurrentList;
				return;
			}
		}
		// wichtig ist das auf die Objecte der markliste nicht zugegriffen wird
		// da diese bereits zerstoert sein koennen
		SdrPageView* pCurPageView = m_pView->GetSdrPageView();
		SdrObjListIter aPageIter( *pPage );
		sal_Bool bFound = sal_True;

		// gibt es noch alle Objecte
		ULONG nCount = m_aMark.GetMarkCount();
		for (ULONG i = 0; i < nCount && bFound; i++)
		{
			SdrMark*   pMark = m_aMark.GetMark(i);
			SdrObject* pObj  = pMark->GetMarkedSdrObj();
			if (pObj->IsGroupObject())
			{
				SdrObjListIter aIter(*pObj->GetSubList());
				while (aIter.IsMore() && bFound)
					bFound = lcl_hasObject(aPageIter, aIter.Next());
			}
			else
				bFound = lcl_hasObject(aPageIter, pObj);

			bFound = bFound && pCurPageView == pMark->GetPageView();
		}

		if (bFound)
		{
			// Das LastObject auswerten
			if (nCount) // Objecte jetzt Markieren
			{
				for (ULONG i = 0; i < nCount; i++)
				{
					SdrMark* pMark = m_aMark.GetMark(i);
					SdrObject* pObj = pMark->GetMarkedSdrObj();
					if ( pObj->GetObjInventor() == FmFormInventor )
						if ( !m_pView->IsObjMarked( pObj ) )
							m_pView->MarkObj( pObj, pMark->GetPageView() );
				}

				_rRestoredMarkList = m_aMark;
			}
		}
		m_aMark.Clear();
	}
}
// -----------------------------------------------------------------------------
void SAL_CALL FmXFormView::focusGained( const FocusEvent& /*e*/ ) throw (RuntimeException)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::focusGained" );
	if ( m_xWindow.is() && m_pView )
	{
        m_pView->SetMoveOutside( TRUE, FmFormView::ImplAccess() );
	}
}
// -----------------------------------------------------------------------------
void SAL_CALL FmXFormView::focusLost( const FocusEvent& /*e*/ ) throw (RuntimeException)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::focusLost" );
	// when switch the focus outside the office the mark didn't change
	// so we can not remove us as focus listener
	if ( m_xWindow.is() && m_pView )
	{
		m_pView->SetMoveOutside( FALSE, FmFormView::ImplAccess() );
	}
}
// -----------------------------------------------------------------------------
void FmXFormView::removeGridWindowListening()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::removeGridWindowListening" );
	if ( m_xWindow.is() )
	{
		m_xWindow->removeFocusListener(this);
		if ( m_pView )
		{
			m_pView->SetMoveOutside( FALSE, FmFormView::ImplAccess() );
		}
		m_xWindow = NULL;
	}
}

// -----------------------------------------------------------------------------
DocumentType FmXFormView::impl_getDocumentType() const
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "svx", "Ocke.Janssen@sun.com", "FmXFormView::impl_getDocumentType" );
    if ( GetFormShell() && GetFormShell()->GetImpl() )
        return GetFormShell()->GetImpl()->getDocumentType();
    return eUnknownDocumentType;
}
