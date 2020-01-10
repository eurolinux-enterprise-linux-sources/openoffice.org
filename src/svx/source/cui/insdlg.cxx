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
#include <com/sun/star/ui/dialogs/TemplateDescription.hpp>
#include <com/sun/star/ui/dialogs/ExecutableDialogResults.hpp>
#include <com/sun/star/ui/dialogs/XFilePicker.hpp>
#include <com/sun/star/ui/dialogs/XFilterManager.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <comphelper/processfactory.hxx>
#include <com/sun/star/embed/EmbedStates.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/embed/XInsertObjectDialog.hpp>
#include <com/sun/star/ucb/CommandAbortedException.hpp>
#include <com/sun/star/task/XInteractionHandler.hpp>

#include "insdlg.hxx"
#include <svx/dialmgr.hxx>
#include <svtools/sores.hxx>

#include <stdio.h>
#include <tools/urlobj.hxx>
#include <tools/debug.hxx>
#include <svtools/urihelper.hxx>
#include <svtools/svmedit.hxx>
#include <vcl/button.hxx>
#include <vcl/fixed.hxx>
#include <vcl/group.hxx>
#include <vcl/lstbox.hxx>
#include <vcl/msgbox.hxx>
#include <vcl/svapp.hxx>
#include <sot/clsids.hxx>
#include <sfx2/frmdescr.hxx>
#include <sfx2/viewsh.hxx>
#include <sfx2/filedlghelper.hxx>
#include <svtools/ownlist.hxx>
#include <comphelper/seqstream.hxx>

#include "svuidlg.hrc"


#include <osl/file.hxx>

#include <com/sun/star/container/XHierarchicalNameAccess.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <unotools/processfactory.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::ui::dialogs;
using ::rtl::OUString;

#define _SVSTDARR_STRINGSDTOR
#include <svtools/svstdarr.hxx>


static String impl_getSvtResString( sal_uInt32 nId )
{
    String aRet;
    com::sun::star::lang::Locale aLocale = Application::GetSettings().GetUILocale();
    ResMgr* pMgr = ResMgr::CreateResMgr( "svt", aLocale );
    if( pMgr )
    {
        aRet = String( ResId( nId, *pMgr ) );
        delete pMgr;
    }
    return aRet;
}

BOOL InsertObjectDialog_Impl::IsCreateNew() const
{
    return FALSE;
}

uno::Reference< io::XInputStream > InsertObjectDialog_Impl::GetIconIfIconified( ::rtl::OUString* /*pGraphicMediaType*/ )
{
	return uno::Reference< io::XInputStream >();
}

InsertObjectDialog_Impl::InsertObjectDialog_Impl( Window * pParent, const ResId & rResId, const com::sun::star::uno::Reference < com::sun::star::embed::XStorage >& xStorage )
 : ModalDialog( pParent, rResId )
 , m_xStorage( xStorage )
 , aCnt( m_xStorage )
{
}

// -----------------------------------------------------------------------

IMPL_LINK_INLINE_START( SvInsertOleDlg, DoubleClickHdl, ListBox *, EMPTYARG )
{
	EndDialog( RET_OK );
	return 0;
}
IMPL_LINK_INLINE_END( SvInsertOleDlg, DoubleClickHdl, ListBox *, pListBox )

// -----------------------------------------------------------------------

