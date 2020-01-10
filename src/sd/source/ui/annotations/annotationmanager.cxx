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
#include "precompiled_sd.hxx"

#include "sddll.hxx"

//#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/beans/XMultiPropertyStates.hpp>
#include <com/sun/star/frame/XController.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/view/XSelectionSupplier.hpp>
#include <com/sun/star/geometry/RealPoint2D.hpp>
#include <com/sun/star/text/XText.hpp>
#include <com/sun/star/document/XEventBroadcaster.hpp>

#include <vcl/menu.hxx>
#include <vcl/msgbox.hxx>

#include <svtools/style.hxx>
#include <svtools/itempool.hxx>
#include <svtools/useroptions.hxx>
#include <svtools/syslocale.hxx>
#include <svtools/saveopt.hxx>

#include <sfx2/imagemgr.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/app.hxx>
#include <sfx2/request.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/objface.hxx>
#include <sfx2/imagemgr.hxx>

#include <svx/editeng.hxx>
#include <svx/eeitem.hxx>
#include <svx/fontitem.hxx>
#include <svx/fhgtitem.hxx>
#include <svx/outlobj.hxx>
#include <svx/postitem.hxx>
#include <svx/wghtitem.hxx>
#include <svx/udlnitem.hxx>
#include <svx/crsditem.hxx>

#include <svx/svdetc.hxx>

#include "annotationmanager.hxx"
#include "annotationmanagerimpl.hxx"
#include "annotationwindow.hxx"
#include "annotations.hrc"

#include "ToolBarManager.hxx"
#include "DrawDocShell.hxx"
#include "DrawViewShell.hxx"
#include "DrawController.hxx"
#include "glob.hrc"
#include "sdresid.hxx"
#include "EventMultiplexer.hxx"
#include "ViewShellManager.hxx"
#include "helpids.h"
#include "sdpage.hxx"
#include "drawdoc.hxx"
#include "textapi.hxx"
#include "optsitem.hxx"

#define C2U(x) OUString( RTL_CONSTASCII_USTRINGPARAM( x ) )
using ::rtl::OUString;
using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::drawing;
using namespace ::com::sun::star::document;
using namespace ::com::sun::star::geometry;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::text;
using namespace ::com::sun::star::view;
using namespace ::com::sun::star::style;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::ui;
using namespace ::com::sun::star::task;
using namespace ::com::sun::star::office;

namespace sd {

// --------------------------------------------------------------------

extern TextApiObject* getTextApiObject( const Reference< XAnnotation >& xAnnotation );

// --------------------------------------------------------------------

SfxItemPool* GetAnnotationPool()
{
    static SfxItemPool* mpAnnotationPool = 0;
	if( mpAnnotationPool == 0 )
	{
		mpAnnotationPool = EditEngine::CreatePool( sal_False );
		mpAnnotationPool->SetPoolDefaultItem(SvxFontHeightItem(423,100,EE_CHAR_FONTHEIGHT));

		Font aAppFont( Application::GetSettings().GetStyleSettings().GetAppFont() );
		String EMPTYSTRING;
		mpAnnotationPool->SetPoolDefaultItem(SvxFontItem(aAppFont.GetFamily(),aAppFont.GetName(), EMPTYSTRING,PITCH_DONTKNOW,RTL_TEXTENCODING_DONTKNOW,EE_CHAR_FONTINFO));
	}

	return mpAnnotationPool;
}

// --------------------------------------------------------------------

static SfxBindings* getBindings( ViewShellBase& rBase )
{
	if( rBase.GetMainViewShell().get() && rBase.GetMainViewShell()->GetViewFrame() )
		return &rBase.GetMainViewShell()->GetViewFrame()->GetBindings();
	else
		return 0;
}

// --------------------------------------------------------------------

static SfxDispatcher* getDispatcher( ViewShellBase& rBase )
{
	if( rBase.GetMainViewShell().get() && rBase.GetMainViewShell()->GetViewFrame() )
		return rBase.GetMainViewShell()->GetViewFrame()->GetDispatcher();
	else
		return 0;
}

com::sun::star::util::DateTime getCurrentDateTime()
{        
    TimeValue osltime;
    osl_getSystemTime( &osltime );
    oslDateTime osldt;
    osl_getDateTimeFromTimeValue( &osltime, &osldt );
    return com::sun::star::util::DateTime( 0, osldt.Seconds, osldt.Minutes, osldt.Hours, osldt.Day, osldt.Month, osldt.Year );	    
}

OUString getAnnotationDateTimeString( const Reference< XAnnotation >& xAnnotation )
{
    OUString sRet;
    if( xAnnotation.is() )
    {
	    const LocaleDataWrapper& rLocalData = SvtSysLocale().GetLocaleData();

        com::sun::star::util::DateTime aDateTime( xAnnotation->getDateTime() );
                
	    Date aDate = Date( aDateTime.Day, aDateTime.Month, aDateTime.Year );
	    if (aDate==Date())
		    sRet = sRet + String(SdResId(STR_ANNOTATION_TODAY));
	    else
	    if (aDate == Date(Date()-1))
		    sRet = sRet + String(SdResId(STR_ANNOTATION_YESTERDAY));
	    else
	    if (aDate.IsValid() )
		    sRet = sRet + rLocalData.getDate(aDate);
    		
        Time aTime( aDateTime.Hours, aDateTime.Minutes, aDateTime.Seconds, aDateTime.HundredthSeconds );		
	    if(aTime.GetTime() != 0)
		    sRet = sRet + rtl::OUString::createFromAscii(" ")  + rLocalData.getTime( aTime,false );
    }
    return sRet;
}

// --------------------------------------------------------------------

AnnotationManagerImpl::AnnotationManagerImpl( ViewShellBase& rViewShellBase )
: AnnotationManagerImplBase( m_aMutex )
, mrBase( rViewShellBase )
, mpDoc( rViewShellBase.GetDocument() )
, mbShowAnnotations( true )
, mnUpdateTagsEvent( 0 )
{
	SdOptions* pOptions = SD_MOD()->GetSdOptions(mpDoc->GetDocumentType());
	if( pOptions )
	    mbShowAnnotations = pOptions->IsShowComments() == TRUE;	
}

// --------------------------------------------------------------------

void AnnotationManagerImpl::init()
{
	// get current controller and initialize listeners
	try
	{
		addListener();	
		mxView = Reference< XDrawView >::query(mrBase.GetController());
	}
	catch( Exception& e )
	{
		(void)e;
		DBG_ERROR( "sd::AnnotationManagerImpl::AnnotationManagerImpl(), Exception caught!" );
	}

    try
    {
        Reference<XEventBroadcaster> xModel (mrBase.GetDocShell()->GetModel(), UNO_QUERY_THROW );
        Reference<XEventListener> xListener( this );
        xModel->addEventListener( xListener );
    }
    catch( Exception& )
    {
    }    
}

// --------------------------------------------------------------------

// WeakComponentImplHelper1
void SAL_CALL AnnotationManagerImpl::disposing ()
{
    try
    {
        Reference<XEventBroadcaster> xModel (mrBase.GetDocShell()->GetModel(), UNO_QUERY_THROW );
        Reference<XEventListener> xListener( this );
        xModel->removeEventListener( xListener );
    }
    catch( Exception& )
    {
    }    

	removeListener();
	DisposeTags();

    if( mnUpdateTagsEvent )
    {
        Application::RemoveUserEvent( mnUpdateTagsEvent );
        mnUpdateTagsEvent = 0;
    }

    mxView.clear();
    mxCurrentPage.clear();
}

// --------------------------------------------------------------------
    
// XEventListener
void SAL_CALL AnnotationManagerImpl::notifyEvent( const ::com::sun::star::document::EventObject& aEvent ) throw (::com::sun::star::uno::RuntimeException)
{
    if( aEvent.EventName.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM( "OnAnnotationInserted") ) ||
        aEvent.EventName.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM( "OnAnnotationRemoved") ) ||
        aEvent.EventName.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM( "OnAnnotationChanged" )) )
    {
        UpdateTags();
    }
}

