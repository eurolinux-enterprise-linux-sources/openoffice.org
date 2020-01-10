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

#undef SC_DLLIMPLEMENTATION



//------------------------------------------------------------------

#include "scitems.hxx"
#include <svx/algitem.hxx>
#include <svx/boxitem.hxx>
#include <svx/brshitem.hxx>
#include <svx/cntritem.hxx>
#include <svx/colritem.hxx>
#include <svx/crsditem.hxx>
#include <svx/fontitem.hxx>
#include <svx/postitem.hxx>
#include <svx/shdditem.hxx>
#include <svx/udlnitem.hxx>
#include <svx/wghtitem.hxx>
#include <svtools/zforlist.hxx>
#include <vcl/msgbox.hxx>
#include <comphelper/processfactory.hxx>

#include "sc.hrc"
#include "scmod.hxx"
#include "attrib.hxx"
#include "zforauto.hxx"
#include "scitems.hxx"
#include "global.hxx"
#include "globstr.hrc"
#include "autoform.hxx"
#include "strindlg.hxx"
#include "miscdlgs.hrc"
#include "scuiautofmt.hxx"
#include "scresid.hxx"
#include "document.hxx"

//========================================================================
// AutoFormat-Dialog:

ScAutoFormatDlg::ScAutoFormatDlg( Window*					pParent,
								  ScAutoFormat* 			pAutoFormat,
                                  const ScAutoFormatData*   pSelFormatData,
                                  ScDocument*               pDoc ) :

	ModalDialog 	( pParent, ScResId( RID_SCDLG_AUTOFORMAT ) ),
	//
    aFlFormat       ( this, ScResId( FL_FORMAT ) ),
	aLbFormat		( this, ScResId( LB_FORMAT ) ),
    pWndPreview     ( new ScAutoFmtPreview( this, ScResId( WND_PREVIEW ), pDoc ) ),
	aBtnOk			( this, ScResId( BTN_OK ) ),
	aBtnCancel		( this, ScResId( BTN_CANCEL ) ),
	aBtnHelp		( this, ScResId( BTN_HELP ) ),
	aBtnAdd 		( this, ScResId( BTN_ADD ) ),
	aBtnRemove		( this, ScResId( BTN_REMOVE ) ),
	aBtnMore		( this, ScResId( BTN_MORE ) ),
    aFlFormatting   ( this, ScResId( FL_FORMATTING ) ),
	aBtnNumFormat	( this, ScResId( BTN_NUMFORMAT ) ),
	aBtnBorder		( this, ScResId( BTN_BORDER ) ),
	aBtnFont		( this, ScResId( BTN_FONT ) ),
	aBtnPattern 	( this, ScResId( BTN_PATTERN ) ),
	aBtnAlignment	( this, ScResId( BTN_ALIGNMENT ) ),
	aBtnAdjust		( this, ScResId( BTN_ADJUST ) ),
	aBtnRename		( this, ScResId( BTN_RENAME ) ),
    aStrTitle       ( ScResId( STR_ADD_TITLE ) ),
    aStrLabel       ( ScResId( STR_ADD_LABEL ) ),
    aStrClose       ( ScResId( STR_BTN_CLOSE ) ),
    aStrDelTitle    ( ScResId( STR_DEL_TITLE ) ),
    aStrDelMsg      ( ScResId( STR_DEL_MSG ) ) ,
    aStrRename      ( ScResId( STR_RENAME_TITLE ) ),
	//
	pFormat 		( pAutoFormat ),
	pSelFmtData 	( pSelFormatData ),
	nIndex			( 0 ),
	bCoreDataChanged( FALSE ),
	bFmtInserted	( FALSE )
{
	Init();
	pWndPreview->NotifyChange( (*pFormat)[0] );
	FreeResource();
}

//------------------------------------------------------------------------

__EXPORT ScAutoFormatDlg::~ScAutoFormatDlg()
{
	delete pWndPreview;
}

