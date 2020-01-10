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
#include "precompiled_chart2.hxx"
#include "ChartModel.hxx"
#include "ImplChartModel.hxx"
#include "servicenames.hxx"
#include "MediaDescriptorHelper.hxx"
#include "macros.hxx"
#include "InternalData.hxx"
#include "servicenames.hxx"
#include "DataSourceHelper.hxx"
#include "NoWarningThisInCTOR.hxx"
#include "ChartModelHelper.hxx"
#include "DisposeHelper.hxx"
#include "ControllerLockGuard.hxx"
#include "ObjectIdentifier.hxx"
#include "ChartModelHelper.hxx"

#include <comphelper/InlineContainer.hxx>
#include <comphelper/processfactory.hxx>

// header for class SvNumberFormatsSupplierObj
#include <svtools/numuno.hxx>
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/view/XSelectionSupplier.hpp>
#include <com/sun/star/embed/XEmbedObjectCreator.hpp>
#include <com/sun/star/embed/XEmbedPersist.hpp>
#include <com/sun/star/embed/EmbedStates.hpp>
#include <com/sun/star/embed/XComponentSupplier.hpp>
#include <com/sun/star/embed/XStorage.hpp>
#include <com/sun/star/embed/EmbedMapUnits.hpp>
#include <com/sun/star/embed/Aspects.hpp>
#include <com/sun/star/awt/XWindow.hpp>
#include <com/sun/star/awt/PosSize.hpp>
#include <com/sun/star/datatransfer/XTransferable.hpp>

#include <map>
#include <algorithm>

using ::com::sun::star::uno::Sequence;
using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Any;
using ::rtl::OUString;
using ::osl::MutexGuard;

using namespace ::com::sun::star;
using namespace ::apphelper;

namespace
{
const OUString lcl_aGDIMetaFileMIMEType(
    RTL_CONSTASCII_USTRINGPARAM("application/x-openoffice-gdimetafile;windows_formatname=\"GDIMetaFile\""));
const OUString lcl_aGDIMetaFileMIMETypeHighContrast(
    RTL_CONSTASCII_USTRINGPARAM("application/x-openoffice-highcontrast-gdimetafile;windows_formatname=\"GDIMetaFile\""));

} // anonymous namespace

//-----------------------------------------------------------------
// ChartModel Constructor and Destructor
//-----------------------------------------------------------------

namespace chart
{

ChartModel::ChartModel(uno::Reference<uno::XComponentContext > const & xContext)
	: m_aLifeTimeManager( this, this )
	, m_bReadOnly( sal_False )
	, m_bModified( sal_False )
    , m_nInLoad(0)
    , m_bUpdateNotificationsPending(false)
    , m_aControllers( m_aModelMutex )
	, m_nControllerLockCount(0)
    , m_xContext( xContext )
    // default visual area is 8 x 7 cm
    , m_aVisualAreaSize( 8000, 7000 )
{
    OSL_TRACE( "ChartModel: CTOR called" );

    // attention: passing this as reference to ImplChartModel
    m_pImplChartModel.reset( new impl::ImplChartModel( xContext, this ));
}

ChartModel::ChartModel( const ChartModel & rOther )
	: impl::ChartModel_Base()
    , m_aLifeTimeManager( this, this )
	, m_bReadOnly( rOther.m_bReadOnly )
	, m_bModified( rOther.m_bModified )
    , m_nInLoad(0)
    , m_bUpdateNotificationsPending(false)
	, m_aResource( rOther.m_aResource )
    , m_aMediaDescriptor( rOther.m_aMediaDescriptor )
	, m_aControllers( m_aModelMutex )
	, m_nControllerLockCount(0)
    , m_xContext( rOther.m_xContext )
    // @note: the old model aggregate must not be shared with other models if it
    // is, you get mutex deadlocks
    , m_xOldModelAgg( 0 ) //rOther.m_xOldModelAgg )
    , m_xStorage( 0 ) //rOther.m_xStorage )
    , m_aVisualAreaSize( rOther.m_aVisualAreaSize )
    , m_aGraphicObjectVector( rOther.m_aGraphicObjectVector )
{
    OSL_TRACE( "ChartModel: Copy-CTOR called" );

    // attention: passing this as reference to ImplChartModel
    if( rOther.m_pImplChartModel.get())
        m_pImplChartModel.reset( new impl::ImplChartModel( * rOther.m_pImplChartModel.get(), this ));
    else
        m_pImplChartModel.reset( new impl::ImplChartModel( m_xContext, this ));
}

ChartModel::~ChartModel()
{
    OSL_TRACE( "ChartModel: DTOR called" );
    if( m_xOldModelAgg.is())
        m_xOldModelAgg->setDelegator( 0 );
}


//-----------------------------------------------------------------
// private methods
//-----------------------------------------------------------------

		::rtl::OUString ChartModel
::impl_g_getLocation()
{

	LifeTimeGuard aGuard(m_aLifeTimeManager);
	if(!aGuard.startApiCall())
		return ::rtl::OUString(); //behave passive if already disposed or closed or throw exception @todo?
	//mutex is acquired
	return m_aResource;
}

		sal_Bool ChartModel
::impl_isControllerConnected( const uno::Reference< frame::XController >& xController )
{
	try
	{
		uno::Sequence< uno::Reference<uno::XInterface> > aSeq = m_aControllers.getElements();
		for( sal_Int32 nN = aSeq.getLength(); nN--; )
		{
			if( aSeq[nN] == xController )
				return sal_True;
		}
	}
	catch( uno::Exception )
	{
	}
	return sal_False;
}

