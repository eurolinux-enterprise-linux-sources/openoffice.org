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
#include "precompiled_sd.hxx"

#include "ViewShell.hxx"
#include "smarttag.hxx"
#include "Window.hxx"
#include "View.hxx"

namespace sd
{

// ====================================================================

SmartTag::SmartTag( ::sd::View& rView )
: mrView( rView )
, mbSelected( false )
{
	SmartTagReference xThis( this );
	mrView.getSmartTags().add( xThis );
}

// --------------------------------------------------------------------

SmartTag::~SmartTag()
{
}

// --------------------------------------------------------------------

bool SmartTag::MouseButtonDown( const MouseEvent&, SmartHdl&  )
{
	return false;
}

/** returns true if the SmartTag consumes this event. */
bool SmartTag::KeyInput( const KeyEvent& /*rKEvt*/ )
{
	return false;
}

/** returns true if the SmartTag consumes this event. */
bool SmartTag::RequestHelp( const HelpEvent& /*rHEvt*/ )
{
    return false;
}

/** returns true if the SmartTag consumes this event. */
bool SmartTag::Command( const CommandEvent& /*rCEvt*/ )
{
    return false;
}

// --------------------------------------------------------------------

void SmartTag::addCustomHandles( SdrHdlList& /*rHandlerList*/ )
{
}

// --------------------------------------------------------------------

void SmartTag::select()
{
	mbSelected = true;
}

// --------------------------------------------------------------------

void SmartTag::deselect()
{
	mbSelected = false;
}

// --------------------------------------------------------------------

bool SmartTag::isSelected() const
{
	return mbSelected;
}

// --------------------------------------------------------------------

void SmartTag::disposing()
{
	SmartTagReference xThis( this );
	mrView.getSmartTags().remove( xThis );
}

// --------------------------------------------------------------------

bool SmartTag::getContext( SdrViewContext& /*rContext*/ )
{
	return false;
}

// --------------------------------------------------------------------

ULONG SmartTag::GetMarkablePointCount() const
{
	return 0;
}

// --------------------------------------------------------------------

ULONG SmartTag::GetMarkedPointCount() const
{
	return 0;
}

// --------------------------------------------------------------------

BOOL SmartTag::MarkPoint(SdrHdl& /*rHdl*/, BOOL /*bUnmark*/ )
{
	return FALSE;
}

// --------------------------------------------------------------------

BOOL SmartTag::MarkPoints(const Rectangle* /*pRect*/, BOOL /*bUnmark*/ )
{
	return FALSE;
}

// --------------------------------------------------------------------

void SmartTag::CheckPossibilities()
{
}

// ====================================================================

SmartTagSet::SmartTagSet( View& rView )
: mrView( rView )
{
}

// --------------------------------------------------------------------

SmartTagSet::~SmartTagSet()
{
}

// --------------------------------------------------------------------

void SmartTagSet::add( const SmartTagReference& xTag )
{
	maSet.insert( xTag );
	mrView.InvalidateAllWin();
}

// --------------------------------------------------------------------

void SmartTagSet::remove( const SmartTagReference& xTag )
{
	std::set< SmartTagReference >::iterator aIter( maSet.find( xTag ) );
	if( aIter != maSet.end() )
		maSet.erase( aIter );
	mrView.InvalidateAllWin();
}

// --------------------------------------------------------------------

void SmartTagSet::Dispose()
{
	std::set< SmartTagReference > aSet;
	aSet.swap( maSet );
	for( std::set< SmartTagReference >::iterator aIter( aSet.begin() ); aIter != aSet.end(); )
		(*aIter++)->Dispose();
	mrView.InvalidateAllWin();
}

// --------------------------------------------------------------------

void SmartTagSet::select( const SmartTagReference& xTag )
{
	if( mxSelectedTag != xTag )
	{
		if( mxSelectedTag.is() )
			mxSelectedTag->deselect();

		mxSelectedTag = xTag;
		mxSelectedTag->select();
		mrView.SetPossibilitiesDirty();
		if( mrView.GetMarkedObjectCount() > 0 )
			mrView.UnmarkAllObj();
		else
			mrView.updateHandles();
	}
}

// --------------------------------------------------------------------

void SmartTagSet::deselect()
{
	if( mxSelectedTag.is() )
	{
		mxSelectedTag->deselect();
		mxSelectedTag.clear();
		mrView.SetPossibilitiesDirty();
		mrView.updateHandles();
	}
}

// --------------------------------------------------------------------

bool SmartTagSet::MouseButtonDown( const MouseEvent& rMEvt )
{
	Point aMDPos( mrView.GetViewShell()->GetActiveWindow()->PixelToLogic( rMEvt.GetPosPixel() ) );
	SdrHdl* pHdl = mrView.PickHandle(aMDPos);

	// check if a smart tag is selected and no handle is hit
	if( mxSelectedTag.is() && !pHdl )
	{
		// deselect smart tag
		deselect();
		return false;
	}

	// if a smart tag handle is hit, foreward event to its smart tag
	SmartHdl* pSmartHdl = dynamic_cast< SmartHdl* >( pHdl );
	if(pSmartHdl && pSmartHdl->getTag().is() )
	{
		SmartTagReference xTag( pSmartHdl->getTag() );
		return xTag->MouseButtonDown( rMEvt, *pSmartHdl );
	}

	return false;
}

// --------------------------------------------------------------------

bool SmartTagSet::KeyInput( const KeyEvent& rKEvt )
{
	if( mxSelectedTag.is() )
		return mxSelectedTag->KeyInput( rKEvt );
	else if( rKEvt.GetKeyCode().GetCode() == KEY_SPACE )
	{
	    SmartHdl* pSmartHdl = dynamic_cast< SmartHdl* >( mrView.GetHdlList().GetFocusHdl() );
	    if( pSmartHdl )
	    {
	        const_cast< SdrHdlList& >( mrView.GetHdlList() ).ResetFocusHdl();
    		SmartTagReference xTag( pSmartHdl->getTag() );
	        select( xTag );
	        return true;
		}
	}
	
	
	return false;
}

// --------------------------------------------------------------------

bool SmartTagSet::RequestHelp( const HelpEvent& rHEvt )
{
	Point aMDPos( mrView.GetViewShell()->GetActiveWindow()->PixelToLogic( rHEvt.GetMousePosPixel() ) );
	SdrHdl* pHdl = mrView.PickHandle(aMDPos);

    if( pHdl )
    {
	    // if a smart tag handle is hit, foreward event to its smart tag
	    SmartHdl* pSmartHdl = dynamic_cast< SmartHdl* >( pHdl );
	    if(pSmartHdl && pSmartHdl->getTag().is() )
	    {
		    SmartTagReference xTag( pSmartHdl->getTag() );
		    return xTag->RequestHelp( rHEvt );
	    }
	}

	return false;
}

// --------------------------------------------------------------------

/** returns true if the SmartTag consumes this event. */
bool SmartTagSet::Command( const CommandEvent& rCEvt )
{
    if( rCEvt.IsMouseEvent() )
    {
	    Point aMDPos( mrView.GetViewShell()->GetActiveWindow()->PixelToLogic( rCEvt.GetMousePosPixel() ) );
	    SdrHdl* pHdl = mrView.PickHandle(aMDPos);

        if( pHdl )
        {
	        // if a smart tag handle is hit, foreward event to its smart tag
	        SmartHdl* pSmartHdl = dynamic_cast< SmartHdl* >( pHdl );
	        if(pSmartHdl && pSmartHdl->getTag().is() )
	        {
		        SmartTagReference xTag( pSmartHdl->getTag() );
		        return xTag->Command( rCEvt );
	        }
	    }
    }
    else
    {
    	if( mxSelectedTag.is() )
	    	return mxSelectedTag->Command( rCEvt );
    
    }

    return false;
}

// --------------------------------------------------------------------

void SmartTagSet::addCustomHandles( SdrHdlList& rHandlerList )
{
	if( !maSet.empty() )
	{
		for( std::set< SmartTagReference >::iterator aIter( maSet.begin() ); aIter != maSet.end(); )
			(*aIter++)->addCustomHandles( rHandlerList );
	}
}

// --------------------------------------------------------------------

/** returns true if the currently selected smart tag has
	a special context, returned in rContext. */
bool SmartTagSet::getContext( SdrViewContext& rContext ) const
{
	if( mxSelectedTag.is() )
		return mxSelectedTag->getContext( rContext );
	else
		return false;
}

// --------------------------------------------------------------------
// support point editing
// --------------------------------------------------------------------

BOOL SmartTagSet::HasMarkablePoints() const
{
	return GetMarkablePointCount() != 0 ? TRUE : FALSE;
}

// --------------------------------------------------------------------

ULONG SmartTagSet::GetMarkablePointCount() const
{
	if( mxSelectedTag.is() )
		return mxSelectedTag->GetMarkablePointCount();
	return 0;
}

// --------------------------------------------------------------------

BOOL SmartTagSet::HasMarkedPoints() const
{
	return GetMarkedPointCount() != 0 ? TRUE : FALSE;
}

// --------------------------------------------------------------------

ULONG SmartTagSet::GetMarkedPointCount() const
{
	if( mxSelectedTag.is() )
		return mxSelectedTag->GetMarkedPointCount();
	else
		return 0;
}

// --------------------------------------------------------------------

BOOL SmartTagSet::IsPointMarkable(const SdrHdl& rHdl) const
{
	const SmartHdl* pSmartHdl = dynamic_cast< const SmartHdl* >( &rHdl );
	
	return pSmartHdl && pSmartHdl->isMarkable();
}

// --------------------------------------------------------------------

BOOL SmartTagSet::MarkPoint(SdrHdl& rHdl, BOOL bUnmark )
{
	if( mxSelectedTag.is() )
		return mxSelectedTag->MarkPoint( rHdl, bUnmark );

	return FALSE;
}

// --------------------------------------------------------------------

BOOL SmartTagSet::MarkPoints(const Rectangle* pRect, BOOL bUnmark)
{
	if( mxSelectedTag.is() )
		return mxSelectedTag->MarkPoints( pRect, bUnmark );
	return FALSE;
}

// --------------------------------------------------------------------

void SmartTagSet::CheckPossibilities()
{
	if( mxSelectedTag.is() )
		mxSelectedTag->CheckPossibilities();
}

// ====================================================================

SmartHdl::SmartHdl( const SmartTagReference& xTag, SdrObject* pObject, const Point& rPnt, SdrHdlKind eNewKind /*=HDL_MOVE*/ )
: SdrHdl( rPnt, eNewKind )
, mxTag( xTag )
{
	SetObj( pObject );
}

// --------------------------------------------------------------------

SmartHdl::SmartHdl( const SmartTagReference& xTag, const Point& rPnt, SdrHdlKind eNewKind /*=HDL_MOVE*/ )
: SdrHdl( rPnt, eNewKind )
, mxTag( xTag )
{
} 

// --------------------------------------------------------------------

bool SmartHdl::isMarkable() const
{
	return false;
}

// ====================================================================

/*
SmartProxyHdl::SmartProxyHdl( const SmartTagReference& xTag, SdrHdl* pProxyHdl )
: SmartHdl( xTag, pProxyHdl->GetPos(), pProxyHdl->GetKind() )
, mpProxyHdl( pProxyHdl )
{
}

// --------------------------------------------------------------------

SmartProxyHdl::~SmartProxyHdl()
{
	delete mpProxyHdl;
}
*/
} // end of namespace sd

