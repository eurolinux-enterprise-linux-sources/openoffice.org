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

#include <svx/sdr/contact/viewobjectcontactofgraphic.hxx>
#include <svx/sdr/contact/viewcontactofgraphic.hxx>
#include <svx/sdr/event/eventhandler.hxx>
#include <svx/svdograf.hxx>
#include <svx/sdr/contact/objectcontact.hxx>
#include <svx/svdmodel.hxx>
#include <svx/svdpage.hxx>

//////////////////////////////////////////////////////////////////////////////

namespace sdr
{
	namespace event
	{
		class AsynchGraphicLoadingEvent : public BaseEvent
		{
			// the ViewContactOfGraphic to work with
			sdr::contact::ViewObjectContactOfGraphic&		mrVOCOfGraphic;

		public:
			// basic constructor.
			AsynchGraphicLoadingEvent(EventHandler& rEventHandler, sdr::contact::ViewObjectContactOfGraphic& rVOCOfGraphic);

			// destructor
			virtual ~AsynchGraphicLoadingEvent();

			// the called method if the event is triggered
			virtual void ExecuteEvent();
		};

		AsynchGraphicLoadingEvent::AsynchGraphicLoadingEvent(
			EventHandler& rEventHandler, sdr::contact::ViewObjectContactOfGraphic& rVOCOfGraphic)
		:	BaseEvent(rEventHandler),
			mrVOCOfGraphic(rVOCOfGraphic)
		{
		}

		AsynchGraphicLoadingEvent::~AsynchGraphicLoadingEvent()
		{
			mrVOCOfGraphic.forgetAsynchGraphicLoadingEvent(this);
		}

		void AsynchGraphicLoadingEvent::ExecuteEvent()
		{
			mrVOCOfGraphic.doAsynchGraphicLoading();
		}
	} // end of namespace event
} // end of namespace sdr

//////////////////////////////////////////////////////////////////////////////

namespace sdr
{
	namespace contact
	{
		// Test graphics state and eventually trigger a SwapIn event or an Asynchronous
		// load event. Return value gives info if SwapIn was triggered or not
		bool ViewObjectContactOfGraphic::impPrepareGraphicWithAsynchroniousLoading()
		{
			bool bRetval(false);
			SdrGrafObj& rGrafObj = getSdrGrafObj();

			if(rGrafObj.IsSwappedOut())
			{
				if(rGrafObj.IsLinkedGraphic())
				{
					// update graphic link
					rGrafObj.ImpUpdateGraphicLink();
				}
				else
				{
					// SwapIn needs to be done. Decide if it can be done asynchronious.
					bool bSwapInAsynchronious(false);
					ObjectContact& rObjectContact = GetObjectContact();

					// only when allowed from configuration
					if(rObjectContact.IsAsynchronGraphicsLoadingAllowed())
					{
						// direct output or vdev output (PageView buffering)
						if(rObjectContact.isOutputToWindow() || rObjectContact.isOutputToVirtualDevice())
						{
							// only when no metafile recording
							if(!rObjectContact.isOutputToRecordingMetaFile())
							{
								// allow asynchronious loading
								bSwapInAsynchronious = true;
							}
						}
					}

					if(bSwapInAsynchronious)
					{
						// maybe it's on the way, then do nothing
						if(!mpAsynchLoadEvent)
						{
							// Trigger asynchronious SwapIn.
							sdr::event::TimerEventHandler& rEventHandler = rObjectContact.GetEventHandler();

                            mpAsynchLoadEvent = new sdr::event::AsynchGraphicLoadingEvent(rEventHandler, *this);
						}
					}
					else
					{
						if(rObjectContact.isOutputToPrinter())
                        {
                            // #i76395#	preview mechanism is only active if
                            // swapin is called from inside paint preparation, so mbInsidePaint
                            // has to be false to be able to print with high resolution 
							rGrafObj.ForceSwapIn();				
                        }
						else									
						{
							// SwapIn direct
							rGrafObj.mbInsidePaint = sal_True;
							rGrafObj.ForceSwapIn();
							rGrafObj.mbInsidePaint = sal_False;
						}

                        bRetval = true;
					}
				}
			}
			else
			{
				// it is not swapped out, somehow it was loaded. In that case, forget
				// about an existing triggered event
				if(mpAsynchLoadEvent)
				{
					// just delete it, this will remove it from the EventHandler and
					// will trigger forgetAsynchGraphicLoadingEvent from the destructor
					delete mpAsynchLoadEvent;
				}
			}

			return bRetval;
		}

		// Test graphics state and eventually trigger a SwapIn event. Return value 
		// gives info if SwapIn was triggered or not
		bool ViewObjectContactOfGraphic::impPrepareGraphicWithSynchroniousLoading()
		{
			bool bRetval(false);
			SdrGrafObj& rGrafObj = getSdrGrafObj();

			if(rGrafObj.IsSwappedOut())
			{
				if(rGrafObj.IsLinkedGraphic())
				{
					// update graphic link
					rGrafObj.ImpUpdateGraphicLink();
				}
				else
				{
					ObjectContact& rObjectContact = GetObjectContact();

					if(rObjectContact.isOutputToPrinter())
                    {
                        // #i76395#	preview mechanism is only active if
                        // swapin is called from inside paint preparation, so mbInsidePaint
                        // has to be false to be able to print with high resolution 
						rGrafObj.ForceSwapIn();				
                    }
					else									
					{
						// SwapIn direct
						rGrafObj.mbInsidePaint = sal_True;
						rGrafObj.ForceSwapIn();
						rGrafObj.mbInsidePaint = sal_False;
					}

                    bRetval = true;
				}
			}

			return bRetval;
		}