			uno::Reference< frame::XController > ChartModel
::impl_getCurrentController() throw(uno::RuntimeException)
{
		//@todo? hold only weak references to controllers

	// get the last active controller of this model
	if( m_xCurrentController.is() )
		return m_xCurrentController;

	// get the first controller of this model
	if( m_aControllers.getLength() )
	{
		uno::Reference<uno::XInterface> xI = m_aControllers.getElements()[0];
		return uno::Reference<frame::XController>( xI, uno::UNO_QUERY );
	}

	//return nothing if no controllers are connected at all
	return uno::Reference< frame::XController > ();
}

		void SAL_CALL ChartModel
::impl_notifyCloseListeners()
		throw( uno::RuntimeException)
{
	::cppu::OInterfaceContainerHelper* pIC = m_aLifeTimeManager.m_aListenerContainer
		.getContainer( ::getCppuType((const uno::Reference< util::XCloseListener >*)0) );
	if( pIC )
	{
		lang::EventObject aEvent( static_cast< lang::XComponent*>(this) );
		::cppu::OInterfaceIteratorHelper aIt( *pIC );
		while( aIt.hasMoreElements() )
			(static_cast< util::XCloseListener*>(aIt.next()))->notifyClosing( aEvent );
	}
}

//-----------------------------------------------------------------
// lang::XServiceInfo
//-----------------------------------------------------------------

APPHELPER_XSERVICEINFO_IMPL(ChartModel,CHART_MODEL_SERVICE_IMPLEMENTATION_NAME)

	uno::Sequence< rtl::OUString > ChartModel
::getSupportedServiceNames_Static()
{
	uno::Sequence< rtl::OUString > aSNS( 3 );
	aSNS[0] = CHART_MODEL_SERVICE_NAME;
	aSNS[1] = C2U( "com.sun.star.document.OfficeDocument" );
    aSNS[2] = C2U( "com.sun.star.chart.ChartDocument" );
	//// @todo : add additional services if you support any further
	return aSNS;
}

//-----------------------------------------------------------------
// frame::XModel (required interface)
//-----------------------------------------------------------------

		sal_Bool SAL_CALL ChartModel
::attachResource( const ::rtl::OUString& rURL
		, const uno::Sequence< beans::PropertyValue >& rMediaDescriptor )
		throw(uno::RuntimeException)
{
    /*
    The method attachResource() is used by the frame loader implementations
    to inform the model about its URL and MediaDescriptor.
    */

	LifeTimeGuard aGuard(m_aLifeTimeManager);
	if(!aGuard.startApiCall())
		return sal_False; //behave passive if already disposed or closed or throw exception @todo?
	//mutex is acquired

	if(m_aResource.getLength()!=0)//we have a resource already //@todo? or is setting a new resource allowed?
		return sal_False;
	m_aResource = rURL;
	m_aMediaDescriptor = rMediaDescriptor;

	//@todo ? check rURL ??
	//@todo ? evaluate m_aMediaDescriptor;
	//@todo ? ... ??? --> nothing, this method is only for setting informations

	return sal_True;
}

		::rtl::OUString SAL_CALL ChartModel
::getURL() throw(uno::RuntimeException)
{
	return impl_g_getLocation();
}

		uno::Sequence< beans::PropertyValue > SAL_CALL ChartModel
::getArgs() throw(uno::RuntimeException)
{
    /*
    The method getArgs() returns a sequence of property values
    that report the resource description according to com.sun.star.document.MediaDescriptor,
    specified on loading or saving with storeAsURL.
    */

	LifeTimeGuard aGuard(m_aLifeTimeManager);
	if(!aGuard.startApiCall())
		return uno::Sequence< beans::PropertyValue >(); //behave passive if already disposed or closed or throw exception @todo?
	//mutex is acquired

	return m_aMediaDescriptor;
}

		void SAL_CALL ChartModel
::connectController( const uno::Reference< frame::XController >& xController )
		throw(uno::RuntimeException)
{
	//@todo? this method is declared as oneway -> ...?

	LifeTimeGuard aGuard(m_aLifeTimeManager);
	if(!aGuard.startApiCall())
		return ; //behave passive if already disposed or closed
	//mutex is acquired

	//--add controller
	m_aControllers.addInterface(xController);
}

		void SAL_CALL ChartModel
::disconnectController( const uno::Reference< frame::XController >& xController )
		throw(uno::RuntimeException)
{
	//@todo? this method is declared as oneway -> ...?

	LifeTimeGuard aGuard(m_aLifeTimeManager);
	if(!aGuard.startApiCall())
		return; //behave passive if already disposed or closed

	//--remove controller
	m_aControllers.removeInterface(xController);

	//case: current controller is disconnected:
	if( m_xCurrentController == xController )
		m_xCurrentController.clear();
    
    DisposeHelper::DisposeAndClear( m_xRangeHighlighter );
}

		void SAL_CALL ChartModel
::lockControllers()	throw(uno::RuntimeException)
{
    /*
    suspends some notifications to the controllers which are used for display updates.

    The calls to lockControllers() and unlockControllers() may be nested
    and even overlapping, but they must be in pairs. While there is at least one lock
    remaining, some notifications for display updates are not broadcasted.
    */

	//@todo? this method is declared as oneway -> ...?

	LifeTimeGuard aGuard(m_aLifeTimeManager);
	if(!aGuard.startApiCall())
		return; //behave passive if already disposed or closed or throw exception @todo?
	++m_nControllerLockCount;
}

