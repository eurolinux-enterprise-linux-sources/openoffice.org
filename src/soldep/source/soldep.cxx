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
//TBD: ToolBox handling prjview/back

#include <osl/file.hxx>
#include <rtl/ustring.hxx>
#include <tools/debug.hxx>
#include <bootstrp/sstring.hxx>
#include <svtools/filedlg.hxx>
#include <tools/iparser.hxx>
#include <tools/geninfo.hxx>
#include <vcl/gdimtf.hxx>
#include <vcl/bitmap.hxx>
#include <appdef.hxx>
#include "time.h"
#include <soldep/depper.hxx>
#include <soldep/soldep.hxx>
#include <soldep/soldlg.hxx>
#include <soldep/XmlBuildList.hxx>
#include "dtsodcmp.hrc"

IMPLEMENT_HASHTABLE_OWNER( SolIdMapper, ByteString, ULONG* );
//IMPLEMENT_HASHTABLE_OWNER( PrjIdMapper, ByteString, ULONG* );
#define EVENT_RESIZE                0x00000001
#define MIN(a,b) (a)<(b)?(a):(b)
#define MAX(a,b) (a)>(b)?(a):(b)


//ByteString sDelimiterLine("#==========================================================================");


//
// class SolDep
//

/*****************************************************************************/
SolDep::SolDep( Window* pBaseWindow )
/*****************************************************************************/
				: Depper( pBaseWindow ),
                mbBServer(FALSE),
                mpTravellerList( NULL ),
				mbIsHide( FALSE ),
				mpXmlBuildList (NULL)
{
	/*
	ByteString sModulPath ("."); // wo soll das Perlmodul stehen???
	try
	{
		mpXmlBuildList = new XmlBuildList (sModulPath);
	}
	catch (XmlBuildListException& Exception) {
        const char* Message = Exception.getMessage();
    }
	*/
    mnSolWinCount = 0;
	mnSolLastId = 0;
//    mpPrjIdMapper = new SolIdMapper( 63997 );
    maTaskBarFrame.EnableAlwaysOnTop();
    maTaskBarFrame.Show();
    maToolBox.SetPosSizePixel( Point( 0,0 ), Size( 1100,35 ));
	maToolBox.SetSelectHdl( LINK ( this, SolDep, ToolSelect ));
    maToolBox.Show();

	mpBaseWin->AddChildEventListener( LINK( this, SolDep, ChildWindowEventListener ));

    // Kontext-Menue (geh�rt zu soldep.cxx)
    InitContextMenueMainWnd();
    InitContextMenuePrjViewWnd( mpBasePrjWin );
}

/*****************************************************************************/
SolDep::~SolDep()
/*****************************************************************************/
{
    mpBaseWin->RemoveChildEventListener( LINK( this, SolDep, ChildWindowEventListener ) );
	delete mpSolIdMapper;
	delete mpStarWriter;
	delete mpStandLst;
	if (mpXmlBuildList)
		delete mpXmlBuildList;
}

/*****************************************************************************/
void SolDep::Init()
/*****************************************************************************/
{
	InformationParser aParser;
	String sStandLst( GetDefStandList(), RTL_TEXTENCODING_ASCII_US );
	mpStandLst = aParser.Execute( sStandLst );
    ByteString aUpdater( getenv("UPDATER") );
	if ( mpStandLst && (aUpdater == "YES") ) {
		if ( GetVersion() )
			ReadSource( TRUE );
	} else
	{
		ReadSource();   // if stand.lst isn't available
	}
}

/*****************************************************************************/
void SolDep::Init( ByteString &rVersion, GenericInformationList *pVersionList )
/*****************************************************************************/
{
    // Interface for bs
    mbBServer=TRUE;
	if ( pVersionList )
		mpStandLst = new GenericInformationList( *pVersionList );
	else {
		InformationParser aParser;
		String sStandLst( GetDefStandList(), RTL_TEXTENCODING_ASCII_US );
		mpStandLst = aParser.Execute( sStandLst );
	}
	if ( mpStandLst ) {
		msVersionMajor = ByteString( rVersion );
		ReadSource(TRUE); //call from build server set UPDATER to TRUE
	}
}

/*****************************************************************************/
IMPL_LINK( SolDep, ChildWindowEventListener, VclSimpleEvent*, pEvent )
/*****************************************************************************/
{
	if ( pEvent && pEvent->ISA( VclWindowEvent ) )
	{
		ProcessChildWindowEvent( *static_cast< VclWindowEvent* >( pEvent ) );
	}
	return 0;
}


/*****************************************************************************/
void SolDep::ProcessChildWindowEvent( const VclWindowEvent& _rVclWindowEvent )
/*****************************************************************************/
{
    Window* pChildWin = _rVclWindowEvent.GetWindow();
//    Window* pParentWin = pChildWin->GetParent();
//Resize();
    if ( isAlive() )
		{
            ULONG id = _rVclWindowEvent.GetId();
			switch ( id )
			{
				case VCLEVENT_USER_MOUSEBUTTON_DOWN:
					{
						ObjectWin* pObjWin = dynamic_cast<ObjectWin*>(pChildWin);
						if( pObjWin )
						{
                            // handle mouse click on ObjectWin object
                            ObjectWin* pWin = (ObjectWin*) pChildWin;
                            //GetObjectList()->ResetSelectedObject();
                            if (IsHideMode())      // simple mouse click left
                            {
                                pWin->CaptureMouse();
			                    pWin->SetMarkMode( MARKMODE_SELECTED );
                                pWin->MarkNeeded();
                                pWin->MarkDepending();
                                pWin->Invalidate();
                            } else
                            {
                                pWin->LoseFocus();
                                pWin->SetMarkMode( MARKMODE_SELECTED );
                                pWin->UnsetMarkMode( MARKMODE_ACTIVATED );
			                    pWin->MarkNeeded( TRUE );
			                    pWin->MarkDepending( TRUE );
                            }

						}
					}
					break;
                case VCLEVENT_USER_MOUSEBUTTON_DOWN_ALT:
                    {
                        ObjectWin* pObjWin = dynamic_cast<ObjectWin*>(pChildWin);
						if( pObjWin )
						{
                            ObjectWin* pWin = (ObjectWin*) pChildWin;
                            MarkObjects( pWin );
                        }
                    }
                    break;
                case VCLEVENT_USER_MOUSEBUTTON_DOWN_DBLCLICK:
                    {
                        ObjectWin* pObjWin = dynamic_cast<ObjectWin*>(pChildWin);
						if( pObjWin )
						{
							if (IsHideMode()) ToggleHideDependency();
                            ByteString text = ((ObjectWin*) pChildWin)->GetBodyText();
                            ViewContent(text);
                        }
                    }
                    break;
                case VCLEVENT_USER_MOUSEBUTTON_UP_SHFT:
                    {
                        ObjectWin* pObjWin = dynamic_cast<ObjectWin*>(pChildWin);
						if( pObjWin )
						{
                            ObjectWin* pWin = (ObjectWin*) pChildWin;
                            GetDepWin()->NewConnector( pWin );
                        }
                    }
                    break;
                case VCLEVENT_USER_MOUSEBUTTON_UP:
                     {
                        ObjectWin* pObjWin = dynamic_cast<ObjectWin*>(pChildWin);
						if( pObjWin )
						{
                            ObjectWin* pWin = (ObjectWin*) pChildWin;
                			pWin->ReleaseMouse();
                            pWin->SetMarkMode(MARKMODE_SELECTED);
                            GetDepWin()->Invalidate();
                        }
                     }
                     break;
			}    // switch
		}  // if isAlive
        //fprintf(stdout,"BLA::Resize: %d\n",pChildWin);
}

