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
#include "precompiled_svx.hxx"

#ifdef SVX_DLLIMPLEMENTATION
#undef SVX_DLLIMPLEMENTATION
#endif

// include ---------------------------------------------------------------
#include <tools/debug.hxx>
#include <tools/urlobj.hxx>
#include <vcl/msgbox.hxx>
#include <sfx2/filedlghelper.hxx>

#include "multipat.hxx"
#include <svx/dialmgr.hxx>

#include "multipat.hrc"
#include <svx/dialogs.hrc>
#include <comphelper/processfactory.hxx>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/ui/dialogs/XFolderPicker.hpp>
#include <com/sun/star/ui/dialogs/ExecutableDialogResults.hpp>

#include <unotools/localfilehelper.hxx>
#include <svtools/pathoptions.hxx>

using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::ui::dialogs;
using namespace ::com::sun::star::uno;

// struct MultiPath_Impl -------------------------------------------------

struct MultiPath_Impl
{
	BOOL	bEmptyAllowed;
	BOOL	bIsClassPathMode;
    bool    bIsRadioButtonMode;

    MultiPath_Impl( BOOL bAllowed ) :
        bEmptyAllowed( bAllowed ), bIsClassPathMode( FALSE ), bIsRadioButtonMode( false )  {}
};

// class SvxMultiPathDialog ----------------------------------------------

IMPL_LINK( SvxMultiPathDialog, SelectHdl_Impl, void *, EMPTYARG )
{
    ULONG nCount = pImpl->bIsRadioButtonMode ? aRadioLB.GetEntryCount() : aPathLB.GetEntryCount();
    bool bIsSelected = pImpl->bIsRadioButtonMode
        ? aRadioLB.FirstSelected() != NULL
        : aPathLB.GetSelectEntryPos() != LISTBOX_ENTRY_NOTFOUND;
    BOOL bEnable = ( pImpl->bEmptyAllowed || nCount > 1 );
    aDelBtn.Enable( bEnable && bIsSelected );
    return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxMultiPathDialog, CheckHdl_Impl, svx::SvxRadioButtonListBox *, pBox )
{
    SvLBoxEntry* pEntry =
        pBox ? pBox->GetEntry( pBox->GetCurMousePoint() ) : aRadioLB.FirstSelected();
    if ( pEntry )
        aRadioLB.HandleEntryChecked( pEntry );
    return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxMultiPathDialog, AddHdl_Impl, PushButton *, EMPTYARG )
{
    rtl::OUString aService( RTL_CONSTASCII_USTRINGPARAM( FOLDER_PICKER_SERVICE_NAME ) );
    Reference < XMultiServiceFactory > xFactory( ::comphelper::getProcessServiceFactory() );
	Reference < XFolderPicker > xFolderPicker( xFactory->createInstance( aService ), UNO_QUERY );

	if ( xFolderPicker->execute() == ExecutableDialogResults::OK )
	{
		INetURLObject aPath( xFolderPicker->getDirectory() );
		aPath.removeFinalSlash();
        String aURL = aPath.GetMainURL( INetURLObject::NO_DECODE );
        String sInsPath;
        ::utl::LocalFileHelper::ConvertURLToSystemPath( aURL, sInsPath );

        if ( pImpl->bIsRadioButtonMode )
        {
            ULONG nPos = aRadioLB.GetEntryPos( sInsPath, 1 );
            if ( 0xffffffff == nPos ) //See svtools/source/contnr/svtabbx.cxx SvTabListBox::GetEntryPos
            {
                String sNewEntry( '\t' );
                sNewEntry += sInsPath;
                SvLBoxEntry* pEntry = aRadioLB.InsertEntry( sNewEntry );
                String* pData = new String( aURL );
                pEntry->SetUserData( pData );
            }
            else
            {
                String sMsg( SVX_RES( RID_MULTIPATH_DBL_ERR ) );
                sMsg.SearchAndReplaceAscii( "%1", sInsPath );
                InfoBox( this, sMsg ).Execute();
            }
        }
        else
        {
            if ( LISTBOX_ENTRY_NOTFOUND != aPathLB.GetEntryPos( sInsPath ) )
            {
                String sMsg( SVX_RES( RID_MULTIPATH_DBL_ERR ) );
                sMsg.SearchAndReplaceAscii( "%1", sInsPath );
                InfoBox( this, sMsg ).Execute();
            }
            else
            {
                USHORT nPos = aPathLB.InsertEntry( sInsPath, LISTBOX_APPEND );
                aPathLB.SetEntryData( nPos, (void*)new String( aURL ) );
            }
        }
        SelectHdl_Impl( NULL );
	}
	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxMultiPathDialog, DelHdl_Impl, PushButton *, EMPTYARG )
{
    if ( pImpl->bIsRadioButtonMode )
    {
        SvLBoxEntry* pEntry = aRadioLB.FirstSelected();
        delete (String*)pEntry->GetUserData();
        bool bChecked = aRadioLB.GetCheckButtonState( pEntry ) == SV_BUTTON_CHECKED;
        ULONG nPos = aRadioLB.GetEntryPos( pEntry );
        aRadioLB.RemoveEntry( pEntry );
        ULONG nCnt = aRadioLB.GetEntryCount();
        if ( nCnt )
        {
            nCnt--;
            if ( nPos > nCnt )
                nPos = nCnt;
            pEntry = aRadioLB.GetEntry( nPos );
            if ( bChecked )
            {
                aRadioLB.SetCheckButtonState( pEntry, SV_BUTTON_CHECKED );
                aRadioLB.HandleEntryChecked( pEntry );
            }
            else
                aRadioLB.Select( pEntry );
        }
    }
    else
    {
        USHORT nPos = aPathLB.GetSelectEntryPos();
        aPathLB.RemoveEntry( nPos );
        USHORT nCnt = aPathLB.GetEntryCount();

        if ( nCnt )
        {
            nCnt--;

            if ( nPos > nCnt )
                nPos = nCnt;
            aPathLB.SelectEntryPos( nPos );
        }
    }
    SelectHdl_Impl( NULL );
	return 0;
}

