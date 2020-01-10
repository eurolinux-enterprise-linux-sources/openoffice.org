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

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_svtools.hxx"


#include <tools/stream.hxx>

#include <svtools/texteng.hxx>
#include <svtools/textview.hxx>
#include <textdoc.hxx>
#include <textdat2.hxx>
#include <textundo.hxx>
#include <textund2.hxx>
#include <svtools/ctloptions.hxx>
#include <vcl/window.hxx>

#include <vcl/edit.hxx>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/beans/PropertyValues.hpp>

#ifndef _COM_SUN_STAR_TEXT_XBREAKITERATOR_HPP_
#include <com/sun/star/i18n/XBreakIterator.hpp>
#endif

#ifndef _COM_SUN_STAR_TEXT_CHARACTERITERATORMODE_HPP_
#include <com/sun/star/i18n/CharacterIteratorMode.hpp>
#endif

#ifndef _COM_SUN_STAR_TEXT_WORDTYPE_HPP_
#include <com/sun/star/i18n/WordType.hpp>
#endif

#ifndef _COM_SUN_STAR_I18N_XEXTENDEDINPUTSEQUENCECHECKER_HDL_
#include <com/sun/star/i18n/XExtendedInputSequenceChecker.hpp>
#endif
#include <com/sun/star/i18n/InputSequenceCheckMode.hpp>
#include <com/sun/star/i18n/ScriptType.hpp>

#include <comphelper/processfactory.hxx>

#include <unotools/localedatawrapper.hxx>
#include <vcl/unohelp.hxx>

#include <vcl/svapp.hxx>
#include <vcl/unohelp.hxx>
#include <vcl/metric.hxx>

#include <unicode/ubidi.h>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::rtl;

typedef TextView* TextViewPtr;
SV_DECL_PTRARR( TextViews, TextViewPtr, 0, 1 )
// SV_IMPL_PTRARR( TextViews, TextViewPtr );

SV_DECL_VARARR_SORT( TESortedPositions, ULONG, 16, 8 )
SV_IMPL_VARARR_SORT( TESortedPositions, ULONG )

#define RESDIFF		10
#define SCRLRANGE	20		// 1/20 der Breite/Hoehe scrollen, wenn im QueryDrop


// -------------------------------------------------------------------------
// (-) class TextEngine
// -------------------------------------------------------------------------
TextEngine::TextEngine()
{
	mpDoc = 0;
	mpTEParaPortions = 0;

	mpViews = new TextViews;
	mpActiveView = NULL;

	mbIsFormatting		= FALSE;
	mbFormatted			= FALSE;
	mbUpdate 			= TRUE;
	mbModified			= FALSE;
	mbUndoEnabled 		= FALSE;
	mbIsInUndo			= FALSE;
	mbDowning			= FALSE;
    mbRightToLeft		= FALSE;
	mbHasMultiLineParas = FALSE;

	meAlign			= TXTALIGN_LEFT;

	mnMaxTextWidth	= 0;
	mnMaxTextLen 	= 0;
	mnCurTextWidth	= 0xFFFFFFFF;
	mnCurTextHeight	= 0;

	mpUndoManager 	= NULL;
   	mpIMEInfos		= NULL;
    mpLocaleDataWrapper = NULL;

    mpIdleFormatter = new IdleFormatter;
	mpIdleFormatter->SetTimeoutHdl( LINK( this, TextEngine, IdleFormatHdl ) );

	mpRefDev = new VirtualDevice;

    ImpInitLayoutMode( mpRefDev );

	ImpInitDoc();

    maTextColor = COL_BLACK;
	Font aFont;
	aFont.SetTransparent( FALSE );
	Color aFillColor( aFont.GetFillColor() );
	aFillColor.SetTransparency( 0 );
	aFont.SetFillColor( aFillColor );
	SetFont( aFont );
}

TextEngine::~TextEngine()
{
	mbDowning = TRUE;

	delete mpIdleFormatter;
	delete mpDoc;
	delete mpTEParaPortions;
	delete mpViews;	// nur die Liste, nicht die Vies
	delete mpRefDev;
	delete mpUndoManager;
	delete mpIMEInfos;
    delete mpLocaleDataWrapper;
}

void TextEngine::InsertView( TextView* pTextView )
{
	mpViews->Insert( pTextView, mpViews->Count() );
	pTextView->SetSelection( TextSelection() );

	if ( !GetActiveView() )
		SetActiveView( pTextView );
}

void TextEngine::RemoveView( TextView* pTextView )
{
	USHORT nPos = mpViews->GetPos( pTextView );
	if( nPos != USHRT_MAX )
	{
		pTextView->HideCursor();
		mpViews->Remove( nPos, 1 );
		if ( pTextView == GetActiveView() )
			SetActiveView( 0 );
	}
}

USHORT TextEngine::GetViewCount() const
{
	return mpViews->Count();
}

TextView* TextEngine::GetView( USHORT nView ) const
{
	return mpViews->GetObject( nView );
}

TextView* TextEngine::GetActiveView() const
{
	return mpActiveView;
}

void TextEngine::SetActiveView( TextView* pTextView )
{
	if ( pTextView != mpActiveView )
	{
		if ( mpActiveView )
			mpActiveView->HideSelection();

		mpActiveView = pTextView;

		if ( mpActiveView )
			mpActiveView->ShowSelection();
	}
}

void TextEngine::SetFont( const Font& rFont )
{
	if ( rFont != maFont )
	{
		maFont = rFont;
        // #i40221# As the font's color now defaults to transparent (since i35764)
        //  we have to choose a useful textcolor in this case.
        // Otherwise maTextColor and maFont.GetColor() are both transparent....
		if( rFont.GetColor() == COL_TRANSPARENT )
		    maTextColor = COL_BLACK;
        else
            maTextColor = rFont.GetColor();

		// Wegen Selektion keinen Transparenten Font zulassen...
		// (Sonst spaeter in ImplPaint den Hintergrund anders loeschen...)
		maFont.SetTransparent( FALSE );
		// Tell VCL not to use the font color, use text color from OutputDevice
		maFont.SetColor( COL_TRANSPARENT );
		Color aFillColor( maFont.GetFillColor() );
		aFillColor.SetTransparency( 0 );
		maFont.SetFillColor( aFillColor );

		maFont.SetAlign( ALIGN_TOP );
		mpRefDev->SetFont( maFont);
		Size aTextSize;
		aTextSize.Width() = mpRefDev->GetTextWidth( String::CreateFromAscii( RTL_CONSTASCII_STRINGPARAM( "    " ) ) );
		aTextSize.Height() = mpRefDev->GetTextHeight();
		if ( !aTextSize.Width() )
			aTextSize.Width() = mpRefDev->GetTextWidth( String::CreateFromAscii( RTL_CONSTASCII_STRINGPARAM( "XXXX" ) ) );

		mnDefTab = (USHORT)aTextSize.Width();
		if ( !mnDefTab )
			mnDefTab = 1;
		mnCharHeight = (USHORT)aTextSize.Height();
/*
        // #93746# Doesn't work with CJK HalfWidth/FullWidth
        FontMetric aRealFont( mpRefDev->GetFontMetric() );
		if ( aRealFont.GetPitch() == PITCH_FIXED )
		{
			String aX100;
			aX100.Fill( 100, 'X' );
			mnFixCharWidth100 = (USHORT)mpRefDev->GetTextWidth( aX100 );
		}
		else
*/
		mnFixCharWidth100 = 0;

		FormatFullDoc();
		UpdateViews();

		for ( USHORT nView = mpViews->Count(); nView; )
		{
			TextView* pView = mpViews->GetObject( --nView );
            pView->GetWindow()->SetInputContext( InputContext( GetFont(), !pView->IsReadOnly() ? INPUTCONTEXT_TEXT|INPUTCONTEXT_EXTTEXTINPUT : 0 ) );
        }
	}
}

void TextEngine::SetDefTab( USHORT nDefTab )
{
	mnDefTab = nDefTab;
	// evtl neu setzen?
}

void TextEngine::SetMaxTextLen( ULONG nLen )
{
	mnMaxTextLen = nLen;
}

void TextEngine::SetMaxTextWidth( ULONG nMaxWidth )
{
	if ( nMaxWidth != mnMaxTextWidth )
	{
		mnMaxTextWidth = Min( nMaxWidth, (ULONG)0x7FFFFFFF );
		FormatFullDoc();
		UpdateViews();
	}
}

static sal_Unicode static_aLFText[] = { '\n', 0 };
static sal_Unicode static_aCRText[] = { '\r', 0 };
static sal_Unicode static_aCRLFText[] = { '\r', '\n', 0 };

static inline const sal_Unicode* static_getLineEndText( LineEnd aLineEnd )
{
	const sal_Unicode* pRet = NULL;

	switch( aLineEnd )
	{
	case LINEEND_LF: pRet = static_aLFText;break;
	case LINEEND_CR: pRet = static_aCRText;break;
	case LINEEND_CRLF: pRet = static_aCRLFText;break;
	}
	return pRet;
}

void  TextEngine::ReplaceText(const TextSelection& rSel, const String& rText)
{        
    ImpInsertText( rSel, rText );
}

String TextEngine::GetText( LineEnd aSeparator ) const
{
	return mpDoc->GetText( static_getLineEndText( aSeparator ) );
}

String TextEngine::GetTextLines( LineEnd aSeparator ) const
{
	String aText;
	ULONG nParas = mpTEParaPortions->Count();
	const sal_Unicode* pSep = static_getLineEndText( aSeparator );
	for ( ULONG nP = 0; nP < nParas; nP++ )
	{
		TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nP );

		USHORT nLines = pTEParaPortion->GetLines().Count();
		for ( USHORT nL = 0; nL < nLines; nL++ )
		{
			TextLine* pLine = pTEParaPortion->GetLines()[nL];
			aText += pTEParaPortion->GetNode()->GetText().Copy( pLine->GetStart(), pLine->GetEnd() - pLine->GetStart() );
			if ( pSep && ( ( (nP+1) < nParas ) || ( (nL+1) < nLines ) ) )
				aText += pSep;
		}
	}
	return aText;
}

String TextEngine::GetText( ULONG nPara ) const
{
	return mpDoc->GetText( nPara );
}

ULONG TextEngine::GetTextLen( LineEnd aSeparator ) const
{
	return mpDoc->GetTextLen( static_getLineEndText( aSeparator ) );
}

ULONG TextEngine::GetTextLen( const TextSelection& rSel, LineEnd aSeparator ) const
{
	TextSelection aSel( rSel );
	aSel.Justify();
	ValidateSelection( aSel );
	return mpDoc->GetTextLen( static_getLineEndText( aSeparator ), &aSel );
}

USHORT TextEngine::GetTextLen( ULONG nPara ) const
{
	return mpDoc->GetNodes().GetObject( nPara )->GetText().Len();
}

void TextEngine::SetUpdateMode( BOOL bUpdate )
{
	if ( bUpdate != mbUpdate )
	{
		mbUpdate = bUpdate;
		if ( mbUpdate )
		{
			FormatAndUpdate( GetActiveView() );
			if ( GetActiveView() )
				GetActiveView()->ShowCursor();
		}
	}
}

BOOL TextEngine::DoesKeyMoveCursor( const KeyEvent& rKeyEvent )
{
	BOOL bDoesMove = FALSE;

	switch ( rKeyEvent.GetKeyCode().GetCode() )
	{
		case KEY_UP:
		case KEY_DOWN:
		case KEY_LEFT:
		case KEY_RIGHT:
		case KEY_HOME:
		case KEY_END:
		case KEY_PAGEUP:
		case KEY_PAGEDOWN:
		{
			if ( !rKeyEvent.GetKeyCode().IsMod2() )
				bDoesMove = TRUE;
		}
		break;
	}
	return bDoesMove;
}

BOOL TextEngine::DoesKeyChangeText( const KeyEvent& rKeyEvent )
{
	BOOL bDoesChange = FALSE;

	KeyFuncType eFunc = rKeyEvent.GetKeyCode().GetFunction();
	if ( eFunc != KEYFUNC_DONTKNOW )
	{
		switch ( eFunc )
		{
			case KEYFUNC_UNDO:
			case KEYFUNC_REDO:
			case KEYFUNC_CUT:
			case KEYFUNC_PASTE:	bDoesChange = TRUE;
			break;
			default:	// wird dann evtl. unten bearbeitet.
						eFunc = KEYFUNC_DONTKNOW;
		}
	}
	if ( eFunc == KEYFUNC_DONTKNOW )
	{
		switch ( rKeyEvent.GetKeyCode().GetCode() )
		{
			case KEY_DELETE:
			case KEY_BACKSPACE:
			{
				if ( !rKeyEvent.GetKeyCode().IsMod2() )
					bDoesChange = TRUE;
			}
			break;
			case KEY_RETURN:
			case KEY_TAB:
			{
				if ( !rKeyEvent.GetKeyCode().IsMod1() && !rKeyEvent.GetKeyCode().IsMod2() )
					bDoesChange = TRUE;
			}
			break;
			default:
			{
				bDoesChange = TextEngine::IsSimpleCharInput( rKeyEvent );
			}
		}
	}
	return bDoesChange;
}

BOOL TextEngine::IsSimpleCharInput( const KeyEvent& rKeyEvent )
{
	if( rKeyEvent.GetCharCode() >= 32 && rKeyEvent.GetCharCode() != 127 &&
        KEY_MOD1 != (rKeyEvent.GetKeyCode().GetModifier() & ~KEY_SHIFT) && // (ssa) #i45714#:
        KEY_MOD2 != (rKeyEvent.GetKeyCode().GetModifier() & ~KEY_SHIFT) )  // check for Ctrl and Alt separately
	{
		return TRUE;
	}
	return FALSE;
}

void TextEngine::ImpInitDoc()
{
	if ( mpDoc )
		mpDoc->Clear();
	else
		mpDoc = new TextDoc;

	delete mpTEParaPortions;
	mpTEParaPortions = new TEParaPortions;

	TextNode* pNode = new TextNode( String() );
	mpDoc->GetNodes().Insert( pNode, 0 );

	TEParaPortion* pIniPortion = new TEParaPortion( pNode );
	mpTEParaPortions->Insert( pIniPortion, (ULONG)0 );

	mbFormatted = FALSE;

	ImpParagraphRemoved( TEXT_PARA_ALL );
	ImpParagraphInserted( 0 );
}

String TextEngine::GetText( const TextSelection& rSel, LineEnd aSeparator ) const
{
	String aText;

	if ( !rSel.HasRange() )
		return aText;

	TextSelection aSel( rSel );
	aSel.Justify();

	ULONG nStartPara = aSel.GetStart().GetPara();
	ULONG nEndPara = aSel.GetEnd().GetPara();
	const sal_Unicode* pSep = static_getLineEndText( aSeparator );
	for ( ULONG nNode = aSel.GetStart().GetPara(); nNode <= nEndPara; nNode++ )
	{
		TextNode* pNode = mpDoc->GetNodes().GetObject( nNode );

		USHORT nStartPos = 0;
		USHORT nEndPos = pNode->GetText().Len();
		if ( nNode == nStartPara )
			nStartPos = aSel.GetStart().GetIndex();
		if ( nNode == nEndPara ) // kann auch == nStart sein!
			nEndPos = aSel.GetEnd().GetIndex();

		aText += pNode->GetText().Copy( nStartPos, nEndPos-nStartPos );
		if ( nNode < nEndPara )
			aText += pSep;
	}
	return aText;
}

void TextEngine::ImpRemoveText()
{
	ImpInitDoc();

	TextPaM aStartPaM( 0, 0 );
	TextSelection aEmptySel( aStartPaM, aStartPaM );
	for ( USHORT nView = 0; nView < mpViews->Count(); nView++ )
	{
		TextView* pView = mpViews->GetObject( nView );
		pView->ImpSetSelection( aEmptySel );
	}
	ResetUndo();
}

