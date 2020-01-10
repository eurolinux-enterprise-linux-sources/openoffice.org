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
#ifndef SD_PRINT_MANAGER_HXX
#define SD_PRINT_MANAGER_HXX

#include "pres.hxx"
#include <tools/solar.h>
#include <tools/errcode.hxx>
#include <sfx2/viewsh.hxx>

class Font;
class MultiSelection;
class PrintDialog;
class SdOptionsPrintItem;
class SdPage;
class String;
class SfxItemSet;
class SfxPrinter;
class SfxProgress;
class SfxTabPage;
class Window;

namespace sd {

class DrawView;
class View;
class ViewShell;
class ViewShellBase;

/** Manage the printing of documents.
    This includes functions about asking the user for specific
    information what to print as well as high to medium level printing.
*/
class PrintManager
{
public:
    /* Create a new print manager for the given view shell.
        @param rViewShell
            The life time of that view shell has to exceed that of the
            print manager.
    */
    PrintManager (ViewShellBase& rViewShell);
    virtual ~PrintManager (void);

    /// Forwarded to the document shell.
	virtual SfxPrinter* GetPrinter (BOOL bCreate = FALSE);
    /// Forwarded to the main sub-shell.
	virtual USHORT SetPrinter (
        SfxPrinter* pNewPrinter,
        USHORT nDiffFlags = SFX_PRINTER_ALL, bool bIsAPI=false);
    /// Forwarded to the main sub-shell.
	virtual PrintDialog* CreatePrintDialog (::Window *pParent);

    /** Create an options tab page for the curren document.
    */
	virtual SfxTabPage* CreatePrintOptionsPage (
        ::Window *pParent,
        const SfxItemSet &rOptions);

    /** Print the document.
        @param pDialog
            The dialog specifies what to print.
    */
    virtual USHORT Print (SfxProgress& rProgress, BOOL bIsAPI, PrintDialog* pDialog);

    /** Show a dialog that allows the user to select the print range
        (among other things) before starting the actual printing.
    */
	virtual ErrCode DoPrint (
        SfxPrinter *pPrinter,
        PrintDialog *pPrintDialog,
        BOOL bSilent, BOOL bIsAPI );

    /** When called with <TRUE/> then printing is restricted to the
        selected pages.  Otherwise all pages are printed.  Note that
        you have to set the page range with a call to
        SetPrintingPageRange().
    */
    void RestrictPrintingToSelection (bool bFlag);

    /** Set the range of the pages to print.  This is taken into
        account only after a previous call to
        RestrictPrintingToSelection() with <TRUE/> as argument.
    */
    void SetPrintingPageRange (const String& rsPageRange);

	USHORT SetPrinterOptDlg (
        SfxPrinter* pNewPrinter,
        USHORT nDiffFlags = SFX_PRINTER_ALL,
        BOOL _bShowDialog = TRUE);

    void PreparePrint (PrintDialog* pPrintDialog = 0);

private:
    ViewShellBase& mrBase;

    bool mbPrintDirectSelected;

    String msPageRange;

    /// Copy constructor not supported.
    PrintManager (const PrintManager&);

    /// Assignment operator not supported.
    PrintManager& operator= (const PrintManager&);

    /** Make sure that the pages that are to be printed do fit to the
        printer pages.  When the internal pages are larger then the printer
        pages and no method of resolution has yet been provided then a
        dialog is opened that asks for one.  These methods are
        a) scale internal page down to the printer page,
        b) crop internal page
        c) print internal page on several printer pages.
        @param pPrinter
            The printer to print the pages on that are specified by the
            msPageRange member.
        @param bSilent
            This flag, when set, tells the method not to wait for user
            input, i.e. show a dialog.
        @return
            The returned flag indicates whether the method was successfull (<TRUE/>)
            or whether the printing has to be aborted (<FALSE/>).  The later
            one is the case only when the dialog is shown and the user
            selects 'Cancel'.
    */
    bool FitPageToPrinterWithDialog (
        SfxPrinter *pPrinter,
        bool bSilent);

    class PrintInfo;

    /** Print outline pages.
    */
	void PrintOutline (
        PrintInfo& rInfo,
        USHORT nPage);

    /** Print handout pages.
    */
	void PrintHandout (
        PrintInfo& rInfo,
        USHORT nPage);

    /** Print slides or notes pages.
    */
	void PrintStdOrNotes (
        PrintInfo& rInfo,
        USHORT nPage,
        PageKind ePageKind,
        BOOL bPrintMarkedOnly);

    /** Print a part of the given page that is specified by the given page
        origin and the size of the printable area.  This method is used
        primarily when the page to print is larger than the printable area
        and the page is printed in several parts.
    */
    void PrintPagePart (
        PrintInfo& rInfo,
        SdPage* pPage,
        DrawView& rPrintView,
        const Point& rPageOrigin,
        BOOL bPrintMarkedOnly,
        const String& rsPageString,
        const Point& rStringOffset);

    /** This method is similar to PrintPagePart() and should be unified with
        that.
    */
	void PrintPage(
        PrintInfo& rInfo,
        ::sd::View* pPrintView,
        SdPage* pPage,
        BOOL bPrintMarkedOnly);

    /** Return <TRUE/> when the document pages have the 'Screen' size.  Only
        the first page of the document is used to determine the size.
    */
    bool IsScreenFormat (void);

	/** creates the needed shapes on the handout page for the given layout */
	void InitHandoutTemplate( PrintInfo& rInfo, USHORT nSlidesPerHandout, BOOL bHandoutHorizontal );
};

} // end of namespace sd

#endif
