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

#include <drawinglayer/attribute/sdrfillbitmapattribute.hxx>
#include <drawinglayer/attribute/fillbitmapattribute.hxx>
#include <basegfx/numeric/ftools.hxx>
#include <basegfx/range/b2drange.hxx>

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace attribute
	{
		SdrFillBitmapAttribute::SdrFillBitmapAttribute(
			const Bitmap& rBitmap, const basegfx::B2DVector& rSize, const basegfx::B2DVector& rOffset,
			const basegfx::B2DVector& rOffsetPosition, const basegfx::B2DVector& rRectPoint,
			bool bTiling, bool bStretch, bool bLogSize)
		:	maBitmap(rBitmap),
			maSize(rSize),
			maOffset(rOffset),
			maOffsetPosition(rOffsetPosition),
			maRectPoint(rRectPoint),
			mbTiling(bTiling),
			mbStretch(bStretch),
			mbLogSize(bLogSize)
		{
		}

		bool SdrFillBitmapAttribute::operator==(const SdrFillBitmapAttribute& rCandidate) const
		{
			return (maBitmap == rCandidate.maBitmap
				&& maSize == rCandidate.maSize
				&& maOffset == rCandidate.maOffset
				&& maOffsetPosition == rCandidate.maOffsetPosition
				&& maRectPoint == rCandidate.maRectPoint
				&& mbTiling == rCandidate.mbTiling 
				&& mbStretch == rCandidate.mbStretch 
				&& mbLogSize == rCandidate.mbLogSize);
		}

		FillBitmapAttribute SdrFillBitmapAttribute::getFillBitmapAttribute(const basegfx::B2DRange& rRange) const
		{
			// get logical size of bitmap (before expanding eventually)
			Bitmap aBitmap(maBitmap);
			const basegfx::B2DVector aLogicalSize(aBitmap.GetPrefSize().getWidth(), aBitmap.GetPrefSize().getHeight());

			// get hor/ver shiftings and apply them eventually to the bitmap, but only
			// when tiling is on
			bool bExpandWidth(false);
			bool bExpandHeight(false);

			if(mbTiling)
			{
				if(0.0 != maOffset.getX() || 0.0 != maOffset.getY())
				{
					const sal_uInt32 nWidth(aBitmap.GetSizePixel().getWidth());
					const sal_uInt32 nHeight(aBitmap.GetSizePixel().getHeight());

					if(0.0 != maOffset.getX())
					{
						bExpandHeight = true;
						const sal_uInt32 nOffset(basegfx::fround(((double)nWidth * maOffset.getX()) / 100.0));
						aBitmap.Expand(0L, nHeight);

						const Size aSizeA(nOffset, nHeight);
						const Rectangle aDstA(Point(0L, nHeight), aSizeA);
						const Rectangle aSrcA(Point(nWidth - nOffset, 0L), aSizeA);
						aBitmap.CopyPixel(aDstA, aSrcA);

						const Size aSizeB(nWidth - nOffset, nHeight);
						const Rectangle aDstB(Point(nOffset, nHeight), aSizeB);
						const Rectangle aSrcB(Point(0L, 0L), aSizeB);
						aBitmap.CopyPixel(aDstB, aSrcB);
					}
					else
					{
						bExpandWidth = true;
						const sal_uInt32 nOffset(basegfx::fround(((double)nHeight * maOffset.getY()) / 100.0));
						aBitmap.Expand(nWidth, 0L);

						const Size aSize(nWidth, nHeight);
						const Rectangle aDst(Point(nWidth, 0L), aSize);
						const Rectangle aSrc(Point(0L, 0L), aSize);
						aBitmap.CopyPixel(aDst, aSrc);

						const Size aSizeA(nWidth, nOffset);
						const Rectangle aDstA(Point(0L, 0L), aSizeA);
						const Rectangle aSrcA(Point(nWidth, nHeight - nOffset), aSizeA);
						aBitmap.CopyPixel(aDstA, aSrcA);

						const Size aSizeB(nWidth, nHeight - nOffset);
						const Rectangle aDstB(Point(0L, nOffset), aSizeB);
						const Rectangle aSrcB(Point(nWidth, 0L), aSizeB);
						aBitmap.CopyPixel(aDstB, aSrcB);
					}
				}
			}

			// init values with defaults
			basegfx::B2DPoint aBitmapSize(1.0, 1.0);
			basegfx::B2DVector aBitmapTopLeft(0.0, 0.0);

			// are canges needed?
			if(mbTiling || !mbStretch)
			{
				// init values with range sizes
				const double fRangeWidth(0.0 != rRange.getWidth() ? rRange.getWidth() : 1.0);
				const double fRangeHeight(0.0 != rRange.getHeight() ? rRange.getHeight() : 1.0);
				aBitmapSize = basegfx::B2DPoint(fRangeWidth, fRangeHeight);

				// size changes
				if(0.0 != maSize.getX())
				{
					if(maSize.getX() < 0.0)
					{
						aBitmapSize.setX(aBitmapSize.getX() * (maSize.getX() * -0.01));
					}
					else
					{
						aBitmapSize.setX(maSize.getX());
					}
				}
				else
				{
					aBitmapSize.setX(aLogicalSize.getX());
				}

				if(0.0 != maSize.getY())
				{
					if(maSize.getY() < 0.0)
					{
						aBitmapSize.setY(aBitmapSize.getY() * (maSize.getY() * -0.01));
					}
					else
					{
						aBitmapSize.setY(maSize.getY());
					}
				}
				else
				{
					aBitmapSize.setY(aLogicalSize.getY());
				}

				// get values, force to centered if necessary
				const basegfx::B2DVector aRectPoint(mbTiling ? maRectPoint : basegfx::B2DVector(0.0, 0.0));

				// position changes X
				if(0.0 == aRectPoint.getX())
				{
					aBitmapTopLeft.setX((fRangeWidth - aBitmapSize.getX()) * 0.5);
				}
				else if(1.0 == aRectPoint.getX())
				{
					aBitmapTopLeft.setX(fRangeWidth - aBitmapSize.getX());
				}

				if(mbTiling && 0.0 != maOffsetPosition.getX())
				{
					aBitmapTopLeft.setX(aBitmapTopLeft.getX() + (aBitmapSize.getX() * (maOffsetPosition.getX() * 0.01)));
				}

				// position changes Y
				if(0.0 == aRectPoint.getY())
				{
					aBitmapTopLeft.setY((fRangeHeight - aBitmapSize.getY()) * 0.5);
				}
				else if(1.0 == aRectPoint.getY())
				{
					aBitmapTopLeft.setY(fRangeHeight - aBitmapSize.getY());
				}

				if(mbTiling && 0.0 != maOffsetPosition.getY())
				{
					aBitmapTopLeft.setY(aBitmapTopLeft.getY() + (aBitmapSize.getY() * (maOffsetPosition.getY() * 0.01)));
				}

				// apply expand
				if(bExpandWidth)
				{
					aBitmapSize.setX(aBitmapSize.getX() * 2.0);
				}

				if(bExpandHeight)
				{
					aBitmapSize.setY(aBitmapSize.getY() * 2.0);
				}

				// apply bitmap size scaling to unit rectangle
				aBitmapTopLeft.setX(aBitmapTopLeft.getX() / fRangeWidth);
				aBitmapTopLeft.setY(aBitmapTopLeft.getY() / fRangeHeight);
				aBitmapSize.setX(aBitmapSize.getX() / fRangeWidth);
				aBitmapSize.setY(aBitmapSize.getY() / fRangeHeight);
			}

			return FillBitmapAttribute(aBitmap, aBitmapTopLeft, aBitmapSize, mbTiling);
		}
	} // end of namespace attribute
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////
// eof
