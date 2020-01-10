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
#ifndef _TEXTUND2_HXX
#define _TEXTUND2_HXX

#include <textundo.hxx>


class TextUndoDelPara : public TextUndo
{
private:
	BOOL 			mbDelObject;
	ULONG 			mnPara;
	TextNode* 		mpNode;	// Zeigt auf das gueltige, nicht zerstoerte Objekt!

public:
					TYPEINFO();
					TextUndoDelPara( TextEngine* pTextEngine, TextNode* pNode, ULONG nPara );
					~TextUndoDelPara();

	virtual void 	Undo();
	virtual void 	Redo();
};


class TextUndoConnectParas : public TextUndo
{
private:
	ULONG 			mnPara;
	USHORT			mnSepPos;

public:
					TYPEINFO();
					TextUndoConnectParas( TextEngine* pTextEngine, ULONG nPara, USHORT nSepPos );
					~TextUndoConnectParas();

	virtual void 	Undo();
	virtual void 	Redo();
};


class TextUndoSplitPara : public TextUndo
{
private:
	ULONG 			mnPara;
	USHORT			mnSepPos;

public:
					TYPEINFO();
					TextUndoSplitPara( TextEngine* pTextEngine, ULONG nPara, USHORT nSepPos );
					~TextUndoSplitPara();

	virtual void 	Undo();
	virtual void 	Redo();
};


class TextUndoInsertChars : public TextUndo
{
private:
	TextPaM			maTextPaM;
	String			maText;

public:
					TYPEINFO();
					TextUndoInsertChars( TextEngine* pTextEngine, const TextPaM& rTextPaM, const String& rStr );

//	const TextPaM&	GetTextPaM() { return aTextPaM; }
//	String&			GetStr() { return aText; }

	virtual void 	Undo();
	virtual void 	Redo();

	virtual BOOL	Merge( SfxUndoAction *pNextAction );
};


class TextUndoRemoveChars : public TextUndo
{
private:
	TextPaM			maTextPaM;
	String			maText;

public:
					TYPEINFO();
					TextUndoRemoveChars( TextEngine* pTextEngine, const TextPaM& rTextPaM, const String& rStr );

//	const TextPaM&		GetTextPaM() { return aTextPaM; }
//	String&			GetStr() { return aText; }

	virtual void 	Undo();
	virtual void 	Redo();
};


class TextUndoSetAttribs: public TextUndo
{
private:
	TextSelection		maSelection;
//	SfxItemSet			aNewAttribs;
//	TextInfoArray		aPrevAttribs;
//	BYTE				nSpecial;
//	BOOL				bSetIsRemove;
//	USHORT				nRemoveWhich;
//
//	void				ImpSetSelection( TextView* pView );


public:
						TYPEINFO();
						TextUndoSetAttribs( TextEngine* pTextEngine, const TextSelection& rESel );
						~TextUndoSetAttribs();

//	TextInfoArray&		GetTextInfos()	{ return aPrevAttribs; }
//	SfxItemSet&			GetNewAttribs()		{ return aNewAttribs; }
//	void				SetSpecial( BYTE n ) 			{ nSpecial = n; }
//	void				SetRemoveAttribs( BOOL b ) 		{ bSetIsRemove = b; }
//	void				SetRemoveWhich( USHORT n )		{ nRemoveWhich = n; }

	virtual void		Undo();
	virtual void		Redo();
};

#endif // _TEXTUND2_HXX