		void SAL_CALL ChartModel
::unlockControllers() throw(uno::RuntimeException)
{
    /*
    resumes the notifications which were suspended by lockControllers() .

    The calls to lockControllers() and unlockControllers() may be nested
    and even overlapping, but they must be in pairs. While there is at least one lock
    remaining, some notifications for display updates are not broadcasted.
    */

	//@todo? this method is declared as oneway -> ...?

	LifeTimeGuard aGuard(m_aLifeTimeManager);
	if(!aGuard.startApiCall())
		return; //behave passive if already disposed or closed or throw exception @todo?
    if( m_nControllerLockCount == 0 )
    {
        OSL_TRACE( "ChartModel: unlockControllers called with m_nControllerLockCount == 0" );
        return;
    }
	--m_nControllerLockCount;
    if( m_nControllerLockCount == 0 && m_bUpdateNotificationsPending  )
    {
        aGuard.clear();
        impl_notifyModifiedListeners();
    }
}

	sal_Bool SAL_CALL ChartModel
::hasControllersLocked() throw(uno::RuntimeException)
{
	LifeTimeGuard aGuard(m_aLifeTimeManager);
	if(!aGuard.startApiCall())
		return sal_False; //behave passive if already disposed or closed or throw exception @todo?
	return ( m_nControllerLockCount != 0 ) ;
}

		uno::Reference< frame::XController > SAL_CALL ChartModel
::getCurrentController() throw(uno::RuntimeException)
{
	LifeTimeGuard aGuard(m_aLifeTimeManager);
	if(!aGuard.startApiCall())
		throw lang::DisposedException(::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(
				"getCurrentController was called on an already disposed or closed model" ) )
				, static_cast< ::cppu::OWeakObject* >(this));

	return impl_getCurrentController();
}

		void SAL_CALL ChartModel
::setCurrentController( const uno::Reference< frame::XController >& xController )
		throw(container::NoSuchElementException, uno::RuntimeException)
{
	LifeTimeGuard aGuard(m_aLifeTimeManager);
	if(!aGuard.startApiCall())
		throw lang::DisposedException(::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(
				"setCurrentController was called on an already disposed or closed model" ) )
				, static_cast< ::cppu::OWeakObject* >(this));

	//OSL_ENSURE( impl_isControllerConnected(xController), "setCurrentController is called with a Controller which is not connected" );
	if(!impl_isControllerConnected(xController))
		throw container::NoSuchElementException(::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(
				"setCurrentController is called with a Controller which is not connected" ) )
				, static_cast< ::cppu::OWeakObject* >(this));

	m_xCurrentController = xController;

    DisposeHelper::DisposeAndClear( m_xRangeHighlighter );
}

		uno::Reference< uno::XInterface > SAL_CALL ChartModel
::getCurrentSelection() throw(uno::RuntimeException)
{
	LifeTimeGuard aGuard(m_aLifeTimeManager);
	if(!aGuard.startApiCall())
		throw lang::DisposedException(::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(
				"getCurrentSelection was called on an already disposed or closed model" ) )
				, static_cast< ::cppu::OWeakObject* >(this));


	uno::Reference< uno::XInterface > xReturn;
	uno::Reference< frame::XController > xController = impl_getCurrentController();

	aGuard.clear();
	if( xController.is() )
	{
		uno::Reference< view::XSelectionSupplier >  xSelectionSupl( xController, uno::UNO_QUERY );
		if ( xSelectionSupl.is() )
		{
			uno::Any aSel = xSelectionSupl->getSelection();
            rtl::OUString aObjectCID;
            if( aSel >>= aObjectCID )
            {
			    xReturn.set( ObjectIdentifier::getObjectPropertySet( aObjectCID, Reference< XChartDocument >(this)));
            }
		}
	}
	return xReturn;
}


//-----------------------------------------------------------------
// lang::XComponent (base of XModel)
//-----------------------------------------------------------------
		void SAL_CALL ChartModel
::dispose() throw(uno::RuntimeException)
{
	//This object should release all resources and references in the
    //easiest possible manner
	//This object must notify all registered listeners using the method
    //<member>XEventListener::disposing</member>

    //hold no mutex
	if( !m_aLifeTimeManager.dispose() )
		return;

	//--release all resources and references
	//// @todo
    if( m_pImplChartModel.get())
        m_pImplChartModel->dispose();

    // not owner of storage
//     if( m_xStorage.is())
//     {
//         Reference< lang::XComponent > xComp( m_xStorage, uno::UNO_QUERY );
//         if( xComp.is())
//             xComp->dispose();
//     }
    m_xStorage.clear();

    if( m_xOldModelAgg.is())
    {
        m_xOldModelAgg->setDelegator( 0 );
        m_xOldModelAgg.clear();
    }

    m_aControllers.disposeAndClear( lang::EventObject( static_cast< cppu::OWeakObject * >( this )));
    m_xCurrentController.clear();

    m_xStorage.clear();
    m_xParent.clear();
    DisposeHelper::DisposeAndClear( m_xRangeHighlighter );
    OSL_TRACE( "ChartModel: dispose() called" );
}

		void SAL_CALL ChartModel
::addEventListener( const uno::Reference< lang::XEventListener > & xListener )
		throw(uno::RuntimeException)
{
	if( m_aLifeTimeManager.impl_isDisposedOrClosed() )
		return; //behave passive if already disposed or closed

	m_aLifeTimeManager.m_aListenerContainer.addInterface( ::getCppuType((const uno::Reference< lang::XEventListener >*)0), xListener );
}

		void SAL_CALL ChartModel