void SAL_CALL AnnotationManagerImpl::disposing( const ::com::sun::star::lang::EventObject& /*Source*/ ) throw (::com::sun::star::uno::RuntimeException)
{
}

void AnnotationManagerImpl::ShowAnnotations( bool bShow )
{
    // enforce show annotations if a new annotation is inserted
    if( mbShowAnnotations != bShow )
    {
        mbShowAnnotations = bShow;

        SdOptions* pOptions = SD_MOD()->GetSdOptions(mpDoc->GetDocumentType());
       	if( pOptions )
	        pOptions->SetShowComments( mbShowAnnotations ? sal_True : sal_False );	
	        
        UpdateTags();	        
	}
}

// --------------------------------------------------------------------

void AnnotationManagerImpl::ExecuteAnnotation(SfxRequest& rReq )
{
    switch( rReq.GetSlot() )
    {
    case SID_INSERT_POSTIT:
        ExecuteInsertAnnotation( rReq );
        break;
	case SID_DELETE_POSTIT:
	case SID_DELETEALL_POSTIT:
	case SID_DELETEALLBYAUTHOR_POSTIT:
	    ExecuteDeleteAnnotation( rReq );
        break;
	case SID_PREVIOUS_POSTIT:
	case SID_NEXT_POSTIT:
	    SelectNextAnnotation( rReq.GetSlot() == SID_NEXT_POSTIT );
        break;    
    case SID_REPLYTO_POSTIT:
        ExecuteReplyToAnnotation( rReq );
        break;
    case SID_SHOW_POSTIT:
        ShowAnnotations( !mbShowAnnotations );
        break;
    }
}

// --------------------------------------------------------------------

void AnnotationManagerImpl::ExecuteInsertAnnotation(SfxRequest& /*rReq*/)
{
    ShowAnnotations(true);
    InsertAnnotation();    
}

// --------------------------------------------------------------------

void AnnotationManagerImpl::ExecuteDeleteAnnotation(SfxRequest& rReq)
{
    ShowAnnotations( true );

    const SfxItemSet* pArgs = rReq.GetArgs();
    
    switch( rReq.GetSlot() )
    {
    case SID_DELETEALL_POSTIT:
        DeleteAllAnnotations();
        break;
    case SID_DELETEALLBYAUTHOR_POSTIT:
        if( pArgs )
        {
	        const SfxPoolItem*  pPoolItem = NULL;
	        if( SFX_ITEM_SET == pArgs->GetItemState( SID_DELETEALLBYAUTHOR_POSTIT, TRUE, &pPoolItem ) )
	        {
	            OUString sAuthor( (( const SfxStringItem* ) pPoolItem )->GetValue() );
		        DeleteAnnotationsByAuthor( sAuthor );
		    }
        }
        break;
    case SID_DELETE_POSTIT:        
        {   
            Reference< XAnnotation > xAnnotation;

            if( rReq.GetSlot() == SID_DELETE_POSTIT )
            {        
                if( pArgs )
                {
                    const SfxPoolItem*  pPoolItem = NULL;
                    if( SFX_ITEM_SET == pArgs->GetItemState( SID_DELETE_POSTIT, TRUE, &pPoolItem ) )
                        ( ( const SfxUnoAnyItem* ) pPoolItem )->GetValue() >>= xAnnotation;
                }
            }
            
            if( !xAnnotation.is() )        
                GetSelectedAnnotation( xAnnotation );
           
            DeleteAnnotation( xAnnotation );
        }
        break;
    }

    UpdateTags();
}

// --------------------------------------------------------------------

