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


#include "fupoor.hxx"

#include <svx/svxids.hrc>
#include <svtools/aeitem.hxx>
#include <svx/svdpagv.hxx>
#include <svx/svdoole2.hxx>
#include <svx/svdograf.hxx>
#include <vcl/seleng.hxx>
#include <sfx2/app.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/request.hxx>
#include <vcl/dialog.hxx>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/drawing/XLayer.hpp>
#include <com/sun/star/drawing/XLayerManager.hpp>
#include <com/sun/star/container/XChild.hpp>

#ifndef SD_FRAMW_VIEW_HXX
#include "FrameView.hxx"
#endif
#include "app.hrc"
#include "fusel.hxx"
#include "sdpage.hxx"
#include "drawview.hxx"
#include "DrawViewShell.hxx"
#ifndef SD_WINDOW_SHELL_HXX
#include "Window.hxx"
#endif
#include "drawdoc.hxx"
#include "DrawDocShell.hxx"
#include "zoomlist.hxx"
#include "Client.hxx"
#include "slideshow.hxx"
#include "LayerTabBar.hxx"

#include <sfx2/viewfrm.hxx>

// #97016# IV
#include <svx/svditer.hxx>

// #98533#
#include <svx/editeng.hxx>

using namespace ::com::sun::star;
using ::com::sun::star::uno::Reference;

