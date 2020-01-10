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

#include <svx/svdotext.hxx>
#include <svx/svdoutl.hxx>
#include <basegfx/vector/b2dvector.hxx>
#include <svx/sdr/primitive2d/sdrtextprimitive2d.hxx>
#include <basegfx/range/b2drange.hxx>
#include <vcl/salbtype.hxx>
#include <svtools/itemset.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <algorithm>
#include <svx/xtextit.hxx>
#include <svx/xftshtit.hxx>
#include <vcl/virdev.hxx>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/i18n/ScriptType.hdl>
#include <com/sun/star/i18n/XBreakIterator.hpp>
#include <comphelper/processfactory.hxx>
#include <com/sun/star/i18n/CharacterIteratorMode.hdl>
#include <unolingu.hxx>
#include <drawinglayer/primitive2d/textlayoutdevice.hxx>
#include <drawinglayer/primitive2d/textprimitive2d.hxx>
#include <basegfx/color/bcolor.hxx>

//////////////////////////////////////////////////////////////////////////////
// primitive decomposition helpers

#include <basegfx/polygon/b2dlinegeometry.hxx>
#include <drawinglayer/attribute/strokeattribute.hxx>
#include <svx/xlnclit.hxx>
#include <svx/xlntrit.hxx>
#include <svx/xlnwtit.hxx>
#include <xlinjoit.hxx>
#include <svx/xlndsit.hxx>
#include <drawinglayer/primitive2d/polygonprimitive2d.hxx>
#include <drawinglayer/primitive2d/unifiedalphaprimitive2d.hxx>
#include <editstat.hxx>
#include <unoapi.hxx>
#include <drawinglayer/geometry/viewinformation2d.hxx>
#include <svx/sdr/attribute/sdrformtextoutlineattribute.hxx>

//////////////////////////////////////////////////////////////////////////////

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::i18n;

//////////////////////////////////////////////////////////////////////////////
// PathTextPortion helper

namespace
{
	class impPathTextPortion
	{
		basegfx::B2DVector							maOffset;
		String										maText;
		xub_StrLen									mnTextStart;
		xub_StrLen									mnTextLength;
		sal_uInt16									mnParagraph;
		xub_StrLen									mnIndex;
		SvxFont										maFont;
		::std::vector< double >						maDblDXArray;	// double DXArray, font size independent -> unit coordinate system
        ::com::sun::star::lang::Locale				maLocale;

		// bitfield
		unsigned									mbRTL : 1;

	public:
		impPathTextPortion(DrawPortionInfo& rInfo)
		:	maOffset(rInfo.mrStartPos.X(), rInfo.mrStartPos.Y()),
			maText(rInfo.mrText),
			mnTextStart(rInfo.mnTextStart),
			mnTextLength(rInfo.mnTextLen),
			mnParagraph(rInfo.mnPara),
			mnIndex(rInfo.mnIndex),
			maFont(rInfo.mrFont),
            maDblDXArray(),
			maLocale(rInfo.mpLocale ? *rInfo.mpLocale : ::com::sun::star::lang::Locale()),
			mbRTL(rInfo.mrFont.IsVertical() ? false : rInfo.IsRTL())
		{
			if(mnTextLength)
			{
				maDblDXArray.reserve(mnTextLength);
				const sal_Int32 nFontWidth(0L == maFont.GetWidth() ? maFont.GetHeight() : maFont.GetWidth());
				const double fScaleFactor(0L != nFontWidth ? 1.0 / (double)nFontWidth : 1.0);
				
				for(xub_StrLen a(0); a < mnTextLength; a++)
				{
					maDblDXArray.push_back((double)rInfo.mpDXArray[a] * fScaleFactor);
				}
			}
		}

		// for ::std::sort
		bool operator<(const impPathTextPortion& rComp) const
		{
			if(mnParagraph < rComp.mnParagraph)
			{
				return true;
			}

			if(maOffset.getX() < rComp.maOffset.getX())
			{
				return true;
			}

			return (maOffset.getY() < rComp.maOffset.getY());
		}