void AnnotationManagerImpl::InsertAnnotation()
{
    SdPage* pPage = GetCurrentPage();
    if( pPage )
    {
        if( mpDoc->IsUndoEnabled() )
            mpDoc->BegUndo( String( SdResId( STR_ANNOTATION_UNDO_INSERT ) ) );
    
        // find free space for new annotation
        int y = 0, x = 0;
        
       	AnnotationVector aAnnotations( pPage->getAnnotations() );
       	if( !aAnnotations.empty() )
   	    {       	
       	    const int page_width = pPage->GetSize().Width();
            const int width = 1000;
            const int height = 800;
            Rectangle aTagRect;
       	           	  
       	    while( true )
       	    {  
                Rectangle aNewRect( x, y, x + width - 1, y + height - 1 );
       	        bool bFree = true;
           	    
       	        for( AnnotationVector::iterator iter = aAnnotations.begin(); iter != aAnnotations.end(); iter++ )
       	        {
       	            RealPoint2D aPoint( (*iter)->getPosition() );
       	            aTagRect.nLeft   = sal::static_int_cast< long >( aPoint.X * 100.0 );
       	            aTagRect.nTop    = sal::static_int_cast< long >( aPoint.Y * 100.0 );
       	            aTagRect.nRight  = aTagRect.nLeft + width - 1;
       	            aTagRect.nBottom = aTagRect.nTop + height - 1;
           	        
       	            if( aNewRect.IsOver( aTagRect ) )
       	            {
       	                bFree = false;
       	                break;
       	            }
       	        }
           	    
       	        if( bFree == false)
       	        {
       	            x += width;
       	            if( x > page_width )
       	            {
       	                x = 0;
       	                y += height;
       	            }       	            
       	        }
       	        else
       	        {
       	            break;
       	        }
       	    }
       	}       	
    
        Reference< XAnnotation > xAnnotation;
        pPage->createAnnotation( xAnnotation );
        
        // set current author to new annotation
        SvtUserOptions aUserOptions;
        xAnnotation->setAuthor( aUserOptions.GetFullName() );
        
        // set current time to new annotation
        xAnnotation->setDateTime( getCurrentDateTime() );
        
        // set position
        RealPoint2D aPos( ((double)x) / 100.0, ((double)y) / 100.0 );
        xAnnotation->setPosition( aPos );

        if( mpDoc->IsUndoEnabled() )
            mpDoc->EndUndo();
        
        UpdateTags(true);
        SelectAnnotation( xAnnotation, true );
    }
}

// --------------------------------------------------------------------

void AnnotationManagerImpl::ExecuteReplyToAnnotation( SfxRequest& rReq )
{
    Reference< XAnnotation > xAnnotation;
    const SfxItemSet* pArgs = rReq.GetArgs();
    if( pArgs )
    {
        const SfxPoolItem*  pPoolItem = NULL;
        if( SFX_ITEM_SET == pArgs->GetItemState( rReq.GetSlot(), TRUE, &pPoolItem ) )
	        ( ( const SfxUnoAnyItem* ) pPoolItem )->GetValue() >>= xAnnotation;
    }


    TextApiObject* pTextApi = getTextApiObject( xAnnotation );
    if( pTextApi )
    {
	    std::auto_ptr< ::Outliner > pOutliner( new ::Outliner(GetAnnotationPool(),OUTLINERMODE_TEXTOBJECT) );
    	
	    mpDoc->SetCalcFieldValueHdl( pOutliner.get() );
	    pOutliner->SetUpdateMode( TRUE );

	    String aStr(SdResId(STR_ANNOTATION_REPLY));
	    OUString sAuthor( xAnnotation->getAuthor() );
	    if( sAuthor.getLength() == 0 )
	        sAuthor = String( SdResId( STR_ANNOTATION_NOAUTHOR ) );
	        
        aStr.SearchAndReplaceAscii("%1", sAuthor);

        aStr.Append( String(RTL_CONSTASCII_USTRINGPARAM(" (") ) );
        aStr.Append( String( getAnnotationDateTimeString( xAnnotation ) ) );
        aStr.Append( String(RTL_CONSTASCII_USTRINGPARAM("): \"") ) );

        String sQuote( pTextApi->GetText() );

        if( sQuote.Len() == 0 )
            sQuote = String( RTL_CONSTASCII_USTRINGPARAM( "..." ) );
        aStr.Append( sQuote );
        aStr.Append( String(RTL_CONSTASCII_USTRINGPARAM("\"\n") ) );
	    
        USHORT nParaCount = aStr.GetTokenCount( '\n' );
        for( USHORT nPara = 0; nPara < nParaCount; nPara++ )
            pOutliner->Insert( aStr.GetToken( nPara, '\n' ), LIST_APPEND, -1 );
        
        if( pOutliner->GetParagraphCount() > 1 )
        {
	        SfxItemSet aAnswerSet( pOutliner->GetEmptyItemSet() );
	        aAnswerSet.Put(SvxPostureItem(ITALIC_NORMAL,EE_CHAR_ITALIC));	    
    	    
	        ESelection aSel;
	        aSel.nEndPara = (USHORT)pOutliner->GetParagraphCount()-2;
	        aSel.nEndPos = pOutliner->GetText( pOutliner->GetParagraph( aSel.nEndPara ) ).Len();
    	    
            pOutliner->QuickSetAttribs( aAnswerSet, aSel );
        }
                
        std::auto_ptr< OutlinerParaObject > pOPO( pOutliner->CreateParaObject() );
        pTextApi->SetText( *pOPO.get() );
        
        SvtUserOptions aUserOptions;
        xAnnotation->setAuthor( aUserOptions.GetFullName() );        

        // set current time to reply
        xAnnotation->setDateTime( getCurrentDateTime() );
                
        UpdateTags(true);
        SelectAnnotation( xAnnotation, true );
	}
}

