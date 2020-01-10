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

#ifdef SD_DLLIMPLEMENTATION
#undef SD_DLLIMPLEMENTATION
#endif
#include <com/sun/star/presentation/ClickAction.hpp>
#include <vcl/svapp.hxx>
#include <vos/mutex.hxx>
#include <vcl/msgbox.hxx>
#include <vcl/lstbox.hxx>
#include <vcl/combobox.hxx>
#include <sfx2/doctempl.hxx>
#include <svtools/lstner.hxx>
#include <sfx2/objsh.hxx>
#include <svtools/ehdl.hxx>
#include <svtools/sfxecode.hxx>
#include <tools/urlobj.hxx>
#include <com/sun/star/presentation/FadeEffect.hpp>
#include <fadedef.h>
#include <sfx2/sfxsids.hrc>
#include <svtools/undo.hxx>
#include "DrawDocShell.hxx"
#include <vcl/gdimtf.hxx>
#include <vcl/wintypes.hxx>
#include "docprev.hxx"
#include <sfx2/app.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/request.hxx>
#include <com/sun/star/sdbc/XResultSet.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/util/XCloseable.hpp>
#include <com/sun/star/uno/RuntimeException.hpp>
#include <com/sun/star/frame/XModuleManager.hpp>
#include <com/sun/star/ucb/XSimpleFileAccess.hpp>
#include <com/sun/star/ui/XModuleUIConfigurationManagerSupplier.hpp>
#include <com/sun/star/ui/XImageManager.hpp>
#include <svtools/historyoptions.hxx>
#include <tools/urlobj.hxx>
#include <osl/file.hxx>
#include <sfx2/filedlghelper.hxx>

#include "sdpage.hxx"
#include "helpids.h"
#include "assclass.hxx"
#include "dlgass.hrc"
#include "dlgass.hxx"
#include "dlgctrls.hxx"
#ifndef _SD_CFGID_HXX
#include "strings.hrc"
#endif
#ifndef _DATETIMEITEM_HXX
#include "dlgassim.hxx"
#endif
#include "TemplateScanner.hxx"
#include "WindowUpdater.hxx"

#include <comphelper/processfactory.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::sd;


void InterpolateFixedBitmap( FixedBitmap * pBitmap )
{
	Bitmap aBmp( pBitmap->GetBitmap() );
	Size aSize = pBitmap->GetSizePixel();
	aBmp.Scale( aSize, BMP_SCALE_INTERPOLATE );
	pBitmap->SetBitmap( aBmp );
}


// ====================================================================
// ====================================================================

UINT32 PageHelpIds[] =
{
	HID_SD_AUTOPILOT_PAGE1,
	HID_SD_AUTOPILOT_PAGE2,
	HID_SD_AUTOPILOT_PAGE3,
	HID_SD_AUTOPILOT_PAGE4,
	HID_SD_AUTOPILOT_PAGE5
};

// ====================================================================

class PasswordEntry
{
public:
	String maPassword;
	String maPath;
};

DECLARE_LIST( PasswordEntryList, PasswordEntry * )

// ====================================================================

/** A simple wrapper that looks like a PushButton and is used to force the
    broadcasting of focus events primarly for accessibility tools.
    Forcing focus events is achieved by using two identical PushButtons
    which, when the focus event is requested, are exchanged and play focus
    ping-pong by moving the focus from one to the other.
*/
class NextButton
{
public:
    NextButton (::Window* pParent, const ResId& rResId);

    void ForceFocusEventBroadcast (void);
	void SetClickHdl (const Link& rLink);
    bool IsEnabled (void);
	void Enable (bool bEnable);
    
private:
    PushButton maNextButton1;
    PushButton maNextButton2;
    bool mbIsFirstButtonActive;
};



// ====================================================================

class AssistentDlgImpl : public SfxListener
{
public:
	AssistentDlgImpl( ::Window* pWindow, const Link& rFinishLink, BOOL bAutoPilot  );
	~AssistentDlgImpl();

    /// Local mutex used to serialize concurrent method calls.
    ::osl::Mutex maMutex;

	SfxObjectShellLock GetDocument();

	/** closes the current preview docshell */
	void CloseDocShell();

	/**	Extract form the history list of recently used files the impress
		files and insert them into a listbox.
	*/
	void	ScanDocmenu			(void);
    /**	Flag that is set to TRUE after the recently used files have been
        scanned.
	*/
	BOOL mbRecentDocumentsReady;

	/**	When the list of templates has not been scanned already this is done
        when this method is called.  That includes requesting the whole list
        of templates from UCB and extracting from that list the impress
        templates and layouts and storing them for later use in
        <member>maPresentList</member>.  Note that the first call to this
        method after installing a new Office may take some time.
	*/
	void ProvideTemplates (void);

	/**	This method transfers the template folders from the template scanner
		to the internal structures of this class.  On termination it sets
		the flag <member>mbTemplatesReady</member> to <TRUE/> to indicate
		that the templates are available.
        @param rTemplateFolders
            This is a list of template folders.  This method takes ownership
            of the supplied entries by removing them from the list and
            transferring them to an internal structure.
	*/
	void TemplateScanDone (std::vector<TemplateDir*>& rTemplateFolders);

	/**	Flag that is set to TRUE after the impress templates have been
        scanned.
	*/
	BOOL mbTemplatesReady;

    /** Flag used to prevent nested or concurrent calls to the
        <member>UpdatePreview</memember> method.  A <TRUE/> value indicates
        that a preview update is currently active.
    */
    BOOL mbPreviewUpdating;

	::Window* mpWindow;

	void SavePassword( SfxObjectShellLock xDoc, const String& rPath );
	void RestorePassword( SfxItemSet* pSet, const String& rPath );
	String GetPassword( const String rPath );
	void DeletePassords();

	PasswordEntryList maPasswordList;

	String maDocFile;
	String maLayoutFile;

	String GetDocFileName();
	String GetLayoutFileName();

	///	List of URLs of recently used impress files.
	std::vector<String*> maOpenFilesList;

	///	List of folders containing data about impress templates.
	std::vector<TemplateDir*> maPresentList;

	///	Currently selected template folder.
	TemplateDir* mpTemplateRegion;

	///	Currently selected layout folder.
	TemplateDir* mpLayoutRegion;

	// preview
	BOOL mbUserDataDirty;
	Timer maPrevTimer;
	Timer maEffectPrevTimer;
	Timer maUpdatePageListTimer;
	Timer maStartScanTimer;

	SfxObjectShellLock xDocShell;

    ::std::auto_ptr<WindowUpdater> mpWindowUpdater;

	BOOL mbPreview;
	USHORT mnShowPage;
	BOOL mbDocPreview;

	ULONG mnTemplate;

	String maPageListFile;

	void UpdatePreview( BOOL bDocPreview );
	void UpdatePageList();
	void UpdateUserData();

	BOOL IsOwnFormat( const String& rPath );

	// dlg status
	void EndDialog( long nResult = 0 );

	void SetStartType( StartType eType );
	StartType GetStartType();

	void SelectTemplateRegion( const String& rRegion );
	void SelectLayoutRegion( const String& rRegion );

	void UpdatePage();
	void ChangePage();
	void LeavePage();

    String GetUiTextForCommand (const ::rtl::OUString& aCommandURL);
    Image GetUiIconForCommand (const ::rtl::OUString& aCommandURL);

	DECL_LINK( StartScanHdl, void * );
	DECL_LINK( SelectFileHdl, ListBox * );
	DECL_LINK( SelectRegionHdl, ListBox * );
	DECL_LINK( UpdatePreviewHdl, void * );
	DECL_LINK( UpdatePageListHdl, void * );
	DECL_LINK( StartTypeHdl, RadioButton * );
	DECL_LINK( SelectTemplateHdl, ListBox * );
	DECL_LINK( NextPageHdl, PushButton * );
	DECL_LINK( LastPageHdl, PushButton * );
	DECL_LINK( PreviewFlagHdl, CheckBox * );
	DECL_LINK( EffectPreviewHdl, Button * );
	DECL_LINK( SelectLayoutHdl, ListBox * );
	DECL_LINK( PageSelectHdl, Control * );
	DECL_LINK( PresTypeHdl, RadioButton * );
	DECL_LINK( UpdateUserDataHdl, Edit* );
	DECL_LINK( SelectEffectHdl, void* );
    DECL_LINK( OpenButtonHdl, Button * );

	// Common
	Assistent			maAssistentFunc;
	CheckBox			maPreviewFlag;
	CheckBox			maStartWithFlag;
	HelpButton			maHelpButton;
	CancelButton		maCancelButton;
	PushButton			maLastPageButton;
    NextButton	        maNextPageButton;
	OKButton			maFinishButton;
	SdDocPreviewWin		maPreview;

    String				maCreateStr;
    String				maOpenStr;

	// Seite 1
	FixedBitmap*		mpPage1FB;
	FixedLine*			mpPage1ArtFL;
	RadioButton*		mpPage1EmptyRB;
	RadioButton*		mpPage1TemplateRB;
	ListBox*			mpPage1RegionLB;
	ListBox*			mpPage1TemplateLB;
	RadioButton*		mpPage1OpenRB;
	ListBox*			mpPage1OpenLB;
    PushButton*         mpPage1OpenPB;