		const basegfx::B2DVector& getOffset() const { return maOffset; }
		const String& getText() const { return maText; }
		xub_StrLen getTextStart() const { return mnTextStart; }
		xub_StrLen getTextLength() const { return mnTextLength; }
		sal_uInt16 getParagraph() const { return mnParagraph; }
		xub_StrLen getIndex() const { return mnIndex; }
		const SvxFont& getFont() const { return maFont; }
		bool isRTL() const { return mbRTL; }
		const ::std::vector< double >& getDoubleDXArray() const { return maDblDXArray; }
        const ::com::sun::star::lang::Locale& getLocale() const { return maLocale; }

		xub_StrLen getPortionIndex(xub_StrLen nIndex, xub_StrLen nLength) const
		{
			if(mbRTL)
			{
				return (mnTextStart + (mnTextLength - (nIndex + nLength)));
			}
			else
			{
				return (mnTextStart + nIndex);
			}
		}

		double getDisplayLength(xub_StrLen nIndex, xub_StrLen nLength) const
		{
			drawinglayer::primitive2d::TextLayouterDevice aTextLayouter;
			double fRetval(0.0);

			if(maFont.IsVertical())
			{
				fRetval = aTextLayouter.getTextHeight() * (double)nLength;
			}
			else
			{
				fRetval = aTextLayouter.getTextWidth(maText, getPortionIndex(nIndex, nLength), nLength);
			}

			return fRetval;
		}
	};
} // end of anonymous namespace

//////////////////////////////////////////////////////////////////////////////
// TextBreakup helper

namespace
{
	class impTextBreakupHandler
	{
		SdrOutliner&								mrOutliner;
		::std::vector< impPathTextPortion >			maPathTextPortions;

		DECL_LINK(decompositionPathTextPrimitive, DrawPortionInfo* );

	public:
		impTextBreakupHandler(SdrOutliner& rOutliner)
		:	mrOutliner(rOutliner)
		{
		}

		const ::std::vector< impPathTextPortion >& decompositionPathTextPrimitive()
		{
			// strip portions to maPathTextPortions
			mrOutliner.SetDrawPortionHdl(LINK(this, impTextBreakupHandler, decompositionPathTextPrimitive));
			mrOutliner.StripPortions();
			
			if(maPathTextPortions.size())
			{
				// sort portions by paragraph, x and y
				::std::sort(maPathTextPortions.begin(), maPathTextPortions.end());
			}

			return maPathTextPortions;
		}
	};

	IMPL_LINK(impTextBreakupHandler, decompositionPathTextPrimitive, DrawPortionInfo*, pInfo)
	{
		maPathTextPortions.push_back(impPathTextPortion(*pInfo));
		return 0;
	}
} // end of anonymous namespace

//////////////////////////////////////////////////////////////////////////////
// TextBreakup one poly and one paragraph helper

namespace
{
	class impPolygonParagraphHandler
	{
        const drawinglayer::attribute::SdrFormTextAttribute&        mrSdrFormTextAttribute;	// FormText parameters
		std::vector< drawinglayer::primitive2d::BasePrimitive2D* >&	mrDecomposition;		// destination primitive list
		std::vector< drawinglayer::primitive2d::BasePrimitive2D* >&	mrShadowDecomposition;	// destination primitive list for shadow
		Reference < com::sun::star::i18n::XBreakIterator >			mxBreak;				// break iterator

		double getParagraphTextLength(const ::std::vector< const impPathTextPortion* >& rTextPortions)
		{
			drawinglayer::primitive2d::TextLayouterDevice aTextLayouter;
			double fRetval(0.0);

			for(sal_uInt32 a(0L); a < rTextPortions.size(); a++)
			{
				const impPathTextPortion* pCandidate = rTextPortions[a];

				if(pCandidate && pCandidate->getTextLength())
				{
					aTextLayouter.setFont(pCandidate->getFont());
					fRetval += pCandidate->getDisplayLength(0L, pCandidate->getTextLength());
				}
			}

			return fRetval;
		}

