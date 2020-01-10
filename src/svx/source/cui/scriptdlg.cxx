/**********************************************************************
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

#include <memory>

#include <sfx2/objsh.hxx>
#include <vcl/svapp.hxx>
#include <vcl/msgbox.hxx>
#include <vos/mutex.hxx>

#include <svx/dialogs.hrc>
#include "scriptdlg.hrc"
#include "fmresids.hrc"
#include "scriptdlg.hxx"
#include <svx/dialmgr.hxx>
#include "selector.hxx"

#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/frame/XDesktop.hpp>
#include <com/sun/star/script/XInvocation.hpp>
#include <com/sun/star/script/provider/XScriptProviderSupplier.hpp>
#include <com/sun/star/script/provider/XScriptProvider.hpp>
#include <com/sun/star/script/browse/BrowseNodeTypes.hpp>
#include <com/sun/star/script/browse/XBrowseNodeFactory.hpp>
#include <com/sun/star/script/browse/BrowseNodeFactoryViewTypes.hpp>
#include <com/sun/star/script/provider/ScriptErrorRaisedException.hpp>
#include <com/sun/star/script/provider/ScriptExceptionRaisedException.hpp>
#include <com/sun/star/script/provider/ScriptFrameworkErrorType.hpp>
#include <com/sun/star/frame/XModuleManager.hpp>
#include <com/sun/star/script/XInvocation.hpp>
#include <com/sun/star/document/XEmbeddedScripts.hpp>

#include <cppuhelper/implbase1.hxx>
#include <comphelper/documentinfo.hxx>
#include <comphelper/uno3.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/broadcasthelper.hxx>
#include <comphelper/propertycontainer.hxx>
#include <comphelper/proparrhlp.hxx>

#include <basic/sbx.hxx>
#include <svtools/imagemgr.hxx>
#include <tools/urlobj.hxx>
#include <vector>
#include <algorithm>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::script;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::document;

void ShowErrorDialog( const Any& aException )
{
    SvxScriptErrorDialog* pDlg = new SvxScriptErrorDialog( NULL, aException );
    pDlg->Execute();
    delete pDlg;
}

SFTreeListBox::SFTreeListBox( Window* pParent, const ResId& rResId ) :
    SvTreeListBox( pParent, ResId( rResId.GetId(),*rResId.GetResMgr() ) ),
    m_hdImage(ResId(IMG_HARDDISK,*rResId.GetResMgr())),
    m_hdImage_hc(ResId(IMG_HARDDISK_HC,*rResId.GetResMgr())),
    m_libImage(ResId(IMG_LIB,*rResId.GetResMgr())),
    m_libImage_hc(ResId(IMG_LIB_HC,*rResId.GetResMgr())),
    m_macImage(ResId(IMG_MACRO,*rResId.GetResMgr())),
    m_macImage_hc(ResId(IMG_MACRO_HC,*rResId.GetResMgr())),
    m_docImage(ResId(IMG_DOCUMENT,*rResId.GetResMgr())),
    m_docImage_hc(ResId(IMG_DOCUMENT_HC,*rResId.GetResMgr())),
    m_sMyMacros(String(ResId(STR_MYMACROS,*rResId.GetResMgr()))),
    m_sProdMacros(String(ResId(STR_PRODMACROS,*rResId.GetResMgr())))
{
    FreeResource();
    SetSelectionMode( SINGLE_SELECTION );
    
    SetWindowBits( GetStyle() | WB_CLIPCHILDREN | WB_HSCROLL |
                   WB_HASBUTTONS | WB_HASBUTTONSATROOT | WB_HIDESELECTION |
                   WB_HASLINES | WB_HASLINESATROOT );
    SetNodeDefaultImages();
    
    nMode = 0xFF;    // Alles
}

SFTreeListBox::~SFTreeListBox()
{
    deleteAllTree();
}

void SFTreeListBox::delUserData( SvLBoxEntry* pEntry )
{
    if ( pEntry )
    {

        String text = GetEntryText( pEntry );
        SFEntry* pUserData = (SFEntry*)pEntry->GetUserData();
        if ( pUserData )
        {
            delete pUserData;
            // TBD seem to get a Select event on node that is remove ( below )
            // so need to be able to detect that this node is not to be 
            // processed in order to do this, setting userData to NULL ( must 
            // be a better way to do this )
            pUserData = 0;
            pEntry->SetUserData( pUserData );
        }
    }
}

void SFTreeListBox::deleteTree( SvLBoxEntry* pEntry )
{

    delUserData( pEntry );
    pEntry = FirstChild( pEntry );
    while ( pEntry )
    {
        SvLBoxEntry* pNextEntry = NextSibling( pEntry );
        deleteTree( pEntry );
        GetModel()->Remove( pEntry );    
        pEntry = pNextEntry;
    }
}

void SFTreeListBox::deleteAllTree()
{
    SvLBoxEntry* pEntry =  GetEntry( 0 );

    // TBD - below is a candidate for a destroyAllTrees method
    if ( pEntry )
    {
        while ( pEntry )
        {
            String text = GetEntryText( pEntry );
            SvLBoxEntry* pNextEntry = NextSibling( pEntry ) ;
            deleteTree( pEntry );
            GetModel()->Remove( pEntry );
            pEntry = pNextEntry;
        }
    }    
}

void SFTreeListBox::Init( const ::rtl::OUString& language  )
{
    SetUpdateMode( FALSE );

    deleteAllTree();

    Reference< browse::XBrowseNode > rootNode;
    Reference< XComponentContext > xCtx;

    Sequence< Reference< browse::XBrowseNode > > children;

    ::rtl::OUString userStr = ::rtl::OUString::createFromAscii("user");    
    ::rtl::OUString shareStr = ::rtl::OUString::createFromAscii("share");    

    ::rtl::OUString singleton = ::rtl::OUString::createFromAscii(
        "/singletons/com.sun.star.script.browse.theBrowseNodeFactory" );

    try
    {
        Reference < beans::XPropertySet > xProps(
            ::comphelper::getProcessServiceFactory(), UNO_QUERY_THROW );

        xCtx.set( xProps->getPropertyValue( rtl::OUString(
            RTL_CONSTASCII_USTRINGPARAM("DefaultContext" ))), UNO_QUERY_THROW );

        Reference< browse::XBrowseNodeFactory > xFac(
            xCtx->getValueByName( singleton ), UNO_QUERY_THROW );

        rootNode.set( xFac->createView(
            browse::BrowseNodeFactoryViewTypes::MACROORGANIZER ) );

    	if (  rootNode.is() && rootNode->hasChildNodes() == sal_True )
        {            
            children = rootNode->getChildNodes();
        }
    }
    catch( Exception& e )
    {
        OSL_TRACE("Exception getting root browse node from factory: %s",
            ::rtl::OUStringToOString(
                e.Message , RTL_TEXTENCODING_ASCII_US ).pData->buffer );
        // TODO exception handling
    }        

	Reference<XModel> xDocumentModel;
    for ( sal_Int32 n = 0; n < children.getLength(); n++ )
    {
        bool app = false;
        ::rtl::OUString uiName = children[ n ]->getName();
        ::rtl::OUString factoryURL;
        if ( uiName.equals( userStr ) || uiName.equals( shareStr ) )
        {
            app = true;
            if ( uiName.equals( userStr ) )
            {
                uiName = m_sMyMacros;
            }
            else
            {
                uiName = m_sProdMacros;
            }
        }
        else
        {
            xDocumentModel.set(getDocumentModel(xCtx, uiName ), UNO_QUERY);

            if ( xDocumentModel.is() )
            {
                Reference< ::com::sun::star::frame::XModuleManager >
                    xModuleManager( xCtx->getServiceManager()->createInstanceWithContext(
                        ::rtl::OUString::createFromAscii("com.sun.star.frame.ModuleManager"), xCtx ),
                                    UNO_QUERY_THROW );

                Reference<container::XNameAccess> xModuleConfig(
                    xModuleManager, UNO_QUERY_THROW );
                // get the long name of the document:
                Sequence<beans::PropertyValue> moduleDescr;
                try{
                    ::rtl::OUString appModule = xModuleManager->identify( xDocumentModel );
                    xModuleConfig->getByName(appModule) >>= moduleDescr; 
                } catch(const uno::Exception&)
                    {}

                beans::PropertyValue const * pmoduleDescr =
                    moduleDescr.getConstArray();
                for ( sal_Int32 pos = moduleDescr.getLength(); pos--; )
                {
                    if (pmoduleDescr[ pos ].Name.equalsAsciiL(
                            RTL_CONSTASCII_STRINGPARAM(
                                "ooSetupFactoryEmptyDocumentURL") ))
                    {
                        pmoduleDescr[ pos ].Value >>= factoryURL;
                        break;
                    }
                }
            }
        }

        ::rtl::OUString lang( language );
        Reference< browse::XBrowseNode > langEntries = 
            getLangNodeFromRootNode( children[ n ], lang );

        /*SvLBoxEntry* pBasicManagerRootEntry =*/
            insertEntry( uiName, app ? IMG_HARDDISK : IMG_DOCUMENT,
                0, true, std::auto_ptr< SFEntry >(new SFEntry( OBJTYPE_SFROOT, langEntries, xDocumentModel )), factoryURL );    
    }

    SetUpdateMode( TRUE );
}

