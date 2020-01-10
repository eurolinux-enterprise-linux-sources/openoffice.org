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

// ============================================================================

#ifndef SC_ASCIIOPT_HXX
#define SC_ASCIIOPT_HXX

#include <tools/string.hxx>
#ifndef _DIALOG_HXX //autogen
#include <vcl/dialog.hxx>
#endif
#ifndef _BUTTON_HXX //autogen
#include <vcl/button.hxx>
#endif
#ifndef _FIXED_HXX //autogen
#include <vcl/fixed.hxx>
#endif
#ifndef _LSTBOX_HXX //autogen
#include <vcl/lstbox.hxx>
#endif
#ifndef _COMBOBOX_HXX //autogen
#include <vcl/combobox.hxx>
#endif
#ifndef _FIELD_HXX //autogen
#include <vcl/field.hxx>
#endif
#include <tools/stream.hxx>
#include <svx/txencbox.hxx>
#include "csvtablebox.hxx"


// ============================================================================

class ScAsciiOptions
{
private:
	BOOL		bFixedLen;
	String		aFieldSeps;
	BOOL		bMergeFieldSeps;
	sal_Unicode	cTextSep;
	CharSet		eCharSet;
	BOOL		bCharSetSystem;
	long		nStartRow;
	USHORT		nInfoCount;
    xub_StrLen* pColStart;  //! TODO replace with vector
    BYTE*       pColFormat; //! TODO replace with vector

public:
					ScAsciiOptions();
					ScAsciiOptions(const ScAsciiOptions& rOpt);
					~ScAsciiOptions();

    static const sal_Unicode cDefaultTextSep = '"';

	ScAsciiOptions&	operator=( const ScAsciiOptions& rCpy );

	BOOL			operator==( const ScAsciiOptions& rCmp ) const;

	void			ReadFromString( const String& rString );
	String			WriteToString() const;

	void			InterpretColumnList( const String& rString );

	CharSet				GetCharSet() const		{ return eCharSet; }
	BOOL				GetCharSetSystem() const	{ return bCharSetSystem; }
	const String&		GetFieldSeps() const	{ return aFieldSeps; }
	BOOL				IsMergeSeps() const		{ return bMergeFieldSeps; }
	sal_Unicode			GetTextSep() const		{ return cTextSep; }
	BOOL				IsFixedLen() const		{ return bFixedLen; }
	USHORT				GetInfoCount() const	{ return nInfoCount; }
	const xub_StrLen*	GetColStart() const		{ return pColStart; }
	const BYTE*			GetColFormat() const	{ return pColFormat; }
	long				GetStartRow() const		{ return nStartRow; }

	void	SetCharSet( CharSet eNew )			{ eCharSet = eNew; }
	void	SetCharSetSystem( BOOL bSet )		{ bCharSetSystem = bSet; }
	void	SetFixedLen( BOOL bSet )			{ bFixedLen = bSet; }
	void	SetFieldSeps( const String& rStr )	{ aFieldSeps = rStr; }
	void	SetMergeSeps( BOOL bSet )			{ bMergeFieldSeps = bSet; }
	void	SetTextSep( sal_Unicode c )			{ cTextSep = c; }
	void	SetStartRow( long nRow)				{ nStartRow= nRow; }

	void	SetColInfo( USHORT nCount, const xub_StrLen* pStart, const BYTE* pFormat );
    void    SetColumnInfo( const ScCsvExpDataVec& rDataVec );
};