		// This is the call from the asynch graphic loading. This may only be called from
		// AsynchGraphicLoadingEvent::ExecuteEvent(). Do load the graphics. The event will
		// be deleted (consumed) and forgetAsynchGraphicLoadingEvent will be called.
		void ViewObjectContactOfGraphic::doAsynchGraphicLoading()
		{
			DBG_ASSERT(mpAsynchLoadEvent, "ViewObjectContactOfGraphic::doAsynchGraphicLoading: I did not trigger a event, why am i called (?)");

			// swap it in
			SdrGrafObj& rGrafObj = getSdrGrafObj();
			rGrafObj.ForceSwapIn();

			// #i103720# forget event to avoid possible deletion by the following ActionChanged call
            // which may use createPrimitive2DSequence/impPrepareGraphicWithAsynchroniousLoading again. 
            // Deletion is actally done by the scheduler who leaded to coming here
			mpAsynchLoadEvent = 0;

            // Invalidate all paint areas and check existing animation (which may have changed).
			GetViewContact().ActionChanged();
		}

		// This is the call from the destructor of the asynch graphic loading event.
		// No one else has to call this. It is needed to let this object forget about
		// the event. The parameter allows checking for the correct event.
        void ViewObjectContactOfGraphic::forgetAsynchGraphicLoadingEvent(sdr::event::AsynchGraphicLoadingEvent* pEvent)
		{
            (void) pEvent; // suppress warning

            if(mpAsynchLoadEvent)
            {
    			OSL_ENSURE(!pEvent || mpAsynchLoadEvent == pEvent, 
                    "ViewObjectContactOfGraphic::forgetAsynchGraphicLoadingEvent: Forced to forget another event then i have scheduled (?)");

                // forget event
			    mpAsynchLoadEvent = 0;
            }
		}

        SdrGrafObj& ViewObjectContactOfGraphic::getSdrGrafObj()
		{
			return static_cast< ViewContactOfGraphic& >(GetViewContact()).GetGrafObject();
		}

		drawinglayer::primitive2d::Primitive2DSequence ViewObjectContactOfGraphic::createPrimitive2DSequence(const DisplayInfo& rDisplayInfo) const
        {
            // prepare primitive generation with evtl. loading the graphic when it's swapped out
			SdrGrafObj& rGrafObj = const_cast< ViewObjectContactOfGraphic* >(this)->getSdrGrafObj();
			bool bDoAsynchronGraphicLoading(rGrafObj.GetModel() && rGrafObj.GetModel()->IsSwapGraphics());
			static bool bSuppressAsynchLoading(false);
			bool bSwapInDone(false);

			if(bDoAsynchronGraphicLoading 
				&& rGrafObj.IsSwappedOut() 
				&& rGrafObj.GetPage() 
				&& rGrafObj.GetPage()->IsMasterPage())
			{
				// #i102380# force Swap-In for GraphicObjects on MasterPage to have a nicer visualisation
				bDoAsynchronGraphicLoading = false;
			}

			if(bDoAsynchronGraphicLoading && !bSuppressAsynchLoading)
			{
				bSwapInDone = const_cast< ViewObjectContactOfGraphic* >(this)->impPrepareGraphicWithAsynchroniousLoading();
			}
			else
			{
				bSwapInDone = const_cast< ViewObjectContactOfGraphic* >(this)->impPrepareGraphicWithSynchroniousLoading();
			}

            // get return value by calling parent
    		drawinglayer::primitive2d::Primitive2DSequence xRetval = ViewObjectContactOfSdrObj::createPrimitive2DSequence(rDisplayInfo);

            if(xRetval.hasElements())
            {
                // #i103255# suppress when graphic needs draft visualisation and output
                // is for PDF export/Printer
			    const ViewContactOfGraphic& rVCOfGraphic = static_cast< const ViewContactOfGraphic& >(GetViewContact());

                if(rVCOfGraphic.visualisationUsesDraft())
                {
			        const ObjectContact& rObjectContact = GetObjectContact();

                    if(rObjectContact.isOutputToPDFFile() || rObjectContact.isOutputToPrinter())
                    {
                        xRetval = drawinglayer::primitive2d::Primitive2DSequence();
                    }
                }
            }

            // if swap in was forced only for printing, swap out again
            const bool bSwapInExclusiveForPrinting(bSwapInDone && GetObjectContact().isOutputToPrinter());

			if(bSwapInExclusiveForPrinting)
            {
                rGrafObj.ForceSwapOut();
            }

            return xRetval;
        }

		ViewObjectContactOfGraphic::ViewObjectContactOfGraphic(ObjectContact& rObjectContact, ViewContact& rViewContact)
		:	ViewObjectContactOfSdrObj(rObjectContact, rViewContact),
			mpAsynchLoadEvent(0)
		{
		}

		ViewObjectContactOfGraphic::~ViewObjectContactOfGraphic()
		{
			// evtl. delete the asynch loading event
			if(mpAsynchLoadEvent)
			{
				// just delete it, this will remove it from the EventHandler and
				// will trigger forgetAsynchGraphicLoadingEvent from the destructor
				delete mpAsynchLoadEvent;
			}
		}
	} // end of namespace contact
} // end of namespace sdr

//////////////////////////////////////////////////////////////////////////////
// eof
