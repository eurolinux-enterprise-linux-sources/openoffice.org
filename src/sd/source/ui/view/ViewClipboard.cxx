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

#include "ViewClipboard.hxx"

#include "DrawDocShell.hxx"
#include "View.hxx"
#include "ViewShell.hxx"
#include "Window.hxx"

#include "drawdoc.hxx"
#include "sdpage.hxx"
#include "sdxfer.hxx"
#include "sdresid.hxx"
#include "glob.hrc"

#include <svx/svdpagv.hxx>
#include <vos/mutex.hxx>
#include <vcl/svapp.hxx>

namespace sd {

ViewClipboard::ViewClipboard (::sd::View& rView)
    : mrView(rView)
{
}




ViewClipboard::~ViewClipboard (void)
{
}




void ViewClipboard::HandlePageDrop (const SdTransferable& rTransferable)
{
    // Determine whether to insert the given set of slides or to assign a
    // given master page.
    SdPage* pMasterPage = GetFirstMasterPage (rTransferable);
    if (pMasterPage != NULL)
        AssignMasterPage (rTransferable, pMasterPage);
    else
        InsertSlides (rTransferable, DetermineInsertPosition (rTransferable));
}




SdPage* ViewClipboard::GetFirstMasterPage (const SdTransferable& rTransferable)
{
    SdPage* pFirstMasterPage = NULL;

    if (rTransferable.HasPageBookmarks())
    {
        do
        {
            const List* pBookmarks = &rTransferable.GetPageBookmarks();
            if (pBookmarks == NULL)
                break;
            
            DrawDocShell* pDocShell = rTransferable.GetPageDocShell();
            if (pDocShell == NULL)
                break;

            SdDrawDocument* pDocument = pDocShell->GetDoc();
            if (pDocument == NULL)
                break;

            if (pBookmarks->Count() <= 0)
                break;

            int nBookmarkCount = pBookmarks->Count();
            for (int nIndex=0; nIndex<nBookmarkCount; nIndex++)
            {
                String sName (*(String*) pBookmarks->GetObject(nIndex));
                BOOL bIsMasterPage;
                
                // SdPage* GetMasterSdPage(USHORT nPgNum, PageKind ePgKind);
                // USHORT GetMasterSdPageCount(PageKind ePgKind) const;

                USHORT nBMPage = pDocument->GetPageByName (
                    sName, bIsMasterPage);
                if ( ! bIsMasterPage)
                {
                    // At least one regular slide: return NULL to indicate
                    // that not all bookmarks point to master pages.
                    pFirstMasterPage = NULL;
                    break;
                }
                else if (pFirstMasterPage == NULL)
                {
                    // Remember the first master page for later.
                    if (nBMPage != SDRPAGE_NOTFOUND)
                        pFirstMasterPage = static_cast<SdPage*>(
                            pDocument->GetMasterPage(nBMPage));
                }
            }
        }
        while (false);
    }

    return pFirstMasterPage;
}




void ViewClipboard::AssignMasterPage (
    const SdTransferable& rTransferable,
    SdPage* pMasterPage)
{
    do
    {
        if (pMasterPage == NULL)
            return;

        // Get the target page to which the master page is assigned.
        SdrPageView* pPageView = mrView.GetSdrPageView();
        if (pPageView == NULL)
            break;

        SdPage* pPage = static_cast<SdPage*>(pPageView->GetPage());
        if (pPage == NULL)
            break;

        SdDrawDocument* pDocument = mrView.GetDoc();
        if (pDocument == NULL)
            break;

        if ( ! rTransferable.HasPageBookmarks())
            break;

        DrawDocShell* pDataDocShell = rTransferable.GetPageDocShell();
        if (pDataDocShell == NULL)
            break;

        SdDrawDocument* pSourceDocument = pDataDocShell->GetDoc();
        if (pSourceDocument == NULL)
            break;

        // We have to remove the layout suffix from the layout name which is
        // appended again by SetMasterPage() to the given name.  Don't ask.
        String sLayoutSuffix (RTL_CONSTASCII_STRINGPARAM(SD_LT_SEPARATOR));
        sLayoutSuffix.Append (SdResId(STR_LAYOUT_OUTLINE));
        USHORT nLength = sLayoutSuffix.Len();
        String sLayoutName (pMasterPage->GetLayoutName());
        if (String(sLayoutName, sLayoutName.Len()-nLength, nLength).Equals (
            sLayoutSuffix))
            sLayoutName = String(sLayoutName, 0, sLayoutName.Len()-nLength);
			
        pDocument->SetMasterPage (
            pPage->GetPageNum() / 2,
            sLayoutName,
            pSourceDocument,
            FALSE, // Exchange the master page of only the target page.
            FALSE // Keep unused master pages.
            );
    }
    while (false);
}




USHORT ViewClipboard::DetermineInsertPosition  (
    const SdTransferable& )
{
	SdDrawDocument* pDoc = mrView.GetDoc();
    USHORT nPgCnt = pDoc->GetSdPageCount( PK_STANDARD );

    // Insert position is the behind the last selected page or behind the
    // last page when the selection is empty.
    USHORT nInsertPos = pDoc->GetSdPageCount( PK_STANDARD ) * 2 + 1;
    for( USHORT nPage = 0; nPage < nPgCnt; nPage++ )
    {
        SdPage* pPage = pDoc->GetSdPage( nPage, PK_STANDARD );

        if( pPage->IsSelected() )
            nInsertPos = nPage * 2 + 3;
    }

    return nInsertPos;
}




USHORT ViewClipboard::InsertSlides (
    const SdTransferable& rTransferable,
    USHORT nInsertPosition)
{
	SdDrawDocument* pDoc = mrView.GetDoc();

    USHORT nInsertPgCnt = 0;
    BOOL bMergeMasterPages = !rTransferable.HasSourceDoc( pDoc );

    // Prepare the insertion.
    const List* pBookmarkList;
    DrawDocShell* pDataDocSh;
    if (rTransferable.HasPageBookmarks())
    {
        // When the transferable contains page bookmarks then the referenced
        // pages are inserted.
        pBookmarkList = &rTransferable.GetPageBookmarks();
        pDataDocSh = rTransferable.GetPageDocShell();
        nInsertPgCnt = (USHORT)pBookmarkList->Count();
    }
    else
    {
        // Otherwise all pages of the document of the transferable are
        // inserted.
        SfxObjectShell* pShell = rTransferable.GetDocShell();
        pDataDocSh = (DrawDocShell*) pShell;
        SdDrawDocument* pDataDoc = pDataDocSh->GetDoc();
        pBookmarkList = NULL;
        if (pDataDoc!=NULL && pDataDoc->GetSdPageCount(PK_STANDARD))
            nInsertPgCnt = pDataDoc->GetSdPageCount(PK_STANDARD);
    }
    if (nInsertPgCnt > 0)
    {
        const ::vos::OGuard aGuard( Application::GetSolarMutex() );
        ::sd::Window* pWin = mrView.GetViewShell()->GetActiveWindow();
        const BOOL bWait = pWin && pWin->IsWait();

        if( bWait )
            pWin->LeaveWait();
		        
        pDoc->InsertBookmarkAsPage(
            const_cast<List*>(pBookmarkList), 
            NULL, 
            FALSE, 
            FALSE, 
            nInsertPosition,
            (&rTransferable == SD_MOD()->pTransferDrag),
            pDataDocSh, 
            TRUE, 
            bMergeMasterPages, 
            FALSE);

        if( bWait )
            pWin->EnterWait();
    }
    
    return nInsertPgCnt;
}


} // end of namespace ::sd
