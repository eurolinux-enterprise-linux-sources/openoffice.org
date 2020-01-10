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
#ifndef _MACROASS_HXX
#define _MACROASS_HXX

#include "sal/config.h"

#include <sfx2/basedlgs.hxx>
#include <sfx2/tabdlg.hxx>
#include <svtools/macitem.hxx>
#include <vcl/lstbox.hxx>
#include <com/sun/star/frame/XFrame.hpp>

class _SfxMacroTabPage;
class SvStringsDtor;
class SvTabListBox;
class Edit;
class String;

typedef SvStringsDtor* (*FNGetRangeHdl)( _SfxMacroTabPage*, const String& rLanguage );
typedef SvStringsDtor* (*FNGetMacrosOfRangeHdl)( _SfxMacroTabPage*, const String& rLanguage, const String& rRange );

class SfxConfigGroupListBox_Impl;
class SfxConfigFunctionListBox_Impl;
class _HeaderTabListBox;
class _SfxMacroTabPage_Impl;

class _SfxMacroTabPage : public SfxTabPage
{
	SvxMacroTableDtor			aTbl;
	DECL_DLLPRIVATE_STATIC_LINK( _SfxMacroTabPage, SelectEvent_Impl, SvTabListBox * );
	DECL_DLLPRIVATE_STATIC_LINK( _SfxMacroTabPage, SelectGroup_Impl, ListBox * );
	DECL_DLLPRIVATE_STATIC_LINK( _SfxMacroTabPage, SelectMacro_Impl, ListBox * );

	DECL_DLLPRIVATE_STATIC_LINK( _SfxMacroTabPage, DoubleClickHdl_Impl, Control* );
	DECL_DLLPRIVATE_STATIC_LINK( _SfxMacroTabPage, AssignDeleteHdl_Impl, PushButton * );

	DECL_DLLPRIVATE_STATIC_LINK( _SfxMacroTabPage, ChangeScriptHdl_Impl, RadioButton * );
	DECL_DLLPRIVATE_STATIC_LINK( _SfxMacroTabPage, GetFocus_Impl, Edit* );
    DECL_DLLPRIVATE_STATIC_LINK( _SfxMacroTabPage, TimeOut_Impl, Timer* );

protected:
	_SfxMacroTabPage_Impl*		mpImpl;

								_SfxMacroTabPage( Window* pParent, const ResId& rId, const SfxItemSet& rItemSet );

	void						InitAndSetHandler();
	void						FillEvents();
	void						FillMacroList();
	void						EnableButtons( const String& rLanguage );

public:

	virtual						~_SfxMacroTabPage();

	void						AddEvent( const String & rEventName, USHORT nEventId );

	const SvxMacroTableDtor&	GetMacroTbl() const;
	void						SetMacroTbl( const SvxMacroTableDtor& rTbl );
	void						ClearMacroTbl();

	virtual void				ScriptChanged( const String& rLanguage );
	virtual void				PageCreated (SfxAllItemSet aSet);

	// zum setzen / abfragen der Links
	void						SetGetRangeLink( FNGetRangeHdl pFn );
	FNGetRangeHdl				GetGetRangeLink() const;
	void						SetGetMacrosOfRangeLink( FNGetMacrosOfRangeHdl pFn );
	FNGetMacrosOfRangeHdl		GetGetMacrosOfRangeLink() const;

	// --------- Erben aus der Basis -------------
	virtual	BOOL				FillItemSet( SfxItemSet& rSet );
	virtual	void				Reset( const SfxItemSet& rSet );

	void						SetReadOnly( BOOL bSet );
	BOOL						IsReadOnly() const;
	void						SelectEvent( const String& rEventName, USHORT nEventId );
};

inline const SvxMacroTableDtor& _SfxMacroTabPage::GetMacroTbl() const
{
	return aTbl;
}

inline void _SfxMacroTabPage::SetMacroTbl( const SvxMacroTableDtor& rTbl )
{
	aTbl = rTbl;
}

inline void _SfxMacroTabPage::ClearMacroTbl()
{
	aTbl.DelDtor();
}

class SfxMacroTabPage : public _SfxMacroTabPage
{
public:
	SfxMacroTabPage(
        Window* pParent,
        const ResId& rId,
        const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& rxDocumentFrame,
		const SfxItemSet& rSet
    );

	// --------- Erben aus der Basis -------------
	static SfxTabPage* Create( Window* pParent, const SfxItemSet& rAttrSet );
};

class SfxMacroAssignDlg : public SfxSingleTabDialog
{
public:
    SfxMacroAssignDlg(
        Window* pParent,
        const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& rxDocumentFrame,
        const SfxItemSet& rSet );
	virtual	~SfxMacroAssignDlg();
};

#endif