Reference< XInterface  > 
SFTreeListBox::getDocumentModel( Reference< XComponentContext >& xCtx, ::rtl::OUString& docName )
{
    Reference< XInterface > xModel;
    Reference< lang::XMultiComponentFactory > mcf =
            xCtx->getServiceManager();
    Reference< frame::XDesktop > desktop (
        mcf->createInstanceWithContext(
            ::rtl::OUString::createFromAscii("com.sun.star.frame.Desktop"),                 xCtx ),
            UNO_QUERY );

    Reference< container::XEnumerationAccess > componentsAccess =
        desktop->getComponents();
    Reference< container::XEnumeration > components =
        componentsAccess->createEnumeration();
    while (components->hasMoreElements())
    {
        Reference< frame::XModel > model(
            components->nextElement(), UNO_QUERY );
        if ( model.is() )
        {
			::rtl::OUString sTdocUrl = ::comphelper::DocumentInfo::getDocumentTitle( model );
            if( sTdocUrl.equals( docName ) )
            {
                xModel = model;
                break;
            }
        }
    }
    return xModel;
}

Reference< browse::XBrowseNode > 
SFTreeListBox::getLangNodeFromRootNode( Reference< browse::XBrowseNode >& rootNode, ::rtl::OUString& language )
{
    Reference< browse::XBrowseNode > langNode;

    try
    {
        Sequence < Reference< browse::XBrowseNode > > children = rootNode->getChildNodes();
        for ( sal_Int32 n = 0; n < children.getLength(); n++ )
        {
            if ( children[ n ]->getName().equals( language ) )
            {
                langNode = children[ n ];
                break;
            }    
        } 
    }
    catch ( Exception& )
    {
        // if getChildNodes() throws an exception we just return
        // the empty Reference
    }
    return langNode;
}

void SFTreeListBox:: RequestSubEntries( SvLBoxEntry* pRootEntry, Reference< ::com::sun::star::script::browse::XBrowseNode >& node,
									   Reference< XModel >& model )
{
    if (! node.is() )
    {
        return;
    }

    Sequence< Reference< browse::XBrowseNode > > children;
    try
    {
        children = node->getChildNodes();
    }
    catch ( Exception& )
    {
        // if we catch an exception in getChildNodes then no entries are added
    }
	
    for ( sal_Int32 n = 0; n < children.getLength(); n++ )
    {
		::rtl::OUString name( children[ n ]->getName() );
        if (  children[ n ]->getType() !=  browse::BrowseNodeTypes::SCRIPT)
        {
            insertEntry( name, IMG_LIB, pRootEntry, true, std::auto_ptr< SFEntry >(new SFEntry( OBJTYPE_SCRIPTCONTAINER, children[ n ],model ))); 
        }
        else
        {
            if ( children[ n ]->getType() == browse::BrowseNodeTypes::SCRIPT )
            {
                insertEntry( name, IMG_MACRO, pRootEntry, false, std::auto_ptr< SFEntry >(new SFEntry( OBJTYPE_METHOD, children[ n ],model ))); 
                
            }
        }
    }
}

void SFTreeListBox::UpdateEntries()
{
}

SvLBoxEntry* SFTreeListBox::FindEntry( SvLBoxEntry* , const String& , BYTE  )
{
    return 0;
}

long SFTreeListBox::ExpandingHdl()
{
    return TRUE;
}

void SFTreeListBox::ExpandAllTrees()
{
}

SvLBoxEntry * SFTreeListBox::insertEntry(
    String const & rText, USHORT nBitmap, SvLBoxEntry * pParent,
    bool bChildrenOnDemand, std::auto_ptr< SFEntry > aUserData, ::rtl::OUString factoryURL )
{
    SvLBoxEntry * p;
    if( nBitmap == IMG_DOCUMENT && factoryURL.getLength() > 0 )
    {
        Image aImage = SvFileInformationManager::GetFileImage(
            INetURLObject(factoryURL), false,
            BMP_COLOR_NORMAL );
        Image aHCImage = SvFileInformationManager::GetFileImage(
            INetURLObject(factoryURL), false,
            BMP_COLOR_HIGHCONTRAST );
        p = InsertEntry(
            rText, aImage, aImage, pParent, bChildrenOnDemand, LIST_APPEND,
            aUserData.release()); // XXX possible leak
        SetExpandedEntryBmp(p, aHCImage, BMP_COLOR_HIGHCONTRAST);
        SetCollapsedEntryBmp(p, aHCImage, BMP_COLOR_HIGHCONTRAST);
    }
    else
    {
        p = insertEntry( rText, nBitmap, pParent, bChildrenOnDemand, aUserData );
    }
    return p;
}