/*****************************************************************************/
IMPL_LINK( SolDep, ToolSelect, SoldepToolBox* , pBox)
/*****************************************************************************/
{
	USHORT nItemId = pBox->GetCurItemId();
	switch ( nItemId )
	{
		case TID_SOLDEP_FIND:
			FindProject();
			break;
        case TID_SOLDEP_CREATEMETA :
        {
            VirtualDevice   aVDev;
			aVDev.SetMapMode( MAP_100TH_MM );
            GDIMetaFile     aMtf;
            aVDev.EnableOutput( FALSE );
            aMtf.Record( &aVDev );

			aVDev.SetLineColor( Color( COL_BLACK ) );
			aVDev.SetTextAlign( ALIGN_TOP );

			Size aSize( GetDepWin()->GetOutputSizePixel() );
			long nXMin = aSize.Width();
			long nXMax = 0;
			long nYMax = 0;
			long nYMin = aSize.Height();

			for ( USHORT i=0; i<mpObjectList->Count(); i++ )
			{
				Point aPoint = mpObjectList->GetObject(i)->GetPosPixel();
				Size aSize = mpObjectList->GetObject(i)->GetSizePixel();
				nXMin = MIN( aPoint.X(), nXMin );
				nXMax = MAX( aPoint.X() + aSize.Width(), nXMax );
				nYMin = MIN( aPoint.Y(), nYMin );
				nYMax = MAX( aPoint.Y() + aSize.Height(), nYMax );
			}

			Point aOffset( nXMin, nYMin );
			aOffset = aVDev.PixelToLogic( aOffset );

            GetDepWin()->DrawOutput( &aVDev, aOffset );
			for ( USHORT i=0; i<mpObjectList->Count(); i++ )
				if ( mpObjectList->GetObject(i)->IsVisible() )
					mpObjectList->GetObject(i)->DrawOutput( &aVDev, aOffset );

            aMtf.Stop();
            aMtf.WindStart();
            aMtf.SetPrefMapMode( aVDev.GetMapMode() );
			Size aDevSize( nXMax-nXMin + 10, nYMax-nYMin + 10);
			aDevSize =	aVDev.PixelToLogic( aDevSize );
            aMtf.SetPrefSize( aDevSize );
			SvFileStream aStream( String::CreateFromAscii("d:\\out.svm"), STREAM_STD_READWRITE );
			aMtf.Write( aStream );
            break;
        }		
		case TID_SOLDEP_HIDE_INDEPENDEND:
			{
                ToggleHideDependency();
				for ( USHORT i=0; i<mpObjectList->Count(); i++ )
					mpObjectList->GetObject(i)->SetViewMask(!mbIsHide);

   	    	    maToolBox.CheckItem(TID_SOLDEP_HIDE_INDEPENDEND, IsHideMode());
       	    	GetDepWin()->Invalidate(); //repaint Main-View
			}
            break;
        case TID_SOLDEP_SELECT_WORKSPACE:
			if (mpStandLst)
			{
				if (GetVersion()) // Version dialog box
                {
                    delete mpSolIdMapper;
	                delete mpStarWriter;
                    mpObjectList->ClearAndDelete();
				    ReadSource(TRUE);
                }
			}
            break;
        case TID_SOLDEP_BACK:
            maToolBox.HideItem(TID_SOLDEP_BACK);
			maToolBox.ShowItem(TID_SOLDEP_SELECT_WORKSPACE);  //disabled for prj view (doubleclick ObjWin)
            maToolBox.ShowItem(TID_SOLDEP_HIDE_INDEPENDEND);  //disabled for prj view (doubleclick ObjWin)
            maToolBox.ShowItem(TID_SOLDEP_FIND);              //disabled for prj view (doubleclick ObjWin)
            maToolBox.Resize();
			TogglePrjViewStatus();
            break;
	}
	return 0;
}

/*****************************************************************************/
void SolDep::ToggleHideDependency()
/*****************************************************************************/
{
	mbIsHide = !mbIsHide;
	maToolBox.CheckItem(TID_SOLDEP_HIDE_INDEPENDEND, IsHideMode());
	ObjectWin* pWin = GetObjectList()->GetObject( 0 );
	pWin->ToggleHideMode();
};

/*****************************************************************************/
BOOL SolDep::GetVersion()
/*****************************************************************************/
{
	SolSelectVersionDlg aVersionDlg( GetDepWin(), mpStandLst );
	if ( aVersionDlg.Execute() == RET_OK ) {
		msVersionMajor = aVersionDlg.GetVersionMajor();
        msVersionMinor = aVersionDlg.GetVersionMinor();
		return TRUE;
	}
	return FALSE;
}

void SolDep::InitContextMenueMainWnd()
{
    InitContextMenuePrjViewWnd( mpBaseWin );
    return; // Disable not actually supported items

    mpBaseWin->mpPopup->InsertItem( DEPPOPUP_AUTOARRANGE, String::CreateFromAscii("Autoarrange")) ;
    mpBaseWin->mpPopup->InsertSeparator();
	mpBaseWin->mpPopup->InsertItem( DEPPOPUP_READ_SOURCE, String::CreateFromAscii("Revert all changes") );
	mpBaseWin->mpPopup->InsertSeparator();
	mpBaseWin->mpPopup->InsertItem( DEPPOPUP_OPEN_SOURCE, String::CreateFromAscii("Open") );
	mpBaseWin->mpPopup->InsertItem( DEPPOPUP_WRITE_SOURCE, String::CreateFromAscii("Save") );
}

void SolDep::InitContextMenuePrjViewWnd(DepWin* pBaseWin )
{
    // temp. disabled pBaseWin->mpPopup->InsertItem( DEPPOPUP_NEW, String::CreateFromAscii("New object") );
	pBaseWin->mpPopup->InsertItem( DEPPOPUP_ZOOMIN, String::CreateFromAscii("Zoom in") );
	pBaseWin->mpPopup->InsertItem( DEPPOPUP_ZOOMOUT, String::CreateFromAscii("Zoom out") );
	pBaseWin->mpPopup->InsertSeparator();
	// temp disabled pBaseWin->mpPopup->InsertItem( DEPPOPUP_CLEAR, String::CreateFromAscii("Clear") );
    pBaseWin->mpPopup->InsertItem( DEPPOPUP_SHOW_TOOLBOX, String::CreateFromAscii("Show Toolbox") );
}

/*****************************************************************************/
ObjectWin *SolDep::RemoveObject( USHORT nId, BOOL bDelete )
/*****************************************************************************/
{
	Prj* pPrj;

//hshtable auf stand halten
	ObjectWin* pWin = RemoveObjectFromList( mpObjectList, mnSolWinCount, nId, FALSE );
	if ( pWin )
	{
		ByteString aBodyText( pWin->GetBodyText() );
		if( (pPrj = mpStarWriter->GetPrj( aBodyText )) )
		{
			mpStarWriter->Remove( pPrj );
//cleanup ist teuer...
			mpStarWriter->CleanUp();
			delete pPrj;
		}
		else
			DBG_ASSERT( FALSE, "project not found - write" );

		mpSolIdMapper->Delete( aBodyText );
		if ( bDelete )
			delete pWin;
		return pWin;
	}
	else
		return NULL;
}

/*****************************************************************************/
ULONG SolDep::AddObject( ByteString& rBodyText, BOOL bInteract )
/*****************************************************************************/
{
    ULONG nObjectId;
	if ( bInteract )
	{
		nObjectId = HandleNewPrjDialog( rBodyText );
	}
	else
	{
//hashtable auf stand halten
		MyHashObject* pHObject;
		nObjectId = AddObjectToList( mpBaseWin, mpObjectList, mnSolLastId, mnSolWinCount, rBodyText, FALSE );
		pHObject = new MyHashObject( nObjectId, ObjIdToPtr(mpObjectList, nObjectId ));
		mpSolIdMapper->Insert( rBodyText, pHObject );
	}
    return nObjectId;
}

/*****************************************************************************/
ULONG SolDep::AddPrjObject( ByteString& rBodyText, BOOL bInteract )
/*****************************************************************************/
{
    ULONG nObjectId;
	if ( bInteract )
	{
        nObjectId = HandleNewDirectoryDialog( rBodyText );
	}
	else
	{
//hshtable auf stand halten
		MyHashObject* pHObject;
		nObjectId = AddObjectToList( mpBasePrjWin, mpObjectPrjList, mnPrjLastId, mnPrjWinCount, rBodyText );
		pHObject = new MyHashObject( nObjectId, ObjIdToPtr( mpObjectPrjList, nObjectId ));
		mpPrjIdMapper->Insert( rBodyText, pHObject ); // mpPrjIdMapper
	}
    return nObjectId;
}

/*****************************************************************************/
USHORT SolDep::AddConnector( ObjectWin* pStartWin, ObjectWin* pEndWin )
/*****************************************************************************/
{
//	DBG_ASSERT( FALSE , "not yet" );
	ByteString sEndName = pEndWin->GetBodyText();
	ByteString sStartName = pStartWin->GetBodyText();

	Prj* pPrj = mpStarWriter->GetPrj( sEndName );
	if ( pPrj )
	{
		pPrj->AddDependencies( sStartName );
		return AddConnectorToObjects( pStartWin, pEndWin );
	}
	else
	{
		DBG_ASSERT( FALSE , "non existing Project" );
		return 1;
	}
}

/*****************************************************************************/
USHORT SolDep::RemoveConnector( ObjectWin* pStartWin, ObjectWin* pEndWin )
/*****************************************************************************/
{
	SByteStringList* pPrjDeps = NULL;
	ByteString sEndName = pEndWin->GetBodyText();
	ByteString sStartName = pStartWin->GetBodyText();

	Prj* pPrj = mpStarWriter->GetPrj( sEndName );
	pPrjDeps = pPrj->GetDependencies( FALSE );
	if ( pPrjDeps )
	{
		ByteString* pString;
		ULONG nPrjDepsCount = pPrjDeps->Count();
		for ( ULONG j = nPrjDepsCount; j > 0; j-- )
		{
			pString = pPrjDeps->GetObject( j - 1 );
			if ( pString->GetToken( 0, '.') == sStartName )
				pPrjDeps->Remove( pString );
		}
	}

	return RemoveConnectorFromObjects( pStartWin, pEndWin );
}