void TextEngine::SetText( const XubString& rText )
{
	ImpRemoveText();

	BOOL bUndoCurrentlyEnabled = IsUndoEnabled();
	// Der von Hand reingesteckte Text kann nicht vom Anwender rueckgaengig gemacht werden.
	EnableUndo( FALSE );

	TextPaM aStartPaM( 0, 0 );
	TextSelection aEmptySel( aStartPaM, aStartPaM );

	TextPaM aPaM = aStartPaM;
	if ( rText.Len() )
		aPaM = ImpInsertText( aEmptySel, rText );

	for ( USHORT nView = 0; nView < mpViews->Count(); nView++ )
	{
		TextView* pView = mpViews->GetObject( nView );
		pView->ImpSetSelection( aEmptySel );

		// Wenn kein Text, dann auch Kein Format&Update
		// => Der Text bleibt stehen.
		if ( !rText.Len() && GetUpdateMode() )
			pView->Invalidate();
	}

	if( !rText.Len() )	// sonst muss spaeter noch invalidiert werden, !bFormatted reicht.
		mnCurTextHeight = 0;

	FormatAndUpdate();

	EnableUndo( bUndoCurrentlyEnabled );
	DBG_ASSERT( !HasUndoManager() || !GetUndoManager().GetUndoActionCount(), "Undo nach SetText?" );
}


void TextEngine::CursorMoved( ULONG nNode )
{
	// Leere Attribute loeschen, aber nur, wenn Absatz nicht leer!
	TextNode* pNode = mpDoc->GetNodes().GetObject( nNode );
	if ( pNode && pNode->GetCharAttribs().HasEmptyAttribs() && pNode->GetText().Len() )
		pNode->GetCharAttribs().DeleteEmptyAttribs();
}

void TextEngine::ImpRemoveChars( const TextPaM& rPaM, USHORT nChars, SfxUndoAction* )
{
	DBG_ASSERT( nChars, "ImpRemoveChars - 0 Chars?!" );
	if ( IsUndoEnabled() && !IsInUndo() )
	{
		// Attribute muessen hier vorm RemoveChars fuer UNDO gesichert werden!
		TextNode* pNode = mpDoc->GetNodes().GetObject( rPaM.GetPara() );
		XubString aStr( pNode->GetText().Copy( rPaM.GetIndex(), nChars ) );

		// Pruefen, ob Attribute geloescht oder geaendert werden:
		USHORT nStart = rPaM.GetIndex();
		USHORT nEnd = nStart + nChars;
		for ( USHORT nAttr = pNode->GetCharAttribs().Count(); nAttr; )
		{
			TextCharAttrib* pAttr = pNode->GetCharAttribs().GetAttrib( --nAttr );
			if ( ( pAttr->GetEnd() >= nStart ) && ( pAttr->GetStart() < nEnd ) )
			{
//				TextSelection aSel( rPaM );
//				aSel.GetEnd().GetIndex() += nChars;
//				TextUndoSetAttribs* pAttrUndo = CreateAttribUndo( aSel );
//				InsertUndo( pAttrUndo );
				break;	// for
			}
		}
//		if ( pCurUndo && ( CreateTextPaM( pCurUndo->GetEPaM() ) == rPaM ) )
//			pCurUndo->GetStr() += aStr;
//		else
			InsertUndo( new TextUndoRemoveChars( this, rPaM, aStr ) );
	}

	mpDoc->RemoveChars( rPaM, nChars );
	ImpCharsRemoved( rPaM.GetPara(), rPaM.GetIndex(), nChars );
}

TextPaM TextEngine::ImpConnectParagraphs( ULONG nLeft, ULONG nRight )
{
	DBG_ASSERT( nLeft != nRight, "Den gleichen Absatz zusammenfuegen ?" );

	TextNode* pLeft = mpDoc->GetNodes().GetObject( nLeft );
	TextNode* pRight = mpDoc->GetNodes().GetObject( nRight );

	if ( IsUndoEnabled() && !IsInUndo() )
		InsertUndo( new TextUndoConnectParas( this, nLeft, pLeft->GetText().Len() ) );

	// Erstmal Portions suchen, da pRight nach ConnectParagraphs weg.
	TEParaPortion* pLeftPortion = mpTEParaPortions->GetObject( nLeft );
	TEParaPortion* pRightPortion = mpTEParaPortions->GetObject( nRight );
	DBG_ASSERT( pLeft && pLeftPortion, "Blinde Portion in ImpConnectParagraphs(1)" );
	DBG_ASSERT( pRight && pRightPortion, "Blinde Portion in ImpConnectParagraphs(2)" );

	TextPaM aPaM = mpDoc->ConnectParagraphs( pLeft, pRight );
	ImpParagraphRemoved( nRight );

	pLeftPortion->MarkSelectionInvalid( aPaM.GetIndex(), pLeft->GetText().Len() );

	mpTEParaPortions->Remove( nRight );
	delete pRightPortion;
	// der rechte Node wird von EditDoc::ConnectParagraphs() geloescht.

	return aPaM;
}

TextPaM TextEngine::ImpDeleteText( const TextSelection& rSel )
{
	if ( !rSel.HasRange() )
		return rSel.GetStart();

	TextSelection aSel( rSel );
	aSel.Justify();
	TextPaM aStartPaM( aSel.GetStart() );
	TextPaM aEndPaM( aSel.GetEnd() );

	CursorMoved( aStartPaM.GetPara() ); // nur damit neu eingestellte Attribute verschwinden...
	CursorMoved( aEndPaM.GetPara() );	// nur damit neu eingestellte Attribute verschwinden...

	DBG_ASSERT( mpDoc->IsValidPaM( aStartPaM ), "Index im Wald in ImpDeleteText" );
	DBG_ASSERT( mpDoc->IsValidPaM( aEndPaM ), "Index im Wald in ImpDeleteText" );

	ULONG nStartNode = aStartPaM.GetPara();
	ULONG nEndNode = aEndPaM.GetPara();

	// Alle Nodes dazwischen entfernen....
	for ( ULONG z = nStartNode+1; z < nEndNode; z++ )
	{
		// Immer nStartNode+1, wegen Remove()!
		ImpRemoveParagraph( nStartNode+1 );
	}

	if ( nStartNode != nEndNode )
	{
		// Den Rest des StartNodes...
		TextNode* pLeft = mpDoc->GetNodes().GetObject( nStartNode );
		USHORT nChars = pLeft->GetText().Len() - aStartPaM.GetIndex();
		if ( nChars )
		{
			ImpRemoveChars( aStartPaM, nChars );
			TEParaPortion* pPortion = mpTEParaPortions->GetObject( nStartNode );
			DBG_ASSERT( pPortion, "Blinde Portion in ImpDeleteText(3)" );
			pPortion->MarkSelectionInvalid( aStartPaM.GetIndex(), pLeft->GetText().Len() );
		}

		// Den Anfang des EndNodes....
		nEndNode = nStartNode+1;	// Die anderen Absaetze wurden geloescht
		nChars = aEndPaM.GetIndex();
		if ( nChars )
		{
			aEndPaM.GetPara() = nEndNode;
			aEndPaM.GetIndex() = 0;
			ImpRemoveChars( aEndPaM, nChars );
			TEParaPortion* pPortion = mpTEParaPortions->GetObject( nEndNode );
			DBG_ASSERT( pPortion, "Blinde Portion in ImpDeleteText(4)" );
			pPortion->MarkSelectionInvalid( 0, pPortion->GetNode()->GetText().Len() );
		}

		// Zusammenfuegen....
		aStartPaM = ImpConnectParagraphs( nStartNode, nEndNode );
	}
	else
	{
		USHORT nChars;
		nChars = aEndPaM.GetIndex() - aStartPaM.GetIndex();
		ImpRemoveChars( aStartPaM, nChars );
		TEParaPortion* pPortion = mpTEParaPortions->GetObject( nStartNode );
		DBG_ASSERT( pPortion, "Blinde Portion in ImpDeleteText(5)" );
		pPortion->MarkInvalid( aEndPaM.GetIndex(), aStartPaM.GetIndex() - aEndPaM.GetIndex() );
	}

//	UpdateSelections();
	TextModified();
	return aStartPaM;
}

void TextEngine::ImpRemoveParagraph( ULONG nPara )
{
	TextNode* pNode = mpDoc->GetNodes().GetObject( nPara );
	TEParaPortion* pPortion = mpTEParaPortions->GetObject( nPara );

	// Der Node wird vom Undo verwaltet und ggf. zerstoert!
	/* delete */ mpDoc->GetNodes().Remove( nPara );
	if ( IsUndoEnabled() && !IsInUndo() )
		InsertUndo( new TextUndoDelPara( this, pNode, nPara ) );
	else
		delete pNode;

	mpTEParaPortions->Remove( nPara );
	delete pPortion;

	ImpParagraphRemoved( nPara );
}

uno::Reference < i18n::XExtendedInputSequenceChecker > TextEngine::GetInputSequenceChecker() const
{
    uno::Reference < i18n::XExtendedInputSequenceChecker > xISC;
//    if ( !xISC.is() )
    {
        uno::Reference< lang::XMultiServiceFactory > xMSF = ::comphelper::getProcessServiceFactory();
        uno::Reference< uno::XInterface > xI = xMSF->createInstance( OUString::createFromAscii( "com.sun.star.i18n.InputSequenceChecker" ) );
        if ( xI.is() )
        {
            Any x = xI->queryInterface( ::getCppuType((const uno::Reference< i18n::XExtendedInputSequenceChecker >*)0) );
            x >>= xISC;
        }
    }
    return xISC;
}

BOOL TextEngine::IsInputSequenceCheckingRequired( sal_Unicode c, const TextSelection& rCurSel ) const
{
    uno::Reference< i18n::XBreakIterator > xBI = ((TextEngine *) this)->GetBreakIterator();
    SvtCTLOptions aCTLOptions;

    // get the index that really is first
    USHORT nFirstPos = rCurSel.GetStart().GetIndex();
    USHORT nMaxPos   = rCurSel.GetEnd().GetIndex();
    if (nMaxPos < nFirstPos)
        nFirstPos = nMaxPos;
    
    sal_Bool bIsSequenceChecking =  
        aCTLOptions.IsCTLFontEnabled() &&
        aCTLOptions.IsCTLSequenceChecking() && 
        nFirstPos != 0 && /* first char needs not to be checked */
        xBI.is() && i18n::ScriptType::COMPLEX == xBI->getScriptType( rtl::OUString( c ), 0 );
    
    return bIsSequenceChecking;
}

TextPaM TextEngine::ImpInsertText( const TextSelection& rCurSel, sal_Unicode c, BOOL bOverwrite )
{
    return ImpInsertText( c, rCurSel, bOverwrite, sal_False );
}

TextPaM TextEngine::ImpInsertText( sal_Unicode c, const TextSelection& rCurSel, BOOL bOverwrite, BOOL bIsUserInput )
{
    DBG_ASSERT( c != '\n', "Zeilenumbruch bei InsertText ?" );
    DBG_ASSERT( c != '\r', "Zeilenumbruch bei InsertText ?" );

    TextPaM aPaM( rCurSel.GetStart() );
    TextNode* pNode = mpDoc->GetNodes().GetObject( aPaM.GetPara() );

    if ( pNode->GetText().Len() < STRING_MAXLEN )
    {
        BOOL bDoOverwrite = ( bOverwrite &&
                ( aPaM.GetIndex() < pNode->GetText().Len() ) ) ? TRUE : FALSE;

        BOOL bUndoAction = ( rCurSel.HasRange() || bDoOverwrite );

        if ( bUndoAction )
            UndoActionStart( TEXTUNDO_INSERT );

        if ( rCurSel.HasRange() )
        {
            aPaM = ImpDeleteText( rCurSel );
        }
        else if ( bDoOverwrite )
        {
            // Wenn Selektion, dann kein Zeichen ueberschreiben
            TextSelection aTmpSel( aPaM );
            aTmpSel.GetEnd().GetIndex()++;
            ImpDeleteText( aTmpSel );
        }

        if (bIsUserInput && IsInputSequenceCheckingRequired( c, rCurSel ))
        {
            uno::Reference < i18n::XExtendedInputSequenceChecker > xISC = GetInputSequenceChecker();
            SvtCTLOptions aCTLOptions;

            if (xISC.is())
            {
                xub_StrLen nTmpPos = aPaM.GetIndex();
                sal_Int16 nCheckMode = aCTLOptions.IsCTLSequenceCheckingRestricted() ? 
                        i18n::InputSequenceCheckMode::STRICT : i18n::InputSequenceCheckMode::BASIC;

                // the text that needs to be checked is only the one 
                // before the current cursor position
                rtl::OUString aOldText( mpDoc->GetText( aPaM.GetPara() ).Copy(0, nTmpPos) );
                rtl::OUString aNewText( aOldText );
                if (aCTLOptions.IsCTLSequenceCheckingTypeAndReplace())
                {
                    xISC->correctInputSequence( aNewText, nTmpPos - 1, c, nCheckMode );

                    // find position of first character that has changed
                    sal_Int32 nOldLen = aOldText.getLength();
                    sal_Int32 nNewLen = aNewText.getLength();
                    const sal_Unicode *pOldTxt = aOldText.getStr();
                    const sal_Unicode *pNewTxt = aNewText.getStr();
                    sal_Int32 nChgPos = 0;
                    while ( nChgPos < nOldLen && nChgPos < nNewLen &&
                            pOldTxt[nChgPos] == pNewTxt[nChgPos] )
                        ++nChgPos;

                    xub_StrLen nChgLen = static_cast< xub_StrLen >(nNewLen - nChgPos);
                    String aChgText( aNewText.copy( nChgPos ), nChgLen );

                    // select text from first pos to be changed to current pos
                    TextSelection aSel( TextPaM( aPaM.GetPara(), (USHORT) nChgPos ), aPaM );

                    if (aChgText.Len())
                        // ImpInsertText implicitly handles undo...
                        return ImpInsertText( aSel, aChgText );
                    else
                        return aPaM;
                }
                else
                {
                    // should the character be ignored (i.e. not get inserted) ?
                    if (!xISC->checkInputSequence( aOldText, nTmpPos - 1, c, nCheckMode ))
                        return aPaM;    // nothing to be done -> no need for undo
                }
            }

            // at this point now we will insert the character 'normally' some lines below...
        }


        if ( IsUndoEnabled() && !IsInUndo() )
        {
            TextUndoInsertChars* pNewUndo = new TextUndoInsertChars( this, aPaM, c );
            BOOL bTryMerge = ( !bDoOverwrite && ( c != ' ' ) ) ? TRUE : FALSE;
            InsertUndo( pNewUndo, bTryMerge );
        }

        TEParaPortion* pPortion = mpTEParaPortions->GetObject( aPaM.GetPara() );
        pPortion->MarkInvalid( aPaM.GetIndex(), 1 );
        if ( c == '\t' )
            pPortion->SetNotSimpleInvalid();
        aPaM = mpDoc->InsertText( aPaM, c );
        ImpCharsInserted( aPaM.GetPara(), aPaM.GetIndex()-1, 1 );

        TextModified();

        if ( bUndoAction )
            UndoActionEnd( TEXTUNDO_INSERT );
    }

    return aPaM;
}


TextPaM TextEngine::ImpInsertText( const TextSelection& rCurSel, const XubString& rStr )
{
    UndoActionStart( TEXTUNDO_INSERT );

	TextPaM aPaM;

	if ( rCurSel.HasRange() )
		aPaM = ImpDeleteText( rCurSel );
	else
		aPaM = rCurSel.GetEnd();

	XubString aText( rStr );
	aText.ConvertLineEnd( LINEEND_LF );

	USHORT nStart = 0;
	while ( nStart < aText.Len() )
	{
		USHORT nEnd = aText.Search( LINE_SEP, nStart );
		if ( nEnd == STRING_NOTFOUND )
			nEnd = aText.Len();	// nicht dereferenzieren!

		// Start == End => Leerzeile
		if ( nEnd > nStart )
		{
			ULONG nL = aPaM.GetIndex();
			nL += ( nEnd-nStart );
			if ( nL > STRING_MAXLEN )
			{
				USHORT nDiff = (USHORT) (nL-STRING_MAXLEN);
				nEnd = nEnd - nDiff;
			}

			XubString aLine( aText, nStart, nEnd-nStart );
			if ( IsUndoEnabled() && !IsInUndo() )
				InsertUndo( new TextUndoInsertChars( this, aPaM, aLine ) );

			TEParaPortion* pPortion = mpTEParaPortions->GetObject( aPaM.GetPara() );
			pPortion->MarkInvalid( aPaM.GetIndex(), aLine.Len() );
			if ( aLine.Search( '\t' ) != STRING_NOTFOUND )
				pPortion->SetNotSimpleInvalid();

			aPaM = mpDoc->InsertText( aPaM, aLine );
			ImpCharsInserted( aPaM.GetPara(), aPaM.GetIndex()-aLine.Len(), aLine.Len() );

		}
		if ( nEnd < aText.Len() )
			aPaM = ImpInsertParaBreak( aPaM );

		nStart = nEnd+1;
		
        if ( nStart < nEnd )    // #108611# overflow 
            break;
	}

    UndoActionEnd( TEXTUNDO_INSERT );

    TextModified();
	return aPaM;
}