//------------------------------------------------------------------------

void ScAutoFormatDlg::Init()
{
	USHORT nCount;
	String aEntry;

	aLbFormat	 .SetSelectHdl( LINK( this, ScAutoFormatDlg, SelFmtHdl ) );
	aBtnNumFormat.SetClickHdl ( LINK( this, ScAutoFormatDlg, CheckHdl ) );
	aBtnBorder	 .SetClickHdl ( LINK( this, ScAutoFormatDlg, CheckHdl ) );
	aBtnFont	 .SetClickHdl ( LINK( this, ScAutoFormatDlg, CheckHdl ) );
	aBtnPattern  .SetClickHdl ( LINK( this, ScAutoFormatDlg, CheckHdl ) );
	aBtnAlignment.SetClickHdl ( LINK( this, ScAutoFormatDlg, CheckHdl ) );
	aBtnAdjust	 .SetClickHdl ( LINK( this, ScAutoFormatDlg, CheckHdl ) );
	aBtnAdd 	 .SetClickHdl ( LINK( this, ScAutoFormatDlg, AddHdl ) );
	aBtnRemove	 .SetClickHdl ( LINK( this, ScAutoFormatDlg, RemoveHdl ) );
	aBtnOk		 .SetClickHdl ( LINK( this, ScAutoFormatDlg, CloseHdl ) );
	aBtnCancel	 .SetClickHdl ( LINK( this, ScAutoFormatDlg, CloseHdl ) );
	aBtnRename	 .SetClickHdl ( LINK( this, ScAutoFormatDlg, RenameHdl ) );
	aLbFormat	 .SetDoubleClickHdl( LINK( this, ScAutoFormatDlg, DblClkHdl ) );

	aBtnMore.AddWindow( &aBtnRename );
	aBtnMore.AddWindow( &aBtnNumFormat );
	aBtnMore.AddWindow( &aBtnBorder );
	aBtnMore.AddWindow( &aBtnFont );
	aBtnMore.AddWindow( &aBtnPattern );
	aBtnMore.AddWindow( &aBtnAlignment );
	aBtnMore.AddWindow( &aBtnAdjust );
    aBtnMore.AddWindow( &aFlFormatting );

	nCount = pFormat->GetCount();

	for ( USHORT i = 0; i < nCount; i++ )
	{
		((*pFormat)[i])->GetName( aEntry );
		aLbFormat.InsertEntry( aEntry );
	}

	if ( nCount == 1 )
		aBtnRemove.Disable();

	aLbFormat.SelectEntryPos( 0 );
	aBtnRename.Disable();
	aBtnRemove.Disable();

	nIndex = 0;
	UpdateChecks();

	if ( !pSelFmtData )
	{
		aBtnAdd.Disable();
		aBtnRemove.Disable();
		bFmtInserted = TRUE;
	}
}

//------------------------------------------------------------------------

void ScAutoFormatDlg::UpdateChecks()
{
	ScAutoFormatData* pData = (*pFormat)[nIndex];

	aBtnNumFormat.Check( pData->GetIncludeValueFormat() );
	aBtnBorder	 .Check( pData->GetIncludeFrame() );
	aBtnFont	 .Check( pData->GetIncludeFont() );
	aBtnPattern  .Check( pData->GetIncludeBackground() );
	aBtnAlignment.Check( pData->GetIncludeJustify() );
	aBtnAdjust	 .Check( pData->GetIncludeWidthHeight() );
}

//------------------------------------------------------------------------
// Handler:
//---------

IMPL_LINK( ScAutoFormatDlg, CloseHdl, PushButton *, pBtn )
{
	if ( pBtn == &aBtnOk || pBtn == &aBtnCancel )
	{
		if ( bCoreDataChanged )
			ScGlobal::GetAutoFormat()->Save();

		EndDialog( (pBtn == &aBtnOk) ? RET_OK : RET_CANCEL );
	}
	return 0;
}