SvLBoxEntry * SFTreeListBox::insertEntry(
    String const & rText, USHORT nBitmap, SvLBoxEntry * pParent,
    bool bChildrenOnDemand, std::auto_ptr< SFEntry > aUserData )
{
    Image aHCImage, aImage;
    if( nBitmap == IMG_HARDDISK )
    {
        aImage = m_hdImage;
        aHCImage = m_hdImage_hc;
    } 
    else if( nBitmap == IMG_LIB )
    {
        aImage = m_libImage;
        aHCImage = m_libImage_hc;
    }
    else if( nBitmap == IMG_MACRO )
    {
        aImage = m_macImage;
        aHCImage = m_macImage_hc;
    }
    else if( nBitmap == IMG_DOCUMENT )
    {
        aImage = m_docImage;
        aHCImage = m_docImage_hc;
    }
    SvLBoxEntry * p = InsertEntry(
        rText, aImage, aImage, pParent, bChildrenOnDemand, LIST_APPEND,
        aUserData.release()); // XXX possible leak
    SetExpandedEntryBmp(p, aHCImage, BMP_COLOR_HIGHCONTRAST);
    SetCollapsedEntryBmp(p, aHCImage, BMP_COLOR_HIGHCONTRAST);
    return p;
}

void __EXPORT SFTreeListBox::RequestingChilds( SvLBoxEntry* pEntry )
{
    SFEntry* userData = 0;
    if ( !pEntry )
    {
        return;
    }
    userData = (SFEntry*)pEntry->GetUserData();

    Reference< browse::XBrowseNode > node;
	Reference< XModel > model;
    if ( userData && !userData->isLoaded() )
    {
        node = userData->GetNode();
		model = userData->GetModel();
        RequestSubEntries( pEntry, node, model );    
        userData->setLoaded();
    }
}

void __EXPORT SFTreeListBox::ExpandedHdl()
{
/*        SvLBoxEntry* pEntry = GetHdlEntry();
        DBG_ASSERT( pEntry, "Was wurde zugeklappt?" );

        if ( !IsExpanded( pEntry ) && pEntry->HasChildsOnDemand() )
        {
                SvLBoxEntry* pChild = FirstChild( pEntry );
                while ( pChild )
                {
                        GetModel()->Remove( pChild );   // Ruft auch den DTOR
                        pChild = FirstChild( pEntry );
                }
        }*/
}

// ----------------------------------------------------------------------------
// InputDialog ------------------------------------------------------------
// ----------------------------------------------------------------------------
InputDialog::InputDialog(Window * pParent, USHORT nMode )
    : ModalDialog( pParent, SVX_RES( RID_DLG_NEWLIB ) ),
        aText( this, SVX_RES( FT_NEWLIB ) ),
        aEdit( this, SVX_RES( ED_LIBNAME ) ),
        aOKButton( this, SVX_RES( PB_OK ) ),
        aCancelButton( this, SVX_RES( PB_CANCEL ) )
{
    aEdit.GrabFocus();
    if ( nMode == INPUTMODE_NEWLIB )
    {
        SetText( String( SVX_RES( STR_NEWLIB ) ) );
    }
    else if ( nMode == INPUTMODE_NEWMACRO )
    {
        SetText( String( SVX_RES( STR_NEWMACRO ) ) );
        aText.SetText( String( SVX_RES( STR_FT_NEWMACRO ) ) );
    }
    else if ( nMode == INPUTMODE_RENAME )
    {
        SetText( String( SVX_RES( STR_RENAME ) ) );
        aText.SetText( String( SVX_RES( STR_FT_RENAME ) ) );
    }
    FreeResource();

    // some resizing so that the text fits
    Point point, newPoint;
    Size siz, newSiz;
    long gap;

    USHORT style = TEXT_DRAW_MULTILINE | TEXT_DRAW_TOP |
                   TEXT_DRAW_LEFT | TEXT_DRAW_WORDBREAK;

    // get dimensions of dialog instructions control
    point = aText.GetPosPixel();
    siz = aText.GetSizePixel();

    // get dimensions occupied by text in the control
    Rectangle rect =
        GetTextRect( Rectangle( point, siz ), aText.GetText(), style );
    newSiz = rect.GetSize();

    // the gap is the difference between the text width and its control width
    gap = siz.Height() - newSiz.Height();

    //resize the text field
    newSiz = Size( siz.Width(), siz.Height() - gap );
    aText.SetSizePixel( newSiz );

    //move the OK & cancel buttons
    point = aEdit.GetPosPixel();
    newPoint = Point( point.X(), point.Y() - gap );
    aEdit.SetPosPixel( newPoint );

}

InputDialog::~InputDialog()
{
}
// ----------------------------------------------------------------------------
// ScriptOrgDialog ------------------------------------------------------------
// ----------------------------------------------------------------------------
SvxScriptOrgDialog::SvxScriptOrgDialog( Window* pParent, ::rtl::OUString language )
    :    SfxModalDialog( pParent, SVX_RES( RID_DLG_SCRIPTORGANIZER ) ),
        aScriptsTxt( this, SVX_RES( SF_TXT_SCRIPTS ) ),
        aScriptsBox( this, SVX_RES( SF_CTRL_SCRIPTSBOX ) ),
        aRunButton(    this, SVX_RES( SF_PB_RUN ) ),
        aCloseButton( this, SVX_RES( SF_PB_CLOSE ) ),
        aCreateButton( this, SVX_RES( SF_PB_CREATE ) ),
        aEditButton( this, SVX_RES( SF_PB_EDIT ) ),
        aRenameButton(this, SVX_RES( SF_PB_RENAME ) ),
        aDelButton(    this, SVX_RES( SF_PB_DEL ) ),
        aHelpButton( this, SVX_RES( SF_PB_HELP ) ),
        m_sLanguage( language ),
        m_delErrStr( SVX_RES( RID_SVXSTR_DELFAILED ) ),
        m_delErrTitleStr( SVX_RES( RID_SVXSTR_DELFAILED_TITLE ) ),
        m_delQueryStr( SVX_RES( RID_SVXSTR_DELQUERY ) ),
        m_delQueryTitleStr( SVX_RES( RID_SVXSTR_DELQUERY_TITLE ) ) ,
        m_createErrStr( SVX_RES ( RID_SVXSTR_CREATEFAILED ) ),
        m_createDupStr( SVX_RES ( RID_SVXSTR_CREATEFAILEDDUP ) ),
        m_createErrTitleStr( SVX_RES( RID_SVXSTR_CREATEFAILED_TITLE ) ),
        m_renameErrStr( SVX_RES ( RID_SVXSTR_RENAMEFAILED ) ),
        m_renameErrTitleStr( SVX_RES( RID_SVXSTR_RENAMEFAILED_TITLE ) ) 
{

    // must be a neater way to deal with the strings than as above
    // append the language to the dialog title
    String winTitle( GetText() );
    winTitle.SearchAndReplace( String::CreateFromAscii( "%MACROLANG" ), language.pData->buffer );
    SetText( winTitle );

    aScriptsBox.SetSelectHdl( LINK( this, SvxScriptOrgDialog, ScriptSelectHdl ) );
    aRunButton.SetClickHdl( LINK( this, SvxScriptOrgDialog, ButtonHdl ) );
    aCloseButton.SetClickHdl( LINK( this, SvxScriptOrgDialog, ButtonHdl ) );
    aRenameButton.SetClickHdl( LINK( this, SvxScriptOrgDialog, ButtonHdl ) );
    aEditButton.SetClickHdl( LINK( this, SvxScriptOrgDialog, ButtonHdl ) );
    aDelButton.SetClickHdl( LINK( this, SvxScriptOrgDialog, ButtonHdl ) );
    aCreateButton.SetClickHdl( LINK( this, SvxScriptOrgDialog, ButtonHdl ) );

    aRunButton.Disable();
    aRenameButton.Disable();
    aEditButton.Disable();
    aDelButton.Disable();
    aCreateButton.Disable();

    aScriptsBox.Init( m_sLanguage );
    RestorePreviousSelection();
    FreeResource();
}