		xub_StrLen getNextGlyphLen(const impPathTextPortion* pCandidate, xub_StrLen nPosition, const ::com::sun::star::lang::Locale& rFontLocale)
		{
			xub_StrLen nNextGlyphLen(1);

			if(mxBreak.is())
			{
				sal_Int32 nDone(0L);
				nNextGlyphLen = (xub_StrLen)mxBreak->nextCharacters(pCandidate->getText(), nPosition, 
					rFontLocale, CharacterIteratorMode::SKIPCELL, 1, nDone) - nPosition;
			}

			return nNextGlyphLen;
		}

	public:
		impPolygonParagraphHandler(
			const drawinglayer::attribute::SdrFormTextAttribute& rSdrFormTextAttribute, 
			std::vector< drawinglayer::primitive2d::BasePrimitive2D* >& rDecomposition, 
			std::vector< drawinglayer::primitive2d::BasePrimitive2D* >& rShadowDecomposition)
		:	mrSdrFormTextAttribute(rSdrFormTextAttribute),
			mrDecomposition(rDecomposition),
			mrShadowDecomposition(rShadowDecomposition)
		{
			// prepare BreakIterator
			Reference < XMultiServiceFactory > xMSF = ::comphelper::getProcessServiceFactory();
			Reference < XInterface > xInterface = xMSF->createInstance(::rtl::OUString::createFromAscii("com.sun.star.i18n.BreakIterator"));
			
			if(xInterface.is())
			{
				Any x = xInterface->queryInterface(::getCppuType((const Reference< XBreakIterator >*)0));
				x >>= mxBreak;
			}
		}

