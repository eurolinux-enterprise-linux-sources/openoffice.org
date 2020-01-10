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
#include "precompiled_basic.hxx"
#include <basic/basicmanagerrepository.hxx>
#include <basic/basmgr.hxx>
#include "scriptcont.hxx"
#include "dlgcont.hxx"
#include <basic/sbuno.hxx>
#include "sbintern.hxx"

/** === begin UNO includes === **/
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/document/XStorageBasedDocument.hpp>
#include <com/sun/star/document/XEmbeddedScripts.hpp>
/** === end UNO includes === **/
#include <svtools/ehdl.hxx>
#include <svtools/sfxecode.hxx>
#include <svtools/pathoptions.hxx>
#include <svtools/smplhint.hxx>
#include <vcl/svapp.hxx>
#include <tools/debug.hxx>
#include <tools/diagnose_ex.h>
#include <tools/urlobj.hxx>
#include <comphelper/stl_types.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/documentinfo.hxx>
#include <unotools/eventlisteneradapter.hxx>

#ifndef INCLUDED_OSL_DOUBLECHECKEDLOCKING_H
#include <rtl/instance.hxx>
#endif

#include <map>

//........................................................................
namespace basic
{
//........................................................................

	/** === begin UNO using === **/
    using ::com::sun::star::uno::Reference;
    using ::com::sun::star::frame::XModel;
    using ::com::sun::star::uno::XInterface;
    using ::com::sun::star::uno::UNO_QUERY;
    using ::com::sun::star::embed::XStorage;
    using ::com::sun::star::script::XPersistentLibraryContainer;
    using ::com::sun::star::uno::Any;
    using ::com::sun::star::lang::XMultiServiceFactory;
    using ::com::sun::star::uno::UNO_QUERY_THROW;
    using ::com::sun::star::beans::XPropertySet;
    using ::com::sun::star::uno::Exception;
    using ::com::sun::star::document::XStorageBasedDocument;
    using ::com::sun::star::lang::XComponent;
    using ::com::sun::star::document::XEmbeddedScripts;
    /** === end UNO using === **/

    typedef BasicManager*   BasicManagerPointer;
    typedef ::std::map< Reference< XInterface >, BasicManagerPointer, ::comphelper::OInterfaceCompare< XInterface > > BasicManagerStore;

    typedef ::std::vector< BasicManagerCreationListener* >  CreationListeners;

	//====================================================================
	//= BasicManagerCleaner
	//====================================================================
    /// is the only instance which is allowed to delete a BasicManager instance
    class BasicManagerCleaner
    {
    public:
        static void deleteBasicManager( BasicManager*& _rpManager )
        {
            delete _rpManager;
            _rpManager = NULL;
        }
    };

	//====================================================================
	//= ImplRepository
	//====================================================================
    class ImplRepository : public ::utl::OEventListenerAdapter, public SfxListener
    {
    private:
        friend struct CreateImplRepository;
        ImplRepository();

    private:
        ::osl::Mutex        m_aMutex;
        BasicManagerStore   m_aStore;
        CreationListeners   m_aCreationListeners;

    public:
        static ImplRepository& Instance();

        BasicManager*   getDocumentBasicManager( const Reference< XModel >& _rxDocumentModel );
        BasicManager*   getApplicationBasicManager( bool _bCreate );
        void            setApplicationBasicManager( BasicManager* _pBasicManager );
        void    registerCreationListener( BasicManagerCreationListener& _rListener );
        void    revokeCreationListener( BasicManagerCreationListener& _rListener );

    private:
        /** retrieves the location at which the BasicManager for the given model
            is stored.

            If previously, the BasicManager for this model has never been requested,
            then the model is added to the map, with an initial NULL BasicManager.

            @param _rxDocumentModel
                the model whose BasicManager's location is to be retrieved. Must not be <NULL/>.

            @precond
                our mutex is locked
        */
        BasicManagerPointer&
                impl_getLocationForModel( const Reference< XModel >& _rxDocumentModel );