// --------------------------------------------------------------------

void AnnotationManagerImpl::DeleteAnnotation( Reference< XAnnotation > xAnnotation )
{     
    SdPage* pPage = GetCurrentPage();

    if( xAnnotation.is() && pPage )
    {
        if( mpDoc->IsUndoEnabled() )
            mpDoc->BegUndo( String( SdResId( STR_ANNOTATION_UNDO_DELETE ) ) );

        pPage->removeAnnotation( xAnnotation );

        if( mpDoc->IsUndoEnabled() )
            mpDoc->EndUndo();

        UpdateTags();
    }
}

void AnnotationManagerImpl::DeleteAnnotationsByAuthor( const rtl::OUString& sAuthor )
{
    if( mpDoc->IsUndoEnabled() )
        mpDoc->BegUndo( String( SdResId( STR_ANNOTATION_UNDO_DELETE ) ) );

    SdPage* pPage = 0;
	do
	{
	    pPage = GetNextPage( pPage, true );
	    
        if( pPage && !pPage->getAnnotations().empty() )
        {     
            AnnotationVector aAnnotations( pPage->getAnnotations() );
            for( AnnotationVector::iterator iter = aAnnotations.begin(); iter != aAnnotations.end(); iter++ )
            {
                Reference< XAnnotation > xAnnotation( *iter );
                if( xAnnotation->getAuthor() == sAuthor )
                {
                    if( mxSelectedAnnotation == xAnnotation )
                        mxSelectedAnnotation.clear();
                    pPage->removeAnnotation( xAnnotation );
                }
            }
         }
    } while( pPage );

    if( mpDoc->IsUndoEnabled() )
        mpDoc->EndUndo();
}

void AnnotationManagerImpl::DeleteAllAnnotations()
{
    if( mpDoc->IsUndoEnabled() )
        mpDoc->BegUndo( String( SdResId( STR_ANNOTATION_UNDO_DELETE ) ) );

    SdPage* pPage = 0;
	do
	{
	    pPage = GetNextPage( pPage, true );
	    
        if( pPage && !pPage->getAnnotations().empty() )
        {     

            AnnotationVector aAnnotations( pPage->getAnnotations() );
            for( AnnotationVector::iterator iter = aAnnotations.begin(); iter != aAnnotations.end(); iter++ )
            {
                pPage->removeAnnotation( (*iter) );            
            }
         }
    }
    while( pPage );

    mxSelectedAnnotation.clear();

    if( mpDoc->IsUndoEnabled() )
        mpDoc->EndUndo();
}

// --------------------------------------------------------------------

void AnnotationManagerImpl::GetAnnotationState(SfxItemSet& rSet)
{
    SdPage* pCurrentPage = GetCurrentPage();
 
    const bool bReadOnly = mrBase.GetDocShell()->IsReadOnly();
    const bool bWrongPageKind = (pCurrentPage == 0) || (pCurrentPage->GetPageKind() != PK_STANDARD);
   
    const SvtSaveOptions::ODFDefaultVersion nCurrentODFVersion( SvtSaveOptions().GetODFDefaultVersion() );

    if( bReadOnly || bWrongPageKind || (nCurrentODFVersion != SvtSaveOptions::ODFVER_LATEST) )
        rSet.DisableItem( SID_INSERT_POSTIT ); 

    rSet.Put(SfxBoolItem(SID_SHOW_POSTIT, mbShowAnnotations));
     
    Reference< XAnnotation > xAnnotation;
    GetSelectedAnnotation( xAnnotation );
    
    if( !xAnnotation.is() || bReadOnly )
        rSet.DisableItem( SID_DELETE_POSTIT );        

	SdPage* pPage = 0;
	
	bool bHasAnnotations = false;
	do
	{
	    pPage = GetNextPage( pPage, true );
	    
        if( pPage && !pPage->getAnnotations().empty() )        
            bHasAnnotations = true;
    }
    while( pPage && !bHasAnnotations );

    if( !bHasAnnotations || bReadOnly )
    {
	    rSet.DisableItem( SID_DELETEALL_POSTIT );
    }	    

    if( bWrongPageKind || !bHasAnnotations )
    {
	    rSet.DisableItem( SID_PREVIOUS_POSTIT );
	    rSet.DisableItem( SID_NEXT_POSTIT );
	}
}

// --------------------------------------------------------------------

