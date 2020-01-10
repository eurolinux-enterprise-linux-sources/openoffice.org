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
#include "precompiled_basctl.hxx"

#include <memory>

#include "vcl/bitmap.hxx"

#include <ide_pch.hxx>


#include <basidesh.hrc>
#include <bastypes.hxx>
#include <bastype2.hxx>
#include <basobj.hxx>
#include <baside2.hrc>
#include <iderid.hxx>
#include <tools/urlobj.hxx>
#include <tools/diagnose_ex.h>
#include <basic/sbx.hxx>
#include <svtools/imagemgr.hxx>
#include <com/sun/star/script/XLibraryContainer.hpp>
#include <com/sun/star/script/XLibraryContainerPassword.hpp>
#include <com/sun/star/frame/XModuleManager.hpp>
#include <comphelper/processfactory.hxx>
#include <comphelper/componentcontext.hxx>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star;


BasicEntry::~BasicEntry()
{
}

BasicDocumentEntry::BasicDocumentEntry( const ScriptDocument& rDocument, LibraryLocation eLocation, BasicEntryType eType )
    :BasicEntry( eType )
    ,m_aDocument( rDocument )
    ,m_eLocation( eLocation )
{
    OSL_ENSURE( m_aDocument.isValid(), "BasicDocumentEntry::BasicDocumentEntry: illegal document!" );
}

BasicDocumentEntry::~BasicDocumentEntry()
{
}

BasicLibEntry::BasicLibEntry( const ScriptDocument& rDocument, LibraryLocation eLocation, const String& rLibName, BasicEntryType eType )
    :BasicDocumentEntry( rDocument, eLocation, eType )
    ,m_aLibName( rLibName )
{
}

BasicLibEntry::~BasicLibEntry()
{
}

BasicEntryDescriptor::BasicEntryDescriptor()
    :m_aDocument( ScriptDocument::getApplicationScriptDocument() )
    ,m_eLocation( LIBRARY_LOCATION_UNKNOWN )
    ,m_eType( OBJ_TYPE_UNKNOWN )
{
}

BasicEntryDescriptor::BasicEntryDescriptor( const ScriptDocument& rDocument, LibraryLocation eLocation, const String& rLibName, const String& rName, BasicEntryType eType )
    :m_aDocument( rDocument )
    ,m_eLocation( eLocation )
    ,m_aLibName( rLibName )
    ,m_aName( rName )
    ,m_eType( eType )
{
    OSL_ENSURE( m_aDocument.isValid(), "BasicEntryDescriptor::BasicEntryDescriptor: invalid document!" );
}

BasicEntryDescriptor::BasicEntryDescriptor( const ScriptDocument& rDocument, LibraryLocation eLocation, const String& rLibName, const String& rName, const String& rMethodName, BasicEntryType eType )
    :m_aDocument( rDocument )
    ,m_eLocation( eLocation )
    ,m_aLibName( rLibName )
    ,m_aName( rName )
    ,m_aMethodName( rMethodName )
    ,m_eType( eType )
{
    OSL_ENSURE( m_aDocument.isValid(), "BasicEntryDescriptor::BasicEntryDescriptor: invalid document!" );
}

BasicEntryDescriptor::~BasicEntryDescriptor()
{
}

BasicEntryDescriptor::BasicEntryDescriptor( const BasicEntryDescriptor& rDesc )
    :m_aDocument( rDesc.m_aDocument )
    ,m_eLocation( rDesc.m_eLocation )
    ,m_aLibName( rDesc.m_aLibName )
    ,m_aName( rDesc.m_aName )
    ,m_aMethodName( rDesc.m_aMethodName )
    ,m_eType( rDesc.m_eType )
{
}

BasicEntryDescriptor& BasicEntryDescriptor::operator=( const BasicEntryDescriptor& rDesc )
{
    m_aDocument = rDesc.m_aDocument;
    m_eLocation = rDesc.m_eLocation;
    m_aLibName = rDesc.m_aLibName;
    m_aName = rDesc.m_aName;
    m_aMethodName = rDesc.m_aMethodName;
    m_eType = rDesc.m_eType;

    return *this;
}

