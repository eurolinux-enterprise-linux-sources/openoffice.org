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
#include "precompiled_drawinglayer.hxx"

#include <drawinglayer/primitive2d/graphicprimitive2d.hxx>
#include <drawinglayer/animation/animationtiming.hxx>
#include <drawinglayer/primitive2d/bitmapprimitive2d.hxx>
#include <drawinglayer/primitive2d/animatedprimitive2d.hxx>
#include <drawinglayer/primitive2d/metafileprimitive2d.hxx>
#include <drawinglayer/primitive2d/transformprimitive2d.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <drawinglayer/primitive2d/maskprimitive2d.hxx>
#include <drawinglayer/primitive2d/drawinglayer_primitivetypes2d.hxx>

//////////////////////////////////////////////////////////////////////////////
// helper class for animated graphics

#include <vcl/animate.hxx>
#include <vcl/graph.hxx>
#include <vcl/virdev.hxx>
#include <vcl/svapp.hxx>

//////////////////////////////////////////////////////////////////////////////

namespace
{
	struct animationStep
	{
		BitmapEx								maBitmapEx;
		sal_uInt32								mnTime;
	};

	class animatedBitmapExPreparator
	{
		::Animation								maAnimation;
		::std::vector< animationStep >			maSteps;

		sal_uInt32 generateStepTime(sal_uInt32 nIndex) const;

	public:
		animatedBitmapExPreparator(const Graphic& rGraphic);

		sal_uInt32 count() const { return maSteps.size(); }
		sal_uInt32 loopCount() const { return (sal_uInt32)maAnimation.GetLoopCount(); }
		sal_uInt32 stepTime(sal_uInt32 a) const { return maSteps[a].mnTime; }
		const BitmapEx& stepBitmapEx(sal_uInt32 a) const { return maSteps[a].maBitmapEx; }
	};

	sal_uInt32 animatedBitmapExPreparator::generateStepTime(sal_uInt32 nIndex) const
	{
		const AnimationBitmap& rAnimBitmap = maAnimation.Get(sal_uInt16(nIndex));
		sal_uInt32 nWaitTime(rAnimBitmap.nWait * 10);

		// #115934#
		// Take care of special value for MultiPage TIFFs. ATM these shall just
		// show their first page. Later we will offer some switching when object
		// is selected.
		if(ANIMATION_TIMEOUT_ON_CLICK == rAnimBitmap.nWait)
		{
			// ATM the huge value would block the timer, so
			// use a long time to show first page (whole day)
			nWaitTime = 100 * 60 * 60 * 24;
		}

		// Bad trap: There are animated gifs with no set WaitTime (!).
		// In that case use a default value.
		if(0L == nWaitTime)
		{
			nWaitTime = 100L;
		}

		return nWaitTime;
	}