void AnnotationManagerImpl::SelectNextAnnotation(bool bForeward)
{
    ShowAnnotations( true );

    Reference< XAnnotation > xCurrent;
    GetSelectedAnnotation( xCurrent );
    SdPage* pPage = GetCurrentPage();
    if( !pPage )
        return;

   	AnnotationVector aAnnotations( pPage->getAnnotations() );

    if( bForeward )
    {
        if( xCurrent.is() )
        {
   	        for( AnnotationVector::iterator iter = aAnnotations.begin(); iter != aAnnotations.end(); iter++ )
   	        {
   	            if( (*iter) == xCurrent )
   	            {
   	                iter++;
   	                if( iter != aAnnotations.end() )
   	                {
   	                    SelectAnnotation( (*iter) );
   	                    return;
   	                }
   	                break;
   	            }
   	        }
        }
        else if( !aAnnotations.empty() )
        {
            SelectAnnotation( *(aAnnotations.begin()) );
            return;
        }
    }
    else
    {       
        if( xCurrent.is() )
        {
   	        for( AnnotationVector::iterator iter = aAnnotations.begin(); iter != aAnnotations.end(); iter++ )
   	        {
   	            if( (*iter) == xCurrent )
   	            {
   	                if( iter != aAnnotations.begin() )
   	                {
   	                    iter--;
                        SelectAnnotation( (*iter) );
                        return;

   	                }
   	                break;
   	            }
   	        }
        }
        else if( !aAnnotations.empty() )
        {        
            AnnotationVector::iterator iter( aAnnotations.end() );
            SelectAnnotation( *(--iter) );
            return;
        }
    }

    mxSelectedAnnotation.clear();
    do
    {
        do
        {
            pPage = GetNextPage( pPage, bForeward );
            
            if( pPage && !pPage->getAnnotations().empty() )        
            {
                // switch to next/previous slide with annotations
                ::boost::shared_ptr<DrawViewShell> pDrawViewShell(::boost::dynamic_pointer_cast<DrawViewShell>(mrBase.GetMainViewShell()));
                if (pDrawViewShell.get() != NULL)
                {
                    pDrawViewShell->ChangeEditMode(pPage->IsMasterPage() ? EM_MASTERPAGE : EM_PAGE, FALSE);
                    pDrawViewShell->SwitchPage((pPage->GetPageNum() - 1) >> 1);
                    
                    SfxDispatcher* pDispatcher = getDispatcher( mrBase );
                    if( pDispatcher )
                        pDispatcher->Execute( bForeward ? SID_NEXT_POSTIT : SID_PREVIOUS_POSTIT );
                        
                    return;
                }            
            }
        }
        while( pPage );
            
        // The question text depends on the search direction.
        bool bImpress = mpDoc->GetDocumentType() == DOCUMENT_TYPE_IMPRESS;
        sal_uInt16 nStringId;
        if(bForeward)
            nStringId = bImpress ? STR_ANNOTATION_WRAP_FORWARD : STR_ANNOTATION_WRAP_FORWARD_DRAW;
        else
            nStringId = bImpress ? STR_ANNOTATION_WRAP_BACKWARD : STR_ANNOTATION_WRAP_BACKWARD_DRAW;

        // Pop up question box that asks the user whether to wrap arround.
        // The dialog is made modal with respect to the whole application.
        QueryBox aQuestionBox (
            NULL,
            WB_YES_NO | WB_DEF_YES,
            String(SdResId(nStringId)));
        aQuestionBox.SetImage (QueryBox::GetStandardImage());
        USHORT nBoxResult = aQuestionBox.Execute();
        if(nBoxResult != BUTTONID_YES)
            break;
    }
    while( true );
}

// --------------------------------------------------------------------

void AnnotationManagerImpl::onTagSelected(	AnnotationTag& rTag )
{
    mxSelectedAnnotation = rTag.GetAnnotation();
    invalidateSlots();
}

// --------------------------------------------------------------------

void AnnotationManagerImpl::onTagDeselected( AnnotationTag& rTag )
{
    if( rTag.GetAnnotation() == mxSelectedAnnotation )
    {
        mxSelectedAnnotation.clear();
        invalidateSlots();
    }
}

// --------------------------------------------------------------------

void AnnotationManagerImpl::SelectAnnotation( ::com::sun::star::uno::Reference< ::com::sun::star::office::XAnnotation > xAnnotation, bool bEdit /* = FALSE */ )
{      
    mxSelectedAnnotation = xAnnotation;
    
    for( AnnotationTagVector::iterator iter( maTagVector.begin() ); iter != maTagVector.end(); iter++ )
    {
        if( (*iter)->GetAnnotation() == xAnnotation )
        {
       		SmartTagReference xTag( (*iter).get() );      
            mrBase.GetMainViewShell()->GetView()->getSmartTags().select( xTag );
            (*iter)->OpenPopup( bEdit );
            break;
        }
    }
}

// --------------------------------------------------------------------

void AnnotationManagerImpl::GetSelectedAnnotation( ::com::sun::star::uno::Reference< ::com::sun::star::office::XAnnotation >& xAnnotation )
{
    xAnnotation = mxSelectedAnnotation;
}

void AnnotationManagerImpl::invalidateSlots()
{
    SfxBindings* pBindings = getBindings( mrBase );
    if( pBindings )
    {
        pBindings->Invalidate( SID_INSERT_POSTIT );
        pBindings->Invalidate( SID_DELETE_POSTIT );
        pBindings->Invalidate( SID_DELETEALL_POSTIT );
        pBindings->Invalidate( SID_PREVIOUS_POSTIT );
        pBindings->Invalidate( SID_NEXT_POSTIT );   
        pBindings->Invalidate( SID_UNDO );   
        pBindings->Invalidate( SID_REDO );
    }
}

// --------------------------------------------------------------------

void AnnotationManagerImpl::onSelectionChanged()
{
	if( mxView.is() && mrBase.GetDrawView() ) try
	{
		Reference< XAnnotationAccess > xPage( mxView->getCurrentPage(), UNO_QUERY );

		if( xPage != mxCurrentPage )
		{
			mxCurrentPage = xPage;
			
			UpdateTags(true);
	    }	 
	}
	catch( Exception& )
	{
		DBG_ERROR( "sd::AnnotationManagerImpl::onSelectionChanged(), exception caught!" );
	}
}

void AnnotationManagerImpl::UpdateTags( bool bSynchron )
{
    if( bSynchron )
    {
        if( mnUpdateTagsEvent )
            Application::RemoveUserEvent( mnUpdateTagsEvent );
            
            UpdateTagsHdl(0);
    }
    else
    {
        if( !mnUpdateTagsEvent && mxView.is() )
            mnUpdateTagsEvent = Application::PostUserEvent( LINK( this, AnnotationManagerImpl, UpdateTagsHdl ) );
    }
}

IMPL_LINK(AnnotationManagerImpl,UpdateTagsHdl, void *, EMPTYARG)
{
    mnUpdateTagsEvent  = 0;
    DisposeTags();
        
    if( mbShowAnnotations )
        CreateTags();
        
    if(  mrBase.GetDrawView() )
        static_cast< ::sd::View* >( mrBase.GetDrawView() )->updateHandles();        

    invalidateSlots();
    
    return 0;
}