bool BasicEntryDescriptor::operator==( const BasicEntryDescriptor& rDesc ) const
{
    return m_aDocument == rDesc.m_aDocument &&
           m_eLocation == rDesc.m_eLocation &&
           m_aLibName == rDesc.m_aLibName &&
           m_aName == rDesc.m_aName &&
           m_aMethodName == rDesc.m_aMethodName &&
           m_eType == rDesc.m_eType;
}

BasicTreeListBox::BasicTreeListBox( Window* pParent, const ResId& rRes ) :
	SvTreeListBox( pParent, IDEResId( sal::static_int_cast<USHORT>( rRes.GetId() ) ) ),
    m_aNotifier( *this )
{
    SetNodeDefaultImages();
    SetSelectionMode( SINGLE_SELECTION );
	nMode = 0xFF;	// Alles
}



BasicTreeListBox::~BasicTreeListBox()
{
    m_aNotifier.dispose();

	// UserDaten zerstoeren
	SvLBoxEntry* pEntry = First();
	while ( pEntry )
	{
		delete (BasicEntry*)pEntry->GetUserData();
		pEntry = Next( pEntry );
	}
}

void BasicTreeListBox::ScanEntry( const ScriptDocument& rDocument, LibraryLocation eLocation )
{
    OSL_ENSURE( rDocument.isAlive(), "BasicTreeListBox::ScanEntry: illegal document!" );
    if ( !rDocument.isAlive() )
        return;

    // can be called multiple times for updating!

	// eigentlich prueffen, ob Basic bereits im Baum ?!
	SetUpdateMode( FALSE );

    // level 1: BasicManager (application, document, ...)
    SvLBoxEntry* pDocumentRootEntry = FindRootEntry( rDocument, eLocation );
    if ( pDocumentRootEntry && IsExpanded( pDocumentRootEntry ) )
        ImpCreateLibEntries( pDocumentRootEntry, rDocument, eLocation );
    if ( !pDocumentRootEntry )
    {
        String aRootName( GetRootEntryName( rDocument, eLocation ) );
        Image aImage;
        Image aImageHC;
        GetRootEntryBitmaps( rDocument, aImage, aImageHC );
        pDocumentRootEntry = AddEntry(
            aRootName,
            aImage,
            aImageHC,
            0, true,
            std::auto_ptr< BasicEntry >( new BasicDocumentEntry( rDocument, eLocation ) ) );
    }

	SetUpdateMode( TRUE );
}