	animatedBitmapExPreparator::animatedBitmapExPreparator(const Graphic& rGraphic)
	:	maAnimation(rGraphic.GetAnimation())
	{
		OSL_ENSURE(GRAPHIC_BITMAP == rGraphic.GetType() && rGraphic.IsAnimated(), "animatedBitmapExPreparator: graphic is not animated (!)");

		// #128539# secure access to Animation, looks like there exist animated GIFs out there
		// with a step count of zero
		if(maAnimation.Count())
		{
			VirtualDevice aVirtualDevice(*Application::GetDefaultDevice());
			VirtualDevice aVirtualDeviceMask(*Application::GetDefaultDevice(), 1L);

			// Prepare VirtualDevices and their states
			aVirtualDevice.EnableMapMode(sal_False);
			aVirtualDeviceMask.EnableMapMode(sal_False);
			aVirtualDevice.SetOutputSizePixel(maAnimation.GetDisplaySizePixel());
			aVirtualDeviceMask.SetOutputSizePixel(maAnimation.GetDisplaySizePixel());
			aVirtualDevice.Erase();
			aVirtualDeviceMask.Erase();

			for(sal_uInt16 a(0L); a < maAnimation.Count(); a++)
			{
				animationStep aNextStep;
				aNextStep.mnTime = generateStepTime(a);
	
				// prepare step
				const AnimationBitmap& rAnimBitmap = maAnimation.Get(sal_uInt16(a));

				switch(rAnimBitmap.eDisposal)
				{
					case DISPOSE_NOT:
					{
						aVirtualDevice.DrawBitmapEx(rAnimBitmap.aPosPix, rAnimBitmap.aBmpEx);
						Bitmap aMask = rAnimBitmap.aBmpEx.GetMask();

						if(aMask.IsEmpty())
						{
							const Point aEmpty;
							const Rectangle aRect(aEmpty, aVirtualDeviceMask.GetOutputSizePixel());
							const Wallpaper aWallpaper(COL_BLACK);
							aVirtualDeviceMask.DrawWallpaper(aRect, aWallpaper);
						}
						else
						{
							BitmapEx aExpandVisibilityMask = BitmapEx(aMask, aMask);
							aVirtualDeviceMask.DrawBitmapEx(rAnimBitmap.aPosPix, aExpandVisibilityMask);
						}

						break;
					}
					case DISPOSE_BACK:
					{
						// #i70772# react on no mask, for primitives, too.
						const Bitmap aMask(rAnimBitmap.aBmpEx.GetMask());
						const Bitmap aContent(rAnimBitmap.aBmpEx.GetBitmap());

						aVirtualDeviceMask.Erase();
						aVirtualDevice.DrawBitmap(rAnimBitmap.aPosPix, aContent);

						if(aMask.IsEmpty())
						{
							const Rectangle aRect(rAnimBitmap.aPosPix, aContent.GetSizePixel());
							aVirtualDeviceMask.SetFillColor(COL_BLACK);
							aVirtualDeviceMask.SetLineColor();
							aVirtualDeviceMask.DrawRect(aRect);
						}
						else
						{
							aVirtualDeviceMask.DrawBitmap(rAnimBitmap.aPosPix, aMask);
						}

						break;
					}
					case DISPOSE_FULL:
					{
						aVirtualDevice.DrawBitmapEx(rAnimBitmap.aPosPix, rAnimBitmap.aBmpEx);
						break;
					}
					case DISPOSE_PREVIOUS :
					{
						aVirtualDevice.DrawBitmapEx(rAnimBitmap.aPosPix, rAnimBitmap.aBmpEx);
						aVirtualDeviceMask.DrawBitmap(rAnimBitmap.aPosPix, rAnimBitmap.aBmpEx.GetMask());
						break;
					}
				}
			
				// create BitmapEx
				Bitmap aMainBitmap = aVirtualDevice.GetBitmap(Point(), aVirtualDevice.GetOutputSizePixel());
				Bitmap aMaskBitmap = aVirtualDeviceMask.GetBitmap(Point(), aVirtualDeviceMask.GetOutputSizePixel());
				aNextStep.maBitmapEx = BitmapEx(aMainBitmap, aMaskBitmap);

				// add to vector
				maSteps.push_back(aNextStep);
			}
		}
	}
} // end of anonymous namespace

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive2d
	{
		Primitive2DSequence GraphicPrimitive2D::createLocalDecomposition(const geometry::ViewInformation2D& /*rViewInformation*/) const
		{
			Primitive2DSequence aRetval;

			if(255L != getGraphicAttr().GetTransparency())
			{
				// get transformed graphic. Suppress rotation and cropping, only filtering is needed
				// here (and may be replaced later on). Cropping is handled below as mask primitive (if set)
				GraphicAttr aSuppressGraphicAttr(getGraphicAttr());
				aSuppressGraphicAttr.SetCrop(0L, 0L, 0L, 0L);
				aSuppressGraphicAttr.SetRotation(0);
				Graphic aTransformedGraphic(getGraphicObject().GetTransformedGraphic(&aSuppressGraphicAttr));
				Primitive2DReference xPrimitive;

				switch(aTransformedGraphic.GetType())
				{
					case GRAPHIC_BITMAP :
					{
						if(aTransformedGraphic.IsAnimated())
						{
							// prepare animation data
							animatedBitmapExPreparator aData(aTransformedGraphic);

							if(aData.count())
							{
								// create sub-primitives for animated bitmap and the needed animation loop
								animation::AnimationEntryLoop aAnimationLoop(aData.loopCount() ? aData.loopCount() : 0xffff);
								Primitive2DSequence aBitmapPrimitives(aData.count());

								for(sal_uInt32 a(0L); a < aData.count(); a++)
								{
                                    animation::AnimationEntryFixed aTime((double)aData.stepTime(a), (double)a / (double)aData.count());
									aAnimationLoop.append(aTime);
									const Primitive2DReference xRef(new BitmapPrimitive2D(aData.stepBitmapEx(a), getTransform()));
									aBitmapPrimitives[a] = xRef;
								}

								// prepare animation list
								animation::AnimationEntryList aAnimationList;
								aAnimationList.append(aAnimationLoop);

								// create and add animated switch primitive
								xPrimitive = Primitive2DReference(new AnimatedSwitchPrimitive2D(aAnimationList, aBitmapPrimitives, false));
							}
						}
						else
						{
							xPrimitive = Primitive2DReference(new BitmapPrimitive2D(aTransformedGraphic.GetBitmapEx(), getTransform()));
						}

						break;
					}

					case GRAPHIC_GDIMETAFILE :
					{
                        // create MetafilePrimitive2D
                        const GDIMetaFile& rMetafile = aTransformedGraphic.GetGDIMetaFile();

                        xPrimitive = Primitive2DReference(
                            new MetafilePrimitive2D(
                                getTransform(), 
                                rMetafile));

                        // #i100357# find out if clipping is needed for this primitive. Unfortunately,
                        // there exist Metafiles who's content is bigger than the proposed PrefSize set
                        // at them. This is an error, but we need to work around this
                        const Size aMetaFilePrefSize(rMetafile.GetPrefSize());
                        const Size aMetaFileRealSize(
                            const_cast< GDIMetaFile& >(rMetafile).GetBoundRect(
                                *Application::GetDefaultDevice()).GetSize());

                        if(aMetaFileRealSize.getWidth() > aMetaFilePrefSize.getWidth()
                            || aMetaFileRealSize.getHeight() > aMetaFilePrefSize.getHeight())
                        {
                            // clipping needed. Embed to MaskPrimitive2D. Create childs and mask polygon
                            const primitive2d::Primitive2DSequence aChildContent(&xPrimitive, 1);
					        basegfx::B2DPolygon aMaskPolygon(
                                basegfx::tools::createPolygonFromRect(
                                    basegfx::B2DRange(0.0, 0.0, 1.0, 1.0)));
                            aMaskPolygon.transform(getTransform());

                            xPrimitive = Primitive2DReference(
                                new MaskPrimitive2D(
                                    basegfx::B2DPolyPolygon(aMaskPolygon), 
                                    aChildContent));
                        }

						break;
					}

					default:
					{
						// nothing to create
						break;
					}
				}

				if(xPrimitive.is())
				{
					// check for cropping
					if(getGraphicAttr().IsCropped())
					{
						// decompose to get current pos and size
						basegfx::B2DVector aScale, aTranslate;
						double fRotate, fShearX;
						getTransform().decompose(aScale, aTranslate, fRotate, fShearX);

						// create ranges. The current object range is just scale and translate
						const basegfx::B2DRange aCurrent(aTranslate.getX(), aTranslate.getY(), aTranslate.getX() + aScale.getX(), aTranslate.getY() + aScale.getY());

						// calculate scalings between real image size and logic object size. This
						// is necessary since the crop values are relative to original bitmap size
						double fFactorX(1.0);
						double fFactorY(1.0);

						{
							const MapMode aMapMode100thmm(MAP_100TH_MM);
							Size aBitmapSize(getGraphicObject().GetPrefSize());
							
							// #i95968# better support PrefMapMode; special for MAP_PIXEL was missing
							if(MAP_PIXEL == getGraphicObject().GetPrefMapMode().GetMapUnit())
							{
								aBitmapSize = Application::GetDefaultDevice()->PixelToLogic(aBitmapSize, aMapMode100thmm);
							}
							else
							{
								aBitmapSize = Application::GetDefaultDevice()->LogicToLogic(aBitmapSize, getGraphicObject().GetPrefMapMode(), aMapMode100thmm);
							}

							const double fDivX(aBitmapSize.Width() - getGraphicAttr().GetLeftCrop() - getGraphicAttr().GetRightCrop());
							const double fDivY(aBitmapSize.Height() - getGraphicAttr().GetTopCrop() - getGraphicAttr().GetBottomCrop());

							if(!basegfx::fTools::equalZero(fDivX))
							{
								fFactorX = aScale.getX() / fDivX;
							}

							if(!basegfx::fTools::equalZero(fDivY))
							{
								fFactorY = aScale.getY() / fDivY;
							}
						}

						// Create cropped range, describes the bounds of the original graphic
						basegfx::B2DRange aCropped;
						aCropped.expand(aCurrent.getMinimum() - basegfx::B2DPoint(getGraphicAttr().GetLeftCrop() * fFactorX, getGraphicAttr().GetTopCrop() * fFactorY));
						aCropped.expand(aCurrent.getMaximum() + basegfx::B2DPoint(getGraphicAttr().GetRightCrop() * fFactorX, getGraphicAttr().GetBottomCrop() * fFactorY));

						if(aCropped.isEmpty())
						{
							// nothing to add since cropped bitmap is completely empty
							// xPrimitive will not be used
						}
						else
						{
							// build new object transformation for transform primitive which contains xPrimitive
							basegfx::B2DHomMatrix aNewObjectTransform(getTransform());
							aNewObjectTransform.invert();
							aNewObjectTransform.scale(aCropped.getWidth(), aCropped.getHeight());
							aNewObjectTransform.translate(aCropped.getMinX() - aCurrent.getMinX(), aCropped.getMinY() - aCurrent.getMinY());
							aNewObjectTransform.shearX(fShearX);
							aNewObjectTransform.rotate(fRotate);
							aNewObjectTransform.translate(aTranslate.getX(), aTranslate.getY());

							// prepare TransformPrimitive2D with xPrimitive
							const Primitive2DReference xTransformPrimitive(new TransformPrimitive2D(aNewObjectTransform, Primitive2DSequence(&xPrimitive, 1L)));

							if(aCurrent.isInside(aCropped))
							{
								// cropped just got smaller, no need to really use a mask. Add to destination directly
								appendPrimitive2DReferenceToPrimitive2DSequence(aRetval, xTransformPrimitive);
							}
							else
							{
								// cropped got bigger, mask it with original object's bounds
								basegfx::B2DPolyPolygon aMaskPolyPolygon(basegfx::tools::createPolygonFromRect(basegfx::B2DRange(0.0, 0.0, 1.0, 1.0)));
								aMaskPolyPolygon.transform(getTransform());

								// create maskPrimitive with aMaskPolyPolygon and aMaskContentVector
								const Primitive2DReference xRefB(new MaskPrimitive2D(aMaskPolyPolygon, Primitive2DSequence(&xTransformPrimitive, 1L)));
								appendPrimitive2DReferenceToPrimitive2DSequence(aRetval, xRefB);
							}
						}
					}
					else
					{
						// add to decomposition
						appendPrimitive2DReferenceToPrimitive2DSequence(aRetval, xPrimitive);
					}
				}
			}

			return aRetval;
		}

		GraphicPrimitive2D::GraphicPrimitive2D(
			const basegfx::B2DHomMatrix& rTransform, 
			const GraphicObject& rGraphicObject,
			const GraphicAttr& rGraphicAttr)
		:	BasePrimitive2D(),
			maTransform(rTransform),
			maGraphicObject(rGraphicObject),
			maGraphicAttr(rGraphicAttr)
		{
		}

		GraphicPrimitive2D::GraphicPrimitive2D(
			const basegfx::B2DHomMatrix& rTransform, 
			const GraphicObject& rGraphicObject)
		:	BasePrimitive2D(),
			maTransform(rTransform),
			maGraphicObject(rGraphicObject),
			maGraphicAttr()
		{
		}

		bool GraphicPrimitive2D::operator==(const BasePrimitive2D& rPrimitive) const
		{
			if(BasePrimitive2D::operator==(rPrimitive))
			{
				const GraphicPrimitive2D& rCompare = (GraphicPrimitive2D&)rPrimitive;

				return (getTransform() == rCompare.getTransform()
					&& getGraphicObject() == rCompare.getGraphicObject()
					&& getGraphicAttr() == rCompare.getGraphicAttr());
			}

			return false;
		}

		basegfx::B2DRange GraphicPrimitive2D::getB2DRange(const geometry::ViewInformation2D& /*rViewInformation*/) const
		{
			basegfx::B2DRange aRetval(0.0, 0.0, 1.0, 1.0);
			aRetval.transform(getTransform());
			return aRetval;
		}

		// provide unique ID
		ImplPrimitrive2DIDBlock(GraphicPrimitive2D, PRIMITIVE2D_ID_GRAPHICPRIMITIVE2D)

	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////
// eof
