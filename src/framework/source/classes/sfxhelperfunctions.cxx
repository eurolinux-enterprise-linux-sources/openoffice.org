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

#ifndef __FRAMEWORK_CLASSES_SFXHELPERFUNCTIONS_CXX_
#include <classes/sfxhelperfunctions.hxx>
#endif

static pfunc_setToolBoxControllerCreator   pToolBoxControllerCreator   = NULL;
static pfunc_setStatusBarControllerCreator pStatusBarControllerCreator = NULL;
static pfunc_getRefreshToolbars	           pRefreshToolbars            = NULL;
static pfunc_createDockingWindow		   pCreateDockingWindow        = NULL;
static pfunc_isDockingWindowVisible        pIsDockingWindowVisible     = NULL;



using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::frame;

namespace framework
{

pfunc_setToolBoxControllerCreator SAL_CALL SetToolBoxControllerCreator( pfunc_setToolBoxControllerCreator pSetToolBoxControllerCreator )
{
	::osl::MutexGuard aGuard( ::osl::Mutex::getGlobalMutex() );
    pfunc_setToolBoxControllerCreator pOldSetToolBoxControllerCreator = pToolBoxControllerCreator;
    pToolBoxControllerCreator = pSetToolBoxControllerCreator;
    return pOldSetToolBoxControllerCreator;
}

svt::ToolboxController* SAL_CALL CreateToolBoxController( const Reference< XFrame >& rFrame, ToolBox* pToolbox, unsigned short nID, const ::rtl::OUString& aCommandURL )
{
	pfunc_setToolBoxControllerCreator pFactory = NULL;
	{
		::osl::MutexGuard aGuard( ::osl::Mutex::getGlobalMutex() );
		pFactory = pToolBoxControllerCreator;
	}
	
	if ( pFactory )
        return (*pFactory)( rFrame, pToolbox, nID, aCommandURL );
    else
        return NULL;
}

pfunc_setStatusBarControllerCreator SAL_CALL SetStatusBarControllerCreator( pfunc_setStatusBarControllerCreator pSetStatusBarControllerCreator )
{
	::osl::MutexGuard aGuard( ::osl::Mutex::getGlobalMutex() );
    pfunc_setStatusBarControllerCreator pOldSetStatusBarControllerCreator = pSetStatusBarControllerCreator;
    pStatusBarControllerCreator = pSetStatusBarControllerCreator;
    return pOldSetStatusBarControllerCreator;
}

svt::StatusbarController* SAL_CALL CreateStatusBarController( const Reference< XFrame >& rFrame, StatusBar* pStatusBar, unsigned short nID, const ::rtl::OUString& aCommandURL )
{
	pfunc_setStatusBarControllerCreator pFactory = NULL;
	{
		::osl::MutexGuard aGuard( ::osl::Mutex::getGlobalMutex() );
		pFactory = pStatusBarControllerCreator;
	}
    
	if ( pFactory )
        return (*pFactory)( rFrame, pStatusBar, nID, aCommandURL );
    else
        return NULL;
}

pfunc_getRefreshToolbars SAL_CALL SetRefreshToolbars( pfunc_getRefreshToolbars pNewRefreshToolbarsFunc )
{
	::osl::MutexGuard aGuard( ::osl::Mutex::getGlobalMutex() );
	pfunc_getRefreshToolbars pOldFunc = pRefreshToolbars;
	pRefreshToolbars = pNewRefreshToolbarsFunc;

	return pOldFunc;
}

void SAL_CALL RefreshToolbars( ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& rFrame )
{
	pfunc_getRefreshToolbars pCallback = NULL;
	{
		::osl::MutexGuard aGuard( ::osl::Mutex::getGlobalMutex() );
		pCallback = pRefreshToolbars;
	}
	
	if ( pCallback )
        (*pCallback)( rFrame );
}

pfunc_createDockingWindow SAL_CALL SetDockingWindowCreator( pfunc_createDockingWindow pNewCreateDockingWindow )
{
	::osl::MutexGuard aGuard( ::osl::Mutex::getGlobalMutex() );
	pfunc_createDockingWindow pOldFunc = pCreateDockingWindow;
	pCreateDockingWindow = pNewCreateDockingWindow;

	return pOldFunc;
}

void SAL_CALL CreateDockingWindow( const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& rFrame, const ::rtl::OUString& rResourceURL )
{
	pfunc_createDockingWindow pFactory = NULL;
	{
		::osl::MutexGuard aGuard( ::osl::Mutex::getGlobalMutex() );
		pFactory = pCreateDockingWindow;
	}
	
	if ( pFactory )
		(*pFactory)( rFrame, rResourceURL );
}

pfunc_isDockingWindowVisible SAL_CALL SetIsDockingWindowVisible( pfunc_isDockingWindowVisible pNewIsDockingWindowVisible)
{
	::osl::MutexGuard aGuard( ::osl::Mutex::getGlobalMutex() );
	pfunc_isDockingWindowVisible pOldFunc = pIsDockingWindowVisible;
	pIsDockingWindowVisible = pNewIsDockingWindowVisible;

	return pOldFunc;
}

bool SAL_CALL IsDockingWindowVisible( const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& rFrame, const ::rtl::OUString& rResourceURL )
{
	pfunc_isDockingWindowVisible pCall = NULL;
	{
		::osl::MutexGuard aGuard( ::osl::Mutex::getGlobalMutex() );
		pCall = pIsDockingWindowVisible;
	}
	
	if ( pIsDockingWindowVisible )
		return (*pIsDockingWindowVisible)( rFrame, rResourceURL );
    else
        return false;
}

}