	// Seite 2
	FixedBitmap*		mpPage2FB;
	FixedLine*			mpPage2LayoutFL;
	ListBox*			mpPage2RegionLB;
	ListBox*			mpPage2LayoutLB;
	FixedLine*			mpPage2OutTypesFL;
	RadioButton*		mpPage2Medium1RB;
	RadioButton*		mpPage2Medium2RB;
	RadioButton*		mpPage2Medium3RB;
	RadioButton*		mpPage2Medium4RB;
	RadioButton*		mpPage2Medium5RB;

	// Seite 3
	FixedBitmap*		mpPage3FB;
	FixedLine*			mpPage3EffectFL;
	FixedText*			mpPage3EffectFT;
	FadeEffectLB*		mpPage3EffectLB;
	FixedText*			mpPage3SpeedFT;
	ListBox*			mpPage3SpeedLB;
	FixedLine*			mpPage3PresTypeFL;
	RadioButton*		mpPage3PresTypeLiveRB;
	RadioButton*		mpPage3PresTypeKioskRB;
	FixedText*			mpPage3PresTimeFT;
	TimeField*			mpPage3PresTimeTMF;
	FixedText*			mpPage3BreakFT;
	TimeField*			mpPage3BreakTMF;
	CheckBox*			mpPage3LogoCB;

	// Seite 4
	FixedBitmap*		mpPage4FB;
	FixedLine*			mpPage4PersonalFL;
	FixedText*			mpPage4AskNameFT;
	Edit*				mpPage4AskNameEDT;
	FixedText*			mpPage4AskTopicFT;
	Edit*				mpPage4AskTopicEDT;
	FixedText*			mpPage4AskInfoFT;
	MultiLineEdit*		mpPage4AskInfoEDT;

	// Seite 5
	FixedBitmap*		mpPage5FB;
	FixedText*			mpPage5PageListFT;
	SdPageListControl*	mpPage5PageListCT;
	CheckBox*			mpPage5SummaryCB;

};



// ====================================================================

AssistentDlgImpl::AssistentDlgImpl( ::Window* pWindow, const Link& rFinishLink, BOOL bAutoPilot ) :
	mpTemplateRegion(NULL),
	mpLayoutRegion(NULL),
	mbUserDataDirty(FALSE),
	xDocShell (NULL),
    mpWindowUpdater (new WindowUpdater()),
	mbPreview(TRUE),
	mnShowPage(0),
	mbDocPreview(FALSE),
	maAssistentFunc(5),
	maPreviewFlag(pWindow,SdResId(CB_PREVIEW)),
	maStartWithFlag(pWindow,SdResId(CB_STARTWITH)),
	maHelpButton(pWindow,SdResId(BUT_HELP)),
	maCancelButton(pWindow,SdResId(BUT_CANCEL)),
	maLastPageButton(pWindow,SdResId(BUT_LAST)),
	maNextPageButton(pWindow,SdResId(BUT_NEXT)),
	maFinishButton(pWindow,SdResId(BUT_FINISH)),
	maPreview(pWindow,SdResId(CT_PREVIEW)),
    maCreateStr(SdResId(STR_CREATE)),
    maOpenStr(SdResId(STR_OPEN))
{
	maPageListFile += sal_Unicode('?'),
    mbRecentDocumentsReady = FALSE;
	mbTemplatesReady = FALSE;
    mbPreviewUpdating = FALSE;

	mpWindow = pWindow;

	if(bAutoPilot)
		maStartWithFlag.Hide();
	else
		maAssistentFunc.InsertControl(1, &maStartWithFlag );

	//page1 wird initialisiert und an die Assistentenfunktionalitaet
	//uebergeben
	maAssistentFunc.InsertControl(1, &maPreview );
	maAssistentFunc.InsertControl(1, &maPreviewFlag );
	maAssistentFunc.InsertControl(1,
		mpPage1FB = new FixedBitmap(pWindow,SdResId(FB_PAGE1)));
	maAssistentFunc.InsertControl(1,
		mpPage1ArtFL = new FixedLine(pWindow,SdResId(FL_PAGE1_ARTGROUP)));
	maAssistentFunc.InsertControl(1,
		mpPage1EmptyRB=new RadioButton(pWindow,SdResId(RB_PAGE1_EMPTY)));
	maAssistentFunc.InsertControl(1,
		mpPage1TemplateRB=new RadioButton(pWindow,SdResId(RB_PAGE1_TEMPLATE)));
	maAssistentFunc.InsertControl(1,
		mpPage1OpenRB=new RadioButton(pWindow,SdResId(RB_PAGE1_OPEN)));
	maAssistentFunc.InsertControl(1,
		mpPage1RegionLB = new ListBox(pWindow,SdResId(LB_PAGE1_REGION)));
	maAssistentFunc.InsertControl(1,
		mpPage1TemplateLB=new ListBox(pWindow,SdResId(LB_PAGE1_TEMPLATES)));
	maAssistentFunc.InsertControl(1,
		mpPage1OpenPB=new PushButton(pWindow,SdResId(PB_PAGE1_OPEN)));
	maAssistentFunc.InsertControl(1,
		mpPage1OpenLB=new ListBox(pWindow,SdResId(LB_PAGE1_OPEN)));

    // Align the button and list box displayed for the "open existing file"
    // radio button with the text of that radio button.
    {
        RadioButton aEmptyRB (mpWindow);
        sal_Int32 nIndent (aEmptyRB.CalcMinimumSize(0).Width());
        sal_Int32 nLeft (mpPage1OpenRB->GetPosPixel().X() + nIndent);
        sal_Int32 nWidth (mpPage1OpenRB->GetSizePixel().Width() - nIndent);
        mpPage1OpenPB->SetPosSizePixel(
            Point(nLeft, mpPage1OpenPB->GetPosPixel().Y()),
            Size(mpPage1OpenPB->GetSizePixel()));
        mpPage1OpenLB->SetPosSizePixel(
            Point(nLeft, mpPage1OpenLB->GetPosPixel().Y()),
            Size(nWidth, mpPage1OpenLB->GetSizePixel().Height()));
    }

    // Set text and icon of the 'Open...' button.
    {
        String sText (GetUiTextForCommand(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(".uno:Open"))));
        // Remove the mnemonic and add a leading space so that icon and text
        // are not too close together.
        sText.SearchAndReplaceAll(String(RTL_CONSTASCII_STRINGPARAM("~")),String());
        sText.Insert(String(RTL_CONSTASCII_STRINGPARAM(" ")),0);
        mpPage1OpenPB->SetText(sText);
        // Place icon left of text and both centered in the button.
        mpPage1OpenPB->SetModeImage(
            GetUiIconForCommand(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(".uno:Open"))),
            BMP_COLOR_NORMAL);
        mpPage1OpenPB->EnableImageDisplay(TRUE);
        mpPage1OpenPB->EnableTextDisplay(TRUE);
        mpPage1OpenPB->SetImageAlign(IMAGEALIGN_LEFT);
        mpPage1OpenPB->SetStyle(mpPage1OpenPB->GetStyle() | WB_CENTER);
    }

	// links&handler
	mpPage1RegionLB->SetSelectHdl(LINK(this,AssistentDlgImpl,SelectRegionHdl));
	mpPage1RegionLB->SetDropDownLineCount( 6 );
	mpPage1TemplateLB->SetSelectHdl(LINK(this,AssistentDlgImpl,SelectTemplateHdl));
	mpPage1TemplateLB->InsertEntry(String(SdResId(STR_ISLOADING)));

	mpPage1EmptyRB->SetClickHdl(LINK(this,AssistentDlgImpl,StartTypeHdl));
	mpPage1TemplateRB->SetClickHdl(LINK(this,AssistentDlgImpl,StartTypeHdl));
	mpPage1OpenRB->SetClickHdl(LINK(this,AssistentDlgImpl,StartTypeHdl));
	mpPage1OpenLB->SetSelectHdl(LINK(this,AssistentDlgImpl,SelectFileHdl));
	mpPage1OpenLB->SetDoubleClickHdl(rFinishLink);
    mpPage1OpenPB->SetClickHdl(LINK(this,AssistentDlgImpl,OpenButtonHdl));
    //	mpPage1OpenLB->InsertEntry(String(SdResId(STR_WIZARD_POSITION)));

	// Seite 2
	maAssistentFunc.InsertControl(2, &maPreview );
	maAssistentFunc.InsertControl(2, &maPreviewFlag );
	maAssistentFunc.InsertControl(2,
		mpPage2FB = new FixedBitmap(pWindow,SdResId(FB_PAGE2)));
	maAssistentFunc.InsertControl(2,
		mpPage2LayoutFL = new FixedLine( pWindow, SdResId(FL_PAGE2_LAYOUT) ));
	maAssistentFunc.InsertControl(2,
		mpPage2RegionLB = new ListBox(pWindow,SdResId(LB_PAGE2_REGION) ));
	maAssistentFunc.InsertControl(2,
		mpPage2LayoutLB = new ListBox(pWindow,SdResId(LB_PAGE2_LAYOUT) ));

	maAssistentFunc.InsertControl(2,
		mpPage2OutTypesFL = new FixedLine( pWindow, SdResId(FL_PAGE2_OUTPUTTYPES) ));
	maAssistentFunc.InsertControl(2,
		mpPage2Medium5RB = new RadioButton( pWindow, SdResId(RB_PAGE2_MEDIUM5) ));
	maAssistentFunc.InsertControl(2,
		mpPage2Medium3RB = new RadioButton( pWindow, SdResId(RB_PAGE2_MEDIUM3) ));
	maAssistentFunc.InsertControl(2,
		mpPage2Medium4RB = new RadioButton( pWindow, SdResId(RB_PAGE2_MEDIUM4) ));
	maAssistentFunc.InsertControl(2,
		mpPage2Medium1RB = new RadioButton( pWindow, SdResId(RB_PAGE2_MEDIUM1) ));
	maAssistentFunc.InsertControl(2,
		mpPage2Medium2RB = new RadioButton( pWindow, SdResId(RB_PAGE2_MEDIUM2) ));
	mpPage2Medium5RB->Check();

	mpPage2RegionLB->SetSelectHdl(LINK(this,AssistentDlgImpl,SelectRegionHdl));
	mpPage2RegionLB->SetDropDownLineCount( 6 );
	mpPage2LayoutLB->SetSelectHdl(LINK(this,AssistentDlgImpl,SelectLayoutHdl));
	mpPage2LayoutLB->InsertEntry(String(SdResId(STR_ISLOADING)));

	// Seite 3
	maAssistentFunc.InsertControl(3, &maPreview );
	maAssistentFunc.InsertControl(3, &maPreviewFlag );
	maAssistentFunc.InsertControl(3,
		mpPage3FB = new FixedBitmap(pWindow,SdResId(FB_PAGE3)));
	maAssistentFunc.InsertControl(3,
		mpPage3EffectFL = new FixedLine( pWindow, SdResId(FL_PAGE3_EFFECT) ));
	maAssistentFunc.InsertControl(3,
		mpPage3EffectFT = new FixedText( pWindow, SdResId(FT_PAGE3_EFFECT) ));
	maAssistentFunc.InsertControl(3,
		mpPage3EffectLB = new FadeEffectLB( pWindow, SdResId(LB_PAGE3_EFFECT) ));
	maAssistentFunc.InsertControl(3,
		mpPage3SpeedFT = new FixedText( pWindow, SdResId(FT_PAGE3_SPEED) ));
	maAssistentFunc.InsertControl(3,
		mpPage3SpeedLB = new FadeEffectLB( pWindow, SdResId(LB_PAGE3_SPEED) ));
	maAssistentFunc.InsertControl(3,
		mpPage3PresTypeFL = new FixedLine( pWindow, SdResId(FL_PAGE3_PRESTYPE) ));
	maAssistentFunc.InsertControl(3,
		mpPage3PresTypeLiveRB = new RadioButton( pWindow, SdResId(RB_PAGE3_LIVE) ));
	maAssistentFunc.InsertControl(3,
		mpPage3PresTypeKioskRB = new RadioButton( pWindow, SdResId(RB_PAGE3_KIOSK) ));
	maAssistentFunc.InsertControl(3,
		mpPage3PresTimeFT = new FixedText( pWindow, SdResId( FT_PAGE3_TIME) ));
	maAssistentFunc.InsertControl(3,
		mpPage3PresTimeTMF = new TimeField( pWindow, SdResId( TMF_PAGE3_TIME) ));
	maAssistentFunc.InsertControl(3,
		mpPage3BreakFT = new FixedText( pWindow, SdResId( FT_PAGE3_BREAK) ));
	maAssistentFunc.InsertControl(3,
		mpPage3BreakTMF = new TimeField( pWindow, SdResId( TMF_PAGE3_BREAK) ));
	maAssistentFunc.InsertControl(3,
		mpPage3LogoCB = new CheckBox( pWindow, SdResId( CB_PAGE3_LOGO) ));

	mpPage3EffectLB->Fill();