// -----------------------------------------------------------------------

SvxMultiPathDialog::SvxMultiPathDialog( Window* pParent, BOOL bEmptyAllowed ) :

	ModalDialog( pParent, SVX_RES( RID_SVXDLG_MULTIPATH ) ),

    aPathFL     ( this, SVX_RES( FL_MULTIPATH) ),
    aPathLB     ( this, SVX_RES( LB_MULTIPATH ) ),
    aRadioLB    ( this, SVX_RES( LB_RADIOBUTTON ) ),
    aRadioFT    ( this, SVX_RES( FT_RADIOBUTTON ) ),
    aAddBtn     ( this, SVX_RES( BTN_ADD_MULTIPATH ) ),
	aDelBtn		( this, SVX_RES( BTN_DEL_MULTIPATH ) ),
	aOKBtn		( this, SVX_RES( BTN_MULTIPATH_OK ) ),
	aCancelBtn	( this, SVX_RES( BTN_MULTIPATH_CANCEL ) ),
	aHelpButton	( this, SVX_RES( BTN_MULTIPATH_HELP ) ),
	pImpl		( new MultiPath_Impl( bEmptyAllowed ) )

{
    static long aStaticTabs[]= { 2, 0, 12 };
    aRadioLB.SvxSimpleTable::SetTabs( aStaticTabs );
    String sHeader( SVX_RES( STR_HEADER_PATHS ) );
    aRadioLB.SetQuickHelpText( sHeader );
    sHeader.Insert( '\t', 0 );
    aRadioLB.InsertHeaderEntry( sHeader, HEADERBAR_APPEND, HIB_LEFT );

    FreeResource();

    aPathLB.SetSelectHdl( LINK( this, SvxMultiPathDialog, SelectHdl_Impl ) );
    aRadioLB.SetSelectHdl( LINK( this, SvxMultiPathDialog, SelectHdl_Impl ) );
    aRadioLB.SetCheckButtonHdl( LINK( this, SvxMultiPathDialog, CheckHdl_Impl ) );
    aAddBtn.SetClickHdl( LINK( this, SvxMultiPathDialog, AddHdl_Impl ) );
	aDelBtn.SetClickHdl( LINK( this, SvxMultiPathDialog, DelHdl_Impl ) );

    SelectHdl_Impl( NULL );
}

// -----------------------------------------------------------------------

SvxMultiPathDialog::~SvxMultiPathDialog()
{
    USHORT nPos = aPathLB.GetEntryCount();
    while ( nPos-- )
		delete (String*)aPathLB.GetEntryData(nPos);
    nPos = (USHORT)aRadioLB.GetEntryCount();
    while ( nPos-- )
    {
        SvLBoxEntry* pEntry = aRadioLB.GetEntry( nPos );
        delete (String*)pEntry->GetUserData();
    }
    delete pImpl;
}

