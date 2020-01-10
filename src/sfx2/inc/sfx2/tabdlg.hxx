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
#ifndef _SFXTABDLG_HXX
#define _SFXTABDLG_HXX

#include "sal/config.h"
#include "sfx2/dllapi.h"
#include "sal/types.h"
#include <vcl/tabdlg.hxx>
#include <vcl/button.hxx>
#include <vcl/tabctrl.hxx>
#include <vcl/tabpage.hxx>
#include <svtools/itempool.hxx>
#include <svtools/itemset.hxx>
#include <com/sun/star/frame/XFrame.hpp>

#if ENABLE_LAYOUT
#include <layout/layout.hxx>
namespace layout { class SfxTabDialog; class SfxTabDialogController; }
#endif /* ENABLE_LAYOUT */

class SfxPoolItem;
class SfxTabDialog;
class SfxViewFrame;
class SfxTabPage;
class SfxBindings;

#ifndef ENABLE_LAYOUT_SFX_TABDIALOG
#define ENABLE_LAYOUT_SFX_TABDIALOG 0
#define NAMESPACE_LAYOUT_SFX_TABDIALOG
#define END_NAMESPACE_LAYOUT_SFX_TABDIALOG
#define LAYOUT_NS_SFX_TABDIALOG
#endif /* !ENABLE_LAYOUT_SFX_TABDIALOG*/

typedef SfxTabPage* (*CreateTabPage)(Window *pParent, const SfxItemSet &rAttrSet);
typedef USHORT*		(*GetTabPageRanges)(); // liefert internationale Which-Wert
struct TabPageImpl;
class SfxUs_Impl;

#if ENABLE_LAYOUT_SFX_TABDIALOG
#include <layout/layout-pre.hxx>
#undef SfxTabDialog
#undef SfxTabPage
#endif /* ENABLE_LAYOUT_SFX_TABDIALOG */

NAMESPACE_LAYOUT_SFX_TABDIALOG

struct TabDlg_Impl;

#define ID_TABCONTROL	1
#define RET_USER 		100
#define RET_USER_CANCEL	101

class SFX2_DLLPUBLIC SfxTabDialogItem: public SfxSetItem
{
public:
	TYPEINFO();
                            SfxTabDialogItem( USHORT nId, const SfxItemSet& rItemSet );
                            SfxTabDialogItem(const SfxTabDialogItem& rAttr, SfxItemPool* pItemPool=NULL);
	virtual SfxPoolItem*    Clone(SfxItemPool* pToPool) const;
	virtual SfxPoolItem*    Create(SvStream& rStream, USHORT nVersion) const;
};

class SFX2_DLLPUBLIC SfxTabDialog : public TabDialog
{
private:
friend class SfxTabPage;
friend class SfxTabDialogController;

	SfxViewFrame*	pFrame;

	TabControl		aTabCtrl;
	OKButton		aOKBtn;
	PushButton*		pUserBtn;
	CancelButton	aCancelBtn;
	HelpButton		aHelpBtn;
	PushButton		aResetBtn;
	PushButton		aBaseFmtBtn;

	const SfxItemSet*	pSet;
	SfxItemSet*			pOutSet;
	TabDlg_Impl*		pImpl;
	USHORT*				pRanges;
	sal_uInt32			nResId;
	USHORT				nAppPageId;
	BOOL				bItemsReset;
	BOOL				bFmt;

//#if 0 // _SOLAR__PRIVATE
	DECL_DLLPRIVATE_LINK( ActivatePageHdl, TabControl * );
	DECL_DLLPRIVATE_LINK( DeactivatePageHdl, TabControl * );
	DECL_DLLPRIVATE_LINK( OkHdl, Button * );
	DECL_DLLPRIVATE_LINK( ResetHdl, Button * );
	DECL_DLLPRIVATE_LINK( BaseFmtHdl, Button * );
	DECL_DLLPRIVATE_LINK( UserHdl, Button * );
	DECL_DLLPRIVATE_LINK( CancelHdl, Button * );
	SAL_DLLPRIVATE void Init_Impl(BOOL, const String *);
//#endif

protected:
	virtual short				Ok();
	// wird im Sfx gel"oscht!
	virtual SfxItemSet*			CreateInputItemSet( USHORT nId );
	// wird *nicht* im Sfx gel"oscht!
	virtual const SfxItemSet*	GetRefreshedSet();
	virtual void				PageCreated( USHORT nId, SfxTabPage &rPage );
    virtual long                Notify( NotifyEvent& rNEvt );

