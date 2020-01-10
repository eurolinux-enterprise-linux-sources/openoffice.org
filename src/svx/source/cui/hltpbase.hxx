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
#ifndef _SVX_TABBASE_HYPERLINK_HXX
#define _SVX_TABBASE_HYPERLINK_HXX

#define INET_TELNET_SCHEME		"telnet://"

#include <sfx2/app.hxx>
#include <sfx2/tabdlg.hxx>
#include <vcl/group.hxx>
#ifndef _SV_BUTTON_HXX
#include <vcl/button.hxx>
#endif
#include <vcl/fixed.hxx>
#include <vcl/combobox.hxx>
#include <vcl/edit.hxx>
#include <vcl/lstbox.hxx>
#include <tools/urlobj.hxx>
#include <svtools/stritem.hxx>
#include <svtools/eitem.hxx>
#include <svtools/transfer.hxx>
#include <sfx2/dispatch.hxx>
#include <vcl/msgbox.hxx>
#include <sfx2/fcontnr.hxx>
#include <svtools/inettbc.hxx>
#include <vcl/timer.hxx>

#include <svx/dialmgr.hxx>
#include <sfx2/docfile.hxx>
#include <svx/dialogs.hrc>

#include <com/sun/star/frame/XFrame.hpp>

#ifndef _SVX_HELPID_HRC
#include "helpid.hrc"
#endif

#include "hlnkitem.hxx"

#include "hlmarkwn.hxx"
#include "iconcdlg.hxx"


/*************************************************************************
|*
|* ComboBox-Control, wich is filled with all current framenames
|*
\************************************************************************/

class SvxFramesComboBox : public ComboBox
{
public:
    SvxFramesComboBox (Window* pParent, const ResId& rResId, SfxDispatcher* pDispatch);
	~SvxFramesComboBox ();
};

/*************************************************************************
|*
|* ComboBox-Control for URL's with History and Autocompletion
|*
\************************************************************************/

class SvxHyperURLBox : public SvtURLBox, public DropTargetHelper
{
private:
	BOOL   mbAccessAddress;

//	String GetAllEmailNamesFromDragItem( USHORT nItem );

protected:

	virtual sal_Int8    AcceptDrop( const AcceptDropEvent& rEvt );
	virtual sal_Int8    ExecuteDrop( const ExecuteDropEvent& rEvt );

	virtual long        Notify( NotifyEvent& rNEvt );
	virtual void        Select();
	virtual void        Modify();
	virtual long        PreNotify( NotifyEvent& rNEvt );

public:
	SvxHyperURLBox( Window* pParent, INetProtocol eSmart = INET_PROT_FILE, BOOL bAddresses = FALSE );

};

/*************************************************************************
|*
|* Tabpage : Basisclass
|*
\************************************************************************/

class SvxHyperlinkTabPageBase : public IconChoicePage
{
private:
	FixedLine           *mpGrpMore;
	FixedText			*mpFtFrame;
	SvxFramesComboBox	*mpCbbFrame;
	FixedText			*mpFtForm;
	ListBox				*mpLbForm;
	FixedText			*mpFtIndication;
	Edit				*mpEdIndication;
	FixedText			*mpFtText;
	Edit				*mpEdText;
	ImageButton			*mpBtScript;

    sal_Bool            mbIsCloseDisabled;

    ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >
                        mxDocumentFrame;

protected:
	Window*				mpDialog;

	BOOL				mbStdControlsInit;

	String				maStrInitURL;

	Timer				maTimer;

	SvxHlinkDlgMarkWnd* mpMarkWnd;

	void InitStdControls ();
	virtual void FillStandardDlgFields ( SvxHyperlinkItem* pHyperlinkItem );
	virtual void FillDlgFields         ( String& aStrURL ) = 0;
	virtual void GetCurentItemData     ( String& aStrURL, String& aStrName,
		                                 String& aStrIntName, String& aStrFrame,
									     SvxLinkInsertMode& eMode ) = 0;
	virtual String CreateUiNameFromURL( const String& aStrURL );

	void		 GetDataFromCommonFields( String& aStrName,
										  String& aStrIntName, String& aStrFrame,
										  SvxLinkInsertMode& eMode );

	DECL_LINK (ClickScriptHdl_Impl, void * );		// Button : Script

	String				aEmptyStr;

	BOOL			FileExists( const INetURLObject& rURL );
    static String   GetSchemeFromURL( String aStrURL );

    inline void     DisableClose( sal_Bool _bDisable ) { mbIsCloseDisabled = _bDisable; }

public:
	SvxHyperlinkTabPageBase (
        Window *pParent,
        const ResId &rResId,
        const SfxItemSet& rItemSet
    );
	virtual ~SvxHyperlinkTabPageBase ();

    void    SetDocumentFrame(
        const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& rxDocumentFrame )
    {
        mxDocumentFrame = rxDocumentFrame;
    }

	virtual BOOL AskApply ();
	virtual void DoApply ();
	virtual void SetOnlineMode( BOOL bEnable );
	virtual void SetInitFocus();
	virtual void SetMarkStr ( String& aStrMark );
	virtual void Reset( const SfxItemSet& );
	virtual BOOL FillItemSet( SfxItemSet& );
	virtual void ActivatePage( const SfxItemSet& rItemSet );
	virtual int	 DeactivatePage( SfxItemSet* pSet = 0 );

	BOOL IsMarkWndVisible ()      { return ((Window*)mpMarkWnd)->IsVisible(); }
	Size GetSizeExtraWnd ()		  { return ( mpMarkWnd->GetSizePixel() ); }
	BOOL MoveToExtraWnd ( Point aNewPos, BOOL bDisConnectDlg = FALSE );

    virtual void        ActivatePage();
    virtual void        DeactivatePage();
    virtual sal_Bool    QueryClose();

protected:
	virtual BOOL ShouldOpenMarkWnd();
	virtual void SetMarkWndShouldOpen(BOOL bOpen);

	void ShowMarkWnd ();
	void HideMarkWnd ()	          { ( ( Window* ) mpMarkWnd )->Hide(); }
	void InvalidateMarkWnd ()     { ( ( Window* ) mpMarkWnd )->Invalidate(); }

	SfxDispatcher* GetDispatcher() const;

	USHORT             GetMacroEvents();
	SvxMacroTableDtor* GetMacroTable();

	BOOL IsHTMLDoc() const;
};

#endif // _SVX_TABBASE_HYPERLINK_HXX