//	mpPage3EffectLB->SelectEffect( presentation::FadeEffect_NONE );
	mpPage3EffectLB->SetSelectHdl( LINK(this,AssistentDlgImpl,SelectEffectHdl ));
	mpPage3EffectLB->SetDropDownLineCount( 12 );

	mpPage3SpeedLB->InsertEntry( String( SdResId(STR_SLOW) ));
	mpPage3SpeedLB->InsertEntry( String( SdResId(STR_MEDIUM) ));
	mpPage3SpeedLB->InsertEntry( String( SdResId(STR_FAST) ));
	mpPage3SpeedLB->SetDropDownLineCount( 3 );
	mpPage3SpeedLB->SetSelectHdl( LINK(this,AssistentDlgImpl,SelectEffectHdl ));
	mpPage3SpeedLB->SelectEntryPos( 1 );

	mpPage3PresTypeLiveRB->Check();
	mpPage3PresTypeLiveRB->SetClickHdl( LINK(this,AssistentDlgImpl, PresTypeHdl ));
	mpPage3PresTypeKioskRB->SetClickHdl( LINK(this,AssistentDlgImpl, PresTypeHdl ));
	mpPage3PresTimeTMF->SetFormat( TIMEF_SEC );
	mpPage3PresTimeTMF->SetTime( Time( 0, 0, 10 ) );
	mpPage3BreakTMF->SetFormat( TIMEF_SEC );
	mpPage3BreakTMF->SetTime( Time( 0, 0, 10 ) );
	mpPage3LogoCB->Check();

	// set cursor in timefield
	Edit *pEditPage3PresTimeTMF = mpPage3PresTimeTMF->GetField();
	Edit *pEditPage3BreakTMF = mpPage3BreakTMF->GetField();
	Selection aSel1( pEditPage3PresTimeTMF->GetMaxTextLen(), pEditPage3PresTimeTMF->GetMaxTextLen() );
	Selection aSel2( pEditPage3BreakTMF->GetMaxTextLen(), pEditPage3BreakTMF->GetMaxTextLen() );
	pEditPage3PresTimeTMF->SetSelection( aSel1 );
	pEditPage3BreakTMF->SetSelection( aSel2 );

	// Seite 4
	maAssistentFunc.InsertControl(4,
		mpPage4FB = new FixedBitmap(pWindow,SdResId(FB_PAGE4)));
	maAssistentFunc.InsertControl(4,
		mpPage4PersonalFL = new FixedLine( pWindow, SdResId(FL_PAGE4_PERSONAL) ));
	maAssistentFunc.InsertControl(4,
		mpPage4AskNameFT   = new FixedText( pWindow, SdResId(FT_PAGE4_ASKNAME) ));
	maAssistentFunc.InsertControl(4,
		mpPage4AskNameEDT  = new Edit( pWindow, SdResId(EDT_PAGE4_ASKNAME) ));
	maAssistentFunc.InsertControl(4,
		mpPage4AskTopicFT= new FixedText( pWindow, SdResId(FT_PAGE4_ASKTOPIC) ));
	maAssistentFunc.InsertControl(4,
		mpPage4AskTopicEDT = new Edit( pWindow, SdResId(EDT_PAGE4_ASKTOPIC) ));
	maAssistentFunc.InsertControl(4,
		mpPage4AskInfoFT   = new FixedText( pWindow, SdResId(FT_PAGE4_ASKINFORMATION) ));
	maAssistentFunc.InsertControl(4,
		mpPage4AskInfoEDT  = new MultiLineEdit( pWindow, SdResId(EDT_PAGE4_ASKINFORMATION) ));

	mpPage4AskNameEDT->SetModifyHdl(LINK(this,AssistentDlgImpl,UpdateUserDataHdl));
	mpPage4AskTopicEDT->SetModifyHdl(LINK(this,AssistentDlgImpl,UpdateUserDataHdl));
	mpPage4AskInfoEDT->SetModifyHdl(LINK(this,AssistentDlgImpl,UpdateUserDataHdl));

	// page 5
	maAssistentFunc.InsertControl(5, &maPreview );
	maAssistentFunc.InsertControl(5, &maPreviewFlag );
	maAssistentFunc.InsertControl(5,
		mpPage5FB = new FixedBitmap(pWindow,SdResId(FB_PAGE5)));
	maAssistentFunc.InsertControl(5,
		mpPage5PageListFT = new FixedText( pWindow, SdResId( FT_PAGE5_PAGELIST ) ));
	maAssistentFunc.InsertControl(5,
		mpPage5PageListCT = new SdPageListControl( pWindow, SdResId( CT_PAGE5_PAGELIST ) ));
	maAssistentFunc.InsertControl(5,
		mpPage5SummaryCB  = new CheckBox( pWindow, SdResId( CB_PAGE5_SUMMARY ) ));

	mpPage5PageListCT->SetSelectHdl(LINK(this,AssistentDlgImpl, PageSelectHdl));


	// generell
    InterpolateFixedBitmap( mpPage1FB );
    InterpolateFixedBitmap( mpPage2FB );
    InterpolateFixedBitmap( mpPage3FB );
    InterpolateFixedBitmap( mpPage4FB );
    InterpolateFixedBitmap( mpPage5FB );

	maLastPageButton.SetClickHdl(LINK(this,AssistentDlgImpl, LastPageHdl ));
	maNextPageButton.SetClickHdl(LINK(this,AssistentDlgImpl, NextPageHdl ));

	maPreviewFlag.Check( mbPreview );
	maPreviewFlag.SetClickHdl(LINK(this, AssistentDlgImpl, PreviewFlagHdl ));
	maPreview.SetClickHdl(LINK(this,AssistentDlgImpl, EffectPreviewHdl ));

	//setzt die Ausgangsseite
	maAssistentFunc.GotoPage(1);
	maLastPageButton.Disable();

	maPrevTimer.SetTimeout( 200 );
	maPrevTimer.SetTimeoutHdl( LINK( this, AssistentDlgImpl, UpdatePreviewHdl));

	maEffectPrevTimer.SetTimeout( 50 );
	maEffectPrevTimer.SetTimeoutHdl( LINK( this, AssistentDlgImpl, EffectPreviewHdl ));

	maUpdatePageListTimer.SetTimeout( 50 );
	maUpdatePageListTimer.SetTimeoutHdl( LINK( this, AssistentDlgImpl, UpdatePageListHdl));

	SetStartType( ST_EMPTY );

	ChangePage();

    mpWindowUpdater->RegisterWindow (&maPreview);

	UpdatePreview( TRUE );

	//check wether we should start with a template document initialy and preselect it
	const ::rtl::OUString aServiceName( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.presentation.PresentationDocument" ) );
    String aStandardTemplate( SfxObjectFactory::GetStandardTemplate( aServiceName ) );
    if( aStandardTemplate.Len() )
    {
        ProvideTemplates();

        //find aStandardTemplate in maPresentList
        TemplateDir*   pStandardTemplateDir = 0;
        TemplateEntry* pStandardTemplateEntry = 0;

        std::vector<TemplateDir*>::iterator	I;
	    for (I=maPresentList.begin(); I!=maPresentList.end(); I++)
	    {
		    TemplateDir* pDir = *I;
		    std::vector<TemplateEntry*>::iterator	J;
			for (J=pDir->maEntries.begin(); J!=pDir->maEntries.end(); J++)
            {
                TemplateEntry* pEntry = *J;
                if(pEntry->msPath == aStandardTemplate)
                {
                    pStandardTemplateDir = pDir;
                    pStandardTemplateEntry = pEntry;
                    break;
                }
            }
            if(pStandardTemplateDir)
                break;
	    }

		//preselect template
        if( pStandardTemplateDir && pStandardTemplateEntry )
        {
            mpPage1RegionLB->SelectEntry( pStandardTemplateDir->msRegion );
            SelectTemplateRegion( pStandardTemplateDir->msRegion );
            mpPage1TemplateLB->SelectEntry( pStandardTemplateEntry->msTitle );
            SelectTemplateHdl(mpPage1TemplateLB);
        }
    }
}