TextPaM TextEngine::ImpInsertParaBreak( const TextSelection& rCurSel, BOOL bKeepEndingAttribs )
{
	TextPaM aPaM;
	if ( rCurSel.HasRange() )
		aPaM = ImpDeleteText( rCurSel );
	else
		aPaM = rCurSel.GetEnd();

	return ImpInsertParaBreak( aPaM, bKeepEndingAttribs );
}

TextPaM TextEngine::ImpInsertParaBreak( const TextPaM& rPaM, BOOL bKeepEndingAttribs )
{
	if ( IsUndoEnabled() && !IsInUndo() )
		InsertUndo( new TextUndoSplitPara( this, rPaM.GetPara(), rPaM.GetIndex() ) );

	TextNode* pNode = mpDoc->GetNodes().GetObject( rPaM.GetPara() );
	BOOL bFirstParaContentChanged = rPaM.GetIndex() < pNode->GetText().Len();

	TextPaM aPaM( mpDoc->InsertParaBreak( rPaM, bKeepEndingAttribs ) );

	TEParaPortion* pPortion = mpTEParaPortions->GetObject( rPaM.GetPara() );
	DBG_ASSERT( pPortion, "Blinde Portion in ImpInsertParaBreak" );
	pPortion->MarkInvalid( rPaM.GetIndex(), 0 );

	TextNode* pNewNode = mpDoc->GetNodes().GetObject( aPaM.GetPara() );
	TEParaPortion* pNewPortion = new TEParaPortion( pNewNode );
	mpTEParaPortions->Insert( pNewPortion, aPaM.GetPara() );
	ImpParagraphInserted( aPaM.GetPara() );

	CursorMoved( rPaM.GetPara() );	// falls leeres Attribut entstanden.
	TextModified();

	if ( bFirstParaContentChanged )
		Broadcast( TextHint( TEXT_HINT_PARACONTENTCHANGED, rPaM.GetPara() ) );

	return aPaM;
}

Rectangle TextEngine::PaMtoEditCursor( const TextPaM& rPaM, BOOL bSpecial )
{
	DBG_ASSERT( GetUpdateMode(), "Darf bei Update=FALSE nicht erreicht werden: PaMtoEditCursor" );

	Rectangle aEditCursor;
	long nY = 0;

	if ( !mbHasMultiLineParas )
	{
		nY = rPaM.GetPara() * mnCharHeight;
	}
	else
	{
		for ( ULONG nPortion = 0; nPortion < rPaM.GetPara(); nPortion++ )
		{
			TEParaPortion* pPortion = mpTEParaPortions->GetObject(nPortion);
			nY += pPortion->GetLines().Count() * mnCharHeight;
		}
	}

	aEditCursor = GetEditCursor( rPaM, bSpecial );
	aEditCursor.Top() += nY;
	aEditCursor.Bottom() += nY;
	return aEditCursor;
}

Rectangle TextEngine::GetEditCursor( const TextPaM& rPaM, BOOL bSpecial, BOOL bPreferPortionStart )
{
    if ( !IsFormatted() && !IsFormatting() )
		FormatAndUpdate();

	TEParaPortion* pPortion = mpTEParaPortions->GetObject( rPaM.GetPara() );
	//TextNode* pNode = mpDoc->GetNodes().GetObject( rPaM.GetPara() );

	/*
	 bSpecial:	Wenn hinter dem letzten Zeichen einer umgebrochenen Zeile,
	 am Ende der Zeile bleiben, nicht am Anfang der naechsten.
	 Zweck: 	- END => wirklich hinter das letzte Zeichen
				- Selektion....
	  bSpecial: If behind the last character of a made up line, stay at the
	  			end of the line, not at the start of the next line.
	  Purpose:  - really END = > behind the last character
	  			- to selection...

	*/

	long nY = 0;
	USHORT nCurIndex = 0;
	TextLine* pLine = 0;
	for ( USHORT nLine = 0; nLine < pPortion->GetLines().Count(); nLine++ )
	{
		TextLine* pTmpLine = pPortion->GetLines().GetObject( nLine );
		if ( ( pTmpLine->GetStart() == rPaM.GetIndex() ) || ( pTmpLine->IsIn( rPaM.GetIndex(), bSpecial ) ) )
		{
			pLine = pTmpLine;
			break;
		}

		nCurIndex = nCurIndex + pTmpLine->GetLen();
		nY += mnCharHeight;
	}
	if ( !pLine )
	{
		// Cursor am Ende des Absatzes.
		DBG_ASSERT( rPaM.GetIndex() == nCurIndex, "Index voll daneben in GetEditCursor!" );

		pLine = pPortion->GetLines().GetObject( pPortion->GetLines().Count()-1 );
		nY -= mnCharHeight;
		nCurIndex = nCurIndex - pLine->GetLen();
	}

	Rectangle aEditCursor;

	aEditCursor.Top() = nY;
	nY += mnCharHeight;
	aEditCursor.Bottom() = nY-1;

	// innerhalb der Zeile suchen....
	long nX = ImpGetXPos( rPaM.GetPara(), pLine, rPaM.GetIndex(), bPreferPortionStart );
	aEditCursor.Left() = aEditCursor.Right() = nX;
	return aEditCursor;
}

long TextEngine::ImpGetXPos( ULONG nPara, TextLine* pLine, USHORT nIndex, BOOL bPreferPortionStart )
{
	DBG_ASSERT( ( nIndex >= pLine->GetStart() ) && ( nIndex <= pLine->GetEnd() ) , "ImpGetXPos muss richtig gerufen werden!" );

    BOOL bDoPreferPortionStart = bPreferPortionStart;
    // Assure that the portion belongs to this line:
    if ( nIndex == pLine->GetStart() )
        bDoPreferPortionStart = TRUE;
    else if ( nIndex == pLine->GetEnd() )
        bDoPreferPortionStart = FALSE;

	TEParaPortion* pParaPortion = mpTEParaPortions->GetObject( nPara );

    USHORT nTextPortionStart = 0;
    USHORT nTextPortion = pParaPortion->GetTextPortions().FindPortion( nIndex, nTextPortionStart, bDoPreferPortionStart );

    DBG_ASSERT( ( nTextPortion >= pLine->GetStartPortion() ) && ( nTextPortion <= pLine->GetEndPortion() ), "GetXPos: Portion not in current line! " );

    TETextPortion* pPortion = pParaPortion->GetTextPortions().GetObject( nTextPortion );

    long nX = ImpGetPortionXOffset( nPara, pLine, nTextPortion );

    long nPortionTextWidth = pPortion->GetWidth();

    if ( nTextPortionStart != nIndex )
    {
        // Search within portion...
        if ( nIndex == ( nTextPortionStart + pPortion->GetLen() ) )
        {
            // End of Portion
            if ( ( pPortion->GetKind() == PORTIONKIND_TAB ) || 
                 ( !IsRightToLeft() && !pPortion->IsRightToLeft() ) ||
                 ( IsRightToLeft() && pPortion->IsRightToLeft() ) )
            {
				nX += nPortionTextWidth;
                if ( ( pPortion->GetKind() == PORTIONKIND_TAB ) && ( (nTextPortion+1) < pParaPortion->GetTextPortions().Count() ) )
                {
                    TETextPortion* pNextPortion = pParaPortion->GetTextPortions().GetObject( nTextPortion+1 );
                    if ( ( pNextPortion->GetKind() != PORTIONKIND_TAB ) && ( 
                              ( !IsRightToLeft() && pNextPortion->IsRightToLeft() ) ||
                              ( IsRightToLeft() && !pNextPortion->IsRightToLeft() ) ) )
                    {
//                        nX += pNextPortion->GetWidth();
                        // End of the tab portion, use start of next for cursor pos
                        DBG_ASSERT( !bPreferPortionStart, "ImpGetXPos - How can we this tab portion here???" );
                        nX = ImpGetXPos( nPara, pLine, nIndex, TRUE );
                    }

                }
            }
        }
        else if ( pPortion->GetKind() == PORTIONKIND_TEXT )
        {
			DBG_ASSERT( nIndex != pLine->GetStart(), "Strange behavior in new ImpGetXPos()" );

            long nPosInPortion = (long)CalcTextWidth( nPara, nTextPortionStart, nIndex-nTextPortionStart );

            if ( ( !IsRightToLeft() && !pPortion->IsRightToLeft() ) ||
                 ( IsRightToLeft() && pPortion->IsRightToLeft() ) )
            {
				nX += nPosInPortion;
            }
            else
            {
				nX += nPortionTextWidth - nPosInPortion;
            }
		}
    }
    else // if ( nIndex == pLine->GetStart() )
    {
        if ( ( pPortion->GetKind() != PORTIONKIND_TAB ) &&
                ( ( !IsRightToLeft() && pPortion->IsRightToLeft() ) ||
                ( IsRightToLeft() && !pPortion->IsRightToLeft() ) ) )
        {
		    nX += nPortionTextWidth;
        }
    }

	return nX;
}

const TextAttrib* TextEngine::FindAttrib( const TextPaM& rPaM, USHORT nWhich ) const
{
	const TextAttrib* pAttr = NULL;
	const TextCharAttrib* pCharAttr = FindCharAttrib( rPaM, nWhich );
	if ( pCharAttr )
		pAttr = &pCharAttr->GetAttr();
	return pAttr;
}

const TextCharAttrib* TextEngine::FindCharAttrib( const TextPaM& rPaM, USHORT nWhich ) const
{
	const TextCharAttrib* pAttr = NULL;
	TextNode* pNode = mpDoc->GetNodes().GetObject( rPaM.GetPara() );
	if ( pNode && ( rPaM.GetIndex() < pNode->GetText().Len() ) )
		pAttr = pNode->GetCharAttribs().FindAttrib( nWhich, rPaM.GetIndex() );
	return pAttr;
}

BOOL TextEngine::HasAttrib( USHORT nWhich ) const
{
	BOOL bAttr = FALSE;
	for ( ULONG n = mpDoc->GetNodes().Count(); --n && !bAttr; )
	{
		TextNode* pNode = mpDoc->GetNodes().GetObject( n );
		bAttr = pNode->GetCharAttribs().HasAttrib( nWhich );
	}
	return bAttr;
}

TextPaM TextEngine::GetPaM( const Point& rDocPos, BOOL bSmart )
{
	DBG_ASSERT( GetUpdateMode(), "Darf bei Update=FALSE nicht erreicht werden: GetPaM" );

	long nY = 0;
	for ( ULONG nPortion = 0; nPortion < mpTEParaPortions->Count(); nPortion++ )
	{
		TEParaPortion* pPortion = mpTEParaPortions->GetObject( nPortion );
		long nTmpHeight = pPortion->GetLines().Count() * mnCharHeight;
		nY += nTmpHeight;
		if ( nY > rDocPos.Y() )
		{
			nY -= nTmpHeight;
			Point aPosInPara( rDocPos );
			aPosInPara.Y() -= nY;

			TextPaM aPaM( nPortion, 0 );
			aPaM.GetIndex() = ImpFindIndex( nPortion, aPosInPara, bSmart );
			return aPaM;
		}
	}

	// Nicht gefunden - Dann den letzten sichtbare...
	ULONG nLastNode = mpDoc->GetNodes().Count() - 1;
	TextNode* pLast = mpDoc->GetNodes().GetObject( nLastNode );
	return TextPaM( nLastNode, pLast->GetText().Len() );
}

USHORT TextEngine::ImpFindIndex( ULONG nPortion, const Point& rPosInPara, BOOL bSmart )
{
	DBG_ASSERT( IsFormatted(), "GetPaM: Nicht formatiert" );
	TEParaPortion* pPortion = mpTEParaPortions->GetObject( nPortion );

	USHORT nCurIndex = 0;

	long nY = 0;
	TextLine* pLine = 0;
	USHORT nLine;
	for ( nLine = 0; nLine < pPortion->GetLines().Count(); nLine++ )
	{
		TextLine* pTmpLine = pPortion->GetLines().GetObject( nLine );
		nY += mnCharHeight;
		if ( nY > rPosInPara.Y() ) 	// das war 'se
		{
			pLine = pTmpLine;
			break;					// richtige Y-Position intressiert nicht
		}
	}
	DBG_ASSERT( pLine, "ImpFindIndex: pLine ?" );

	nCurIndex = GetCharPos( nPortion, nLine, rPosInPara.X(), bSmart );

	if ( nCurIndex && ( nCurIndex == pLine->GetEnd() ) &&
		 ( pLine != pPortion->GetLines().GetObject( pPortion->GetLines().Count()-1) ) )
	{
		uno::Reference < i18n::XBreakIterator > xBI = GetBreakIterator();
		sal_Int32 nCount = 1;
		nCurIndex = (USHORT)xBI->previousCharacters( pPortion->GetNode()->GetText(), nCurIndex, GetLocale(), i18n::CharacterIteratorMode::SKIPCELL, nCount, nCount );
	}
	return nCurIndex;
}

USHORT TextEngine::GetCharPos( ULONG nPortion, USHORT nLine, long nXPos, BOOL )
{

	TEParaPortion* pPortion = mpTEParaPortions->GetObject( nPortion );
	TextLine* pLine = pPortion->GetLines().GetObject( nLine );

	USHORT nCurIndex = pLine->GetStart();

	long nTmpX = pLine->GetStartX();
	if ( nXPos <= nTmpX )
		return nCurIndex;

	for ( USHORT i = pLine->GetStartPortion(); i <= pLine->GetEndPortion(); i++ )
	{
		TETextPortion* pTextPortion = pPortion->GetTextPortions().GetObject( i );
		nTmpX += pTextPortion->GetWidth();

		if ( nTmpX > nXPos )
		{
			if( pTextPortion->GetLen() > 1 )
			{
				nTmpX -= pTextPortion->GetWidth();	// vor die Portion stellen
				// Optimieren: Kein GetTextBreak, wenn feste Fontbreite...
				Font aFont;
				SeekCursor( nPortion, nCurIndex+1, aFont, NULL );
				mpRefDev->SetFont( aFont);
                long nPosInPortion = nXPos-nTmpX;
                if ( IsRightToLeft() != pTextPortion->IsRightToLeft() )
                    nPosInPortion = pTextPortion->GetWidth() - nPosInPortion;
				nCurIndex = mpRefDev->GetTextBreak( pPortion->GetNode()->GetText(), nPosInPortion, nCurIndex );
                // MT: GetTextBreak should assure that we are not withing a CTL cell...
			}
			return nCurIndex;
		}
		nCurIndex = nCurIndex + pTextPortion->GetLen();
	}
	return nCurIndex;
}


ULONG TextEngine::GetTextHeight() const
{
	DBG_ASSERT( GetUpdateMode(), "Sollte bei Update=FALSE nicht verwendet werden: GetTextHeight" );

    if ( !IsFormatted() && !IsFormatting() )
		((TextEngine*)this)->FormatAndUpdate();

	return mnCurTextHeight;
}

ULONG TextEngine::GetTextHeight( ULONG nParagraph ) const
{
	DBG_ASSERT( GetUpdateMode(), "Sollte bei Update=FALSE nicht verwendet werden: GetTextHeight" );

  	if ( !IsFormatted() && !IsFormatting() )
		((TextEngine*)this)->FormatAndUpdate();

	return CalcParaHeight( nParagraph );
}

ULONG TextEngine::CalcTextWidth( ULONG nPara )
{
	ULONG nParaWidth = 0;
	TEParaPortion* pPortion = mpTEParaPortions->GetObject( nPara );
	for ( USHORT nLine = pPortion->GetLines().Count(); nLine; )
	{
		ULONG nLineWidth = 0;
		TextLine* pLine = pPortion->GetLines().GetObject( --nLine );
		for ( USHORT nTP = pLine->GetStartPortion(); nTP <= pLine->GetEndPortion(); nTP++ )
		{
			TETextPortion* pTextPortion = pPortion->GetTextPortions().GetObject( nTP );
			nLineWidth += pTextPortion->GetWidth();
		}
		if ( nLineWidth > nParaWidth )
			nParaWidth = nLineWidth;
	}
	return nParaWidth;
}

