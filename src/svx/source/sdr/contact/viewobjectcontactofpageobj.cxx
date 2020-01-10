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

#include <svx/sdr/contact/viewobjectcontactofpageobj.hxx>
#include <svx/sdr/contact/viewcontactofpageobj.hxx>
#include <svx/svdopage.hxx>
#include <svx/sdr/contact/displayinfo.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <drawinglayer/primitive2d/polypolygonprimitive2d.hxx>
#include <drawinglayer/primitive2d/polygonprimitive2d.hxx>
#include <svx/sdr/contact/objectcontactofobjlistpainter.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <svx/svdpage.hxx>
#include <unoapi.hxx>
#include <drawinglayer/primitive2d/pagepreviewprimitive2d.hxx>

//////////////////////////////////////////////////////////////////////////////

using namespace com::sun::star;

//////////////////////////////////////////////////////////////////////////////

namespace sdr
{
    namespace contact 
    {	
        class PagePrimitiveExtractor : public ObjectContactOfPagePainter, public Timer
	    {
	    private:
		    // the ViewObjectContactOfPageObj using this painter
		    ViewObjectContactOfPageObj&			mrViewObjectContactOfPageObj;

	    public:
		    // basic constructor/destructor
		    PagePrimitiveExtractor(ViewObjectContactOfPageObj& rVOC);
		    virtual ~PagePrimitiveExtractor();

		    // LazyInvalidate request. Supported here to not automatically
		    // invalidate the second interaction state all the time at the
		    // original OC
		    virtual void setLazyInvalidate(ViewObjectContact& rVOC);

		    // From baseclass Timer, the timeout call triggered by te LazyInvalidate mechanism
		    virtual void Timeout();

		    // get primitive visualization
		    drawinglayer::primitive2d::Primitive2DSequence createPrimitive2DSequenceForPage(const DisplayInfo& rDisplayInfo);

		    // Own reaction on changes which will be forwarded to the OC of the owner-VOC
		    virtual void InvalidatePartOfView(const basegfx::B2DRange& rRange) const;

            // forward access to SdrPageView of ViewObjectContactOfPageObj
		    virtual bool isOutputToPrinter() const;
		    virtual bool isOutputToWindow() const;
		    virtual bool isOutputToVirtualDevice() const;
		    virtual bool isOutputToRecordingMetaFile() const;
			virtual bool isOutputToPDFFile() const;
		    virtual bool isDrawModeGray() const;
		    virtual bool isDrawModeBlackWhite() const;
		    virtual bool isDrawModeHighContrast() const;
		    virtual SdrPageView* TryToGetSdrPageView() const;
		    virtual OutputDevice* TryToGetOutputDevice() const;
	    };

	    PagePrimitiveExtractor::PagePrimitiveExtractor(
		    ViewObjectContactOfPageObj& rVOC)
	    :	ObjectContactOfPagePainter(0, rVOC.GetObjectContact()),
		    mrViewObjectContactOfPageObj(rVOC)
	    {
		    // make this renderer a preview renderer
		    setPreviewRenderer(true);

		    // init timer
		    SetTimeout(1);
		    Stop();
	    }

	    PagePrimitiveExtractor::~PagePrimitiveExtractor()
	    {
		    // execute missing LazyInvalidates and stop timer
		    Timeout();
	    }
    	
	    void PagePrimitiveExtractor::setLazyInvalidate(ViewObjectContact& /*rVOC*/)
	    {
		    // do NOT call parent, but remember that something is to do by
		    // starting the LazyInvalidateTimer
		    Start();
	    }

	    // From baseclass Timer, the timeout call triggered by te LazyInvalidate mechanism
	    void PagePrimitiveExtractor::Timeout()
	    {
		    // stop the timer
		    Stop();

		    // invalidate all LazyInvalidate VOCs new situations
		    const sal_uInt32 nVOCCount(getViewObjectContactCount());

		    for(sal_uInt32 a(0); a < nVOCCount; a++)
		    {
			    ViewObjectContact* pCandidate = getViewObjectContact(a);
			    pCandidate->triggerLazyInvalidate();
		    }
	    }

