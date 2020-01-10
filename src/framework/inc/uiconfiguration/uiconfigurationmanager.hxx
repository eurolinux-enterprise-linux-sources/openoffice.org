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

#ifndef __FRAMEWORK_UICONFIGURATION_UICONFIGMANAGER_HXX_
#define __FRAMEWORK_UICONFIGURATION_UICONFIGMANAGER_HXX_


/** Attention: stl headers must(!) be included at first. Otherwhise it can make trouble
               with solaris headers ...
*/
#include <vector>
#include <list>
#include <hash_map>

//_________________________________________________________________________________________________________________
//	my own includes
//_________________________________________________________________________________________________________________
#include <threadhelp/threadhelpbase.hxx>
#include <macros/generic.hxx>
#include <macros/xinterface.hxx>
#include <macros/xtypeprovider.hxx>
#include <macros/xserviceinfo.hxx>
#include <stdtypes.h>
#include <uiconfiguration/imagemanager.hxx>

//_________________________________________________________________________________________________________________
//	interface includes
//_________________________________________________________________________________________________________________
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XTypeProvider.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/ui/XUIConfiguration.hpp>
#include <com/sun/star/ui/XUIConfigurationPersistence.hpp>

#ifndef _COM_SUN_STAR_UI_XUICONFIGURATIONSTORGAE_HPP_
#include <com/sun/star/ui/XUIConfigurationStorage.hpp>
#endif
#include <com/sun/star/ui/XUIConfigurationManager.hpp>
#include <com/sun/star/ui/ConfigurationEvent.hpp>
#include <com/sun/star/ui/UIElementType.hpp>
#include <com/sun/star/container/XIndexContainer.hpp>

//_________________________________________________________________________________________________________________
//	other includes
//_________________________________________________________________________________________________________________
#include <cppuhelper/weak.hxx>
#include <cppuhelper/interfacecontainer.hxx>
#include <rtl/ustring.hxx>