void BasicTreeListBox::ImpCreateLibEntries( SvLBoxEntry* pDocumentRootEntry, const ScriptDocument& rDocument, LibraryLocation eLocation )
{
    // get a sorted list of library names
    Sequence< ::rtl::OUString > aLibNames( rDocument.getLibraryNames() );
    sal_Int32 nLibCount = aLibNames.getLength();
	const ::rtl::OUString* pLibNames = aLibNames.getConstArray();

    for ( sal_Int32 i = 0 ; i < nLibCount ; i++ )
	{
        String aLibName = pLibNames[ i ];

        if ( eLocation == rDocument.getLibraryLocation( aLibName ) )
        {
            // check, if the module library is loaded
            BOOL bModLibLoaded = FALSE;
            ::rtl::OUString aOULibName( aLibName );
            Reference< script::XLibraryContainer > xModLibContainer( rDocument.getLibraryContainer( E_SCRIPTS ) );
		    if ( xModLibContainer.is() && xModLibContainer->hasByName( aOULibName ) && xModLibContainer->isLibraryLoaded( aOULibName ) ) 
                bModLibLoaded = TRUE;

            // check, if the dialog library is loaded
            BOOL bDlgLibLoaded = FALSE;
            Reference< script::XLibraryContainer > xDlgLibContainer( rDocument.getLibraryContainer( E_DIALOGS ) );
            if ( xDlgLibContainer.is() && xDlgLibContainer->hasByName( aOULibName ) && xDlgLibContainer->isLibraryLoaded( aOULibName ) ) 
                bDlgLibLoaded = TRUE;

            BOOL bLoaded = bModLibLoaded || bDlgLibLoaded;

            // if only one of the libraries is loaded, load also the other
            if ( bLoaded )
            {
                if ( xModLibContainer.is() && xModLibContainer->hasByName( aOULibName ) && !xModLibContainer->isLibraryLoaded( aOULibName ) )
                    xModLibContainer->loadLibrary( aOULibName );

                if ( xDlgLibContainer.is() && xDlgLibContainer->hasByName( aOULibName ) && !xDlgLibContainer->isLibraryLoaded( aOULibName ) )
                    xDlgLibContainer->loadLibrary( aOULibName );
            }

            // create tree list box entry
            USHORT nId, nIdHC;
            if ( ( nMode & BROWSEMODE_DIALOGS ) && !( nMode & BROWSEMODE_MODULES ) )
            {
                nId = bLoaded ? RID_IMG_DLGLIB : RID_IMG_DLGLIBNOTLOADED;
                nIdHC = bLoaded ? RID_IMG_DLGLIB_HC : RID_IMG_DLGLIBNOTLOADED_HC;
            }
            else
            {
                nId = bLoaded ? RID_IMG_MODLIB : RID_IMG_MODLIBNOTLOADED;
                nIdHC = bLoaded ? RID_IMG_MODLIB_HC : RID_IMG_MODLIBNOTLOADED_HC;
            }
		    SvLBoxEntry* pLibRootEntry = FindEntry( pDocumentRootEntry, aLibName, OBJ_TYPE_LIBRARY );
            if ( pLibRootEntry )
            {
                SetEntryBitmaps( pLibRootEntry, Image( IDEResId( nId ) ), Image( IDEResId( nIdHC ) ) );
                if ( IsExpanded( pLibRootEntry ) )
			        ImpCreateLibSubEntries( pLibRootEntry, rDocument, aLibName );
            }
            else
            {
                pLibRootEntry = AddEntry(
                    aLibName,                  
                    Image( IDEResId( nId ) ), 
                    Image( IDEResId( nIdHC ) ),
                    pDocumentRootEntry, true,
                    std::auto_ptr< BasicEntry >( new BasicEntry( OBJ_TYPE_LIBRARY ) ) );
            }
        }
    }
}