	    drawinglayer::primitive2d::Primitive2DSequence PagePrimitiveExtractor::createPrimitive2DSequenceForPage(const DisplayInfo& /*rDisplayInfo*/)
	    {
		    drawinglayer::primitive2d::Primitive2DSequence xRetval;
		    const SdrPage* pStartPage = GetStartPage();

		    if(pStartPage)
		    {
                // update own ViewInformation2D for visualized page
			    const drawinglayer::geometry::ViewInformation2D& rOriginalViewInformation = mrViewObjectContactOfPageObj.GetObjectContact().getViewInformation2D();
                const drawinglayer::geometry::ViewInformation2D aNewViewInformation2D(
				    rOriginalViewInformation.getObjectTransformation(),
				    rOriginalViewInformation.getViewTransformation(),

                    // #i101075# use empty range for page content here to force
                    // the content not to be physically clipped in any way. This
                    // would be possible, but would require the internal transformation
                    // which maps between the page visualisation object and the page
                    // content, including the aspect ratios (for details see in 
                    // PagePreviewPrimitive2D::createLocalDecomposition)
                    basegfx::B2DRange(),

                    GetXDrawPageForSdrPage(const_cast< SdrPage* >(pStartPage)),
				    0.0, // no time; page previews are not animated
				    rOriginalViewInformation.getExtendedInformationSequence());
			    updateViewInformation2D(aNewViewInformation2D);

			    // create copy of DisplayInfo to set PagePainting
			    DisplayInfo aDisplayInfo;

			    // get page's VOC
			    ViewObjectContact& rDrawPageVOContact = pStartPage->GetViewContact().GetViewObjectContact(*this);

			    // get whole Primitive2DSequence
			    xRetval = rDrawPageVOContact.getPrimitive2DSequenceHierarchy(aDisplayInfo);
		    }

		    return xRetval;
	    }

	    void PagePrimitiveExtractor::InvalidatePartOfView(const basegfx::B2DRange& rRange) const
	    {
		    // an invalidate is called at this view, this needs to be translated to an invalidate
		    // for the using VOC. Coordinates are in page coordinate system.
		    const SdrPage* pStartPage = GetStartPage();

		    if(pStartPage && !rRange.isEmpty())
		    {
			    const basegfx::B2DRange aPageRange(0.0, 0.0, (double)pStartPage->GetWdt(), (double)pStartPage->GetHgt());

			    if(rRange.overlaps(aPageRange))
			    {
				    // if object on the page is inside or overlapping with page, create ActionChanged() for 
				    // involved VOC
				    mrViewObjectContactOfPageObj.ActionChanged();
			    }
		    }
	    }

        // forward access to SdrPageView to VOCOfPageObj
	    bool PagePrimitiveExtractor::isOutputToPrinter() const { return mrViewObjectContactOfPageObj.GetObjectContact().isOutputToPrinter(); }
	    bool PagePrimitiveExtractor::isOutputToWindow() const { return mrViewObjectContactOfPageObj.GetObjectContact().isOutputToWindow(); }
	    bool PagePrimitiveExtractor::isOutputToVirtualDevice() const { return mrViewObjectContactOfPageObj.GetObjectContact().isOutputToVirtualDevice(); }
	    bool PagePrimitiveExtractor::isOutputToRecordingMetaFile() const { return mrViewObjectContactOfPageObj.GetObjectContact().isOutputToRecordingMetaFile(); }
		bool PagePrimitiveExtractor::isOutputToPDFFile() const { return mrViewObjectContactOfPageObj.GetObjectContact().isOutputToPDFFile(); }
	    bool PagePrimitiveExtractor::isDrawModeGray() const { return mrViewObjectContactOfPageObj.GetObjectContact().isDrawModeGray(); }
	    bool PagePrimitiveExtractor::isDrawModeBlackWhite() const { return mrViewObjectContactOfPageObj.GetObjectContact().isDrawModeBlackWhite(); }
	    bool PagePrimitiveExtractor::isDrawModeHighContrast() const { return mrViewObjectContactOfPageObj.GetObjectContact().isDrawModeHighContrast(); }
	    SdrPageView* PagePrimitiveExtractor::TryToGetSdrPageView() const { return mrViewObjectContactOfPageObj.GetObjectContact().TryToGetSdrPageView(); }
	    OutputDevice* PagePrimitiveExtractor::TryToGetOutputDevice() const { return mrViewObjectContactOfPageObj.GetObjectContact().TryToGetOutputDevice(); }
	} // end of namespace contact
} // end of namespace sdr

//////////////////////////////////////////////////////////////////////////////

