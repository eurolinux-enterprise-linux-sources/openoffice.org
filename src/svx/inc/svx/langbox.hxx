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
#ifndef _SVX_LANGBOX_HXX
#define _SVX_LANGBOX_HXX

// include ---------------------------------------------------------------

#include <com/sun/star/uno/Sequence.hxx>
#include <vcl/lstbox.hxx>
#include "svx/svxdllapi.h"
#include <vcl/image.hxx>
#include <tools/solar.h>
#include <layout/layout.hxx>

class SvtLanguageTable;

#define LANG_LIST_EMPTY             0x0000
#define LANG_LIST_ALL               0x0001
#define LANG_LIST_WESTERN           0x0002
#define LANG_LIST_CTL               0x0004
#define LANG_LIST_CJK               0x0008
#define LANG_LIST_FBD_CHARS         0x0010
#define LANG_LIST_SPELL_AVAIL       0x0020
#define LANG_LIST_HYPH_AVAIL        0x0040
#define LANG_LIST_THES_AVAIL        0x0080
#define LANG_LIST_ONLY_KNOWN        0x0100  // list only locales provided by I18N
#define LANG_LIST_SPELL_USED        0x0200
#define LANG_LIST_HYPH_USED         0x0400
#define LANG_LIST_THES_USED         0x0800
#define LANG_LIST_ALSO_PRIMARY_ONLY 0x1000  // Do not exclude primary-only 
                                            // languages that do not form a 
                                            // locale, such as Arabic as 
                                            // opposed to Arabic-Egypt.


class SVX_DLLPUBLIC SvxLanguageBox : public ListBox
{
public:

private:
	Image					m_aNotCheckedImage;
	Image					m_aCheckedImage;
    Image                   m_aCheckedImageHC;
	String					m_aAllString;
    com::sun::star::uno::Sequence< INT16 >  *m_pSpellUsedLang;
    SvtLanguageTable*       m_pLangTable;
	INT16					m_nLangList;
	BOOL                    m_bHasLangNone;
	BOOL					m_bLangNoneIsLangAll;
	BOOL					m_bWithCheckmark;

    SVX_DLLPRIVATE void                    Init();
    SVX_DLLPRIVATE USHORT                  ImplInsertImgEntry( const String& rEntry, USHORT nPos, bool bChecked );
    SVX_DLLPRIVATE USHORT                  ImplInsertLanguage(LanguageType, USHORT, sal_Int16 );

public:
    SvxLanguageBox( Window* pParent, WinBits nWinStyle, BOOL bCheck = FALSE);
    SvxLanguageBox( Window* pParent, const ResId& rResId, BOOL bCheck = FALSE);
	~SvxLanguageBox();

	void			SetLanguageList( INT16 nLangList,
							BOOL bHasLangNone, BOOL bLangNoneIsLangAll = FALSE,
							BOOL bCheckSpellAvail = FALSE );

	USHORT			InsertLanguage( const LanguageType eLangType, USHORT nPos = LISTBOX_APPEND );
	USHORT			InsertDefaultLanguage( sal_Int16 nType, USHORT nPos = LISTBOX_APPEND );
    USHORT          InsertLanguage( const LanguageType eLangType,
                            BOOL bCheckEntry, USHORT nPos = LISTBOX_APPEND );
	void			RemoveLanguage( const LanguageType eLangType );
	void			SelectLanguage( const LanguageType eLangType, BOOL bSelect = TRUE );
	LanguageType	GetSelectLanguage() const;
	BOOL			IsLanguageSelected( const LanguageType eLangType ) const;
};

#if ENABLE_LAYOUT
namespace layout
{
class SvxLanguageBoxImpl;
class SVX_DLLPUBLIC SvxLanguageBox : public ListBox
{
    /*DECL_GET_IMPL( SvxLanguageBox );
    DECL_CONSTRUCTORS( SvxLanguageBox, ListBox, WB_BORDER );
    DECL_GET_WINDOW (SvxLanguageBox);*/

public:
	SvxLanguageBox( Context*, const char*, BOOL bCheck = FALSE );
    ~SvxLanguageBox ();
    void SetLanguageList (sal_Int16 list, bool hasLangNone, bool langNoneIsLangAll=false, bool checkSpellAvailable=false);

    sal_uInt16 InsertLanguage (LanguageType const type, sal_uInt16 pos=LISTBOX_APPEND);
    sal_uInt16 InsertLanguage (LanguageType const type, bool checkEntry, sal_uInt16 pos=LISTBOX_APPEND);
    void RemoveLanguage (LanguageType const type);
    void SelectLanguage (LanguageType const type, bool select=true);
    LanguageType GetSelectLanguage () const;
    bool IsLanguageSelected( LanguageType const type) const;
};
};
#endif

#endif