::removeEventListener( const uno::Reference< lang::XEventListener > & xListener )
		throw(uno::RuntimeException)
{
	if( m_aLifeTimeManager.impl_isDisposedOrClosed() )
		return; //behave passive if already disposed or closed

	m_aLifeTimeManager.m_aListenerContainer.removeInterface( ::getCppuType((const uno::Reference< lang::XEventListener >*)0), xListener );
	return;
}

//-----------------------------------------------------------------
// util::XCloseBroadcaster (base of XCloseable)
//-----------------------------------------------------------------
		void SAL_CALL ChartModel
::addCloseListener( const uno::Reference<	util::XCloseListener > & xListener )
		throw(uno::RuntimeException)
{
	m_aLifeTimeManager.g_addCloseListener( xListener );
}

		void SAL_CALL ChartModel
::removeCloseListener( const uno::Reference< util::XCloseListener > & xListener )
		throw(uno::RuntimeException)
{
	if( m_aLifeTimeManager.impl_isDisposedOrClosed() )
		return; //behave passive if already disposed or closed

	m_aLifeTimeManager.m_aListenerContainer.removeInterface( ::getCppuType((const uno::Reference< util::XCloseListener >*)0), xListener );
	return;
}

//-----------------------------------------------------------------
// util::XCloseable
//-----------------------------------------------------------------
		void SAL_CALL ChartModel
::close( sal_Bool bDeliverOwnership )
            throw( util::CloseVetoException,
                   uno::RuntimeException )
{
	//hold no mutex

	if( !m_aLifeTimeManager.g_close_startTryClose( bDeliverOwnership ) )
		return;
	//no mutex is acquired

	// At the end of this method may we must dispose ourself ...
    // and may nobody from outside hold a reference to us ...
    // then it's a good idea to do that by ourself.
    uno::Reference< uno::XInterface > xSelfHold( static_cast< ::cppu::OWeakObject* >(this) );

	//the listeners have had no veto
	//check wether we self can close
	{
		util::CloseVetoException aVetoException = util::CloseVetoException(
						::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(
						"the model itself could not be closed" ) )
						, static_cast< ::cppu::OWeakObject* >(this));

		if( m_aLifeTimeManager.g_close_isNeedToCancelLongLastingCalls( bDeliverOwnership, aVetoException ) )
		{
			////you can empty this block, if you never start longlasting calls or
			////if your longlasting calls are per default not cancelable (check how you have constructed your LifeTimeManager)

			sal_Bool bLongLastingCallsAreCanceled = sal_False;
			try
			{
				//try to cancel running longlasting calls
				//// @todo
			}
			catch( uno::Exception )
			{
				//// @todo
				//do not throw anything here!! (without endTryClose)
			}
			//if not successful canceled
			if(!bLongLastingCallsAreCanceled)
			{
				m_aLifeTimeManager.g_close_endTryClose( bDeliverOwnership, sal_True );
                throw aVetoException;
			}
		}

	}
	m_aLifeTimeManager.g_close_endTryClose_doClose();

    // BM @todo: is it ok to call the listeners here?
    impl_notifyCloseListeners();
}

//-----------------------------------------------------------------
// lang::XTypeProvider
//-----------------------------------------------------------------
    uno::Sequence< uno::Type > SAL_CALL ChartModel
::getTypes()
        throw (uno::RuntimeException)
{
    uno::Reference< lang::XTypeProvider > xAggTypeProvider;
    if( (m_xOldModelAgg->queryAggregation( ::getCppuType( & xAggTypeProvider )) >>= xAggTypeProvider)
        && xAggTypeProvider.is())
    {
        uno::Sequence< uno::Type > aOwnTypes( impl::ChartModel_Base::getTypes());
        uno::Sequence< uno::Type > aAggTypes( xAggTypeProvider->getTypes());
        uno::Sequence< uno::Type > aResult( aOwnTypes.getLength() + aAggTypes.getLength());
        sal_Int32 i=0;
        for( ;i<aOwnTypes.getLength(); ++i )
            aResult[i] = aOwnTypes[i];
        for( sal_Int32 j=0; i<aResult.getLength(); ++j, ++i)
            aResult[i] = aAggTypes[j];
        return aResult;
    }

    return impl::ChartModel_Base::getTypes();
}

//-----------------------------------------------------------------
// document::XDocumentPropertiesSupplier
//-----------------------------------------------------------------
uno::Reference< document::XDocumentProperties > SAL_CALL
        ChartModel::getDocumentProperties() throw (uno::RuntimeException)
{
    if ( !m_xDocumentProperties.is() )
    {
        uno::Reference< document::XDocumentProperties > xDocProps(
            ::comphelper::getProcessServiceFactory()->createInstance(
                C2U("com.sun.star.document.DocumentProperties") ), uno::UNO_QUERY );
        m_xDocumentProperties.set(xDocProps);
    }
    return m_xDocumentProperties;
}

//-----------------------------------------------------------------
// chart2::XChartDocument
//-----------------------------------------------------------------

        uno::Reference< chart2::XDiagram > SAL_CALL ChartModel