namespace sd {

TYPEINIT0( FuPoor );

/*************************************************************************
|*
|* Konstruktor
|*
\************************************************************************/

FuPoor::FuPoor (
    ViewShell* pViewSh,
    ::sd::Window* pWin,
    ::sd::View* pView,
    SdDrawDocument* pDrDoc,
    SfxRequest& rReq)
    : mpView(pView),
	  mpViewShell(pViewSh),
	  mpWindow(pWin),
	  mpDocSh( pDrDoc->GetDocSh() ),
	  mpDoc(pDrDoc),
      nSlotId( rReq.GetSlot() ),
	  nSlotValue(0),
	  pDialog(NULL),
	  bIsInDragMode(FALSE),
	  bNoScrollUntilInside (TRUE),
	  bScrollable (FALSE),
	  bDelayActive (FALSE),
	  // #95491# remember MouseButton state
	  mnCode(0)
{
	ReceiveRequest(rReq);

	aScrollTimer.SetTimeoutHdl( LINK(this, FuPoor, ScrollHdl) );
	aScrollTimer.SetTimeout(SELENG_AUTOREPEAT_INTERVAL);

	aDragTimer.SetTimeoutHdl( LINK(this, FuPoor, DragHdl) );
	aDragTimer.SetTimeout(SELENG_DRAGDROP_TIMEOUT);

	aDelayToScrollTimer.SetTimeoutHdl( LINK(this, FuPoor, DelayHdl) );
	aDelayToScrollTimer.SetTimeout(2000);
}

/*************************************************************************
|*
|* Destruktor
|*
\************************************************************************/

FuPoor::~FuPoor()
{
	aDragTimer.Stop();
	aScrollTimer.Stop();
	aDelayToScrollTimer.Stop ();

	if (pDialog)
		delete pDialog;
}

/*************************************************************************
|*
|* Function aktivieren
|*
\************************************************************************/

void FuPoor::Activate()
{
	if (pDialog)
	{
		pDialog->Show();
	}
}

/*************************************************************************
|*
|* Function deaktivieren
|*
\************************************************************************/

void FuPoor::Deactivate()
{
	aDragTimer.Stop();
	aScrollTimer.Stop();
	aDelayToScrollTimer.Stop ();
		bScrollable  =
		bDelayActive = FALSE;

	if (pDialog)
	{
		pDialog->Hide();
	}

	if (mpWindow) mpWindow->ReleaseMouse ();
}

/*************************************************************************
|*
|* Scrollen bei Erreichen des Fensterrandes; wird von
|* MouseMove aufgerufen
|*
\************************************************************************/

void FuPoor::ForceScroll(const Point& aPixPos)
{
	aScrollTimer.Stop();

	if ( !mpView->IsDragHelpLine() && !mpView->IsSetPageOrg() &&
			!SlideShow::IsRunning( mpViewShell->GetViewShellBase() ) )
	{
/*		Size aSize = mpWindow->GetSizePixel();
		short dx = 0, dy = 0;

		if ( aPixPos.X() <= 0			   ) dx = -1;
		if ( aPixPos.X() >= aSize.Width()  ) dx =  1;
		if ( aPixPos.Y() <= 0			   ) dy = -1;
		if ( aPixPos.Y() >= aSize.Height() ) dy =  1;
*/
		Point aPos = mpWindow->OutputToScreenPixel(aPixPos);
		const Rectangle& rRect = mpViewShell->GetAllWindowRect();

		if ( bNoScrollUntilInside )
		{
			if ( rRect.IsInside(aPos) )
				bNoScrollUntilInside = FALSE;
		}
		else
		{
			short dx = 0, dy = 0;

			if ( aPos.X() <= rRect.Left()	) dx = -1;
			if ( aPos.X() >= rRect.Right()	) dx =	1;
			if ( aPos.Y() <= rRect.Top()	) dy = -1;
			if ( aPos.Y() >= rRect.Bottom() ) dy =	1;

			if ( dx != 0 || dy != 0 )
			{
				if (bScrollable)
				{
					// Scrollaktion in abgeleiteter Klasse
					ScrollStart();
					mpViewShell->ScrollLines(dx, dy);
					ScrollEnd();
					aScrollTimer.Start();
				}
				else if (! bDelayActive) StartDelayToScrollTimer ();
			}
		}
	}
}

/*************************************************************************
|*
|* Timer-Handler fuer Fensterscrolling
|*
\************************************************************************/

IMPL_LINK_INLINE_START( FuPoor, ScrollHdl, Timer *, EMPTYARG )
{
	Point aPnt(mpWindow->GetPointerPosPixel());
	
	// #95491# use remembered MouseButton state to create correct
	// MouseEvents for this artifical MouseMove.
	MouseMove(MouseEvent(aPnt, 1, 0, GetMouseButtonCode()));

	return 0;
}
IMPL_LINK_INLINE_END( FuPoor, ScrollHdl, Timer *, pTimer )

/*************************************************************************
|*
|* String in Applikations-Statuszeile ausgeben
|*
\************************************************************************/

void FuPoor::WriteStatus(const String& )
{
}

/*************************************************************************
|*
|* Tastaturereignisse bearbeiten
|*
|* Wird ein KeyEvent bearbeitet, so ist der Return-Wert TRUE, andernfalls
|* FALSE.
|*
\************************************************************************/

BOOL FuPoor::KeyInput(const KeyEvent& rKEvt)
{
	USHORT          nCode = rKEvt.GetKeyCode().GetCode();
	BOOL            bReturn = FALSE;
	BOOL            bSlideShow = SlideShow::IsRunning( mpViewShell->GetViewShellBase() );

	switch (nCode)
	{
		// #97016# IV
		case KEY_RETURN:
		{
			if(rKEvt.GetKeyCode().IsMod1())
			{
				if(mpViewShell && mpViewShell->ISA(DrawViewShell))
				{
					DrawViewShell* pDrawViewShell = 
                        static_cast<DrawViewShell*>(mpViewShell);
					SdPage* pActualPage = pDrawViewShell->GetActualPage();
					SdrTextObj* pCandidate = 0L;

					if(pActualPage)
					{
						SdrObjListIter aIter(*pActualPage, IM_DEEPNOGROUPS);

						while(aIter.IsMore() && !pCandidate)
						{
							SdrObject* pObj = aIter.Next();

							if(pObj && pObj->ISA(SdrTextObj))
							{
								sal_uInt32 nInv(pObj->GetObjInventor());
								sal_uInt16 nKnd(pObj->GetObjIdentifier());

								if(SdrInventor == nInv && 
									(OBJ_TITLETEXT == nKnd || OBJ_OUTLINETEXT == nKnd || OBJ_TEXT == nKnd))
								{
									pCandidate = (SdrTextObj*)pObj;
								}
							}
						}
					}

					if(pCandidate)
					{
						mpView->UnMarkAll();
						mpView->MarkObj(pCandidate, mpView->GetSdrPageView());

						mpViewShell->GetViewFrame()->GetDispatcher()->Execute(
							SID_ATTR_CHAR, SFX_CALLMODE_ASYNCHRON);
					}
					else
					{
						// insert a new page with the same page layout
						mpViewShell->GetViewFrame()->GetDispatcher()->Execute(
							SID_INSERTPAGE_QUICK, SFX_CALLMODE_ASYNCHRON);
					}

					// consumed
					bReturn = TRUE;
				}
			}
			else
			{
                // #98255# activate OLE object on RETURN for selected object
				// #98198# activate text edit on RETURN for selected object
				const SdrMarkList& rMarkList = mpView->GetMarkedObjectList();
	
				if( !mpView->IsTextEdit() && 1 == rMarkList.GetMarkCount() )
				{
                    SdrObject* pObj = rMarkList.GetMark( 0 )->GetMarkedSdrObj();

        			if( pObj && pObj->ISA( SdrOle2Obj ) && !mpDocSh->IsUIActive() )
			        {
                        //HMHmpView->HideMarkHdl();
                        mpViewShell->ActivateObject( static_cast< SdrOle2Obj* >( pObj ), 0 );
			        }
			        else if( pObj && pObj->IsEmptyPresObj() && pObj->ISA( SdrGrafObj ) )
			        {
				        mpViewShell->GetViewFrame()->GetDispatcher()->Execute( SID_INSERT_GRAPHIC, SFX_CALLMODE_ASYNCHRON | SFX_CALLMODE_RECORD );
			        }
                    else
                    {
    					mpViewShell->GetViewFrame()->GetDispatcher()->Execute( SID_ATTR_CHAR, SFX_CALLMODE_ASYNCHRON | SFX_CALLMODE_RECORD );
                    }

					// consumed
					bReturn = TRUE;
				}
			}
		}
		break;

		// #97016# II
		case KEY_TAB:
		{
			// #98994# handle Mod1 and Mod2 to get travelling running on different systems
			if(rKEvt.GetKeyCode().IsMod1() || rKEvt.GetKeyCode().IsMod2())
			{
				// #97016# II do something with a selected handle?
				const SdrHdlList& rHdlList = mpView->GetHdlList();
				sal_Bool bForward(!rKEvt.GetKeyCode().IsShift());

				((SdrHdlList&)rHdlList).TravelFocusHdl(bForward);

				// guarantee visibility of focused handle
				SdrHdl* pHdl = rHdlList.GetFocusHdl();

				if(pHdl)
				{
					Point aHdlPosition(pHdl->GetPos());
					Rectangle aVisRect(aHdlPosition - Point(100, 100), Size(200, 200));
					mpView->MakeVisible(aVisRect, *mpWindow);
				}

				// consumed
				bReturn = TRUE;
			}
		}
		break;
	
		case KEY_ESCAPE:
		{
			bReturn = FuPoor::cancel();
		}
		break;

		case KEY_ADD:
		{
			if (!mpView->IsTextEdit() && !bSlideShow && !mpDocSh->IsUIActive())
			{
				// Zoom vergroessern
				mpViewShell->SetZoom(mpWindow->GetZoom() * 3 / 2);

				if (mpViewShell->ISA(DrawViewShell))
					static_cast<DrawViewShell*>(mpViewShell)
                        ->SetZoomOnPage(FALSE);

				bReturn = TRUE;
			}
		}
		break;

		case KEY_SUBTRACT:
		{
			if (!mpView->IsTextEdit() && !bSlideShow && !mpDocSh->IsUIActive())
			{
				// Zoom verringern
				mpViewShell->SetZoom(mpWindow->GetZoom() * 2 / 3);

				if (mpViewShell->ISA(DrawViewShell))
					static_cast<DrawViewShell*>(mpViewShell)
                        ->SetZoomOnPage(FALSE);

				bReturn = TRUE;
			}
		}
		break;

		case KEY_MULTIPLY:
		{
			if (!mpView->IsTextEdit() && !bSlideShow)
			{
				// Zoom auf Seite
				mpViewShell->GetViewFrame()->GetDispatcher()->
				Execute(SID_SIZE_PAGE, SFX_CALLMODE_ASYNCHRON | SFX_CALLMODE_RECORD);
				bReturn = TRUE;
			}
		}
		break;

		case KEY_DIVIDE:
		{
			if (!mpView->IsTextEdit() && !bSlideShow)
			{
				// Zoom auf selektierte Objekte
				mpViewShell->GetViewFrame()->GetDispatcher()->
				Execute(SID_SIZE_OPTIMAL, SFX_CALLMODE_ASYNCHRON | SFX_CALLMODE_RECORD);
				bReturn = TRUE;
			}
		}
		break;

		case KEY_POINT:
		{
			ZoomList* pZoomList = mpViewShell->GetZoomList();

			if (!mpView->IsTextEdit() && pZoomList->IsNextPossible() && !bSlideShow && !mpDocSh->IsUIActive())
			{
				// Naechstes ZoomRect einstellen
				mpViewShell->SetZoomRect(pZoomList->GetNextZoomRect());
				bReturn = TRUE;
			}
		}
		break;

		case KEY_COMMA:
		{
			ZoomList* pZoomList = mpViewShell->GetZoomList();

			if (!mpView->IsTextEdit() && pZoomList->IsPreviousPossible() && !bSlideShow && !mpDocSh->IsUIActive())
			{
				// Vorheriges ZoomRect einstellen
				mpViewShell->SetZoomRect(pZoomList->GetPreviousZoomRect());
				bReturn = TRUE;
			}
		}
		break;

		case KEY_HOME:
		{
			if (!mpView->IsTextEdit() 
                && mpViewShell->ISA(DrawViewShell) 
                && !bSlideShow)
			{
			   // Sprung zu erster Seite
			   static_cast<DrawViewShell*>(mpViewShell)->SwitchPage(0);
			   bReturn = TRUE;
			}
		}
		break;

		case KEY_END:
		{
			if (!mpView->IsTextEdit() 
                && mpViewShell->ISA(DrawViewShell) 
                && !bSlideShow)
			{
				// Sprung zu letzter Seite
				SdPage* pPage = 
                    static_cast<DrawViewShell*>(mpViewShell)->GetActualPage();
				static_cast<DrawViewShell*>(mpViewShell)
                    ->SwitchPage(mpDoc->GetSdPageCount(
                        pPage->GetPageKind()) - 1);
				bReturn = TRUE;
			}
		}
		break;

		case KEY_PAGEUP:
		{
		    if( rKEvt.GetKeyCode().IsMod1() && rKEvt.GetKeyCode().IsMod2() )
		        break;
		
			if(mpViewShell->ISA(DrawViewShell) && !bSlideShow)
			{
                // The page-up key switches layers or pages depending on the
                // modifier key.
                if ( ! rKEvt.GetKeyCode().GetAllModifier())
                {
                    // With no modifier pressed we move to the previous
                    // slide.
                    mpView->SdrEndTextEdit();
		
                    // Previous page.
                    bReturn = TRUE;
                    SdPage* pPage = static_cast<DrawViewShell*>(mpViewShell)->GetActualPage();
                    USHORT nSdPage = (pPage->GetPageNum() - 1) / 2;
                        
                    if (nSdPage > 0)
                    {
                        // Switch the page and send events regarding
                        // deactivation the old page and activating the new
                        // one.
                        TabControl* pPageTabControl =
                            static_cast<DrawViewShell*>(mpViewShell)
                            ->GetPageTabControl();
                        if (pPageTabControl->IsReallyShown())
                            pPageTabControl->SendDeactivatePageEvent ();
                        static_cast<DrawViewShell*>(mpViewShell)->SwitchPage(nSdPage - 1);
                        if (pPageTabControl->IsReallyShown())
                            pPageTabControl->SendActivatePageEvent ();
                    }
                }
                else if (rKEvt.GetKeyCode().IsMod1())
                {
                    // With the CONTROL modifier we switch layers.
                    if (static_cast<DrawViewShell*>(mpViewShell)->IsLayerModeActive())
                    {
                        // Moves to the previous layer.
                        SwitchLayer (-1);
                    }
                }
            }
		}
		break;

		case KEY_PAGEDOWN:
		{
		    if( rKEvt.GetKeyCode().IsMod1() && rKEvt.GetKeyCode().IsMod2() )
		        break;
            if(mpViewShell->ISA(DrawViewShell) && !bSlideShow)
            {
                // The page-down key switches layers or pages depending on the
                // modifier key.
                if ( ! rKEvt.GetKeyCode().GetAllModifier())
                {
                    // With no modifier pressed we move to the next slide.                      
                    mpView->SdrEndTextEdit();
                    
                    // Next page.
                    bReturn = TRUE;
                    SdPage* pPage = static_cast<DrawViewShell*>(mpViewShell)->GetActualPage();
                    USHORT nSdPage = (pPage->GetPageNum() - 1) / 2;
                        
                    if (nSdPage < mpDoc->GetSdPageCount(pPage->GetPageKind()) - 1)
                    {
                        // Switch the page and send events regarding
                        // deactivation the old page and activating the new
                        // one.
                        TabControl* pPageTabControl =
                            static_cast<DrawViewShell*>(mpViewShell)->GetPageTabControl();
                        if (pPageTabControl->IsReallyShown())
                            pPageTabControl->SendDeactivatePageEvent ();
                        static_cast<DrawViewShell*>(mpViewShell)->SwitchPage(nSdPage + 1);
                        if (pPageTabControl->IsReallyShown())
                            pPageTabControl->SendActivatePageEvent ();
                    }
                }
                else if (rKEvt.GetKeyCode().IsMod1())
                { 
                    // With the CONTROL modifier we switch layers.
                    if (static_cast<DrawViewShell*>(mpViewShell)->IsLayerModeActive())
                    {
                        // With the layer mode active pressing page-down
                        // moves to the next layer.
                        SwitchLayer (+1);
                    }
                }
            }
        }
        break;

		// #97016# II change select state when focus is on poly point
		case KEY_SPACE:
		{
			const SdrHdlList& rHdlList = mpView->GetHdlList();
			SdrHdl* pHdl = rHdlList.GetFocusHdl();

			if(pHdl)
			{
				if(pHdl->GetKind() == HDL_POLY)
				{
					// rescue ID of point with focus
					sal_uInt32 nPol(pHdl->GetPolyNum());
					sal_uInt32 nPnt(pHdl->GetPointNum());
					
					if(mpView->IsPointMarked(*pHdl))
					{
						if(rKEvt.GetKeyCode().IsShift())
						{
							mpView->UnmarkPoint(*pHdl);
						}
					}
					else
					{
						if(!rKEvt.GetKeyCode().IsShift())
						{
							mpView->UnmarkAllPoints();
						}

						mpView->MarkPoint(*pHdl);
					}
		
					if(0L == rHdlList.GetFocusHdl())
					{
						// restore point with focus
						SdrHdl* pNewOne = 0L;

						for(sal_uInt32 a(0); !pNewOne && a < rHdlList.GetHdlCount(); a++)
						{
							SdrHdl* pAct = rHdlList.GetHdl(a);
							
							if(pAct 
								&& pAct->GetKind() == HDL_POLY 
								&& pAct->GetPolyNum() == nPol
								&& pAct->GetPointNum() == nPnt)
							{
								pNewOne = pAct;
							}
						}

						if(pNewOne)
						{
							((SdrHdlList&)rHdlList).SetFocusHdl(pNewOne);
						}
					}

					bReturn = TRUE;
				}
			}
		}
		break;

		case KEY_UP:
		case KEY_DOWN:
		case KEY_LEFT:
		case KEY_RIGHT:
		{
			if (!mpView->IsTextEdit() && !bSlideShow)
			{
				long nX = 0;
				long nY = 0;

				if (nCode == KEY_UP)
				{
					// Scroll nach oben
					nX = 0;
					nY =-1;
				}
				else if (nCode == KEY_DOWN)
				{
					// Scroll nach unten
					nX = 0;
					nY = 1;
				}
				else if (nCode == KEY_LEFT)
				{
					// Scroll nach links
					nX =-1;
					nY = 0;
				}
				else if (nCode == KEY_RIGHT)
				{
					// Scroll nach rechts
					nX = 1;
					nY = 0;
				}

				if (mpView->AreObjectsMarked() && !rKEvt.GetKeyCode().IsMod1() &&
					!mpDocSh->IsReadOnly())
				{
					// #97016# II
					const SdrHdlList& rHdlList = mpView->GetHdlList();
					SdrHdl* pHdl = rHdlList.GetFocusHdl();

					// #109007#
					sal_Bool bIsMoveOfConnectedHandle(sal_False);
					sal_Bool bOldSuppress = false;
					SdrEdgeObj* pEdgeObj = 0L;

					if(pHdl && pHdl->GetObj() && pHdl->GetObj()->ISA(SdrEdgeObj) && 0 == pHdl->GetPolyNum())
					{
						pEdgeObj = (SdrEdgeObj*)pHdl->GetObj();

						if(0L == pHdl->GetPointNum())
						{
							if(pEdgeObj->GetConnection(sal_True).GetObject())
							{
								bIsMoveOfConnectedHandle = sal_True;
							}
						}
						if(1L == pHdl->GetPointNum())
						{
							if(pEdgeObj->GetConnection(sal_False).GetObject())
							{
								bIsMoveOfConnectedHandle = sal_True;
							}
						}
					}

					// #109007#
					if(pEdgeObj)
					{
						// Suppress default connects to inside object and object center
						bOldSuppress = pEdgeObj->GetSuppressDefaultConnect();
						pEdgeObj->SetSuppressDefaultConnect(sal_True);
					}

					// #109007#
					if(bIsMoveOfConnectedHandle)
					{
						sal_uInt16 nMarkHdSiz(mpView->GetMarkHdlSizePixel());
						Size aHalfConSiz(nMarkHdSiz + 1, nMarkHdSiz + 1);
						aHalfConSiz = mpWindow->PixelToLogic(aHalfConSiz);

						if(100 < aHalfConSiz.Width())
							nX *= aHalfConSiz.Width();
						else
							nX *= 100;

						if(100 < aHalfConSiz.Height())
							nY *= aHalfConSiz.Height();
						else
							nY *= 100;
					}
					else if(rKEvt.GetKeyCode().IsMod2())
					{
						// #97016# move in 1 pixel distance
						Size aLogicSizeOnePixel = (mpWindow) ? mpWindow->PixelToLogic(Size(1,1)) : Size(100, 100);
						nX *= aLogicSizeOnePixel.Width();
						nY *= aLogicSizeOnePixel.Height();
					}
					else if(rKEvt.GetKeyCode().IsShift())
					{
						nX *= 1000;
						nY *= 1000;
					}
					else
					{
						// old, fixed move distance
						nX *= 100;
						nY *= 100;
					}

					if(0L == pHdl)
					{
						// #67368# only take action when move is allowed
						if(mpView->IsMoveAllowed())
						{
							// #90129# restrict movement to WorkArea
							const Rectangle& rWorkArea = mpView->GetWorkArea();

							if(!rWorkArea.IsEmpty())
							{
								Rectangle aMarkRect(mpView->GetMarkedObjRect());
								aMarkRect.Move(nX, nY);

								if(!aMarkRect.IsInside(rWorkArea))
								{
									if(aMarkRect.Left() < rWorkArea.Left())
									{
										nX += rWorkArea.Left() - aMarkRect.Left();
									}

									if(aMarkRect.Right() > rWorkArea.Right())
									{
										nX -= aMarkRect.Right() - rWorkArea.Right();
									}

									if(aMarkRect.Top() < rWorkArea.Top())
									{
										nY += rWorkArea.Top() - aMarkRect.Top();
									}

									if(aMarkRect.Bottom() > rWorkArea.Bottom())
									{
										nY -= aMarkRect.Bottom() - rWorkArea.Bottom();
									}
								}
							}

							// no handle selected
							if(0 != nX || 0 != nY)
							{
								mpView->MoveAllMarked(Size(nX, nY));

								// #97016# II
								mpView->MakeVisible(mpView->GetAllMarkedRect(), *mpWindow);
							}
						}
					}
					else
					{
						// move handle with index nHandleIndex
						if(pHdl && (nX || nY))
						{
							// now move the Handle (nX, nY)
							Point aStartPoint(pHdl->GetPos());
							Point aEndPoint(pHdl->GetPos() + Point(nX, nY));
							const SdrDragStat& rDragStat = mpView->GetDragStat();

							// start dragging
							mpView->BegDragObj(aStartPoint, 0, pHdl, 0);

					        if(mpView->IsDragObj())
							{
								FASTBOOL bWasNoSnap = rDragStat.IsNoSnap();
								BOOL bWasSnapEnabled = mpView->IsSnapEnabled();

								// switch snapping off
								if(!bWasNoSnap)
									((SdrDragStat&)rDragStat).SetNoSnap(TRUE);
								if(bWasSnapEnabled)
									mpView->SetSnapEnabled(FALSE);

								mpView->MovAction(aEndPoint);
								mpView->EndDragObj();
							
								// restore snap
								if(!bWasNoSnap)
									((SdrDragStat&)rDragStat).SetNoSnap(bWasNoSnap);
								if(bWasSnapEnabled)
									mpView->SetSnapEnabled(bWasSnapEnabled);
							}

							// make moved handle visible
							Rectangle aVisRect(aEndPoint - Point(100, 100), Size(200, 200));
							mpView->MakeVisible(aVisRect, *mpWindow);
						}
					}

					// #109007#
					if(pEdgeObj)
					{
						// Restore original suppress value
						pEdgeObj->SetSuppressDefaultConnect(bOldSuppress);
					}
				}
				else
				{
					// Seite scrollen
					ScrollStart();
					mpViewShell->ScrollLines(nX, nY);
					ScrollEnd();
				}

				bReturn = TRUE;
			}
		}
		break;
	}

	if (bReturn)
	{
		mpWindow->ReleaseMouse();
	}

	// #98198# when a text-editable object is selected and the
	// input character is printable, activate text edit on that object
	// and feed character to object
	if(!bReturn && !mpDocSh->IsReadOnly())
	{
		if(!mpView->IsTextEdit() && mpViewShell)
		{
			const SdrMarkList& rMarkList = mpView->GetMarkedObjectList();

			if(1 == rMarkList.GetMarkCount())
			{
				SdrObject* pObj = rMarkList.GetMark(0)->GetMarkedSdrObj();

				if(pObj->ISA(SdrTextObj) && pObj->HasTextEdit() && !pObj->ISA(SdrOle2Obj))
				{
					// #98533# use common IsSimpleCharInput from
					// the EditEngine.
					sal_Bool bPrintable(EditEngine::IsSimpleCharInput(rKEvt));
					
					if(bPrintable)
					{
						// try to activate textedit mode for the selected object
						SfxStringItem aInputString(SID_ATTR_CHAR, String(rKEvt.GetCharCode()));
						
						mpViewShell->GetViewFrame()->GetDispatcher()->Execute(
							SID_ATTR_CHAR, 
							SFX_CALLMODE_ASYNCHRON,
							&aInputString,
							0L);

						// consumed
						bReturn = TRUE;
					}
				}
			}
			else
			{
				// #99039# test if there is a title object there. If yes, try to
				// set it to edit mode and start typing...
				if(mpViewShell->ISA(DrawViewShell) 
                    && EditEngine::IsSimpleCharInput(rKEvt))
				{
					DrawViewShell* pDrawViewShell = 
                        static_cast<DrawViewShell*>(mpViewShell);
					SdPage* pActualPage = pDrawViewShell->GetActualPage();
					SdrTextObj* pCandidate = 0L;

					if(pActualPage)
					{
						SdrObjListIter aIter(*pActualPage, IM_DEEPNOGROUPS);

						while(aIter.IsMore() && !pCandidate)
						{
							SdrObject* pObj = aIter.Next();

							if(pObj && pObj->ISA(SdrTextObj))
							{
								sal_uInt32 nInv(pObj->GetObjInventor());
								sal_uInt16 nKnd(pObj->GetObjIdentifier());

								if(SdrInventor == nInv && OBJ_TITLETEXT == nKnd)
								{
									pCandidate = (SdrTextObj*)pObj;
								}
							}
						}
					}

					// when candidate found and candidate is untouched, start editing text...
					if(pCandidate && pCandidate->IsEmptyPresObj())
					{
						mpView->UnMarkAll();
						mpView->MarkObj(pCandidate, mpView->GetSdrPageView());
						SfxStringItem aInputString(SID_ATTR_CHAR, String(rKEvt.GetCharCode()));

						mpViewShell->GetViewFrame()->GetDispatcher()->Execute(
							SID_ATTR_CHAR, 
							SFX_CALLMODE_ASYNCHRON,
							&aInputString,
							0L);

						// consumed
						bReturn = TRUE;
					}
				}
			}
		}
	}

	return(bReturn);
}

BOOL FuPoor::MouseMove(const MouseEvent& )
{
	return FALSE;
}

// #97016# II
void FuPoor::SelectionHasChanged()
{
	const SdrHdlList& rHdlList = mpView->GetHdlList();
	((SdrHdlList&)rHdlList).ResetFocusHdl();
}

/*************************************************************************
|*
|* Cut object to clipboard
|*
\************************************************************************/

void FuPoor::DoCut()
{
	if (mpView)
	{
		mpView->DoCut(mpWindow);
	}
}

/*************************************************************************
|*
|* Copy object to clipboard
|*
\************************************************************************/

void FuPoor::DoCopy()
{
	if (mpView)
	{
		mpView->DoCopy(mpWindow);
	}
}

/*************************************************************************
|*
|* Paste object from clipboard
|*
\************************************************************************/

void FuPoor::DoPaste()
{
	if (mpView)
	{
		mpView->DoPaste(mpWindow);
	}
}

/*************************************************************************
|*
|* Timer-Handler fuer Drag&Drop
|*
\************************************************************************/

IMPL_LINK( FuPoor, DragHdl, Timer *, EMPTYARG )
{
	if( mpView )
	{
		USHORT nHitLog = USHORT ( mpWindow->PixelToLogic(Size(HITPIX,0)).Width() );
		SdrHdl* pHdl = mpView->PickHandle(aMDPos);

		if ( pHdl==NULL && mpView->IsMarkedHit(aMDPos, nHitLog)
			 && !mpView->IsPresObjSelected(FALSE, TRUE) )
		{
			mpWindow->ReleaseMouse();
			bIsInDragMode = TRUE;
			mpView->StartDrag( aMDPos, mpWindow );
		}
	}
	return 0;
}

/*************************************************************************
|*
|* Command-event
|*
\************************************************************************/

BOOL FuPoor::Command(const CommandEvent& rCEvt)
{
	return( mpView->Command(rCEvt,mpWindow) );
}

/*************************************************************************
|*
|* Timer-Handler fuer Fensterscrolling
|*
\************************************************************************/

IMPL_LINK_INLINE_START( FuPoor, DelayHdl, Timer *, EMPTYARG )
{
	aDelayToScrollTimer.Stop ();
	bScrollable = TRUE;

	Point aPnt(mpWindow->GetPointerPosPixel());
	
	// #95491# use remembered MouseButton state to create correct
	// MouseEvents for this artifical MouseMove.
	MouseMove(MouseEvent(aPnt, 1, 0, GetMouseButtonCode()));

	return 0;
}
IMPL_LINK_INLINE_END( FuPoor, DelayHdl, Timer *, pTimer )

/*************************************************************************
|*
|* Handler fuer Maustaste
|*
\************************************************************************/

BOOL FuPoor::MouseButtonUp (const MouseEvent& rMEvt)
{
	// #95491# remember button state for creation of own MouseEvents
	SetMouseButtonCode(rMEvt.GetButtons());

	aDelayToScrollTimer.Stop ();
	return bScrollable	= 
		bDelayActive = FALSE;
}

BOOL FuPoor::MouseButtonDown(const MouseEvent& rMEvt) 
{ 
	// #95491# remember button state for creation of own MouseEvents
	SetMouseButtonCode(rMEvt.GetButtons());

	return FALSE; 
}

/*************************************************************************
|*
|* Handler fuer Maustaste
|*
\************************************************************************/

void FuPoor::StartDelayToScrollTimer ()
{
	bDelayActive = TRUE;
	aDelayToScrollTimer.Start ();
}

/*************************************************************************
|*
|* Handler fuer Maustaste
|*
\************************************************************************/

long FuPoor::diffPoint (long pos1, long pos2)
{
	return (pos1 > pos2) ? pos1 - pos2
						 : pos2 - pos1;
}

/*************************************************************************
|*
|* Help-event
|*
\************************************************************************/

BOOL FuPoor::RequestHelp(const HelpEvent& rHEvt)
{
	BOOL bReturn = FALSE;

	SdrPageView* pPV = mpView->GetSdrPageView();

	if (pPV)
	{
		SdPage* pPage = (SdPage*) pPV->GetPage();

		if (pPage)
		{
			bReturn = pPage->RequestHelp(mpWindow, mpView, rHEvt);
		}
	}

	return(bReturn);
}

void FuPoor::Paint(const Rectangle&, ::sd::Window* )
{
}

/*************************************************************************
|*
|* Request verarbeiten
|*
\************************************************************************/

void FuPoor::ReceiveRequest(SfxRequest& rReq)
{
	const SfxItemSet* pSet = rReq.GetArgs();

	if (pSet)
	{
		if( pSet->GetItemState( nSlotId ) == SFX_ITEM_SET )
		{
			const SfxPoolItem& rItem = pSet->Get( nSlotId );

			if( rItem.ISA( SfxAllEnumItem ) )
			{
				nSlotValue = ( ( const SfxAllEnumItem& ) rItem ).GetValue();
			}
		}
	}
}

/*************************************************************************
|*
|* #97016#
|*
\************************************************************************/

SdrObject* FuPoor::CreateDefaultObject(const sal_uInt16, const Rectangle& )
{
	// empty base implementation
	return 0L;
}

void FuPoor::ImpForceQuadratic(Rectangle& rRect)
{
	if(rRect.GetWidth() > rRect.GetHeight())
	{
		rRect = Rectangle(
			Point(rRect.Left() + ((rRect.GetWidth() - rRect.GetHeight()) / 2), rRect.Top()), 
			Size(rRect.GetHeight(), rRect.GetHeight()));
	}
	else
	{
		rRect = Rectangle(
			Point(rRect.Left(), rRect.Top() + ((rRect.GetHeight() - rRect.GetWidth()) / 2)), 
			Size(rRect.GetWidth(), rRect.GetWidth()));
	}
}




void FuPoor::SwitchLayer (sal_Int32 nOffset)
{
    if(mpViewShell && mpViewShell->ISA(DrawViewShell))
    {
        DrawViewShell* pDrawViewShell = 
            static_cast<DrawViewShell*>(mpViewShell);

        // Calculate the new index.
        sal_Int32 nIndex = pDrawViewShell->GetActiveTabLayerIndex() + nOffset;

        // Make sure the new index lies inside the range of valid indices.
        if (nIndex < 0)
            nIndex = 0;
        else if (nIndex >= pDrawViewShell->GetTabLayerCount ())
            nIndex = pDrawViewShell->GetTabLayerCount() - 1;

        // Set the new active layer.
        if (nIndex != pDrawViewShell->GetActiveTabLayerIndex ())
        {
            LayerTabBar* pLayerTabControl =
                static_cast<DrawViewShell*>(mpViewShell)->GetLayerTabControl();
            if (pLayerTabControl != NULL)
                pLayerTabControl->SendDeactivatePageEvent ();

            pDrawViewShell->SetActiveTabLayerIndex (nIndex);

            if (pLayerTabControl != NULL)
                pLayerTabControl->SendActivatePageEvent ();
        }
    }
}

/** is called when the currenct function should be aborted. <p>
	This is used when a function gets a KEY_ESCAPE but can also
	be called directly.

	@returns true if a active function was aborted
*/
bool FuPoor::cancel()
{
	if ( !this->ISA(FuSelection) )
	{
		mpViewShell->GetViewFrame()->GetDispatcher()->Execute(SID_OBJECT_SELECT, SFX_CALLMODE_ASYNCHRON);
		return true;
	}

	return false;
}

// #i33136#
bool FuPoor::doConstructOrthogonal() const
{
	return (
		SID_DRAW_XLINE == nSlotId ||
		SID_DRAW_CIRCLEARC == nSlotId ||
		SID_DRAW_SQUARE == nSlotId ||
		SID_DRAW_SQUARE_NOFILL == nSlotId ||
		SID_DRAW_SQUARE_ROUND == nSlotId ||
		SID_DRAW_SQUARE_ROUND_NOFILL == nSlotId ||
		SID_DRAW_CIRCLE == nSlotId ||
		SID_DRAW_CIRCLE_NOFILL == nSlotId ||
		SID_DRAW_CIRCLEPIE == nSlotId ||
		SID_DRAW_CIRCLEPIE_NOFILL == nSlotId ||
		SID_DRAW_CIRCLECUT == nSlotId ||
		SID_DRAW_CIRCLECUT_NOFILL == nSlotId ||
		SID_DRAW_XPOLYGON == nSlotId ||
		SID_DRAW_XPOLYGON_NOFILL == nSlotId ||
		SID_3D_CUBE == nSlotId ||
		SID_3D_SPHERE == nSlotId ||
		SID_3D_SHELL == nSlotId ||
		SID_3D_HALF_SPHERE == nSlotId ||
		SID_3D_TORUS == nSlotId ||
		SID_3D_CYLINDER == nSlotId ||
		SID_3D_CONE == nSlotId ||
		SID_3D_PYRAMID == nSlotId);
}

void FuPoor::DoExecute( SfxRequest& )
{
}

} // end of namespace sd