__EXPORT SvxScriptOrgDialog::~SvxScriptOrgDialog()
{
    // clear the SelectHdl so that it isn't called during the dtor
    aScriptsBox.SetSelectHdl( Link() );
};

short SvxScriptOrgDialog::Execute()
{

    SfxObjectShell *pDoc = SfxObjectShell::GetFirst();

    // force load of MSPs for all documents    
    while ( pDoc )    
    {
        Reference< provider::XScriptProviderSupplier > xSPS = 
            Reference< provider::XScriptProviderSupplier >
                                        ( pDoc->GetModel(), UNO_QUERY );
        if ( xSPS.is() )
        {
            Reference< provider::XScriptProvider > ScriptProvider = 
            xSPS->getScriptProvider();
        }
            
        pDoc = SfxObjectShell::GetNext(*pDoc);
    }
    aScriptsBox.ExpandAllTrees();
    
    Window* pPrevDlgParent = Application::GetDefDialogParent();
    Application::SetDefDialogParent( this );
    short nRet = ModalDialog::Execute();
    Application::SetDefDialogParent( pPrevDlgParent );
    return nRet;
}

void SvxScriptOrgDialog::EnableButton( Button& , BOOL  )
{
}

void SvxScriptOrgDialog::CheckButtons( Reference< browse::XBrowseNode >& node )
{
    if ( node.is() )
    {
        if ( node->getType() == browse::BrowseNodeTypes::SCRIPT)
        {
            aRunButton.Enable(); 
        }
        else
        {
            aRunButton.Disable(); 
        }
        Reference< beans::XPropertySet > xProps( node, UNO_QUERY );
    
        if ( !xProps.is() )
        {
            aEditButton.Disable();
            aDelButton.Disable();
            aCreateButton.Disable();
            aRunButton.Disable(); 
            return;
        }

        ::rtl::OUString sName;
        sName = String::CreateFromAscii("Editable")  ;

        if ( getBoolProperty( xProps, sName ) )
        {
            aEditButton.Enable();
        }
        else
        {
            aEditButton.Disable();
        }

        sName = String::CreateFromAscii("Deletable")  ;

        if ( getBoolProperty( xProps, sName ) )
        {
            aDelButton.Enable();
        }
        else
        {
            aDelButton.Disable();
        }

        sName = String::CreateFromAscii("Creatable")  ;

        if ( getBoolProperty( xProps, sName ) )
        {
            aCreateButton.Enable();
        }
        else
        {
            aCreateButton.Disable();
        }

        sName = String::CreateFromAscii("Renamable")  ;

        if ( getBoolProperty( xProps, sName ) )
        {
            aRenameButton.Enable();
        }
        else
        {
            aRenameButton.Disable();
        }
    }    
    else
    {
        // no node info available, disable all configurable actions
        aDelButton.Disable();
        aCreateButton.Disable();
        aEditButton.Disable();
        aRunButton.Disable(); 
        aRenameButton.Disable();
    }
}

IMPL_LINK_INLINE_START( SvxScriptOrgDialog, MacroDoubleClickHdl, SvTreeListBox *, EMPTYARG )
{
    return 0;
}

IMPL_LINK_INLINE_END( SvxScriptOrgDialog, MacroDoubleClickHdl, SvTreeListBox *, EMPTYARG )

IMPL_LINK( SvxScriptOrgDialog, ScriptSelectHdl, SvTreeListBox *, pBox )
{
    if ( !pBox->IsSelected( pBox->GetHdlEntry() ) )
    {
        return 0;
    }

    SvLBoxEntry* pEntry = pBox->GetHdlEntry();

    SFEntry* userData = 0;
    if ( !pEntry )
    {
        return 0;
    }
    userData = (SFEntry*)pEntry->GetUserData();

    Reference< browse::XBrowseNode > node;
    if ( userData )
    {
              node = userData->GetNode();
        CheckButtons( node );
    }
    
    return 0;
}

IMPL_LINK( SvxScriptOrgDialog, ButtonHdl, Button *, pButton )
{
    if ( pButton == &aCloseButton )
    {
        StoreCurrentSelection();
        EndDialog( 0 );
    }
    if ( pButton == &aEditButton ||  
            pButton == &aCreateButton ||
            pButton == &aDelButton ||
            pButton == &aRunButton ||
            pButton == &aRenameButton )

    {
        if ( aScriptsBox.IsSelected( aScriptsBox.GetHdlEntry() ) )
        {
            SvLBoxEntry* pEntry = aScriptsBox.GetHdlEntry();
            SFEntry* userData = 0;
            if ( !pEntry )
            {
                return 0;
            }
            userData = (SFEntry*)pEntry->GetUserData();
            if ( userData )
            {
                Reference< browse::XBrowseNode > node;
				Reference< XModel > xModel;

                node = userData->GetNode();
				xModel = userData->GetModel();

                if ( !node.is() )
                {
                    return 0;
                }
                
				if ( pButton == &aRunButton )
                {
                    ::rtl::OUString tmpString;
                    Reference< beans::XPropertySet > xProp( node, UNO_QUERY );
                    Reference< provider::XScriptProvider > mspNode;
					if( !xProp.is() )
					{
						return 0;
					}

	                if ( xModel.is() )
					{
						Reference< XEmbeddedScripts >  xEmbeddedScripts( xModel, UNO_QUERY);
						if( !xEmbeddedScripts.is() )
						{
							return 0;
						}

						if (!xEmbeddedScripts->getAllowMacroExecution())
						{
							// Please FIXME: Show a message box if AllowMacroExecution is false
							return 0;
						}
					}


                    SvLBoxEntry* pParent = aScriptsBox.GetParent( pEntry );
                    while ( pParent && !mspNode.is() )
                    {
                        SFEntry* mspUserData = (SFEntry*)pParent->GetUserData();
                        mspNode.set( mspUserData->GetNode() , UNO_QUERY );
                        pParent = aScriptsBox.GetParent( pParent );
                    }
                    xProp->getPropertyValue( String::CreateFromAscii("URI" ) ) >>= tmpString;
                    const String scriptURL( tmpString );

                    if ( mspNode.is() )
                    {
                        try
                        {
                            Reference< provider::XScript > xScript(
                            mspNode->getScript( scriptURL ), UNO_QUERY_THROW );

                            const Sequence< Any > args(0);
                            Any aRet;
                            Sequence< sal_Int16 > outIndex;
                            Sequence< Any > outArgs( 0 );
                            aRet = xScript->invoke( args, outIndex, outArgs );
                        }
                        catch ( reflection::InvocationTargetException& ite )
                        {
                            ::com::sun::star::uno::Any a = makeAny(ite);
                            ShowErrorDialog(a);
                        }
                        catch ( provider::ScriptFrameworkErrorException& ite )
                        {
                            ::com::sun::star::uno::Any a = makeAny(ite);
                            ShowErrorDialog(a);
                        }
                        catch ( RuntimeException& re )
                        {
                            ::com::sun::star::uno::Any a = makeAny(re);
                            ShowErrorDialog(a);
                        }
                        catch ( Exception& e )
                        {
                            ::com::sun::star::uno::Any a = makeAny(e);
                            ShowErrorDialog(a);
                        }
                    }
                    StoreCurrentSelection();
                    EndDialog( 0 );
                }
                else if ( pButton == &aEditButton )
                {
                    Reference< script::XInvocation > xInv( node, UNO_QUERY );
                    if ( xInv.is() )
                    {
                        StoreCurrentSelection();
                        EndDialog( 0 );
                        Sequence< Any > args(0);
                        Sequence< Any > outArgs( 0 );
                        Sequence< sal_Int16 > outIndex;
                        try
                        {
                            // ISSUE need code to run script here
                            xInv->invoke( ::rtl::OUString::createFromAscii( "Editable" ), args, outIndex, outArgs );
                        }
                        catch( Exception& e )
                        {
                            OSL_TRACE("Caught exception trying to invoke %s", ::rtl::OUStringToOString( e.Message, RTL_TEXTENCODING_ASCII_US ).pData->buffer );

                        }
                    }
                }
                else if ( pButton == &aCreateButton )
                {
                    createEntry( pEntry );
                }
                else if ( pButton == &aDelButton )
                {
                    deleteEntry( pEntry );
                }
                else if ( pButton == &aRenameButton )
                {
                    renameEntry( pEntry );
                }
            }
        }            
    }
    return 0;
}