IMPL_LINK( SvInsertOleDlg, BrowseHdl, PushButton *, EMPTYARG )
{
    Reference< XMultiServiceFactory > xFactory( ::comphelper::getProcessServiceFactory() );
    if( xFactory.is() )
    {
        Reference< XFilePicker > xFilePicker( xFactory->createInstance( OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.ui.dialogs.FilePicker" ) ) ), UNO_QUERY );
        DBG_ASSERT( xFilePicker.is(), "could not get FilePicker service" );

        Reference< XInitialization > xInit( xFilePicker, UNO_QUERY );
        Reference< XFilterManager > xFilterMgr( xFilePicker, UNO_QUERY );
        if( xInit.is() && xFilePicker.is() && xFilterMgr.is() )
        {
            Sequence< Any > aServiceType( 1 );
            aServiceType[0] <<= TemplateDescription::FILEOPEN_SIMPLE;
            xInit->initialize( aServiceType );

            // add filter
            try
            {
                xFilterMgr->appendFilter(
                     OUString(),
                     OUString( RTL_CONSTASCII_USTRINGPARAM( "*.*" ) )
                     );
            }
            catch( IllegalArgumentException& )
            {
                DBG_ASSERT( 0, "caught IllegalArgumentException when registering filter\n" );
            }

            if( xFilePicker->execute() == ExecutableDialogResults::OK )
            {
                Sequence< OUString > aPathSeq( xFilePicker->getFiles() );
				INetURLObject aObj( aPathSeq[0] );
				aEdFilepath.SetText( aObj.PathToFileName() );
            }
        }
    }

	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvInsertOleDlg, RadioHdl, RadioButton *, EMPTYARG )
{
	if ( aRbNewObject.IsChecked() )
	{
		aLbObjecttype.Show();
		aEdFilepath.Hide();
		aBtnFilepath.Hide();
		aCbFilelink.Hide();
		aGbObject.SetText( _aOldStr );
	}
	else
	{
        aCbFilelink.Show();
		aLbObjecttype.Hide();
		aEdFilepath.Show();
		aBtnFilepath.Show();
		aCbFilelink.Show();
		aGbObject.SetText( aStrFile );
	}
	return 0;
}

// -----------------------------------------------------------------------

void SvInsertOleDlg::SelectDefault()
{
	aLbObjecttype.SelectEntryPos( 0 );
}

void SvInsertOleDlg::FillObjectServerList( SvObjectServerList* pList )
{
	pList->FillInsertObjects();
}

// -----------------------------------------------------------------------
SvInsertOleDlg::SvInsertOleDlg
(
    Window* pParent,
    const Reference < embed::XStorage >& xStorage,
    const SvObjectServerList* pServers
)
    : InsertObjectDialog_Impl( pParent, SVX_RES( MD_INSERT_OLEOBJECT ), xStorage ),
    aRbNewObject( this, SVX_RES( RB_NEW_OBJECT ) ),
    aRbObjectFromfile( this, SVX_RES( RB_OBJECT_FROMFILE ) ),
    aLbObjecttype( this, SVX_RES( LB_OBJECTTYPE ) ),
    aEdFilepath( this, SVX_RES( ED_FILEPATH ) ),
    aBtnFilepath( this, SVX_RES( BTN_FILEPATH ) ),
    aCbFilelink( this, SVX_RES( CB_FILELINK ) ),
    aGbObject( this, SVX_RES( GB_OBJECT ) ),
    aOKButton1( this, SVX_RES( 1 ) ),
    aCancelButton1( this, SVX_RES( 1 ) ),
    aHelpButton1( this, SVX_RES( 1 ) ),
    aStrFile( SVX_RES( STR_FILE ) ),
    m_pServers( pServers )
{
    FreeResource();
	_aOldStr = aGbObject.GetText();
    aLbObjecttype.SetDoubleClickHdl( LINK( this, SvInsertOleDlg, DoubleClickHdl ) );
	aBtnFilepath.SetClickHdl( LINK( this, SvInsertOleDlg, BrowseHdl ) );
	Link aLink( LINK( this, SvInsertOleDlg, RadioHdl ) );
	aRbNewObject.SetClickHdl( aLink );
	aRbObjectFromfile.SetClickHdl( aLink );
	aRbNewObject.Check( TRUE );
	RadioHdl( NULL );
}

short SvInsertOleDlg::Execute()
{
    short nRet = RET_OK;
	SvObjectServerList  aObjS;
    if ( !m_pServers )
	{
        // if no list was provided, take the complete one
		aObjS.FillInsertObjects();
        m_pServers = &aObjS;
	}

    // fill listbox and select default
    ListBox& rBox = GetObjectTypes();
	rBox.SetUpdateMode( FALSE );
    for ( ULONG i = 0; i < m_pServers->Count(); i++ )
        rBox.InsertEntry( (*m_pServers)[i].GetHumanName() );
	rBox.SetUpdateMode( TRUE );
    SelectDefault();
    ::rtl::OUString aName;

    DBG_ASSERT( m_xStorage.is(), "No storage!");
    if ( m_xStorage.is() && ( nRet = Dialog::Execute() ) == RET_OK )
	{
        String aFileName;
        BOOL bLink = FALSE;
        BOOL bCreateNew = IsCreateNew();
        if ( bCreateNew )
		{
            // create and insert new embedded object
            String aServerName = rBox.GetSelectEntry();
            const SvObjectServer* pS = m_pServers->Get( aServerName );
			if ( pS )
			{
                if( pS->GetClassName() == SvGlobalName( SO3_OUT_CLASSID ) )
				{
					try
					{
                    	uno::Reference < embed::XInsertObjectDialog > xDialogCreator(
							::comphelper::getProcessServiceFactory()->createInstance(
                            	::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("com.sun.star.embed.MSOLEObjectSystemCreator")) ),
							uno::UNO_QUERY );

						if ( xDialogCreator.is() )
						{
							aName = aCnt.CreateUniqueObjectName();
                        	embed::InsertedObjectInfo aNewInf = xDialogCreator->createInstanceByDialog(
																	m_xStorage,
																	aName,
                                    								uno::Sequence < beans::PropertyValue >() );

							OSL_ENSURE( aNewInf.Object.is(), "The object must be created or an exception must be thrown!" );
							m_xObj = aNewInf.Object;
							for ( sal_Int32 nInd = 0; nInd < aNewInf.Options.getLength(); nInd++ )
								if ( aNewInf.Options[nInd].Name.equals( ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "Icon" ) ) ) )
								{
									aNewInf.Options[nInd].Value >>= m_aIconMetaFile;
								}
								else if ( aNewInf.Options[nInd].Name.equals( ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "IconFormat" ) ) ) )
								{
									datatransfer::DataFlavor aFlavor;
									if ( aNewInf.Options[nInd].Value >>= aFlavor )
										m_aIconMediaType = aFlavor.MimeType;
								}

						}
					}
					catch( ucb::CommandAbortedException& )
					{
						// the user has pressed cancel
					}
					catch( uno::Exception& )
					{
						// TODO: Error handling
					}
				}
                else
                {
                    // create object with desired ClassId
                    m_xObj = aCnt.CreateEmbeddedObject( pS->GetClassName().GetByteSequence(), aName );
                }

                if ( !m_xObj.is() )
				{
                    if( aFileName.Len() )  // from OLE Dialog
					{
						// Objekt konnte nicht aus Datei erzeugt werden
                        // global Resource from svtools (former so3 resource)
                        String aErr( impl_getSvtResString( STR_ERROR_OBJNOCREATE_FROM_FILE ) );
                        aErr.SearchAndReplace( String( '%' ), aFileName );
                        ErrorBox( this, WB_3DLOOK | WB_OK, aErr ).Execute();
					}
					else
					{
						// Objekt konnte nicht erzeugt werden
                        // global Resource from svtools (former so3 resource)
                        String aErr( impl_getSvtResString( STR_ERROR_OBJNOCREATE ) );
                        aErr.SearchAndReplace( String( '%' ), aServerName );
                        ErrorBox( this, WB_3DLOOK | WB_OK, aErr ).Execute();
					}
				}
			}
		}
		else
		{
            aFileName = GetFilePath();
            INetURLObject aURL;
            aURL.SetSmartProtocol( INET_PROT_FILE );
            aURL.SetSmartURL( aFileName );
            aFileName = aURL.GetMainURL( INetURLObject::NO_DECODE );
            bLink = IsLinked();

            if ( aFileName.Len() )
            {
                // create MediaDescriptor for file to create object from
                uno::Sequence < beans::PropertyValue > aMedium( 2 );
                aMedium[0].Name = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "URL" ) );
                aMedium[0].Value <<= ::rtl::OUString( aFileName );

        		uno::Reference< task::XInteractionHandler > xInteraction;
    			uno::Reference< lang::XMultiServiceFactory > xFactory = ::comphelper::getProcessServiceFactory();
    			if ( xFactory.is() )
        			xInteraction = uno::Reference< task::XInteractionHandler >(
						xFactory->createInstance(
							DEFINE_CONST_UNICODE("com.sun.star.task.InteractionHandler") ),
						uno::UNO_QUERY_THROW );

				if ( xInteraction.is() )
				{
               		aMedium[1].Name = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "InteractionHandler" ) );
               		aMedium[1].Value <<= xInteraction;
				}
				else
				{
					OSL_ASSERT( "Can not get InteractionHandler!\n" );
					aMedium.realloc( 1 );
				}

                // create object from media descriptor
				if ( bLink )
					m_xObj = aCnt.InsertEmbeddedLink( aMedium, aName );
				else
                	m_xObj = aCnt.InsertEmbeddedObject( aMedium, aName );
            }

            if ( !m_xObj.is() )
			{
				// Objekt konnte nicht aus Datei erzeugt werden
                // global Resource from svtools (former so3 resource)
                String aErr( impl_getSvtResString( STR_ERROR_OBJNOCREATE_FROM_FILE ) );
                aErr.SearchAndReplace( String( '%' ), aFileName );
                ErrorBox( this, WB_3DLOOK | WB_OK, aErr ).Execute();
			}
		}
	}

    m_pServers = 0;
    return nRet;
}