        /** creates a new BasicManager instance for the given model
        */
        BasicManagerPointer
                impl_createManagerForModel( const Reference< XModel >& _rxDocumentModel );

        /** creates the application-wide BasicManager
        */
        BasicManagerPointer impl_createApplicationBasicManager();

        /** notifies all listeners which expressed interest in the creation of BasicManager instances.
        */
        void    impl_notifyCreationListeners(
                    const Reference< XModel >& _rxDocumentModel,
                    BasicManager& _rManager
                 );

        /** retrieves the current storage of a given document

            @param  _rxDocument
                the document whose storage is to be retrieved.

            @param  _out_rStorage
                takes the storage upon successful return. Note that this might be <NULL/> even
                if <TRUE/> is returned. In this case, the document has not yet been saved.

            @return
                <TRUE/> if the storage could be successfully retrieved (in which case
                <arg>_out_rStorage</arg> might or might not be <NULL/>), <FALSE/> otherwise.
                In the latter case, processing this document should stop.
        */
        bool    impl_getDocumentStorage_nothrow( const Reference< XModel >& _rxDocument, Reference< XStorage >& _out_rStorage );

        /** retrieves the containers for Basic and Dialog libraries for a given document

            @param  _rxDocument
                the document whose containers are to be retrieved.

            @param _out_rxBasicLibraries
                takes the basic library container upon successful return

            @param _out_rxDialogLibraries
                takes the dialog library container upon successful return

            @return
                <TRUE/> if and only if both containers exist, and could successfully be retrieved
        */
        bool    impl_getDocumentLibraryContainers_nothrow(
                    const Reference< XModel >& _rxDocument,
                    Reference< XPersistentLibraryContainer >& _out_rxBasicLibraries,
                    Reference< XPersistentLibraryContainer >& _out_rxDialogLibraries
                );

        /** initializes the given library containers, which belong to a document
        */
        void    impl_initDocLibraryContainers_nothrow(
                    const Reference< XPersistentLibraryContainer >& _rxBasicLibraries,
                    const Reference< XPersistentLibraryContainer >& _rxDialogLibraries
                );

        // OEventListenerAdapter overridables
		virtual void _disposing( const ::com::sun::star::lang::EventObject& _rSource );

        // SfxListener overridables
	    virtual void Notify( SfxBroadcaster& _rBC, const SfxHint& _rHint );

        /** removes the Model/BasicManager pair given by iterator from our store
        */
        void impl_removeFromRepository( BasicManagerStore::iterator _pos );

    private:
        StarBASIC* impl_getDefaultAppBasicLibrary();
    };

    //====================================================================
	//= CreateImplRepository
	//====================================================================
    struct CreateImplRepository
    {
        ImplRepository* operator()()
        {
            static ImplRepository* pRepository = new ImplRepository;
            return pRepository;
        }
    };


	//====================================================================
	//= ImplRepository
	//====================================================================
	//--------------------------------------------------------------------
    ImplRepository::ImplRepository()
    {
    }

	//--------------------------------------------------------------------
    ImplRepository& ImplRepository::Instance()
    {
        return *rtl_Instance< ImplRepository, CreateImplRepository, ::osl::MutexGuard, ::osl::GetGlobalMutex >::
            create( CreateImplRepository(), ::osl::GetGlobalMutex() );
    }

	//--------------------------------------------------------------------
    BasicManager* ImplRepository::getDocumentBasicManager( const Reference< XModel >& _rxDocumentModel )
    {
        ::osl::MutexGuard aGuard( m_aMutex );

        BasicManagerPointer& pBasicManager = impl_getLocationForModel( _rxDocumentModel );
        if ( pBasicManager == NULL )
            pBasicManager = impl_createManagerForModel( _rxDocumentModel );

        return pBasicManager;
    }

	//--------------------------------------------------------------------
    BasicManager* ImplRepository::getApplicationBasicManager( bool _bCreate )
    {
        ::osl::MutexGuard aGuard( m_aMutex );

        BasicManager* pAppManager = GetSbData()->pAppBasMgr;
        if ( ( pAppManager == NULL ) && _bCreate )
            pAppManager = impl_createApplicationBasicManager();

        return pAppManager;
    }