void BasicTreeListBox::ImpCreateLibSubEntries( SvLBoxEntry* pLibRootEntry, const ScriptDocument& rDocument, const String& rLibName )
{
    ::rtl::OUString aOULibName( rLibName );

	// modules
    if ( nMode & BROWSEMODE_MODULES )
    {
        Reference< script::XLibraryContainer > xModLibContainer( rDocument.getLibraryContainer( E_SCRIPTS ) );

        if ( xModLibContainer.is() && xModLibContainer->hasByName( aOULibName ) && xModLibContainer->isLibraryLoaded( aOULibName ) )
        {
            try
		    {
                // get a sorted list of module names
                Sequence< ::rtl::OUString > aModNames = rDocument.getObjectNames( E_SCRIPTS, rLibName );
                sal_Int32 nModCount = aModNames.getLength();
	            const ::rtl::OUString* pModNames = aModNames.getConstArray();

                for ( sal_Int32 i = 0 ; i < nModCount ; i++ )
				{
                    String aModName = pModNames[ i ];
                    SvLBoxEntry* pModuleEntry = FindEntry( pLibRootEntry, aModName, OBJ_TYPE_MODULE );
                    if ( !pModuleEntry )
                        pModuleEntry = AddEntry(
                            aModName,
                            Image( IDEResId( RID_IMG_MODULE ) ),
                            Image( IDEResId( RID_IMG_MODULE_HC ) ),
                            pLibRootEntry, false,
                            std::auto_ptr< BasicEntry >( new BasicEntry( OBJ_TYPE_MODULE ) ) );

					// methods
					if ( nMode & BROWSEMODE_SUBS )
					{
                        Sequence< ::rtl::OUString > aNames = BasicIDE::GetMethodNames( rDocument, rLibName, aModName );
						sal_Int32 nCount = aNames.getLength();
						const ::rtl::OUString* pNames = aNames.getConstArray();

						for ( sal_Int32 j = 0 ; j < nCount ; j++ )
						{
							String aName = pNames[ j ];
							SvLBoxEntry* pEntry = FindEntry( pModuleEntry, aName, OBJ_TYPE_METHOD );
                            if ( !pEntry )
                                pEntry = AddEntry(
                                    aName,
                                    Image( IDEResId( RID_IMG_MACRO ) ),
                                    Image( IDEResId( RID_IMG_MACRO_HC ) ),
                                    pModuleEntry, false,
                                    std::auto_ptr< BasicEntry >( new BasicEntry( OBJ_TYPE_METHOD ) ) );
						}
                    }
				}
            }
		    catch ( const container::NoSuchElementException& )
		    {
                DBG_UNHANDLED_EXCEPTION();
		    }
        }   
    }

	// dialogs
    if ( nMode & BROWSEMODE_DIALOGS )
    {
         Reference< script::XLibraryContainer > xDlgLibContainer( rDocument.getLibraryContainer( E_SCRIPTS ) );

         if ( xDlgLibContainer.is() && xDlgLibContainer->hasByName( aOULibName ) && xDlgLibContainer->isLibraryLoaded( aOULibName ) )
         {
            try
		    {
                // get a sorted list of dialog names
                Sequence< ::rtl::OUString > aDlgNames( rDocument.getObjectNames( E_DIALOGS, rLibName ) );
                sal_Int32 nDlgCount = aDlgNames.getLength();
	            const ::rtl::OUString* pDlgNames = aDlgNames.getConstArray();

                for ( sal_Int32 i = 0 ; i < nDlgCount ; i++ )
				{
					String aDlgName = pDlgNames[ i ];
					SvLBoxEntry* pDialogEntry = FindEntry( pLibRootEntry, aDlgName, OBJ_TYPE_DIALOG );
                    if ( !pDialogEntry )
                        pDialogEntry = AddEntry(
                            aDlgName,
                            Image( IDEResId( RID_IMG_DIALOG ) ),
                            Image( IDEResId( RID_IMG_DIALOG_HC ) ),
                            pLibRootEntry, false,
                            std::auto_ptr< BasicEntry >( new BasicEntry( OBJ_TYPE_DIALOG ) ) );
				}
            }
		    catch ( container::NoSuchElementException& )
		    {
			    DBG_UNHANDLED_EXCEPTION();
		    }
        }
    }
}

void BasicTreeListBox::onDocumentCreated( const ScriptDocument& /*_rDocument*/ )
{
    UpdateEntries();
}

void BasicTreeListBox::onDocumentOpened( const ScriptDocument& /*_rDocument*/ )
{
    UpdateEntries();
}

void BasicTreeListBox::onDocumentSave( const ScriptDocument& /*_rDocument*/ )
{
    // not interested in
}

void BasicTreeListBox::onDocumentSaveDone( const ScriptDocument& /*_rDocument*/ )
{
    // not interested in
}

void BasicTreeListBox::onDocumentSaveAs( const ScriptDocument& /*_rDocument*/ )
{
    // not interested in
}

void BasicTreeListBox::onDocumentSaveAsDone( const ScriptDocument& /*_rDocument*/ )
{
    UpdateEntries();
}

void BasicTreeListBox::onDocumentClosed( const ScriptDocument& /*_rDocument*/ )
{
    UpdateEntries();
}

void BasicTreeListBox::onDocumentTitleChanged( const ScriptDocument& /*_rDocument*/ )
{
    // not interested in
}

void BasicTreeListBox::onDocumentModeChanged( const ScriptDocument& /*_rDocument*/ )
{
    // not interested in
}

void BasicTreeListBox::UpdateEntries()
{
    BasicEntryDescriptor aCurDesc( GetEntryDescriptor( FirstSelected() ) );

	// Erstmal die vorhandenen Eintraege auf existens pruefen:
	SvLBoxEntry* pLastValid = 0;
	SvLBoxEntry* pEntry = First();
	while ( pEntry )
	{
		if ( IsValidEntry( pEntry ) )
			pLastValid = pEntry;
		else
		{
			delete (BasicEntry*)pEntry->GetUserData();
			GetModel()->Remove( pEntry );
		}
		pEntry = pLastValid ? Next( pLastValid ) : First();
	}

	// Jetzt ueber die Basics rennen und in die Zweige eintragen
	ScanAllEntries();

    SetCurrentEntry( aCurDesc );
}