// -----------------------------------------------------------------------

String SvxMultiPathDialog::GetPath() const
{
    String sNewPath;
	sal_Unicode cDelim = pImpl->bIsClassPathMode ? CLASSPATH_DELIMITER : SVT_SEARCHPATH_DELIMITER;

    if ( pImpl->bIsRadioButtonMode )
    {
        String sWritable;
        for ( USHORT i = 0; i < aRadioLB.GetEntryCount(); ++i )
        {
            SvLBoxEntry* pEntry = aRadioLB.GetEntry(i);
            if ( aRadioLB.GetCheckButtonState( pEntry ) == SV_BUTTON_CHECKED )
                sWritable = *(String*)pEntry->GetUserData();
            else
            {
                if ( sNewPath.Len() > 0 )
                    sNewPath += cDelim;
                sNewPath += *(String*)pEntry->GetUserData();
            }
        }
        if ( sNewPath.Len() > 0 )
            sNewPath += cDelim;
        sNewPath += sWritable;
    }
    else
    {
        for ( USHORT i = 0; i < aPathLB.GetEntryCount(); ++i )
        {
            if ( sNewPath.Len() > 0 )
                sNewPath += cDelim;
            sNewPath += *(String*)aPathLB.GetEntryData(i);
        }
    }
    return sNewPath;
}

// -----------------------------------------------------------------------

void SvxMultiPathDialog::SetPath( const String& rPath )
{
	sal_Unicode cDelim = pImpl->bIsClassPathMode ? CLASSPATH_DELIMITER : SVT_SEARCHPATH_DELIMITER;
    USHORT nPos, nCount = rPath.GetTokenCount( cDelim );

    for ( USHORT i = 0; i < nCount; ++i )
	{
        String sPath = rPath.GetToken( i, cDelim );
        String sSystemPath;
        sal_Bool bIsSystemPath =
            ::utl::LocalFileHelper::ConvertURLToSystemPath( sPath, sSystemPath );

        if ( pImpl->bIsRadioButtonMode )
        {
            String sEntry( '\t' );
            sEntry += bIsSystemPath ? sSystemPath : sPath;
            SvLBoxEntry* pEntry = aRadioLB.InsertEntry( sEntry );
            String* pURL = new String( sPath );
            pEntry->SetUserData( pURL );
        }
        else
        {
            if ( bIsSystemPath )
                nPos = aPathLB.InsertEntry( sSystemPath, LISTBOX_APPEND );
            else
                nPos = aPathLB.InsertEntry( sPath, LISTBOX_APPEND );
            aPathLB.SetEntryData( nPos, (void*)new String( sPath ) );
        }
	}

    if ( pImpl->bIsRadioButtonMode && nCount > 0 )
    {
        SvLBoxEntry* pEntry = aRadioLB.GetEntry( nCount - 1 );
        if ( pEntry )
        {
            aRadioLB.SetCheckButtonState( pEntry, SV_BUTTON_CHECKED );
            aRadioLB.HandleEntryChecked( pEntry );
        }
    }

    SelectHdl_Impl( NULL );
}

// -----------------------------------------------------------------------

void SvxMultiPathDialog::SetClassPathMode()
{
	pImpl->bIsClassPathMode = TRUE;
	SetText( SVX_RES( RID_SVXSTR_ARCHIVE_TITLE ));
    aPathFL.SetText( SVX_RES( RID_SVXSTR_ARCHIVE_HEADLINE ) );
}

// -----------------------------------------------------------------------

sal_Bool SvxMultiPathDialog::IsClassPathMode() const
{
    return pImpl->bIsClassPathMode;
}

// -----------------------------------------------------------------------

void SvxMultiPathDialog::EnableRadioButtonMode()
{
    pImpl->bIsRadioButtonMode = true;

    aPathFL.Hide();
    aPathLB.Hide();

    aRadioLB.ShowTable();
    aRadioFT.Show();

    Point aNewPos = aAddBtn.GetPosPixel();
    long nDelta = aNewPos.Y() - aRadioLB.GetPosPixel().Y();
    aNewPos.Y() -= nDelta;
    aAddBtn.SetPosPixel( aNewPos );
    aNewPos = aDelBtn.GetPosPixel();
    aNewPos.Y() -= nDelta;
    aDelBtn.SetPosPixel( aNewPos );
}