ULONG TextEngine::CalcTextWidth()
{
	if ( !IsFormatted() && !IsFormatting() )
		FormatAndUpdate();

	if ( mnCurTextWidth == 0xFFFFFFFF )
	{
		mnCurTextWidth = 0;
		for ( ULONG nPara = mpTEParaPortions->Count(); nPara; )
		{
			ULONG nParaWidth = CalcTextWidth( --nPara );
			if ( nParaWidth > mnCurTextWidth )
				mnCurTextWidth = nParaWidth; 
		}
	}
	return mnCurTextWidth+1;// Ein breiter, da in CreateLines bei >= umgebrochen wird.
}

ULONG TextEngine::CalcTextHeight()
{
	DBG_ASSERT( GetUpdateMode(), "Sollte bei Update=FALSE nicht verwendet werden: CalcTextHeight" );

	ULONG nY = 0;
	for ( ULONG nPortion = mpTEParaPortions->Count(); nPortion; )
		nY += CalcParaHeight( --nPortion );
	return nY;
}

ULONG TextEngine::CalcTextWidth( ULONG nPara, USHORT nPortionStart, USHORT nLen, const Font* pFont )
{
	// Innerhalb des Textes darf es keinen Portionwechsel (Attribut/Tab) geben!
	DBG_ASSERT( mpDoc->GetNodes().GetObject( nPara )->GetText().Search( '\t', nPortionStart ) >= (nPortionStart+nLen), "CalcTextWidth: Tab!" );

	ULONG nWidth;
	if ( mnFixCharWidth100 )
    {
		nWidth = (ULONG)nLen*mnFixCharWidth100/100;
    }
	else
	{
		if ( pFont )
		{
			if ( !mpRefDev->GetFont().IsSameInstance( *pFont ) )
				mpRefDev->SetFont( *pFont );
		}
		else
		{
			Font aFont;
			SeekCursor( nPara, nPortionStart+1, aFont, NULL );
			mpRefDev->SetFont( aFont );
		}
		TextNode* pNode = mpDoc->GetNodes().GetObject( nPara );
		nWidth = (ULONG)mpRefDev->GetTextWidth( pNode->GetText(), nPortionStart, nLen );

	}
	return nWidth;
}


USHORT TextEngine::GetLineCount( ULONG nParagraph ) const
{
	DBG_ASSERT( nParagraph < mpTEParaPortions->Count(), "GetLineCount: Out of range" );

	TEParaPortion* pPPortion = mpTEParaPortions->GetObject( nParagraph );
	if ( pPPortion )
		return pPPortion->GetLines().Count();

	return 0xFFFF;
}

USHORT TextEngine::GetLineLen( ULONG nParagraph, USHORT nLine ) const
{
	DBG_ASSERT( nParagraph < mpTEParaPortions->Count(), "GetLineCount: Out of range" );

	TEParaPortion* pPPortion = mpTEParaPortions->GetObject( nParagraph );
	if ( pPPortion && ( nLine < pPPortion->GetLines().Count() ) )
	{
		TextLine* pLine = pPPortion->GetLines().GetObject( nLine );
		return pLine->GetLen();
	}

	return 0xFFFF;
}

ULONG TextEngine::CalcParaHeight( ULONG nParagraph ) const
{
	ULONG nHeight = 0;

	TEParaPortion* pPPortion = mpTEParaPortions->GetObject( nParagraph );
	DBG_ASSERT( pPPortion, "Absatz nicht gefunden: GetParaHeight" );
	if ( pPPortion )
		nHeight = pPPortion->GetLines().Count() * mnCharHeight;

	return nHeight;
}

void TextEngine::UpdateSelections()
{
}

Range TextEngine::GetInvalidYOffsets( ULONG nPortion )
{
	TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPortion );
	USHORT nLines = pTEParaPortion->GetLines().Count();
	USHORT nLastInvalid, nFirstInvalid = 0;
	USHORT nLine;
	for ( nLine = 0; nLine < nLines; nLine++ )
	{
		TextLine* pL = pTEParaPortion->GetLines().GetObject( nLine );
		if ( pL->IsInvalid() )
		{
			nFirstInvalid = nLine;
			break;
		}
	}

	for ( nLastInvalid = nFirstInvalid; nLastInvalid < nLines; nLastInvalid++ )
	{
		TextLine* pL = pTEParaPortion->GetLines().GetObject( nLine );
		if ( pL->IsValid() )
			break;
	}

	if ( nLastInvalid >= nLines )
		nLastInvalid = nLines-1;

	return Range( nFirstInvalid*mnCharHeight, ((nLastInvalid+1)*mnCharHeight)-1 );
}

ULONG TextEngine::GetParagraphCount() const
{
	return mpDoc->GetNodes().Count();
}

void TextEngine::EnableUndo( BOOL bEnable )
{
	// Beim Umschalten des Modus Liste loeschen:
	if ( bEnable != IsUndoEnabled() )
		ResetUndo();

	mbUndoEnabled = bEnable;
}

SfxUndoManager& TextEngine::GetUndoManager()
{
	if ( !mpUndoManager )
		mpUndoManager = new TextUndoManager( this );
	return *mpUndoManager;
}

void TextEngine::UndoActionStart( USHORT nId )
{
	if ( IsUndoEnabled() && !IsInUndo() )
	{
		String aComment;
		// ...
		GetUndoManager().EnterListAction( aComment, XubString(), nId );
	}
}

void TextEngine::UndoActionEnd( USHORT )
{
	if ( IsUndoEnabled() && !IsInUndo() )
		GetUndoManager().LeaveListAction();
}

void TextEngine::InsertUndo( TextUndo* pUndo, BOOL bTryMerge )
{
	DBG_ASSERT( !IsInUndo(), "InsertUndo im Undomodus!" );
	GetUndoManager().AddUndoAction( pUndo, bTryMerge );
}

void TextEngine::ResetUndo()
{
	if ( mpUndoManager )
		mpUndoManager->Clear();
}

void TextEngine::InsertContent( TextNode* pNode, ULONG nPara )
{
	DBG_ASSERT( pNode, "NULL-Pointer in InsertContent! " );
	DBG_ASSERT( IsInUndo(), "InsertContent nur fuer Undo()!" );
	TEParaPortion* pNew = new TEParaPortion( pNode );
	mpTEParaPortions->Insert( pNew, nPara );
	mpDoc->GetNodes().Insert( pNode, nPara );
	ImpParagraphInserted( nPara );
}

TextPaM TextEngine::SplitContent( ULONG nNode, USHORT nSepPos )
{
    #ifdef DBG_UTIL
	TextNode* pNode = mpDoc->GetNodes().GetObject( nNode );
	DBG_ASSERT( pNode, "Ungueltiger Node in SplitContent" );
	DBG_ASSERT( IsInUndo(), "SplitContent nur fuer Undo()!" );
	DBG_ASSERT( nSepPos <= pNode->GetText().Len(), "Index im Wald: SplitContent" );
    #endif
	TextPaM aPaM( nNode, nSepPos );
	return ImpInsertParaBreak( aPaM );
}

TextPaM TextEngine::ConnectContents( ULONG nLeftNode )
{
	DBG_ASSERT( IsInUndo(), "ConnectContent nur fuer Undo()!" );
	return ImpConnectParagraphs( nLeftNode, nLeftNode+1 );
}

void TextEngine::SeekCursor( ULONG nPara, USHORT nPos, Font& rFont, OutputDevice* pOutDev )
{
	rFont = maFont;
	if ( pOutDev )
		pOutDev->SetTextColor( maTextColor );

	TextNode* pNode = mpDoc->GetNodes().GetObject( nPara );
	USHORT nAttribs = pNode->GetCharAttribs().Count();
	for ( USHORT nAttr = 0; nAttr < nAttribs; nAttr++ )
	{
		TextCharAttrib* pAttrib = pNode->GetCharAttribs().GetAttrib( nAttr );
		if ( pAttrib->GetStart() > nPos )
			break;

		// Beim Seeken nicht die Attr beruecksichtigen, die dort beginnen!
		// Leere Attribute werden beruecksichtigt( verwendet), da diese
		// gerade eingestellt wurden.
		// 12.4.95: Doch keine Leeren Attribute verwenden:
		// - Wenn gerade eingestellt und leer => keine Auswirkung auf Font
		// In einem leeren Absatz eingestellte Zeichen werden sofort wirksam.
		if ( ( ( pAttrib->GetStart() < nPos ) && ( pAttrib->GetEnd() >= nPos ) )
					|| !pNode->GetText().Len() )
		{
			if ( pAttrib->Which() != TEXTATTR_FONTCOLOR )
			{
				pAttrib->GetAttr().SetFont( rFont );
			}
			else
			{
				if ( pOutDev )
					pOutDev->SetTextColor( ((TextAttribFontColor&)pAttrib->GetAttr()).GetColor() );
			}
		}
	}

	if ( mpIMEInfos && mpIMEInfos->pAttribs && ( mpIMEInfos->aPos.GetPara() == nPara ) &&
		( nPos > mpIMEInfos->aPos.GetIndex() ) && ( nPos <= ( mpIMEInfos->aPos.GetIndex() + mpIMEInfos->nLen ) ) )
	{
		sal_uInt16 nAttr = mpIMEInfos->pAttribs[ nPos - mpIMEInfos->aPos.GetIndex() - 1 ];
		if ( nAttr & EXTTEXTINPUT_ATTR_UNDERLINE )
			rFont.SetUnderline( UNDERLINE_SINGLE );
		else if ( nAttr & EXTTEXTINPUT_ATTR_BOLDUNDERLINE )
			rFont.SetUnderline( UNDERLINE_BOLD );
		else if ( nAttr & EXTTEXTINPUT_ATTR_DOTTEDUNDERLINE )
			rFont.SetUnderline( UNDERLINE_DOTTED );
		else if ( nAttr & EXTTEXTINPUT_ATTR_DASHDOTUNDERLINE )
			rFont.SetUnderline( UNDERLINE_DOTTED );
		if ( nAttr & EXTTEXTINPUT_ATTR_REDTEXT )
			rFont.SetColor( Color( COL_RED ) );
		else if ( nAttr & EXTTEXTINPUT_ATTR_HALFTONETEXT )
			rFont.SetColor( Color( COL_LIGHTGRAY ) );
		if ( nAttr & EXTTEXTINPUT_ATTR_HIGHLIGHT )
		{
			const StyleSettings& rStyleSettings = Application::GetSettings().GetStyleSettings();
			rFont.SetColor( rStyleSettings.GetHighlightTextColor() );
			rFont.SetFillColor( rStyleSettings.GetHighlightColor() );
			rFont.SetTransparent( FALSE );
		}
		else if ( nAttr & EXTTEXTINPUT_ATTR_GRAYWAVELINE )
		{
			rFont.SetUnderline( UNDERLINE_WAVE );
//			if( pOut )
//				pOut->SetTextLineColor( Color( COL_LIGHTGRAY ) );
		}
	}
}

void TextEngine::SetUpdateMode( BOOL bUp, TextView* pCurView, BOOL bForceUpdate )
{
	BOOL bChanged = ( GetUpdateMode() != bUp );

	mbUpdate = bUp;
	if ( mbUpdate && ( bChanged || bForceUpdate ) )
		FormatAndUpdate( pCurView );
}

void TextEngine::FormatAndUpdate( TextView* pCurView )
{
	if ( mbDowning )
		return ;

	if ( IsInUndo() )
		IdleFormatAndUpdate( pCurView );
	else
	{
		FormatDoc();
		UpdateViews( pCurView );
	}
}


void TextEngine::IdleFormatAndUpdate( TextView* pCurView, USHORT nMaxTimerRestarts )
{
	mpIdleFormatter->DoIdleFormat( pCurView, nMaxTimerRestarts );
}

void TextEngine::TextModified()
{
	mbFormatted = FALSE;
	mbModified = TRUE;
}

void TextEngine::UpdateViews( TextView* pCurView )
{
	if ( !GetUpdateMode() || IsFormatting() || maInvalidRec.IsEmpty() )
		return;

	DBG_ASSERT( IsFormatted(), "UpdateViews: Doc nicht formatiert!" );

	for ( USHORT nView = 0; nView < mpViews->Count(); nView++ )
	{
		TextView* pView = mpViews->GetObject( nView );
		pView->HideCursor();

		Rectangle aClipRec( maInvalidRec );
        Size aOutSz = pView->GetWindow()->GetOutputSizePixel();
		Rectangle aVisArea( pView->GetStartDocPos(), aOutSz );
		aClipRec.Intersection( aVisArea );
		if ( !aClipRec.IsEmpty() )
		{
			// in Fensterkoordinaten umwandeln....
            Point aNewPos = pView->GetWindowPos( aClipRec.TopLeft() );
            if ( IsRightToLeft() )
                aNewPos.X() -= aOutSz.Width() - 1;
			aClipRec.SetPos( aNewPos );

			if ( pView == pCurView )
				pView->ImpPaint( aClipRec, !pView->GetWindow()->IsPaintTransparent() );
			else
				pView->GetWindow()->Invalidate( aClipRec );
		}
	}

	if ( pCurView )
	{
        pCurView->ShowCursor( pCurView->IsAutoScroll() );
	}

	maInvalidRec = Rectangle();
}

IMPL_LINK( TextEngine, IdleFormatHdl, Timer *, EMPTYARG )
{
	FormatAndUpdate( mpIdleFormatter->GetView() );
	return 0;
}

void TextEngine::CheckIdleFormatter()
{
	mpIdleFormatter->ForceTimeout();
}

void TextEngine::FormatFullDoc()
{
	for ( ULONG nPortion = 0; nPortion < mpTEParaPortions->Count(); nPortion++ )
	{
		TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPortion );		USHORT nLen = pTEParaPortion->GetNode()->GetText().Len();
		pTEParaPortion->MarkSelectionInvalid( 0, nLen );
	}
	mbFormatted = FALSE;
	FormatDoc();
}

void TextEngine::FormatDoc()
{
	if ( IsFormatted() || !GetUpdateMode() || IsFormatting() )
		return;

	mbIsFormatting = TRUE;
	mbHasMultiLineParas = FALSE;

	long nY = 0;
	BOOL bGrow = FALSE;

	maInvalidRec = Rectangle();	// leermachen
	for ( ULONG nPara = 0; nPara < mpTEParaPortions->Count(); nPara++ )
	{
		TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPara );
		if ( pTEParaPortion->IsInvalid() )
		{
			ULONG nOldParaWidth = 0xFFFFFFFF;
			if ( mnCurTextWidth != 0xFFFFFFFF )
				nOldParaWidth = CalcTextWidth( nPara );

			ImpFormattingParagraph( nPara );

			if ( CreateLines( nPara ) )
				bGrow = TRUE;

			// InvalidRec nur einmal setzen...
			if ( maInvalidRec.IsEmpty() )
			{
				// Bei Paperwidth 0 (AutoPageSize) bleibt es sonst Empty()...
				long nWidth = (long)mnMaxTextWidth;
				if ( !nWidth )
					nWidth = 0x7FFFFFFF;
				Range aInvRange( GetInvalidYOffsets( nPara ) );
				maInvalidRec = Rectangle( Point( 0, nY+aInvRange.Min() ),
					Size( nWidth, aInvRange.Len() ) );
			}
			else
			{
				maInvalidRec.Bottom() = nY + CalcParaHeight( nPara );
			}

			if ( mnCurTextWidth != 0xFFFFFFFF )
			{
				ULONG nNewParaWidth = CalcTextWidth( nPara );
				if ( nNewParaWidth >= mnCurTextWidth )
					mnCurTextWidth = nNewParaWidth; 
				else if ( ( nOldParaWidth != 0xFFFFFFFF ) && ( nOldParaWidth >= mnCurTextWidth ) )
					mnCurTextWidth = 0xFFFFFFFF;
			}
		}
		else if ( bGrow )
		{
			maInvalidRec.Bottom() = nY + CalcParaHeight( nPara );
		}
		nY += CalcParaHeight( nPara );
		if ( !mbHasMultiLineParas && pTEParaPortion->GetLines().Count() > 1 )
			mbHasMultiLineParas = TRUE;
	}

	if ( !maInvalidRec.IsEmpty() )
	{
		ULONG nNewHeight = CalcTextHeight();
		long nDiff = nNewHeight - mnCurTextHeight;
		if ( nNewHeight < mnCurTextHeight )
		{
			maInvalidRec.Bottom() = (long)Max( nNewHeight, mnCurTextHeight );
			if ( maInvalidRec.IsEmpty() )
			{
				maInvalidRec.Top() = 0;
				// Left und Right werden nicht ausgewertet, aber wegen IsEmpty gesetzt.
				maInvalidRec.Left() = 0;
				maInvalidRec.Right() = mnMaxTextWidth;
			}
		}

		mnCurTextHeight = nNewHeight;
		if ( nDiff )
		{
			mbFormatted = TRUE;
			ImpTextHeightChanged();
		}
	}

	mbIsFormatting = FALSE;
	mbFormatted = TRUE;

	ImpTextFormatted();
}