		void HandlePair(const basegfx::B2DPolygon rPolygonCandidate, const ::std::vector< const impPathTextPortion* >& rTextPortions)
		{
			// prepare polygon geometry, take into account as many parameters as possible
			basegfx::B2DPolygon aPolygonCandidate(rPolygonCandidate);
			const double fPolyLength(basegfx::tools::getLength(aPolygonCandidate));
			double fPolyEnd(fPolyLength);
			double fPolyStart(0.0);
			double fScaleFactor(1.0);

			if(mrSdrFormTextAttribute.getFormTextMirror())
			{
				aPolygonCandidate.flip();
			}

			if(mrSdrFormTextAttribute.getFormTextStart() 
                && (XFT_LEFT == mrSdrFormTextAttribute.getFormTextAdjust() 
                    || XFT_RIGHT == mrSdrFormTextAttribute.getFormTextAdjust()))
			{
				if(XFT_LEFT == mrSdrFormTextAttribute.getFormTextAdjust())
				{
					fPolyStart += mrSdrFormTextAttribute.getFormTextStart();

					if(fPolyStart > fPolyEnd)
					{
						fPolyStart = fPolyEnd;
					}
				}
				else
				{
					fPolyEnd -= mrSdrFormTextAttribute.getFormTextStart();

					if(fPolyEnd < fPolyStart)
					{
						fPolyEnd = fPolyStart;
					}
				}
			}

			if(XFT_LEFT != mrSdrFormTextAttribute.getFormTextAdjust())
			{
				// calculate total text length of this paragraph, some layout needs to be done
				const double fParagraphTextLength(getParagraphTextLength(rTextPortions));

				// check if text is too long for paragraph. If yes, handle as if left aligned (default),
				// but still take care of XFT_AUTOSIZE in that case
				const bool bTextTooLong(fParagraphTextLength > (fPolyEnd - fPolyStart));

				if(XFT_RIGHT == mrSdrFormTextAttribute.getFormTextAdjust())
				{
					if(!bTextTooLong)
					{
						// if right aligned, add difference to polygon start
						fPolyStart += ((fPolyEnd - fPolyStart) - fParagraphTextLength);
					}
				}
				else if(XFT_CENTER == mrSdrFormTextAttribute.getFormTextAdjust())
				{
					if(!bTextTooLong)
					{
						// if centered, add half of difference to polygon start
						fPolyStart += ((fPolyEnd - fPolyStart) - fParagraphTextLength) / 2.0;
					}
				}
				else if(XFT_AUTOSIZE == mrSdrFormTextAttribute.getFormTextAdjust())
				{
					// if scale, prepare scale factor between curve length and text length
					if(0.0 != fParagraphTextLength)
					{
						fScaleFactor = (fPolyEnd - fPolyStart) / fParagraphTextLength;
					}
				}
			}

			// handle text portions for this paragraph
			for(sal_uInt32 a(0L); a < rTextPortions.size() && fPolyStart < fPolyEnd; a++)
			{
				const impPathTextPortion* pCandidate = rTextPortions[a];
				basegfx::B2DVector aFontScaling;
				const drawinglayer::primitive2d::FontAttributes aCandidateFontAttributes(
                    drawinglayer::primitive2d::getFontAttributesFromVclFont(
                        aFontScaling, 
                        pCandidate->getFont(),
                        pCandidate->isRTL(),
                        false));

				if(pCandidate && pCandidate->getTextLength())
				{
					drawinglayer::primitive2d::TextLayouterDevice aTextLayouter;
					aTextLayouter.setFont(pCandidate->getFont());
					xub_StrLen nUsedTextLength(0);

					while(nUsedTextLength < pCandidate->getTextLength() && fPolyStart < fPolyEnd)
					{
						xub_StrLen nNextGlyphLen(getNextGlyphLen(pCandidate, pCandidate->getTextStart() + nUsedTextLength, pCandidate->getLocale()));

						// prepare portion length. Takes RTL sections into account.
						double fPortionLength(pCandidate->getDisplayLength(nUsedTextLength, nNextGlyphLen));

						if(XFT_AUTOSIZE == mrSdrFormTextAttribute.getFormTextAdjust())
						{
							// when scaling, expand portion length
							fPortionLength *= fScaleFactor;
						}

						// create transformation
						basegfx::B2DHomMatrix aNewTransformA, aNewTransformB, aNewShadowTransform;
						basegfx::B2DPoint aStartPos(basegfx::tools::getPositionAbsolute(aPolygonCandidate, fPolyStart, fPolyLength));
						basegfx::B2DPoint aEndPos(aStartPos);

						// add font scaling
						aNewTransformA.scale(aFontScaling.getX(), aFontScaling.getY());

						// prepare scaling of text primitive
						if(XFT_AUTOSIZE == mrSdrFormTextAttribute.getFormTextAdjust())
						{
							// when scaling, expand text primitive scaling
							aNewTransformA.scale(fScaleFactor, fScaleFactor);
						}

						// eventually create shadow primitives from aDecomposition and add to rDecomposition
						const bool bShadow(XFTSHADOW_NONE != mrSdrFormTextAttribute.getFormTextShadow());

						if(bShadow)
						{
							if(XFTSHADOW_NORMAL == mrSdrFormTextAttribute.getFormTextShadow())
							{
								aNewShadowTransform.translate(
                                    mrSdrFormTextAttribute.getFormTextShdwXVal(), 
                                    -mrSdrFormTextAttribute.getFormTextShdwYVal());
							}
							else // XFTSHADOW_SLANT
							{
								double fScaleValue(mrSdrFormTextAttribute.getFormTextShdwYVal() / 100.0);
								double fShearValue(-mrSdrFormTextAttribute.getFormTextShdwXVal() * F_PI1800);
								
								aNewShadowTransform.scale(1.0, fScaleValue);
								aNewShadowTransform.shearX(sin(fShearValue));
								aNewShadowTransform.scale(1.0, cos(fShearValue));
							}
						}

						switch(mrSdrFormTextAttribute.getFormTextStyle())
						{
							case XFT_ROTATE :
							{
								aEndPos = basegfx::tools::getPositionAbsolute(aPolygonCandidate, fPolyStart + fPortionLength, fPolyLength);
								const basegfx::B2DVector aDirection(aEndPos - aStartPos);
								aNewTransformB.rotate(atan2(aDirection.getY(), aDirection.getX()));
								aNewTransformB.translate(aStartPos.getX(), aStartPos.getY());
								
								break;
							}
							case XFT_UPRIGHT :
							{
								aNewTransformB.translate(aStartPos.getX() - (fPortionLength / 2.0), aStartPos.getY());
								
								break;
							}
							case XFT_SLANTX :
							{
								aEndPos = basegfx::tools::getPositionAbsolute(aPolygonCandidate, fPolyStart + fPortionLength, fPolyLength);
								const basegfx::B2DVector aDirection(aEndPos - aStartPos);
								const double fShearValue(atan2(aDirection.getY(), aDirection.getX()));
                                const double fSin(sin(fShearValue));
                                const double fCos(cos(fShearValue));

   								aNewTransformB.shearX(-fSin);

								// Scale may lead to objects without height since fCos == 0.0 is possible.
								// Renderers need to handle that, it's not a forbidden value and does not
								// need to be avoided
                                aNewTransformB.scale(1.0, fCos);
                                aNewTransformB.translate(aStartPos.getX() - (fPortionLength / 2.0), aStartPos.getY());
								
								break;
							}
							case XFT_SLANTY :
							{
								aEndPos = basegfx::tools::getPositionAbsolute(aPolygonCandidate, fPolyStart + fPortionLength, fPolyLength);
								const basegfx::B2DVector aDirection(aEndPos - aStartPos);
								const double fShearValue(atan2(aDirection.getY(), aDirection.getX()));
                                const double fCos(cos(fShearValue));
                                const double fTan(tan(fShearValue));
								
								// shear to 'stand' on the curve
								aNewTransformB.shearY(fTan);

								// scale in X to make as tight as needed. As with XFT_SLANT_X, this may
								// lead to primitives without width which the renderers will handle
   								aNewTransformA.scale(fCos, 1.0);

								aNewTransformB.translate(aStartPos.getX(), aStartPos.getY());
								
								break;
							}
							default : break; // XFT_NONE
						}

						// distance from path?
						if(mrSdrFormTextAttribute.getFormTextDistance())
						{
							if(aEndPos.equal(aStartPos))
							{
								aEndPos = basegfx::tools::getPositionAbsolute(aPolygonCandidate, fPolyStart + fPortionLength, fPolyLength);
							}

							// use back vector (aStartPos - aEndPos) here to get mirrored perpendicular as in old stuff
							const basegfx::B2DVector aPerpendicular(
                                basegfx::getNormalizedPerpendicular(aStartPos - aEndPos) * 
                                mrSdrFormTextAttribute.getFormTextDistance());
							aNewTransformB.translate(aPerpendicular.getX(), aPerpendicular.getY());
						}

						// shadow primitive creation
						if(bShadow)
						{
							if(pCandidate->getText().Len() && nNextGlyphLen)
							{
								const Color aShadowColor(mrSdrFormTextAttribute.getFormTextShdwColor());
								const basegfx::BColor aRGBShadowColor(aShadowColor.getBColor());
								const xub_StrLen nPortionIndex(pCandidate->getPortionIndex(nUsedTextLength, nNextGlyphLen));
								const ::std::vector< double > aNewDXArray(
									pCandidate->getDoubleDXArray().begin() + nPortionIndex,
									pCandidate->getDoubleDXArray().begin() + nPortionIndex + nNextGlyphLen);

								drawinglayer::primitive2d::TextSimplePortionPrimitive2D* pNew = 
                                    new drawinglayer::primitive2d::TextSimplePortionPrimitive2D(
									    aNewTransformB * aNewShadowTransform * aNewTransformA, 
									    pCandidate->getText(), 
									    nPortionIndex, 
									    nNextGlyphLen,
									    aNewDXArray, 
									    aCandidateFontAttributes, 
                                        pCandidate->getLocale(),
									    aRGBShadowColor);
								
								mrShadowDecomposition.push_back(pNew);
							}
						}

						// primitive creation
						if(pCandidate->getText().Len() && nNextGlyphLen)
						{
							const Color aColor(pCandidate->getFont().GetColor());
							const basegfx::BColor aRGBColor(aColor.getBColor());
							const xub_StrLen nPortionIndex(pCandidate->getPortionIndex(nUsedTextLength, nNextGlyphLen));
							const ::std::vector< double > aNewDXArray(
								pCandidate->getDoubleDXArray().begin() + nPortionIndex,
								pCandidate->getDoubleDXArray().begin() + nPortionIndex + nNextGlyphLen);

							drawinglayer::primitive2d::TextSimplePortionPrimitive2D* pNew = 
                                new drawinglayer::primitive2d::TextSimplePortionPrimitive2D(
								    aNewTransformB * aNewTransformA, 
								    pCandidate->getText(), 
								    nPortionIndex, 
								    nNextGlyphLen, 
								    aNewDXArray, 
								    aCandidateFontAttributes, 
                                    pCandidate->getLocale(),
								    aRGBColor);
							
							mrDecomposition.push_back(pNew);
						}

						// consume from portion // no += here, xub_StrLen is USHORT and the compiler will gererate a warning here
						nUsedTextLength = nUsedTextLength + nNextGlyphLen;

						// consume from polygon
						fPolyStart += fPortionLength;
					}
				}
			}
		}
	};
} // end of anonymous namespace

