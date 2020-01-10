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
#ifndef _SFX_TEMPDLGI_HXX
#define _SFX_TEMPDLGI_HXX

class SfxTemplateControllerItem;

#ifndef _BUTTON_HXX //autogen
#include <vcl/button.hxx>
#endif
#ifndef _TOOLBOX_HXX //autogen
#include <vcl/toolbox.hxx>
#endif
#ifndef _LSTBOX_HXX //autogen
#include <vcl/lstbox.hxx>
#endif
#include <svtools/lstner.hxx>
#include <svtools/svtreebx.hxx>
#include <svtools/eitem.hxx>

#define _SVSTDARR_USHORTS
#include <svtools/svstdarr.hxx>		// SvUShorts

#include <rsc/rscsfx.hxx>
#include <tools/rtti.hxx>

#include <sfx2/childwin.hxx>
#include <sfx2/templdlg.hxx>

class SfxStyleFamilies;
class SfxStyleFamilyItem;
class SfxTemplateItem;
class SfxBindings;
class SfxStyleSheetBasePool;
class SvTreeListBox ;
class StyleTreeListBox_Impl;
class SfxTemplateDialog_Impl;
class SfxCommonTemplateDialog_Impl;
class SfxTemplateDialogWrapper;
class SfxDockingWindow;

namespace com { namespace sun { namespace star { namespace frame { class XModuleManager; } } } }

// class DropListBox_Impl ------------------------------------------------

class DropListBox_Impl : public SvTreeListBox
{
private:
    DECL_LINK( OnAsyncExecuteDrop, SvLBoxEntry* );
    DECL_LINK( OnAsyncExecuteError, void* );

protected:
    SfxCommonTemplateDialog_Impl* pDialog;
    USHORT                        nModifier;

public:
    DropListBox_Impl( Window* pParent, const ResId& rId, SfxCommonTemplateDialog_Impl* pD ) :
        SvTreeListBox( pParent, rId ), pDialog( pD ) {}
    DropListBox_Impl( Window* pParent, WinBits nWinBits, SfxCommonTemplateDialog_Impl* pD ) :
        SvTreeListBox( pParent, nWinBits ), pDialog( pD ) {}

    virtual void     MouseButtonDown( const MouseEvent& rMEvt );
    virtual sal_Int8 AcceptDrop( const AcceptDropEvent& rEvt );
    using SvLBox::ExecuteDrop;
    virtual sal_Int8 ExecuteDrop( const ExecuteDropEvent& rEvt );

    USHORT           GetModifier() const { return nModifier; }

    virtual long     Notify( NotifyEvent& rNEvt );
};

// class SfxActionListBox ------------------------------------------------

class SfxActionListBox : public DropListBox_Impl
{
protected:
public:
	SfxActionListBox( SfxCommonTemplateDialog_Impl* pParent, WinBits nWinBits );
	SfxActionListBox( SfxCommonTemplateDialog_Impl* pParent, const ResId &rResId );

	virtual PopupMenu*	CreateContextMenu( void );
};

// class SfxCommonTemplateDialog_Impl ------------------------------------

struct Deleted
{
    bool bDead;

    Deleted() : bDead(false) {}

    inline bool operator()() { return bDead; }
};

class SfxCommonTemplateDialog_Impl : public SfxListener
{
private:
	class ISfxTemplateCommon_Impl : public ISfxTemplateCommon
	{
	private:
		SfxCommonTemplateDialog_Impl* pDialog;
	public:
		ISfxTemplateCommon_Impl( SfxCommonTemplateDialog_Impl* pDialogP ) : pDialog( pDialogP ) {}
		virtual SfxStyleFamily GetActualFamily() const { return pDialog->GetActualFamily(); }
		virtual String GetSelectedEntry() const { return pDialog->GetSelectedEntry(); }
	};

	ISfxTemplateCommon_Impl		aISfxTemplateCommon;

	void	ReadResource();
	void	ClearResource();

protected:
#define MAX_FAMILIES			5
#define COUNT_BOUND_FUNC		13

#define UPDATE_FAMILY_LIST		0x0001
#define UPDATE_FAMILY			0x0002

	friend class DropListBox_Impl;
	friend class SfxTemplateControllerItem;
	friend class SfxTemplateDialogWrapper;

	SfxBindings*				pBindings;
	SfxTemplateControllerItem*	pBoundItems[COUNT_BOUND_FUNC];

	Window*						pWindow;
	SfxModule*					pModule;
	Timer*						pTimer;

	ResId*						m_pStyleFamiliesId;
	SfxStyleFamilies*			pStyleFamilies;
	SfxTemplateItem*			pFamilyState[MAX_FAMILIES];
	SfxStyleSheetBasePool*		pStyleSheetPool;
	SvTreeListBox*				pTreeBox;
	SfxObjectShell*				pCurObjShell;
    ::com::sun::star::uno::Reference< ::com::sun::star::frame::XModuleManager >
                                xModuleManager;
    Deleted*                    pbDeleted;

	SfxActionListBox			aFmtLb;
	ListBox						aFilterLb;
	Size						aSize;