::getFirstDiagram()
            throw (uno::RuntimeException)
{
    OSL_ASSERT( m_pImplChartModel.get() != 0 );
    // /--
    MutexGuard aGuard( m_aModelMutex );
    try
    {
        return m_pImplChartModel->GetDiagram( 0 );
    }
    catch( container::NoSuchElementException )
    {
    }

    return uno::Reference< chart2::XDiagram >();
    // \--
}

        void SAL_CALL ChartModel
::setFirstDiagram( const uno::Reference< chart2::XDiagram >& xDiagram )
            throw (uno::RuntimeException)
{
    OSL_ASSERT( m_pImplChartModel.get() != 0 );
    {
        // /--
        MutexGuard aGuard( m_aModelMutex );
        m_pImplChartModel->RemoveAllDiagrams();
        m_pImplChartModel->AppendDiagram( xDiagram );
        // \--
    }
    setModified( sal_True );
}

        void SAL_CALL ChartModel
::createInternalDataProvider( sal_Bool bCloneExistingData )
            throw (util::CloseVetoException,
                   uno::RuntimeException)
{
    OSL_ASSERT( m_pImplChartModel.get() != 0 );
    // don't lock the mutex, because this call calls out to code that tries to
    // lock the solar mutex. On the other hand, a paint locks the solar mutex
    // and calls to the model lock the model's mutex => deadlock
    // @todo: lock a separate mutex in the InternalData class
    m_pImplChartModel->CreateInternalDataProvider( bCloneExistingData, this );
    setModified( sal_True );
}

sal_Bool SAL_CALL ChartModel::hasInternalDataProvider()
    throw (uno::RuntimeException)
{
    return m_pImplChartModel->HasInternalDataProvider();
}

        uno::Reference< chart2::data::XDataProvider > SAL_CALL ChartModel
::getDataProvider()
            throw (uno::RuntimeException)
{
    OSL_ASSERT( m_pImplChartModel.get() != 0 );
    // /--
    MutexGuard aGuard( m_aModelMutex );
    return m_pImplChartModel->GetDataProvider();
    // \--
}

// ____ XDataReceiver ____

        void SAL_CALL ChartModel
::attachDataProvider( const uno::Reference< chart2::data::XDataProvider >& xProvider )
            throw (uno::RuntimeException)
{
    OSL_ASSERT( m_pImplChartModel.get() != 0 );
    {
        // /--
        MutexGuard aGuard( m_aModelMutex );
        uno::Reference< beans::XPropertySet > xProp( xProvider, uno::UNO_QUERY );
        if( xProp.is() )
        {
            try
            {
                sal_Bool bIncludeHiddenCells = ChartModelHelper::isIncludeHiddenCells( Reference< frame::XModel >(this) );
                xProp->setPropertyValue(C2U("IncludeHiddenCells"), uno::makeAny(bIncludeHiddenCells));
            }
            catch( const beans::UnknownPropertyException& )
            {
            }
        }

        m_pImplChartModel->SetDataProvider( xProvider );
        // \--
    }
    setModified( sal_True );
}

            void SAL_CALL ChartModel
::attachNumberFormatsSupplier( const uno::Reference< util::XNumberFormatsSupplier >& xSupplier )
            throw (uno::RuntimeException)
{
    OSL_ASSERT( m_pImplChartModel.get() != 0 );
    {
        // /--
        MutexGuard aGuard( m_aModelMutex );
        m_pImplChartModel->SetNumberFormatsSupplier( xSupplier );
        // \--
    }
    setModified( sal_True );
}

        void SAL_CALL ChartModel
::setArguments( const Sequence< beans::PropertyValue >& aArguments )
            throw (lang::IllegalArgumentException,
                   uno::RuntimeException)
{
    OSL_ASSERT( m_pImplChartModel.get() != 0 );
    {
        // /--
        MutexGuard aGuard( m_aModelMutex );
        lockControllers();
        try
        {
            m_pImplChartModel->SetArguments( aArguments, true /* bSetData */ );
        }
        catch( const uno::Exception & ex )
        {
            ASSERT_EXCEPTION( ex );
        }
        unlockControllers();
        // \--
    }
    setModified( sal_True );
}

        Sequence< OUString > SAL_CALL ChartModel
::getUsedRangeRepresentations()
            throw (uno::RuntimeException)
{
    return DataSourceHelper::getUsedDataRanges( Reference< frame::XModel >(this));
}

        Reference< chart2::data::XDataSource > SAL_CALL ChartModel
::getUsedData()
            throw (uno::RuntimeException)
{
    return DataSourceHelper::getUsedData( Reference< chart2::XChartDocument >(this));
}

        Reference< chart2::data::XRangeHighlighter > SAL_CALL ChartModel
::getRangeHighlighter()
            throw (uno::RuntimeException)
{
    if( ! m_xRangeHighlighter.is())
    {
        uno::Reference< view::XSelectionSupplier > xSelSupp( this->getCurrentController(), uno::UNO_QUERY );
        if( xSelSupp.is() )
            m_xRangeHighlighter.set( ChartModelHelper::createRangeHighlighter( xSelSupp ));
    }
    return m_xRangeHighlighter;
}


        void SAL_CALL ChartModel
::setChartTypeManager( const uno::Reference< chart2::XChartTypeManager >& xNewManager )
            throw (uno::RuntimeException)
{
    OSL_ASSERT( m_pImplChartModel.get() != 0 );
    {
        // /--
        MutexGuard aGuard( m_aModelMutex );
        m_pImplChartModel->SetChartTypeManager( xNewManager );
        // \--
    }
    setModified( sal_True );
}

        uno::Reference< chart2::XChartTypeManager > SAL_CALL ChartModel
