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
#ifndef _SVX_TABPAGE_INET_HYPERLINK_HXX
#define _SVX_TABPAGE_INET_HYPERLINK_HXX

#include <hyperdlg.hxx>
#include "cuihyperdlg.hxx"
#include "hltpbase.hxx"

/*************************************************************************
|*
|* Tabpage : Hyperlink - Internet
|*
\************************************************************************/

class SvxHyperlinkInternetTp : public SvxHyperlinkTabPageBase
{
private:
	FixedLine			maGrpLinkTyp;
	RadioButton         maRbtLinktypInternet;
	RadioButton         maRbtLinktypFTP;
	RadioButton         maRbtLinktypTelnet;
	FixedText           maFtTarget;
	SvxHyperURLBox		maCbbTarget;
	FixedText			maFtLogin;
	Edit				maEdLogin;
	FixedText			maFtPassword;
	Edit				maEdPassword;
	CheckBox			maCbAnonymous;
	ImageButton			maBtBrowse;
	ImageButton			maBtTarget;

	String				maStrOldUser;
	String				maStrOldPassword;

	BOOL				mbMarkWndOpen;

	String				maStrStdDocURL;

    DECL_LINK (Click_SmartProtocol_Impl  , void * );	    // Radiobutton clicked: Type Internet, FTP or Telnet
	DECL_LINK (ClickAnonymousHdl_Impl    , void * );		// Checkbox : Anonymer Benutzer
	DECL_LINK (ClickBrowseHdl_Impl       , void * );		// Button : Browse
	DECL_LINK (ClickTargetHdl_Impl       , void * );		// Button : Ziel
	DECL_LINK (ModifiedLoginHdl_Impl     , void * );		// Contens of editfield "Login" modified
	DECL_LINK (LostFocusTargetHdl_Impl   , void * );		// Combobox "Target" lost its focus
	DECL_LINK (ModifiedTargetHdl_Impl    , void * );		// Contens of editfield "Target" modified

	DECL_LINK (TimeoutHdl_Impl           , Timer * );		// Handler for timer -timeout


    void    SetScheme( const String& aScheme );
    void    RemoveImproperProtocol(const String& aProperScheme);
    String  GetSchemeFromButtons() const;
	INetProtocol GetSmartProtocolFromButtons() const;

    String CreateAbsoluteURL() const;

    void   setAnonymousFTPUser();
    void   setFTPUser(const String& rUser, const String& rPassword);
    void   RefreshMarkWindow();

protected:
	virtual void FillDlgFields	   ( String& aStrURL );
	virtual void GetCurentItemData ( String& aStrURL, String& aStrName, 
		                             String& aStrIntName, String& aStrFrame, 
									 SvxLinkInsertMode& eMode );
	virtual BOOL ShouldOpenMarkWnd () {return ( mbMarkWndOpen && maRbtLinktypInternet.IsChecked() );}
	virtual void SetMarkWndShouldOpen (BOOL bOpen) {mbMarkWndOpen=bOpen;}

public:
	SvxHyperlinkInternetTp ( Window *pParent, const SfxItemSet& rItemSet);
	~SvxHyperlinkInternetTp ();

	static  IconChoicePage* Create( Window* pWindow, const SfxItemSet& rItemSet );

	virtual void		SetMarkStr ( String& aStrMark );
	virtual void		SetOnlineMode( BOOL bEnable );

	virtual void		SetInitFocus();
};


#endif // _SVX_TABPAGE_INET_HYPERLINK_HXX