Reference< browse::XBrowseNode > SvxScriptOrgDialog::getBrowseNode( SvLBoxEntry* pEntry )
{
    Reference< browse::XBrowseNode > node;
    if ( pEntry )
    {
        SFEntry* userData = (SFEntry*)pEntry->GetUserData();        
        if ( userData )
        {
            node = userData->GetNode();
        }
    }

    return node;
}

Reference< XModel > SvxScriptOrgDialog::getModel( SvLBoxEntry* pEntry )
{
    Reference< XModel > model;
    if ( pEntry )
    {
        SFEntry* userData = (SFEntry*)pEntry->GetUserData();        
        if ( userData )
        {
            model = userData->GetModel();
        }
    }

    return model;
}

Reference< XInterface  > 
SvxScriptOrgDialog::getDocumentModel( Reference< XComponentContext >& xCtx, ::rtl::OUString& docName )
{
    Reference< XInterface > xModel;
    Reference< lang::XMultiComponentFactory > mcf =
            xCtx->getServiceManager();
    Reference< frame::XDesktop > desktop (
        mcf->createInstanceWithContext(
            ::rtl::OUString::createFromAscii("com.sun.star.frame.Desktop"), xCtx ),
            UNO_QUERY );

    Reference< container::XEnumerationAccess > componentsAccess =
        desktop->getComponents();
    Reference< container::XEnumeration > components =
        componentsAccess->createEnumeration();
    while (components->hasMoreElements())
    {
        Reference< frame::XModel > model(
            components->nextElement(), UNO_QUERY );
        if ( model.is() )
        {
			::rtl::OUString sTdocUrl = ::comphelper::DocumentInfo::getDocumentTitle( model );
            if( sTdocUrl.equals( docName ) )
            {
                xModel = model;
                break;
            }
        }
    }
    return xModel;
}

void SvxScriptOrgDialog::createEntry( SvLBoxEntry* pEntry )
{

    Reference< browse::XBrowseNode >  aChildNode;
    Reference< browse::XBrowseNode > node = getBrowseNode( pEntry );
    Reference< script::XInvocation > xInv( node, UNO_QUERY );
    
    if ( xInv.is() )
    {
        ::rtl::OUString aNewName;
        ::rtl::OUString aNewStdName;
        USHORT nMode = INPUTMODE_NEWLIB;
        if( aScriptsBox.GetModel()->GetDepth( pEntry ) == 0 ) 
        {
            aNewStdName = ::rtl::OUString::createFromAscii( "Library" ) ;
        }
        else
        {
            aNewStdName = ::rtl::OUString::createFromAscii( "Macro" ) ;
            nMode = INPUTMODE_NEWMACRO;
        }
        //do we need L10N for this? ie somethng like:
        //String aNewStdName( ResId( STR_STDMODULENAME ) );
        BOOL bValid = FALSE;
        USHORT i = 1;
        
        Sequence< Reference< browse::XBrowseNode > > childNodes;
        // no children => ok to create Parcel1 or Script1 without checking
        try
        {
            if( node->hasChildNodes() == sal_False )
            {
                aNewName = aNewStdName;
                aNewName += String::CreateFromInt32( i );
                bValid = TRUE;
            }
            else
            {
                childNodes = node->getChildNodes();
            }
        }
        catch ( Exception& )
        {
            // ignore, will continue on with empty sequence
        }

        ::rtl::OUString extn;
        while ( !bValid )
        {
            aNewName = aNewStdName;
            aNewName += String::CreateFromInt32( i );
            BOOL bFound = FALSE;
            if(childNodes.getLength() > 0 )
            {
                ::rtl::OUString nodeName = childNodes[0]->getName();
                sal_Int32 extnPos = nodeName.lastIndexOf( '.' );
                if(extnPos>0)
                    extn = nodeName.copy(extnPos);
            }
            for( sal_Int32 index = 0; index < childNodes.getLength(); index++ )
            {
                if ( (aNewName+extn).equals( childNodes[index]->getName() ) )
                {
                    bFound = TRUE;
                    break;
                }
            }
            if( bFound )
            {
                i++;
            }
            else
            {
                bValid = TRUE;
            }
        }

        std::auto_ptr< InputDialog > xNewDlg( new InputDialog( static_cast<Window*>(this), nMode ) );
        xNewDlg->SetObjectName( aNewName );

        do
        {
            if ( xNewDlg->Execute() && xNewDlg->GetObjectName().Len() )
            {
                ::rtl::OUString aUserSuppliedName = xNewDlg->GetObjectName();
                bValid = TRUE;
                for( sal_Int32 index = 0; index < childNodes.getLength(); index++ )
                {
                    if ( (aUserSuppliedName+extn).equals( childNodes[index]->getName() ) )
                    {
                        bValid = FALSE;
                        String aError( m_createErrStr );
                        aError.Append( m_createDupStr );
                        ErrorBox aErrorBox( static_cast<Window*>(this), WB_OK | RET_OK, aError );
                        aErrorBox.SetText( m_createErrTitleStr );
                        aErrorBox.Execute();
                        xNewDlg->SetObjectName( aNewName );
                        break;
                    }
                }
                if( bValid )
                    aNewName = aUserSuppliedName;
            }
            else
            {
                // user hit cancel or hit OK with nothing in the editbox

                return;
            }
        }
        while ( !bValid );

        // open up parent node (which ensures it's loaded)
        aScriptsBox.RequestingChilds( pEntry );

        Sequence< Any > args( 1 );
        args[ 0 ] <<= aNewName;
        Sequence< Any > outArgs( 0 );
        Sequence< sal_Int16 > outIndex;
        try
        {
            Any aResult;
            aResult = xInv->invoke( ::rtl::OUString::createFromAscii( "Creatable" ), args, outIndex, outArgs );
            Reference< browse::XBrowseNode > newNode( aResult, UNO_QUERY );
            aChildNode = newNode;

        }
        catch( Exception& e )
        {
            OSL_TRACE("Caught exception trying to Create %s",
                ::rtl::OUStringToOString(
                    e.Message, RTL_TEXTENCODING_ASCII_US ).pData->buffer );
        }
    }                
    if ( aChildNode.is() )
    {
        String aChildName = aChildNode->getName();
        SvLBoxEntry* pNewEntry = NULL;

			
		::rtl::OUString name( aChildName );
		Reference<XModel> xDocumentModel = getModel( pEntry );

        // ISSUE do we need to remove all entries for parent
        // to achieve sort? Just need to determine position
        // SvTreeListBox::InsertEntry can take position arg
        // -- Basic doesn't do this on create. 
        // Suppose we could avoid this too. -> created nodes are
        // not in alphabetical order
        if ( aChildNode->getType() == browse::BrowseNodeTypes::SCRIPT )
        {
            pNewEntry = aScriptsBox.insertEntry( aChildName,
                    IMG_MACRO, pEntry, false, std::auto_ptr< SFEntry >(new SFEntry( OBJTYPE_METHOD, aChildNode,xDocumentModel ) ) );

        }
        else
        {
            pNewEntry = aScriptsBox.insertEntry( aChildName,
                IMG_LIB, pEntry, false, std::auto_ptr< SFEntry >(new SFEntry( OBJTYPE_SCRIPTCONTAINER, aChildNode,xDocumentModel ) ) );
                        // If the Parent is not loaded then set to
                        // loaded, this will prevent RequestingChilds ( called
                        // from vcl via RequestingChilds ) from
                        // creating new ( duplicate ) children
                        SFEntry* userData = (SFEntry*)pEntry->GetUserData();
                        if ( userData &&  !userData->isLoaded() )
                        {
                            userData->setLoaded();
                        }
        }
        aScriptsBox.SetCurEntry( pNewEntry );
        aScriptsBox.Select( aScriptsBox.GetCurEntry() );

    }
    else
    {
        //ISSUE L10N & message from exception?
        String aError( m_createErrStr );
        ErrorBox aErrorBox( static_cast<Window*>(this), WB_OK | RET_OK, aError );
        aErrorBox.SetText( m_createErrTitleStr );
        aErrorBox.Execute();
    }
}