void TextEngine::CreateAndInsertEmptyLine( ULONG nPara )
{
	TextNode* pNode = mpDoc->GetNodes().GetObject( nPara );
	TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPara );

	TextLine* pTmpLine = new TextLine;
	pTmpLine->SetStart( pNode->GetText().Len() );
	pTmpLine->SetEnd( pTmpLine->GetStart() );
	pTEParaPortion->GetLines().Insert( pTmpLine, pTEParaPortion->GetLines().Count() );

	if ( ImpGetAlign() == TXTALIGN_CENTER )
		pTmpLine->SetStartX( (short)(mnMaxTextWidth / 2) );
	else if ( ImpGetAlign() == TXTALIGN_RIGHT )
		pTmpLine->SetStartX( (short)mnMaxTextWidth );
	else
		pTmpLine->SetStartX( mpDoc->GetLeftMargin() );

	BOOL bLineBreak = pNode->GetText().Len() ? TRUE : FALSE;

	TETextPortion* pDummyPortion = new TETextPortion( 0 );
	pDummyPortion->GetWidth() = 0;
	pTEParaPortion->GetTextPortions().Insert( pDummyPortion, pTEParaPortion->GetTextPortions().Count() );

	if ( bLineBreak == TRUE )
	{
		// -2: Die neue ist bereits eingefuegt.
        #ifdef DBG_UTIL
		TextLine* pLastLine = pTEParaPortion->GetLines().GetObject( pTEParaPortion->GetLines().Count()-2 );
		DBG_ASSERT( pLastLine, "Weicher Umbruch, keine Zeile ?!" );
        #endif
		USHORT nPos = (USHORT) pTEParaPortion->GetTextPortions().Count() - 1 ;
		pTmpLine->SetStartPortion( nPos );
		pTmpLine->SetEndPortion( nPos );
	}
}

void TextEngine::ImpBreakLine( ULONG nPara, TextLine* pLine, TETextPortion*, USHORT nPortionStart, long nRemainingWidth )
{
	TextNode* pNode = mpDoc->GetNodes().GetObject( nPara );

	// Font sollte noch eingestellt sein.
	USHORT nMaxBreakPos = mpRefDev->GetTextBreak( pNode->GetText(), nRemainingWidth, nPortionStart );

	DBG_ASSERT( nMaxBreakPos < pNode->GetText().Len(), "Break?!" );

	if ( nMaxBreakPos == STRING_LEN )	// GetTextBreak() ist anderer Auffassung als GetTextSize()
		nMaxBreakPos = pNode->GetText().Len() - 1;

	uno::Reference < i18n::XBreakIterator > xBI = GetBreakIterator();
	i18n::LineBreakHyphenationOptions aHyphOptions( NULL, uno::Sequence< beans::PropertyValue >(), 1 );

    i18n::LineBreakUserOptions aUserOptions;
	aUserOptions.forbiddenBeginCharacters = ImpGetLocaleDataWrapper()->getForbiddenCharacters().beginLine;
	aUserOptions.forbiddenEndCharacters = ImpGetLocaleDataWrapper()->getForbiddenCharacters().endLine;
	aUserOptions.applyForbiddenRules = sal_True;
	aUserOptions.allowPunctuationOutsideMargin = sal_False;
	aUserOptions.allowHyphenateEnglish = sal_False;

    static const com::sun::star::lang::Locale aDefLocale;
	i18n::LineBreakResults aLBR = xBI->getLineBreak( pNode->GetText(), nMaxBreakPos, aDefLocale, pLine->GetStart(), aHyphOptions, aUserOptions );
	USHORT nBreakPos = (USHORT)aLBR.breakIndex;
	if ( nBreakPos <= pLine->GetStart() )
    {
		nBreakPos = nMaxBreakPos;
		if ( nBreakPos <= pLine->GetStart() )
			nBreakPos = pLine->GetStart() + 1; 	// Sonst Endlosschleife!
    }


	// die angeknackste Portion ist die End-Portion
	pLine->SetEnd( nBreakPos );
	USHORT nEndPortion = SplitTextPortion( nPara, nBreakPos );

	sal_Bool bBlankSeparator = ( ( nBreakPos >= pLine->GetStart() ) &&
								 ( pNode->GetText().GetChar( nBreakPos ) == ' ' ) ) ? sal_True : sal_False;
	if ( bBlankSeparator )
	{
		// Blanks am Zeilenende generell unterdruecken...
		TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPara );
		TETextPortion* pTP = pTEParaPortion->GetTextPortions().GetObject( nEndPortion );
		DBG_ASSERT( nBreakPos > pLine->GetStart(), "SplitTextPortion am Anfang der Zeile?" );
		pTP->GetWidth() = (long)CalcTextWidth( nPara, nBreakPos-pTP->GetLen(), pTP->GetLen()-1 );
	}
	pLine->SetEndPortion( nEndPortion );
}

USHORT TextEngine::SplitTextPortion( ULONG nPara, USHORT nPos )
{

	// Die Portion bei nPos wird geplittet, wenn bei nPos nicht
	// sowieso ein Wechsel ist
	if ( nPos == 0 )
		return 0;

	USHORT nSplitPortion;
	USHORT nTmpPos = 0;
	TETextPortion* pTextPortion = 0;
	TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPara );
	USHORT nPortions = pTEParaPortion->GetTextPortions().Count();
	for ( nSplitPortion = 0; nSplitPortion < nPortions; nSplitPortion++ )
	{
		TETextPortion* pTP = pTEParaPortion->GetTextPortions().GetObject(nSplitPortion);
		nTmpPos = nTmpPos + pTP->GetLen();
		if ( nTmpPos >= nPos )
		{
			if ( nTmpPos == nPos )	// dann braucht nichts geteilt werden
				return nSplitPortion;
			pTextPortion = pTP;
			break;
		}
	}

	DBG_ASSERT( pTextPortion, "Position ausserhalb des Bereichs!" );

	USHORT nOverlapp = nTmpPos - nPos;
	pTextPortion->GetLen() = pTextPortion->GetLen() - nOverlapp;
	TETextPortion* pNewPortion = new TETextPortion( nOverlapp );
	pTEParaPortion->GetTextPortions().Insert( pNewPortion, nSplitPortion+1 );
	pTextPortion->GetWidth() = (long)CalcTextWidth( nPara, nPos-pTextPortion->GetLen(), pTextPortion->GetLen() );

	return nSplitPortion;
}

void TextEngine::CreateTextPortions( ULONG nPara, USHORT nStartPos )
{
	TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPara );
	TextNode* pNode = pTEParaPortion->GetNode();
	DBG_ASSERT( pNode->GetText().Len(), "CreateTextPortions sollte nicht fuer leere Absaetze verwendet werden!" );

	TESortedPositions aPositions;
	ULONG nZero = 0;
	aPositions.Insert( nZero );

	USHORT nAttribs = pNode->GetCharAttribs().Count();
	for ( USHORT nAttr = 0; nAttr < nAttribs; nAttr++ )
	{
		TextCharAttrib* pAttrib = pNode->GetCharAttribs().GetAttrib( nAttr );

		// Start und Ende in das Array eintragen...
		// Die InsertMethode laesst keine doppelten Werte zu....
		aPositions.Insert( pAttrib->GetStart() );
		aPositions.Insert( pAttrib->GetEnd() );
	}
	aPositions.Insert( pNode->GetText().Len() );

    const TEWritingDirectionInfos& rWritingDirections = pTEParaPortion->GetWritingDirectionInfos();
	for ( USHORT nD = 0; nD < rWritingDirections.Count(); nD++ )
		aPositions.Insert( rWritingDirections[nD].nStartPos );

	if ( mpIMEInfos && mpIMEInfos->pAttribs && ( mpIMEInfos->aPos.GetPara() == nPara ) )
	{
		sal_uInt16 nLastAttr = 0xFFFF;
		for( sal_uInt16 n = 0; n < mpIMEInfos->nLen; n++ )
		{
			if ( mpIMEInfos->pAttribs[n] != nLastAttr )
			{
				aPositions.Insert( mpIMEInfos->aPos.GetIndex() + n );
				nLastAttr = mpIMEInfos->pAttribs[n];
			}
		}
	}

	USHORT nTabPos = pNode->GetText().Search( '\t', 0 );
	while ( nTabPos != STRING_NOTFOUND )
	{
		aPositions.Insert( nTabPos );
		aPositions.Insert( nTabPos + 1 );
		nTabPos = pNode->GetText().Search( '\t', nTabPos+1 );
	}

	// Ab ... loeschen:
	// Leider muss die Anzahl der TextPortions mit aPositions.Count()
	// nicht uebereinstimmen, da evtl. Zeilenumbrueche...
	USHORT nPortionStart = 0;
	USHORT nInvPortion = 0;
    USHORT nP;
	for ( nP = 0; nP < pTEParaPortion->GetTextPortions().Count(); nP++ )
	{
		TETextPortion* pTmpPortion = pTEParaPortion->GetTextPortions().GetObject(nP);
		nPortionStart = nPortionStart + pTmpPortion->GetLen();
		if ( nPortionStart >= nStartPos )
		{
			nPortionStart = nPortionStart - pTmpPortion->GetLen();
			nInvPortion = nP;
			break;
		}
	}
	DBG_ASSERT( nP < pTEParaPortion->GetTextPortions().Count() || !pTEParaPortion->GetTextPortions().Count(), "Nichts zum loeschen: CreateTextPortions" );
	if ( nInvPortion && ( nPortionStart+pTEParaPortion->GetTextPortions().GetObject(nInvPortion)->GetLen() > nStartPos ) )
	{
		// lieber eine davor...
		// Aber nur wenn es mitten in der Portion war, sonst ist es evtl.
		// die einzige in der Zeile davor !
		nInvPortion--;
		nPortionStart = nPortionStart - pTEParaPortion->GetTextPortions().GetObject(nInvPortion)->GetLen();
	}
	pTEParaPortion->GetTextPortions().DeleteFromPortion( nInvPortion );

	// Eine Portion kann auch durch einen Zeilenumbruch entstanden sein:
	aPositions.Insert( nPortionStart );

	USHORT nInvPos;
    #ifdef DBG_UTIL
	BOOL bFound = 
    #endif
        aPositions.Seek_Entry( nPortionStart, &nInvPos );
	DBG_ASSERT( bFound && ( nInvPos < (aPositions.Count()-1) ), "InvPos ?!" );
	for ( USHORT i = nInvPos+1; i < aPositions.Count(); i++ )
	{
		TETextPortion* pNew = new TETextPortion( (USHORT)aPositions[i] - (USHORT)aPositions[i-1] );
		pTEParaPortion->GetTextPortions().Insert( pNew, pTEParaPortion->GetTextPortions().Count());
	}

	DBG_ASSERT( pTEParaPortion->GetTextPortions().Count(), "Keine Portions?!" );
#ifdef EDITDEBUG
	DBG_ASSERT( pTEParaPortion->DbgCheckTextPortions(), "Portions kaputt?" );
#endif
}

void TextEngine::RecalcTextPortion( ULONG nPara, USHORT nStartPos, short nNewChars )
{
	TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPara );
	DBG_ASSERT( pTEParaPortion->GetTextPortions().Count(), "Keine Portions!" );
	DBG_ASSERT( nNewChars, "RecalcTextPortion mit Diff == 0" );

	TextNode* const pNode = pTEParaPortion->GetNode();
	if ( nNewChars > 0 )
	{
		// Wenn an nStartPos ein Attribut beginnt/endet, oder vor nStartPos
		// ein Tab steht, faengt eine neue Portion an,
		// ansonsten wird die Portion an nStartPos erweitert.
		// Oder wenn ganz vorne ( StartPos 0 ) und dann ein Tab

		if ( ( pNode->GetCharAttribs().HasBoundingAttrib( nStartPos ) ) ||
			 ( nStartPos && ( pNode->GetText().GetChar( nStartPos - 1 ) == '\t' ) ) ||
			 ( ( !nStartPos && ( nNewChars < pNode->GetText().Len() ) && pNode->GetText().GetChar( nNewChars ) == '\t' ) ) )
		{
			USHORT nNewPortionPos = 0;
			if ( nStartPos )
				nNewPortionPos = SplitTextPortion( nPara, nStartPos ) + 1;
//			else if ( ( pTEParaPortion->GetTextPortions().Count() == 1 ) &&
//						!pTEParaPortion->GetTextPortions()[0]->GetLen() )
//				pTEParaPortion->GetTextPortions().Reset();	// DummyPortion

			// Eine leere Portion kann hier stehen, wenn der Absatz leer war,
			// oder eine Zeile durch einen harten Zeilenumbruch entstanden ist.
			if ( ( nNewPortionPos < pTEParaPortion->GetTextPortions().Count() ) &&
					!pTEParaPortion->GetTextPortions()[nNewPortionPos]->GetLen() )
			{
				// Dann die leere Portion verwenden.
                USHORT & r =
                    pTEParaPortion->GetTextPortions()[nNewPortionPos]->GetLen();
                r = r + nNewChars;
			}
			else
			{
				TETextPortion* pNewPortion = new TETextPortion( nNewChars );
				pTEParaPortion->GetTextPortions().Insert( pNewPortion, nNewPortionPos );
			}
		}
		else
		{
			USHORT nPortionStart;
			const USHORT nTP = pTEParaPortion->GetTextPortions().
				FindPortion( nStartPos, nPortionStart );
			TETextPortion* const pTP = pTEParaPortion->GetTextPortions()[ nTP ];
			DBG_ASSERT( pTP, "RecalcTextPortion: Portion nicht gefunden"  );
			pTP->GetLen() = pTP->GetLen() + nNewChars;
			pTP->GetWidth() = (-1);
		}
	}
	else
	{
		// Portion schrumpfen oder ggf. entfernen.
		// Vor Aufruf dieser Methode muss sichergestellt sein, dass
		// keine Portions in dem geloeschten Bereich lagen!

		// Es darf keine reinragende oder im Bereich startende Portion geben,
		// also muss nStartPos <= nPos <= nStartPos - nNewChars(neg.) sein
		USHORT nPortion = 0;
		USHORT nPos = 0;
		USHORT nEnd = nStartPos-nNewChars;
		USHORT nPortions = pTEParaPortion->GetTextPortions().Count();
		TETextPortion* pTP = 0;
		for ( nPortion = 0; nPortion < nPortions; nPortion++ )
		{
			pTP = pTEParaPortion->GetTextPortions()[ nPortion ];
			if ( ( nPos+pTP->GetLen() ) > nStartPos )
			{
				DBG_ASSERT( nPos <= nStartPos, "Start falsch!" );
				DBG_ASSERT( nPos+pTP->GetLen() >= nEnd, "End falsch!" );
				break;
			}
			nPos = nPos + pTP->GetLen();
		}
		DBG_ASSERT( pTP, "RecalcTextPortion: Portion nicht gefunden" );
		if ( ( nPos == nStartPos ) && ( (nPos+pTP->GetLen()) == nEnd ) )
		{
			// Portion entfernen;
			pTEParaPortion->GetTextPortions().Remove( nPortion );
			delete pTP;
		}
		else
		{
			DBG_ASSERT( pTP->GetLen() > (-nNewChars), "Portion zu klein zum schrumpfen!" );
			pTP->GetLen() = pTP->GetLen() + nNewChars;
		}
		DBG_ASSERT( pTEParaPortion->GetTextPortions().Count(), "RecalcTextPortions: Keine mehr da!" );
	}

#ifdef EDITDEBUG
	DBG_ASSERT( pTEParaPortion->DbgCheckTextPortions(), "Portions kaputt?" );
#endif
}