/*****************************************************************************/
void SolDep::RemoveAllObjects( ObjectList* pObjLst )
/*****************************************************************************/
{

	Depper::RemoveAllObjects( pObjLst );

	if ( mpSolIdMapper )
	{
		delete mpSolIdMapper;
		mpSolIdMapper = NULL;
	}
	if ( mpStarWriter )
	{
		delete mpStarWriter;
		mpStarWriter = NULL;
	}
}

/*****************************************************************************/
ULONG SolDep::GetStart(SolIdMapper* pIdMapper, ObjectList* pObjList)
/*****************************************************************************/
{
//	DBG_ASSERT( FALSE , "soldep" );
	MyHashObject* pHObject = pIdMapper->Find( "null" );//null_project

	if ( !pHObject ) {
		ByteString sNullPrj = "null";//null_project
		ULONG nObjectId = AddObject( sNullPrj, FALSE );
		ObjIdToPtr( pObjList, nObjectId )->SetViewMask( 1 );
		return nObjectId;
	}

	return pHObject->GetId();
}

/*****************************************************************************/
ULONG SolDep::GetStartPrj(SolIdMapper* pIdMapper, ObjectList* pObjList)
/*****************************************************************************/
{
//	DBG_ASSERT( FALSE , "prjdep" );
	MyHashObject* pHObject = mpPrjIdMapper->Find( ByteString( "null" ) ); //null_dir
	if ( !pHObject )
	{
		ByteString bsNull("null");
		ULONG nObjectId = AddPrjObject( bsNull, FALSE); //null_dir
		return nObjectId;
	}
	else
		return pHObject->GetId();
}

/*****************************************************************************/
USHORT SolDep::OpenSource()
/*****************************************************************************/
{
	if ( mpStandLst ) {
		if ( GetVersion())
			return ReadSource();
	}
	return 0;
}

/*****************************************************************************/
USHORT SolDep::ReadSource(BOOL bUpdater)
/*****************************************************************************/
{
	mpBaseWin->EnablePaint( FALSE );
    mpBaseWin->Hide();
	ULONG nObjectId, nHashedId;
	ULONG i;
	MyHashObject* pHObject;
	ByteString* pStr;
	ObjectWin *pStartWin, *pEndWin;

    mpSolIdMapper = new SolIdMapper( 63997 );
    if (mpStandLst && bUpdater)
    {
	    mpStarWriter = new StarWriter( mpXmlBuildList, mpStandLst, msVersionMajor, msVersionMinor, TRUE );
    } else
    {
        SolarFileList* pSolarFileList;
        pSolarFileList = GetPrjListFromDir();
        mpStarWriter = new StarWriter( mpXmlBuildList, pSolarFileList, TRUE );
    }
	ByteString sTitle( SOLDEPL_NAME );
	if ( mpStarWriter->GetMode() == STAR_MODE_SINGLE_PARSE ) {
		sTitle += ByteString( " - mode: single file [" );
	   	sTitle += (ByteString) mpStarWriter->GetName();
		sTitle += ByteString( "]" );
	}
	else if ( mpStarWriter->GetMode() == STAR_MODE_MULTIPLE_PARSE ) {
		sTitle += ByteString( " - mode: multiple files [" );
		sTitle += ByteString( "]" );
	}
	SetTitle( String( sTitle, RTL_TEXTENCODING_UTF8) );

	ULONG nCount = mpStarWriter->Count();
	for ( i=0; i<nCount; i++ )
	{
		Prj *pPrj = mpStarWriter->GetObject(i);
		ByteString sPrjName = pPrj->GetProjectName();
		nObjectId = AddObject( sPrjName, FALSE );
		ObjIdToPtr( mpObjectList, nObjectId )->SetViewMask( 1 );
	}
	for ( i=0; i<nCount; i++ )
	{
		Prj *pPrj = mpStarWriter->GetObject(i);
		SByteStringList *pLst = pPrj->GetDependencies( FALSE );
		if ( pLst )
		{
			ULONG nDepCount = pLst->Count();
			for ( ULONG m=0; m<nDepCount; m++)
			{
				pStr = pLst->GetObject(m);
				pHObject = mpSolIdMapper->Find( *pStr );
				/*if ( !pHObject )
				{
	// create new prj
					Prj *pNewPrj = new Prj( *pStr );
					ByteString sPrjName = pNewPrj->GetProjectName();
					nObjectId = AddObject( sPrjName, FALSE );
					pHObject = mpSolIdMapper->Find( *pStr );
					ObjIdToPtr( mpObjectList, nObjectId )->SetViewMask( 2 );
				}*/

				if ( pHObject )
				{
				nHashedId = pHObject->GetId();
				ByteString sF_Os2 = pPrj->GetProjectName();
				pStr = &sF_Os2;
				pHObject = mpSolIdMapper->Find( *pStr );
				nObjectId = pHObject->GetId();
				pStartWin = ObjIdToPtr( mpObjectList, nHashedId );
				pEndWin = ObjIdToPtr( mpObjectList, nObjectId );
				AddConnectorToObjects( pStartWin, pEndWin );
			}
		}
	}
	}
    if (!IsPrjView())
    {
	    AutoArrange( mpSolIdMapper, mpObjectList, GetStart(mpSolIdMapper,mpObjectList), 0, GetStart(mpSolIdMapper,mpObjectList) );
	    GetDepWin()->EnablePaint( TRUE );
    }
	return 0;
}

SolarFileList* SolDep::GetPrjListFromDir()
{
    SolarFileList* pSolarFileList = new SolarFileList();
    String sPrjDir( String::CreateFromAscii( "prj" ));
	String sBuildLst( String::CreateFromAscii( "build.lst" ));
    DirEntry aCurrent( getenv( SOURCEROOT ) );

    aCurrent.ToAbs();
    Dir aDir( aCurrent, FSYS_KIND_DIR );

	USHORT nEntries = aDir.Count();
	if( nEntries )
	{
		UniStringList aSortDirList;
		for ( USHORT n = 0; n < nEntries; n++ )
		{
			DirEntry& rEntry = aDir[n];
			UniString aName( rEntry.GetName() );
			if( aName.Len() && ( aName.GetChar(0) != '.' ) && rEntry.Exists() )
			{
                rEntry += DirEntry( sPrjDir );
                rEntry += DirEntry( sBuildLst );
                if (rEntry.Exists())
                {
                    pSolarFileList->Insert( new String( rEntry.GetFull() ), LIST_APPEND );
                    ByteString aName_dbg(rEntry.GetFull(),RTL_TEXTENCODING_UTF8);
                    fprintf(stdout, "bla:%s\n", aName_dbg.GetBuffer());
                }
            }
        }
    }
    if ( !pSolarFileList->Count() )
    {
         //is empty!! TBD
         delete pSolarFileList;
         return NULL;
    }
    return pSolarFileList;
}

/*****************************************************************************/
USHORT SolDep::WriteSource()
/*****************************************************************************/
{
/* zur Sicherheit deaktiviert
	USHORT nMode = mpStarWriter->GetMode();
	if ( nMode == STAR_MODE_SINGLE_PARSE ) {
		ByteString sFileName = mpStarWriter->GetName();
		if ( sFileName.Len()) {
			mpStarWriter->Write( String( sFileName, RTL_TEXTENCODING_UTF8) );
			mpStarWriter->RemoveProject( ByteString( "null"));  //null_project
		}
	}
	else if ( nMode == STAR_MODE_MULTIPLE_PARSE ) {
	// *OBO*
	//String sRoot = mpStarWriter->GetSourceRoot();
	//nicht mehr unterst�tzt mpStarWriter->GetSourceRoot()
		ByteString sFileName = mpStarWriter->GetName();
		DirEntry aEntry( sFileName );
		aEntry.ToAbs();
		aEntry = aEntry.GetPath().GetPath().GetPath();
		String sRoot = aEntry.GetFull();

		if ( sRoot.Len()) {
			mpStarWriter->RemoveProject( ByteString( "null")); //null_project
			mpStarWriter->WriteMultiple( sRoot );
		}
	}
*/
	return 1;
}