namespace sdr
{
	namespace contact
	{
		drawinglayer::primitive2d::Primitive2DSequence ViewObjectContactOfPageObj::createPrimitive2DSequence(const DisplayInfo& rDisplayInfo) const
		{
			drawinglayer::primitive2d::Primitive2DSequence xRetval;
			const SdrPageObj& rPageObject((static_cast< ViewContactOfPageObj& >(GetViewContact())).GetPageObj());
			const SdrPage* pPage = rPageObject.GetReferencedPage();
			const svtools::ColorConfig aColorConfig;

            // get PageObject's geometry
            basegfx::B2DHomMatrix aPageObjectTransform;
			{
				const Rectangle aPageObjectModelData(rPageObject.GetLastBoundRect());
				const basegfx::B2DRange aPageObjectBound(
					aPageObjectModelData.Left(), aPageObjectModelData.Top(), 
					aPageObjectModelData.Right(), aPageObjectModelData.Bottom());
				
				aPageObjectTransform.set(0, 0, aPageObjectBound.getWidth());
				aPageObjectTransform.set(1, 1, aPageObjectBound.getHeight());
				aPageObjectTransform.set(0, 2, aPageObjectBound.getMinX());
				aPageObjectTransform.set(1, 2, aPageObjectBound.getMinY());
			}
		
			// get displayed page's content. This is the uscaled page content
            if(mpExtractor && pPage)
			{
	            // get displayed page's geometry
	   			drawinglayer::primitive2d::Primitive2DSequence xPageContent;
				const Size aPageSize(pPage->GetSize());
				const double fPageWidth(aPageSize.getWidth());
				const double fPageHeight(aPageSize.getHeight());
				
				// The case that a PageObject contains another PageObject which visualizes the
				// same page again would lead to a recursion. Limit that recursion depth to one
				// by using a local static bool
				static bool bInCreatePrimitive2D(false);

				if(bInCreatePrimitive2D)
				{
					// Recursion is possible. Create a replacement primitive
					xPageContent.realloc(2);
					const Color aDocColor(aColorConfig.GetColorValue(svtools::DOCCOLOR).nColor);
					const Color aBorderColor(aColorConfig.GetColorValue(svtools::DOCBOUNDARIES).nColor);
					const basegfx::B2DRange aPageBound(0.0, 0.0, fPageWidth, fPageHeight);
					const basegfx::B2DPolygon aOutline(basegfx::tools::createPolygonFromRect(aPageBound));

					// add replacement fill
					xPageContent[0L] = drawinglayer::primitive2d::Primitive2DReference(
						new drawinglayer::primitive2d::PolyPolygonColorPrimitive2D(basegfx::B2DPolyPolygon(aOutline), aDocColor.getBColor()));

					// add replacement border
					xPageContent[1L] = drawinglayer::primitive2d::Primitive2DReference(
						new drawinglayer::primitive2d::PolygonHairlinePrimitive2D(aOutline, aBorderColor.getBColor()));
				}
				else
				{
					// set recursion flag
					bInCreatePrimitive2D = true;

					// init extractor, guarantee existance, set page there
					mpExtractor->SetStartPage(pPage);

					// create page content
					xPageContent = mpExtractor->createPrimitive2DSequenceForPage(rDisplayInfo);

					// reset recursion flag
					bInCreatePrimitive2D = false;
				}

				// prepare retval
				if(xPageContent.hasElements())
				{
					const uno::Reference< drawing::XDrawPage > xDrawPage(GetXDrawPageForSdrPage(const_cast< SdrPage*>(pPage)));
    				const drawinglayer::primitive2d::Primitive2DReference xPagePreview(new drawinglayer::primitive2d::PagePreviewPrimitive2D(
						xDrawPage, aPageObjectTransform, fPageWidth, fPageHeight, xPageContent, true));
					xRetval = drawinglayer::primitive2d::Primitive2DSequence(&xPagePreview, 1);
				}
            }

			// add a gray outline frame, except not when printing
			// #i102637# add frame also when printing and page exists (handout pages)
			if(!GetObjectContact().isOutputToPrinter() || pPage)
			{
				const Color aFrameColor(aColorConfig.GetColorValue(svtools::OBJECTBOUNDARIES).nColor);
				basegfx::B2DPolygon aOwnOutline(basegfx::tools::createPolygonFromRect(basegfx::B2DRange(0.0, 0.0, 1.0, 1.0)));
				aOwnOutline.transform(aPageObjectTransform);
				
				const drawinglayer::primitive2d::Primitive2DReference xGrayFrame(
					new drawinglayer::primitive2d::PolygonHairlinePrimitive2D(aOwnOutline, aFrameColor.getBColor()));

				drawinglayer::primitive2d::appendPrimitive2DReferenceToPrimitive2DSequence(xRetval, xGrayFrame);
			}

			return xRetval;
		}

		ViewObjectContactOfPageObj::ViewObjectContactOfPageObj(ObjectContact& rObjectContact, ViewContact& rViewContact)
		:	ViewObjectContactOfSdrObj(rObjectContact, rViewContact),
			mpExtractor(new PagePrimitiveExtractor(*this))
		{
		}

		ViewObjectContactOfPageObj::~ViewObjectContactOfPageObj()
		{
			// delete the helper OC
			if(mpExtractor)
			{
				// remember candidate and reset own pointer to avoid action when createPrimitive2DSequence()
				// would be called for any reason
				PagePrimitiveExtractor* pCandidate = mpExtractor;
				mpExtractor = 0;

				// also reset the StartPage to avoid ActionChanged() forwardings in the
				// PagePrimitiveExtractor::InvalidatePartOfView() implementation
				pCandidate->SetStartPage(0);
				delete pCandidate;
			}
		}
	} // end of namespace contact
} // end of namespace sdr

//////////////////////////////////////////////////////////////////////////////
// eof