//////////////////////////////////////////////////////////////////////////////
// primitive decomposition helpers

namespace
{
	void impAddPolygonStrokePrimitives(
		const basegfx::B2DPolyPolygonVector& rB2DPolyPolyVector, 
		const basegfx::B2DHomMatrix& rTransform, 
		const drawinglayer::attribute::LineAttribute& rLineAttribute, 
		const drawinglayer::attribute::StrokeAttribute& rStrokeAttribute, 
		std::vector< drawinglayer::primitive2d::BasePrimitive2D* >& rTarget)
	{
		for(basegfx::B2DPolyPolygonVector::const_iterator aPolygon(rB2DPolyPolyVector.begin()); aPolygon != rB2DPolyPolyVector.end(); aPolygon++)
		{
			// prepare PolyPolygons
			basegfx::B2DPolyPolygon aB2DPolyPolygon = *aPolygon;
			aB2DPolyPolygon.transform(rTransform);
			
			for(sal_uInt32 a(0L); a < aB2DPolyPolygon.count(); a++)
			{
				// create one primitive per polygon
				drawinglayer::primitive2d::PolygonStrokePrimitive2D* pNew = 
                    new drawinglayer::primitive2d::PolygonStrokePrimitive2D(
                        aB2DPolyPolygon.getB2DPolygon(a), rLineAttribute, rStrokeAttribute);
				rTarget.push_back(pNew);
			}
		}
	}