AssistentDlgImpl::~AssistentDlgImpl()
{
	CloseDocShell();

	DeletePassords();

	//	Delete the template file infos.
	std::vector<TemplateDir*>::iterator	I;
	std::vector<TemplateEntry*>::iterator	J;
	for (I=maPresentList.begin(); I!=maPresentList.end(); I++)
	{
		for (J=(*I)->maEntries.begin(); J!=(*I)->maEntries.end(); J++)
			delete (*J);
		delete (*I);
	}

	// Seite 1
	delete mpPage1FB;
	delete mpPage1ArtFL;
	delete mpPage1EmptyRB;
	delete mpPage1TemplateRB;
	delete mpPage1TemplateLB;
	delete mpPage1RegionLB;
	delete mpPage1OpenRB;
	delete mpPage1OpenLB;
    delete mpPage1OpenPB;

	// Seite 2
	delete mpPage2FB;
	delete mpPage2LayoutFL;
	delete mpPage2RegionLB;
	delete mpPage2LayoutLB;
	delete mpPage2OutTypesFL;
	delete mpPage2Medium1RB;
	delete mpPage2Medium2RB;
	delete mpPage2Medium3RB;
	delete mpPage2Medium4RB;
	delete mpPage2Medium5RB;

	// Seite 3
	delete mpPage3FB;
	delete mpPage3EffectFL;
	delete mpPage3EffectFT;
	delete mpPage3EffectLB;
	delete mpPage3SpeedFT;
	delete mpPage3SpeedLB;
	delete mpPage3PresTypeFL;
	delete mpPage3PresTypeLiveRB;
	delete mpPage3PresTypeKioskRB;
	delete mpPage3PresTimeFT;
	delete mpPage3PresTimeTMF;
	delete mpPage3BreakFT;
	delete mpPage3BreakTMF;
	delete mpPage3LogoCB;

	// Seite 4
	delete mpPage4FB;
	delete mpPage4PersonalFL;
	delete mpPage4AskNameFT;
	delete mpPage4AskNameEDT;
	delete mpPage4AskTopicFT;
	delete mpPage4AskTopicEDT;
	delete mpPage4AskInfoFT;
	delete mpPage4AskInfoEDT;

	// Seite 5
	delete mpPage5FB;
	delete mpPage5PageListFT;
	delete mpPage5PageListCT;
	delete mpPage5SummaryCB;

	//	Delete the file history list.
	std::vector<String*>::iterator	I2;
	for (I2=maOpenFilesList.begin(); I2!=maOpenFilesList.end(); I2++)
		delete *I2;
}

void AssistentDlgImpl::CloseDocShell()
{
	if(xDocShell.Is())
	{
		//uno::Reference< lang::XComponent > xModel( xDocShell->GetModel(), uno::UNO_QUERY );
		uno::Reference< util::XCloseable > xCloseable( xDocShell->GetModel(), uno::UNO_QUERY );
		//if( xModel.is() )
		if( xCloseable.is() )
		{
			xCloseable->close( sal_True );
			xDocShell = NULL;
			//xModel->dispose();
		}
		else
		{
			xDocShell->DoClose();
			xDocShell = NULL;
		}
	}
}

void AssistentDlgImpl::EndDialog( long )
{
	mpWindow = NULL;
}