USHORT SolDep::Load( const ByteString& rFileName )
{
// moved from depper class
	DBG_ASSERT( FALSE , "you are dead!" );
	SvFileStream aInFile( String( rFileName, RTL_TEXTENCODING_UTF8 ), STREAM_READ );
	depper_head dh;
	ULONG i;
	ULONG nLoadOffs = mnSolLastId;     //or Prj??
	ObjectWin* pNewWin;
	aInFile.Read( &dh, sizeof( dh ));

	ULONG nObjCount = dh.nObjectCount;
	ULONG nCnctrCount = dh.nCnctrCount;

	for ( i=0; i < nObjCount ; i++ )
	{
		ObjectWin* pWin = new ObjectWin( mpBaseWin, WB_BORDER );
		pWin->Load( aInFile );
		pNewWin = ObjIdToPtr( mpObjectList, AddObjectToList( mpBaseWin, mpObjectList, mnSolLastId, mnSolWinCount, pWin->GetBodyText(), FALSE ));
		pNewWin->SetId( nLoadOffs + pWin->GetId());
		pNewWin->SetPosPixel( pWin->GetPosPixel());
		pNewWin->SetSizePixel( pWin->GetSizePixel());
	}

	ULONG nStartId;
	ULONG nEndId;
//	ueber addconnector fuehren!
	for ( i=0; i < nCnctrCount ; i++ )
	{
		Connector* pCon = new Connector( mpBaseWin, WB_NOBORDER );
		pCon->Load( aInFile );

		nStartId = nLoadOffs + pCon->GetStartId();
		nEndId = nLoadOffs + pCon->GetEndId();

		ObjectWin* pStartWin = ObjIdToPtr( mpObjectList, nStartId );
		ObjectWin* pEndWin = ObjIdToPtr( mpObjectList, nEndId );

		pCon->Initialize( pStartWin, pEndWin );
	}


	return 0;
}

/*****************************************************************************/
BOOL SolDep::ViewContent( ByteString& rObjectName )
/*****************************************************************************/
{
	mpFocusWin = NULL;
	SetPrjViewStatus(TRUE);

	for ( ULONG i = 0; i < mpObjectList->Count() && !mpFocusWin; i++ )
		if ( mpObjectList->GetObject( i )->HasFocus())
			mpFocusWin = mpObjectList->GetObject( i );
    //HideObjectsAndConnections( mpObjectList );
	mpProcessWin->Resize();
    GetDepWin()->Show();
	return InitPrj( rObjectName );
}

/*****************************************************************************/
BOOL SolDep::InitPrj( ByteString& rListName )
/*****************************************************************************/
{
	ULONG nObjectId, nHashedId;
	ULONG i, j;
	MyHashObject* pHObject;
	ByteString *pDepName;
	ByteString *pFlagName;
	Prj* pPrj;
	ObjectWin *pStartWin, *pEndWin;
	maToolBox.HideItem(TID_SOLDEP_SELECT_WORKSPACE);
	maToolBox.HideItem(TID_SOLDEP_HIDE_INDEPENDEND);
	maToolBox.HideItem(TID_SOLDEP_FIND);
	maToolBox.ShowItem(TID_SOLDEP_BACK);
	maToolBox.Invalidate();

    //clean up
    mpObjectPrjList->ClearAndDelete();
    GetDepWin()->ClearConnectorList();
    if (mpPrjIdMapper) delete mpPrjIdMapper;
    mpPrjIdMapper = new SolIdMapper( 63997 ); //generate clean mapper
    mnPrjWinCount = 0;
	mnPrjLastId = 0;

	ULONG nCount = mpStarWriter->Count();
	GetDepWin()->EnablePaint( FALSE );
    Point aPnt = GetGraphWin()->GetPosPixel();
    Size aSize = GetGraphWin()->GetSizePixel();

    GetGraphWin()->SetPosSizePixel( aPnt, aSize );         // Hier wird das Window gesetzt

	BOOL bReturn = FALSE;

	for ( i=0; i<nCount; i++ )
	{
// pPrj->GetProjectName() returns the name of
// the project e.g. svtools
		pPrj = mpStarWriter->GetObject(i);
		ByteString sPrjName = pPrj->GetProjectName();
		if ( sPrjName == rListName )
		{
			bReturn = TRUE;

			mpPrj = mpStarWriter->GetObject(i);
			ULONG nDirCount = mpPrj->Count();
			for ( j=0; j<nDirCount; j++ )
			{
				CommandData *pData = mpPrj->GetObject(j);
				fprintf( stdout, "\tProjectDir : %s\n",
						pData->GetLogFile().GetBuffer());
// pData->GetLogFile() contains internal project IDs
// e.g. st_mkout etc.
				if ( pData->GetLogFile() != "" )
				{
					ByteString sItem = pData->GetLogFile();
					nObjectId = AddPrjObject( sItem, FALSE);
// there may be faster ways......
					ObjectWin *pWin = ObjIdToPtr( mpObjectPrjList, nObjectId );
					pWin->SetViewMask( 0x0001 );
// pData->GetPath() contains internal project directories
// e.g. svtools/inc etc.
                    ByteString sPath = pData->GetPath();
					pWin->SetTipText( sPath );
				}
			}

// set connectors for dependencies here
			for ( j=0; j<nDirCount; j++ )
			{
				CommandData *pData = mpPrj->GetObject(j);
				SByteStringList *pDeps = pData->GetDependencies();
				if ( pDeps )
				{
					ByteString sFlagName = pData->GetLogFile();
					pFlagName = &sFlagName;
					//pHObject = mpPrjIdMapper->Find( (*pFlagName).GetToken( 0, '.'));//mpSolIdMapper see ReadSource()
                    pHObject = mpPrjIdMapper->Find( sFlagName.GetToken( 0, '.'));
                    if (pHObject)
                    {

    					nObjectId = pHObject->GetId();

	    				ULONG nDepCount = pDeps->Count();
		    			for ( ULONG k=0; k<nDepCount; k++ )
			    		{
				    		pDepName = pDeps->GetObject(k);
					    	pHObject = mpPrjIdMapper->Find( (*pDepName).GetToken( 0, '.'));
						    if (pHObject )
    						{
	    						nHashedId = pHObject->GetId();
		    					pStartWin = ObjIdToPtr( mpObjectPrjList, nHashedId );
			    				pEndWin = ObjIdToPtr( mpObjectPrjList, nObjectId );

				    			AddConnectorToObjects( pStartWin, pEndWin );
					    	}
						    else
    						{
	    						String sMessage;
		    					sMessage += String::CreateFromAscii("can't find ");
			    				sMessage += String( *pDepName, RTL_TEXTENCODING_UTF8 );
				    			sMessage += String::CreateFromAscii(".\ndependency ignored");
					    		WarningBox aBox( GetDepWin(), WB_OK, sMessage);
						    	aBox.Execute();
						    }
					    }
                    }
				}

			}

			break;
		}
	}
	ByteString sNullDir = "null";
	nObjectId = AddPrjObject( sNullDir, FALSE);
	ObjectWin *pWin = ObjIdToPtr( mpObjectPrjList, nObjectId );
	pWin->SetViewMask( 0x0001 );
	mpGraphPrjWin->EnablePaint( TRUE );
    //debug
//    int test_l = GetStartPrj(mpPrjIdMapper, mpObjectPrjList);
//    ObjectWin *pTestWin = ObjIdToPtr( mpObjectPrjList, test_l );
	AutoArrange( mpPrjIdMapper, mpObjectPrjList, GetStartPrj(mpPrjIdMapper, mpObjectPrjList), 0, GetStartPrj(mpPrjIdMapper, mpObjectPrjList) );
    mpGraphWin->Hide();
    mpGraphPrjWin->Show();
	mpGraphPrjWin->Invalidate();

	return bReturn;
}

/*****************************************************************************/
USHORT SolDep::CloseWindow()
/*****************************************************************************/
{

	((SystemWindow*)mpProcessWin)->Close();
	return 0;
}

/*****************************************************************************/
void SolDep::ShowHelp()
/*****************************************************************************/
{
	SvFileStream aHelpFile( String::CreateFromAscii( "g:\\soldep.hlp" ), STREAM_READ );
	String aHelpText;
	ByteString aGetStr;

	if ( aHelpFile.IsOpen() )
	{
		while ( aHelpFile.ReadLine( aGetStr ) )
		{
			aHelpText += String (aGetStr, RTL_TEXTENCODING_UTF8);
			aHelpText += String::CreateFromAscii("\n");
		}
	}
	else
		aHelpText = String::CreateFromAscii("No Helpfile found.");

	SolHelpDlg aHelpDlg( mpBaseWin, DtSodResId( RID_SD_DIALOG_HELP ));
	aHelpDlg.maMLEHelp.SetText( aHelpText );
	aHelpDlg.maMLEHelp.SetReadOnly();
	aHelpDlg.maMLEHelp.EnableFocusSelectionHide( TRUE );
	aHelpDlg.Execute();
}

/*****************************************************************************/
BOOL SolDep::FindProject()
/*****************************************************************************/
{
	SolFindProjectDlg aFindProjectDlg( GetDepWin(), GetObjectList() );
	ObjectWin* pObjectWin = NULL;
    mpObjectList->ResetSelectedObject();
    if (IsHideMode())
    {
        GetDepWin()->Invalidate();
    }

    mpFocusWin=NULL;

	if ( aFindProjectDlg.Execute() == RET_OK ) {
		msProject = aFindProjectDlg.GetProject();
		//now we have a project string
        pObjectWin = mpObjectList->GetPtrByName( msProject );
        mpObjectList->ResetSelectedObject();
        MarkObjects( pObjectWin );
	}
	return FALSE;
}

