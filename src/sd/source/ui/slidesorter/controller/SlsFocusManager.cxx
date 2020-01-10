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

#include "precompiled_sd.hxx"
#include "controller/SlsFocusManager.hxx"

#include "SlideSorter.hxx"
#include "PaneDockingWindow.hxx"
#include "controller/SlideSorterController.hxx"
#include "controller/SlsSelectionManager.hxx"
#include "model/SlideSorterModel.hxx"
#include "model/SlsPageDescriptor.hxx"
#include "view/SlideSorterView.hxx"
#include "view/SlsLayouter.hxx"
#include <vcl/toolbox.hxx>

#include "Window.hxx"
#include "sdpage.hxx"

namespace sd { namespace slidesorter { namespace controller {

FocusManager::FocusManager (SlideSorter& rSlideSorter)
    : mrSlideSorter(rSlideSorter),
      mnPageIndex(0),
      mbPageIsFocused(false)
{
    if (mrSlideSorter.GetModel().GetPageCount() > 0)
        mnPageIndex = 0;
}




FocusManager::~FocusManager (void)
{
}




void FocusManager::MoveFocus (FocusMoveDirection eDirection)
{
    if (mnPageIndex >= 0 && mbPageIsFocused)
    {
        HideFocusIndicator (GetFocusedPageDescriptor());

        int nColumnCount (mrSlideSorter.GetView().GetLayouter().GetColumnCount());
        switch (eDirection)
        {
            case FMD_NONE:
                if (mnPageIndex >= mrSlideSorter.GetModel().GetPageCount())
                    mnPageIndex = mrSlideSorter.GetModel().GetPageCount() - 1;
                break;

            case FMD_LEFT:
                mnPageIndex -= 1;
                if (mnPageIndex < 0)
                {
                    mnPageIndex = mrSlideSorter.GetModel().GetPageCount() - 1;
                    SetFocusToToolBox();
                }
                break;
                
            case FMD_RIGHT:
                mnPageIndex += 1;
                if (mnPageIndex >= mrSlideSorter.GetModel().GetPageCount())
                {
                    mnPageIndex = 0;
                    SetFocusToToolBox();
                }
                break;

            case FMD_UP:
            {
                int nColumn = mnPageIndex % nColumnCount;
                mnPageIndex -= nColumnCount;
                if (mnPageIndex < 0)
                {
                    // Wrap arround to the bottom row or the one above and
                    // go to the correct column.
                    int nCandidate = mrSlideSorter.GetModel().GetPageCount()-1;
                    int nCandidateColumn = nCandidate % nColumnCount;
                    if (nCandidateColumn > nColumn)
                        mnPageIndex = nCandidate - (nCandidateColumn-nColumn);
                    else if (nCandidateColumn < nColumn)
                        mnPageIndex = nCandidate 
                            - nColumnCount
                            + (nColumn - nCandidateColumn);
                    else
                        mnPageIndex = nCandidate;
                }
            }
            break;

            case FMD_DOWN:
            {
                int nColumn = mnPageIndex % nColumnCount;
                mnPageIndex += nColumnCount;
                if (mnPageIndex >= mrSlideSorter.GetModel().GetPageCount())
                {
                    // Wrap arround to the correct column.
                    mnPageIndex = nColumn;
                }
            }
            break;
        }

        if (mbPageIsFocused)
            ShowFocusIndicator(GetFocusedPageDescriptor(), true);
    }
}




void FocusManager::ShowFocus (const bool bScrollToFocus)
{
    mbPageIsFocused = true;
    ShowFocusIndicator(GetFocusedPageDescriptor(), bScrollToFocus);
}




void FocusManager::HideFocus (void)
{
    mbPageIsFocused = false;
    HideFocusIndicator(GetFocusedPageDescriptor());
}




bool FocusManager::ToggleFocus (void)
{
    if (mnPageIndex >= 0)
    {
        if (mbPageIsFocused)
            HideFocus ();
        else
            ShowFocus ();
    }
    return mbPageIsFocused;
}




bool FocusManager::HasFocus (void) const
{
    return mrSlideSorter.GetView().GetWindow()->HasFocus();
}




model::SharedPageDescriptor FocusManager::GetFocusedPageDescriptor (void) const
{
    return mrSlideSorter.GetModel().GetPageDescriptor(mnPageIndex);
}




sal_Int32 FocusManager::GetFocusedPageIndex (void) const
{
    return mnPageIndex;
}




void FocusManager::FocusPage (sal_Int32 nPageIndex)
{
    if (nPageIndex != mnPageIndex)
    {
        // Hide the focus while switching it to the specified page.
        FocusHider aHider (*this);
        mnPageIndex = nPageIndex;
    }

    if (HasFocus() && !IsFocusShowing())
        ShowFocus();
}




void FocusManager::SetFocusedPage (const model::SharedPageDescriptor& rpDescriptor)
{
    if (rpDescriptor.get() != NULL)
    {
        FocusHider aFocusHider (*this);
        mnPageIndex = (rpDescriptor->GetPage()->GetPageNum()-1)/2;
    }
}




void FocusManager::SetFocusedPage (sal_Int32 nPageIndex)
{
    FocusHider aFocusHider (*this);
    mnPageIndex = nPageIndex;
}




bool FocusManager::IsFocusShowing (void) const
{
    return HasFocus() && mbPageIsFocused;
}




void FocusManager::HideFocusIndicator (const model::SharedPageDescriptor& rpDescriptor)
{
	if (rpDescriptor.get() != NULL)
	{
	    rpDescriptor->RemoveFocus();
		mrSlideSorter.GetView().RequestRepaint(rpDescriptor);
	}
}




void FocusManager::ShowFocusIndicator (
    const model::SharedPageDescriptor& rpDescriptor,
    const bool bScrollToFocus)
{
    if (rpDescriptor.get() != NULL)
    {
        rpDescriptor->SetFocus ();

        if (bScrollToFocus)
        {
            // Scroll the focused page object into the visible area and repaint
            // it, so that the focus indicator becomes visible.
            view::SlideSorterView& rView (mrSlideSorter.GetView());
            mrSlideSorter.GetController().GetSelectionManager()->MakeRectangleVisible (
                rView.GetPageBoundingBox (
                    GetFocusedPageDescriptor(),
                    view::SlideSorterView::CS_MODEL,
                    view::SlideSorterView::BBT_INFO));
        }

        mrSlideSorter.GetView().RequestRepaint (rpDescriptor);
        NotifyFocusChangeListeners();
    }
}




void FocusManager::AddFocusChangeListener (const Link& rListener)
{
    if (::std::find (maFocusChangeListeners.begin(), maFocusChangeListeners.end(), rListener)
        == maFocusChangeListeners.end())
    {
        maFocusChangeListeners.push_back (rListener);
    }
}




void FocusManager::RemoveFocusChangeListener (const Link& rListener)
{
    maFocusChangeListeners.erase (
        ::std::find (maFocusChangeListeners.begin(), maFocusChangeListeners.end(), rListener));
}




void FocusManager::SetFocusToToolBox (void)
{
    HideFocus();

    if (mrSlideSorter.GetViewShell() != NULL)
    {
        ::Window* pParentWindow = mrSlideSorter.GetViewShell()->GetParentWindow();
        DockingWindow* pDockingWindow = NULL;
        while (pParentWindow!=NULL && pDockingWindow==NULL)
        {
            pDockingWindow = dynamic_cast<DockingWindow*>(pParentWindow);
            pParentWindow = pParentWindow->GetParent();
        }
        if (pDockingWindow)
        {
            PaneDockingWindow* pPaneDockingWindow = dynamic_cast<PaneDockingWindow*>(pDockingWindow);
            if (pPaneDockingWindow != NULL)
                pPaneDockingWindow->GetTitleToolBox()->GrabFocus();
        }
    }
}




void FocusManager::NotifyFocusChangeListeners (void) const
{
    // Create a copy of the listener list to be safe when that is modified.
    ::std::vector<Link> aListeners (maFocusChangeListeners);
    
    // Tell the slection change listeners that the selection has changed.
    ::std::vector<Link>::iterator iListener (aListeners.begin()); 
    ::std::vector<Link>::iterator iEnd (aListeners.end());
    for (; iListener!=iEnd; ++iListener)
    {
        iListener->Call(NULL);
    }
}




FocusManager::FocusHider::FocusHider (FocusManager& rManager)
: mbFocusVisible(rManager.IsFocusShowing())
, mrManager(rManager)
{
    mrManager.HideFocus();
}




FocusManager::FocusHider::~FocusHider (void)
{
    if (mbFocusVisible)
        mrManager.ShowFocus();
}

} } } // end of namespace ::sd::slidesorter::controller