void AnnotationManagerImpl::CreateTags()
{

    if( mxCurrentPage.is() && mpDoc ) try
    {
	    int nIndex = 1;
	    maFont = Application::GetSettings().GetStyleSettings().GetAppFont();

        rtl::Reference< AnnotationTag > xSelectedTag;
        
	    Reference< XAnnotationEnumeration > xEnum( mxCurrentPage->createAnnotationEnumeration() );
	    while( xEnum->hasMoreElements() )
	    {
	        Reference< XAnnotation > xAnnotation( xEnum->nextElement() );
	        Color aColor( GetColorLight( mpDoc->GetAnnotationAuthorIndex( xAnnotation->getAuthor() ) ) );
		    rtl::Reference< AnnotationTag > xTag( new AnnotationTag( *this, *mrBase.GetMainViewShell()->GetView(), xAnnotation, aColor, nIndex++, maFont  ) );
		    maTagVector.push_back(xTag);
		    
		    if( xAnnotation == mxSelectedAnnotation )
		    {
		        xSelectedTag = xTag;
		    }
	    }
	    
	    if( xSelectedTag.is() )
	    {
       		SmartTagReference xTag( xSelectedTag.get() );      	    
            mrBase.GetMainViewShell()->GetView()->getSmartTags().select( xTag );
	    }
	    else
	    {
	        // no tag, no selection!
	        mxSelectedAnnotation.clear();
	    }
    }
    catch( Exception& )
    {
        DBG_ERROR( "sd::AnnotationManagerImpl::onSelectionChanged(), exception caught!" );
    }
}

// --------------------------------------------------------------------

void AnnotationManagerImpl::DisposeTags()
{
	if( !maTagVector.empty() )
	{
		AnnotationTagVector::iterator iter = maTagVector.begin();
		do
		{
			(*iter++)->Dispose();
		}
		while( iter != maTagVector.end() );
		
		maTagVector.clear();
	}
}

// --------------------------------------------------------------------

void AnnotationManagerImpl::addListener()
{
	Link aLink( LINK(this,AnnotationManagerImpl,EventMultiplexerListener) );
    mrBase.GetEventMultiplexer()->AddEventListener (
        aLink,
        tools::EventMultiplexerEvent::EID_EDIT_VIEW_SELECTION
        | tools::EventMultiplexerEvent::EID_CURRENT_PAGE
        | tools::EventMultiplexerEvent::EID_MAIN_VIEW_REMOVED
        | tools::EventMultiplexerEvent::EID_MAIN_VIEW_ADDED);
}

// --------------------------------------------------------------------

void AnnotationManagerImpl::removeListener()
{
	Link aLink( LINK(this,AnnotationManagerImpl,EventMultiplexerListener) );
    mrBase.GetEventMultiplexer()->RemoveEventListener( aLink );
}

// --------------------------------------------------------------------

IMPL_LINK(AnnotationManagerImpl,EventMultiplexerListener,
    tools::EventMultiplexerEvent*,pEvent)
{
    switch (pEvent->meEventId)
    {
        case tools::EventMultiplexerEvent::EID_CURRENT_PAGE:
        case tools::EventMultiplexerEvent::EID_EDIT_VIEW_SELECTION:
            onSelectionChanged();
            break;

        case tools::EventMultiplexerEvent::EID_MAIN_VIEW_REMOVED:
            mxView = Reference<XDrawView>();
            onSelectionChanged();
            break;

        case tools::EventMultiplexerEvent::EID_MAIN_VIEW_ADDED:
            mxView = Reference<XDrawView>::query( mrBase.GetController() );
            onSelectionChanged();
            break;
    }
    return 0;
}

OUString AnnotationManagerImpl::GetHelpText( ::com::sun::star::uno::Reference< ::com::sun::star::office::XAnnotation >& xAnnotation )
{
    OUString sRet;
    if( xAnnotation.is() )
    {
	    OUString sAuthor( xAnnotation->getAuthor() );
	    if( sAuthor.getLength() != 0 )
	    {        
	        sRet += sAuthor;
	    }
	    sRet += OUString( RTL_CONSTASCII_USTRINGPARAM( " [" ) );
	    
	    sRet += getAnnotationDateTimeString( xAnnotation );
	    sRet += OUString( RTL_CONSTASCII_USTRINGPARAM( "]\n" ) );
	    
	    Reference< XText > xText( xAnnotation->getTextRange() );
	    if( xText.is() )
	        sRet += xText->getString();
    }
    
    return sRet;
}


