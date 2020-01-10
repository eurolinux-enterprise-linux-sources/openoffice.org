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

#include <svtools/svmedit.hxx>
#include <svtools/xtextedt.hxx>
#include <svtools/editsyntaxhighlighter.hxx>
#include "../../inc/txtattr.hxx"


MultiLineEditSyntaxHighlight::MultiLineEditSyntaxHighlight( Window* pParent, WinBits nWinStyle, 
	HighlighterLanguage aLanguage): MultiLineEdit(pParent,nWinStyle), mbDoBracketHilight(true)
{
	EnableUpdateData(300);
	aHighlighter.initialize( aLanguage );
}

MultiLineEditSyntaxHighlight::MultiLineEditSyntaxHighlight( Window* pParent, const ResId& rResId , 
	HighlighterLanguage aLanguage): MultiLineEdit(pParent,rResId), mbDoBracketHilight(true)
{
	EnableUpdateData(300);
	aHighlighter.initialize( aLanguage );
}

MultiLineEditSyntaxHighlight::~MultiLineEditSyntaxHighlight()
{
}

void MultiLineEditSyntaxHighlight::EnableBracketHilight(bool aHilight)
{
	mbDoBracketHilight = aHilight;
}

bool MultiLineEditSyntaxHighlight::IsBracketHilight()
{
	return mbDoBracketHilight;
}
		
void MultiLineEditSyntaxHighlight::SetText(const String& rNewText)
{
	MultiLineEdit::SetText(rNewText);
	UpdateData();
}

void MultiLineEditSyntaxHighlight::DoBracketHilight(USHORT aKey)
{
	TextSelection aCurrentPos = GetTextView()->GetSelection();
	xub_StrLen aStartPos  = aCurrentPos.GetStart().GetIndex();
	ULONG nStartPara = aCurrentPos.GetStart().GetPara();
	USHORT aCount = 0;
	int aChar = -1;

	switch (aKey)
	{
		case '\'':	// no break
		case '"': 
		{
			aChar = aKey;
			break;
		}
		case '}' :
		{
			aChar = '{';
			break;
		}
		case ')':
		{
			aChar = '(';
			break;
		}
		case ']':
		{
			aChar = '[';
			break;
		}
	}

	if (aChar != -1)
	{
		for (long aPara =nStartPara; aPara>=0;--aPara)
		{
            if ( aStartPos == 0 )
                continue;

			String aLine( GetTextEngine()->GetText( aPara ) );
			for (USHORT i = ((unsigned long)aPara==nStartPara) ? aStartPos-1 : (USHORT)(aLine.Len()-1); i>0; --i)
			{	
				if (aLine.GetChar(i)==aChar)
				{
					if (!aCount)
					{
						GetTextEngine()->SetAttrib( TextAttribFontWeight( WEIGHT_ULTRABOLD ), aPara, i, i+1, TRUE );
						GetTextEngine()->SetAttrib( TextAttribFontColor( Color(0,0,0) ), aPara, i, i+1, TRUE );
						GetTextEngine()->SetAttrib( TextAttribFontWeight( WEIGHT_ULTRABOLD ), nStartPara, aStartPos, aStartPos, TRUE );
						GetTextEngine()->SetAttrib( TextAttribFontColor( Color(0,0,0) ), nStartPara, aStartPos, aStartPos, TRUE );
						return;
					}
					else
						aCount--;
				}
				if (aLine.GetChar(i)==aKey)
					aCount++;
			}
		}
	}
}

long MultiLineEditSyntaxHighlight::PreNotify( NotifyEvent& rNEvt )
{
	if ( mbDoBracketHilight && (rNEvt.GetType() == EVENT_KEYINPUT) )
		DoBracketHilight(rNEvt.GetKeyEvent()->GetCharCode());

	return MultiLineEdit::PreNotify(rNEvt);
}

Color MultiLineEditSyntaxHighlight::GetColorValue(TokenTypes aToken)
{
	Color aColor;
	switch (aHighlighter.GetLanguage())
	{
		case HIGHLIGHT_SQL:
		{
			switch (aToken)
			{
				case TT_IDENTIFIER:	aColor = (ColorData)m_aColorConfig.GetColorValue(svtools::SQLIDENTIFIER).nColor; break;
				case TT_NUMBER:		aColor = (ColorData)m_aColorConfig.GetColorValue(svtools::SQLNUMBER).nColor; break;
				case TT_STRING:		aColor = (ColorData)m_aColorConfig.GetColorValue(svtools::SQLSTRING).nColor; break;
				case TT_OPERATOR:	aColor = (ColorData)m_aColorConfig.GetColorValue(svtools::SQLOPERATOR).nColor; break;
				case TT_KEYWORDS:	aColor = (ColorData)m_aColorConfig.GetColorValue(svtools::SQLKEYWORD).nColor; break;
				case TT_PARAMETER:  aColor = (ColorData)m_aColorConfig.GetColorValue(svtools::SQLPARAMETER).nColor; break;
				case TT_COMMENT:	aColor = (ColorData)m_aColorConfig.GetColorValue(svtools::SQLCOMMENT).nColor; break;
				default:			aColor = Color(0,0,0);
			}
			break;
		}
		case HIGHLIGHT_BASIC:
		{
			switch (aToken)
			{
				case TT_IDENTIFIER:	aColor = Color(255,0,0); break;
				case TT_COMMENT:	aColor = Color(0,0,45); break;
				case TT_NUMBER:		aColor = Color(204,102,204); break;
				case TT_STRING:		aColor = Color(0,255,45); break;
				case TT_OPERATOR:	aColor = Color(0,0,100); break;
				case TT_KEYWORDS:	aColor = Color(0,0,255); break;
				case TT_ERROR :		aColor = Color(0,255,255); break;
				default:			aColor = Color(0,0,0);
			}
			break;
		}
		default: aColor = Color(0,0,0);

	}
	return aColor;
}

void MultiLineEditSyntaxHighlight::UpdateData()
{
	// syntax highlighting
	// this must be possible improved by using notifychange correctly
	BOOL bTempModified = GetTextEngine()->IsModified();
	for (unsigned int nLine=0; nLine < GetTextEngine()->GetParagraphCount(); nLine++)
	{
		String aLine( GetTextEngine()->GetText( nLine ) );
		Range aChanges = aHighlighter.notifyChange( nLine, 0, &aLine, 1 );
			
		GetTextEngine()->RemoveAttribs( nLine, TRUE );
		HighlightPortions aPortions;
		aHighlighter.getHighlightPortions( nLine, aLine, aPortions );
		for ( USHORT i = 0; i < aPortions.Count(); i++ )
		{
			HighlightPortion& r = aPortions[i];
			GetTextEngine()->SetAttrib( TextAttribFontColor( GetColorValue(r.tokenType) ), nLine, r.nBegin, r.nEnd, TRUE );
		}
	}
	GetTextView()->ShowCursor( false, true );
	GetTextEngine()->SetModified(bTempModified);
}