SvLBoxEntry* __EXPORT BasicTreeListBox::CloneEntry( SvLBoxEntry* pSource )
{
	SvLBoxEntry* pNew = SvTreeListBox::CloneEntry( pSource );
	BasicEntry* pUser = (BasicEntry*)pSource->GetUserData();

	DBG_ASSERT( pUser, "User-Daten?!" );
    DBG_ASSERT( pUser->GetType() != OBJ_TYPE_DOCUMENT, "BasicTreeListBox::CloneEntry: document?!" );

	BasicEntry* pNewUser = new BasicEntry( *pUser );
	pNew->SetUserData( pNewUser );
	return pNew;
}

SvLBoxEntry* BasicTreeListBox::FindEntry( SvLBoxEntry* pParent, const String& rText, BasicEntryType eType )
{
	ULONG nRootPos = 0;
	SvLBoxEntry* pEntry = pParent ? FirstChild( pParent ) : GetEntry( nRootPos );
	while ( pEntry )
	{
		BasicEntry* pBasicEntry = (BasicEntry*)pEntry->GetUserData();
		DBG_ASSERT( pBasicEntry, "FindEntry: Kein BasicEntry ?!" );
		if ( ( pBasicEntry->GetType() == eType  ) && ( GetEntryText( pEntry ) == rText ) )
			return pEntry;

		pEntry = pParent ? NextSibling( pEntry ) : GetEntry( ++nRootPos );
	}
	return 0;
}

long BasicTreeListBox::ExpandingHdl()
{
	// Expanding oder Collaps?
	BOOL bOK = TRUE;
	if ( GetModel()->GetDepth( GetHdlEntry() ) == 1 )
	{
        SvLBoxEntry* pCurEntry = GetCurEntry();
        BasicEntryDescriptor aDesc( GetEntryDescriptor( pCurEntry ) );
        ScriptDocument aDocument( aDesc.GetDocument() );
        OSL_ENSURE( aDocument.isAlive(), "BasicTreeListBox::ExpandingHdl: no document, or document is dead!" );
        if ( aDocument.isAlive() )
        {
            String aLibName( aDesc.GetLibName() );
            String aName( aDesc.GetName() );
            String aMethodName( aDesc.GetMethodName() );

            if ( aLibName.Len() && !aName.Len() && !aMethodName.Len() )
		    {
                // check password, if library is password protected and not verified
                ::rtl::OUString aOULibName( aLibName );
                Reference< script::XLibraryContainer > xModLibContainer( aDocument.getLibraryContainer( E_SCRIPTS ) );
                if ( xModLibContainer.is() && xModLibContainer->hasByName( aOULibName ) )
                {                
                    Reference< script::XLibraryContainerPassword > xPasswd( xModLibContainer, UNO_QUERY );
                    if ( xPasswd.is() && xPasswd->isLibraryPasswordProtected( aOULibName ) && !xPasswd->isLibraryPasswordVerified( aOULibName ) )
                    {
                        String aPassword;
				        bOK = QueryPassword( xModLibContainer, aLibName, aPassword );
                    }
                }
            }
        }
	}
	return bOK;
}