void SvxScriptOrgDialog::renameEntry( SvLBoxEntry* pEntry )
{

    Reference< browse::XBrowseNode >  aChildNode;
    Reference< browse::XBrowseNode > node = getBrowseNode( pEntry );
    Reference< script::XInvocation > xInv( node, UNO_QUERY );

    if ( xInv.is() )
    {
        ::rtl::OUString aNewName = node->getName();
        sal_Int32 extnPos = aNewName.lastIndexOf( '.' );
        ::rtl::OUString extn;
        if(extnPos>0)
        {
            extn = aNewName.copy(extnPos);
            aNewName = aNewName.copy(0,extnPos);
        }
        USHORT nMode = INPUTMODE_RENAME;

        std::auto_ptr< InputDialog > xNewDlg( new InputDialog( static_cast<Window*>(this), nMode ) );
        xNewDlg->SetObjectName( aNewName );

        BOOL bValid;
        do
        {
            if ( xNewDlg->Execute() && xNewDlg->GetObjectName().Len() )
            {
                ::rtl::OUString aUserSuppliedName = xNewDlg->GetObjectName();
                bValid = TRUE;
                /*
                for( sal_Int32 index = 0; index < childNodes.getLength(); index++ )
                {
                    if ( (aUserSuppliedName+extn).equals( childNodes[index]->getName() ) )
                    {
                        bValid = FALSE;
                        String aError( m_createErrStr );
                        aError.Append( m_createDupStr );
                        ErrorBox aErrorBox( static_cast<Window*>(this), WB_OK | RET_OK, aError );
                        aErrorBox.SetText( m_createErrTitleStr );
                        aErrorBox.Execute();
                        xNewDlg->SetObjectName( aNewName );
                        break;
                    }
                } */
                if( bValid )
                    aNewName = aUserSuppliedName;
            }
            else
            {
                // user hit cancel or hit OK with nothing in the editbox
                return;
            }
        }
        while ( !bValid );

        Sequence< Any > args( 1 );
        args[ 0 ] <<= aNewName;
        Sequence< Any > outArgs( 0 );
        Sequence< sal_Int16 > outIndex;
        try
        {
            Any aResult;
            aResult = xInv->invoke( ::rtl::OUString::createFromAscii( "Renamable" ), args, outIndex, outArgs );
            Reference< browse::XBrowseNode > newNode( aResult, UNO_QUERY );
            aChildNode = newNode;

        }
        catch( Exception& e )
        {
            OSL_TRACE("Caught exception trying to Rename %s",
                ::rtl::OUStringToOString(
                    e.Message, RTL_TEXTENCODING_ASCII_US ).pData->buffer );
        }
    }                
    if ( aChildNode.is() )
    {
        aScriptsBox.SetEntryText( pEntry, aChildNode->getName() );
        aScriptsBox.SetCurEntry( pEntry );
        aScriptsBox.Select( aScriptsBox.GetCurEntry() );

    }
    else
    {
        //ISSUE L10N & message from exception?
        String aError( m_renameErrStr );
        ErrorBox aErrorBox( static_cast<Window*>(this), WB_OK | RET_OK, aError );
        aErrorBox.SetText( m_renameErrTitleStr );
        aErrorBox.Execute();
    }
}
void SvxScriptOrgDialog::deleteEntry( SvLBoxEntry* pEntry )
{
    sal_Bool result = sal_False;
    Reference< browse::XBrowseNode > node = getBrowseNode( pEntry );
    // ISSUE L10N string & can we centre list?
    String aQuery( m_delQueryStr );
    aQuery.Append( getListOfChildren( node, 0 ) );
    QueryBox aQueryBox( static_cast<Window*>(this), WB_YES_NO | WB_DEF_YES, aQuery );
    aQueryBox.SetText( m_delQueryTitleStr );
    if ( aQueryBox.Execute() == RET_NO )
    {
        return;
    }

    Reference< script::XInvocation > xInv( node, UNO_QUERY );
    if ( xInv.is() )
    {
        Sequence< Any > args( 0 );
        Sequence< Any > outArgs( 0 );
        Sequence< sal_Int16 > outIndex;
        try
        {
            Any aResult;
            aResult = xInv->invoke( ::rtl::OUString::createFromAscii( "Deletable" ), args, outIndex, outArgs );
            aResult >>= result; // or do we just assume true if no exception ?
        }
        catch( Exception& e )
        {
            OSL_TRACE("Caught exception trying to delete %s",
                ::rtl::OUStringToOString(
                    e.Message, RTL_TEXTENCODING_ASCII_US ).pData->buffer );
        }
    }        
    
    if ( result == sal_True )
    {
        aScriptsBox.deleteTree( pEntry );
        aScriptsBox.GetModel()->Remove( pEntry );    
    }
    else
    {
        //ISSUE L10N & message from exception?
        ErrorBox aErrorBox( static_cast<Window*>(this), WB_OK | RET_OK, m_delErrStr );
        aErrorBox.SetText( m_delErrTitleStr );
        aErrorBox.Execute();
    }

}