	drawinglayer::primitive2d::Primitive2DSequence impAddPathTextOutlines(
		const std::vector< drawinglayer::primitive2d::BasePrimitive2D* >& rSource, 
        const drawinglayer::attribute::SdrFormTextOutlineAttribute& rOutlineAttribute)
	{
		std::vector< drawinglayer::primitive2d::BasePrimitive2D* > aNewPrimitives;
		
		for(sal_uInt32 a(0L); a < rSource.size(); a++)
		{
			const drawinglayer::primitive2d::TextSimplePortionPrimitive2D* pTextCandidate = dynamic_cast< const drawinglayer::primitive2d::TextSimplePortionPrimitive2D* >(rSource[a]);

			if(pTextCandidate)
			{
				basegfx::B2DPolyPolygonVector aB2DPolyPolyVector;
			    basegfx::B2DHomMatrix aPolygonTransform;

                // get text outlines and their object transformation
                pTextCandidate->getTextOutlinesAndTransformation(aB2DPolyPolyVector, aPolygonTransform);

				if(aB2DPolyPolyVector.size())
                {
				    // create stroke primitives
				    std::vector< drawinglayer::primitive2d::BasePrimitive2D* > aStrokePrimitives;
				    impAddPolygonStrokePrimitives(
                        aB2DPolyPolyVector, 
                        aPolygonTransform, 
                        rOutlineAttribute.getLineAttribute(), 
                        rOutlineAttribute.getStrokeAttribute(), 
                        aStrokePrimitives);
				    const sal_uInt32 nStrokeCount(aStrokePrimitives.size());

				    if(nStrokeCount)
				    {
					    if(rOutlineAttribute.getTransparence())
					    {
						    // create UnifiedAlphaPrimitive2D
						    drawinglayer::primitive2d::Primitive2DSequence aStrokePrimitiveSequence(nStrokeCount);

						    for(sal_uInt32 b(0L); b < nStrokeCount; b++)
						    {
							    aStrokePrimitiveSequence[b] = drawinglayer::primitive2d::Primitive2DReference(aStrokePrimitives[b]);
						    }

						    drawinglayer::primitive2d::UnifiedAlphaPrimitive2D* pNew2 = 
                                new drawinglayer::primitive2d::UnifiedAlphaPrimitive2D(
                                    aStrokePrimitiveSequence, 
                                    (double)rOutlineAttribute.getTransparence() / 100.0);
						    aNewPrimitives.push_back(pNew2);
					    }
					    else
					    {
						    // add polygons to rDecomposition as polygonStrokePrimitives
						    aNewPrimitives.insert(aNewPrimitives.end(), aStrokePrimitives.begin(), aStrokePrimitives.end());
					    }
                    }
				}
			}
		}

		const sal_uInt32 nNewCount(aNewPrimitives.size());

		if(nNewCount)
		{
			drawinglayer::primitive2d::Primitive2DSequence aRetval(nNewCount);

			for(sal_uInt32 a(0L); a < nNewCount; a++)
			{
				aRetval[a] = drawinglayer::primitive2d::Primitive2DReference(aNewPrimitives[a]);
			}

			return aRetval;
		}
		else
		{
			return drawinglayer::primitive2d::Primitive2DSequence();
		}
	}
} // end of anonymous namespace