//------------------------------------------------------------------------

IMPL_LINK_INLINE_START( ScAutoFormatDlg, DblClkHdl, void *, EMPTYARG )
{
	if ( bCoreDataChanged )
		ScGlobal::GetAutoFormat()->Save();

	EndDialog( RET_OK );
	return 0;
}
IMPL_LINK_INLINE_END( ScAutoFormatDlg, DblClkHdl, void *, EMPTYARG )

//------------------------------------------------------------------------

IMPL_LINK( ScAutoFormatDlg, CheckHdl, Button *, pBtn )
{
	ScAutoFormatData* pData  = (*pFormat)[nIndex];
	BOOL			  bCheck = ((CheckBox*)pBtn)->IsChecked();

	if ( pBtn == &aBtnNumFormat )
		pData->SetIncludeValueFormat( bCheck );
	else if ( pBtn == &aBtnBorder )
		pData->SetIncludeFrame( bCheck );
	else if ( pBtn == &aBtnFont )
		pData->SetIncludeFont( bCheck );
	else if ( pBtn == &aBtnPattern )
		pData->SetIncludeBackground( bCheck );
	else if ( pBtn == &aBtnAlignment )
		pData->SetIncludeJustify( bCheck );
	else if ( pBtn == &aBtnAdjust )
		pData->SetIncludeWidthHeight( bCheck );

	if ( !bCoreDataChanged )
	{
		aBtnCancel.SetText( aStrClose );
		bCoreDataChanged = TRUE;
	}

	pWndPreview->NotifyChange( pData );

	return 0;
}

//------------------------------------------------------------------------

IMPL_LINK( ScAutoFormatDlg, AddHdl, void *, EMPTYARG )
{
	if ( !bFmtInserted && pSelFmtData )
	{
		String				aStrStandard( ScResId(STR_STANDARD) );
		String				aFormatName;
		ScStringInputDlg*	pDlg;
		BOOL				bOk = FALSE;

		while ( !bOk )
		{
			pDlg = new ScStringInputDlg( this,
										 aStrTitle,
										 aStrLabel,
										 aFormatName,
										 HID_SC_ADD_AUTOFMT );

			if ( pDlg->Execute() == RET_OK )
			{
				pDlg->GetInputString( aFormatName );

				if ( (aFormatName.Len() > 0) && (aFormatName != aStrStandard) )
				{
					ScAutoFormatData* pNewData
						= new ScAutoFormatData( *pSelFmtData );

					pNewData->SetName( aFormatName );
					bFmtInserted = pFormat->Insert( pNewData );

					if ( bFmtInserted )
					{
						USHORT nAt = pFormat->IndexOf( pNewData );

						aLbFormat.InsertEntry( aFormatName, nAt );
						aLbFormat.SelectEntry( aFormatName );
						aBtnAdd.Disable();

						if ( !bCoreDataChanged )
						{
							aBtnCancel.SetText( aStrClose );
							bCoreDataChanged = TRUE;
						}

						SelFmtHdl( 0 );
						bOk = TRUE;
					}
					else
						delete pNewData;

				}

				if ( !bFmtInserted )
				{
					USHORT nRet = ErrorBox( this,
											WinBits( WB_OK_CANCEL | WB_DEF_OK),
											ScGlobal::GetRscString(STR_INVALID_AFNAME)
										  ).Execute();

					bOk = ( nRet == RET_CANCEL );
				}
			}
			else
				bOk = TRUE;

			delete pDlg;
		}
	}

	return 0;
}

//------------------------------------------------------------------------