void	AssistentDlgImpl::ScanDocmenu	(void)
{
    if( mbRecentDocumentsReady )
        return;

	uno::Sequence<uno::Sequence<beans::PropertyValue> > aHistory =
		SvtHistoryOptions().GetList (ePICKLIST);

    uno::Reference< lang::XMultiServiceFactory > xFactory( ::comphelper::getProcessServiceFactory() );
    uno::Reference< container::XNameAccess > xFilterFactory( xFactory->createInstance( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.document.FilterFactory" ) ) ), uno::UNO_QUERY );

    Reference< ::com::sun::star::ucb::XSimpleFileAccess > xFileAccess(
        xFactory->createInstance(
            ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("com.sun.star.ucb.SimpleFileAccess"))),
        UNO_QUERY_THROW);

	sal_uInt32 nCount = aHistory.getLength();
	for (sal_uInt32 nItem=0; nItem<nCount; ++nItem)
	{
		//	Get the current history item's properties.
		uno::Sequence<beans::PropertyValue> aPropertySet = aHistory[nItem];
		rtl::OUString	sURL;
		rtl::OUString	sFilter;
		rtl::OUString	sTitle;
		rtl::OUString	sPassword;
		sal_uInt32 nPropertyCount = aPropertySet.getLength();
		for (sal_uInt32 nProperty=0; nProperty<nPropertyCount; ++nProperty)
			if (aPropertySet[nProperty].Name == HISTORY_PROPERTYNAME_URL)
				aPropertySet[nProperty].Value >>= sURL;
			else if (aPropertySet[nProperty].Name == HISTORY_PROPERTYNAME_FILTER)
				aPropertySet[nProperty].Value >>= sFilter;
			else if (aPropertySet[nProperty].Name == HISTORY_PROPERTYNAME_TITLE)
				aPropertySet[nProperty].Value >>= sTitle;
			else if (aPropertySet[nProperty].Name == HISTORY_PROPERTYNAME_PASSWORD)
				aPropertySet[nProperty].Value >>= sPassword;

        //	If the entry is an impress file then insert it into the
        //	history list and the list box.
        uno::Sequence< beans::PropertyValue > lProps;
        if (xFilterFactory->hasByName(sFilter))
        {
            uno::Any aFilterPropSet = xFilterFactory->getByName( sFilter );
            aFilterPropSet >>= lProps;
        }

        sal_Int32 nPropCount = lProps.getLength();
        rtl::OUString sFactoryName;
        for( sal_Int32 i=0; i<nPropCount; ++i )
        {
            if( lProps[i].Name.compareToAscii( "DocumentService" ) == 0 &&
                (lProps[i].Value >>= sFactoryName) &&
                sFactoryName.compareToAscii( "com.sun.star.presentation.PresentationDocument" ) == 0 )
            {
                // yes, it's an impress document
                INetURLObject aURL;

                // Do not include the file if it does not exist.
                if (xFileAccess.is() && ! xFileAccess->exists(sURL))
                    continue;

                aURL.SetSmartURL (sURL);
                // The password is set only when it is not empty.
                if (sPassword.getLength() > 0)
	                aURL.SetPass (sPassword);
                maOpenFilesList.push_back (new String (aURL.GetMainURL( INetURLObject::NO_DECODE )));
                mpPage1OpenLB->InsertEntry (sTitle);
                break;
            }
		}
	}
    mbRecentDocumentsReady = TRUE;
    try
    {
        UpdatePreview(TRUE);
    }
    catch (uno::RuntimeException& )
    {
        // Ignore all exceptions.
    }
}



void AssistentDlgImpl::ProvideTemplates (void)
{
    if ( ! mbTemplatesReady)
    {
        TemplateScanner aScanner;
        aScanner.Scan ();
        TemplateScanDone (aScanner.GetFolderList());

        try
        {
            UpdatePreview(TRUE);
        }
        catch (uno::RuntimeException& e)
        {
            // Ignore all exceptions.
            (void) e;
        }
    }
}

void AssistentDlgImpl::TemplateScanDone (
    std::vector<TemplateDir*>& rTemplateFolder)
{
	//	This method is called from a thread.  Therefore we get the solar mutex.
	::vos::OGuard aGuard (Application::GetSolarMutex());

    // Copy the contents of the given template folders to a local list.
    maPresentList.swap (rTemplateFolder);

	//	Fill in the list box on the first page.
	int nFirstEntry = 0;
	mpPage1RegionLB->Clear();
	std::vector<TemplateDir*>::iterator	I;
	int	i;
	for (i=0,I=maPresentList.begin(); I!=maPresentList.end(); I++,i++)
	{
		TemplateDir* pDir = *I;
        if (pDir == NULL)
            continue;
        
        // HACK! presnt directory is always initially selected.
        // We have to look at the first entry to get a URL.
        if (pDir->maEntries.size() > 0)
        {
            TemplateEntry* pEntry = pDir->maEntries.front();
            if (pEntry != NULL)
                if (pEntry->msPath.SearchAscii("presnt") != STRING_NOTFOUND)
                    nFirstEntry = i;
        }

		mpPage1RegionLB->InsertEntry (pDir->msRegion);
	}
	mpPage1RegionLB->SelectEntryPos ((USHORT)nFirstEntry);
	mpPage1RegionLB->Update();
	SelectTemplateRegion (mpPage1RegionLB->GetSelectEntry());

	//	Fill in the list box on the second page.
	nFirstEntry = 0;
	mpPage2RegionLB->Clear();
	for (i=0,I=maPresentList.begin(); I!=maPresentList.end(); I++,i++)
	{
		TemplateDir* pDir = *I;
        if (pDir == NULL)
            continue;
        
		// HACK! layout directory is always initially selected.
        // We have to look at the first entry to get a URL.
        if (pDir->maEntries.size() > 0)
        {
            TemplateEntry* pEntry = pDir->maEntries.front();
            if (pEntry != NULL)
                if (pEntry->msPath.SearchAscii("layout") != STRING_NOTFOUND)
                    nFirstEntry = i;
        }

		mpPage2RegionLB->InsertEntry (pDir->msRegion);
	}
	mpPage2RegionLB->SelectEntryPos ((USHORT)nFirstEntry);
	mpPage2RegionLB->Update();
	SelectLayoutRegion (mpPage2RegionLB->GetSelectEntry());

	//	Make the changes visible.
	mbTemplatesReady = TRUE;
	if (mpWindow)
		UpdatePage();
}




// ********************************************************************
// Status Methoden
// ********************************************************************

void AssistentDlgImpl::SetStartType( StartType eType )
{
	mpPage1EmptyRB->SetState( eType == ST_EMPTY );
	mpPage1TemplateRB->SetState( eType == ST_TEMPLATE );
	mpPage1OpenRB->SetState( eType == ST_OPEN );
	maNextPageButton.Enable( eType != ST_OPEN );

	mpPage1RegionLB->Show(eType == ST_TEMPLATE);
	mpPage1TemplateLB->Show(eType == ST_TEMPLATE);
	mpPage1OpenLB->Show(eType == ST_OPEN);
	mpPage1OpenPB->Show(eType == ST_OPEN);
	
	if (eType == ST_OPEN)
		maFinishButton.SetText(maOpenStr);
	else
		maFinishButton.SetText(maCreateStr);	
}

StartType AssistentDlgImpl::GetStartType()
{
	if( mpPage1EmptyRB->IsChecked() )
		return ST_EMPTY;
	else if( mpPage1TemplateRB->IsChecked() )
		return ST_TEMPLATE;
	else
		return ST_OPEN;
}

String AssistentDlgImpl::GetDocFileName()
{
	String aTitle;
	if(mpWindow)
	{
		aTitle = mpWindow->GetText();
		USHORT nPos = aTitle.Search(sal_Unicode('('));
		if(nPos != STRING_NOTFOUND)
			aTitle.Erase( nPos-1 );
	}

	String aDocFile;
	if( GetStartType() == ST_TEMPLATE )
	{
		const USHORT nEntry = mpPage1TemplateLB->GetSelectEntryPos();
		TemplateEntry* pEntry = NULL;
		if(nEntry != (USHORT)-1)
			pEntry = mpTemplateRegion->maEntries[nEntry];

		if(pEntry)
		{
			aDocFile = pEntry->msPath;

			aTitle.AppendAscii( RTL_CONSTASCII_STRINGPARAM(  " (" ) );
			aTitle.Append( pEntry->msTitle );
			aTitle.Append( sal_Unicode(')') );
		}
	}
	else if( GetStartType() == ST_OPEN )
	{
		const USHORT nEntry = mpPage1OpenLB->GetSelectEntryPos();
		if(nEntry != (USHORT)-1 )
			aDocFile = *maOpenFilesList[nEntry];
	}

	if(mpWindow)
		mpWindow->SetText(aTitle);

	return aDocFile;
}

String AssistentDlgImpl::GetLayoutFileName()
{
	String aFile;
	const USHORT nEntry = mpPage2LayoutLB->GetSelectEntryPos();
	TemplateEntry* pEntry = NULL;
	if(nEntry != (USHORT)-1 && nEntry > 0)
		pEntry = mpLayoutRegion->maEntries[nEntry-1];

	if(pEntry)
		aFile = pEntry->msPath;

	return aFile;
}

SfxObjectShellLock AssistentDlgImpl::GetDocument()
{
//	mbPreview = FALSE;		// Document nicht anzeigen
	UpdatePreview(FALSE);	// aber komplett laden
	UpdatePageList();

	SfxObjectShell* pShell = xDocShell;
	::sd::DrawDocShell* pDocShell = PTR_CAST(::sd::DrawDocShell,pShell);
	SdDrawDocument* pDoc = pDocShell?pDocShell->GetDoc():NULL;

	if(pDoc)
	{
		const USHORT nPageCount = pDoc->GetSdPageCount(PK_STANDARD);
		BOOL bKiosk = mpPage3PresTypeKioskRB->IsChecked();
		UINT32 nNewTime = (UINT32)mpPage3PresTimeTMF->GetTime().GetMSFromTime() / 1000;
		if(bKiosk)
		{
			PresentationSettings& rSettings = pDoc->getPresentationSettings();
			rSettings.mbEndless = bKiosk;
			rSettings.mnPauseTimeout = (sal_Int32)mpPage3BreakTMF->GetTime().GetMSFromTime() / 1000;
			rSettings.mbShowPauseLogo = mpPage3LogoCB->IsChecked();
		}

		USHORT nPgAbsNum = 0;
		USHORT nPgRelNum = 0;
		while( nPgAbsNum < nPageCount )
		{
			SdPage* pPage = pDoc->GetSdPage( nPgRelNum, PK_STANDARD );
			if( mpPage5PageListCT->IsPageChecked(nPgAbsNum) )
			{
				mpPage3EffectLB->applySelected(pPage);
				const sal_uInt16 nPos = mpPage3SpeedLB->GetSelectEntryPos();
		        pPage->setTransitionDuration( (nPos == 0) ? 3.0 : (nPos == 1) ? 2.0 : 1.0 );
				if(bKiosk)
				{
					pPage->SetPresChange( PRESCHANGE_AUTO );
					pPage->SetTime(nNewTime);
				}
				nPgRelNum++;
			}
			else
			{
				// diese Seite loeschen
				pDoc->DeletePage( (nPgRelNum << 1) + 2 ); // Notizseite loeschen
				pDoc->DeletePage( (nPgRelNum << 1) + 1 );	// Seite loeschen
			}

			nPgAbsNum++;
		}
	}
	else
	{
		DBG_ERROR("sd::AssistentDlgImpl::GetDocument(), no template?");
	}

	SfxObjectShellLock xRet = xDocShell;
	xDocShell = NULL;

	return xRet;
}

void AssistentDlgImpl::LeavePage()
{
	int nPage = maAssistentFunc.GetCurrentPage();

	if( nPage == 4 && mbUserDataDirty )
		maPrevTimer.Start();
}

void AssistentDlgImpl::ChangePage()
{
	maNextPageButton.Enable(!maAssistentFunc.IsLastPage());
	maLastPageButton.Enable(!maAssistentFunc.IsFirstPage());

	USHORT nPage = (USHORT)maAssistentFunc.GetCurrentPage();

	if( mpWindow )
		mpWindow->SetHelpId( PageHelpIds[nPage-1]);

	UpdatePage();

	if( maNextPageButton.IsEnabled() )
	{
	  maNextPageButton.ForceFocusEventBroadcast();
	}
	else
		maFinishButton.GrabFocus();
}

void AssistentDlgImpl::UpdatePage()
{
	USHORT nPage = (USHORT)maAssistentFunc.GetCurrentPage();

	switch(nPage)
	{
	case 1:
		{
			// Elemente auf der ersten Seite abhaengig vom Starttype Zeigen
			SetStartType( GetStartType() );
			mpPage1TemplateRB->Enable(TRUE /*mbTemplatesReady*/);
			break;
		}

	case 2:
		{
			mpPage2RegionLB->Enable(mbTemplatesReady);
			mpPage2LayoutLB->Enable(mbTemplatesReady);

			if( GetStartType() != ST_EMPTY )
			{
				mpPage2Medium5RB->Enable( TRUE );
			}
			else
			{
				mpPage2Medium5RB->Enable( FALSE );
				if(mpPage2Medium5RB->IsChecked())
					mpPage2Medium1RB->Check();
			}

			break;
		}
	case 5:
		{
			if(mbDocPreview || maPageListFile != maDocFile)
				mpPage5PageListCT->Clear();

			maUpdatePageListTimer.Start();
			break;
		}

	case 3:
		{
			if(GetStartType() != ST_TEMPLATE)
				maNextPageButton.Enable(false);

			BOOL bKiosk = mpPage3PresTypeKioskRB->IsChecked();
			mpPage3PresTimeFT->Enable(bKiosk);
			mpPage3BreakFT->Enable(bKiosk);
			mpPage3PresTimeTMF->Enable(bKiosk);
			mpPage3BreakTMF->Enable(bKiosk);
			mpPage3LogoCB->Enable(bKiosk);
			break;
		}
	}
}

// ********************************************************************
// UI-Handler
// ********************************************************************

IMPL_LINK( AssistentDlgImpl, SelectRegionHdl, ListBox *, pLB )
{
	if( pLB == mpPage1RegionLB )
	{
		SelectTemplateRegion( pLB->GetSelectEntry() );
		SetStartType( ST_TEMPLATE );
		mpPage2Medium5RB->Check();
	}
	else
	{
		SelectLayoutRegion( pLB->GetSelectEntry() );
	}

	return 0;
}

IMPL_LINK( AssistentDlgImpl, SelectEffectHdl, void*, EMPTYARG )
{
	maEffectPrevTimer.Start();
	return 0;
}

IMPL_LINK( AssistentDlgImpl, OpenButtonHdl, Button*, pButton )
{
    // Clear the selection and forward the call.
    mpPage1OpenLB->SetNoSelection();
    return mpPage1OpenLB->GetDoubleClickHdl().Call(pButton);
}

IMPL_LINK( AssistentDlgImpl, EffectPreviewHdl, Button *, EMPTYARG )
{
	if(mbPreview && xDocShell.Is() )
	{
		SfxObjectShell* pShell = xDocShell;
		DrawDocShell* pDocShell = dynamic_cast< DrawDocShell * >(pShell);
		if( pDocShell )
		{
			SdDrawDocument* pDoc = pDocShell->GetDoc();
			if( pDoc )
			{
				SdPage* pPage = pDoc->GetSdPage( mnShowPage, PK_STANDARD );
				if( pPage )
					mpPage3EffectLB->applySelected(pPage);
			}
		}
		maPreview.startPreview();
	}
	return 0;
}

IMPL_LINK( AssistentDlgImpl, PreviewFlagHdl, CheckBox *, EMPTYARG )

{
	if( maPreviewFlag.IsChecked() != mbPreview )
	{
		mbPreview = maPreviewFlag.IsChecked();
		UpdatePreview(TRUE);
	}
	return 0;
}

IMPL_LINK( AssistentDlgImpl, SelectTemplateHdl, ListBox *, EMPTYARG )
{
	SetStartType( ST_TEMPLATE );
	mpPage2Medium5RB->Check();
	mpPage2LayoutLB->SelectEntryPos(0);
	maPrevTimer.Start();
	return 0;
}

IMPL_LINK( AssistentDlgImpl, SelectLayoutHdl, ListBox *, EMPTYARG )
{
	maPrevTimer.Start();
	return 0;
}

IMPL_LINK( AssistentDlgImpl, SelectFileHdl, ListBox *, EMPTYARG )
{
	SetStartType( ST_OPEN );
	maPrevTimer.Start();
	return 0;
}

IMPL_LINK( AssistentDlgImpl, PageSelectHdl, Control *, EMPTYARG )
{
	USHORT nPage = mpPage5PageListCT->GetSelectedPage();
	if( mnShowPage != nPage )
	{
		mnShowPage = nPage;
		UpdatePreview(FALSE);
	}

	return 0;
}

IMPL_LINK( AssistentDlgImpl, UpdatePageListHdl, void *, EMPTYARG )
{
	UpdatePageList();
	return 0;
}

IMPL_LINK( AssistentDlgImpl, UpdatePreviewHdl, void *, EMPTYARG )
{
	UpdatePreview( TRUE );
	return 0;
}

IMPL_LINK( AssistentDlgImpl, StartTypeHdl, RadioButton *, pButton )
{
	StartType eType = pButton == mpPage1EmptyRB?ST_EMPTY:pButton == mpPage1TemplateRB?ST_TEMPLATE:ST_OPEN;

	if(eType == ST_TEMPLATE)
        ProvideTemplates();
    else if(eType == ST_OPEN)
        ScanDocmenu();

	SetStartType( eType );

	if(eType == ST_TEMPLATE)
	{
		mpPage1TemplateLB->SelectEntryPos(0);
		mpPage2Medium5RB->Check();
	}
	else if(eType == ST_OPEN)
		mpPage1OpenLB->SelectEntryPos(0);

	maPrevTimer.Start();
	return 0;
}


IMPL_LINK( AssistentDlgImpl, NextPageHdl, PushButton *, EMPTYARG )
{
    // When changing from the first to the second page make sure that the
    // templates are present.
	if (maAssistentFunc.GetCurrentPage() == 1)
        ProvideTemplates();

    // Change to the next page.
	LeavePage();
	maAssistentFunc.NextPage();
	ChangePage();
	return 0;
}

IMPL_LINK( AssistentDlgImpl, LastPageHdl, PushButton *, EMPTYARG )
{
	LeavePage();
	maAssistentFunc.PreviousPage();
	ChangePage();
	return 0;
}

IMPL_LINK( AssistentDlgImpl, PresTypeHdl, RadioButton*, EMPTYARG )
{
	if(maDocFile.Len() == 0)
	  {
		maNextPageButton.Enable(false);
	  }

	BOOL bKiosk = mpPage3PresTypeKioskRB->IsChecked();
	mpPage3PresTimeFT->Enable(bKiosk);
	mpPage3BreakFT->Enable(bKiosk);
	mpPage3PresTimeTMF->Enable(bKiosk);
	mpPage3BreakTMF->Enable(bKiosk);
	mpPage3LogoCB->Enable(bKiosk);
	return 0;
}

IMPL_LINK( AssistentDlgImpl, UpdateUserDataHdl, Edit*, EMPTYARG )
{
	mbUserDataDirty = TRUE;
	String aTopic = mpPage4AskTopicEDT->GetText();
	String aName  = mpPage4AskNameEDT->GetText();
	String aInfo  = mpPage4AskInfoEDT->GetText();

	if(aTopic.Len() == 0 && aName.Len() == 0 && aInfo.Len() == 0)
		maDocFile.Erase();

	return 0;
}

// ********************************************************************
// ********************************************************************

void AssistentDlgImpl::SelectTemplateRegion( const String& rRegion )
{
	mpPage1TemplateLB->Clear();
	std::vector<TemplateDir*>::iterator	I;
	for (I=maPresentList.begin(); I!=maPresentList.end(); I++)
	{
		TemplateDir * pDir = *I;
		mpTemplateRegion = *I;
		if (pDir->msRegion.Equals( rRegion ) )
		{
			std::vector<TemplateEntry*>::iterator	J;
			for (J=pDir->maEntries.begin(); J!=pDir->maEntries.end(); J++)
				mpPage1TemplateLB->InsertEntry ((*J)->msTitle);
			mpPage1TemplateLB->Update();
			if(GetStartType() == ST_TEMPLATE)
			{
				mpPage1TemplateLB->SelectEntryPos( 0 );
				SelectTemplateHdl(NULL);
			}
			break;
		}
	}
}

void AssistentDlgImpl::SelectLayoutRegion( const String& rRegion )
{
	mpPage2LayoutLB->Clear();
	mpPage2LayoutLB->InsertEntry(String(SdResId(STR_WIZARD_ORIGINAL)));
	std::vector<TemplateDir*>::iterator	I;
	for (I=maPresentList.begin(); I!=maPresentList.end(); I++)
	{
		TemplateDir * pDir = *I;
		mpLayoutRegion = *I;

		if (pDir->msRegion.Equals (rRegion))
		{
			std::vector<TemplateEntry*>::iterator	J;
			for (J=pDir->maEntries.begin(); J!=pDir->maEntries.end(); J++)
				mpPage2LayoutLB->InsertEntry ((*J)->msTitle);
			mpPage2LayoutLB->Update();
			break;
		}
	}
}

void AssistentDlgImpl::UpdateUserData()
{
	String aTopic = mpPage4AskTopicEDT->GetText();
	String aName  = mpPage4AskNameEDT->GetText();
	String aInfo  = mpPage4AskInfoEDT->GetText();

	SfxObjectShell* pShell = xDocShell;
	DrawDocShell* pDocShell = PTR_CAST(DrawDocShell,pShell);
	SdDrawDocument* pDoc = pDocShell?pDocShell->GetDoc():NULL;
	SdPage* pPage = pDoc?pDoc->GetSdPage(0, PK_STANDARD):NULL;

	if(pPage && ( aTopic.Len() != 0 || aName.Len() != 0 || aInfo.Len() != 0 ) )
	{
		if( pPage->GetAutoLayout() == AUTOLAYOUT_NONE )
			pPage->SetAutoLayout(AUTOLAYOUT_TITLE, TRUE);

		SdrTextObj* pObj;
		String aEmptyString;

		if( aTopic.Len() )
		{
			pObj  = dynamic_cast<SdrTextObj*>( pPage->GetPresObj( PRESOBJ_TITLE ) );
			if( pObj )
			{
				pPage->SetObjText( pObj, NULL, PRESOBJ_TITLE, aTopic );
				pObj->NbcSetStyleSheet( pPage->GetStyleSheetForPresObj( PRESOBJ_TITLE ), TRUE );
				pObj->SetEmptyPresObj(FALSE);
			}

		}

		if ( aName.Len() || aInfo.Len() )
		{
			String aStrTmp( aName );
			if( aName.Len() )
				aStrTmp.AppendAscii( RTL_CONSTASCII_STRINGPARAM( "\n\n" ) );
			aStrTmp.Append( aInfo );

			pObj = dynamic_cast<SdrTextObj*>( pPage->GetPresObj( PRESOBJ_OUTLINE ) );
			if( pObj )
			{
				pPage->SetObjText( pObj, NULL, PRESOBJ_OUTLINE, aStrTmp );
				pObj->NbcSetStyleSheet( pPage->GetStyleSheetForPresObj( PRESOBJ_OUTLINE ), TRUE );
				pObj->SetEmptyPresObj(FALSE);
			}
			else
			{
				pObj = dynamic_cast<SdrTextObj*>( pPage->GetPresObj( PRESOBJ_TEXT ) );
				if( pObj )
				{
					pPage->SetObjText( pObj, NULL, PRESOBJ_TEXT, aStrTmp );
					pObj->NbcSetStyleSheet( pPage->GetStyleSheetForPresObj( PRESOBJ_TEXT ), TRUE );
					pObj->SetEmptyPresObj(FALSE);
				}
			}
		}
	}

	mbUserDataDirty = FALSE;
}

void AssistentDlgImpl::UpdatePageList()
{
	if(mbDocPreview || !mbPreview)
		UpdatePreview(FALSE);
	else if(maPageListFile == maDocFile)
		return;

	maPageListFile = maDocFile;

	SfxObjectShell* pShell = xDocShell;
	DrawDocShell* pDocShell = PTR_CAST(DrawDocShell,pShell);
	SdDrawDocument* pDoc = pDocShell?pDocShell->GetDoc():NULL;

	mpPage5PageListCT->Clear();

	if(pDoc)
		mpPage5PageListCT->Fill(pDoc);
}

void AssistentDlgImpl::UpdatePreview( BOOL bDocPreview )
{
    // Guard against multiple concurrent execution to this method caused either
    // by calls from different threads or recursion.
    ::osl::MutexGuard aGuard (maMutex);
    if (mbPreviewUpdating)
        return;
    mbPreviewUpdating = TRUE;

	if(!mbPreview && bDocPreview)
	{
		maPreview.Invalidate();
		maPreview.SetObjectShell(0);
        mbPreviewUpdating = FALSE;
		return;
	}

	String aDocFile = GetDocFileName();
	String aLayoutFile = GetLayoutFileName();
	String aEmptyStr;

	SfxApplication *pSfxApp = SFX_APP();
	ULONG lErr;
	BOOL bChangeMaster = aLayoutFile.Len() != 0;

	if( aDocFile.Len() == 0 )
	{
		if( !xDocShell.Is() || maDocFile.Len() != 0 ||
			(maDocFile.Len() == 0 && maLayoutFile.Len() != 0 && aLayoutFile.Len() == 0 ))
		{
			CloseDocShell();

			DrawDocShell* pNewDocSh;
			xDocShell = pNewDocSh = new DrawDocShell(SFX_CREATE_MODE_STANDARD, FALSE);
			pNewDocSh->DoInitNew(NULL);
			SdDrawDocument* pDoc = pNewDocSh->GetDoc();
			pDoc->CreateFirstPages();
			pDoc->StopWorkStartupDelay();
			mbDocPreview = FALSE;

			maDocFile = aDocFile;
			mbUserDataDirty = TRUE;
		}
		else
			bChangeMaster = (aLayoutFile.Len() != 0) && (maLayoutFile != aLayoutFile);
	}
	else if( aDocFile == maDocFile && ( mbDocPreview == bDocPreview || bDocPreview ) )
	{
		if( aLayoutFile != maLayoutFile )
		{
			SfxObjectShell* pShell = xDocShell;
			DrawDocShell* pDocShell = PTR_CAST(DrawDocShell,pShell);
			SfxUndoManager* pUndoMgr = pDocShell?pDocShell->GetUndoManager():NULL;
			if(pUndoMgr)
				pUndoMgr->Undo();
			mbUserDataDirty = TRUE;
		}
		else
			bChangeMaster = FALSE;
	}
	else
	{
		CloseDocShell();

		::Window *pParent = Application::GetDefDialogParent();
		Application::SetDefDialogParent( mpWindow );

		SfxErrorContext eEC(ERRCTX_SFX_LOADTEMPLATE,mpWindow);

		SfxItemSet* pSet = new SfxAllItemSet( pSfxApp->GetPool() );
		if(IsOwnFormat(aDocFile))
		{
			pSet->Put( SfxBoolItem( SID_TEMPLATE, TRUE ) );
			if(bDocPreview)
				pSet->Put( SfxBoolItem( SID_PREVIEW, TRUE ) );
			RestorePassword( pSet, aDocFile );
            if( (lErr = pSfxApp->LoadTemplate( xDocShell, aDocFile, TRUE, pSet )) != 0 )
				ErrorHandler::HandleError(lErr);
			else
				SavePassword( xDocShell, aDocFile );
		}
		else
		{
			const String aTargetStr( RTL_CONSTASCII_USTRINGPARAM("_default") );

			SfxRequest aReq( SID_OPENDOC, SFX_CALLMODE_SYNCHRON, SFX_APP()->GetPool() );
			aReq.AppendItem( SfxStringItem( SID_FILE_NAME, aDocFile ));
			aReq.AppendItem( SfxStringItem( SID_REFERER, aEmptyStr ) );
	        aReq.AppendItem( SfxStringItem( SID_TARGETNAME, aTargetStr ) );
			aReq.AppendItem( SfxBoolItem( SID_VIEW, FALSE ) );
			aReq.AppendItem( SfxBoolItem( SID_PREVIEW, bDocPreview ) );

			const SfxObjectShellItem* pRet = (SfxObjectShellItem*)
				SFX_APP()->ExecuteSlot( aReq );

			if( pRet && pRet->GetObjectShell() )
				xDocShell = pRet->GetObjectShell();
		}


		Application::SetDefDialogParent( pParent );

		mnShowPage = 0;
		mbDocPreview = bDocPreview;
		maDocFile = aDocFile;
		mbUserDataDirty = TRUE;
	}

	if(bChangeMaster && (aLayoutFile != maDocFile))
	{
		// Layoutvorlage laden
		SfxObjectShellLock xLayoutDocShell;
		SfxErrorContext eEC(ERRCTX_SFX_LOADTEMPLATE,mpWindow);
		SfxItemSet* pSet = new SfxAllItemSet( pSfxApp->GetPool() );

		::Window *pParent = Application::GetDefDialogParent();
		Application::SetDefDialogParent( mpWindow );

		if(IsOwnFormat(aLayoutFile))
		{
			pSet->Put( SfxBoolItem( SID_TEMPLATE, TRUE ) );
			pSet->Put( SfxBoolItem( SID_PREVIEW, TRUE ) );

			RestorePassword( pSet, aLayoutFile );
            if( (lErr = pSfxApp->LoadTemplate( xLayoutDocShell, aLayoutFile, TRUE, pSet )) != 0 )
				ErrorHandler::HandleError(lErr);
			SavePassword( xLayoutDocShell, aLayoutFile );
		}

		Application::SetDefDialogParent( pParent );

		// die Implementierung ermitteln
		SfxObjectShell* pShell = xDocShell;
		DrawDocShell* pDocShell = PTR_CAST(DrawDocShell,pShell);
		SdDrawDocument* pDoc = pDocShell?pDocShell->GetDoc():NULL;

		pShell = xLayoutDocShell;
		pDocShell = PTR_CAST(DrawDocShell,pShell);
		SdDrawDocument* pLayoutDoc = pDocShell?pDocShell->GetDoc():NULL;

		if( pDoc && pLayoutDoc )
		{
			pDoc->SetMasterPage(0, aEmptyStr, pLayoutDoc, TRUE,  FALSE );
		}
		else
		{
			DBG_ERROR("sd::AssistentDlgImpl::UpdatePreview(), no document for preview?");
		}

		mbUserDataDirty = TRUE;
	}
	maLayoutFile = aLayoutFile;

	if(mbUserDataDirty)
		UpdateUserData();

	if ( !xDocShell.Is() || !mbPreview )
		maPreview.SetObjectShell( 0 );
	else
	{
		maPreview.SetObjectShell( xDocShell, mnShowPage );
	}

    mbPreviewUpdating = FALSE;
}

void AssistentDlgImpl::SavePassword( SfxObjectShellLock xDoc, const String& rPath )
{
	if(xDoc.Is())
	{
		SfxMedium * pMedium = xDoc->GetMedium();
		if(pMedium && pMedium->IsStorage())
		{
			SfxItemSet * pSet = pMedium->GetItemSet();
          const SfxPoolItem *pItem = 0;
          if( pSet->GetItemState(SID_PASSWORD, TRUE, &pItem) == SFX_ITEM_SET )
          {
            //TODO/MBA: testing
            String aPass( ((const SfxStringItem*)pItem)->GetValue());
            if(aPass.Len() == 0)
                return;

            PasswordEntry* pEntry = maPasswordList.First();
            while(pEntry)
            {
                if(pEntry->maPath == rPath)
                    break;

                pEntry = maPasswordList.Next();

            }

            if(pEntry == NULL)
            {
                pEntry = new PasswordEntry();
                pEntry->maPath = rPath;
                maPasswordList.Insert( pEntry );
            }

            if(pEntry)
                pEntry->maPassword = aPass;
          }
		}
	}
}

void AssistentDlgImpl::RestorePassword( SfxItemSet* pSet, const String& rPath )
{
	String aPassword( GetPassword( rPath ) );

	if(aPassword.Len())
		pSet->Put( SfxStringItem( SID_PASSWORD, aPassword ) );
}

String AssistentDlgImpl::GetPassword( const String rPath )
{
	PasswordEntry* pEntry = maPasswordList.First();
	while(pEntry)
	{
		if(pEntry->maPath == rPath)
			return pEntry->maPassword;

		pEntry = maPasswordList.Next();
	}

	return String();
}

void AssistentDlgImpl::DeletePassords()
{
	PasswordEntry* pEntry = maPasswordList.First();
	while(pEntry)
	{
		delete pEntry;
		pEntry = maPasswordList.Next();
	}
}

BOOL AssistentDlgImpl::IsOwnFormat( const String& rPath )
{
	INetURLObject	aURL( rPath );
	String			aExt( aURL.GetFileExtension() );

	DBG_ASSERT( aURL.GetProtocol() != INET_PROT_NOT_VALID, "invalid URL" );

	return !aExt.EqualsIgnoreCaseAscii( "ppt" );
}




String AssistentDlgImpl::GetUiTextForCommand (const ::rtl::OUString& sCommandURL)
{
    String sLabel;
    Reference<container::XNameAccess> xUICommandLabels;

    try
    {
        do
        {
            if (sCommandURL.getLength() ==  0)
                break;

            // Retrieve popup menu labels
            Reference<lang::XMultiServiceFactory> xFactory (
                ::comphelper::getProcessServiceFactory ());
            if ( ! xFactory.is())
                break;

            ::rtl::OUString sModuleIdentifier (
                RTL_CONSTASCII_USTRINGPARAM("com.sun.star.presentation.PresentationDocument"));
            Reference<container::XNameAccess> xNameAccess (
                xFactory->createInstance(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(
                    "com.sun.star.frame.UICommandDescription"))),
                UNO_QUERY);
            if ( ! xNameAccess.is())
                break;
            Any a = xNameAccess->getByName(sModuleIdentifier);
            a >>= xUICommandLabels;
            if ( ! xUICommandLabels.is())
                break;

            ::rtl::OUString sString;
            Sequence<beans::PropertyValue> aProperties;
            Any aAny (xUICommandLabels->getByName(sCommandURL));
            if (aAny >>= aProperties)
            {
                sal_Int32 nCount (aProperties.getLength());
                for (sal_Int32 i=0; i<nCount; i++)
                {
                    ::rtl::OUString sPropertyName (aProperties[i].Name);
                    if (sPropertyName.equalsAscii("Label"))
                    {
                        aProperties[i].Value >>= sString;
                        break;
                    }
                }
            }
            sLabel = sString;
        }
        while(false);
    }
    catch (com::sun::star::uno::Exception& rException)
    {
        (void)rException;
    }

    return sLabel;
}