void TextEngine::ImpPaint( OutputDevice* pOutDev, const Point& rStartPos, Rectangle const* pPaintArea, TextSelection const* pPaintRange, TextSelection const* pSelection )
{
	if ( !GetUpdateMode() )
		return;

	if ( !IsFormatted() )
		FormatDoc();
    
    bool bTransparent = false;
    Window* pOutWin = dynamic_cast<Window*>(pOutDev);
    bTransparent = (pOutWin && pOutWin->IsPaintTransparent());

	long nY = rStartPos.Y();

	TextPaM const* pSelStart = 0;
	TextPaM const* pSelEnd = 0;
	if ( pSelection && pSelection->HasRange() )
	{
		BOOL bInvers = pSelection->GetEnd() < pSelection->GetStart();
		pSelStart = !bInvers ? &pSelection->GetStart() : &pSelection->GetEnd();
		pSelEnd = bInvers ? &pSelection->GetStart() : &pSelection->GetEnd();
	}
	DBG_ASSERT( !pPaintRange || ( pPaintRange->GetStart() < pPaintRange->GetEnd() ), "ImpPaint: Paint-Range?!" );

	const StyleSettings& rStyleSettings = pOutDev->GetSettings().GetStyleSettings();

	// --------------------------------------------------
	// Ueber alle Absaetze...
	// --------------------------------------------------
	for ( ULONG nPara = 0; nPara < mpTEParaPortions->Count(); nPara++ )
	{
		TEParaPortion* pPortion = mpTEParaPortions->GetObject( nPara );
		// falls beim Tippen Idle-Formatierung, asynchrones Paint.
		if ( pPortion->IsInvalid() )
			return;

		ULONG nParaHeight = CalcParaHeight( nPara );
		USHORT nIndex = 0;
		if ( ( !pPaintArea || ( ( nY + (long)nParaHeight ) > pPaintArea->Top() ) )
				&& ( !pPaintRange || ( ( nPara >= pPaintRange->GetStart().GetPara() ) && ( nPara <= pPaintRange->GetEnd().GetPara() ) ) ) )
		{
			// --------------------------------------------------
			// Ueber die Zeilen des Absatzes...
			// --------------------------------------------------
			USHORT nLines = pPortion->GetLines().Count();
			for ( USHORT nLine = 0; nLine < nLines; nLine++ )
			{
				TextLine* pLine = pPortion->GetLines().GetObject(nLine);
				Point aTmpPos( rStartPos.X() + pLine->GetStartX(), nY );

				if ( ( !pPaintArea || ( ( nY + mnCharHeight ) > pPaintArea->Top() ) )
					&& ( !pPaintRange || (
						( TextPaM( nPara, pLine->GetStart() ) < pPaintRange->GetEnd() ) &&
						( TextPaM( nPara, pLine->GetEnd() ) > pPaintRange->GetStart() ) ) ) )
				{
					// --------------------------------------------------
					// Ueber die Portions der Zeile...
					// --------------------------------------------------
					nIndex = pLine->GetStart();
					for ( USHORT y = pLine->GetStartPortion(); y <= pLine->GetEndPortion(); y++ )
					{
						DBG_ASSERT( pPortion->GetTextPortions().Count(), "Zeile ohne Textportion im Paint!" );
						TETextPortion* pTextPortion = pPortion->GetTextPortions().GetObject( y );
						DBG_ASSERT( pTextPortion, "NULL-Pointer im Portioniterator in UpdateViews" );

                        ImpInitLayoutMode( pOutDev /*, pTextPortion->IsRightToLeft() */);

						long nTxtWidth = pTextPortion->GetWidth();
                        aTmpPos.X() = rStartPos.X() + ImpGetOutputOffset( nPara, pLine, nIndex, nIndex );

						// nur ausgeben, was im sichtbaren Bereich beginnt:
						if ( ( ( aTmpPos.X() + nTxtWidth ) >= 0 )
							&& ( !pPaintRange || (
								( TextPaM( nPara, nIndex ) < pPaintRange->GetEnd() ) &&
									( TextPaM( nPara, nIndex + pTextPortion->GetLen() ) > pPaintRange->GetStart() ) ) ) )
						{
							switch ( pTextPortion->GetKind() )
							{
								case PORTIONKIND_TEXT:
								{
									{
										Font aFont;
										SeekCursor( nPara, nIndex+1, aFont, pOutDev );
                                        if( bTransparent )
                                            aFont.SetTransparent( TRUE );
										else if ( pSelection )
											aFont.SetTransparent( FALSE );
										pOutDev->SetFont( aFont );

										USHORT nTmpIndex = nIndex;
										USHORT nEnd = nTmpIndex + pTextPortion->GetLen();
										Point aPos = aTmpPos;
										if ( pPaintRange )
										{
											// evtl soll nicht alles ausgegeben werden...
											if ( ( pPaintRange->GetStart().GetPara() == nPara )
													&& ( nTmpIndex < pPaintRange->GetStart().GetIndex() ) )
											{
												nTmpIndex = pPaintRange->GetStart().GetIndex();
											}
											if ( ( pPaintRange->GetEnd().GetPara() == nPara )
													&& ( nEnd > pPaintRange->GetEnd().GetIndex() ) )
											{
												nEnd = pPaintRange->GetEnd().GetIndex();
											}
										}

										BOOL bDone = FALSE;
										if ( pSelStart )
										{
											// liegt ein Teil in der Selektion???
											TextPaM aTextStart( nPara, nTmpIndex );
											TextPaM aTextEnd( nPara, nEnd );
											if ( ( aTextStart < *pSelEnd ) && ( aTextEnd > *pSelStart ) )
											{
												USHORT nL;

												// 1) Bereich vor Selektion
												if ( aTextStart < *pSelStart )
												{
													nL = pSelStart->GetIndex() - nTmpIndex;
													pOutDev->SetFont( aFont);
                                                    aPos.X() = rStartPos.X() + ImpGetOutputOffset( nPara, pLine, nTmpIndex, nTmpIndex+nL );
													pOutDev->DrawText( aPos, pPortion->GetNode()->GetText(), nTmpIndex, nL );
													nTmpIndex = nTmpIndex + nL;

												}
												// 2) Bereich mit Selektion
												nL = nEnd-nTmpIndex;
												if ( aTextEnd > *pSelEnd )
													nL = pSelEnd->GetIndex() - nTmpIndex;
												if ( nL )
												{
													Color aOldTextColor = pOutDev->GetTextColor();
													pOutDev->SetTextColor( rStyleSettings.GetHighlightTextColor() );
													pOutDev->SetTextFillColor( rStyleSettings.GetHighlightColor() );
													aPos.X() = rStartPos.X() + ImpGetOutputOffset( nPara, pLine, nTmpIndex, nTmpIndex+nL );
													pOutDev->DrawText( aPos, pPortion->GetNode()->GetText(), nTmpIndex, nL );
													pOutDev->SetTextColor( aOldTextColor );
													pOutDev->SetTextFillColor();
													nTmpIndex = nTmpIndex + nL;
												}

												// 3) Bereich nach Selektion
												if ( nTmpIndex < nEnd )
												{
												    nL = nEnd-nTmpIndex;
                                                    aPos.X() = rStartPos.X() + ImpGetOutputOffset( nPara, pLine, nTmpIndex, nTmpIndex+nL );
													pOutDev->DrawText( aPos, pPortion->GetNode()->GetText(), nTmpIndex, nEnd-nTmpIndex );
												}
												bDone = TRUE;
											}
										}
										if ( !bDone )
                                        {
                                            aPos.X() = rStartPos.X() + ImpGetOutputOffset( nPara, pLine, nTmpIndex, nEnd );
											pOutDev->DrawText( aPos, pPortion->GetNode()->GetText(), nTmpIndex, nEnd-nTmpIndex );
                                        }
									}

								}
								break;
								case PORTIONKIND_TAB:
								{
									// Bei HideSelection() nur Range, pSelection = 0.
									if ( pSelStart || pPaintRange )
									{
										Rectangle aTabArea( aTmpPos, Point( aTmpPos.X()+nTxtWidth, aTmpPos.Y()+mnCharHeight-1 ) );
										BOOL bDone = FALSE;
										if ( pSelStart )
										{
											// liegt der Tab in der Selektion???
											TextPaM aTextStart( nPara, nIndex );
											TextPaM aTextEnd( nPara, nIndex+1 );
											if ( ( aTextStart < *pSelEnd ) && ( aTextEnd > *pSelStart ) )
											{
												Color aOldColor = pOutDev->GetFillColor();
												pOutDev->SetFillColor( rStyleSettings.GetHighlightColor() );
												pOutDev->DrawRect( aTabArea );
												pOutDev->SetFillColor( aOldColor );
												bDone = TRUE;
											}
										}
										if ( !bDone )
										{
											pOutDev->Erase( aTabArea );
										}
									}
#ifdef EDITDEBUG
									Rectangle aTabArea( aTmpPos, Point( aTmpPos.X()+nTxtWidth, aTmpPos.Y()+mnCharHeight-1 ) );
									Color aOldColor = pOutDev->GetFillColor();
                                    pOutDev->SetFillColor( (y%2) ? COL_RED : COL_GREEN );
									pOutDev->DrawRect( aTabArea );
									pOutDev->SetFillColor( aOldColor );
#endif
								}
								break;
								default:	DBG_ERROR( "ImpPaint: Unknown Portion-Type !" );
							}
						}

						nIndex = nIndex + pTextPortion->GetLen();
					}
				}

				nY += mnCharHeight;

				if ( pPaintArea && ( nY >= pPaintArea->Bottom() ) )
					break;	// keine sichtbaren Aktionen mehr...
			}
		}
		else
		{
			nY += nParaHeight;
		}

		if ( pPaintArea && ( nY > pPaintArea->Bottom() ) )
			break;	// keine sichtbaren Aktionen mehr...
	}
}

BOOL TextEngine::CreateLines( ULONG nPara )
{
	// BOOL: Aenderung der Hoehe des Absatzes Ja/Nein - TRUE/FALSE

	TextNode* pNode = mpDoc->GetNodes().GetObject( nPara );
	TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPara );
	DBG_ASSERT( pTEParaPortion->IsInvalid(), "CreateLines: Portion nicht invalid!" );

	USHORT nOldLineCount = pTEParaPortion->GetLines().Count();

	// ---------------------------------------------------------------
	// Schnelle Sonderbehandlung fuer leere Absaetze...
	// ---------------------------------------------------------------
	if ( pTEParaPortion->GetNode()->GetText().Len() == 0 )
	{
		// schnelle Sonderbehandlung...
		if ( pTEParaPortion->GetTextPortions().Count() )
			pTEParaPortion->GetTextPortions().Reset();
		if ( pTEParaPortion->GetLines().Count() )
			pTEParaPortion->GetLines().DeleteAndDestroy( 0, pTEParaPortion->GetLines().Count() );
		CreateAndInsertEmptyLine( nPara );
		pTEParaPortion->SetValid();
		return nOldLineCount != pTEParaPortion->GetLines().Count();
	}

	// ---------------------------------------------------------------
	// Initialisierung......
	// ---------------------------------------------------------------

	if ( pTEParaPortion->GetLines().Count() == 0 )
	{
		TextLine* pL = new TextLine;
		pTEParaPortion->GetLines().Insert( pL, 0 );
	}

	const short nInvalidDiff = pTEParaPortion->GetInvalidDiff();
	const USHORT nInvalidStart = pTEParaPortion->GetInvalidPosStart();
	const USHORT nInvalidEnd =  nInvalidStart + Abs( nInvalidDiff );
	BOOL bQuickFormat = FALSE;

	if ( !pTEParaPortion->GetWritingDirectionInfos().Count() )
		ImpInitWritingDirections( nPara );

    if ( pTEParaPortion->GetWritingDirectionInfos().Count() == 1 )
    {
	    if ( pTEParaPortion->IsSimpleInvalid() && ( nInvalidDiff > 0 ) )
	    {
		    bQuickFormat = TRUE;
	    }
	    else if ( ( pTEParaPortion->IsSimpleInvalid() ) && ( nInvalidDiff < 0 ) )
	    {
		    // pruefen, ob loeschen ueber Portiongrenzen erfolgte...
		    USHORT nStart = nInvalidStart;	// DOPPELT !!!!!!!!!!!!!!!
		    USHORT nEnd = nStart - nInvalidDiff;  // neg.
		    bQuickFormat = TRUE;
		    USHORT nPos = 0;
		    USHORT nPortions = pTEParaPortion->GetTextPortions().Count();
		    for ( USHORT nTP = 0; nTP < nPortions; nTP++ )
		    {
			    // Es darf kein Start/Ende im geloeschten Bereich liegen.
			    TETextPortion* const pTP = pTEParaPortion->GetTextPortions().GetObject( nTP );
			    nPos = nPos + pTP->GetLen();
			    if ( ( nPos > nStart ) && ( nPos < nEnd ) )
			    {
				    bQuickFormat = FALSE;
				    break;
			    }
		    }
	    }
    }

	if ( bQuickFormat )
		RecalcTextPortion( nPara, nInvalidStart, nInvalidDiff );
	else
		CreateTextPortions( nPara, nInvalidStart );

	// ---------------------------------------------------------------
	// Zeile mit InvalidPos suchen, eine Zeile davor beginnen...
	// Zeilen flaggen => nicht removen !
	// ---------------------------------------------------------------

	USHORT nLine = pTEParaPortion->GetLines().Count()-1;
	for ( USHORT nL = 0; nL <= nLine; nL++ )
	{
		TextLine* pLine = pTEParaPortion->GetLines().GetObject( nL );
		if ( pLine->GetEnd() > nInvalidStart )
		{
			nLine = nL;
			break;
		}
		pLine->SetValid();
	}
	// Eine Zeile davor beginnen...
	// Wenn ganz hinten getippt wird, kann sich die Zeile davor nicht aendern.
	if ( nLine && ( !pTEParaPortion->IsSimpleInvalid() || ( nInvalidEnd < pNode->GetText().Len() ) || ( nInvalidDiff <= 0 ) ) )
		nLine--;

	TextLine* pLine = pTEParaPortion->GetLines().GetObject( nLine );

	// ---------------------------------------------------------------
	// Ab hier alle Zeilen durchformatieren...
	// ---------------------------------------------------------------
	USHORT nDelFromLine = 0xFFFF;
	BOOL bLineBreak = FALSE;

	USHORT nIndex = pLine->GetStart();
	TextLine aSaveLine( *pLine );

	Font aFont;

	BOOL bCalcPortion = TRUE;

	while ( nIndex < pNode->GetText().Len() )
	{
		BOOL bEOL = FALSE;
		BOOL bEOC = FALSE;
		USHORT nPortionStart = 0;
		USHORT nPortionEnd = 0;

		USHORT nTmpPos = nIndex;
		USHORT nTmpPortion = pLine->GetStartPortion();
		long nTmpWidth = mpDoc->GetLeftMargin();
//		long nXWidth = mnMaxTextWidth ? ( mnMaxTextWidth - mpDoc->GetLeftMargin() ) : 0x7FFFFFFF;
		// Margin nicht abziehen, ist schon in TmpWidth enthalten.
		long nXWidth = mnMaxTextWidth ? mnMaxTextWidth : 0x7FFFFFFF;
		if ( nXWidth < nTmpWidth )
			nXWidth = nTmpWidth;

		// Portion suchen, die nicht mehr in Zeile passt....
		TETextPortion* pPortion = 0;
		BOOL bBrokenLine = FALSE;
		bLineBreak = FALSE;

		while ( ( nTmpWidth <= nXWidth ) && !bEOL && ( nTmpPortion < pTEParaPortion->GetTextPortions().Count() ) )
		{
			nPortionStart = nTmpPos;
			pPortion = pTEParaPortion->GetTextPortions().GetObject( nTmpPortion );
			DBG_ASSERT( pPortion->GetLen(), "Leere Portion in CreateLines ?!" );
			if ( pNode->GetText().GetChar( nTmpPos ) == '\t' )
			{
				long nCurPos = nTmpWidth-mpDoc->GetLeftMargin();
				nTmpWidth = ((nCurPos/mnDefTab)+1)*mnDefTab+mpDoc->GetLeftMargin();
				pPortion->GetWidth() = nTmpWidth - nCurPos - mpDoc->GetLeftMargin();
				// Wenn dies das erste Token in der Zeile ist, und
				// nTmpWidth > aPaperSize.Width, habe ich eine Endlos-Schleife!
				if ( ( nTmpWidth >= nXWidth ) && ( nTmpPortion == pLine->GetStartPortion() ) )
				{
					// Aber was jetzt ? Tab passend machen!
					pPortion->GetWidth() = nXWidth-1;
					nTmpWidth = pPortion->GetWidth();
					bEOL = TRUE;
					bBrokenLine = TRUE;
				}
				pPortion->GetKind() = PORTIONKIND_TAB;
			}
			else
			{

				if ( bCalcPortion || !pPortion->HasValidSize() )
					pPortion->GetWidth() = (long)CalcTextWidth( nPara, nTmpPos, pPortion->GetLen() );
				nTmpWidth += pPortion->GetWidth();

                pPortion->GetRightToLeft() = ImpGetRightToLeft( nPara, nTmpPos+1 );
				pPortion->GetKind() = PORTIONKIND_TEXT;
			}

			nTmpPos = nTmpPos + pPortion->GetLen();
			nPortionEnd = nTmpPos;
			nTmpPortion++;
		}

		// das war evtl. eine Portion zu weit:
		BOOL bFixedEnd = FALSE;
		if ( nTmpWidth > nXWidth )
		{
			nPortionEnd = nTmpPos;
			nTmpPos = nTmpPos - pPortion->GetLen();
			nPortionStart = nTmpPos;
			nTmpPortion--;
			bEOL = FALSE;
			bEOC = FALSE;

			nTmpWidth -= pPortion->GetWidth();
			if ( pPortion->GetKind() == PORTIONKIND_TAB )
			{
				bEOL = TRUE;
				bFixedEnd = TRUE;
			}
		}
		else
		{
			bEOL = TRUE;
			bEOC = TRUE;
			pLine->SetEnd( nPortionEnd );
			DBG_ASSERT( pTEParaPortion->GetTextPortions().Count(), "Keine TextPortions?" );
			pLine->SetEndPortion( (USHORT)pTEParaPortion->GetTextPortions().Count() - 1 );
		}

		if ( bFixedEnd )
		{
			pLine->SetEnd( nPortionStart );
			pLine->SetEndPortion( nTmpPortion-1 );
		}
		else if ( bLineBreak || bBrokenLine )
		{
			pLine->SetEnd( nPortionStart+1 );
			pLine->SetEndPortion( nTmpPortion-1 );
			bEOC = FALSE; // wurde oben gesetzt, vielleich mal die if's umstellen?
		}
		else if ( !bEOL )
		{
			DBG_ASSERT( (nPortionEnd-nPortionStart) == pPortion->GetLen(), "Doch eine andere Portion?!" );
			long nRemainingWidth = mnMaxTextWidth - nTmpWidth;
			ImpBreakLine( nPara, pLine, pPortion, nPortionStart, nRemainingWidth );
		}

		if ( ( ImpGetAlign() == TXTALIGN_CENTER ) || ( ImpGetAlign() == TXTALIGN_RIGHT ) )
		{
			// Ausrichten...
			long nTextWidth = 0;
			for ( USHORT nTP = pLine->GetStartPortion(); nTP <= pLine->GetEndPortion(); nTP++ )
			{
				TETextPortion* pTextPortion = pTEParaPortion->GetTextPortions().GetObject( nTP );
				nTextWidth += pTextPortion->GetWidth();
			}
			long nSpace = mnMaxTextWidth - nTextWidth;
			if ( nSpace > 0 )
			{
				if ( ImpGetAlign() == TXTALIGN_CENTER )
					pLine->SetStartX( (USHORT)(nSpace / 2) );
				else	// TXTALIGN_RIGHT
					pLine->SetStartX( (USHORT)nSpace );
			}
		}
		else
		{
			pLine->SetStartX( mpDoc->GetLeftMargin() );
		}

		// -----------------------------------------------------------------
		// pruefen, ob die Zeile neu ausgegeben werden muss...
		// -----------------------------------------------------------------
		pLine->SetInvalid();

		if ( pTEParaPortion->IsSimpleInvalid() )
		{
			// Aenderung durch einfache Textaenderung...
			// Formatierung nicht abbrechen, da Portions evtl. wieder
			// gesplittet werden muessen!
			// Wenn irgendwann mal abbrechbar, dann fogende Zeilen Validieren!
			// Aber ggf. als Valid markieren, damit weniger Ausgabe...
			if ( pLine->GetEnd() < nInvalidStart )
			{
				if ( *pLine == aSaveLine )
				{
					pLine->SetValid();
				}
			}
			else
			{
				USHORT nStart = pLine->GetStart();
				USHORT nEnd = pLine->GetEnd();

				if ( nStart > nInvalidEnd )
				{
					if ( ( ( nStart-nInvalidDiff ) == aSaveLine.GetStart() ) &&
							( ( nEnd-nInvalidDiff ) == aSaveLine.GetEnd() ) )
					{
						pLine->SetValid();
						if ( bCalcPortion && bQuickFormat )
						{
							bCalcPortion = FALSE;
							pTEParaPortion->CorrectValuesBehindLastFormattedLine( nLine );
							break;
						}
					}
				}
				else if ( bQuickFormat && ( nEnd > nInvalidEnd) )
				{
					// Wenn die ungueltige Zeile so endet, dass die naechste an
					// der 'gleichen' Textstelle wie vorher beginnt, also nicht
					// anders umgebrochen wird, brauche ich dort auch nicht die
					// textbreiten neu bestimmen:
					if ( nEnd == ( aSaveLine.GetEnd() + nInvalidDiff ) )
					{
						bCalcPortion = FALSE;
						pTEParaPortion->CorrectValuesBehindLastFormattedLine( nLine );
						break;
					}
				}
			}
		}

		nIndex = pLine->GetEnd();	// naechste Zeile Start = letzte Zeile Ende
									// weil nEnd hinter das letzte Zeichen zeigt!

		USHORT nEndPortion = pLine->GetEndPortion();

		// Naechste Zeile oder ggf. neue Zeile....
		pLine = 0;
		if ( nLine < pTEParaPortion->GetLines().Count()-1 )
			pLine = pTEParaPortion->GetLines().GetObject( ++nLine );
		if ( pLine && ( nIndex >= pNode->GetText().Len() ) )
		{
			nDelFromLine = nLine;
			break;
		}
		if ( !pLine && ( nIndex < pNode->GetText().Len() )  )
		{
			pLine = new TextLine;
			pTEParaPortion->GetLines().Insert( pLine, ++nLine );
		}
		if ( pLine )
		{
			aSaveLine = *pLine;
			pLine->SetStart( nIndex );
			pLine->SetEnd( nIndex );
			pLine->SetStartPortion( nEndPortion+1 );
			pLine->SetEndPortion( nEndPortion+1 );
		}
	}	// while ( Index < Len )

	if ( nDelFromLine != 0xFFFF )
		pTEParaPortion->GetLines().DeleteAndDestroy( nDelFromLine, pTEParaPortion->GetLines().Count() - nDelFromLine );

	DBG_ASSERT( pTEParaPortion->GetLines().Count(), "Keine Zeile nach CreateLines!" );

	if ( bLineBreak == TRUE )
		CreateAndInsertEmptyLine( nPara );

	pTEParaPortion->SetValid();

	return nOldLineCount != pTEParaPortion->GetLines().Count();
}