void AnnotationManagerImpl::ExecuteAnnotationContextMenu( Reference< XAnnotation > xAnnotation, ::Window* pParent, const Rectangle& rContextRect, bool bButtonMenu /* = false */ )
{
    SfxDispatcher* pDispatcher( getDispatcher( mrBase ) );
    if( !pDispatcher )
        return;
    
    const bool bReadOnly = mrBase.GetDocShell()->IsReadOnly();
    
    AnnotationWindow* pAnnotationWindow = bButtonMenu ? 0 : dynamic_cast< AnnotationWindow* >( pParent );
    
    if( bReadOnly && !pAnnotationWindow )
        return;
   
	std::auto_ptr< PopupMenu > pMenu( new PopupMenu( SdResId( pAnnotationWindow ? RID_ANNOTATION_CONTEXTMENU : RID_ANNOTATION_TAG_CONTEXTMENU ) ) );

    SvtUserOptions aUserOptions;
    OUString sCurrentAuthor( aUserOptions.GetFullName() );	
    OUString sAuthor( xAnnotation->getAuthor() );
    
    String aStr( pMenu->GetItemText( SID_DELETEALLBYAUTHOR_POSTIT ) ), aReplace( sAuthor );
    if( aReplace.Len() == 0 )
        aReplace = String( SdResId( STR_ANNOTATION_NOAUTHOR ) );
    aStr.SearchAndReplaceAscii("%1", aReplace);
    pMenu->SetItemText( SID_DELETEALLBYAUTHOR_POSTIT, aStr );
    pMenu->EnableItem( SID_REPLYTO_POSTIT, (sAuthor != sCurrentAuthor) && !bReadOnly );
    pMenu->EnableItem( SID_DELETE_POSTIT, (xAnnotation.is() && !bReadOnly) ? TRUE : FALSE );  
    pMenu->EnableItem( SID_DELETEALLBYAUTHOR_POSTIT, !bReadOnly );  
    pMenu->EnableItem( SID_DELETEALL_POSTIT, !bReadOnly );  

    if( pAnnotationWindow )
    {
        if( pAnnotationWindow->IsProtected() || bReadOnly )
        {
            pMenu->EnableItem( SID_ATTR_CHAR_WEIGHT, FALSE );
            pMenu->EnableItem( SID_ATTR_CHAR_POSTURE, FALSE );
            pMenu->EnableItem( SID_ATTR_CHAR_UNDERLINE, FALSE );
            pMenu->EnableItem( SID_ATTR_CHAR_STRIKEOUT, FALSE );
            pMenu->EnableItem( SID_PASTE, FALSE );
        }
        else
        {
            SfxItemSet aSet(pAnnotationWindow->getView()->GetAttribs());
            
	        if ( aSet.GetItemState( EE_CHAR_WEIGHT ) == SFX_ITEM_ON )
	        {
		        if( ((const SvxWeightItem&)aSet.Get( EE_CHAR_WEIGHT )).GetWeight() == WEIGHT_BOLD )
		            pMenu->CheckItem( SID_ATTR_CHAR_WEIGHT );
		    }
    		
	        if ( aSet.GetItemState( EE_CHAR_ITALIC ) == SFX_ITEM_ON )
	        {
		        if( ((const SvxPostureItem&)aSet.Get( EE_CHAR_ITALIC )).GetPosture() != ITALIC_NONE )
		            pMenu->CheckItem( SID_ATTR_CHAR_POSTURE );
    		    
		    }
	        if ( aSet.GetItemState( EE_CHAR_UNDERLINE ) == SFX_ITEM_ON )
	        {
		        if( ((const SvxUnderlineItem&)aSet.Get( EE_CHAR_UNDERLINE )).GetLineStyle() != UNDERLINE_NONE )
		            pMenu->CheckItem( SID_ATTR_CHAR_UNDERLINE );
	        }

	        if ( aSet.GetItemState( EE_CHAR_STRIKEOUT ) == SFX_ITEM_ON )
	        {
		        if( ((const SvxCrossedOutItem&)aSet.Get( EE_CHAR_STRIKEOUT )).GetStrikeout() != STRIKEOUT_NONE )
		            pMenu->CheckItem( SID_ATTR_CHAR_STRIKEOUT );
	        }
            TransferableDataHelper aDataHelper( TransferableDataHelper::CreateFromSystemClipboard( pAnnotationWindow ) );
            pMenu->EnableItem( SID_PASTE, aDataHelper.GetFormatCount() != 0 );
	    }
        
        pMenu->EnableItem( SID_COPY, pAnnotationWindow->getView()->HasSelection() );
    }

    USHORT nId = 0;
    
    // set slot images
    Reference< ::com::sun::star::frame::XFrame > xFrame( mrBase.GetMainViewShell()->GetViewFrame()->GetFrame()->GetFrameInterface() );
    if( xFrame.is() )
    {
        const bool bHighContrast = Application::GetSettings().GetStyleSettings().GetHighContrastMode();
        for( USHORT nPos = 0; nPos < pMenu->GetItemCount(); nPos++ )
        {
            nId = pMenu->GetItemId( nPos );    
            if( pMenu->IsItemEnabled( nId ) )
            {
                OUString sSlotURL( RTL_CONSTASCII_USTRINGPARAM( "slot:" ));
                sSlotURL += OUString::valueOf( sal_Int32( nId ));
                
                Image aImage( GetImage( xFrame, sSlotURL, false, bHighContrast ) );
                if( !!aImage )
                    pMenu->SetItemImage( nId, aImage );
            }
        }
    }

    nId = pMenu->Execute( pParent, rContextRect, POPUPMENU_EXECUTE_DOWN|POPUPMENU_NOMOUSEUPCLOSE );
	switch( nId )
	{
    case SID_REPLYTO_POSTIT:
    {
		const SfxUnoAnyItem aItem( SID_REPLYTO_POSTIT, Any( xAnnotation ) );
		pDispatcher->Execute( SID_REPLYTO_POSTIT, SFX_CALLMODE_ASYNCHRON, &aItem, 0 );
        break;
    }
    case SID_DELETE_POSTIT:
    {
		const SfxUnoAnyItem aItem( SID_DELETE_POSTIT, Any( xAnnotation ) );
		pDispatcher->Execute( SID_DELETE_POSTIT, SFX_CALLMODE_ASYNCHRON, &aItem, 0 );
        break;
    }
    case SID_DELETEALLBYAUTHOR_POSTIT:
    {
		const SfxStringItem aItem( SID_DELETEALLBYAUTHOR_POSTIT, sAuthor );
		pDispatcher->Execute( SID_DELETEALLBYAUTHOR_POSTIT, SFX_CALLMODE_ASYNCHRON, &aItem, 0 );
        break;
    }
    case SID_DELETEALL_POSTIT:
        pDispatcher->Execute( SID_DELETEALL_POSTIT );
        break;	
    case SID_COPY:
    case SID_PASTE:
    case SID_ATTR_CHAR_WEIGHT: 
    case SID_ATTR_CHAR_POSTURE:
    case SID_ATTR_CHAR_UNDERLINE:
    case SID_ATTR_CHAR_STRIKEOUT:
        if( pAnnotationWindow )
            pAnnotationWindow->ExecuteSlot( nId );
        break;
	}
}