BOOL SvxScriptOrgDialog::getBoolProperty( Reference< beans::XPropertySet >& xProps,
                ::rtl::OUString& propName )
{
    BOOL result = false;
    try
    {
        sal_Bool bTemp = sal_False;
        xProps->getPropertyValue( propName ) >>= bTemp;
        result = ( bTemp == sal_True );
    }
    catch ( Exception& )
    {
        return result;
    }
    return result;
}

String SvxScriptOrgDialog::getListOfChildren( Reference< browse::XBrowseNode > node, int depth )
{
    String result;
    result.Append( String::CreateFromAscii( "\n" ) );
    for( int i=0;i<=depth;i++ )
    {
        result.Append( String::CreateFromAscii( "\t" ) );
    }
    result.Append( String( node->getName() ) );

    try
    {
        if ( node->hasChildNodes() == sal_True )
        {
            Sequence< Reference< browse::XBrowseNode > > children
                = node->getChildNodes();
            for ( sal_Int32 n = 0; n < children.getLength(); n++ )
            {
                result.Append( getListOfChildren( children[ n ] , depth+1 ) );
            }
        }
    }
    catch ( Exception& )
    {
        // ignore, will return an empty string
    }

    return result;
}

Selection_hash SvxScriptOrgDialog::m_lastSelection;

void SvxScriptOrgDialog::StoreCurrentSelection()
{
    String aDescription;
    if ( aScriptsBox.IsSelected( aScriptsBox.GetHdlEntry() ) )
    {
        SvLBoxEntry* pEntry = aScriptsBox.GetHdlEntry();
        while( pEntry )
        {
            aDescription.Insert( aScriptsBox.GetEntryText( pEntry ), 0 );
            pEntry = aScriptsBox.GetParent( pEntry );
            if ( pEntry )
                aDescription.Insert( ';', 0 );
        }
        ::rtl::OUString sDesc( aDescription );
        m_lastSelection[ m_sLanguage ] = sDesc;
    }
}

void SvxScriptOrgDialog::RestorePreviousSelection()
{
    String aStoredEntry = String( m_lastSelection[ m_sLanguage ] );
    if( aStoredEntry.Len() <= 0 )
        return;
    SvLBoxEntry* pEntry = 0;
    USHORT nIndex = 0;
    while ( nIndex != STRING_NOTFOUND )
    {
        String aTmp( aStoredEntry.GetToken( 0, ';', nIndex ) );
        SvLBoxEntry* pTmpEntry = aScriptsBox.FirstChild( pEntry );
        ::rtl::OUString debugStr(aTmp);
        while ( pTmpEntry )
        {
            debugStr = ::rtl::OUString(aScriptsBox.GetEntryText( pTmpEntry ));
            if ( aScriptsBox.GetEntryText( pTmpEntry ) == aTmp )
            {
                pEntry = pTmpEntry;
                break;
            }
            pTmpEntry = aScriptsBox.NextSibling( pTmpEntry );
        }
        if ( !pTmpEntry )
            break;
        aScriptsBox.RequestingChilds( pEntry );
    }
    aScriptsBox.SetCurEntry( pEntry );
}

BOOL SFTreeListBox::dialogSort1( Reference< browse::XBrowseNode > node1, 
    Reference< browse::XBrowseNode > node2 )
{
    ::rtl::OUString userStr = ::rtl::OUString::createFromAscii("user");    
    ::rtl::OUString shareStr = ::rtl::OUString::createFromAscii("share");    
    if( node1->getName().equals( userStr ) )
        return true;
    if( node2->getName().equals( userStr ) )
        return false;
    if( node1->getName().equals( shareStr ) )
        return true;
    if( node2->getName().equals( shareStr ) )
        return false;
    return dialogSort2( node1, node2 );
}

BOOL SFTreeListBox::dialogSort2( Reference< browse::XBrowseNode > node1, 
    Reference< browse::XBrowseNode > node2 )
{
    return ( node1->getName().compareTo( node2->getName() ) < 0 );
}

::rtl::OUString ReplaceString(
    const ::rtl::OUString& source,
    const ::rtl::OUString& token,
    const ::rtl::OUString& value )
{
    sal_Int32 pos = source.indexOf( token );
                                                                                
    if ( pos != -1 && value.getLength() != 0 )
    {
        return source.replaceAt( pos, token.getLength(), value );
    }
    else
    {
        return source;
    }
}

::rtl::OUString FormatErrorString(
    const ::rtl::OUString& unformatted,
    const ::rtl::OUString& language,
    const ::rtl::OUString& script,
    const ::rtl::OUString& line,
    const ::rtl::OUString& type,
    const ::rtl::OUString& message )
{
    ::rtl::OUString result = unformatted.copy( 0 );

    result = ReplaceString(
        result, ::rtl::OUString::createFromAscii( "%LANGUAGENAME" ), language );
    result = ReplaceString(
        result, ::rtl::OUString::createFromAscii( "%SCRIPTNAME" ), script );
    result = ReplaceString(
        result, ::rtl::OUString::createFromAscii( "%LINENUMBER" ), line );

    if ( type.getLength() != 0 )
    {
        result += ::rtl::OUString::createFromAscii( "\n\n" );
        result += ::rtl::OUString(String(SVX_RES(RID_SVXSTR_ERROR_TYPE_LABEL)));
        result += ::rtl::OUString::createFromAscii( " " );
        result += type;
    }

    if ( message.getLength() != 0 )
    {
        result += ::rtl::OUString::createFromAscii( "\n\n" );
        result += ::rtl::OUString(String(SVX_RES(RID_SVXSTR_ERROR_MESSAGE_LABEL)));
        result += ::rtl::OUString::createFromAscii( " " );
        result += message;
    }

    return result;
}