::getChartTypeManager()
            throw (uno::RuntimeException)
{
    OSL_ASSERT( m_pImplChartModel.get() != 0 );
    // /--
    MutexGuard aGuard( m_aModelMutex );
    return m_pImplChartModel->GetChartTypeManager();
    // \--
}

    uno::Reference< beans::XPropertySet > SAL_CALL ChartModel
::getPageBackground()
    throw (uno::RuntimeException)
{
    OSL_ASSERT( m_pImplChartModel.get() != 0 );
    // /--
    MutexGuard aGuard( m_aModelMutex );
    return m_pImplChartModel->GetPageBackground();
    // \--
}

// ____ XTitled ____
uno::Reference< chart2::XTitle > SAL_CALL ChartModel::getTitleObject()
    throw (uno::RuntimeException)
{
    OSL_ASSERT( m_pImplChartModel.get() != 0 );
    // /--
    MutexGuard aGuard( m_aModelMutex );
    return m_pImplChartModel->GetTitle();
    // \--
}

void SAL_CALL ChartModel::setTitleObject(
    const uno::Reference<
    chart2::XTitle >& Title )
    throw (uno::RuntimeException)
{
    OSL_ASSERT( m_pImplChartModel.get() != 0 );
    {
        // /--
        MutexGuard aGuard( m_aModelMutex );
        m_pImplChartModel->SetTitle( Title );
        // \--
    }
    setModified( sal_True );
}

void ChartModel::impl_createOldModelAgg()
{
    if( ! m_xOldModelAgg.is())
    {
        m_xOldModelAgg.set(
            m_xContext->getServiceManager()->createInstanceWithContext(
            CHART_CHARTAPIWRAPPER_SERVICE_NAME,
            m_xContext ), uno::UNO_QUERY_THROW );
        m_xOldModelAgg->setDelegator( static_cast< ::cppu::OWeakObject* >( this ));
    }
}

// ____ XInterface (for old API wrapper) ____
uno::Any SAL_CALL ChartModel::queryInterface( const uno::Type& aType )
    throw (uno::RuntimeException)
{
    uno::Any aResult( impl::ChartModel_Base::queryInterface( aType ));

    if( ! aResult.hasValue())
    {
        // try old API wrapper
        try
        {
            impl_createOldModelAgg();
            if( m_xOldModelAgg.is())
                aResult = m_xOldModelAgg->queryAggregation( aType );
        }
        catch( uno::Exception & ex )
        {
            ASSERT_EXCEPTION( ex );
        }
    }

    return aResult;
}

// ____ XCloneable ____
Reference< util::XCloneable > SAL_CALL ChartModel::createClone()
    throw (uno::RuntimeException)
{
    return Reference< util::XCloneable >( new ChartModel( *this ));
}

// ____ XVisualObject ____
void SAL_CALL ChartModel::setVisualAreaSize( ::sal_Int64 nAspect, const awt::Size& aSize )
    throw (lang::IllegalArgumentException,
           embed::WrongStateException,
           uno::Exception,
           uno::RuntimeException)
{
    if( nAspect == embed::Aspects::MSOLE_CONTENT )
    {
        bool bChanged =
            (m_aVisualAreaSize.Width != aSize.Width ||
             m_aVisualAreaSize.Height != aSize.Height);
        m_aVisualAreaSize = aSize;
        if( bChanged )
            setModified( sal_True );
    }
    else
    {
        OSL_ENSURE( false, "setVisualAreaSize: Aspect not implemented yet.");
    }
}

awt::Size SAL_CALL ChartModel::getVisualAreaSize( ::sal_Int64 nAspect )
    throw (lang::IllegalArgumentException,
           embed::WrongStateException,
           uno::Exception,
           uno::RuntimeException)
{
    OSL_ENSURE( nAspect == embed::Aspects::MSOLE_CONTENT,
                "No aspects other than content are supported" );
    (void)(nAspect); // avoid warning in non-debug builds
    // other possible aspects are MSOLE_THUMBNAIL, MSOLE_ICON and MSOLE_DOCPRINT

    return m_aVisualAreaSize;
}

embed::VisualRepresentation SAL_CALL ChartModel::getPreferredVisualRepresentation( ::sal_Int64 nAspect )
    throw (lang::IllegalArgumentException,
           embed::WrongStateException,
           uno::Exception,
           uno::RuntimeException)
{
    OSL_ENSURE( nAspect == embed::Aspects::MSOLE_CONTENT,
                "No aspects other than content are supported" );
    (void)(nAspect); // avoid warning in non-debug builds

    embed::VisualRepresentation aResult;

    try
    {
        Sequence< sal_Int8 > aMetafile;

        //get view from old api wrapper
        Reference< datatransfer::XTransferable > xTransferable(
            this->createInstance( CHART_VIEW_SERVICE_NAME ), uno::UNO_QUERY );
        if( xTransferable.is() )
        {
            datatransfer::DataFlavor aDataFlavor( lcl_aGDIMetaFileMIMEType,
                    C2U( "GDIMetaFile" ),
		            ::getCppuType( (const uno::Sequence< sal_Int8 >*) 0 ) );

            uno::Any aData( xTransferable->getTransferData( aDataFlavor ) );
            aData >>= aMetafile;
        }

        aResult.Flavor.MimeType = lcl_aGDIMetaFileMIMEType;
        aResult.Flavor.DataType = getCppuType( &aMetafile );

        aResult.Data <<= aMetafile;
    }
    catch( uno::Exception & ex )
    {
        ASSERT_EXCEPTION( ex );
    }

    return aResult;
}