BOOL SolDep::MarkObjects( ObjectWin* pObjectWin )
{
    if (pObjectWin)
    {
        if (!(pObjectWin->IsNullObject()))
        {
            pObjectWin->SetMarkMode( MARKMODE_SELECTED );
	  	    pObjectWin->MarkNeeded();
	        pObjectWin->MarkDepending();
            if (IsHideMode())
            {
                GetDepWin()->Invalidate();
            }
        } else
        {
            fprintf(stdout,"null\n");
        }
    }
    return TRUE;
}

void SolDep::Resize()
{
//funzt! mu� aber von der applikation aufgerufen werden.
    Point aOutPos = Point( 0, 0 );
	Size aOutSize = mpProcessWin->GetOutputSizePixel();
		// calculate output size
    ULONG nTaskHeight = maToolBox.CalcWindowSizePixel().Height();
    ULONG nTaskWidth  = maToolBox.CalcWindowSizePixel().Width();
	Size aSize( aOutSize.Width(), nTaskHeight );

//	ULONG nMenuHeight = 0;
    Point aGraphWinPos = Point(0,0);
    Size  aGraphWinSize = Size(0,0);

//wei� nicht wie:    nMenuHeight = aMenuBar.GetWindow()->GetSizePixel().Height(); //H�he des Menues

    //aInRect = pTBManager->Resize( Rectangle( aOutPos, aOutSize );
    // Set Docking-Rectangle for ToolBar
    Rectangle aInRect;

    if (( !maToolBox.IsFloatingMode() ) && ( maToolBox.GetAlign() == WINDOWALIGN_TOP ))
    {
        // waagerechte Toolbar oben
        maToolBox.SetPosSizePixel( aOutPos, Size( aOutSize.Width(), maToolBox.CalcWindowSizePixel().Height()));
        if( maToolBox.IsVisible())
        {
            Point aOutPosTmp;
            Size aOutSizeTmp;
            aOutPosTmp = Point( aOutPos.X(), aOutPos.Y() + maToolBox.CalcWindowSizePixel().Height());
            aOutSizeTmp = Size( aOutSize.Width(), aOutSize.Height() - maToolBox.CalcWindowSizePixel().Height());
            aInRect = Rectangle( aOutPosTmp, aOutSizeTmp );
            aGraphWinPos = Point( 0, nTaskHeight );
            aGraphWinSize = Size( aOutSize.Width(), aOutSize.Height() - nTaskHeight);
        }
    }
    if (( !maToolBox.IsFloatingMode() ) && ( maToolBox.GetAlign() == WINDOWALIGN_BOTTOM ))
    {
        // waagerechte Toolbar unten
        Point aTbPos  = Point( aOutPos.X(), aOutPos.Y() + aOutSize.Height() - maToolBox.CalcWindowSizePixel().Height());
        Size  aTbSize = Size( aOutSize.Width(), maToolBox.CalcWindowSizePixel().Height());
        maToolBox.SetPosSizePixel( aTbPos, aTbSize );
        if( maToolBox.IsVisible())
        {
            Point aOutPosTmp;
            Size aOutSizeTmp;
            aOutPosTmp = Point( aOutPos.X(), aOutPos.Y() + maToolBox.CalcWindowSizePixel().Height());
            aOutSizeTmp = Size( aOutSize.Width(), aOutSize.Height() - maToolBox.CalcWindowSizePixel().Height());
            aInRect = Rectangle( aOutPosTmp, aOutSizeTmp );
            aGraphWinPos = Point( 0, 0 );
            aGraphWinSize = Size( aOutSize.Width(), aOutSize.Height() - nTaskHeight);
        }
    }
    if (( !maToolBox.IsFloatingMode() ) && ( maToolBox.GetAlign() == WINDOWALIGN_LEFT ))
    {
        // senkrechte ToolBar links
        maToolBox.SetPosSizePixel( aOutPos, Size( maToolBox.CalcWindowSizePixel().Width(), aOutSize.Height()));
        if( maToolBox.IsVisible())
        {
            Point aOutPosTmp;
            Size aOutSizeTmp;
            aOutPosTmp = Point( aOutPos.X() + maToolBox.CalcWindowSizePixel().Width(), aOutPos.Y());
            aOutSizeTmp = Size( aOutSize.Width()- maToolBox.CalcWindowSizePixel().Width(), aOutSize.Height());
            aInRect = Rectangle( aOutPosTmp, aOutSizeTmp );
            aGraphWinPos = Point( nTaskWidth, 0 );
            aGraphWinSize = Size( aOutSize.Width() - nTaskWidth, aOutSize.Height());
        }
    }
    if (( !maToolBox.IsFloatingMode() ) && ( maToolBox.GetAlign() == WINDOWALIGN_RIGHT ))
    {
        // senkrechte ToolBar rechts
        Point aTbPos = Point( aOutPos.X() + aOutSize.Width() - maToolBox.CalcWindowSizePixel().Width(), aOutPos.Y());
        Size  aTbSize= Size( maToolBox.CalcWindowSizePixel().Width(), aOutSize.Height());
        maToolBox.SetPosSizePixel( aTbPos, aTbSize);
        if( maToolBox.IsVisible())
        {
            Point aOutPosTmp;
            Size aOutSizeTmp;
            aOutPosTmp = Point( aOutPos.X() + maToolBox.CalcWindowSizePixel().Width(), aOutPos.Y());
            aOutSizeTmp = Size( aOutSize.Width()- maToolBox.CalcWindowSizePixel().Width(), aOutSize.Height());
            aInRect = Rectangle( aOutPosTmp, aOutSizeTmp );
            aGraphWinPos = Point( 0, 0 );
            aGraphWinSize = Size( aOutSize.Width() - nTaskWidth, aOutSize.Height());
        }
    }

	Rectangle rout = Rectangle( Point( 0,0 ), aOutSize ); //OutputToScreenPixel( aOutPos )
    Rectangle rin  = Rectangle( Point( 0,0 ),//OutputToScreenPixel( Point( aOutPos.X() - 20, aInRect.Top())
                Size( aOutSize.Width(), aOutSize.Height()));
/*
	Rectangle rout = mpProcessWin->OutputToScreenPixel( aOutPos );
    Rectangle rin  = Rectangle( Point( 0,0 ),//OutputToScreenPixel( Point( aOutPos.X() - 20, aInRect.Top())
                Size( aOutSize.Width(), aOutSize.Height()));
*/
    maToolBox.SetDockingRects( rout, rin );

    BOOL bFloating = maToolBox.IsFloatingMode();

    if ( bFloating )
    {
        GetGraphWin()->SetPosSizePixel(Point(0,0),aOutSize);
        //if (IsPrjView() && (mpPrjDep)) mpPrjDep->Resize();
        if (maToolBox.IsVisible()) maToolBox.Show();
    } else
    {
        GetGraphWin()->SetPosSizePixel( aGraphWinPos, aGraphWinSize );
    }
    if (maToolBox.IsVisible()) maToolBox.Show();
}

USHORT SolDep::AddConnectorPrjView( ObjectWin* pStartWin, ObjectWin* pEndWin )
{
//	DBG_ASSERT( FALSE , "not yet" );
	ByteString sEndName = pEndWin->GetBodyText();
	ByteString sStartName = pStartWin->GetBodyText();
	if ( sStartName != ByteString("null"))
	{
		CommandData* pEndData = mpPrj->GetDirectoryData( sEndName );
		SByteStringList* pDeps = pEndData->GetDependencies();
		if ( pDeps )
			pDeps->PutString( &sStartName );
		else
		{
			pDeps = new SByteStringList();
			pEndData->SetDependencies( pDeps );
			pDeps->PutString( &sStartName );
			pEndData->GetDependencies();
		}
	}
	return AddConnectorToObjects( pStartWin, pEndWin );
}

USHORT SolDep::RemoveConnectorPrjView( ObjectWin* pStartWin, ObjectWin* pEndWin )
{
	ByteString sEndName = pEndWin->GetBodyText();
	ByteString sStartName = pStartWin->GetBodyText();
	CommandData* pEndData = mpPrj->GetDirectoryData( sEndName );
	SByteStringList* pDeps = pEndData->GetDependencies();
	if ( pDeps )
	{
		ByteString* pString;
		ULONG nDepsCount = pDeps->Count();
		for ( ULONG j = nDepsCount; j > 0; j-- )
		{
			pString = pDeps->GetObject( j - 1 );
			if ( pString->GetToken( 0, '.') == sStartName )
				pDeps->Remove( pString );
		}
	}
	return RemoveConnectorFromObjects( pStartWin, pEndWin );
}

USHORT SolDep::AutoArrange( SolIdMapper* pIdMapper, ObjectList* pObjLst, ULONG nTopId, ULONG nBottmId, ULONG aObjID )
{
    AutoArrangeDlgStart();
	OptimizePos(pIdMapper, pObjLst, nTopId, nBottmId, aObjID );
    AutoArrangeDlgStop();
	return 0;
}