	//--------------------------------------------------------------------
    void ImplRepository::setApplicationBasicManager( BasicManager* _pBasicManager )
    {
        ::osl::MutexGuard aGuard( m_aMutex );

        BasicManager* pPreviousManager = getApplicationBasicManager( false );
        BasicManagerCleaner::deleteBasicManager( pPreviousManager );

        GetSbData()->pAppBasMgr = _pBasicManager;
    }

	//--------------------------------------------------------------------
    BasicManager* ImplRepository::impl_createApplicationBasicManager()
    {
        ::osl::MutexGuard aGuard( m_aMutex );
        OSL_PRECOND( getApplicationBasicManager( false ) == NULL, "ImplRepository::impl_createApplicationBasicManager: there already is one!" );

        // Determine Directory
		SvtPathOptions aPathCFG;
		String aAppBasicDir( aPathCFG.GetBasicPath() );
		if ( !aAppBasicDir.Len() )
            aPathCFG.SetBasicPath( String::CreateFromAscii("$(prog)") );

		// #58293# soffice.new search only in user dir => first dir
		String aAppFirstBasicDir = aAppBasicDir.GetToken(1);

		// Create basic and load it
		// MT: #47347# AppBasicDir is now a PATH
        INetURLObject aAppBasic( SvtPathOptions().SubstituteVariable( String::CreateFromAscii("$(progurl)") ) );
        aAppBasic.insertName( Application::GetAppName() );

    	BasicManager* pBasicManager = new BasicManager( new StarBASIC, &aAppBasicDir );
        setApplicationBasicManager( pBasicManager );

		// Als Destination das erste Dir im Pfad:
		String aFileName( aAppBasic.getName() );
        aAppBasic = INetURLObject( aAppBasicDir.GetToken(1) );
        DBG_ASSERT( aAppBasic.GetProtocol() != INET_PROT_NOT_VALID, "Invalid URL!" );
		aAppBasic.insertName( aFileName );
		pBasicManager->SetStorageName( aAppBasic.PathToFileName() );

		// Basic container
		SfxScriptLibraryContainer* pBasicCont = new SfxScriptLibraryContainer( Reference< XStorage >() );
        Reference< XPersistentLibraryContainer > xBasicCont( pBasicCont );
        pBasicCont->setBasicManager( pBasicManager );

		// Dialog container
		SfxDialogLibraryContainer* pDialogCont = new SfxDialogLibraryContainer( Reference< XStorage >() );
        Reference< XPersistentLibraryContainer > xDialogCont( pDialogCont );

	    LibraryContainerInfo aInfo( xBasicCont, xDialogCont, static_cast< OldBasicPassword* >( pBasicCont ) );
	    pBasicManager->SetLibraryContainerInfo( aInfo );

        // global constants

        // StarDesktop
        Reference< XMultiServiceFactory > xSMgr = ::comphelper::getProcessServiceFactory();
        pBasicManager->SetGlobalUNOConstant(
            "StarDesktop",
            makeAny( xSMgr->createInstance( ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.frame.Desktop" ) ) ) )
         );

        // (BasicLibraries and DialogLibraries have automatically been added in SetLibraryContainerInfo)

        // notify
        impl_notifyCreationListeners( NULL, *pBasicManager );

        // outta here
        return pBasicManager;
    }

	//--------------------------------------------------------------------
    void ImplRepository::registerCreationListener( BasicManagerCreationListener& _rListener )
    {
        ::osl::MutexGuard aGuard( m_aMutex );
        m_aCreationListeners.push_back( &_rListener );
    }

	//--------------------------------------------------------------------
    void ImplRepository::revokeCreationListener( BasicManagerCreationListener& _rListener )
    {
        ::osl::MutexGuard aGuard( m_aMutex );
        CreationListeners::iterator pos = ::std::find( m_aCreationListeners.begin(), m_aCreationListeners.end(), &_rListener );
        if ( pos != m_aCreationListeners.end() )
            m_aCreationListeners.erase( pos );
        else {
            DBG_ERROR( "ImplRepository::revokeCreationListener: listener is not registered!" );
        }
    }

	//--------------------------------------------------------------------
    void ImplRepository::impl_notifyCreationListeners( const Reference< XModel >& _rxDocumentModel, BasicManager& _rManager )
    {
        for (   CreationListeners::const_iterator loop = m_aCreationListeners.begin();
                loop != m_aCreationListeners.end();
                ++loop
            )
        {
            (*loop)->onBasicManagerCreated( _rxDocumentModel, _rManager );
        }
    }

	//--------------------------------------------------------------------
    StarBASIC* ImplRepository::impl_getDefaultAppBasicLibrary()
    {
        BasicManager* pAppManager = getApplicationBasicManager( true );

        StarBASIC* pAppBasic = pAppManager ? pAppManager->GetLib(0) : NULL;
        DBG_ASSERT( pAppBasic != NULL, "impl_getApplicationBasic: unable to determine the default application's Basic library!" );
        return pAppBasic;
    }

	//--------------------------------------------------------------------
    BasicManagerPointer& ImplRepository::impl_getLocationForModel( const Reference< XModel >& _rxDocumentModel )
    {
        Reference< XInterface > xNormalized( _rxDocumentModel, UNO_QUERY );
        DBG_ASSERT( xNormalized.is(), "ImplRepository::impl_getLocationForModel: invalid model!" );

        BasicManagerPointer& location = m_aStore[ xNormalized ];
        return location;
    }

	//--------------------------------------------------------------------
    void ImplRepository::impl_initDocLibraryContainers_nothrow( const Reference< XPersistentLibraryContainer >& _rxBasicLibraries, const Reference< XPersistentLibraryContainer >& _rxDialogLibraries )
    {
        OSL_PRECOND( _rxBasicLibraries.is() && _rxDialogLibraries.is(),
            "ImplRepository::impl_initDocLibraryContainers_nothrow: illegal library containers, this will crash!" );

        try
        {
	        // ensure there's a standard library in the basic container
            ::rtl::OUString aStdLibName( RTL_CONSTASCII_USTRINGPARAM( "Standard" ) );
	        if ( !_rxBasicLibraries->hasByName( aStdLibName ) )
		        _rxBasicLibraries->createLibrary( aStdLibName );
            // as well as in the dialog container
	        if ( !_rxDialogLibraries->hasByName( aStdLibName ) )
		        _rxDialogLibraries->createLibrary( aStdLibName );
        }
        catch( const Exception& )
        {
        	DBG_UNHANDLED_EXCEPTION();
        }
    }

    //--------------------------------------------------------------------
    BasicManagerPointer ImplRepository::impl_createManagerForModel( const Reference< XModel >& _rxDocumentModel )
    {
        StarBASIC* pAppBasic = impl_getDefaultAppBasicLibrary();

        BasicManager* pBasicManager( NULL );
        Reference< XStorage > xStorage;
        if ( !impl_getDocumentStorage_nothrow( _rxDocumentModel, xStorage ) )
            // the document is not able to provide the storage it is based on.
            return pBasicManager;

        Reference< XPersistentLibraryContainer > xBasicLibs;
        Reference< XPersistentLibraryContainer > xDialogLibs;
        if ( !impl_getDocumentLibraryContainers_nothrow( _rxDocumentModel, xBasicLibs, xDialogLibs ) )
            // the document does not have BasicLibraries and DialogLibraries
            return pBasicManager;

        if ( xStorage.is() )
        {
		    // load BASIC-manager
		    SfxErrorContext aErrContext( ERRCTX_SFX_LOADBASIC,
                ::comphelper::DocumentInfo::getDocumentTitle( _rxDocumentModel ) );
		    String aAppBasicDir = SvtPathOptions().GetBasicPath();

            // Storage and BaseURL are only needed by binary documents!
		    SotStorageRef xDummyStor = new SotStorage( ::rtl::OUString() );
            pBasicManager = new BasicManager( *xDummyStor, String() /* TODO/LATER: xStorage */,
															    pAppBasic,
															    &aAppBasicDir, TRUE );
		    if ( pBasicManager->HasErrors() )
		    {
			    // handle errors
			    BasicError* pErr = pBasicManager->GetFirstError();
			    while ( pErr )
			    {
				    // show message to user
				    if ( ERRCODE_BUTTON_CANCEL == ErrorHandler::HandleError( pErr->GetErrorId() ) )
				    {
					    // user wants to break loading of BASIC-manager
                        BasicManagerCleaner::deleteBasicManager( pBasicManager );
					    xStorage.clear();
					    break;
				    }
				    pErr = pBasicManager->GetNextError();
			    }
		    }
	    }

	    // not loaded?
	    if ( !xStorage.is() )
	    {
		    // create new BASIC-manager
		    StarBASIC* pBasic = new StarBASIC( pAppBasic );
		    pBasic->SetFlag( SBX_EXTSEARCH );
		    pBasicManager = new BasicManager( pBasic, NULL, TRUE );
	    }

        // knit the containers with the BasicManager
	    LibraryContainerInfo aInfo( xBasicLibs, xDialogLibs, dynamic_cast< OldBasicPassword* >( xBasicLibs.get() ) );
        OSL_ENSURE( aInfo.mpOldBasicPassword, "ImplRepository::impl_createManagerForModel: wrong BasicLibraries implementation!" );
	    pBasicManager->SetLibraryContainerInfo( aInfo );
        //pBasicCont->setBasicManager( pBasicManager );
            // that's not needed anymore today. The containers will retrieve their associated
            // BasicManager from the BasicManagerRepository, when needed.

        // initialize the containers
        impl_initDocLibraryContainers_nothrow( xBasicLibs, xDialogLibs );

	    // damit auch Dialoge etc. 'qualifiziert' angesprochen werden k"onnen
	    pBasicManager->GetLib(0)->SetParent( pAppBasic );

	    // global properties in the document's Basic
        pBasicManager->SetGlobalUNOConstant( "ThisComponent", makeAny( _rxDocumentModel ) );

        // notify
        impl_notifyCreationListeners( _rxDocumentModel, *pBasicManager );

        // register as listener for this model being disposed/closed
        Reference< XComponent > xDocumentComponent( _rxDocumentModel, UNO_QUERY );
        OSL_ENSURE( xDocumentComponent.is(), "ImplRepository::impl_createManagerForModel: the document must be an XComponent!" );
        startComponentListening( xDocumentComponent );

        // register as listener for the BasicManager being destroyed
        StartListening( *pBasicManager );

        return pBasicManager;
    }

    //--------------------------------------------------------------------
    bool ImplRepository::impl_getDocumentStorage_nothrow( const Reference< XModel >& _rxDocument, Reference< XStorage >& _out_rStorage )
    {
        _out_rStorage.clear();
        try
        {
            Reference< XStorageBasedDocument > xStorDoc( _rxDocument, UNO_QUERY_THROW );
            _out_rStorage.set( xStorDoc->getDocumentStorage() );
        }
        catch( const Exception& )
        {
        	DBG_UNHANDLED_EXCEPTION();
            return false;
        }
        return true;
    }

    //--------------------------------------------------------------------
    bool ImplRepository::impl_getDocumentLibraryContainers_nothrow( const Reference< XModel >& _rxDocument,
        Reference< XPersistentLibraryContainer >& _out_rxBasicLibraries, Reference< XPersistentLibraryContainer >& _out_rxDialogLibraries )
    {
        _out_rxBasicLibraries.clear();
        _out_rxDialogLibraries.clear();
        try
        {
            Reference< XEmbeddedScripts > xScripts( _rxDocument, UNO_QUERY_THROW );
            _out_rxBasicLibraries.set( xScripts->getBasicLibraries(), UNO_QUERY_THROW );
            _out_rxDialogLibraries.set( xScripts->getDialogLibraries(), UNO_QUERY_THROW );
        }
        catch( const Exception& )
        {
        	DBG_UNHANDLED_EXCEPTION();
        }
        return _out_rxBasicLibraries.is() && _out_rxDialogLibraries.is();
    }

	//--------------------------------------------------------------------
    void ImplRepository::impl_removeFromRepository( BasicManagerStore::iterator _pos )
    {
        OSL_PRECOND( _pos != m_aStore.end(), "ImplRepository::impl_removeFromRepository: invalid position!" );

        BasicManager* pManager = _pos->second;

        // *first* remove from map (else Notify won't work properly)
        m_aStore.erase( _pos );

        // *then* delete the BasicManager
        EndListening( *pManager );
        BasicManagerCleaner::deleteBasicManager( pManager );
    }

	//--------------------------------------------------------------------
    void ImplRepository::_disposing( const ::com::sun::star::lang::EventObject& _rSource )
    {
        ::osl::MutexGuard aGuard( m_aMutex );

        Reference< XInterface > xNormalizedSource( _rSource.Source, UNO_QUERY );
    #if OSL_DEBUG_LEVEL > 0
        bool bFound = false;
    #endif

        for (   BasicManagerStore::iterator loop = m_aStore.begin();
                loop != m_aStore.end();
                ++loop
            )
        {
            if ( loop->first.get() == xNormalizedSource.get() )
            {
                impl_removeFromRepository( loop );
            #if OSL_DEBUG_LEVEL > 0
                bFound = true;
            #endif
                break;
            }
        }

        OSL_ENSURE( bFound, "ImplRepository::_disposing: where does this come from?" );
    }

	//--------------------------------------------------------------------
    void ImplRepository::Notify( SfxBroadcaster& _rBC, const SfxHint& _rHint )
    {
        const SfxSimpleHint* pSimpleHint = dynamic_cast< const SfxSimpleHint* >( &_rHint );
        if ( !pSimpleHint || ( pSimpleHint->GetId() != SFX_HINT_DYING ) )
            // not interested in
            return;

        BasicManager* pManager = dynamic_cast< BasicManager* >( &_rBC );
        OSL_ENSURE( pManager, "ImplRepository::Notify: where does this come from?" );

        for (   BasicManagerStore::iterator loop = m_aStore.begin();
                loop != m_aStore.end();
                ++loop
            )
        {
            if ( loop->second == pManager )
            {
                // a BasicManager which is still in our repository is being deleted.
                // That's bad, since by definition, we *own* all instances in our
                // repository.
                OSL_ENSURE( false, "ImplRepository::Notify: nobody should tamper with the managers, except ourself!" );
                m_aStore.erase( loop );
                break;
            }
        }
    }

	//====================================================================
	//= BasicManagerRepository
	//====================================================================
	//--------------------------------------------------------------------
    BasicManager* BasicManagerRepository::getDocumentBasicManager( const Reference< XModel >& _rxDocumentModel )
    {
        return ImplRepository::Instance().getDocumentBasicManager( _rxDocumentModel );
    }

	//--------------------------------------------------------------------
    BasicManager* BasicManagerRepository::getApplicationBasicManager( bool _bCreate )
    {
        return ImplRepository::Instance().getApplicationBasicManager( _bCreate );
    }

	//--------------------------------------------------------------------
    void BasicManagerRepository::resetApplicationBasicManager()
    {
        return ImplRepository::Instance().setApplicationBasicManager( NULL );
    }

	//--------------------------------------------------------------------
    void BasicManagerRepository::registerCreationListener( BasicManagerCreationListener& _rListener )
    {
        ImplRepository::Instance().registerCreationListener( _rListener );
    }

	//--------------------------------------------------------------------
    void BasicManagerRepository::revokeCreationListener( BasicManagerCreationListener& _rListener )
    {
        ImplRepository::Instance().revokeCreationListener( _rListener );
    }

//........................................................................
} // namespace basic
//........................................................................

