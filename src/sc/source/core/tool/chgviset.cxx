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
#include "precompiled_sc.hxx"



#include <unotools/textsearch.hxx>

#include "chgviset.hxx"
#include "rechead.hxx"
#include "chgtrack.hxx"

// -----------------------------------------------------------------------
ScChangeViewSettings::~ScChangeViewSettings()
{
	if(pCommentSearcher!=NULL)
		delete pCommentSearcher;
}

ScChangeViewSettings::ScChangeViewSettings( const ScChangeViewSettings& r )
{
	SetTheComment(r.aComment);

	aFirstDateTime	=r.aFirstDateTime;
	aLastDateTime	=r.aLastDateTime;
	aAuthorToShow	=r.aAuthorToShow;
	aRangeList		=r.aRangeList;
	eDateMode		=r.eDateMode;
	bShowIt			=r.bShowIt;
	bIsDate			=r.bIsDate;
	bIsAuthor		=r.bIsAuthor;
	bIsComment		=r.bIsComment;
	bIsRange		=r.bIsRange;
	bEveryoneButMe	=r.bEveryoneButMe;
	bShowAccepted	=r.bShowAccepted;
	bShowRejected	=r.bShowRejected;
    mbIsActionRange = r.mbIsActionRange;
    mnFirstAction   = r.mnFirstAction;
    mnLastAction    = r.mnLastAction;

}

ScChangeViewSettings& ScChangeViewSettings::operator=( const ScChangeViewSettings& r )
{
	SetTheComment(r.aComment);

	aFirstDateTime	=r.aFirstDateTime;
	aLastDateTime	=r.aLastDateTime;
	aAuthorToShow	=r.aAuthorToShow;
	aRangeList		=r.aRangeList;
	eDateMode		=r.eDateMode;
	bShowIt			=r.bShowIt;
	bIsDate			=r.bIsDate;
	bIsAuthor		=r.bIsAuthor;
	bIsComment		=r.bIsComment;
	bIsRange		=r.bIsRange;
	bEveryoneButMe	=r.bEveryoneButMe;
	bShowAccepted	=r.bShowAccepted;
	bShowRejected	=r.bShowRejected;
    mbIsActionRange = r.mbIsActionRange;
    mnFirstAction   = r.mnFirstAction;
    mnLastAction    = r.mnLastAction;

	return *this;
}

BOOL ScChangeViewSettings::IsValidComment(const String* pCommentStr) const
{
	BOOL nTheFlag=TRUE;

	if(pCommentSearcher!=NULL)
	{
		xub_StrLen nStartPos = 0;
		xub_StrLen nEndPos = pCommentStr->Len();

        nTheFlag=sal::static_int_cast<BOOL>(pCommentSearcher->SearchFrwrd( *pCommentStr, &nStartPos, &nEndPos));
	}
	return nTheFlag;
}

void ScChangeViewSettings::SetTheComment(const String& rString)
{
	aComment=rString;
	if(pCommentSearcher!=NULL)
	{
		delete pCommentSearcher;
		pCommentSearcher=NULL;
	}

	if(rString.Len()>0)
	{
		utl::SearchParam aSearchParam( rString,
			utl::SearchParam::SRCH_REGEXP,FALSE,FALSE,FALSE );

		pCommentSearcher = new utl::TextSearch( aSearchParam, *ScGlobal::pCharClass );
	}
}

void ScChangeViewSettings::AdjustDateMode( const ScDocument& rDoc )
{
    switch ( eDateMode )
    {	// corresponds with ScViewUtil::IsActionShown
        case SCDM_DATE_EQUAL :
        case SCDM_DATE_NOTEQUAL :
            aFirstDateTime.SetTime( 0 );
            aLastDateTime = aFirstDateTime;
            aLastDateTime.SetTime( 23595999 );
            break;
        case SCDM_DATE_SAVE:
        {
            const ScChangeAction* pLast = 0;
            ScChangeTrack* pTrack = rDoc.GetChangeTrack();
            if ( pTrack )
            {
                pLast = pTrack->GetLastSaved();
                if ( pLast )
                {
                    aFirstDateTime = pLast->GetDateTime();
#if 0
// This would be the proper handling. But since the SvxTPFilter dialog uses
// DateField/TimeField, and the filter dialog is used in ScAcceptChgDlg as the
// controlling instance, and the TimeFields are used there without seconds or
// 100ths, we'd display some extra entries between the floor of the minute and
// the start of the next minute.
                    // add one 100th second to point past last saved
                    aFirstDateTime += Time( 0, 0, 0, 1 );
#else
                    // Set the next minute as the start time and assume that
                    // the document isn't saved, reloaded, edited and filter set
                    // all together during the gap between those two times.
                    aFirstDateTime += Time( 0, 1 );
                    aFirstDateTime.SetSec(0);
                    aFirstDateTime.Set100Sec(0);
#endif
                }
            }
            if ( !pLast )
            {
                aFirstDateTime.SetDate( 18990101 );
                aFirstDateTime.SetTime( 0 );
            }
            aLastDateTime = Date();
            aLastDateTime.SetYear( aLastDateTime.GetYear() + 100 );
        }
        break;
        default:
        {
            // added to avoid warnings
        }
    }
}