namespace framework
{
    class UIConfigurationManager :   public com::sun::star::lang::XTypeProvider	,
                                     public com::sun::star::lang::XServiceInfo	,
                                     public com::sun::star::lang::XComponent    ,
                                     public ::com::sun::star::ui::XUIConfiguration             ,
                                     public ::com::sun::star::ui::XUIConfigurationManager      ,
                                     public ::com::sun::star::ui::XUIConfigurationPersistence  ,
                                     public ::com::sun::star::ui::XUIConfigurationStorage      ,
                                     private ThreadHelpBase						,	// Struct for right initalization of mutex member! Must be first of baseclasses.
							         public ::cppu::OWeakObject
    {
        public:
            //  XInterface, XTypeProvider, XServiceInfo
            FWK_DECLARE_XINTERFACE
            FWK_DECLARE_XTYPEPROVIDER
            DECLARE_XSERVICEINFO

            UIConfigurationManager( com::sun::star::uno::Reference< com::sun::star::lang::XMultiServiceFactory > xServiceManager );
            virtual ~UIConfigurationManager();

            // XComponent
            virtual void SAL_CALL dispose() throw (::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL addEventListener( const ::com::sun::star::uno::Reference< ::com::sun::star::lang::XEventListener >& xListener ) throw (::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL removeEventListener( const ::com::sun::star::uno::Reference< ::com::sun::star::lang::XEventListener >& aListener ) throw (::com::sun::star::uno::RuntimeException);

            // XUIConfiguration
            virtual void SAL_CALL addConfigurationListener( const ::com::sun::star::uno::Reference< ::com::sun::star::ui::XUIConfigurationListener >& Listener ) throw (::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL removeConfigurationListener( const ::com::sun::star::uno::Reference< ::com::sun::star::ui::XUIConfigurationListener >& Listener ) throw (::com::sun::star::uno::RuntimeException);

            // XUIConfigurationManager
            virtual void SAL_CALL reset() throw (::com::sun::star::uno::RuntimeException);
            virtual ::com::sun::star::uno::Sequence< ::com::sun::star::uno::Sequence< ::com::sun::star::beans::PropertyValue > > SAL_CALL getUIElementsInfo( sal_Int16 ElementType ) throw (::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::uno::RuntimeException);
            virtual ::com::sun::star::uno::Reference< ::com::sun::star::container::XIndexContainer > SAL_CALL createSettings(  ) throw (::com::sun::star::uno::RuntimeException);
            virtual sal_Bool SAL_CALL hasSettings( const ::rtl::OUString& ResourceURL ) throw (::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::uno::RuntimeException);
            virtual ::com::sun::star::uno::Reference< ::com::sun::star::container::XIndexAccess > SAL_CALL getSettings( const ::rtl::OUString& ResourceURL, sal_Bool bWriteable ) throw (::com::sun::star::container::NoSuchElementException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL replaceSettings( const ::rtl::OUString& ResourceURL, const ::com::sun::star::uno::Reference< ::com::sun::star::container::XIndexAccess >& aNewData ) throw (::com::sun::star::container::NoSuchElementException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::lang::IllegalAccessException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL removeSettings( const ::rtl::OUString& ResourceURL ) throw (::com::sun::star::container::NoSuchElementException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::lang::IllegalAccessException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL insertSettings( const ::rtl::OUString& NewResourceURL, const ::com::sun::star::uno::Reference< ::com::sun::star::container::XIndexAccess >& aNewData ) throw (::com::sun::star::container::ElementExistException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::lang::IllegalAccessException, ::com::sun::star::uno::RuntimeException);
            virtual ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > SAL_CALL getImageManager() throw (::com::sun::star::uno::RuntimeException);
            virtual ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > SAL_CALL getShortCutManager() throw (::com::sun::star::uno::RuntimeException);
            virtual ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > SAL_CALL getEventsManager() throw (::com::sun::star::uno::RuntimeException);

            // XUIConfigurationPersistence
            virtual void SAL_CALL reload() throw (::com::sun::star::uno::Exception, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL store() throw (::com::sun::star::uno::Exception, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL storeToStorage( const ::com::sun::star::uno::Reference< ::com::sun::star::embed::XStorage >& Storage ) throw (::com::sun::star::uno::Exception, ::com::sun::star::uno::RuntimeException);
            virtual sal_Bool SAL_CALL isModified() throw (::com::sun::star::uno::RuntimeException);
            virtual sal_Bool SAL_CALL isReadOnly() throw (::com::sun::star::uno::RuntimeException);

            // XUIConfigurationStorage
            virtual void SAL_CALL setStorage( const ::com::sun::star::uno::Reference< ::com::sun::star::embed::XStorage >& Storage ) throw (::com::sun::star::uno::RuntimeException);
            virtual sal_Bool SAL_CALL hasStorage() throw (::com::sun::star::uno::RuntimeException);

        private:
            // private data types
            enum NotifyOp
            {
                NotifyOp_Remove,
                NotifyOp_Insert,
                NotifyOp_Replace
            };

            struct UIElementInfo
            {
                UIElementInfo( const rtl::OUString& rResourceURL, const rtl::OUString& rUIName ) :
                    aResourceURL( rResourceURL), aUIName( rUIName ) {}
                rtl::OUString   aResourceURL;
                rtl::OUString   aUIName;
            };
            
            struct UIElementData
            {
                UIElementData() : bModified( false ), bDefault( true ) {};

                rtl::OUString aResourceURL;
                rtl::OUString aName;
                bool          bModified;        // has been changed since last storing
                bool          bDefault;         // default settings
                com::sun::star::uno::Reference< com::sun::star::container::XIndexAccess > xSettings;
            };

		    struct UIElementType;
            friend struct UIElementType;
		    typedef ::std::hash_map< rtl::OUString, UIElementData, OUStringHashCode, ::std::equal_to< rtl::OUString > > UIElementDataHashMap;
            
            struct UIElementType
            {
                UIElementType() : bModified( false ), 
                                  bLoaded( false ), 
                                  bDefaultLayer( false ),
                                  nElementType( ::com::sun::star::ui::UIElementType::UNKNOWN ) {}
                

                bool                                                              bModified;
                bool                                                              bLoaded;
                bool                                                              bDefaultLayer;
                sal_Int16                                                         nElementType;
                UIElementDataHashMap                                              aElementsHashMap;
                com::sun::star::uno::Reference< com::sun::star::embed::XStorage > xStorage;
            };

            typedef ::std::vector< UIElementType > UIElementTypesVector;
            typedef ::std::vector< ::com::sun::star::ui::ConfigurationEvent > ConfigEventNotifyContainer;
            typedef ::std::hash_map< rtl::OUString, UIElementInfo, OUStringHashCode, ::std::equal_to< rtl::OUString > > UIElementInfoHashMap;

            // private methods
            void            impl_Initialize();
            void            implts_notifyContainerListener( const ::com::sun::star::ui::ConfigurationEvent& aEvent, NotifyOp eOp );
            void            impl_fillSequenceWithElementTypeInfo( UIElementInfoHashMap& aUIElementInfoCollection, sal_Int16 nElementType );
            void            impl_preloadUIElementTypeList( sal_Int16 nElementType );
            UIElementData*  impl_findUIElementData( const rtl::OUString& aResourceURL, sal_Int16 nElementType, bool bLoad = true );
            void            impl_requestUIElementData( sal_Int16 nElementType, UIElementData& aUIElementData );
            void            impl_storeElementTypeData( com::sun::star::uno::Reference< com::sun::star::embed::XStorage >& xStorage, UIElementType& rElementType, bool bResetModifyState = true );
            void            impl_resetElementTypeData( UIElementType& rDocElementType, ConfigEventNotifyContainer& rRemoveNotifyContainer );
            void            impl_reloadElementTypeData( UIElementType& rDocElementType, ConfigEventNotifyContainer& rRemoveNotifyContainer, ConfigEventNotifyContainer& rReplaceNotifyContainer );

            UIElementTypesVector                                                            m_aUIElements;
            com::sun::star::uno::Reference< com::sun::star::embed::XStorage >               m_xDocConfigStorage;
            bool                                                                            m_bReadOnly;
            bool                                                                            m_bInitialized;
            bool                                                                            m_bModified;
            bool                                                                            m_bConfigRead;
            bool                                                                            m_bDisposed;
            rtl::OUString                                                                   m_aXMLPostfix;
            rtl::OUString                                                                   m_aPropUIName;
            rtl::OUString                                                                   m_aPropResourceURL;
            rtl::OUString                                                                   m_aModuleIdentifier;
            com::sun::star::uno::Reference< com::sun::star::lang::XMultiServiceFactory >    m_xServiceManager;
            ::cppu::OMultiTypeInterfaceContainerHelper                                      m_aListenerContainer;   /// container for ALL Listener
            com::sun::star::uno::Reference< com::sun::star::lang::XComponent >              m_xImageManager;
            com::sun::star::uno::Reference< com::sun::star::uno::XInterface >               m_xAccConfig;
   };
}

#endif // __FRAMEWORK_UICONFIGURATION_UICONFIGMANAGER_HXX_