	SfxItemSet*		pExampleSet;
	SfxItemSet*		GetInputSetImpl();
	SfxTabPage*		GetTabPage( USHORT nPageId ) const;

	BOOL			IsInOK() const;
	/**	prepare to leace the current page. Calls the DeactivatePage method of the current page, (if necessary),
		handles the item sets to copy.
		@return TRUE if it is allowed to leave the current page, FALSE otherwise
	*/
	bool PrepareLeaveCurrentPage();

public:
    SfxTabDialog( Window* pParent, const ResId &rResId, USHORT nSetId, SfxBindings& rBindings,
				  BOOL bEditFmt = FALSE, const String *pUserButtonText = 0 );
	SfxTabDialog( Window* pParent, const ResId &rResId, const SfxItemSet * = 0,
				  BOOL bEditFmt = FALSE, const String *pUserButtonText = 0 );
	SfxTabDialog( SfxViewFrame *pViewFrame, Window* pParent, const ResId &rResId,
				  const SfxItemSet * = 0, BOOL bEditFmt = FALSE,
				  const String *pUserButtonText = 0 );
	~SfxTabDialog();

	void				AddTabPage( USHORT nId,
									CreateTabPage pCreateFunc,		// != 0
									GetTabPageRanges pRangesFunc,	// darf 0 sein
									BOOL bItemsOnDemand = FALSE);
	void				AddTabPage( USHORT nId,
									const String &rRiderText,
									CreateTabPage pCreateFunc,		// != 0
									GetTabPageRanges pRangesFunc,	// darf 0 sein
									BOOL bItemsOnDemand = FALSE,
									USHORT nPos = TAB_APPEND);
	void				AddTabPage( USHORT nId,
									const Bitmap &rRiderBitmap,
									CreateTabPage pCreateFunc,		// != 0
									GetTabPageRanges pRangesFunc,	// darf 0 sein
									BOOL bItemsOnDemand = FALSE,
									USHORT nPos = TAB_APPEND);

	void				AddTabPage( USHORT nId,
									BOOL bItemsOnDemand = FALSE);
	void				AddTabPage( USHORT nId,
									const String &rRiderText,
									BOOL bItemsOnDemand = FALSE,
									USHORT nPos = TAB_APPEND);
	void				AddTabPage( USHORT nId,
									const Bitmap &rRiderBitmap,
									BOOL bItemsOnDemand = FALSE,
									USHORT nPos = TAB_APPEND);

	void				RemoveTabPage( USHORT nId );

	void				SetCurPageId( USHORT nId ) { nAppPageId = nId; }
	USHORT				GetCurPageId() const
							{ return aTabCtrl.GetCurPageId(); }
	void				ShowPage( USHORT nId );

						// liefert ggf. per Map konvertierte lokale Slots
	const USHORT*		GetInputRanges( const SfxItemPool& );
	void				SetInputSet( const SfxItemSet* pInSet );
	const SfxItemSet*	GetOutputItemSet() const { return pOutSet; }
	const SfxItemSet*	GetOutputItemSet( USHORT nId ) const;
	int					FillOutputItemSet();
	BOOL IsFormat() const { return bFmt; }

	const OKButton& 	GetOKButton() const { return aOKBtn; }
	OKButton&			GetOKButton() { return aOKBtn; }
	const CancelButton& GetCancelButton() const { return aCancelBtn; }
	CancelButton&		GetCancelButton() { return aCancelBtn; }
	const HelpButton&	GetHelpButton() const { return aHelpBtn; }
	HelpButton&			GetHelpButton() { return aHelpBtn; }
	const PushButton& 	GetResetButton() const { return aResetBtn; }
	PushButton&			GetResetButton() { return aResetBtn; }

	const PushButton*	GetApplyButton() const;
	PushButton*			GetApplyButton();

	const PushButton*	GetUserButton() const { return pUserBtn; }
	PushButton*			GetUserButton() { return pUserBtn; }
	void				RemoveResetButton();

	short				Execute();
    void                StartExecuteModal( const Link& rEndDialogHdl );
	void				Start( BOOL bShow = TRUE );

#if !ENABLE_LAYOUT_SFX_TABDIALOG
	const SfxItemSet*	GetExampleSet() const { return pExampleSet; }
#else /* ENABLE_LAYOUT_SFX_TABDIALOG */
    SfxItemSet*	GetExampleSet() const { return 0; }
#endif /* ENABLE_LAYOUT_SFX_TABDIALOG */
	SfxViewFrame*		GetViewFrame() const { return pFrame; }