Point SolDep::CalcPos( USHORT nSet, USHORT nIndex )
{
	int nRowIndex = nIndex / DEPPER_MAX_WIDTH;
	ULONG nPosX = mnXOffset + nRowIndex % 3 * GetDefSize().Width() / 3 + ( nIndex - ( DEPPER_MAX_WIDTH * nRowIndex )) * (GetDefSize().Width() + OBJWIN_X_SPACING );

	ULONG nPosY = ( nSet + mnLevelOffset + nRowIndex ) * ( GetDefSize().Height() + OBJWIN_Y_SPACING ) + OBJWIN_Y_SPACING;
	Point aPos( nPosX, nPosY );
	return aPos;
}

ULONG SolDep::CalcXOffset( ULONG nObjectsToFit )
{
	long nDynXOffs;
	long nXMiddle;
	ULONG nTrigger;

	nXMiddle = GetDepWin()->PixelToLogic( GetDepWin()->GetSizePixel()).Width() / 2;
	if ( nObjectsToFit > DEPPER_MAX_WIDTH )
		nObjectsToFit = DEPPER_MAX_WIDTH - 1 + DEPPER_MAX_WIDTH % 2;
	nTrigger = ( nObjectsToFit - 1 ) / 2;
	nDynXOffs = ( GetDefSize().Width() + OBJWIN_X_SPACING ) * nTrigger;
	ULONG nXOffs = nXMiddle - nDynXOffs;

	if ( ULONG(nXMiddle - nDynXOffs) < mnMinDynXOffs )
		mnMinDynXOffs = nXMiddle - nDynXOffs;

	return nXOffs;

}

double SolDep::CalcDistSum( ObjWinList* pObjList, DistType eDistType )
{
	ObjectWin* pWin;
	Connector* pCon;
	ULONG nObjCount = pObjList->Count();
	double dRetVal = 0;
	double dWinVal;
	USHORT i, j;
	BOOL bIsStart;

	for ( i = 0; i < nObjCount; i++ )
	{
		pWin = pObjList->GetObject( i );

		if ( pWin && pWin->IsVisible())
		{
			j = 0;
			dWinVal = 0;
			while ( (pCon = pWin->GetConnector( j )) )
			{
				if ( pCon->IsVisible()) {
					bIsStart = pCon->IsStart( pWin );
					if ( eDistType != BOTH )
						if ( eDistType == TOPDOWN )
						{
							if ( bIsStart )
							{
								pCon->UpdatePosition( pWin, FALSE );
								dWinVal += pCon->GetLen() * pWin->mnHeadDist;
							}
						}
						else
						{
							if ( !bIsStart )
							{
								pCon->UpdatePosition( pWin, FALSE );
								dWinVal += pCon->GetLen() * pWin->mnRootDist;
							}

						}
					else
					{
						pCon->UpdatePosition( pWin, FALSE );
						if ( !bIsStart )
							dWinVal += pCon->GetLen() * ( pWin->mnHeadDist + 1 );
						else
							dWinVal += pCon->GetLen() * pWin->mnRootDist;
					}
				}
				j++;
			}
//			if ( j != 0 )
//				dWinVal /= j;
			dRetVal += dWinVal;
		}
	}

	return dRetVal;
}

USHORT SolDep::Impl_Traveller( ObjectWin* pWin, USHORT nDepth )
{
	USHORT i = 0;
	ObjectWin* pNewWin;
	Connector* pCon;

	nDepth++;

	USHORT nMaxDepth = nDepth;

	pWin->mbVisited = TRUE;
	pWin->mnRootDist = Max ( nDepth, pWin-> mnRootDist );
	if ( nDepth > DEPPER_MAX_DEPTH )
	{
		DBG_ASSERT( nDepth != DEPPER_MAX_DEPTH + 1, "Ringabh�ngigkeit!" );
		nDepth++;
		return DEP_ENDLES_RECURSION_FOUND;
	}

	while ( (pCon = pWin->GetConnector( i )) )
	{
		if ( pCon->IsStart( pWin )&& pCon->IsVisible() ) //removed: don't show null_project
		{
			pNewWin = pCon->GetOtherWin( pWin );
			nMaxDepth = Max( Impl_Traveller( pNewWin, nDepth ), nMaxDepth );
			if( nMaxDepth == DEP_ENDLES_RECURSION_FOUND )
			{
				mpTravellerList->Insert( pWin, LIST_APPEND );
				return DEP_ENDLES_RECURSION_FOUND;
			}
		}
		i++;
	}
	pWin->mnHeadDist = MAX( pWin->mnHeadDist, nMaxDepth - nDepth );
	return nMaxDepth;
}


double SolDep::Impl_PermuteMin( ObjWinList& rObjList, Point* pPosArray, ObjWinList& rResultList, double dMinDist, ULONG nStart, ULONG nSize, DistType eDistType )
{

	ULONG i, j, l;
	ULONG nEnd = nStart + nSize;
	ObjectWin* pSwapWin;
	ULONG nLevelObjCount = rObjList.Count();

//dont use full recusion for more than 6 objects
	if ( nLevelObjCount > 6 )
	{
		srand(( unsigned ) time( NULL ));

		ULONG nIdx1, nIdx2;
		for ( i = 0; i < 101; i++ )
		{
            UpdateSubProgrssBar(i);
			for ( j = 0; j < 100; j++ )
			{
				nIdx1 = (ULONG) ( double( rand() ) / RAND_MAX * nLevelObjCount );
				while ( rObjList.GetObject( nIdx1 ) == NULL )
					nIdx1 = (ULONG) ( double( rand() ) / RAND_MAX * nLevelObjCount );
				nIdx2 = (ULONG) ( double( rand() ) / RAND_MAX * nLevelObjCount );
				while ( nIdx1 == nIdx2 || nIdx2 == nLevelObjCount )
					nIdx2 = (ULONG) ( double( rand() ) / RAND_MAX * nLevelObjCount );

				pSwapWin = rObjList.GetObject( nIdx1 );
				if ( pSwapWin )
					pSwapWin->SetCalcPosPixel( pPosArray[ nIdx2 ] );
				pSwapWin = rObjList.Replace( pSwapWin, nIdx2 );
				if ( pSwapWin )
					pSwapWin->SetCalcPosPixel( pPosArray[ nIdx1 ] );
				rObjList.Replace( pSwapWin, nIdx1 );

				double dCurDist = CalcDistSum( &rObjList, eDistType );

				if ( dCurDist < dMinDist )
				{
					dMinDist = dCurDist;
					rResultList.Clear();
					for ( l = 0; l < nLevelObjCount; l++ )
					{
						pSwapWin = rObjList.GetObject( l );
						rResultList.Insert( pSwapWin, LIST_APPEND);
					}
				}
//				if ( dCurDist > dMinDist * 1.5 )
				if ( dCurDist > dMinDist * 15 )
				{
					pSwapWin = rObjList.GetObject( nIdx1 );
					if ( pSwapWin )
						pSwapWin->SetCalcPosPixel( pPosArray[ nIdx2 ] );
					pSwapWin = rObjList.Replace( pSwapWin, nIdx2 );
					if ( pSwapWin )
						pSwapWin->SetCalcPosPixel( pPosArray[ nIdx1 ] );
					rObjList.Replace( pSwapWin, nIdx1 );
				}
			}
		}
	}
	else
	{
		for ( i = nStart ; i < nEnd; i++)
		{
			if ( nSize > 1 )
			{
				pSwapWin = rObjList.GetObject( i );
				pSwapWin = rObjList.Replace( pSwapWin, nStart );
				rObjList.Replace( pSwapWin, i );
                double dPermuteDist = Impl_PermuteMin( rObjList, pPosArray, rResultList, dMinDist, nStart + 1, nSize - 1, eDistType );
				dMinDist = MIN( dMinDist, dPermuteDist);
				pSwapWin = rObjList.GetObject( i );
				pSwapWin = rObjList.Replace( pSwapWin, nStart );
				rObjList.Replace( pSwapWin, i );

			}
			else
			{
				for ( l = 0; l < nLevelObjCount; l++ )
				{
					pSwapWin = rObjList.GetObject( l );
					if ( pSwapWin )
						pSwapWin->SetCalcPosPixel( pPosArray[ l ] );
				}

				double dCurDist = CalcDistSum( &rObjList, eDistType );

				if ( dCurDist < dMinDist )
				{
					dMinDist = dCurDist;
					rResultList.Clear();
					for ( l = 0; l < nLevelObjCount; l++ )
					{
						pSwapWin = rObjList.GetObject( l );
						rResultList.Insert( pSwapWin, LIST_APPEND);
					}
				}

			}
		}
	}

	return dMinDist;
}