String TextEngine::GetWord( const TextPaM& rCursorPos, TextPaM* pStartOfWord )
{
	String aWord;
	if ( rCursorPos.GetPara() < mpDoc->GetNodes().Count() )
	{
		TextSelection aSel( rCursorPos );
		TextNode* pNode = mpDoc->GetNodes().GetObject(  rCursorPos.GetPara() );
		uno::Reference < i18n::XBreakIterator > xBI = GetBreakIterator();
		i18n::Boundary aBoundary = xBI->getWordBoundary( pNode->GetText(), rCursorPos.GetIndex(), GetLocale(), i18n::WordType::ANYWORD_IGNOREWHITESPACES, sal_True );
		aSel.GetStart().GetIndex() = (USHORT)aBoundary.startPos;
		aSel.GetEnd().GetIndex() = (USHORT)aBoundary.endPos;
		aWord = pNode->GetText().Copy( aSel.GetStart().GetIndex(), aSel.GetEnd().GetIndex() - aSel.GetStart().GetIndex() );
		if ( pStartOfWord )
			*pStartOfWord = aSel.GetStart();
	}
	return aWord;
}

BOOL TextEngine::Read( SvStream& rInput, const TextSelection* pSel )
{
	BOOL bUpdate = GetUpdateMode();
	SetUpdateMode( FALSE );

	UndoActionStart( TEXTUNDO_READ );
	TextSelection aSel;
	if ( pSel )
		aSel = *pSel;
	else
	{
		ULONG nParas = mpDoc->GetNodes().Count();
		TextNode* pNode = mpDoc->GetNodes().GetObject( nParas - 1 );
		aSel = TextPaM( nParas-1 , pNode->GetText().Len() );
	}

	if ( aSel.HasRange() )
		aSel = ImpDeleteText( aSel );

	ByteString aLine;
	BOOL bDone = rInput.ReadLine( aLine );
	String aTmpStr( aLine, rInput.GetStreamCharSet() ), aStr;
	while ( bDone )
	{
		aSel = ImpInsertText( aSel, aTmpStr );
		bDone = rInput.ReadLine( aLine );
		aTmpStr = String( aLine, rInput.GetStreamCharSet() );
		if ( bDone )
			aSel = ImpInsertParaBreak( aSel.GetEnd() );
	}

	UndoActionEnd( TEXTUNDO_READ );

	TextSelection aNewSel( aSel.GetEnd(), aSel.GetEnd() );

	// Damit bei FormatAndUpdate nicht auf die ungueltige Selektion zugegriffen wird.
	if ( GetActiveView() )
		GetActiveView()->ImpSetSelection( aNewSel );

	SetUpdateMode( bUpdate );
	FormatAndUpdate( GetActiveView() );

	return rInput.GetError() ? FALSE : TRUE;
}

BOOL TextEngine::Write( SvStream& rOutput, const TextSelection* pSel, BOOL bHTML )
{
	TextSelection aSel;
	if ( pSel )
		aSel = *pSel;
	else
	{
		ULONG nParas = mpDoc->GetNodes().Count();
		TextNode* pNode = mpDoc->GetNodes().GetObject( nParas - 1 );
		aSel.GetStart() = TextPaM( 0, 0 );
		aSel.GetEnd() = TextPaM( nParas-1, pNode->GetText().Len() );
	}

	if ( bHTML )
	{
		rOutput.WriteLine( "<HTML>" );
		rOutput.WriteLine( "<BODY>" );
	}

	for ( ULONG nPara = aSel.GetStart().GetPara(); nPara <= aSel.GetEnd().GetPara(); nPara++  )
	{
		TextNode* pNode = mpDoc->GetNodes().GetObject( nPara );

		USHORT nStartPos = 0;
		USHORT nEndPos = pNode->GetText().Len();
		if ( nPara == aSel.GetStart().GetPara() )
			nStartPos = aSel.GetStart().GetIndex();
		if ( nPara == aSel.GetEnd().GetPara() )
			nEndPos = aSel.GetEnd().GetIndex();

		String aText;
		if ( !bHTML )
		{
			aText = pNode->GetText().Copy( nStartPos, nEndPos-nStartPos );
		}
		else
		{
			aText.AssignAscii( RTL_CONSTASCII_STRINGPARAM( "<P STYLE=\"margin-bottom: 0cm\">" ) );

			if ( nStartPos == nEndPos )
			{
				// Leerzeilen werden von Writer wegoptimiert
				aText.AppendAscii( RTL_CONSTASCII_STRINGPARAM( "<BR>" ) );
			}
			else
			{
				USHORT nTmpStart = nStartPos;
				USHORT nTmpEnd = nEndPos;
				do
				{
					TextCharAttrib* pAttr = pNode->GetCharAttribs().FindNextAttrib( TEXTATTR_HYPERLINK, nTmpStart, nEndPos );
					nTmpEnd = pAttr ? pAttr->GetStart() : nEndPos;

					// Text vor dem Attribut
					aText += pNode->GetText().Copy( nTmpStart, nTmpEnd-nTmpStart );

					if ( pAttr )
					{
						nTmpEnd = Min( pAttr->GetEnd(), nEndPos );

						// z.B. <A HREF="http://www.mopo.de/">Morgenpost</A>
						aText.AppendAscii( RTL_CONSTASCII_STRINGPARAM( "<A HREF=\"" ) );
						aText += ((const TextAttribHyperLink&) pAttr->GetAttr() ).GetURL();
						aText.AppendAscii( RTL_CONSTASCII_STRINGPARAM( "\">" ) );
						nTmpStart = pAttr->GetStart();
						aText += pNode->GetText().Copy( nTmpStart, nTmpEnd-nTmpStart );
						aText.AppendAscii( RTL_CONSTASCII_STRINGPARAM( "</A>" ) );

						nTmpStart = pAttr->GetEnd();
					}
				} while ( nTmpEnd < nEndPos );
			}

			aText.AppendAscii( RTL_CONSTASCII_STRINGPARAM( "</P>" ) );
		}
		rOutput.WriteLine( ByteString( aText, rOutput.GetStreamCharSet() ) );
	}

	if ( bHTML )
	{
		rOutput.WriteLine( "</BODY>" );
		rOutput.WriteLine( "</HTML>" );
	}

	return rOutput.GetError() ? FALSE : TRUE;
}

void TextEngine::RemoveAttribs( ULONG nPara, BOOL bIdleFormatAndUpdate )
{
	if ( nPara < mpDoc->GetNodes().Count() )
	{
		TextNode* pNode = mpDoc->GetNodes().GetObject( nPara );
		if ( pNode->GetCharAttribs().Count() )
		{
			pNode->GetCharAttribs().Clear( TRUE );

			TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPara );
			pTEParaPortion->MarkSelectionInvalid( 0, pNode->GetText().Len() );

			mbFormatted = FALSE;

    		if ( bIdleFormatAndUpdate )
    			IdleFormatAndUpdate( NULL, 0xFFFF );
    		else
    			FormatAndUpdate( NULL );
		}
	}
}
void TextEngine::RemoveAttribs( ULONG nPara, USHORT nWhich, BOOL bIdleFormatAndUpdate )
{
    if ( nPara < mpDoc->GetNodes().Count() )
    {
        TextNode* pNode = mpDoc->GetNodes().GetObject( nPara );
        if ( pNode->GetCharAttribs().Count() )
        {
            TextCharAttribList& rAttribs = pNode->GetCharAttribs();
            USHORT nAttrCount = rAttribs.Count();
            for(USHORT nAttr = nAttrCount; nAttr; --nAttr)
            {
                if(rAttribs.GetAttrib( nAttr - 1 )->Which() == nWhich)
                    rAttribs.RemoveAttrib( nAttr -1 );
            }
            TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPara );
            pTEParaPortion->MarkSelectionInvalid( 0, pNode->GetText().Len() );
            mbFormatted = FALSE;
            if(bIdleFormatAndUpdate)
                IdleFormatAndUpdate( NULL, 0xFFFF );
            else
                FormatAndUpdate( NULL );
        }
    }
}        
void TextEngine::RemoveAttrib( ULONG nPara, const TextCharAttrib& rAttrib )
{
    if ( nPara < mpDoc->GetNodes().Count() )
    {
        TextNode* pNode = mpDoc->GetNodes().GetObject( nPara );
        if ( pNode->GetCharAttribs().Count() )
        {
            TextCharAttribList& rAttribs = pNode->GetCharAttribs();
            USHORT nAttrCount = rAttribs.Count();
            for(USHORT nAttr = nAttrCount; nAttr; --nAttr)
            {
                if(rAttribs.GetAttrib( nAttr - 1 ) == &rAttrib)
                {        
                    rAttribs.RemoveAttrib( nAttr -1 );
                    break;
                }
            }
            TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPara );
            pTEParaPortion->MarkSelectionInvalid( 0, pNode->GetText().Len() );
            mbFormatted = FALSE;
            FormatAndUpdate( NULL );
        }
    }
}        

void TextEngine::SetAttrib( const TextAttrib& rAttr, ULONG nPara, USHORT nStart, USHORT nEnd, BOOL bIdleFormatAndUpdate )
{
	// Es wird hier erstmal nicht geprueft, ob sich Attribute ueberlappen!
	// Diese Methode ist erstmal nur fuer einen Editor, der fuer eine Zeile
	// _schnell_ das Syntax-Highlight einstellen will.

	// Da die TextEngine z.Zt fuer Editoren gedacht ist gibt es auch kein
	// Undo fuer Attribute!

	if ( nPara < mpDoc->GetNodes().Count() )
	{
		TextNode* pNode = mpDoc->GetNodes().GetObject( nPara );
		TEParaPortion* pTEParaPortion = mpTEParaPortions->GetObject( nPara );

		USHORT nMax = pNode->GetText().Len();
		if ( nStart > nMax )
			nStart = nMax;
		if ( nEnd > nMax )
			nEnd = nMax;

		pNode->GetCharAttribs().InsertAttrib( new TextCharAttrib( rAttr, nStart, nEnd ) );
		pTEParaPortion->MarkSelectionInvalid( nStart, nEnd );

		mbFormatted = FALSE;
		if ( bIdleFormatAndUpdate )
			IdleFormatAndUpdate( NULL, 0xFFFF );
		else
			FormatAndUpdate( NULL );
	}
}