::rtl::OUString GetErrorMessage(
    const provider::ScriptErrorRaisedException& eScriptError )
{
    ::rtl::OUString unformatted = String( SVX_RES( RID_SVXSTR_ERROR_AT_LINE ) );

    ::rtl::OUString unknown = ::rtl::OUString::createFromAscii( "UNKNOWN" );
    ::rtl::OUString language = unknown;
    ::rtl::OUString script = unknown;
    ::rtl::OUString line = unknown; 
    ::rtl::OUString type = ::rtl::OUString();
    ::rtl::OUString message = eScriptError.Message;

        if ( eScriptError.language.getLength() != 0 )
        {
            language = eScriptError.language;
        }

        if ( eScriptError.scriptName.getLength() != 0 )
        {
            script = eScriptError.scriptName;
        }

        if ( eScriptError.Message.getLength() != 0 )
        {
            message = eScriptError.Message;
        }
        if ( eScriptError.lineNum != -1 )
        {
            line = ::rtl::OUString::valueOf( eScriptError.lineNum );
            unformatted = String(
                SVX_RES( RID_SVXSTR_ERROR_AT_LINE ) );
        }
        else
        {
            unformatted = String(
				SVX_RES( RID_SVXSTR_ERROR_RUNNING ) );
        }

    return FormatErrorString(
        unformatted, language, script, line, type, message );
}

::rtl::OUString GetErrorMessage(
    const provider::ScriptExceptionRaisedException& eScriptException )
{
    ::rtl::OUString unformatted =
		  String( SVX_RES( RID_SVXSTR_EXCEPTION_AT_LINE ) );

    ::rtl::OUString unknown = ::rtl::OUString::createFromAscii( "UNKNOWN" );
    ::rtl::OUString language = unknown;
    ::rtl::OUString script = unknown;
    ::rtl::OUString line = unknown;
    ::rtl::OUString type = unknown;
    ::rtl::OUString message = eScriptException.Message;
    
    if ( eScriptException.language.getLength() != 0 )
    {
        language = eScriptException.language;
    }
    if ( eScriptException.scriptName.getLength() != 0 )
    {
        script = eScriptException.scriptName;
    }

    if ( eScriptException.Message.getLength() != 0 )
    {
        message = eScriptException.Message;
    }

    if ( eScriptException.lineNum != -1 )
    {
        line = ::rtl::OUString::valueOf( eScriptException.lineNum );
        unformatted = String(
            SVX_RES( RID_SVXSTR_EXCEPTION_AT_LINE ) );
    }
    else
    {
        unformatted = String(
            SVX_RES( RID_SVXSTR_EXCEPTION_RUNNING ) );
    }

    if ( eScriptException.exceptionType.getLength() != 0 )
    {
        type = eScriptException.exceptionType;
    }

    return FormatErrorString(
        unformatted, language, script, line, type, message );

}
::rtl::OUString GetErrorMessage(
    const provider::ScriptFrameworkErrorException& sError )
{
    ::rtl::OUString unformatted = String(
        SVX_RES( RID_SVXSTR_FRAMEWORK_ERROR_RUNNING ) );

    ::rtl::OUString language = 
        ::rtl::OUString::createFromAscii( "UNKNOWN" );

    ::rtl::OUString script = 
        ::rtl::OUString::createFromAscii( "UNKNOWN" );

    ::rtl::OUString message;

    if ( sError.scriptName.getLength() > 0 )
    {
        script = sError.scriptName;
    }
    if ( sError.language.getLength() > 0 )
    {
        language = sError.language;
    }
    if ( sError.errorType == provider::ScriptFrameworkErrorType::NOTSUPPORTED )
    {
        message = String(
            SVX_RES(  RID_SVXSTR_ERROR_LANG_NOT_SUPPORTED ) );
        message =  ReplaceString(
			message, ::rtl::OUString::createFromAscii( "%LANGUAGENAME" ), language );
 
    }
    else
    { 
        message = sError.Message;
    }
    return FormatErrorString(
        unformatted, language, script, ::rtl::OUString(), ::rtl::OUString(), message );
}

::rtl::OUString GetErrorMessage( const RuntimeException& re )
{
    Type t = ::getCppuType( &re );
    ::rtl::OUString message = t.getTypeName();
    message += re.Message;

    return message;
}

::rtl::OUString GetErrorMessage( const Exception& e )
{
    Type t = ::getCppuType( &e );
    ::rtl::OUString message = t.getTypeName();
    message += e.Message;

    return message;
}

::rtl::OUString GetErrorMessage( const com::sun::star::uno::Any& aException )
{
    ::rtl::OUString exType;
    if ( aException.getValueType() == 
         ::getCppuType( (const reflection::InvocationTargetException* ) NULL ) )
    {
        reflection::InvocationTargetException ite;
        aException >>= ite;
        if ( ite.TargetException.getValueType() == ::getCppuType( ( const provider::ScriptErrorRaisedException* ) NULL ) )
        {
            // Error raised by script
            provider::ScriptErrorRaisedException scriptError;
            ite.TargetException >>= scriptError;
            return GetErrorMessage( scriptError );
        }
        else if ( ite.TargetException.getValueType() == ::getCppuType( ( const provider::ScriptExceptionRaisedException* ) NULL ) )
        {
            // Exception raised by script
            provider::ScriptExceptionRaisedException scriptException;
            ite.TargetException >>= scriptException;
            return GetErrorMessage( scriptException );
        }
        else 
        {
            // Unknown error, shouldn't happen
            // OSL_ASSERT(...)
        }
        
    }
    else if ( aException.getValueType() == ::getCppuType( ( const provider::ScriptFrameworkErrorException* ) NULL ) )
    {
        // A Script Framework error has occured
        provider::ScriptFrameworkErrorException sfe;
        aException >>= sfe;
        return GetErrorMessage( sfe );
       
    }
    // unknown exception
    Exception e;
    RuntimeException rte;
    if ( aException >>= rte )
    {
        return GetErrorMessage( rte );
    }

    aException >>= e; 
    return GetErrorMessage( e );
    
}

SvxScriptErrorDialog::SvxScriptErrorDialog(
    Window* , ::com::sun::star::uno::Any aException )
    : m_sMessage()
{
    ::vos::OGuard aGuard( Application::GetSolarMutex() );
    m_sMessage = GetErrorMessage( aException );
}

SvxScriptErrorDialog::~SvxScriptErrorDialog()
{
}

short SvxScriptErrorDialog::Execute()
{
    // Show Error dialog asynchronously
    //
    // Pass a copy of the message to the ShowDialog method as the
    // SvxScriptErrorDialog may be deleted before ShowDialog is called
    Application::PostUserEvent(
        LINK( this, SvxScriptErrorDialog, ShowDialog ),
        new rtl::OUString( m_sMessage ) );

    return 0;
}

IMPL_LINK( SvxScriptErrorDialog, ShowDialog, ::rtl::OUString*, pMessage )
{
    ::rtl::OUString message;

    if ( pMessage && pMessage->getLength() != 0 )
    {
        message = *pMessage;
    }
    else
    {
        message = String( SVX_RES( RID_SVXSTR_ERROR_TITLE ) );
    }

    MessBox* pBox = new WarningBox( NULL, WB_OK, message );
    pBox->SetText( SVX_RES( RID_SVXSTR_ERROR_TITLE ) );
    pBox->Execute();

    if ( pBox ) delete pBox;
    if ( pMessage ) delete pMessage;

    return 0;
}
