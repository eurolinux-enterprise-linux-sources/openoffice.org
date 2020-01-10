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
#ifndef _CUI_CHARMAP_HXX
#define _CUI_CHARMAP_HXX

// include ---------------------------------------------------------------

#include <vcl/ctrl.hxx>
#include <vcl/metric.hxx>
#include <vcl/button.hxx>
#include <vcl/fixed.hxx> 
#include <vcl/lstbox.hxx> 
#include <sfx2/basedlgs.hxx>
#include <svx/charmap.hxx>

class SubsetMap;
class SvxCharMapData;

// define ----------------------------------------------------------------

#define CHARMAP_MAXLEN  32

namespace svx
{
	struct SvxShowCharSetItem;
	class SvxShowCharSetVirtualAcc;
}
// class SvxCharacterMap -------------------------------------------------
// the main purpose of this dialog is to enable the use of characters
// that are not easily accesible from the keyboard

// class SvxShowText =====================================================

class SvxShowText : public Control
{
public:
                    SvxShowText( Window* pParent,
                                 const ResId& rResId,
                                 BOOL bCenter = FALSE );
                    ~SvxShowText();

    void            SetFont( const Font& rFont );
    void            SetText( const String& rText );

protected:
    virtual void    Paint( const Rectangle& );

private:
    long            mnY;
    BOOL            mbCenter;

};

class SvxCharMapData
{
public:
                    SvxCharMapData( class SfxModalDialog* pDialog, BOOL bOne_, ResMgr* pResContext );

    void            SetCharFont( const Font& rFont );

private:
friend class SvxCharacterMap;
    SfxModalDialog* mpDialog;

    SvxShowCharSet  aShowSet;
    SvxShowText     aShowText;
//    SvxShowText     aShowShortcut;
    OKButton        aOKBtn;
    CancelButton    aCancelBtn;
    HelpButton      aHelpBtn;
    PushButton      aDeleteBtn;
//    PushButton		aAssignBtn;
    FixedText       aFontText;
    ListBox         aFontLB;
    FixedText       aSubsetText;
    ListBox         aSubsetLB;
    FixedText       aSymbolText;
    SvxShowText     aShowChar;
    FixedText       aCharCodeText;
//	FixedText		aAssignText;
    Font            aFont;
    BOOL            bOne;
    const SubsetMap* pSubsetMap;

    DECL_LINK( OKHdl, OKButton* );
    DECL_LINK( FontSelectHdl, ListBox* );
    DECL_LINK( SubsetSelectHdl, ListBox* );
    DECL_LINK( CharDoubleClickHdl, Control* pControl );
    DECL_LINK( CharSelectHdl, Control* pControl );
    DECL_LINK( CharHighlightHdl, Control* pControl );
    DECL_LINK( CharPreSelectHdl, Control* pControl );
    DECL_LINK( DeleteHdl, PushButton* pBtn );
    DECL_LINK( AssignHdl, PushButton* pBtn );
};

class SvxCharacterMap : public SfxModalDialog
{
private:
    SvxCharMapData* mpCharMapData;

public:
                    SvxCharacterMap( Window* pParent, BOOL bOne=TRUE, const SfxItemSet* pSet=0 );
                    ~SvxCharacterMap();

    void            DisableFontSelection();

    const Font&     GetCharFont() const;
    void            SetCharFont( const Font& rFont );

    void            SetChar( sal_UCS4 );
    sal_UCS4        GetChar() const;

    String          GetCharacters() const;

	virtual short	Execute();
};

#endif
 