void TextEngine::SetTextAlign( TxtAlign eAlign )
{
	if ( eAlign != meAlign )
	{
		meAlign = eAlign;
		FormatFullDoc();
		UpdateViews();
	}
}


void TextEngine::ValidateSelection( TextSelection& rSel ) const
{
	ValidatePaM( rSel.GetStart() );
	ValidatePaM( rSel.GetEnd() );
}

void TextEngine::ValidatePaM( TextPaM& rPaM ) const
{
	ULONG nMaxPara = mpDoc->GetNodes().Count() - 1;
	if ( rPaM.GetPara() > nMaxPara )
	{
		rPaM.GetPara() = nMaxPara;
		rPaM.GetIndex() = 0xFFFF;
	}

	USHORT nMaxIndex = GetTextLen( rPaM.GetPara() );
	if ( rPaM.GetIndex() > nMaxIndex )
		rPaM.GetIndex() = nMaxIndex;
}


// Status & Selektionsanpassung

void TextEngine::ImpParagraphInserted( ULONG nPara )
{
	// Die aktive View braucht nicht angepasst werden, aber bei allen
	// passiven muss die Selektion angepasst werden:
	if ( mpViews->Count() > 1 )
	{
		for ( USHORT nView = mpViews->Count(); nView; )
		{
			TextView* pView = mpViews->GetObject( --nView );
			if ( pView != GetActiveView() )
			{
//				BOOL bInvers = pView->maSelection.GetEnd() < pView->maSelection.GetStart();
//				TextPaM& rMin = !bInvers ? pView->maSelection.GetStart(): pView->maSelection.GetEnd();
//				TextPaM& rMax = bInvers ? pView->maSelection.GetStart() : pView->maSelection.GetEnd();
//
//				if ( rMin.GetPara() >= nPara )
//					rMin.GetPara()++;
//				if ( rMax.GetPara() >= nPara )
//					rMax.GetPara()++;
				for ( int n = 0; n <= 1; n++ )
				{
                    TextPaM& rPaM = n ? pView->GetSelection().GetStart(): pView->GetSelection().GetEnd();
					if ( rPaM.GetPara() >= nPara )
						rPaM.GetPara()++;
				}
			}
		}
	}
	Broadcast( TextHint( TEXT_HINT_PARAINSERTED, nPara ) );
}

void TextEngine::ImpParagraphRemoved( ULONG nPara )
{
	if ( mpViews->Count() > 1 )
	{
		for ( USHORT nView = mpViews->Count(); nView; )
		{
			TextView* pView = mpViews->GetObject( --nView );
			if ( pView != GetActiveView() )
			{
				ULONG nParas = mpDoc->GetNodes().Count();
				for ( int n = 0; n <= 1; n++ )
				{
                    TextPaM& rPaM = n ? pView->GetSelection().GetStart(): pView->GetSelection().GetEnd();
					if ( rPaM.GetPara() > nPara )
						rPaM.GetPara()--;
					else if ( rPaM.GetPara() == nPara )
					{
						rPaM.GetIndex() = 0;
						if ( rPaM.GetPara() >= nParas )
							rPaM.GetPara()--;
					}
				}
			}
		}
	}
	Broadcast( TextHint( TEXT_HINT_PARAREMOVED, nPara ) );
}

void TextEngine::ImpCharsRemoved( ULONG nPara, USHORT nPos, USHORT nChars )
{
	if ( mpViews->Count() > 1 )
	{
		for ( USHORT nView = mpViews->Count(); nView; )
		{
			TextView* pView = mpViews->GetObject( --nView );
			if ( pView != GetActiveView() )
			{
				USHORT nEnd = nPos+nChars;
				for ( int n = 0; n <= 1; n++ )
				{
                    TextPaM& rPaM = n ? pView->GetSelection().GetStart(): pView->GetSelection().GetEnd();
					if ( rPaM.GetPara() == nPara )
					{
						if ( rPaM.GetIndex() > nEnd )
							rPaM.GetIndex() = rPaM.GetIndex() - nChars;
						else if ( rPaM.GetIndex() > nPos )
							rPaM.GetIndex() = nPos;
					}
				}
			}
		}
	}
	Broadcast( TextHint( TEXT_HINT_PARACONTENTCHANGED, nPara ) );
}

void TextEngine::ImpCharsInserted( ULONG nPara, USHORT nPos, USHORT nChars )
{
	if ( mpViews->Count() > 1 )
	{
		for ( USHORT nView = mpViews->Count(); nView; )
		{
			TextView* pView = mpViews->GetObject( --nView );
			if ( pView != GetActiveView() )
			{
				for ( int n = 0; n <= 1; n++ )
				{
                    TextPaM& rPaM = n ? pView->GetSelection().GetStart(): pView->GetSelection().GetEnd();
					if ( rPaM.GetPara() == nPara )
					{
						if ( rPaM.GetIndex() >= nPos )
							rPaM.GetIndex() = rPaM.GetIndex() + nChars;
					}
				}
			}
		}
	}
	Broadcast( TextHint( TEXT_HINT_PARACONTENTCHANGED, nPara ) );
}

void TextEngine::ImpFormattingParagraph( ULONG nPara )
{
	Broadcast( TextHint( TEXT_HINT_FORMATPARA, nPara ) );
}

void TextEngine::ImpTextHeightChanged()
{
	Broadcast( TextHint( TEXT_HINT_TEXTHEIGHTCHANGED ) );
}

void TextEngine::ImpTextFormatted()
{
	Broadcast( TextHint( TEXT_HINT_TEXTFORMATTED ) );
}

void TextEngine::Draw( OutputDevice* pDev, const Point& rPos )
{
	ImpPaint( pDev, rPos, NULL );
}

void TextEngine::SetLeftMargin( USHORT n )
{
	mpDoc->SetLeftMargin( n );
}

USHORT TextEngine::GetLeftMargin() const
{
	return mpDoc->GetLeftMargin();
}

uno::Reference< i18n::XBreakIterator > TextEngine::GetBreakIterator()
{
	if ( !mxBreakIterator.is() )
		mxBreakIterator = vcl::unohelper::CreateBreakIterator();
    DBG_ASSERT( mxBreakIterator.is(), "Could not create BreakIterator" );
	return mxBreakIterator;
}

void TextEngine::SetLocale( const ::com::sun::star::lang::Locale& rLocale )
{
    maLocale = rLocale;
    delete mpLocaleDataWrapper;
    mpLocaleDataWrapper = NULL;
}

::com::sun::star::lang::Locale TextEngine::GetLocale()
{
	if ( !maLocale.Language.getLength() )
	{
        maLocale = Application::GetSettings().GetUILocale();
	}
	return maLocale;
}

LocaleDataWrapper* TextEngine::ImpGetLocaleDataWrapper()
{
    if ( !mpLocaleDataWrapper )
        mpLocaleDataWrapper = new LocaleDataWrapper( vcl::unohelper::GetMultiServiceFactory(), GetLocale() );

    return mpLocaleDataWrapper;
}

void TextEngine::SetRightToLeft( BOOL bR2L ) 
{ 
    if ( mbRightToLeft != bR2L )
    {
        mbRightToLeft = bR2L; 
        meAlign = bR2L ? TXTALIGN_RIGHT : TXTALIGN_LEFT;
		FormatFullDoc();
		UpdateViews();
    }
}

void TextEngine::ImpInitWritingDirections( ULONG nPara )
{
	TEParaPortion* pParaPortion = mpTEParaPortions->GetObject( nPara );
	TEWritingDirectionInfos& rInfos = pParaPortion->GetWritingDirectionInfos();
	rInfos.Remove( 0, rInfos.Count() );

	if ( pParaPortion->GetNode()->GetText().Len() )
	{
        const UBiDiLevel nBidiLevel = IsRightToLeft() ? 1 /*RTL*/ : 0 /*LTR*/;
        String aText( pParaPortion->GetNode()->GetText() );

        //
        // Bidi functions from icu 2.0
        //
        UErrorCode nError = U_ZERO_ERROR;
        UBiDi* pBidi = ubidi_openSized( aText.Len(), 0, &nError );
        nError = U_ZERO_ERROR;

        ubidi_setPara( pBidi, reinterpret_cast<const UChar *>(aText.GetBuffer()), aText.Len(), nBidiLevel, NULL, &nError );	// UChar != sal_Unicode in MinGW
        nError = U_ZERO_ERROR;

        long nCount = ubidi_countRuns( pBidi, &nError );

        int32_t nStart = 0;
        int32_t nEnd;
        UBiDiLevel nCurrDir;

        for ( USHORT nIdx = 0; nIdx < nCount; ++nIdx )
        {
            ubidi_getLogicalRun( pBidi, nStart, &nEnd, &nCurrDir );
            rInfos.Insert( TEWritingDirectionInfo( nCurrDir, (USHORT)nStart, (USHORT)nEnd ), rInfos.Count() );
            nStart = nEnd;
        }

        ubidi_close( pBidi );
	}

    // No infos mean no CTL and default dir is L2R...
    if ( !rInfos.Count() )
        rInfos.Insert( TEWritingDirectionInfo( 0, 0, (USHORT)pParaPortion->GetNode()->GetText().Len() ), rInfos.Count() );

}

BYTE TextEngine::ImpGetRightToLeft( ULONG nPara, USHORT nPos, USHORT* pStart, USHORT* pEnd )
{
    BYTE nRightToLeft = 0;

    TextNode* pNode = mpDoc->GetNodes().GetObject( nPara );
    if ( pNode && pNode->GetText().Len() )
    {
		TEParaPortion* pParaPortion = mpTEParaPortions->GetObject( nPara );
		if ( !pParaPortion->GetWritingDirectionInfos().Count() )
			ImpInitWritingDirections( nPara );

		TEWritingDirectionInfos& rDirInfos = pParaPortion->GetWritingDirectionInfos();
		for ( USHORT n = 0; n < rDirInfos.Count(); n++ ) 
		{
			if ( ( rDirInfos[n].nStartPos <= nPos ) && ( rDirInfos[n].nEndPos >= nPos ) )
	   		{
				nRightToLeft = rDirInfos[n].nType;
                if ( pStart )
                    *pStart = rDirInfos[n].nStartPos;
                if ( pEnd )
                    *pEnd = rDirInfos[n].nEndPos;
				break;
			}
		}
    }
    return nRightToLeft;
}

long TextEngine::ImpGetPortionXOffset( ULONG nPara, TextLine* pLine, USHORT nTextPortion )
{
	long nX = pLine->GetStartX();

	TEParaPortion* pParaPortion = mpTEParaPortions->GetObject( nPara );

    for ( USHORT i = pLine->GetStartPortion(); i < nTextPortion; i++ )
	{
		TETextPortion* pPortion = pParaPortion->GetTextPortions().GetObject( i );
		nX += pPortion->GetWidth();
    }

    TETextPortion* pDestPortion = pParaPortion->GetTextPortions().GetObject( nTextPortion );
    if ( pDestPortion->GetKind() != PORTIONKIND_TAB )
    {
        if ( !IsRightToLeft() && pDestPortion->GetRightToLeft() )
        {
            // Portions behind must be added, visual before this portion
            sal_uInt16 nTmpPortion = nTextPortion+1;
            while ( nTmpPortion <= pLine->GetEndPortion() )
            {
		        TETextPortion* pNextTextPortion = pParaPortion->GetTextPortions().GetObject( nTmpPortion );
                if ( pNextTextPortion->GetRightToLeft() && ( pNextTextPortion->GetKind() != PORTIONKIND_TAB ) )
                    nX += pNextTextPortion->GetWidth();
                else
                    break;
                nTmpPortion++;
            }
            // Portions before must be removed, visual behind this portion
            nTmpPortion = nTextPortion;
            while ( nTmpPortion > pLine->GetStartPortion() )
            {
                --nTmpPortion;
		        TETextPortion* pPrevTextPortion = pParaPortion->GetTextPortions().GetObject( nTmpPortion );
                if ( pPrevTextPortion->GetRightToLeft() && ( pPrevTextPortion->GetKind() != PORTIONKIND_TAB ) )
                    nX -= pPrevTextPortion->GetWidth();
                else
                    break;
            }
        }
        else if ( IsRightToLeft() && !pDestPortion->IsRightToLeft() )
        {
            // Portions behind must be removed, visual behind this portion
            sal_uInt16 nTmpPortion = nTextPortion+1;
            while ( nTmpPortion <= pLine->GetEndPortion() )
            {
		        TETextPortion* pNextTextPortion = pParaPortion->GetTextPortions().GetObject( nTmpPortion );
                if ( !pNextTextPortion->IsRightToLeft() && ( pNextTextPortion->GetKind() != PORTIONKIND_TAB ) )
                    nX += pNextTextPortion->GetWidth();
                else
                    break;
                nTmpPortion++;
            }
            // Portions before must be added, visual before this portion
            nTmpPortion = nTextPortion;
            while ( nTmpPortion > pLine->GetStartPortion() )
            {
                --nTmpPortion;
		        TETextPortion* pPrevTextPortion = pParaPortion->GetTextPortions().GetObject( nTmpPortion );
                if ( !pPrevTextPortion->IsRightToLeft() && ( pPrevTextPortion->GetKind() != PORTIONKIND_TAB ) )
                    nX -= pPrevTextPortion->GetWidth();
                else
                    break;
            }
        }
    }
/*
    if ( IsRightToLeft() )
    {
        // Switch X postions...
        DBG_ASSERT( GetMaxTextWidth(), "GetPortionXOffset - max text width?!" );
        DBG_ASSERT( nX <= (long)GetMaxTextWidth(), "GetPortionXOffset - position out of paper size!" );
        nX = GetMaxTextWidth() - nX;
        nX -= pDestPortion->GetWidth();
    }
*/

    return nX;
}

void TextEngine::ImpInitLayoutMode( OutputDevice* pOutDev, BOOL bDrawingR2LPortion )
{
    ULONG nLayoutMode = pOutDev->GetLayoutMode();
    
    nLayoutMode &= ~(TEXT_LAYOUT_BIDI_RTL | TEXT_LAYOUT_COMPLEX_DISABLED | TEXT_LAYOUT_BIDI_STRONG );
    if ( bDrawingR2LPortion )
        nLayoutMode |= TEXT_LAYOUT_BIDI_RTL;
    
    pOutDev->SetLayoutMode( nLayoutMode );
}

TxtAlign TextEngine::ImpGetAlign() const
{
    TxtAlign eAlign = meAlign;
    if ( IsRightToLeft() )
    {
        if ( eAlign == TXTALIGN_LEFT )
            eAlign = TXTALIGN_RIGHT;
        else if ( eAlign == TXTALIGN_RIGHT )
            eAlign = TXTALIGN_LEFT;
    }
    return eAlign;
}

long TextEngine::ImpGetOutputOffset( ULONG nPara, TextLine* pLine, USHORT nIndex, USHORT nIndex2 )
{
    TEParaPortion* pPortion = mpTEParaPortions->GetObject( nPara );

    USHORT nPortionStart;
    USHORT nPortion = pPortion->GetTextPortions().FindPortion( nIndex, nPortionStart, TRUE );

    TETextPortion* pTextPortion = pPortion->GetTextPortions().GetObject( nPortion );

    long nX;

    if ( ( nIndex == nPortionStart ) && ( nIndex == nIndex2 )  )
    {
        // Output of full portion, so we need portion x offset.
        // Use ImpGetPortionXOffset, because GetXPos may deliver left or right position from portioon, depending on R2L, L2R
        nX = ImpGetPortionXOffset( nPara, pLine, nPortion );
        if ( IsRightToLeft() )
        {
	        nX = -nX -pTextPortion->GetWidth();
        }
    }
    else
    {
        nX = ImpGetXPos( nPara, pLine, nIndex, nIndex == nPortionStart );
        if ( nIndex2 != nIndex )
        {
            long nX2 = ImpGetXPos( nPara, pLine, nIndex2, FALSE );
            if ( ( !IsRightToLeft() && ( nX2 < nX ) ) ||
                 ( IsRightToLeft() && ( nX2 > nX ) ) )
            {
                nX = nX2;
            }
        }
        if ( IsRightToLeft() )
        {
	        nX = -nX;
        }
    }

    return nX;
}