IMPL_LINK( ScAutoFormatDlg, RemoveHdl, void *, EMPTYARG )
{
	if ( (nIndex > 0) && (aLbFormat.GetEntryCount() > 0) )
	{
		String aMsg( aStrDelMsg.GetToken( 0, '#' ) );

		aMsg += aLbFormat.GetSelectEntry();
		aMsg += aStrDelMsg.GetToken( 1, '#' );

		if ( RET_YES ==
			 QueryBox( this, WinBits( WB_YES_NO | WB_DEF_YES ), aMsg ).Execute() )
		{
			aLbFormat.RemoveEntry( nIndex );
			aLbFormat.SelectEntryPos( nIndex-1 );

			if ( nIndex-1 == 0 )
				aBtnRemove.Disable();

			if ( !bCoreDataChanged )
			{
				aBtnCancel.SetText( aStrClose );
				bCoreDataChanged = TRUE;
			}

			pFormat->AtFree( nIndex ); // in der Core loeschen
			nIndex--;

			SelFmtHdl( 0 );
		}
	}

	SelFmtHdl( 0 );

	return 0;
}

IMPL_LINK( ScAutoFormatDlg, RenameHdl, void *, EMPTYARG )
{
	BOOL bOk = FALSE;
	while( !bOk )
	{

		String aFormatName=aLbFormat.GetSelectEntry();
		String aEntry;

		ScStringInputDlg* pDlg = new ScStringInputDlg( this,
										 aStrRename,
										 aStrLabel,
										 aFormatName,
										 HID_SC_RENAME_AUTOFMT );
		if( pDlg->Execute() == RET_OK )
		{
			BOOL bFmtRenamed = FALSE;
			pDlg->GetInputString( aFormatName );
			USHORT n;

			if ( aFormatName.Len() > 0 )
			{
				for( n = 0; n < pFormat->GetCount(); ++n )
				{
					(*pFormat)[n]->GetName(aEntry);
					if ( aEntry== aFormatName)
						break;
				}
				if( n >= pFormat->GetCount() )
				{
					// Format mit dem Namen noch nicht vorhanden, also
					// umbenennen

					aLbFormat.RemoveEntry(nIndex );
					ScAutoFormatData* p=(*pFormat)[ nIndex ];
					ScAutoFormatData* pNewData
						= new ScAutoFormatData(*p);

					pFormat->AtFree( nIndex );

					pNewData->SetName( aFormatName );

					pFormat->Insert( pNewData);

					USHORT nCount = pFormat->GetCount();

					aLbFormat.SetUpdateMode(FALSE);
					aLbFormat.Clear();
					for ( USHORT i = 0; i < nCount; i++ )
					{
						((*pFormat)[i])->GetName( aEntry );
						aLbFormat.InsertEntry( aEntry );
					}

					aLbFormat.SetUpdateMode( TRUE);
					aLbFormat.SelectEntry( aFormatName);

					if ( !bCoreDataChanged )
					{
						aBtnCancel.SetText( aStrClose );
						bCoreDataChanged = TRUE;
					}


					SelFmtHdl( 0 );
					bOk = TRUE;
					bFmtRenamed = TRUE;
				}
			}
			if( !bFmtRenamed )
			{
				bOk = RET_CANCEL == ErrorBox( this,
									WinBits( WB_OK_CANCEL | WB_DEF_OK),
									ScGlobal::GetRscString(STR_INVALID_AFNAME)
									).Execute();
			}
		}
		else
			bOk = TRUE;
		delete pDlg;
	}

	return 0;
}

//------------------------------------------------------------------------

IMPL_LINK( ScAutoFormatDlg, SelFmtHdl, void *, EMPTYARG )
{
	nIndex = aLbFormat.GetSelectEntryPos();
	UpdateChecks();

	if ( nIndex == 0 )
	{
		aBtnRename.Disable();
		aBtnRemove.Disable();
	}
	else
	{
		aBtnRename.Enable();
		aBtnRemove.Enable();
	}

	pWndPreview->NotifyChange( (*pFormat)[nIndex] );

	return 0;
}

//------------------------------------------------------------------------

String __EXPORT ScAutoFormatDlg::GetCurrFormatName()
{
	String	aResult;

	((*pFormat)[nIndex])->GetName( aResult );

	return aResult;
}