::sal_Int32 SAL_CALL ChartModel::getMapUnit( ::sal_Int64 nAspect )
    throw (uno::Exception,
           uno::RuntimeException)
{
    OSL_ENSURE( nAspect == embed::Aspects::MSOLE_CONTENT,
                "No aspects other than content are supported" );
    (void)(nAspect); // avoid warning in non-debug builds
    return embed::EmbedMapUnits::ONE_100TH_MM;
}

// ____ datatransfer::XTransferable ____
uno::Any SAL_CALL ChartModel::getTransferData( const datatransfer::DataFlavor& aFlavor )
    throw (datatransfer::UnsupportedFlavorException,
           io::IOException,
           uno::RuntimeException)
{
    uno::Any aResult;
    if( this->isDataFlavorSupported( aFlavor ))
    {
        try
        {
            //get view from old api wrapper
            Reference< datatransfer::XTransferable > xTransferable(
                this->createInstance( CHART_VIEW_SERVICE_NAME ), uno::UNO_QUERY );
            if( xTransferable.is() &&
                xTransferable->isDataFlavorSupported( aFlavor ))
            {
                aResult = xTransferable->getTransferData( aFlavor );
            }
        }
        catch( uno::Exception & ex )
        {
            ASSERT_EXCEPTION( ex );
        }
    }
    else
    {
        throw datatransfer::UnsupportedFlavorException(
            aFlavor.MimeType, static_cast< ::cppu::OWeakObject* >( this ));
    }

    return aResult;
}

Sequence< datatransfer::DataFlavor > SAL_CALL ChartModel::getTransferDataFlavors()
    throw (uno::RuntimeException)
{
    uno::Sequence< datatransfer::DataFlavor > aRet(1);

//     aRet[0] = datatransfer::DataFlavor( lcl_aGDIMetaFileMIMEType,
//         C2U( "GDIMetaFile" ),
// 		::getCppuType( (const uno::Sequence< sal_Int8 >*) NULL ) );
    aRet[0] = datatransfer::DataFlavor( lcl_aGDIMetaFileMIMETypeHighContrast,
        C2U( "GDIMetaFile" ),
		::getCppuType( (const uno::Sequence< sal_Int8 >*) NULL ) );

	return aRet;
}

::sal_Bool SAL_CALL ChartModel::isDataFlavorSupported( const datatransfer::DataFlavor& aFlavor )
    throw (uno::RuntimeException)
{
//     return ( aFlavor.MimeType.equals(lcl_aGDIMetaFileMIMEType) ||
//              aFlavor.MimeType.equals(lcl_aGDIMetaFileMIMETypeHighContrast) );
    return aFlavor.MimeType.equals(lcl_aGDIMetaFileMIMETypeHighContrast);
}



namespace
{
enum eServiceType
{
    SERVICE_DASH_TABLE,
    SERVICE_GARDIENT_TABLE,
    SERVICE_HATCH_TABLE,
    SERVICE_BITMAP_TABLE,
    SERVICE_TRANSP_GRADIENT_TABLE,
    SERVICE_MARKER_TABLE,
    SERVICE_NAMESPACE_MAP
};

typedef ::std::map< ::rtl::OUString, enum eServiceType > tServiceNameMap;
typedef ::comphelper::MakeMap< ::rtl::OUString, enum eServiceType > tMakeServiceNameMap;

tServiceNameMap & lcl_getStaticServiceNameMap()
{
    static tServiceNameMap aServiceNameMap(
        tMakeServiceNameMap
        ( C2U( "com.sun.star.drawing.DashTable" ),                    SERVICE_DASH_TABLE )
        ( C2U( "com.sun.star.drawing.GradientTable" ),                SERVICE_GARDIENT_TABLE )
        ( C2U( "com.sun.star.drawing.HatchTable" ),                   SERVICE_HATCH_TABLE )
        ( C2U( "com.sun.star.drawing.BitmapTable" ),                  SERVICE_BITMAP_TABLE )
        ( C2U( "com.sun.star.drawing.TransparencyGradientTable" ),    SERVICE_TRANSP_GRADIENT_TABLE )
        ( C2U( "com.sun.star.drawing.MarkerTable" ),                  SERVICE_MARKER_TABLE )
        ( C2U( "com.sun.star.xml.NamespaceMap" ),                     SERVICE_NAMESPACE_MAP )
        );
    return aServiceNameMap;
}
}
// ____ XMultiServiceFactory ____
Reference< uno::XInterface > SAL_CALL ChartModel::createInstance( const OUString& rServiceSpecifier )
            throw( uno::Exception, uno::RuntimeException )
{
    if( ! m_pImplChartModel.get() )
        return 0;

    uno::Reference< uno::XInterface > xResult;
    tServiceNameMap & rMap = lcl_getStaticServiceNameMap();

    tServiceNameMap::const_iterator aIt( rMap.find( rServiceSpecifier ));
    if( aIt != rMap.end())
    {
        switch( (*aIt).second )
        {
            case SERVICE_DASH_TABLE:
                return m_pImplChartModel->GetDashTable();
            case SERVICE_GARDIENT_TABLE:
                return m_pImplChartModel->GetGradientTable();
            case SERVICE_HATCH_TABLE:
                return m_pImplChartModel->GetHatchTable();
            case SERVICE_BITMAP_TABLE:
                return m_pImplChartModel->GetBitmapTable();
            case SERVICE_TRANSP_GRADIENT_TABLE:
                return m_pImplChartModel->GetTransparencyGradientTable();
            case SERVICE_MARKER_TABLE:
                // not supported
                return 0;
            case SERVICE_NAMESPACE_MAP:
                // not yet supported, @todo
//                 return 0;
                return m_pImplChartModel->GetXMLNameSpaceMap();
        }
    }
    else
    {
        impl_createOldModelAgg();
        if( m_xOldModelAgg.is() )
        {
            Any aAny = m_xOldModelAgg->queryAggregation( ::getCppuType((const uno::Reference< lang::XMultiServiceFactory >*)0) );
            uno::Reference< lang::XMultiServiceFactory > xOldModelFactory;
            if( (aAny >>= xOldModelFactory) && xOldModelFactory.is() )
            {
                return xOldModelFactory->createInstance( rServiceSpecifier );
            }
        }
    }
    return 0;
}