Image AssistentDlgImpl::GetUiIconForCommand (const ::rtl::OUString& sCommandURL)
{
    Image aIcon;
    Reference<container::XNameAccess> xUICommandLabels;

    try
    {
        do
        {
            if (sCommandURL.getLength() ==  0)
                break;

            // Retrieve popup menu labels
            Reference<lang::XMultiServiceFactory> xFactory (
                ::comphelper::getProcessServiceFactory ());
            if ( ! xFactory.is())
                break;

            ::rtl::OUString sModuleIdentifier (
                RTL_CONSTASCII_USTRINGPARAM("com.sun.star.presentation.PresentationDocument"));

            Reference<com::sun::star::ui::XModuleUIConfigurationManagerSupplier> xSupplier (
                xFactory->createInstance(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(
                    "com.sun.star.ui.ModuleUIConfigurationManagerSupplier"))),
                UNO_QUERY_THROW);

            Reference<com::sun::star::ui::XUIConfigurationManager> xManager (
                xSupplier->getUIConfigurationManager(sModuleIdentifier));
            if ( ! xManager.is())
                break;

            Reference<com::sun::star::ui::XImageManager> xImageManager (
                xManager->getImageManager(),
                UNO_QUERY_THROW);

            Sequence<rtl::OUString> aCommandList(1);
            aCommandList[0] = sCommandURL;
            Sequence<Reference<graphic::XGraphic> > xIconList (
                xImageManager->getImages(0,aCommandList));
            if ( ! xIconList.hasElements())
                break;

            aIcon = Graphic(xIconList[0]).GetBitmapEx();
        }
        while(false);
    }
    catch (com::sun::star::uno::Exception& rException)
    {
        (void)rException;
    }

    return aIcon;
}