	USHORT						nActFamily;	// Id in der ToolBox = Position - 1
	USHORT						nActFilter;	// FilterIdx
	USHORT						nAppFilter;	// Filter, den die Applikation gesetzt hat (fuer automatisch)

	BOOL						bDontUpdate				:1,
								bIsWater				:1,
								bEnabled				:1,
								bUpdate					:1,
								bUpdateFamily			:1,
								bCanEdit				:1,
								bCanDel					:1,
								bCanNew					:1,
								bWaterDisabled			:1,
								bNewByExampleDisabled	:1,
								bUpdateByExampleDisabled:1,
								bTreeDrag				:1,
								bHierarchical			:1,
								bBindingUpdate			:1;

	DECL_LINK( FilterSelectHdl, ListBox * );
	DECL_LINK( FmtSelectHdl, SvTreeListBox * );
	DECL_LINK( ApplyHdl, Control * );
	DECL_LINK( DropHdl, StyleTreeListBox_Impl * );
	DECL_LINK( TimeOut, Timer * );


	virtual void		EnableItem( USHORT /*nMesId*/, BOOL /*bCheck*/ = TRUE ) {}
	virtual void		CheckItem( USHORT /*nMesId*/, BOOL /*bCheck*/ = TRUE ) {}
	virtual BOOL		IsCheckedItem( USHORT /*nMesId*/ ) { return TRUE; }
	virtual void		LoadedFamilies() {}
	virtual void		Update() { UpdateStyles_Impl(UPDATE_FAMILY_LIST); }
	virtual void		InvalidateBindings();
	virtual void		InsertFamilyItem( USHORT nId, const SfxStyleFamilyItem* pIten ) = 0;
	virtual void		EnableFamilyItem( USHORT nId, BOOL bEnabled = TRUE ) = 0;
	virtual void		ClearFamilyList() = 0;
    virtual void        ReplaceUpdateButtonByMenu();

        void				NewHdl( void* );
	void				EditHdl( void* );
	void				DeleteHdl( void* );

	BOOL				Execute_Impl( USHORT nId, const String& rStr, const String& rRefStr,
									  USHORT nFamily, USHORT nMask = 0,
									  USHORT* pIdx = NULL, const USHORT* pModifier = NULL );

	void						UpdateStyles_Impl(USHORT nFlags);
	const SfxStyleFamilyItem*	GetFamilyItem_Impl() const;
	BOOL						IsInitialized() { return nActFamily != 0xffff; }
	void						ResetFocus();
	void						EnableDelete();
	void						Initialize();

	void				FilterSelect( USHORT nFilterIdx, BOOL bForce = FALSE );
	void				SetFamilyState( USHORT nSlotId, const SfxTemplateItem* );
	void				SetWaterCanState( const SfxBoolItem* pItem );

	void				SelectStyle( const String& rStyle );
	BOOL				HasSelectedStyle() const;
	void				FillTreeBox();
	void				Update_Impl();
	void				UpdateFamily_Impl();

	// In welchem FamilyState muss ich nachsehen, um die Info der i-ten
	// Family in der pStyleFamilies zu bekommen.
	USHORT				StyleNrToInfoOffset( USHORT i );
	USHORT				InfoOffsetToStyleNr( USHORT i );

	void				Notify( SfxBroadcaster& rBC, const SfxHint& rHint );

	void				FamilySelect( USHORT nId );
	void				SetFamily( USHORT nId );
	void				ActionSelect( USHORT nId );

    sal_Int32           LoadFactoryStyleFilter( SfxObjectShell* i_pObjSh );
    void                SaveFactoryStyleFilter( SfxObjectShell* i_pObjSh, sal_Int32 i_nFilter );

public:
	TYPEINFO();

	SfxCommonTemplateDialog_Impl( SfxBindings* pB, SfxDockingWindow* );
	SfxCommonTemplateDialog_Impl( SfxBindings* pB, ModalDialog* );
	~SfxCommonTemplateDialog_Impl();

	DECL_LINK( MenuSelectHdl, Menu * );

	virtual void		EnableEdit( BOOL b = TRUE )	{ bCanEdit = b; }
	virtual void		EnableDel( BOOL b = TRUE )	{ bCanDel = b; }
	virtual void		EnableNew( BOOL b = TRUE )	{ bCanNew = b; }

	ISfxTemplateCommon*	GetISfxTemplateCommon() { return &aISfxTemplateCommon; }
	Window*				GetWindow() { return pWindow; }

	void				EnableTreeDrag( BOOL b = TRUE );
	void				ExecuteContextMenu_Impl( const Point& rPos, Window* pWin );
	void				EnableExample_Impl( USHORT nId, BOOL bEnable );
	SfxStyleFamily		GetActualFamily() const;
	String				GetSelectedEntry() const;
	SfxObjectShell*		GetObjectShell() const { return pCurObjShell; }

	virtual void		PrepareDeleteAction();	// disable buttons, change button text, etc. when del is going to happen

	inline BOOL			CanEdit( void ) const	{ return bCanEdit; }
	inline BOOL			CanDel( void ) const	{ return bCanDel; }
	inline BOOL			CanNew( void ) const	{ return bCanNew; }