Reference< uno::XInterface > SAL_CALL ChartModel::createInstanceWithArguments(
            const OUString& rServiceSpecifier , const Sequence< Any >& Arguments )
            throw( uno::Exception, uno::RuntimeException )
{
    OSL_ENSURE( Arguments.getLength(), "createInstanceWithArguments: Warning: Arguments are ignored" );
    (void)(Arguments); // avoid warning in non-debug builds
    return createInstance( rServiceSpecifier );
}

Sequence< OUString > SAL_CALL ChartModel::getAvailableServiceNames()
            throw( uno::RuntimeException )
{
    uno::Sequence< ::rtl::OUString > aResult;

    impl_createOldModelAgg();
    if( m_xOldModelAgg.is())
    {
        Any aAny = m_xOldModelAgg->queryAggregation( ::getCppuType((const uno::Reference< lang::XMultiServiceFactory >*)0) );
        uno::Reference< lang::XMultiServiceFactory > xOldModelFactory;
        if( (aAny >>= xOldModelFactory) && xOldModelFactory.is() )
        {
            return xOldModelFactory->getAvailableServiceNames();
        }
    }
    return aResult;
}

// ____ XUnoTunnel ___
::sal_Int64 SAL_CALL ChartModel::getSomething( const Sequence< ::sal_Int8 >& aIdentifier )
        throw( uno::RuntimeException)
{
    if( aIdentifier.getLength() == 16 && 0 == rtl_compareMemory( SvNumberFormatsSupplierObj::getUnoTunnelId().getConstArray(),
														 aIdentifier.getConstArray(), 16 ) )
	{
        OSL_ENSURE( m_pImplChartModel.get(), "need a model implementation to provide a numberformatter" );
        if( m_pImplChartModel.get() )
        {
            Reference< lang::XUnoTunnel > xTunnel( m_pImplChartModel->GetNumberFormatsSupplier(), uno::UNO_QUERY );
            if( xTunnel.is() )
                return xTunnel->getSomething( aIdentifier );
        }
        return 0;
	}
	return 0;
}

// ____ XNumberFormatsSupplier ____
uno::Reference< beans::XPropertySet > SAL_CALL ChartModel::getNumberFormatSettings()
    throw (uno::RuntimeException)
{
    OSL_ENSURE( m_pImplChartModel.get(), "need a model implementation to provide a numberformatter" );
    if( m_pImplChartModel.get() )
    {
        Reference< util::XNumberFormatsSupplier > xSupplier( m_pImplChartModel->GetNumberFormatsSupplier() );
        if( xSupplier.is() )
            return xSupplier->getNumberFormatSettings();
    }
    return uno::Reference< beans::XPropertySet >();
}

uno::Reference< util::XNumberFormats > SAL_CALL ChartModel::getNumberFormats()
    throw (uno::RuntimeException)
{
    OSL_ENSURE( m_pImplChartModel.get(), "need a model implementation to provide a numberformatter" );
    if( m_pImplChartModel.get() )
    {
        Reference< util::XNumberFormatsSupplier > xSupplier( m_pImplChartModel->GetNumberFormatsSupplier() );
        if( xSupplier.is() )
            return xSupplier->getNumberFormats();
    }
    return uno::Reference< util::XNumberFormats >();
}

// ____ XChild ____
Reference< uno::XInterface > SAL_CALL ChartModel::getParent()
    throw (uno::RuntimeException)
{
    return Reference< uno::XInterface >(m_xParent,uno::UNO_QUERY);
}

void SAL_CALL ChartModel::setParent( const Reference< uno::XInterface >& Parent )
    throw (lang::NoSupportException,
           uno::RuntimeException)
{
    if( Parent != m_xParent )
        m_xParent.set( Parent, uno::UNO_QUERY );
}

// ____ XUndoManager ____
Reference< chart2::XUndoManager > SAL_CALL ChartModel::getUndoManager()
    throw (uno::RuntimeException)
{
    return m_pImplChartModel->GetUndoManager();
}

// ____ XDataSource ____
uno::Sequence< Reference< chart2::data::XLabeledDataSequence > > SAL_CALL ChartModel::getDataSequences()
    throw (uno::RuntimeException)
{
    Reference< chart2::data::XDataSource > xSource( 
        DataSourceHelper::getUsedData( uno::Reference< frame::XModel >(this) ) );
    if( xSource.is())
        return xSource->getDataSequences();

    return uno::Sequence< Reference< chart2::data::XLabeledDataSequence > >();
}

}  // namespace chart