//////////////////////////////////////////////////////////////////////////////
// primitive decomposition

void SdrTextObj::impDecomposePathTextPrimitive(
	drawinglayer::primitive2d::Primitive2DSequence& rTarget, 
	const drawinglayer::primitive2d::SdrPathTextPrimitive2D& rSdrPathTextPrimitive,
	const drawinglayer::geometry::ViewInformation2D& aViewInformation) const
{
	drawinglayer::primitive2d::Primitive2DSequence aRetvalA;
	drawinglayer::primitive2d::Primitive2DSequence aRetvalB;

	// prepare outliner
	SdrOutliner& rOutliner = ImpGetDrawOutliner();
	rOutliner.SetUpdateMode(true);
	rOutliner.Clear();
	rOutliner.SetPaperSize(Size(LONG_MAX,LONG_MAX));
	rOutliner.SetText(rSdrPathTextPrimitive.getOutlinerParaObject());

	// set visualizing page at Outliner; needed e.g. for PageNumberField decomposition
	rOutliner.setVisualizedPage(GetSdrPageFromXDrawPage(aViewInformation.getVisualizedPage()));

	// now break up to text portions
	impTextBreakupHandler aConverter(rOutliner);
	const ::std::vector< impPathTextPortion > rPathTextPortions = aConverter.decompositionPathTextPrimitive();

	if(rPathTextPortions.size())
	{
		// get FormText and polygon values
        const drawinglayer::attribute::SdrFormTextAttribute& rFormTextAttribute = rSdrPathTextPrimitive.getSdrFormTextAttribute();
		const basegfx::B2DPolyPolygon& rPathPolyPolygon(rSdrPathTextPrimitive.getPathPolyPolygon());

		// get loop count
		sal_uInt32 nLoopCount(rPathPolyPolygon.count());

		if(rOutliner.GetParagraphCount() < nLoopCount)
		{
			nLoopCount = rOutliner.GetParagraphCount();
		}

		if(nLoopCount)
		{
			// prepare common decomposition stuff
			std::vector< drawinglayer::primitive2d::BasePrimitive2D* > aRegularDecomposition;
			std::vector< drawinglayer::primitive2d::BasePrimitive2D* > aShadowDecomposition;
			impPolygonParagraphHandler aPolygonParagraphHandler(
                rFormTextAttribute, aRegularDecomposition, aShadowDecomposition);
			sal_uInt32 a;

			for(a = 0L; a < nLoopCount; a++)
			{
				// filter text portions for this paragraph
				::std::vector< const impPathTextPortion* > aParagraphTextPortions;

				for(sal_uInt32 b(0L); b < rPathTextPortions.size(); b++)
				{
					const impPathTextPortion& rCandidate = rPathTextPortions[b];

					if(rCandidate.getParagraph() == a)
					{
						aParagraphTextPortions.push_back(&rCandidate);
					}
				}

				// handle data pair polygon/ParagraphTextPortions
				if(aParagraphTextPortions.size())
				{
					aPolygonParagraphHandler.HandlePair(rPathPolyPolygon.getB2DPolygon(a), aParagraphTextPortions);
				}
			}

			const sal_uInt32 nShadowCount(aShadowDecomposition.size());
			const sal_uInt32 nRegularCount(aRegularDecomposition.size());
			
			if(nShadowCount)
			{
				// add shadow primitives to decomposition
				aRetvalA.realloc(nShadowCount);

				for(a = 0L; a < nShadowCount; a++)
				{
					aRetvalA[a] = drawinglayer::primitive2d::Primitive2DReference(aShadowDecomposition[a]);
				}

				// evtl. add shadow outlines
				if(rFormTextAttribute.getFormTextOutline() && rFormTextAttribute.getShadowOutline())
				{
					const drawinglayer::primitive2d::Primitive2DSequence aOutlines(
                        impAddPathTextOutlines(aShadowDecomposition, *rFormTextAttribute.getShadowOutline()));
					drawinglayer::primitive2d::appendPrimitive2DSequenceToPrimitive2DSequence(aRetvalA, aOutlines);
				}
			}

			if(nRegularCount)
			{
				// add normal primitives to decomposition
				aRetvalB.realloc(nRegularCount);

				for(a = 0L; a < nRegularCount; a++)
				{
					aRetvalB[a] = drawinglayer::primitive2d::Primitive2DReference(aRegularDecomposition[a]);
				}

				// evtl. add outlines
				if(rFormTextAttribute.getFormTextOutline() && rFormTextAttribute.getOutline())
				{
					const drawinglayer::primitive2d::Primitive2DSequence aOutlines(
                        impAddPathTextOutlines(aRegularDecomposition, *rFormTextAttribute.getOutline()));
					drawinglayer::primitive2d::appendPrimitive2DSequenceToPrimitive2DSequence(aRetvalB, aOutlines);
				}
			}
		}
	}

	// cleanup outliner
	rOutliner.SetDrawPortionHdl(Link());
	rOutliner.Clear();
	rOutliner.setVisualizedPage(0);

	// concatenate all results
	drawinglayer::primitive2d::appendPrimitive2DSequenceToPrimitive2DSequence(rTarget, aRetvalA);
	drawinglayer::primitive2d::appendPrimitive2DSequenceToPrimitive2DSequence(rTarget, aRetvalB);
}

//////////////////////////////////////////////////////////////////////////////
// eof
