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

#include <svx/sdr/contact/viewcontactofsdrcaptionobj.hxx>
#include <svx/svdocapt.hxx>
#include <svx/sdr/primitive2d/sdrattributecreator.hxx>
#include <svx/sdr/attribute/sdrallattribute.hxx>
#include <svx/sdr/primitive2d/sdrcaptionprimitive2d.hxx>

//////////////////////////////////////////////////////////////////////////////
// includes for special text box shadow (SC)

#include <svtools/itemset.hxx>
#include <svx/xhatch.hxx>
#include <svx/xflhtit.hxx>
#include <svx/xflclit.hxx>
#include <svx/xfltrit.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <svx/sdr/primitive2d/sdrdecompositiontools.hxx>

//////////////////////////////////////////////////////////////////////////////

namespace sdr
{
	namespace contact
	{
		ViewContactOfSdrCaptionObj::ViewContactOfSdrCaptionObj(SdrCaptionObj& rCaptionObj)
		:	ViewContactOfSdrRectObj(rCaptionObj)
		{
		}

		ViewContactOfSdrCaptionObj::~ViewContactOfSdrCaptionObj()
		{
		}

		drawinglayer::primitive2d::Primitive2DSequence ViewContactOfSdrCaptionObj::createViewIndependentPrimitive2DSequence() const
		{
			drawinglayer::primitive2d::Primitive2DSequence xRetval;
			const SdrCaptionObj& rCaptionObj(GetCaptionObj());
			const SfxItemSet& rItemSet = rCaptionObj.GetMergedItemSet();
			SdrText* pSdrText = rCaptionObj.getText(0);

			if(pSdrText)
			{
				drawinglayer::attribute::SdrLineFillShadowTextAttribute* pAttribute = drawinglayer::primitive2d::createNewSdrLineFillShadowTextAttribute(rItemSet, *pSdrText);

				if(pAttribute)
				{
					if(pAttribute->isVisible())
					{
						// take unrotated snap rect (direct model data) for position and size
						const Rectangle& rRectangle = rCaptionObj.GetGeoRect();
						const ::basegfx::B2DRange aObjectRange(rRectangle.Left(), rRectangle.Top(), rRectangle.Right(), rRectangle.Bottom());
						const GeoStat& rGeoStat(rCaptionObj.GetGeoStat());
						::basegfx::B2DHomMatrix aObjectMatrix;

						// fill object matrix
						if(!::basegfx::fTools::equalZero(aObjectRange.getWidth()))
						{
							aObjectMatrix.set(0, 0, aObjectRange.getWidth());
						}

						if(!::basegfx::fTools::equalZero(aObjectRange.getHeight()))
						{
							aObjectMatrix.set(1, 1, aObjectRange.getHeight());
						}

						if(rGeoStat.nShearWink)
						{
							aObjectMatrix.shearX(tan((36000 - rGeoStat.nShearWink) * F_PI18000));
						}

						if(rGeoStat.nDrehWink)
						{
							aObjectMatrix.rotate((36000 - rGeoStat.nDrehWink) * F_PI18000);
						}

						aObjectMatrix.translate(aObjectRange.getMinX(), aObjectRange.getMinY());

						// calculate corner radius
						double fCornerRadiusX;
						double fCornerRadiusY;
						drawinglayer::primitive2d::calculateRelativeCornerRadius(rCaptionObj.GetEckenradius(), aObjectRange, fCornerRadiusX, fCornerRadiusY);

						// create primitive
						const drawinglayer::primitive2d::Primitive2DReference xReference(new drawinglayer::primitive2d::SdrCaptionPrimitive2D(
							aObjectMatrix, *pAttribute, rCaptionObj.getTailPolygon(), fCornerRadiusX, fCornerRadiusY));
						xRetval = drawinglayer::primitive2d::Primitive2DSequence(&xReference, 1);

						if(rCaptionObj.GetSpecialTextBoxShadow())
						{
							// for SC, the caption object may have a specialized shadow. The usual object shadow is off
							// and a specialized shadow gets created here (see old paint)
							const SdrShadowColorItem& rShadColItem = (SdrShadowColorItem&)(rItemSet.Get(SDRATTR_SHADOWCOLOR));
							const sal_uInt16 nTransp(((SdrShadowTransparenceItem&)(rItemSet.Get(SDRATTR_SHADOWTRANSPARENCE))).GetValue());
							const Color aShadCol(rShadColItem.GetColorValue());
							const XFillStyle eStyle = ((XFillStyleItem&)(rItemSet.Get(XATTR_FILLSTYLE))).GetValue();

							// Create own ItemSet and modify as needed
							// Always hide lines for special calc shadow
							SfxItemSet aSet(rItemSet);
							aSet.Put(XLineStyleItem(XLINE_NONE));

							if(XFILL_HATCH == eStyle)
							{
								// #41666# Hatch color is set hard to shadow color
								XHatch aHatch = ((XFillHatchItem&)(rItemSet.Get(XATTR_FILLHATCH))).GetHatchValue();
								aHatch.SetColor(aShadCol);
								aSet.Put(XFillHatchItem(String(),aHatch));
							}
							else
							{
								if(XFILL_NONE != eStyle && XFILL_SOLID != eStyle)
								{
									// force fill to solid (for Gradient and Bitmap)
									aSet.Put(XFillStyleItem(XFILL_SOLID));
								}

								aSet.Put(XFillColorItem(String(),aShadCol));
								aSet.Put(XFillTransparenceItem(nTransp));
							}

							// crete FillAttribute from modified ItemSet
							drawinglayer::attribute::SdrFillAttribute* pFill = drawinglayer::primitive2d::createNewSdrFillAttribute(aSet);
							drawinglayer::primitive2d::Primitive2DReference xSpecialShadow;

							if(pFill)
							{
								if(pFill->isVisible())
								{
									// add shadow offset to object matrix
									const sal_uInt32 nXDist(((SdrShadowXDistItem&)(rItemSet.Get(SDRATTR_SHADOWXDIST))).GetValue());
									const sal_uInt32 nYDist(((SdrShadowYDistItem&)(rItemSet.Get(SDRATTR_SHADOWYDIST))).GetValue());
									aObjectMatrix.translate(nXDist, nYDist);
									
									// create unit outline polygon as geometry (see SdrCaptionPrimitive2D::createLocalDecomposition)
									basegfx::B2DPolygon aUnitOutline(basegfx::tools::createPolygonFromRect(basegfx::B2DRange(0.0, 0.0, 1.0, 1.0), fCornerRadiusX, fCornerRadiusY));

									// create the specialized shadow primitive
									xSpecialShadow = drawinglayer::primitive2d::createPolyPolygonFillPrimitive(basegfx::B2DPolyPolygon(aUnitOutline), aObjectMatrix, *pFill);
								}

								delete pFill;
							}

							if(xSpecialShadow.is())
							{
								// if we really got a special shadow, create a two-element retval with the shadow
								// behind the standard object's geometry
								xRetval.realloc(2);
								
								xRetval[0] = xSpecialShadow;
								xRetval[1] = xReference;
							}
						}
					}

					delete pAttribute;
				}
			}

			return xRetval;
		}
	} // end of namespace contact
} // end of namespace sdr

//////////////////////////////////////////////////////////////////////////////
// eof