uno::Reference< io::XInputStream > SvInsertOleDlg::GetIconIfIconified( ::rtl::OUString* pGraphicMediaType )
{
	if ( m_aIconMetaFile.getLength() )
	{
		if ( pGraphicMediaType )
			*pGraphicMediaType = m_aIconMediaType;

		return uno::Reference< io::XInputStream >( new ::comphelper::SequenceInputStream( m_aIconMetaFile ) );
	}

	return uno::Reference< io::XInputStream >();
}

IMPL_LINK( SvInsertPlugInDialog, BrowseHdl, PushButton *, EMPTYARG )
{
    Sequence< OUString > aFilterNames, aFilterTypes;
    void fillNetscapePluginFilters( Sequence< OUString >& rNames, Sequence< OUString >& rTypes );
    fillNetscapePluginFilters( aFilterNames, aFilterTypes );

    Reference< XMultiServiceFactory > xFactory( ::comphelper::getProcessServiceFactory() );
    if( xFactory.is() )
    {
        Reference< XFilePicker > xFilePicker( xFactory->createInstance( OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.ui.dialogs.FilePicker" ) ) ), UNO_QUERY );
        DBG_ASSERT( xFilePicker.is(), "could not get FilePicker service" );

        Reference< XInitialization > xInit( xFilePicker, UNO_QUERY );
        Reference< XFilterManager > xFilterMgr( xFilePicker, UNO_QUERY );
        if( xInit.is() && xFilePicker.is() && xFilterMgr.is() )
        {
            Sequence< Any > aServiceType( 1 );
            aServiceType[0] <<= TemplateDescription::FILEOPEN_SIMPLE;
            xInit->initialize( aServiceType );

            // add the filters
            try
            {
                const OUString* pNames = aFilterNames.getConstArray();
                const OUString* pTypes = aFilterTypes.getConstArray();
                for( int i = 0; i < aFilterNames.getLength(); i++ )
                    xFilterMgr->appendFilter( pNames[i], pTypes[i] );
            }
            catch( IllegalArgumentException& )
            {
                DBG_ASSERT( 0, "caught IllegalArgumentException when registering filter\n" );
            }

            if( xFilePicker->execute() == ExecutableDialogResults::OK )
            {
                Sequence< OUString > aPathSeq( xFilePicker->getFiles() );
				INetURLObject aObj( aPathSeq[0] );
				aEdFileurl.SetText( aObj.PathToFileName() );
            }
        }
    }

	return 0;
}

// -----------------------------------------------------------------------

SvInsertPlugInDialog::SvInsertPlugInDialog( Window* pParent, const uno::Reference < embed::XStorage >& xStorage )
    : InsertObjectDialog_Impl( pParent, SVX_RES( MD_INSERT_OBJECT_PLUGIN ), xStorage ),
    aEdFileurl( this, SVX_RES( ED_FILEURL ) ),
    aBtnFileurl( this, SVX_RES( BTN_FILEURL ) ),
    aGbFileurl( this, SVX_RES( GB_FILEURL ) ),
    aEdPluginsOptions( this, SVX_RES( ED_PLUGINS_OPTIONS ) ),
    aGbPluginsOptions( this, SVX_RES( GB_PLUGINS_OPTIONS ) ),
    aOKButton1( this, SVX_RES( 1 ) ),
    aCancelButton1( this, SVX_RES( 1 ) ),
    aHelpButton1( this, SVX_RES( 1 ) ),
    m_pURL(0)
{
    FreeResource();
    aBtnFileurl.SetClickHdl( LINK( this, SvInsertPlugInDialog, BrowseHdl ) );
}

SvInsertPlugInDialog::~SvInsertPlugInDialog()
{
    delete m_pURL;
}

// -----------------------------------------------------------------------

static void Plugin_ImplFillCommandSequence( const String& aCommands, uno::Sequence< beans::PropertyValue >& aCommandSequence )
{
    USHORT nEaten;
    SvCommandList aLst;
    aLst.AppendCommands( aCommands, &nEaten );

    const sal_Int32 nCount = aLst.Count();
	aCommandSequence.realloc( nCount );
	for( sal_Int32 nIndex = 0; nIndex < nCount; nIndex++ )
	{
        const SvCommand& rCommand = aLst[ nIndex ];

		aCommandSequence[nIndex].Name = rCommand.GetCommand();
		aCommandSequence[nIndex].Handle = -1;
		aCommandSequence[nIndex].Value = makeAny( OUString( rCommand.GetArgument() ) );
        aCommandSequence[nIndex].State = beans::PropertyState_DIRECT_VALUE;
	}
}

short SvInsertPlugInDialog::Execute()
{
    short nRet = RET_OK;
    m_aCommands.Erase();
    DBG_ASSERT( m_xStorage.is(), "No storage!");
    if ( m_xStorage.is() && ( nRet = Dialog::Execute() ) == RET_OK )
	{
        if ( !m_pURL )
            m_pURL = new INetURLObject();
		else
            *m_pURL = INetURLObject();

        m_aCommands = GetPlugInOptions();
        String aURL = GetPlugInFile();

        // URL can be a valid and absolute URL or a system file name
        m_pURL->SetSmartProtocol( INET_PROT_FILE );
        if ( !aURL.Len() || m_pURL->SetSmartURL( aURL ) )
		{
            // create a plugin object
            ::rtl::OUString aName;
            SvGlobalName aClassId( SO3_PLUGIN_CLASSID );
            m_xObj = aCnt.CreateEmbeddedObject( aClassId.GetByteSequence(), aName );
        }

        if ( m_xObj.is() )
        {
            // set properties from dialog
            if ( m_xObj->getCurrentState() == embed::EmbedStates::LOADED )
                m_xObj->changeState( embed::EmbedStates::RUNNING );

            uno::Reference < beans::XPropertySet > xSet( m_xObj->getComponent(), uno::UNO_QUERY );
            if ( xSet.is() )
            {
                xSet->setPropertyValue( ::rtl::OUString::createFromAscii("PluginURL"),
                        makeAny( ::rtl::OUString( m_pURL->GetMainURL( INetURLObject::NO_DECODE ) ) ) );
                uno::Sequence< beans::PropertyValue > aCommandSequence;
                Plugin_ImplFillCommandSequence( m_aCommands, aCommandSequence );
                xSet->setPropertyValue( ::rtl::OUString::createFromAscii("PluginCommands"), makeAny( aCommandSequence ) );
            }
		}
		else
		{
			// PlugIn konnte nicht erzeugt werden
            // global Resource from svtools (former so3 resource)
            String aErr( impl_getSvtResString( STR_ERROR_OBJNOCREATE_PLUGIN ) );
			aErr.SearchAndReplace( String( '%' ), aURL );
            ErrorBox( this, WB_3DLOOK | WB_OK, aErr ).Execute();
		}
	}

    return nRet;
}

// class SvInsertAppletDlg -----------------------------------------------

IMPL_LINK( SvInsertAppletDialog, BrowseHdl, PushButton *, EMPTYARG )
{
    Reference< XMultiServiceFactory > xFactory( ::comphelper::getProcessServiceFactory() );
    if( xFactory.is() )
    {
        Reference< XFilePicker > xFilePicker( xFactory->createInstance( OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.ui.dialogs.FilePicker" ) ) ), UNO_QUERY );
        DBG_ASSERT( xFilePicker.is(), "could not get FilePicker service" );

        Reference< XInitialization > xInit( xFilePicker, UNO_QUERY );
        Reference< XFilterManager > xFilterMgr( xFilePicker, UNO_QUERY );
        if( xInit.is() && xFilePicker.is() && xFilterMgr.is() )
        {
            Sequence< Any > aServiceType( 1 );
            aServiceType[0] <<= TemplateDescription::FILEOPEN_SIMPLE;
            xInit->initialize( aServiceType );

            // add filter
            try
            {
                xFilterMgr->appendFilter(
                     OUString( RTL_CONSTASCII_USTRINGPARAM( "Applet" ) ),
                     OUString( RTL_CONSTASCII_USTRINGPARAM( "*.class" ) )
                     );
            }
            catch( IllegalArgumentException& )
            {
                DBG_ASSERT( 0, "caught IllegalArgumentException when registering filter\n" );
            }

            if( xFilePicker->execute() == ExecutableDialogResults::OK )
            {
                Sequence< OUString > aPathSeq( xFilePicker->getFiles() );

				INetURLObject aObj( aPathSeq[0] );
                aEdClassfile.SetText( aObj.getName() );
                aObj.removeSegment();
				aEdClasslocation.SetText( aObj.PathToFileName() );
            }
        }
    }

	return 0;
}

// -----------------------------------------------------------------------

SvInsertAppletDialog::SvInsertAppletDialog( Window* pParent, const uno::Reference < embed::XStorage >& xStorage )
    : InsertObjectDialog_Impl( pParent, SVX_RES( MD_INSERT_OBJECT_APPLET ), xStorage ),
    aFtClassfile( this, SVX_RES( FT_CLASSFILE ) ),
    aEdClassfile( this, SVX_RES( ED_CLASSFILE ) ),
    aFtClasslocation( this, SVX_RES( FT_CLASSLOCATION ) ),
    aEdClasslocation( this, SVX_RES( ED_CLASSLOCATION ) ),
    aBtnClass( this, SVX_RES( BTN_CLASS ) ),
    aGbClass( this, SVX_RES( GB_CLASS ) ),
    aEdAppletOptions( this, SVX_RES( ED_APPLET_OPTIONS ) ),
    aGbAppletOptions( this, SVX_RES( GB_APPLET_OPTIONS ) ),
    aOKButton1( this, SVX_RES( 1 ) ),
    aCancelButton1( this, SVX_RES( 1 ) ),
    aHelpButton1( this, SVX_RES( 1 ) ),
    m_pURL(0)
{
    FreeResource();
    aBtnClass.SetClickHdl( LINK( this, SvInsertAppletDialog, BrowseHdl ) );
}

SvInsertAppletDialog::SvInsertAppletDialog( Window* pParent, const uno::Reference < embed::XEmbeddedObject >& xObj )
    : InsertObjectDialog_Impl( pParent, SVX_RES( MD_INSERT_OBJECT_APPLET ), uno::Reference < embed::XStorage >() ),
    aFtClassfile( this, SVX_RES( FT_CLASSFILE ) ),
    aEdClassfile( this, SVX_RES( ED_CLASSFILE ) ),
    aFtClasslocation( this, SVX_RES( FT_CLASSLOCATION ) ),
    aEdClasslocation( this, SVX_RES( ED_CLASSLOCATION ) ),
    aBtnClass( this, SVX_RES( BTN_CLASS ) ),
    aGbClass( this, SVX_RES( GB_CLASS ) ),
    aEdAppletOptions( this, SVX_RES( ED_APPLET_OPTIONS ) ),
    aGbAppletOptions( this, SVX_RES( GB_APPLET_OPTIONS ) ),
    aOKButton1( this, SVX_RES( 1 ) ),
    aCancelButton1( this, SVX_RES( 1 ) ),
    aHelpButton1( this, SVX_RES( 1 ) ),
    m_pURL(0)
{
    m_xObj = xObj;
    FreeResource();
    aBtnClass.SetClickHdl( LINK( this, SvInsertAppletDialog, BrowseHdl ) );
}


SvInsertAppletDialog::~SvInsertAppletDialog()
{
    delete m_pURL;
}

short SvInsertAppletDialog::Execute()
{
    short nRet = RET_OK;
    m_aClass.Erase();
    m_aCommands.Erase();

    BOOL bOK = FALSE;
    uno::Reference < beans::XPropertySet > xSet;
    if ( m_xObj.is() )
	{
        try
        {
            if ( m_xObj->getCurrentState() == embed::EmbedStates::LOADED )
                m_xObj->changeState( embed::EmbedStates::RUNNING );
            xSet = uno::Reference < beans::XPropertySet >( m_xObj->getComponent(), uno::UNO_QUERY );
            ::rtl::OUString aStr;
            uno::Any aAny = xSet->getPropertyValue( ::rtl::OUString::createFromAscii("AppletCode") );
            if ( aAny >>= aStr )
                SetClass( aStr );
            aAny = xSet->getPropertyValue( ::rtl::OUString::createFromAscii("AppletCodeBase") );
            if ( aAny >>= aStr )
                SetClassLocation( aStr );
            uno::Sequence< beans::PropertyValue > aCommands;
            aAny = xSet->getPropertyValue( ::rtl::OUString::createFromAscii("AppletCommands") );
            if ( aAny >>= aCommands )
            {
                SvCommandList aList;
                aList.FillFromSequence( aCommands );
                SetAppletOptions( aList.GetCommands() );
            }

            String aText( SVX_RES( STR_EDIT_APPLET ) );
            SetText( aText );
            bOK = TRUE;
        }
        catch ( uno::Exception& )
        {
            DBG_ERROR( "No Applet!" );
        }
	}
    else
    {
        DBG_ASSERT( m_xStorage.is(), "No storage!");
        bOK = m_xStorage.is();
    }

    if ( bOK && ( nRet = Dialog::Execute() ) == RET_OK )
	{
        if ( !m_xObj.is() )
        {
            ::rtl::OUString aName;
            SvGlobalName aClassId( SO3_APPLET_CLASSID );
            m_xObj = aCnt.CreateEmbeddedObject( aClassId.GetByteSequence(), aName );
            if ( m_xObj->getCurrentState() == embed::EmbedStates::LOADED )
                m_xObj->changeState( embed::EmbedStates::RUNNING );
            xSet = uno::Reference < beans::XPropertySet >( m_xObj->getComponent(), uno::UNO_QUERY );
        }

        if ( m_xObj.is() )
        {
            try
            {
                BOOL bIPActive = m_xObj->getCurrentState() == embed::EmbedStates::INPLACE_ACTIVE;
                if ( bIPActive )
                    m_xObj->changeState( embed::EmbedStates::RUNNING );

                String aClassLocation = GetClassLocation();

                // Hack, aFileName wird auch fuer Class benutzt
                m_aClass = GetClass();
                xSet->setPropertyValue( ::rtl::OUString::createFromAscii("AppletCode"), makeAny( ::rtl::OUString( m_aClass ) ) );

                ::rtl::OUString tmp = aClassLocation;
                ::osl::File::getFileURLFromSystemPath(tmp, tmp);
                aClassLocation = tmp;
                xSet->setPropertyValue( ::rtl::OUString::createFromAscii("AppletCodeBase"), makeAny( tmp ) );
                m_aCommands = GetAppletOptions();

                uno::Sequence< beans::PropertyValue > aCommandSequence;
                Plugin_ImplFillCommandSequence( m_aCommands, aCommandSequence );
                xSet->setPropertyValue( ::rtl::OUString::createFromAscii("AppletCommands"), makeAny( aCommandSequence ) );

                if ( bIPActive )
                    m_xObj->changeState( embed::EmbedStates::INPLACE_ACTIVE );
            }
            catch ( uno::Exception& )
            {
                DBG_ERROR( "No Applet!" );
            }
        }
    }

    return nRet;
}

SfxInsertFloatingFrameDialog::SfxInsertFloatingFrameDialog( Window *pParent,
                            const com::sun::star::uno::Reference < com::sun::star::embed::XStorage >& xStorage )
    : InsertObjectDialog_Impl( pParent, SVX_RES( MD_INSERT_OBJECT_IFRAME ), xStorage )
	, aFTName ( this, SVX_RES( FT_FRAMENAME ) )
	, aEDName ( this, SVX_RES( ED_FRAMENAME ) )
	, aFTURL ( this, SVX_RES( FT_URL ) )
	, aEDURL ( this, SVX_RES( ED_URL ) )
	, aBTOpen ( this, SVX_RES(BT_FILEOPEN ) )
	, aRBScrollingOn ( this, SVX_RES( RB_SCROLLINGON ) )
	, aRBScrollingOff ( this, SVX_RES( RB_SCROLLINGOFF ) )
	, aRBScrollingAuto ( this, SVX_RES( RB_SCROLLINGAUTO ) )
    , aFLScrolling ( this, SVX_RES( GB_SCROLLING ) )
    , aFLSepLeft( this, SVX_RES( FL_SEP_LEFT ) )
	, aRBFrameBorderOn ( this, SVX_RES( RB_FRMBORDER_ON ) )
	, aRBFrameBorderOff ( this, SVX_RES( RB_FRMBORDER_OFF ) )
    , aFLFrameBorder( this, SVX_RES( GB_BORDER ) )
    , aFLSepRight( this, SVX_RES( FL_SEP_RIGHT ) )
	, aFTMarginWidth ( this, SVX_RES( FT_MARGINWIDTH ) )
	, aNMMarginWidth ( this, SVX_RES( NM_MARGINWIDTH ) )
	, aCBMarginWidthDefault( this, SVX_RES( CB_MARGINHEIGHTDEFAULT ) )
	, aFTMarginHeight ( this, SVX_RES( FT_MARGINHEIGHT ) )
	, aNMMarginHeight ( this, SVX_RES( NM_MARGINHEIGHT ) )
	, aCBMarginHeightDefault( this, SVX_RES( CB_MARGINHEIGHTDEFAULT ) )
    , aFLMargin( this, SVX_RES( GB_MARGIN ) )
    , aOKButton1( this, SVX_RES( 1 ) )
    , aCancelButton1( this, SVX_RES( 1 ) )
    , aHelpButton1( this, SVX_RES( 1 ) )
{
	FreeResource();

	aFLSepLeft.SetStyle(aFLSepLeft.GetStyle()|WB_VERT);
    aFLSepRight.SetStyle(aFLSepRight.GetStyle()|WB_VERT);

    Link aLink( STATIC_LINK( this, SfxInsertFloatingFrameDialog, CheckHdl ) );
	aCBMarginWidthDefault.SetClickHdl( aLink );
	aCBMarginHeightDefault.SetClickHdl( aLink );

    aCBMarginWidthDefault.Check();
    aCBMarginHeightDefault.Check();
    aRBScrollingAuto.Check();
    aRBFrameBorderOn.Check();

    aBTOpen.SetClickHdl( STATIC_LINK( this, SfxInsertFloatingFrameDialog, OpenHdl ) );
}

SfxInsertFloatingFrameDialog::SfxInsertFloatingFrameDialog( Window *pParent, const uno::Reference < embed::XEmbeddedObject >& xObj )
    : InsertObjectDialog_Impl( pParent, SVX_RES( MD_INSERT_OBJECT_IFRAME ), uno::Reference < embed::XStorage >() )
	, aFTName ( this, SVX_RES( FT_FRAMENAME ) )
	, aEDName ( this, SVX_RES( ED_FRAMENAME ) )
	, aFTURL ( this, SVX_RES( FT_URL ) )
	, aEDURL ( this, SVX_RES( ED_URL ) )
	, aBTOpen ( this, SVX_RES(BT_FILEOPEN ) )
	, aRBScrollingOn ( this, SVX_RES( RB_SCROLLINGON ) )
	, aRBScrollingOff ( this, SVX_RES( RB_SCROLLINGOFF ) )
	, aRBScrollingAuto ( this, SVX_RES( RB_SCROLLINGAUTO ) )
    , aFLScrolling ( this, SVX_RES( GB_SCROLLING ) )
    , aFLSepLeft( this, SVX_RES( FL_SEP_LEFT ) )
	, aRBFrameBorderOn ( this, SVX_RES( RB_FRMBORDER_ON ) )
	, aRBFrameBorderOff ( this, SVX_RES( RB_FRMBORDER_OFF ) )
    , aFLFrameBorder( this, SVX_RES( GB_BORDER ) )
    , aFLSepRight( this, SVX_RES( FL_SEP_RIGHT ) )
	, aFTMarginWidth ( this, SVX_RES( FT_MARGINWIDTH ) )
	, aNMMarginWidth ( this, SVX_RES( NM_MARGINWIDTH ) )
	, aCBMarginWidthDefault( this, SVX_RES( CB_MARGINHEIGHTDEFAULT ) )
	, aFTMarginHeight ( this, SVX_RES( FT_MARGINHEIGHT ) )
	, aNMMarginHeight ( this, SVX_RES( NM_MARGINHEIGHT ) )
	, aCBMarginHeightDefault( this, SVX_RES( CB_MARGINHEIGHTDEFAULT ) )
    , aFLMargin( this, SVX_RES( GB_MARGIN ) )
    , aOKButton1( this, SVX_RES( 1 ) )
    , aCancelButton1( this, SVX_RES( 1 ) )
    , aHelpButton1( this, SVX_RES( 1 ) )
{
	FreeResource();

    m_xObj = xObj;

	aFLSepLeft.SetStyle(aFLSepLeft.GetStyle()|WB_VERT);
    aFLSepRight.SetStyle(aFLSepRight.GetStyle()|WB_VERT);

    Link aLink( STATIC_LINK( this, SfxInsertFloatingFrameDialog, CheckHdl ) );
	aCBMarginWidthDefault.SetClickHdl( aLink );
	aCBMarginHeightDefault.SetClickHdl( aLink );

    aCBMarginWidthDefault.Check();
    aCBMarginHeightDefault.Check();
    aRBScrollingAuto.Check();
    aRBFrameBorderOn.Check();

    aBTOpen.SetClickHdl( STATIC_LINK( this, SfxInsertFloatingFrameDialog, OpenHdl ) );
}

short SfxInsertFloatingFrameDialog::Execute()
{
    short nRet = RET_OK;
    BOOL bOK = FALSE;
    uno::Reference < beans::XPropertySet > xSet;
    if ( m_xObj.is() )
	{
        try
        {
            if ( m_xObj->getCurrentState() == embed::EmbedStates::LOADED )
                m_xObj->changeState( embed::EmbedStates::RUNNING );
            xSet = uno::Reference < beans::XPropertySet >( m_xObj->getComponent(), uno::UNO_QUERY );
            ::rtl::OUString aStr;
            uno::Any aAny = xSet->getPropertyValue( ::rtl::OUString::createFromAscii("FrameURL") );
            if ( aAny >>= aStr )
                aEDURL.SetText( aStr );
            aAny = xSet->getPropertyValue( ::rtl::OUString::createFromAscii("FrameName") );
            if ( aAny >>= aStr )
                aEDName.SetText( aStr );

            sal_Int32 nSize = SIZE_NOT_SET;
            aAny = xSet->getPropertyValue( ::rtl::OUString::createFromAscii("FrameMarginWidth") );
            aAny >>= nSize;

            if ( nSize == SIZE_NOT_SET )
            {
                aCBMarginWidthDefault.Check( TRUE );
                aNMMarginWidth.SetText( String::CreateFromInt32( DEFAULT_MARGIN_WIDTH )  );
                aFTMarginWidth.Enable( FALSE );
                aNMMarginWidth.Enable( FALSE );
            }
            else
                aNMMarginWidth.SetText( String::CreateFromInt32( nSize ) );

            aAny = xSet->getPropertyValue( ::rtl::OUString::createFromAscii("FrameMarginHeight") );
            aAny >>= nSize;

            if ( nSize == SIZE_NOT_SET )
            {
                aCBMarginHeightDefault.Check( TRUE );
                aNMMarginHeight.SetText( String::CreateFromInt32( DEFAULT_MARGIN_HEIGHT )  );
                aFTMarginHeight.Enable( FALSE );
                aNMMarginHeight.Enable( FALSE );
            }
            else
                aNMMarginHeight.SetText( String::CreateFromInt32( nSize ) );

            BOOL bScrollOn = FALSE;
            BOOL bScrollOff = FALSE;
            BOOL bScrollAuto = FALSE;

            sal_Bool bSet = sal_False;
            aAny = xSet->getPropertyValue( ::rtl::OUString::createFromAscii("FrameIsAutoScroll") );
            aAny >>= bSet;
            if ( !bSet )
            {
                aAny = xSet->getPropertyValue( ::rtl::OUString::createFromAscii("FrameIsScrollingMode") );
                aAny >>= bSet;
                bScrollOn = bSet;
                bScrollOff = !bSet;
            }
            else
                bScrollAuto = TRUE;

            aRBScrollingOn.Check( bScrollOn );
            aRBScrollingOff.Check( bScrollOff );
            aRBScrollingAuto.Check( bScrollAuto );

            bSet = sal_False;
            aAny = xSet->getPropertyValue( ::rtl::OUString::createFromAscii("FrameIsAutoBorder") );
            aAny >>= bSet;
            if ( !bSet )
            {
                aAny = xSet->getPropertyValue( ::rtl::OUString::createFromAscii("FrameIsBorder") );
                aAny >>= bSet;
                aRBFrameBorderOn.Check( bSet );
                aRBFrameBorderOff.Check( !bSet );
            }

            SetUpdateMode( TRUE );
            bOK = TRUE;
        }
        catch ( uno::Exception& )
        {
            DBG_ERROR( "No IFrame!" );
        }
	}
    else
    {
        DBG_ASSERT( m_xStorage.is(), "No storage!");
        bOK = m_xStorage.is();
    }

    if ( bOK && ( nRet = Dialog::Execute() ) == RET_OK )
	{
        ::rtl::OUString aURL;
        if ( aEDURL.GetText().Len() )
        {
            // URL can be a valid and absolute URL or a system file name
            INetURLObject aObj;
            aObj.SetSmartProtocol( INET_PROT_FILE );
            if ( aObj.SetSmartURL( aEDURL.GetText() ) )
                aURL = aObj.GetMainURL( INetURLObject::NO_DECODE );
        }

        if ( !m_xObj.is() && aURL.getLength() )
		{
            // create the object
            ::rtl::OUString aName;
            SvGlobalName aClassId( SO3_IFRAME_CLASSID );
            m_xObj = aCnt.CreateEmbeddedObject( aClassId.GetByteSequence(), aName );
            if ( m_xObj->getCurrentState() == embed::EmbedStates::LOADED )
                m_xObj->changeState( embed::EmbedStates::RUNNING );
            xSet = uno::Reference < beans::XPropertySet >( m_xObj->getComponent(), uno::UNO_QUERY );
        }

        if ( m_xObj.is() )
        {
            try
            {
                BOOL bIPActive = m_xObj->getCurrentState() == embed::EmbedStates::INPLACE_ACTIVE;
                if ( bIPActive )
                    m_xObj->changeState( embed::EmbedStates::RUNNING );

                ::rtl::OUString aName = aEDName.GetText();
                ScrollingMode eScroll = ScrollingNo;
                if ( aRBScrollingOn.IsChecked() )
                    eScroll = ScrollingYes;
                if ( aRBScrollingOff.IsChecked() )
                    eScroll = ScrollingNo;
                if ( aRBScrollingAuto.IsChecked() )
                    eScroll = ScrollingAuto;

                sal_Bool bHasBorder = aRBFrameBorderOn.IsChecked();

                long lMarginWidth;
                if ( !aCBMarginWidthDefault.IsChecked() )
                    lMarginWidth = (long) aNMMarginWidth.GetText().ToInt32();
                else
                    lMarginWidth = SIZE_NOT_SET;

                long lMarginHeight;
                if ( !aCBMarginHeightDefault.IsChecked() )
                    lMarginHeight = (long) aNMMarginHeight.GetText().ToInt32();
                else
                    lMarginHeight = SIZE_NOT_SET;

                xSet->setPropertyValue( ::rtl::OUString::createFromAscii("FrameURL"), makeAny( aURL ) );
                xSet->setPropertyValue( ::rtl::OUString::createFromAscii("FrameName"), makeAny( aName ) );

                if ( eScroll == ScrollingAuto )
                    xSet->setPropertyValue( ::rtl::OUString::createFromAscii("FrameIsAutoScroll"),
                        makeAny( sal_True ) );
                else
                    xSet->setPropertyValue( ::rtl::OUString::createFromAscii("FrameIsScrollingMode"),
                        makeAny( (sal_Bool) ( eScroll == ScrollingYes) ) );

                //if ( aFrmDescr.IsFrameBorderSet() )
                    xSet->setPropertyValue( ::rtl::OUString::createFromAscii("FrameIsBorder"),
                        makeAny( bHasBorder ) );
                /*else
                    xSet->setPropertyValue( ::rtl::OUString::createFromAscii("FrameIsAutoBorder"),
                        makeAny( sal_True ) );*/

                xSet->setPropertyValue( ::rtl::OUString::createFromAscii("FrameMarginWidth"),
                    makeAny( sal_Int32( lMarginWidth ) ) );

                xSet->setPropertyValue( ::rtl::OUString::createFromAscii("FrameMarginHeight"),
                    makeAny( sal_Int32( lMarginHeight ) ) );

                if ( bIPActive )
                    m_xObj->changeState( embed::EmbedStates::INPLACE_ACTIVE );
            }
            catch ( uno::Exception& )
            {
                DBG_ERROR( "No IFrame!" );
            }
        }
    }

    return nRet;
}

//------------------------------------------------------------------------------

IMPL_STATIC_LINK( SfxInsertFloatingFrameDialog, CheckHdl, CheckBox*, pCB )
{
	if ( pCB == &pThis->aCBMarginWidthDefault )
	{
		if ( pCB->IsChecked() )
			pThis->aNMMarginWidth.SetText( String::CreateFromInt32( DEFAULT_MARGIN_WIDTH ) );
		pThis->aFTMarginWidth.Enable( !pCB->IsChecked() );
		pThis->aNMMarginWidth.Enable( !pCB->IsChecked() );
	}

	if ( pCB == &pThis->aCBMarginHeightDefault )
	{
		if ( pCB->IsChecked() )
			pThis->aNMMarginHeight.SetText( String::CreateFromInt32( DEFAULT_MARGIN_HEIGHT ) );
		pThis->aFTMarginHeight.Enable( !pCB->IsChecked() );
		pThis->aNMMarginHeight.Enable( !pCB->IsChecked() );
	}

	return 0L;
}

//------------------------------------------------------------------------------

IMPL_STATIC_LINK( SfxInsertFloatingFrameDialog, OpenHdl, PushButton*, EMPTYARG )
{
    Window* pOldParent = Application::GetDefDialogParent();
	Application::SetDefDialogParent( pThis );

    // create the file dialog
	sfx2::FileDialogHelper aFileDlg( WB_OPEN | SFXWB_PASSWORD, String() );

	// set the title
    aFileDlg.SetTitle( OUString( String( SVX_RES( MD_INSERT_OBJECT_IFRAME ) ) ) );

	// show the dialog
    if ( aFileDlg.Execute() == ERRCODE_NONE )
		pThis->aEDURL.SetText(
			INetURLObject( aFileDlg.GetPath() ).GetMainURL( INetURLObject::DECODE_WITH_CHARSET ) );

    Application::SetDefDialogParent( pOldParent );
	return 0L;
}

