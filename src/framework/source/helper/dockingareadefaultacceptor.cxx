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
#include "precompiled_framework.hxx"

//_________________________________________________________________________________________________________________
//	my own includes
//_________________________________________________________________________________________________________________
#include <helper/dockingareadefaultacceptor.hxx>
#include <threadhelp/resetableguard.hxx>

//_________________________________________________________________________________________________________________
//	interface includes
//_________________________________________________________________________________________________________________
#include <com/sun/star/awt/XDevice.hpp>
#include <com/sun/star/awt/PosSize.hpp>
#include <com/sun/star/awt/XLayoutConstrains.hpp>

//_________________________________________________________________________________________________________________
//	includes of other projects
//_________________________________________________________________________________________________________________

#include <vcl/svapp.hxx>

//_________________________________________________________________________________________________________________
//	namespace
//_________________________________________________________________________________________________________________

namespace framework{

using namespace ::com::sun::star::container		;
using namespace ::com::sun::star::frame			;
using namespace ::com::sun::star::lang			;
using namespace ::com::sun::star::uno			;
using namespace ::cppu							;
using namespace ::osl							;
using namespace ::rtl							;

//_________________________________________________________________________________________________________________
//	non exported const
//_________________________________________________________________________________________________________________

//_________________________________________________________________________________________________________________
//	non exported definitions
//_________________________________________________________________________________________________________________

//_________________________________________________________________________________________________________________
//	declarations
//_________________________________________________________________________________________________________________

//*****************************************************************************************************************
//	constructor
//*****************************************************************************************************************
DockingAreaDefaultAcceptor::DockingAreaDefaultAcceptor(	const	Reference< XFrame >&		xOwner	)
		//	Init baseclasses first
		:	ThreadHelpBase  ( &Application::GetSolarMutex() )
		// Init member
		,	m_xOwner		( xOwner	)
{
}

//*****************************************************************************************************************
//	destructor
//*****************************************************************************************************************
DockingAreaDefaultAcceptor::~DockingAreaDefaultAcceptor()
{
}

//*****************************************************************************************************************
//	XDockingAreaAcceptor
//*****************************************************************************************************************
css::uno::Reference< css::awt::XWindow > SAL_CALL DockingAreaDefaultAcceptor::getContainerWindow() throw (css::uno::RuntimeException)
{
	// Ready for multithreading
	ResetableGuard aGuard( m_aLock );

	// Try to "lock" the frame for access to taskscontainer.
	Reference< XFrame > xFrame( m_xOwner.get(), UNO_QUERY );
    Reference< css::awt::XWindow > xContainerWindow( xFrame->getContainerWindow() );

    return xContainerWindow;
}

sal_Bool SAL_CALL DockingAreaDefaultAcceptor::requestDockingAreaSpace( const css::awt::Rectangle& RequestedSpace ) throw (css::uno::RuntimeException)
{
	// Ready for multithreading
	ResetableGuard aGuard( m_aLock );

	// Try to "lock" the frame for access to taskscontainer.
	Reference< XFrame > xFrame( m_xOwner.get(), UNO_QUERY );
    aGuard.unlock();
    
	if ( xFrame.is() == sal_True )
	{
        Reference< css::awt::XWindow > xContainerWindow( xFrame->getContainerWindow() );
        Reference< css::awt::XWindow > xComponentWindow( xFrame->getComponentWindow() );

        if (( xContainerWindow.is() == sal_True ) &&
            ( xComponentWindow.is() == sal_True )       )
        {
            css::uno::Reference< css::awt::XDevice > xDevice( xContainerWindow, css::uno::UNO_QUERY );
            // Convert relativ size to output size.
            css::awt::Rectangle  aRectangle  = xContainerWindow->getPosSize();
            css::awt::DeviceInfo aInfo       = xDevice->getInfo();
            css::awt::Size       aSize       (  aRectangle.Width  - aInfo.LeftInset - aInfo.RightInset  ,
                                                aRectangle.Height - aInfo.TopInset  - aInfo.BottomInset );

            // client size of container window
//            css::uno::Reference< css::awt::XLayoutConstrains > xLayoutContrains( xComponentWindow, css::uno::UNO_QUERY );
            css::awt::Size aMinSize( 0, 0 ); // = xLayoutContrains->getMinimumSize();

            // Check if request border space would decrease component window size below minimum size
            if ((( aSize.Width - RequestedSpace.X - RequestedSpace.Width ) < aMinSize.Width ) ||
                (( aSize.Height - RequestedSpace.Y - RequestedSpace.Height ) < aMinSize.Height  )       )
                return sal_False;

            return sal_True;
        }
    }

    return sal_False;
}

void SAL_CALL DockingAreaDefaultAcceptor::setDockingAreaSpace( const css::awt::Rectangle& BorderSpace ) throw (css::uno::RuntimeException)
{
	// Ready for multithreading
	ResetableGuard aGuard( m_aLock );

	// Try to "lock" the frame for access to taskscontainer.
	Reference< XFrame > xFrame( m_xOwner.get(), UNO_QUERY );
	if ( xFrame.is() == sal_True )
	{
        Reference< css::awt::XWindow > xContainerWindow( xFrame->getContainerWindow() );
        Reference< css::awt::XWindow > xComponentWindow( xFrame->getComponentWindow() );

        if (( xContainerWindow.is() == sal_True ) &&
            ( xComponentWindow.is() == sal_True )       )
        {
            css::uno::Reference< css::awt::XDevice > xDevice( xContainerWindow, css::uno::UNO_QUERY );
            // Convert relativ size to output size.
            css::awt::Rectangle  aRectangle  = xContainerWindow->getPosSize();
            css::awt::DeviceInfo aInfo       = xDevice->getInfo();
            css::awt::Size       aSize       (  aRectangle.Width  - aInfo.LeftInset - aInfo.RightInset  ,
                                                aRectangle.Height - aInfo.TopInset  - aInfo.BottomInset );
            // client size of container window
//            css::uno::Reference< css::awt::XLayoutConstrains > xLayoutContrains( xComponentWindow, css::uno::UNO_QUERY );
            css::awt::Size aMinSize( 0, 0 );// = xLayoutContrains->getMinimumSize();

            // Check if request border space would decrease component window size below minimum size
            sal_Int32 nWidth     = aSize.Width - BorderSpace.X - BorderSpace.Width;
            sal_Int32 nHeight    = aSize.Height - BorderSpace.Y - BorderSpace.Height;

            if (( nWidth > aMinSize.Width ) && ( nHeight > aMinSize.Height ))
            {
                // Resize our component window.
                xComponentWindow->setPosSize( BorderSpace.X, BorderSpace.Y, nWidth, nHeight, css::awt::PosSize::POSSIZE );
            }
        }
    }
}

} // namespace framework