// ====================================================================

Color AnnotationManagerImpl::GetColor(sal_uInt16 aAuthorIndex)
{
	if (!Application::GetSettings().GetStyleSettings().GetHighContrastMode())
	{
		static const Color aArrayNormal[] = {
			COL_AUTHOR1_NORMAL,		COL_AUTHOR2_NORMAL,		COL_AUTHOR3_NORMAL,
			COL_AUTHOR4_NORMAL,		COL_AUTHOR5_NORMAL,		COL_AUTHOR6_NORMAL,
			COL_AUTHOR7_NORMAL,		COL_AUTHOR8_NORMAL,		COL_AUTHOR9_NORMAL };

		return Color( aArrayNormal[ aAuthorIndex % (sizeof( aArrayNormal )/ sizeof( aArrayNormal[0] ))]);
	}
	else
		return Color(COL_WHITE);
}

Color AnnotationManagerImpl::GetColorLight(sal_uInt16 aAuthorIndex)
{
	if (!Application::GetSettings().GetStyleSettings().GetHighContrastMode())
	{
		static const Color aArrayLight[] = {
			COL_AUTHOR1_LIGHT,		COL_AUTHOR2_LIGHT,		COL_AUTHOR3_LIGHT,
			COL_AUTHOR4_LIGHT,		COL_AUTHOR5_LIGHT,		COL_AUTHOR6_LIGHT,
			COL_AUTHOR7_LIGHT,		COL_AUTHOR8_LIGHT,		COL_AUTHOR9_LIGHT };

		return Color( aArrayLight[ aAuthorIndex % (sizeof( aArrayLight )/ sizeof( aArrayLight[0] ))]);
	}
	else
		return Color(COL_WHITE);
}

Color AnnotationManagerImpl::GetColorDark(sal_uInt16 aAuthorIndex)
{
	if (!Application::GetSettings().GetStyleSettings().GetHighContrastMode())
	{
		static const Color aArrayAnkor[] = {
			COL_AUTHOR1_DARK,		COL_AUTHOR2_DARK,		COL_AUTHOR3_DARK,
			COL_AUTHOR4_DARK,		COL_AUTHOR5_DARK,		COL_AUTHOR6_DARK,
			COL_AUTHOR7_DARK,		COL_AUTHOR8_DARK,		COL_AUTHOR9_DARK };

		return Color( aArrayAnkor[  aAuthorIndex % (sizeof( aArrayAnkor )	/ sizeof( aArrayAnkor[0] ))]);
	}
	else
		return Color(COL_WHITE);
}

SdPage* AnnotationManagerImpl::GetNextPage( SdPage* pPage, bool bForeward )
{
    if( pPage == 0 )
        return bForeward ? GetFirstPage() : GetLastPage();
        
    sal_uInt16 nPageNum = (pPage->GetPageNum() - 1) >> 1;

    // first all non master pages
    if( !pPage->IsMasterPage() )
    {
        if( bForeward )
        {            
            if( nPageNum >= mpDoc->GetSdPageCount(PK_STANDARD)-1 )
            {
                // we reached end of draw pages, start with master pages (skip handout master for draw)
                return mpDoc->GetMasterSdPage( (mpDoc->GetDocumentType() == DOCUMENT_TYPE_IMPRESS) ? 0 : 1, PK_STANDARD );
            }
            nPageNum++;
        }
        else
        {
            if( nPageNum == 0 )
                return 0; // we are already on the first draw page, finished
                
            nPageNum--;            
        }
        return mpDoc->GetSdPage(nPageNum, PK_STANDARD);
    }
    else
    {
        if( bForeward )
        {            
            if( nPageNum >= mpDoc->GetMasterSdPageCount(PK_STANDARD)-1 )
            {
                return 0;   // we reached the end, there is nothing more to see here
            }
            nPageNum++;
        }
        else
        {
            if( nPageNum == (mpDoc->GetDocumentType() == DOCUMENT_TYPE_IMPRESS) ? 0 : 1 )
            {
                // we reached beginning of master pages, start with end if pages
                return mpDoc->GetSdPage( mpDoc->GetSdPageCount(PK_STANDARD)-1, PK_STANDARD );
            }        
                
            nPageNum--;            
        }
        return mpDoc->GetMasterSdPage(nPageNum,PK_STANDARD);
    }       
}

SdPage* AnnotationManagerImpl::GetFirstPage()
{
    // return first drawing page
    return mpDoc->GetSdPage(0, PK_STANDARD );
}

SdPage* AnnotationManagerImpl::GetLastPage()
{
    return mpDoc->GetMasterSdPage( mpDoc->GetMasterSdPageCount(PK_STANDARD) - 1, PK_STANDARD );
}

SdPage* AnnotationManagerImpl::GetCurrentPage()
{
/*
    ::boost::shared_ptr<DrawViewShell> pDrawViewShell(::boost::dynamic_pointer_cast<DrawViewShell>(mrBase.GetMainViewShell()));
    if (pDrawViewShell.get() != NULL)
        return pDrawViewShell->GetActualPage();
*/
    return mrBase.GetMainViewShell()->getCurrentPage();
}

// ====================================================================

AnnotationManager::AnnotationManager( ViewShellBase& rViewShellBase )
: mxImpl( new AnnotationManagerImpl( rViewShellBase ) )
{
    mxImpl->init();
}

AnnotationManager::~AnnotationManager()
{
    mxImpl->dispose();
}

void AnnotationManager::ExecuteAnnotation(SfxRequest& rRequest)
{
    mxImpl->ExecuteAnnotation( rRequest );
}

void AnnotationManager::GetAnnotationState(SfxItemSet& rItemSet)
{
    mxImpl->GetAnnotationState(rItemSet);
}

}