//////////////////////////////////////////////

AssistentDlg::AssistentDlg(Window* pParent, BOOL bAutoPilot) :
	ModalDialog(pParent,SdResId(DLG_ASS))
{
	Link aFinishLink = LINK(this,AssistentDlg, FinishHdl);
	mpImpl = new AssistentDlgImpl( this, aFinishLink, bAutoPilot );

	//Buttonbelegung
	mpImpl->maFinishButton.SetClickHdl(LINK(this,AssistentDlg,FinishHdl));

	FreeResource();
}

IMPL_LINK( AssistentDlg, FinishHdl, OKButton *, EMPTYARG )
{
    if( GetStartType() == ST_OPEN )
	{
        //if we do not have a file here asked for one before ending the dialog
        String aFileToOpen = GetDocPath();
		if(aFileToOpen.Len() == 0)
		{
            sfx2::FileDialogHelper aFileDlg( WB_OPEN, ::String::CreateFromAscii("simpress") );

			if ( aFileDlg.Execute() == ERRCODE_NONE )
				aFileToOpen = aFileDlg.GetPath();
            if( aFileToOpen.Len() == 0)
                return 1;
            else
            {
                //add the selected file to the recent-file-listbox and select the new entry
                //this is necessary for 'GetDocPath()' returning the selected file after end of dialog

                INetURLObject aURL;
                aURL.SetSmartURL(aFileToOpen);
                mpImpl->maOpenFilesList.push_back (new String (aURL.GetMainURL( INetURLObject::NO_DECODE )));
                USHORT nNewPos = mpImpl->mpPage1OpenLB->InsertEntry(aURL.getName());
                mpImpl->mpPage1OpenLB->SelectEntryPos(nNewPos);
            }
		}
    }

	//Ende
	mpImpl->EndDialog(RET_OK);
	EndDialog(RET_OK);
	return 0;
}

