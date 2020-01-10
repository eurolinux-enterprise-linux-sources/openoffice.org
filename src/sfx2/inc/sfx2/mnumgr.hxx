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
#ifndef _SFXMNUMGR_HXX
#define _SFXMNUMGR_HXX

#include <stdarg.h>

#ifndef _MENU_HXX //autogen //wg. MENU_APPEND !!!!
#include <vcl/menu.hxx>
#endif
#include <vcl/wintypes.hxx>
#include <tools/link.hxx>
#include <com/sun/star/embed/VerbDescriptor.hpp>
#include <com/sun/star/uno/Sequence.hxx>

#include <tools/stream.hxx>
#include <tools/resid.hxx>
#include <sfx2/sfx.hrc>

class SfxVirtualMenu;
class SfxPoolItem;
class SfxBindings;
class ResMgr;
class Menu;
class PopupMenu;
class SfxBindings;
class ResId;
class Point;
class SfxObjectShell;
class SfxModule;
class SfxViewFrame;

class SfxMenuManager
{
friend class SfxPopupMenuManager;

	SfxVirtualMenu* 		pMenu;			// das eigentliche Menu
	SfxVirtualMenu* 		pOldMenu;		// only while reconfiguring
	BOOL					bMenuBar;		// Popup oder MenuBar
	SfxBindings*			pBindings;
	ResMgr* 				pResMgr;
    sal_uInt32              nType;
	BOOL					bAddClipboardFuncs : 1;

	void					Construct( SfxVirtualMenu& rMenu );

protected:
                            SfxMenuManager( Menu*, SfxBindings& );
                            SfxMenuManager( const ResId&, SfxBindings& );
							~SfxMenuManager();
    USHORT                  GetItemPos( USHORT nId );
    sal_uInt32              GetType() { return nType; }
public:

            void            UseDefault();

	DECL_LINK( Select, Menu* );

	SfxVirtualMenu* 		GetMenu() const
							{ return pMenu; }

	SfxBindings&			GetBindings() { return *pBindings; }
	const SfxBindings&		GetBindings() const { return *pBindings; }
	void					SetResMgr(ResMgr* pMgr)  {pResMgr = pMgr; }
    ResMgr*                 GetResMgr() const { return pResMgr; }
	void					SetPopupMenu( USHORT nId, PopupMenu *pMenu );

//#if 0 // _SOLAR__PRIVATE
	void			Construct_Impl( Menu* pMenu, BOOL bWithHelp );
//#endif
};

//--------------------------------------------------------------------

class SAL_DLLPUBLIC_EXPORT SfxPopupMenuManager : public SfxMenuManager
{
private:
	DECL_LINK( SelectHdl, void * );
	Menu*				pSVMenu;

private:
						// only declared, but not defined: don't allow copying
						SfxPopupMenuManager( const SfxPopupMenuManager& );
						SfxPopupMenuManager& operator=( const SfxPopupMenuManager& );

public:
                        SfxPopupMenuManager( const ResId&, SfxBindings& );
                        SfxPopupMenuManager( PopupMenu*, SfxBindings& );
						~SfxPopupMenuManager();
    static void         ExecutePopup( const ResId&, SfxViewFrame* pViewFrame, const Point& rPoint, Window* pWindow );
	static SfxPopupMenuManager* Popup( const ResId& rResId, SfxViewFrame* pFrame,const Point& rPoint, Window* pWindow );

	USHORT				Execute( const Point& rPos, Window *pWindow );
    USHORT              Execute( const Point& rPoint, Window* pWindow, va_list pArgs, const SfxPoolItem *pArg1 );
    USHORT              Execute( const Point& rPoint, Window* pWindow, const SfxPoolItem *pArg1 ... );

	void				StartInsert();
	void				EndInsert();
	void				CheckItem( USHORT, BOOL );
	void				RemoveItem( USHORT );
	void 				InsertItem( USHORT, const String&, MenuItemBits,
								USHORT nPos = MENU_APPEND );
	void 				InsertSeparator( USHORT nPos = MENU_APPEND );
	void				RemoveDisabledEntries();
	void				AddClipboardFunctions();
	Menu*					GetSVMenu();
};

#endif // #ifndef _SFXMNUMGR_HXX