BOOL BasicTreeListBox::IsEntryProtected( SvLBoxEntry* pEntry )
{
	BOOL bProtected = FALSE;
	if ( pEntry && ( GetModel()->GetDepth( pEntry ) == 1 ) )
	{
        BasicEntryDescriptor aDesc( GetEntryDescriptor( pEntry ) );
        ScriptDocument aDocument( aDesc.GetDocument() );
        OSL_ENSURE( aDocument.isAlive(), "BasicTreeListBox::IsEntryProtected: no document, or document is dead!" );
        if ( aDocument.isAlive() )
        {
            ::rtl::OUString aOULibName( aDesc.GetLibName() );
            Reference< script::XLibraryContainer > xModLibContainer( aDocument.getLibraryContainer( E_SCRIPTS ) );
            if ( xModLibContainer.is() && xModLibContainer->hasByName( aOULibName ) )
            {                
                Reference< script::XLibraryContainerPassword > xPasswd( xModLibContainer, UNO_QUERY );
                if ( xPasswd.is() && xPasswd->isLibraryPasswordProtected( aOULibName ) && !xPasswd->isLibraryPasswordVerified( aOULibName ) )
                {
			        bProtected = TRUE;
                }
            }
        }
    }
	return bProtected;
}

SvLBoxEntry* BasicTreeListBox::AddEntry( 
    const String& rText, const Image& rImage, const Image& rImageHC,
    SvLBoxEntry* pParent, bool bChildrenOnDemand, std::auto_ptr< BasicEntry > aUserData )
{
    SvLBoxEntry* p = InsertEntry(
        rText, rImage, rImage, pParent, bChildrenOnDemand, LIST_APPEND,
        aUserData.release() ); // XXX possible leak
    SetExpandedEntryBmp( p, rImageHC, BMP_COLOR_HIGHCONTRAST );
    SetCollapsedEntryBmp( p, rImageHC, BMP_COLOR_HIGHCONTRAST );
    return p;
}

void BasicTreeListBox::SetEntryBitmaps( SvLBoxEntry * pEntry, const Image& rImage, const Image& rImageHC )
{
    SetExpandedEntryBmp( pEntry, rImage, BMP_COLOR_NORMAL );
    SetCollapsedEntryBmp( pEntry, rImage, BMP_COLOR_NORMAL );
    SetExpandedEntryBmp( pEntry, rImageHC, BMP_COLOR_HIGHCONTRAST );
    SetCollapsedEntryBmp( pEntry, rImageHC, BMP_COLOR_HIGHCONTRAST );
}

LibraryType BasicTreeListBox::GetLibraryType() const
{
    LibraryType eType = LIBRARY_TYPE_ALL;
    if ( ( nMode & BROWSEMODE_MODULES ) && !( nMode & BROWSEMODE_DIALOGS ) )
        eType = LIBRARY_TYPE_MODULE;
    else if ( !( nMode & BROWSEMODE_MODULES ) && ( nMode & BROWSEMODE_DIALOGS ) )
        eType = LIBRARY_TYPE_DIALOG;
    return eType;
}

String BasicTreeListBox::GetRootEntryName( const ScriptDocument& rDocument, LibraryLocation eLocation ) const
{
    return rDocument.getTitle( eLocation, GetLibraryType() );
}

void BasicTreeListBox::GetRootEntryBitmaps( const ScriptDocument& rDocument, Image& rImage, Image& rImageHC )
{
    OSL_ENSURE( rDocument.isValid(), "BasicTreeListBox::GetRootEntryBitmaps: illegal document!" );
    if ( !rDocument.isValid() )
        return;

    if ( rDocument.isDocument() )
    {
        ::rtl::OUString sFactoryURL;
        ::comphelper::ComponentContext aContext( ::comphelper::getProcessServiceFactory() );
        Reference< ::com::sun::star::frame::XModuleManager > xModuleManager;
        if ( aContext.createComponent( "com.sun.star.frame.ModuleManager", xModuleManager ) )
        {
            try
            {
                ::rtl::OUString sModule( xModuleManager->identify( rDocument.getDocument() ) );
                Reference< container::XNameAccess > xModuleConfig( xModuleManager, UNO_QUERY );
                if ( xModuleConfig.is() )
                {
                    Sequence< beans::PropertyValue > aModuleDescr;
                    xModuleConfig->getByName( sModule ) >>= aModuleDescr;                
                    sal_Int32 nCount = aModuleDescr.getLength();
                    const beans::PropertyValue* pModuleDescr = aModuleDescr.getConstArray();
                    for ( sal_Int32 i = 0; i < nCount; ++i )
                    {
                        if ( pModuleDescr[ i ].Name.equalsAsciiL( 
                            RTL_CONSTASCII_STRINGPARAM( "ooSetupFactoryEmptyDocumentURL" ) ) )
                        {
                            pModuleDescr[ i ].Value >>= sFactoryURL;
                            break;
                        }
                    }
                }
            }
            catch( const Exception& )
            {
            	DBG_UNHANDLED_EXCEPTION();
            }
        }

        if ( sFactoryURL.getLength() )
        {
            rImage = SvFileInformationManager::GetFileImage( INetURLObject( sFactoryURL ), 
                FALSE /* small */, 
                FALSE /* normal */ );

            rImageHC = SvFileInformationManager::GetFileImage( INetURLObject( sFactoryURL ), 
                FALSE /* small */, 
                TRUE /* high contrast */ );
        }
        else
        {
            // default icon
            rImage = Image( IDEResId( RID_IMG_DOCUMENT ) );
            rImageHC = Image( IDEResId( RID_IMG_DOCUMENT_HC ) );
        }
    }
    else
    {
        rImage = Image( IDEResId( RID_IMG_INSTALLATION ) );
        rImageHC = Image( IDEResId( RID_IMG_INSTALLATION_HC ) );
    }
}