USHORT SolDep::OptimizePos(SolIdMapper* pIdMapper, ObjectList* pObjLst, ULONG nTopId, ULONG nBottmId, ULONG aObjID )
{
	ObjWinList aWorkList;
	ObjectWin* pWin;
	Connector* pCon;
	USHORT nRootDist = (USHORT) -1;
	USHORT i, j, k, l, nRetVal;
	USHORT LevelUse[ DEPPER_MAX_DEPTH ];
	USHORT LevelSecUse[ DEPPER_MAX_DEPTH ];
	ObjWinList* LevelList[ DEPPER_MAX_DEPTH ];
	ObjWinList* LevelSecList[ DEPPER_MAX_DEPTH ];
	Point aPosArray[ DEPPER_MAX_LEVEL_WIDTH * DEPPER_MAX_WIDTH ];

	mnMinDynXOffs = 0xffff;

	for ( i = 0; i < DEPPER_MAX_DEPTH; i++ )
	{
		LevelUse[ i ] = 0;
		LevelList[ i ] = NULL;
		LevelSecUse[ i ] = 0;
		LevelSecList[ i ] = NULL;
	}

    GetDepWin()->EnablePaint( FALSE );

	ULONG nObjCount = pObjLst->Count();
	for ( i = 0; i < nObjCount; i++ )
	{
		pWin = pObjLst->GetObject( i );
		if ( pWin->IsVisible()) {
			pWin->mbVisited = FALSE;
			pWin->mnHeadDist = 0;
			pWin->mnRootDist = 0;

			// find initial objects which need to be connected with
			// root object
			j = 0;
			USHORT nStartCount = 0;
			USHORT nEndCount = 0;
			while ( (pCon = pWin->GetConnector( j )) )
			{
				if ( pCon->IsVisible()) {                   //null_project
					if( pCon->IsStart( pWin ))
						nStartCount++;
					else
					{
						nEndCount = 1;
						break;
					}
				}
				j++;
			}

			if ( nStartCount > 0 && nEndCount == 0 )
				if ( nTopId != pWin->GetId())
					AddConnectorToObjects( pObjLst, nTopId, pWin->GetId());

		}
	}

	pWin = ObjIdToPtr( pObjLst, nTopId );

	if ( mpTravellerList )
	{
		mpTravellerList->Clear();
		delete mpTravellerList;
	}
	mpTravellerList = new ObjWinList();
	// set root and top distance
	nRetVal = Impl_Traveller( pWin, nRootDist );

	DBG_ASSERT( nRetVal < DEPPER_MAX_DEPTH , "zu tief" );
	if ( nRetVal == DEP_ENDLES_RECURSION_FOUND )
	{
        WriteToErrorFile();
		return nRetVal;
	}

	ULONG nUnvisited = 0;
	ULONG nUnvisYOffs = 0;

	// seperate mainstream, secondary and unconnected
	for ( i = 0; i < nObjCount; i++ )
	{
		pWin = pObjLst->GetObject( i );
		if ( pWin->IsVisible()) {
			if (( pWin->mnHeadDist + pWin->mnRootDist ) == nRetVal )
			{
				if ( !LevelList[ pWin->mnHeadDist ] )
						LevelList[ pWin->mnHeadDist ] = new ObjWinList;
    			LevelList[ pWin->mnHeadDist ]->Insert( pWin );
				LevelUse[ pWin->mnHeadDist ]++;
			}
			else
				if ( pWin->mbVisited )
				{
					if ( !LevelSecList[ nRetVal - pWin->mnRootDist ] )
						LevelSecList[ nRetVal - pWin->mnRootDist ] = new ObjWinList;
					LevelSecList[ nRetVal - pWin->mnRootDist ]->Insert( pWin );
					LevelSecUse[ nRetVal - pWin->mnRootDist ]++;
				}
				else
				{
	//				need to be arranged more intelligent...
					Point aPos( 5, nUnvisYOffs );
					pWin->SetCalcPosPixel( aPos );

					Point aTmpPos = pWin->GetCalcPosPixel();
					pWin->SetPosPixel( mpBaseWin->LogicToPixel( aTmpPos ));

					nUnvisYOffs += pWin->PixelToLogic( pWin->GetSizePixel()).Height();
					nUnvisited++;
				}
		}
	}

	mnLevelOffset = 0;

	USHORT nScaleVal;

	if ( nRetVal == 0 )
		nScaleVal = 1;
	else
		nScaleVal = nRetVal;

	i = 0;

	USHORT nStep = 0;

	while ( LevelList[ i ] )
	{
        UpdateMainProgressBar(i, nScaleVal, nStep);
		DBG_ASSERT( LevelUse[ i ] == LevelList[ i ]->Count() , "level index im a..." );
		ObjectWin* pSwapWin;
		ULONG nLevelObjCount = LevelList[ i ]->Count();

		if ( nLevelObjCount % 2 == 0 )
		{
			LevelList[ i ]->Insert( NULL, LIST_APPEND );
			nLevelObjCount++;
//			LevelUse bleibt orginal...
//			LevelUse[ i ]++;
		}

// catch too big lists
		DBG_ASSERT( nLevelObjCount < DEPPER_MAX_LEVEL_WIDTH * DEPPER_MAX_WIDTH , "graph zu breit! dat geiht nich gut. breaking" );
		if ( nLevelObjCount >= DEPPER_MAX_LEVEL_WIDTH * DEPPER_MAX_WIDTH )
		{
			WarningBox aWBox( mpBaseWin, WB_OK, String::CreateFromAscii("graph zu breit! dat geiht nich gut. breaking"));
			aWBox.Execute();
			break;
		}
		mnXOffset = CalcXOffset( nLevelObjCount );
		aWorkList.Clear();

		// initial positioning for mainstream
		for ( j = 0; j < nLevelObjCount; j++ )
		{
			pSwapWin = LevelList[ i ]->GetObject( j );
			aWorkList.Insert( pSwapWin, LIST_APPEND);
			Point aPos = CalcPos( i, j );
			aPosArray[ j ] = aPos;
			if ( pSwapWin )
				pSwapWin->SetCalcPosPixel( aPosArray[ j ] );
		}

		double dMinDist = CalcDistSum( LevelList[ i ] );

		// optimize mainstream order and return best matching list in "aWorkList"
		dMinDist = MIN( dMinDist, Impl_PermuteMin( *(LevelList[ i ]), aPosArray, aWorkList, dMinDist, 0, nLevelObjCount ));

		// set optimized positions - may still be wrong from later tries
		for ( j = 0; j < nLevelObjCount; j++ )
		{
			pSwapWin = aWorkList.GetObject( j );
			if ( pSwapWin )
				pSwapWin->SetCalcPosPixel(  aPosArray[ j ] );
		}

		if ( LevelSecList[ i ] != NULL )
		{
			ULONG nLevelSecObjCount = LevelSecList[ i ]->Count();
			// expand list for better positioning
			while ( nLevelSecObjCount + LevelUse[ i ] < DEPPER_MAX_WIDTH - 1 )
			{
				LevelSecList[ i ]->Insert( NULL, LIST_APPEND );
				nLevelSecObjCount++;
			}
			if ( ( nLevelSecObjCount + LevelUse[ i ])% 2 == 0 )
			{
				LevelSecList[ i ]->Insert( NULL, LIST_APPEND );
				nLevelSecObjCount++;
			}

			DBG_ASSERT( nLevelSecObjCount < DEPPER_MAX_LEVEL_WIDTH * DEPPER_MAX_WIDTH , "graph zu breit! dat geiht nich gut. breaking" );
			if ( nLevelObjCount >= DEPPER_MAX_LEVEL_WIDTH * DEPPER_MAX_WIDTH )
			{
				WarningBox aWBox( mpBaseWin, WB_OK, String::CreateFromAscii("graph zu breit! dat geiht nich gut. breaking"));
				aWBox.Execute();
				break;
			}
			mnXOffset = CalcXOffset( LevelUse[ i ] + nLevelSecObjCount );
			aWorkList.Clear();

			l = 0;
			BOOL bUsedPos;

			// find free positions for secondary objects
			for ( j = 0; j < ( LevelUse[ i ] + nLevelSecObjCount ) ; j++ )
			{
				Point aPos = CalcPos( i, j );
				bUsedPos = FALSE;
				// is already occupied?
				for ( k = 0; k < nLevelObjCount; k++ )
				{
					if ( LevelList[ i ]->GetObject( k ) )
						if ( aPos == LevelList[ i ]->GetObject( k )->GetCalcPosPixel() )
							bUsedPos = TRUE;
				}
				// if its free, add to pool
				if ( !bUsedPos )
				{
					aPosArray[ l ] = aPos;
					l++;
				}
			}

			// initial positioning for secodaries
			for ( j = 0 ; j < nLevelSecObjCount ; j++ )
			{
				pSwapWin = LevelSecList[ i ]->GetObject( j );
				aWorkList.Insert( pSwapWin, LIST_APPEND);
				if ( pSwapWin )
					pSwapWin->SetCalcPosPixel( aPosArray[ j ] );
			}
			dMinDist = CalcDistSum( LevelSecList[ i ] );

			dMinDist = MIN( dMinDist, Impl_PermuteMin( *(LevelSecList[ i ]), aPosArray, aWorkList, dMinDist, 0, nLevelSecObjCount ));

			// set optimized positions - may still be wrong from later tries
			for ( j = 0; j < nLevelSecObjCount; j++ )
			{
				pSwapWin = aWorkList.GetObject( j );
				if ( pSwapWin )
					pSwapWin->SetCalcPosPixel(  aPosArray[ j ] );
			}
			if ( LevelUse[ i ] + LevelSecUse[ i ] > DEPPER_MAX_WIDTH )
				mnLevelOffset++;
		}
		if ( LevelUse[ i ] + LevelSecUse[ i ] > DEPPER_MAX_WIDTH )
			mnLevelOffset+= ( LevelUse[ i ] + LevelSecUse[ i ] ) / DEPPER_MAX_WIDTH ;
		i++;
	}

	mnMinDynXOffs = 0xffff;

// and back again...
	// get better results form already preoptimized upper and lower rows

	do
	{
		i--;
        UpdateMainProgressBar(i, nScaleVal, nStep, TRUE); // TRUE ~ counting down
		if ( LevelUse[ i ] + LevelSecUse[ i ] > DEPPER_MAX_WIDTH )
			mnLevelOffset-= ( LevelUse[ i ] + LevelSecUse[ i ] ) / DEPPER_MAX_WIDTH ;
		ObjectWin* pSwapWin;
		ULONG nLevelObjCount = LevelList[ i ]->Count();
		mnXOffset = CalcXOffset( nLevelObjCount );
		aWorkList.Clear();

		for ( j = 0; j < nLevelObjCount; j++ )
		{
			pSwapWin = LevelList[ i ]->GetObject( j );
			aWorkList.Insert( pSwapWin, LIST_APPEND);
			Point aPos = CalcPos( i, j );
			aPosArray[ j ] = aPos;
//no need to do this stuff....... 	?????
			if ( pSwapWin )
				pSwapWin->SetCalcPosPixel( aPosArray[ j ] );
		}

		double dMinDist = CalcDistSum( LevelList[ i ], BOTH );

		dMinDist = MIN( dMinDist, Impl_PermuteMin( *(LevelList[ i ]), aPosArray, aWorkList, dMinDist, 0, nLevelObjCount, BOTH ));
// wrong position for remaping - keep old positions for comparing
		for ( j = 0; j < nLevelObjCount; j++ )
		{
			pSwapWin = aWorkList.GetObject( j );
			if ( pSwapWin )
//				pSwapWin->SetCalcPosPixel( mpBaseWin->LogicToPixel( aPosArray[ j ] ));
				pSwapWin->SetCalcPosPixel( aPosArray[ j ] );
		}

		if ( LevelSecList[ i ] != NULL )
		{
			ULONG nLevelSecObjCount = LevelSecList[ i ]->Count();
			mnXOffset = CalcXOffset( LevelUse[ i ] + nLevelSecObjCount );
			aWorkList.Clear();

			l = 0;
			BOOL bUsedPos;

			for ( j = 0; j < ( LevelUse[ i ] + nLevelSecObjCount ) ; j++ )
			{
				Point aPos = CalcPos( i, j );
				bUsedPos = FALSE;
// could be faster
				for ( k = 0; k < nLevelObjCount; k++ )
				{
					if ( LevelList[ i ]->GetObject( k ) )
						if ( aPos == LevelList[ i ]->GetObject( k )->GetCalcPosPixel() )
							bUsedPos = TRUE;
				}
				if ( !bUsedPos )
				{
					aPosArray[ l ] = aPos;
					l++;
				}
			}

			for ( j = 0 ; j < nLevelSecObjCount ; j++ )
			{
				pSwapWin = LevelSecList[ i ]->GetObject( j );
				aWorkList.Insert( pSwapWin, LIST_APPEND);
				if ( pSwapWin )
					pSwapWin->SetCalcPosPixel( aPosArray[ j ] );
			}
			dMinDist = CalcDistSum( LevelSecList[ i ], BOTH );

			dMinDist = MIN( dMinDist, Impl_PermuteMin( *(LevelSecList[ i ]), aPosArray, aWorkList, dMinDist, 0, nLevelSecObjCount, BOTH ));
// wrong position for remaping - keep old positions for comparing
			for ( j = 0; j < nLevelSecObjCount; j++ )
			{
				pSwapWin = aWorkList.GetObject( j );
				if ( pSwapWin )
					pSwapWin->SetCalcPosPixel( aPosArray[ j ] );
			}
		}
//		i--;
	} while ( i != 0 );
    SetMainProgressBar( 100 );

	ULONG nNewXSize = ( DEPPER_MAX_WIDTH + 1 )  * ( OBJWIN_X_SPACING + GetDefSize().Width() );

    //    ULONG aObjID = GetStart(pIdMapper, pObjLst) //hier mu� man switchen GetStart/GetPrjStart oder so

    ObjectWin* pObjWin = ObjIdToPtr( pObjLst, aObjID);

	ULONG nNewYSize = pObjWin->GetCalcPosPixel().Y() + GetDefSize().Height() + 2 * OBJWIN_Y_SPACING;
	if (( nUnvisYOffs + GetDefSize().Height()) > nNewYSize )
		nNewYSize = nUnvisYOffs + GetDefSize().Height();

	MapMode aMapMode = GetDepWin()->GetMapMode();
	Size aTmpSize( (ULONG) (double(nNewXSize) * double( aMapMode.GetScaleX())), (ULONG) (double( nNewYSize) * double( aMapMode.GetScaleY())));

	Size aNowSize( GetGraphWin()->GetSizePixel());

	if ( GetDepWin()->LogicToPixel( aNowSize ).Width() > aTmpSize.Width() )
		aTmpSize.Width() = GetDepWin()->LogicToPixel( aNowSize ).Width() ;

	if ( GetDepWin()->LogicToPixel( aNowSize ).Height()  > aTmpSize.Height() )
		aTmpSize.Height() = GetDepWin()->LogicToPixel( aNowSize ).Height() ;

//	if ( nZoomed <= 0 )
//	{
//		mpBaseWin->SetSizePixel( aTmpSize );
//		mpGraphWin->SetTotalSize( aTmpSize );
//		mpGraphWin->EndScroll( 0, 0 );
//	}

// now remap all objects
	ULONG nAllObjCount = pObjLst->Count();
	Point aTmpPos;
	for ( j = 0; j < nAllObjCount; j++ )
	{
		pWin = pObjLst->GetObject( j );
		if ( pWin->IsVisible()) {
			aTmpPos = pWin->GetCalcPosPixel();
			if ( pWin->mbVisited )
			{
// reserve space for unconnected
				aTmpPos.X() -= mnMinDynXOffs;
				aTmpPos.X() += GetDefSize().Width() + OBJWIN_X_SPACING;
// center window
				aTmpPos.X() += GetDefSize().Width() / 2;
				aTmpPos.X() -= pWin->PixelToLogic( pWin->GetSizePixel()).Width() / 2 ;
			}
			pWin->SetPosPixel( GetDepWin()->LogicToPixel( aTmpPos ));
		}
	}
	aWorkList.Clear();
	GetDepWin()->EnablePaint( TRUE );
	GetDepWin()->Invalidate();
//LevelListen loeschen	                H�? Welche Levellisten?

//Update all Connectors
// --> To be done: Don't call twice Object1-Connector-Object2
	ObjectWin* pObject1;
	for ( i = 0 ; i < nObjCount ; i++)
	{
	    pObject1 = pObjLst->GetObject( i );
		if ( pObject1->IsVisible())
			pObject1->UpdateConnectors();
	};
	return 0;
}

void SolDep::WriteToErrorFile()
{
//Needs some improvement
    ObjectWin* pWin;
    WarningBox aWBox( mpBaseWin, WB_OK, String::CreateFromAscii("graph too deep! dat geiht nich gut.\nlook at depper.err in your Tmp-directory\nfor list of objects"));
	aWBox.Execute();
	char *tmpdir = getenv("TMP");
	char *errfilebasename = "depper.err";
	char *ErrFileName = (char*) malloc( strlen( tmpdir ) + strlen( errfilebasename) + 3 );
	*ErrFileName = '\0';
	strcat( ErrFileName, tmpdir );
	strcat( ErrFileName, "\\" );
	strcat( ErrFileName, errfilebasename );
	FILE* pErrFile = fopen( "depper.err", "w+" );
	if ( pErrFile )
	{
		for ( USHORT i = 0; i < mpTravellerList->Count(); i++ )
		{
			pWin = mpTravellerList->GetObject( i );
			fprintf( pErrFile, " %s -> \n", (pWin->GetBodyText()).GetBuffer());
		}
		fclose( pErrFile );
	}
}