	// normaly for derivates from SvTreeListBoxes, but in this case the dialog handles context menus
	virtual PopupMenu*	CreateContextMenu( void );

    // Rechnet von den SFX_STYLE_FAMILY Ids auf 1-5 um
    static USHORT       SfxFamilyIdToNId( SfxStyleFamily nFamily );

    void                SetAutomaticFilter();
};
/* -----------------10.12.2003 11:42-----------------

 --------------------------------------------------*/

class DropToolBox_Impl : public ToolBox, public DropTargetHelper
{
    SfxTemplateDialog_Impl&     rParent;
protected:
    virtual sal_Int8    AcceptDrop( const AcceptDropEvent& rEvt );
    virtual sal_Int8    ExecuteDrop( const ExecuteDropEvent& rEvt );
public:
    DropToolBox_Impl(Window* pParent, SfxTemplateDialog_Impl* pTemplateDialog);
    ~DropToolBox_Impl();
};
// class SfxTemplateDialog_Impl ------------------------------------------

class SfxTemplateDialog_Impl :  public SfxCommonTemplateDialog_Impl
{
private:
	friend class SfxTemplateControllerItem;
	friend class SfxTemplateDialogWrapper;
    friend class DropToolBox_Impl;

	SfxTemplateDialog*	m_pFloat;
	BOOL				m_bZoomIn;
    DropToolBox_Impl    m_aActionTbL;
	ToolBox				m_aActionTbR;

	DECL_LINK( ToolBoxLSelect, ToolBox * );
	DECL_LINK( ToolBoxRSelect, ToolBox * );
    DECL_LINK( ToolBoxRClick, ToolBox * );
    DECL_LINK( MenuSelectHdl, Menu* );

protected:
	virtual void	Command( const CommandEvent& rMEvt );
	virtual void	EnableEdit( BOOL = TRUE );
	virtual void	EnableItem( USHORT nMesId, BOOL bCheck = TRUE );
	virtual void	CheckItem( USHORT nMesId, BOOL bCheck = TRUE );
	virtual BOOL	IsCheckedItem( USHORT nMesId );
	virtual void	LoadedFamilies();
	virtual void	InsertFamilyItem( USHORT nId, const SfxStyleFamilyItem* pIten );
	virtual void	EnableFamilyItem( USHORT nId, BOOL bEnabled = TRUE );
	virtual void	ClearFamilyList();
    virtual void    ReplaceUpdateButtonByMenu();

	void 			Resize();
	Size			GetMinOutputSizePixel();

	void			updateFamilyImages();
	void			updateNonFamilyImages();

public:
	friend class SfxTemplateDialog;
	TYPEINFO();

	SfxTemplateDialog_Impl( Window* pParent, SfxBindings*, SfxTemplateDialog* pWindow );
	~SfxTemplateDialog_Impl();
};

// class SfxTemplateCatalog_Impl -----------------------------------------

class SfxTemplateCatalog_Impl : public SfxCommonTemplateDialog_Impl
{
private:
	friend class SfxTemplateControllerItem;
	friend class SfxCommonTemplateDialog_Impl;

	ListBox					aFamList;
	OKButton				aOkBtn;
	CancelButton			aCancelBtn;
	PushButton				aNewBtn;
	PushButton				aChangeBtn;
	PushButton	   	 		aDelBtn;
	PushButton				aOrgBtn;
	HelpButton				aHelpBtn;

	SfxTemplateCatalog*		pReal;
	SvUShorts				aFamIds;
	SfxModalDefParentHelper	aHelper;

protected:
	virtual void	EnableItem( USHORT nMesId, BOOL bCheck = TRUE );
	virtual void	CheckItem( USHORT nMesId, BOOL bCheck = TRUE );
	virtual BOOL	IsCheckedItem( USHORT nMesId );
	virtual void	InsertFamilyItem( USHORT nId, const SfxStyleFamilyItem* pIten );
	virtual void	EnableFamilyItem( USHORT nId, BOOL bEnabled = TRUE );
	virtual void	ClearFamilyList();
	virtual void	EnableEdit( BOOL = TRUE );
	virtual void	EnableDel( BOOL = TRUE );
	virtual void	EnableNew( BOOL = TRUE );

        using SfxCommonTemplateDialog_Impl::NewHdl;
	DECL_LINK( FamListSelect, ListBox * );
	DECL_LINK( OkHdl, Button * );
	DECL_LINK( CancelHdl, Button * );
	DECL_LINK( NewHdl, Button * );
	DECL_LINK( ChangeHdl, Button * );
	DECL_LINK( DelHdl, Button * );
	DECL_LINK( OrgHdl, Button * );

public:
	TYPEINFO();
	SfxTemplateCatalog_Impl( Window* pParent, SfxBindings*, SfxTemplateCatalog* pWindow );
	~SfxTemplateCatalog_Impl();

friend class SfxTemplateCatalog;

	virtual void	PrepareDeleteAction();
};

#endif // #ifndef _SFX_TEMPDLGI_HXX