void BasicTreeListBox::SetCurrentEntry( BasicEntryDescriptor& rDesc )
{
    SvLBoxEntry* pCurEntry = 0;
    BasicEntryDescriptor aDesc( rDesc );
    if ( aDesc.GetType() == OBJ_TYPE_UNKNOWN )
    {
        aDesc = BasicEntryDescriptor(
            ScriptDocument::getApplicationScriptDocument(),
            LIBRARY_LOCATION_USER, String::CreateFromAscii( "Standard" ), 
            String::CreateFromAscii( "." ), OBJ_TYPE_UNKNOWN );
    }
    ScriptDocument aDocument( aDesc.GetDocument() );
    OSL_ENSURE( aDocument.isValid(), "BasicTreeListBox::SetCurrentEntry: invalid document!" );
    LibraryLocation eLocation( aDesc.GetLocation() );
    SvLBoxEntry* pRootEntry = FindRootEntry( aDocument, eLocation );
    if ( pRootEntry )
    {
        pCurEntry = pRootEntry;
        String aLibName( aDesc.GetLibName() );
        if ( aLibName.Len() )
        {
            Expand( pRootEntry );
            SvLBoxEntry* pLibEntry = FindEntry( pRootEntry, aLibName, OBJ_TYPE_LIBRARY );
            if ( pLibEntry )
            {
                pCurEntry = pLibEntry;
                String aName( aDesc.GetName() );
                if ( aName.Len() )
                {
                    Expand( pLibEntry );
                    BasicEntryType eType = OBJ_TYPE_MODULE;
                    if ( aDesc.GetType() == OBJ_TYPE_DIALOG )
                        eType = OBJ_TYPE_DIALOG;
                    SvLBoxEntry* pEntry = FindEntry( pLibEntry, aName, eType );
                    if ( pEntry )
                    {
                        pCurEntry = pEntry;
                        String aMethodName( aDesc.GetMethodName() );
                        if ( aMethodName.Len() )
                        {
                            Expand( pEntry );
                            SvLBoxEntry* pSubEntry = FindEntry( pEntry, aMethodName, OBJ_TYPE_METHOD );
                            if ( pSubEntry )
                            {
                                pCurEntry = pSubEntry;
                            }
                            else
                            {
                                pSubEntry = FirstChild( pEntry );
                                if ( pSubEntry )
                                    pCurEntry = pSubEntry;
                            }
                        }
                    }
                    else
                    {
                        pEntry = FirstChild( pLibEntry );
                        if ( pEntry )
                            pCurEntry = pEntry;
                    }
                }
            }
            else
            {
                pLibEntry = FirstChild( pRootEntry );
                if ( pLibEntry )
                    pCurEntry = pLibEntry;
            }
        }
    }
    else
    {
        pRootEntry = First();
        if ( pRootEntry )
            pCurEntry = pRootEntry;
    }

    SetCurEntry( pCurEntry );
}