AssistentDlg::~AssistentDlg()
{
	delete mpImpl;
}


SfxObjectShellLock AssistentDlg::GetDocument()
{
	return mpImpl->GetDocument();
}

String AssistentDlg::GetTopic() const
{
	return mpImpl->mpPage4AskTopicEDT->GetText();
}

String AssistentDlg::GetUserName() const
{
	return mpImpl->mpPage4AskNameEDT->GetText();
}

String AssistentDlg::GetInformation() const
{
	return mpImpl->mpPage4AskInfoEDT->GetText();
}

OutputType AssistentDlg::GetOutputMedium() const
{
	if(mpImpl->mpPage2Medium1RB->IsChecked())
		return OUTPUT_PRESENTATION;
	else if(mpImpl->mpPage2Medium2RB->IsChecked())
		return OUTPUT_SLIDE;
	else if(mpImpl->mpPage2Medium3RB->IsChecked())
		return OUTPUT_OVERHEAD;
	else if(mpImpl->mpPage2Medium4RB->IsChecked())
		return OUTPUT_PAGE;
	else
		return OUTPUT_ORIGINAL;
}

BOOL AssistentDlg::IsSummary() const
{
	return mpImpl->mpPage5SummaryCB->IsChecked();
}

StartType AssistentDlg::GetStartType() const
{
	return mpImpl->GetStartType();
}

String AssistentDlg::GetDocPath() const
{
	return mpImpl->GetDocFileName();
}

BOOL AssistentDlg::GetStartWithFlag() const
{
	return !mpImpl->maStartWithFlag.IsChecked();
}

BOOL AssistentDlg::IsDocEmpty() const
{
	return mpImpl->GetDocFileName().Len() == 0 &&
		   mpImpl->GetLayoutFileName().Len() == 0;
}

String AssistentDlg::GetPassword()
{
	return mpImpl->GetPassword( mpImpl->maDocFile );
}




//===== NextButton ============================================================

NextButton::NextButton (::Window* pParent, const ResId& rResId)
    : maNextButton1(pParent, rResId),
      maNextButton2(pParent, rResId),
      mbIsFirstButtonActive(true)
{
    // Hide the unused button.
    maNextButton2.Hide();
}




void NextButton::ForceFocusEventBroadcast (void)
{
    // Hide the currently visible button and show and focus the other one.
    if (mbIsFirstButtonActive)
    {
        mbIsFirstButtonActive = false;
        maNextButton2.Show();
        maNextButton2.GrabFocus();
        maNextButton1.Hide();
    }
    else
    {
        mbIsFirstButtonActive = true;
        maNextButton1.Show();
        maNextButton1.GrabFocus();
        maNextButton2.Hide();
    }
}
	



void NextButton::SetClickHdl (const Link& rLink)
{
    // Forward the setting of the click handler to the two buttons
    // regardless of which one is currently visible.
    maNextButton1.SetClickHdl(rLink);
    maNextButton2.SetClickHdl(rLink);
}




bool NextButton::IsEnabled (void)
{
    // Because the buttons are both either enabled or disabled, it is
    // sufficient to ask one to determine the state.
    return maNextButton1.IsEnabled();
}




void NextButton::Enable (bool bEnable)
{
    // Enable or disable both buttons but do not change visibility or focus.
    maNextButton1.Enable(bEnable);
    maNextButton2.Enable(bEnable);
}