	void				EnableApplyButton(BOOL bEnable = TRUE);
	BOOL				IsApplyButtonEnabled() const;
	void				SetApplyHandler(const Link& _rHdl);
	Link				GetApplyHandler() const;

//#if 0 // _SOLAR__PRIVATE
	SAL_DLLPRIVATE void Start_Impl();
	SAL_DLLPRIVATE BOOL OK_Impl() { return PrepareLeaveCurrentPage(); }
//#endif
};

END_NAMESPACE_LAYOUT_SFX_TABDIALOG

#if ENABLE_LAYOUT_SFX_TABDIALOG
#include <layout/layout-post.hxx>
#endif /* ENABLE_LAYOUT_SFX_TABDIALOG */

#if !ENABLE_LAYOUT_SFX_TABDIALOG

namespace sfx { class ItemConnectionBase; }

class SFX2_DLLPUBLIC SfxTabPage: public TabPage
{
friend class SfxTabDialog;
    #if ENABLE_LAYOUT
     friend class layout::SfxTabDialog;
    #endif

private:
	const SfxItemSet*	pSet;
	String				aUserString;
	BOOL				bHasExchangeSupport;
	SfxTabDialog*		pTabDlg;
	TabPageImpl*		pImpl;

	SAL_DLLPRIVATE void SetTabDialog( SfxTabDialog* pNew ) { pTabDlg = pNew; }
	SAL_DLLPRIVATE void SetInputSet( const SfxItemSet* pNew ) { pSet = pNew; }

protected:
	SfxTabPage( Window *pParent, const ResId &, const SfxItemSet &rAttrSet );
	SfxTabPage( Window *pParent, WinBits nStyle, const SfxItemSet &rAttrSet );

	USHORT				GetSlot( USHORT nWhich ) const
							{ return pSet->GetPool()->GetSlotId( nWhich ); }
	USHORT				GetWhich( USHORT nSlot, sal_Bool bDeep = sal_True ) const
							{ return pSet->GetPool()->GetWhich( nSlot, bDeep ); }
	const SfxPoolItem* 	GetOldItem( const SfxItemSet& rSet, USHORT nSlot, sal_Bool bDeep = sal_True );
	const SfxPoolItem* 	GetExchangeItem( const SfxItemSet& rSet, USHORT nSlot );
	SfxTabDialog*		GetTabDialog() const { return pTabDlg; }

    void                AddItemConnection( sfx::ItemConnectionBase* pConnection );

public:
	virtual             ~SfxTabPage();

	const SfxItemSet&	GetItemSet() const { return *pSet; }

    virtual BOOL        FillItemSet( SfxItemSet& );
    virtual void        Reset( const SfxItemSet& );

	BOOL				HasExchangeSupport() const
							{ return bHasExchangeSupport; }
	void				SetExchangeSupport( BOOL bNew = TRUE )
							{ bHasExchangeSupport = bNew; }

	enum sfxpg {
		KEEP_PAGE = 0x0000,		// Fehlerbehandlung; Seite nicht wechseln
			// 2. F"ullen eines ItemSets f"ur die Aktualilsierung
			// "ubergeordneter Beispiele; dieser Pointer kann immer
			// NULL sein!!
		LEAVE_PAGE = 0x0001,
			// Set aktualisieren und andere Page aktualisieren
		REFRESH_SET = 0x0002
	};

        using TabPage::ActivatePage;
        using TabPage::DeactivatePage;
	virtual void            ActivatePage( const SfxItemSet& );
	virtual int             DeactivatePage( SfxItemSet* pSet = 0 );
	void                    SetUserData(const String& rString)
							{ aUserString = rString; }
	String				GetUserData() { return aUserString; }
	virtual void		FillUserData();
	virtual BOOL		IsReadOnly() const;
	virtual void PageCreated (SfxAllItemSet aSet); //add CHINA001
	static const SfxPoolItem* GetItem( const SfxItemSet& rSet, USHORT nSlot, sal_Bool bDeep = sal_True );

    void SetFrame(const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& xFrame);
    ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame > GetFrame();
};

#endif /* !ENABLE_LAYOUT_SFX_TABDIALOG */

#endif

