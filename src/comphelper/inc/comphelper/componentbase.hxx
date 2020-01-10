/*************************************************************************
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

#ifndef COMPHELPER_COMPONENTBASE_HXX
#define COMPHELPER_COMPONENTBASE_HXX

#include "comphelper/comphelperdllapi.h"

/** === begin UNO includes === **/
/** === end UNO includes === **/

#include <cppuhelper/interfacecontainer.hxx>

//........................................................................
namespace comphelper
{
//........................................................................

	//====================================================================
	//= ComponentBase
	//====================================================================
	class COMPHELPER_DLLPUBLIC ComponentBase
	{
    protected:
        /** creates a ComponentBase instance

            The instance is not initialized. As a consequence, every ComponentMethodGuard instantiated for
            this component will throw a <type scope="com::sun::star::lang">NotInitializedException</type>,
            until ->setInitialized() is called.
        */
        ComponentBase( ::cppu::OBroadcastHelper& _rBHelper )
            :m_rBHelper( _rBHelper )
            ,m_bInitialized( false )
        {
        }

        struct NoInitializationNeeded { };

        /** creates a ComponentBase instance

            The instance is already initialized, so there's no need to call setInitialized later on. Use this
            constructor for component implementations which do not require explicit initialization.
        */
        ComponentBase( ::cppu::OBroadcastHelper& _rBHelper, NoInitializationNeeded )
            :m_rBHelper( _rBHelper )
            ,m_bInitialized( true )
        {
        }

        /** marks the instance as initialized

            Subsequent instantiations of a ComponentMethodGuard won't throw the NotInitializedException now.
        */
        inline void setInitialized()    { m_bInitialized = true; }

    public:
        /// helper struct to grant access to selected public methods to the ComponentMethodGuard class
        struct GuardAccess { friend class ComponentMethodGuard; private: GuardAccess() { } };

        /// retrieves the component's mutex
        inline  ::osl::Mutex&   getMutex( GuardAccess )                 { return getMutex(); }
        /// checks whether the component is already disposed, throws a DisposedException if so.
        inline  void            checkDisposed( GuardAccess ) const      { impl_checkDisposed_throw(); }
        /// checks whether the component is already initialized, throws a NotInitializedException if not.
        inline  void            checkInitialized( GuardAccess ) const   { impl_checkInitialized_throw(); }

    protected:
        /// retrieves the component's broadcast helper
        inline  ::cppu::OBroadcastHelper&   getBroadcastHelper()    { return m_rBHelper; }
        /// retrieves the component's mutex
        inline  ::osl::Mutex&               getMutex()              { return m_rBHelper.rMutex; }
        /// determines whether the instance is already disposed
        inline  bool                        impl_isDisposed() const { return m_rBHelper.bDisposed; }

        /// checks whether the component is already disposed. Throws a DisposedException if so.
        void    impl_checkDisposed_throw() const;

        /// checks whether the component is already initialized. Throws a NotInitializedException if not.
        void    impl_checkInitialized_throw() const;

        /// determines whether the component is already initialized
        inline  bool
                impl_isInitialized_nothrow() const { return m_bInitialized; }

        /** returns the context to be used when throwing exceptions

            The default implementation returns <NULL/>.
        */
        virtual ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface >
                getComponent() const;

    private:
        ::cppu::OBroadcastHelper&   m_rBHelper;
        bool                        m_bInitialized;
	};

    class ComponentMethodGuard
    {
    public:
        enum MethodType
        {
            /// allow the method to be called only when being initialized and not being disposed
            Default,
            /// allow the method to be called without being initialized
            WithoutInit

        };

        ComponentMethodGuard( ComponentBase& _rComponent, const MethodType _eType = Default )
            :m_aMutexGuard( _rComponent.getMutex( ComponentBase::GuardAccess() ) )
        {
            if ( _eType != WithoutInit )
                _rComponent.checkInitialized( ComponentBase::GuardAccess() );
            _rComponent.checkDisposed( ComponentBase::GuardAccess() );
        }

        ~ComponentMethodGuard()
        {
        }

        inline void clear()
        {
            m_aMutexGuard.clear();
        }
        inline void reset()
        {
            m_aMutexGuard.reset();
        }

    private:
        ::osl::ResettableMutexGuard   m_aMutexGuard;
    };

//........................................................................
} // namespace ComponentBase
//........................................................................

#endif // COMPHELPER_COMPONENTBASE_HXX