//CHINA001 // ============================================================================
//CHINA001 
//CHINA001 class ScImportAsciiDlg : public ModalDialog
//CHINA001 {
//CHINA001 SvStream*                   pDatStream;
//CHINA001 ULONG*                      pRowPosArray;
//CHINA001 ULONG*                      pRowPosArrayUnicode;
//CHINA001 USHORT                      nArrayEndPos;
//CHINA001 USHORT                      nArrayEndPosUnicode;
//CHINA001 ULONG                       nStreamPos;
//CHINA001 ULONG                       nStreamPosUnicode;
//CHINA001 BOOL                        bVFlag;
//CHINA001 
//CHINA001 FixedLine                   aFlFieldOpt;
//CHINA001 FixedText                   aFtCharSet;
//CHINA001 SvxTextEncodingBox          aLbCharSet;
//CHINA001 
//CHINA001 FixedText                   aFtRow;
//CHINA001 NumericField                aNfRow;
//CHINA001 
//CHINA001 FixedLine                   aFlSepOpt;
//CHINA001 RadioButton                 aRbFixed;
//CHINA001 RadioButton                 aRbSeparated;
//CHINA001 
//CHINA001 CheckBox                    aCkbTab;
//CHINA001 CheckBox                    aCkbSemicolon;
//CHINA001 CheckBox                    aCkbComma;
//CHINA001 CheckBox                    aCkbSpace;
//CHINA001 CheckBox                    aCkbOther;
//CHINA001 Edit                        aEdOther;
//CHINA001 CheckBox                    aCkbAsOnce;
//CHINA001 FixedText                   aFtTextSep;
//CHINA001 ComboBox                    aCbTextSep;
//CHINA001 
//CHINA001 FixedLine                   aFlWidth;
//CHINA001 FixedText                   aFtType;
//CHINA001 ListBox                     aLbType;
//CHINA001 
//CHINA001 ScCsvTableBox               maTableBox;
//CHINA001 
//CHINA001 OKButton                    aBtnOk;
//CHINA001 CancelButton                aBtnCancel;
//CHINA001 HelpButton                  aBtnHelp;
//CHINA001 
//CHINA001 String                      aCharSetUser;
//CHINA001 String                      aColumnUser;
//CHINA001 String                      aFldSepList;
//CHINA001 String                      aTextSepList;
//CHINA001 
//CHINA001 // aPreviewLine contains the byte string as read from the file
//CHINA001 ByteString                  aPreviewLine[ CSV_PREVIEW_LINES ];
//CHINA001 // same for Unicode
//CHINA001 String                      aPreviewLineUnicode[ CSV_PREVIEW_LINES ];
//CHINA001 
//CHINA001 CharSet                     meCharSet;          /// Selected char set.
//CHINA001 bool                        mbCharSetSystem;    /// Is System char set selected?
//CHINA001 
//CHINA001 public:
//CHINA001 ScImportAsciiDlg(
//CHINA001 Window* pParent, String aDatName,
//CHINA001 SvStream* pInStream, sal_Unicode cSep = '\t' );
//CHINA001 ~ScImportAsciiDlg();
//CHINA001 
//CHINA001 void                        GetOptions( ScAsciiOptions& rOpt );
//CHINA001 
//CHINA001 private:
//CHINA001 /** Sets the selected char set data to meCharSet and mbCharSetSystem. */
//CHINA001 void                        SetSelectedCharSet();
//CHINA001 /** Returns all separator characters in a string. */
//CHINA001 String                      GetSeparators() const;
//CHINA001 
//CHINA001 /** Enables or disables all separator checkboxes and edit fields. */
//CHINA001 void                        SetupSeparatorCtrls();
//CHINA001 
//CHINA001 void                        UpdateVertical( bool bSwitchToFromUnicode = false );
//CHINA001 
//CHINA001 DECL_LINK( CharSetHdl, SvxTextEncodingBox* );
//CHINA001 DECL_LINK( FirstRowHdl, NumericField* );
//CHINA001 DECL_LINK( RbSepFixHdl, RadioButton* );
//CHINA001 DECL_LINK( SeparatorHdl, Control* );
//CHINA001 DECL_LINK( LbColTypeHdl, ListBox* );
//CHINA001 DECL_LINK( UpdateTextHdl, ScCsvTableBox* );
//CHINA001 DECL_LINK( ColTypeHdl, ScCsvTableBox* );
//CHINA001 };
//CHINA001 
//CHINA001 
// ============================================================================

#endif

