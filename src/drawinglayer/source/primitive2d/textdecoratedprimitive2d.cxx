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

#include <drawinglayer/primitive2d/textdecoratedprimitive2d.hxx>
#include <drawinglayer/primitive2d/textlayoutdevice.hxx>
#include <drawinglayer/primitive2d/polygonprimitive2d.hxx>
#include <drawinglayer/attribute/strokeattribute.hxx>
#include <drawinglayer/primitive2d/drawinglayer_primitivetypes2d.hxx>
#include <basegfx/matrix/b2dhommatrixtools.hxx>
#include <comphelper/processfactory.hxx>
#include <com/sun/star/i18n/WordType.hpp>
#include <drawinglayer/primitive2d/texteffectprimitive2d.hxx>
#include <drawinglayer/primitive2d/shadowprimitive2d.hxx>
#include <com/sun/star/i18n/XBreakIterator.hpp>
#include <drawinglayer/primitive2d/transformprimitive2d.hxx>

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive2d
	{
        void TextDecoratedPortionPrimitive2D::impCreateTextLine(
            std::vector< Primitive2DReference >& rTarget,
            basegfx::DecomposedB2DHomMatrixContainer& rDecTrans,
            const basegfx::B2DHomMatrix &rUnscaledTransform,
            FontUnderline eLineStyle,
            double fLineOffset,
            double fLineHeight,
            double fLineWidth,
            const basegfx::BColor& rLineColor) const
        {
            bool bDoubleLine(false);
            bool bWaveLine(false);
            bool bBoldLine(false);
            const int* pDotDashArray(0);
            basegfx::B2DLineJoin eLineJoin(basegfx::B2DLINEJOIN_NONE);

            static const int aDottedArray[]     = { 1, 1, 0};               // DOTTED LINE
            static const int aDotDashArray[]    = { 1, 1, 4, 1, 0};         // DASHDOT
            static const int aDashDotDotArray[] = { 1, 1, 1, 1, 4, 1, 0};   // DASHDOTDOT
            static const int aDashedArray[]     = { 5, 2, 0};               // DASHED LINE
            static const int aLongDashArray[]   = { 7, 2, 0};               // LONGDASH

            switch(eLineStyle)
            {
                default: // case FONT_UNDERLINE_SINGLE:
                {
                    break;
                }
                case FONT_UNDERLINE_DOUBLE:
                {
                    bDoubleLine = true;
                    break;
                }
                case FONT_UNDERLINE_DOTTED:
                {
                    pDotDashArray = aDottedArray;
                    break;
                }
                case FONT_UNDERLINE_DASH:
                {
                    pDotDashArray = aDashedArray;
                    break;
                }
                case FONT_UNDERLINE_LONGDASH:
                {
                    pDotDashArray = aLongDashArray;
                    break;
                }
                case FONT_UNDERLINE_DASHDOT:
                {
                    pDotDashArray = aDotDashArray;
                    break;
                }
                case FONT_UNDERLINE_DASHDOTDOT:
                {
                    pDotDashArray = aDashDotDotArray;
                    break;
                }
                case FONT_UNDERLINE_SMALLWAVE:
                {
                    bWaveLine = true;
                    break;
                }
                case FONT_UNDERLINE_WAVE:
                {
                    bWaveLine = true;
                    break;
                }
                case FONT_UNDERLINE_DOUBLEWAVE:
                {
                    bDoubleLine = true;
                    bWaveLine = true;
                    break;
                }
                case FONT_UNDERLINE_BOLD:
                {
                    bBoldLine = true;
                    break;
                }
                case FONT_UNDERLINE_BOLDDOTTED:
                {
                    bBoldLine = true;
                    pDotDashArray = aDottedArray;
                    break;
                }
                case FONT_UNDERLINE_BOLDDASH:
                {
                    bBoldLine = true;
                    pDotDashArray = aDashedArray;
                    break;
                }
                case FONT_UNDERLINE_BOLDLONGDASH:
                {
                    bBoldLine = true;
                    pDotDashArray = aLongDashArray;
                    break;
                }
                case FONT_UNDERLINE_BOLDDASHDOT:
                {
                    bBoldLine = true;
                    pDotDashArray = aDotDashArray;
                    break;
                }
                case FONT_UNDERLINE_BOLDDASHDOTDOT:
                {
                    bBoldLine = true;
                    pDotDashArray = aDashDotDotArray;
                    break;
                }
                case FONT_UNDERLINE_BOLDWAVE:
                {
                    bWaveLine = true;
                    bBoldLine = true;
                    break;
                }
            }

            if(bBoldLine)
            {
                fLineHeight *= 2.0;
            }

            if(bDoubleLine)
            {
                fLineOffset -= 0.50 * fLineHeight;
                fLineHeight *= 0.64;
            }

            if(bWaveLine)
            {
                eLineJoin = basegfx::B2DLINEJOIN_ROUND;
                fLineHeight *= 0.25;
            }

            // prepare Line and Stroke Attributes
            const attribute::LineAttribute aLineAttribute(rLineColor, fLineHeight, eLineJoin);
            attribute::StrokeAttribute aStrokeAttribute;

            if(pDotDashArray)
            {
                ::std::vector< double > aDoubleArray;

                for(const int* p = pDotDashArray; *p; ++p)
                {
                    aDoubleArray.push_back((double)(*p) * fLineHeight);
                }

                aStrokeAttribute = attribute::StrokeAttribute(aDoubleArray);
            }

            // create base polygon and new primitive
            basegfx::B2DPolygon aLine;
            Primitive2DReference aNewPrimitive;

            aLine.append(basegfx::B2DPoint(0.0, fLineOffset));
            aLine.append(basegfx::B2DPoint(fLineWidth, fLineOffset));
            aLine.transform(rUnscaledTransform);

            if(bWaveLine)
            {
                double fWaveWidth(10.6 * fLineHeight);

                if(FONT_UNDERLINE_SMALLWAVE == eLineStyle)
                {
                    fWaveWidth *= 0.7;
                }
                else if(FONT_UNDERLINE_WAVE == eLineStyle)
                {
                    // extra multiply to get the same WaveWidth as with the bold version
                    fWaveWidth *= 2.0;
                }

                aNewPrimitive = Primitive2DReference(new PolygonWavePrimitive2D(aLine, aLineAttribute, aStrokeAttribute, fWaveWidth, fWaveWidth * 0.5));
            }
            else
            {
                aNewPrimitive = Primitive2DReference(new PolygonStrokePrimitive2D(aLine, aLineAttribute, aStrokeAttribute));
            }

            // add primitive
            rTarget.push_back(aNewPrimitive);

            if(bDoubleLine)
            {
                // double line, create 2nd primitive with offset using TransformPrimitive based on
                // already created NewPrimitive
                double fLineDist(2.3 * fLineHeight);
                
                if(bWaveLine)
                {
                    fLineDist = 6.3 * fLineHeight;
                }

                basegfx::B2DHomMatrix aTransform;

                // move base point of text to 0.0 and de-rotate
                aTransform.translate(-rDecTrans.getTranslate().getX(), -rDecTrans.getTranslate().getY());
                aTransform.rotate(-rDecTrans.getRotate());

                // translate in Y by offset
                aTransform.translate(0.0, fLineDist);

                // move back and rotate
                aTransform.rotate(rDecTrans.getRotate());
                aTransform.translate(rDecTrans.getTranslate().getX(), rDecTrans.getTranslate().getY());

                // add transform primitive
                const Primitive2DSequence aContent(&aNewPrimitive, 1);
                rTarget.push_back(Primitive2DReference(new TransformPrimitive2D(aTransform, aContent)));
            }
        }

        void TextDecoratedPortionPrimitive2D::impCreateGeometryContent(
            std::vector< Primitive2DReference >& rTarget,
            basegfx::DecomposedB2DHomMatrixContainer& rDecTrans,
            const String& rText,
			xub_StrLen aTextPosition,
			xub_StrLen aTextLength,
            const ::std::vector< double >& rDXArray,
            const FontAttributes& rFontAttributes) const
        {
            // create the SimpleTextPrimitive needed in any case
	        rTarget.push_back(Primitive2DReference(
                new TextSimplePortionPrimitive2D(
                    rDecTrans.getB2DHomMatrix(),
                    rText,
				    aTextPosition,
				    aTextLength,
                    rDXArray,
                    rFontAttributes,
                    getLocale(),
                    getFontColor())));

			// see if something else needs to be done
            const bool bOverlineUsed(FONT_UNDERLINE_NONE != getFontOverline());
            const bool bUnderlineUsed(FONT_UNDERLINE_NONE != getFontUnderline());
			const bool bStrikeoutUsed(FONT_STRIKEOUT_NONE != getFontStrikeout());

            if(bUnderlineUsed || bStrikeoutUsed || bOverlineUsed)
			{
				// common preparations
    		    basegfx::B2DHomMatrix aUnscaledTransform;
			    TextLayouterDevice aTextLayouter;

				// unscaled is needed since scale contains already the font size
				aUnscaledTransform.shearX(rDecTrans.getShearX());
				aUnscaledTransform.rotate(rDecTrans.getRotate());
				aUnscaledTransform.translate(rDecTrans.getTranslate().getX(), rDecTrans.getTranslate().getY());

                // TextLayouterDevice is needed to get metrics for text decorations like
                // underline/strikeout/emphasis marks from it. For setup, the font size is needed
			    aTextLayouter.setFontAttributes(
                    getFontAttributes(), 
                    rDecTrans.getScale().getX(), 
                    rDecTrans.getScale().getY(),
                    getLocale());

				// get text width
				double fTextWidth(0.0);

				if(rDXArray.empty())
				{
					fTextWidth = aTextLayouter.getTextWidth(rText, aTextPosition, aTextLength);
				}
				else
				{
					fTextWidth = rDXArray.back() * rDecTrans.getScale().getX();
                    const double fFontScaleX(rDecTrans.getScale().getX());
                    
                    if(!basegfx::fTools::equal(fFontScaleX, 1.0) 
                        && !basegfx::fTools::equalZero(fFontScaleX))
                    {
                        // need to take FontScaling out of the DXArray
                        fTextWidth /= fFontScaleX;
                    }
				}

                if(bOverlineUsed)
                {
                    // create primitive geometry for overline
                    impCreateTextLine(rTarget, rDecTrans, aUnscaledTransform, getFontOverline(), aTextLayouter.getOverlineOffset(),
                                      aTextLayouter.getOverlineHeight(), fTextWidth, getOverlineColor());
                }

				if(bUnderlineUsed)
				{
					// create primitive geometry for underline
                    impCreateTextLine(rTarget, rDecTrans, aUnscaledTransform, getFontUnderline(), aTextLayouter.getUnderlineOffset(),
                                      aTextLayouter.getUnderlineHeight(), fTextWidth, getTextlineColor());
                }

				if(bStrikeoutUsed)
				{
					// create primitive geometry for strikeout
                    if(FONT_STRIKEOUT_SLASH == getFontStrikeout() || FONT_STRIKEOUT_X == getFontStrikeout())
                    {
                        // strikeout with character
                        const sal_Unicode aStrikeoutChar(FONT_STRIKEOUT_SLASH == getFontStrikeout() ? '/' : 'X');
			            const String aSingleCharString(aStrikeoutChar);
			            const double fStrikeCharWidth(aTextLayouter.getTextWidth(aSingleCharString, 0, 1));
                        const double fStrikeCharCount(fabs(fTextWidth/fStrikeCharWidth));
			            const sal_uInt32 nStrikeCharCount(static_cast< sal_uInt32 >(fStrikeCharCount + 0.5));
						std::vector<double> aDXArray(nStrikeCharCount);
			            String aStrikeoutString;

                        for(sal_uInt32 a(0); a < nStrikeCharCount; a++)
                        {
				            aStrikeoutString += aSingleCharString;
				            aDXArray[a] = (a + 1) * fStrikeCharWidth;
                        }

    				    rTarget.push_back(Primitive2DReference(
                            new TextSimplePortionPrimitive2D(
                                rDecTrans.getB2DHomMatrix(),
                                aStrikeoutString,
							    0,
							    aStrikeoutString.Len(),
                                aDXArray,
                                rFontAttributes,
                                getLocale(),
                                getFontColor())));
                    }
                    else
                    {
                        // strikeout with geometry
			            double fStrikeoutHeight(aTextLayouter.getUnderlineHeight());
			            double fStrikeoutOffset(aTextLayouter.getStrikeoutOffset());
			            bool bDoubleLine(false);

                        // set line attribute
                        switch(getFontStrikeout())
			            {
                            default : // case primitive2d::FONT_STRIKEOUT_SINGLE:
                            {
					            break;
                            }
				            case primitive2d::FONT_STRIKEOUT_DOUBLE:
                            {
					            bDoubleLine = true;
					            break;
                            }
				            case primitive2d::FONT_STRIKEOUT_BOLD:
                            {
					            fStrikeoutHeight *= 2.0;
					            break;
                            }
			            }

			            if(bDoubleLine)
			            {
				            fStrikeoutOffset -= 0.50 * fStrikeoutHeight;
				            fStrikeoutHeight *= 0.64;
			            }

					    // create base polygon and new primitive
			            basegfx::B2DPolygon aStrikeoutLine;

					    aStrikeoutLine.append(basegfx::B2DPoint(0.0, -fStrikeoutOffset));
				        aStrikeoutLine.append(basegfx::B2DPoint(fTextWidth, -fStrikeoutOffset));
                        aStrikeoutLine.transform(aUnscaledTransform);

				        const attribute::LineAttribute aLineAttribute(getFontColor(), fStrikeoutHeight, basegfx::B2DLINEJOIN_NONE);
    					Primitive2DReference aNewPrimitive(new PolygonStrokePrimitive2D(aStrikeoutLine, aLineAttribute));

    					// add primitive
                        rTarget.push_back(aNewPrimitive);

                        if(bDoubleLine)
					    {
						    // double line, create 2nd primitive with offset using TransformPrimitive based on
						    // already created NewPrimitive
					        const double fLineDist(2.0 * fStrikeoutHeight);
						    basegfx::B2DHomMatrix aTransform;

						    // move base point of text to 0.0 and de-rotate
						    aTransform.translate(-rDecTrans.getTranslate().getX(), -rDecTrans.getTranslate().getY());
						    aTransform.rotate(-rDecTrans.getRotate());

						    // translate in Y by offset
						    aTransform.translate(0.0, -fLineDist);

						    // move back and rotate
						    aTransform.rotate(rDecTrans.getRotate());
						    aTransform.translate(rDecTrans.getTranslate().getX(), rDecTrans.getTranslate().getY());

						    // add transform primitive
						    const Primitive2DSequence aContent(&aNewPrimitive, 1);
						    rTarget.push_back(Primitive2DReference(new TransformPrimitive2D(aTransform, aContent)));
					    }
                    }
				}
			}

            // TODO: Handle Font Emphasis Above/Below
        }

		void TextDecoratedPortionPrimitive2D::impCorrectTextBoundary(::com::sun::star::i18n::Boundary& rNextWordBoundary) const
		{
			// truncate aNextWordBoundary to min/max possible values. This is necessary since the word start may be
			// before/after getTextPosition() when a long string is the content and getTextPosition()
			// is right inside a word. Same for end.
			const sal_Int32 aMinPos(static_cast< sal_Int32 >(getTextPosition()));
			const sal_Int32 aMaxPos(aMinPos + static_cast< sal_Int32 >(getTextLength()));

			if(rNextWordBoundary.startPos < aMinPos)
			{
				rNextWordBoundary.startPos = aMinPos;
			}
			else if(rNextWordBoundary.startPos > aMaxPos)
			{
				rNextWordBoundary.startPos = aMaxPos;
			}

			if(rNextWordBoundary.endPos < aMinPos)
			{
				rNextWordBoundary.endPos = aMinPos;
			}
			else if(rNextWordBoundary.endPos > aMaxPos)
			{
				rNextWordBoundary.endPos = aMaxPos;
			}
		}

        void TextDecoratedPortionPrimitive2D::impSplitSingleWords(
            std::vector< Primitive2DReference >& rTarget,
            basegfx::DecomposedB2DHomMatrixContainer& rDecTrans) const
        {
            // break iterator support
            // made static so it only needs to be fetched once, even with many single
            // constructed VclMetafileProcessor2D. It's still incarnated on demand,
            // but exists for OOo runtime now by purpose.
            static ::com::sun::star::uno::Reference< ::com::sun::star::i18n::XBreakIterator > xLocalBreakIterator;

			if(!xLocalBreakIterator.is())
            {
                ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory > xMSF(::comphelper::getProcessServiceFactory());
                xLocalBreakIterator.set(xMSF->createInstance(rtl::OUString::createFromAscii("com.sun.star.i18n.BreakIterator")), ::com::sun::star::uno::UNO_QUERY);
            }

            if(xLocalBreakIterator.is() && getTextLength())
            {
                // init word iterator, get first word and truncate to possibilities
                ::com::sun::star::i18n::Boundary aNextWordBoundary(xLocalBreakIterator->getWordBoundary(
                    getText(), getTextPosition(), getLocale(), ::com::sun::star::i18n::WordType::ANYWORD_IGNOREWHITESPACES, sal_True));

                if(aNextWordBoundary.endPos == getTextPosition())
                {
                    // backward hit, force next word
                    aNextWordBoundary = xLocalBreakIterator->getWordBoundary(
                        getText(), getTextPosition() + 1, getLocale(), ::com::sun::star::i18n::WordType::ANYWORD_IGNOREWHITESPACES, sal_True);
                }

				impCorrectTextBoundary(aNextWordBoundary);

				// prepare new font attributes WITHOUT outline
                const FontAttributes aNewFontAttributes(
                    getFontAttributes().getFamilyName(),
                    getFontAttributes().getStyleName(),
                    getFontAttributes().getWeight(),
                    getFontAttributes().getSymbol(),
                    getFontAttributes().getVertical(),
                    getFontAttributes().getItalic(),
                    false,             // no outline anymore, handled locally
                    getFontAttributes().getRTL(),
                    getFontAttributes().getBiDiStrong());

				if(aNextWordBoundary.startPos == getTextPosition() && aNextWordBoundary.endPos == getTextLength())
				{
					// it IS only a single word, handle as one word
	                impCreateGeometryContent(rTarget, rDecTrans, getText(), getTextPosition(), getTextLength(), getDXArray(), aNewFontAttributes);
				}
				else
				{
					// prepare TextLayouter
					const bool bNoDXArray(getDXArray().empty());
					TextLayouterDevice aTextLayouter;

					if(bNoDXArray)
					{
						// ..but only completely when no DXArray
						aTextLayouter.setFontAttributes(
                            getFontAttributes(), 
                            rDecTrans.getScale().getX(), 
                            rDecTrans.getScale().getY(),
                            getLocale());
					}

					// do iterate over single words
					while(aNextWordBoundary.startPos != aNextWordBoundary.endPos)
					{
						// prepare values for new portion
						const xub_StrLen nNewTextStart(static_cast< xub_StrLen >(aNextWordBoundary.startPos));
						const xub_StrLen nNewTextEnd(static_cast< xub_StrLen >(aNextWordBoundary.endPos));

						// prepare transform for the single word
						basegfx::B2DHomMatrix aNewTransform;
						::std::vector< double > aNewDXArray;
						const bool bNewStartIsNotOldStart(nNewTextStart > getTextPosition());

						if(!bNoDXArray)
						{
							// prepare new DXArray for the single word
							aNewDXArray = ::std::vector< double >(
								getDXArray().begin() + static_cast< sal_uInt32 >(nNewTextStart - getTextPosition()), 
								getDXArray().begin() + static_cast< sal_uInt32 >(nNewTextEnd - getTextPosition()));
						}

						if(bNewStartIsNotOldStart)
						{
							// needs to be moved to a new start position
							double fOffset(0.0);
							
							if(bNoDXArray)
							{
								// evaluate using TextLayouter
								fOffset = aTextLayouter.getTextWidth(getText(), getTextPosition(), nNewTextStart);
							}
							else
							{
								// get from DXArray
								const sal_uInt32 nIndex(static_cast< sal_uInt32 >(nNewTextStart - getTextPosition()));
								fOffset = getDXArray()[nIndex - 1];
							}

                            // need offset without FontScale for building the new transformation. The
                            // new transformation will be multiplied with the current text transformation
                            // so FontScale would be double
							double fOffsetNoScale(fOffset);
                            const double fFontScaleX(rDecTrans.getScale().getX());
                            
                            if(!basegfx::fTools::equal(fFontScaleX, 1.0) 
                                && !basegfx::fTools::equalZero(fFontScaleX))
                            {
                                fOffsetNoScale /= fFontScaleX;
                            }

							// apply needed offset to transformation
                            aNewTransform.translate(fOffsetNoScale, 0.0);

							if(!bNoDXArray)
							{
								// DXArray values need to be corrected with the offset, too. Here,
                                // take the scaled offset since the DXArray is scaled
								const sal_uInt32 nArraySize(aNewDXArray.size());

								for(sal_uInt32 a(0); a < nArraySize; a++)
								{
									aNewDXArray[a] -= fOffset;
								}
							}
						}

						// add text transformation to new transformation
						aNewTransform *= rDecTrans.getB2DHomMatrix();

						// create geometry content for the single word. Do not forget
						// to use the new transformation
						basegfx::DecomposedB2DHomMatrixContainer aDecTrans(aNewTransform);
						
						impCreateGeometryContent(rTarget, aDecTrans, getText(), nNewTextStart, 
							nNewTextEnd - nNewTextStart, aNewDXArray, aNewFontAttributes);

                        if(aNextWordBoundary.endPos >= getTextPosition() + getTextLength())
                        {
                            // end reached
                            aNextWordBoundary.startPos = aNextWordBoundary.endPos;
                        }
                        else
                        {
                            // get new word portion
                            const sal_Int32 nLastEndPos(aNextWordBoundary.endPos);

                            aNextWordBoundary = xLocalBreakIterator->getWordBoundary(
                                getText(), aNextWordBoundary.endPos, getLocale(), 
                                ::com::sun::star::i18n::WordType::ANYWORD_IGNOREWHITESPACES, sal_True);

                            if(nLastEndPos == aNextWordBoundary.endPos)
                            {
                                // backward hit, force next word
                                aNextWordBoundary = xLocalBreakIterator->getWordBoundary(
                                    getText(), nLastEndPos + 1, getLocale(), 
                                    ::com::sun::star::i18n::WordType::ANYWORD_IGNOREWHITESPACES, sal_True);
                            }

                            impCorrectTextBoundary(aNextWordBoundary);
                        }
					}
				}
            }
        }

		Primitive2DSequence TextDecoratedPortionPrimitive2D::createLocalDecomposition(const geometry::ViewInformation2D& /*rViewInformation*/) const
        {
            std::vector< Primitive2DReference > aNewPrimitives;
            basegfx::DecomposedB2DHomMatrixContainer aDecTrans(getTextTransform());
            Primitive2DSequence aRetval;

            // create basic geometry such as SimpleTextPrimitive, Overline, Underline,
            // Strikeout, etc...
            if(getWordLineMode())
            {
                // support for single word mode
                impSplitSingleWords(aNewPrimitives, aDecTrans);
            }
            else
            {
                // prepare new font attributes WITHOUT outline
                const FontAttributes aNewFontAttributes(
                    getFontAttributes().getFamilyName(),
                    getFontAttributes().getStyleName(),
                    getFontAttributes().getWeight(),
                    getFontAttributes().getSymbol(),
                    getFontAttributes().getVertical(),
                    getFontAttributes().getItalic(),
                    false,             // no outline anymore, handled locally
                    getFontAttributes().getRTL(),
                    getFontAttributes().getBiDiStrong());

				// handle as one word
                impCreateGeometryContent(aNewPrimitives, aDecTrans, getText(), getTextPosition(), getTextLength(), getDXArray(), aNewFontAttributes);
            }

            // convert to Primitive2DSequence
            const sal_uInt32 nMemberCount(aNewPrimitives.size());

			if(nMemberCount)
            {
                aRetval.realloc(nMemberCount);

                for(sal_uInt32 a(0); a < nMemberCount; a++)
                {
                    aRetval[a] = aNewPrimitives[a];
                }
            }

            // Handle Shadow, Outline and FontRelief
            if(aRetval.hasElements())
            {
                // outline AND shadow depend on NO FontRelief (see dialog)
                const bool bHasFontRelief(FONT_RELIEF_NONE != getFontRelief());
                const bool bHasShadow(!bHasFontRelief && getShadow());
                const bool bHasOutline(!bHasFontRelief && getFontAttributes().getOutline());

                if(bHasShadow || bHasFontRelief || bHasOutline)
                {
                    Primitive2DReference aShadow;

                    if(bHasShadow)
                    {
                        // create shadow with current content (in aRetval). Text shadow
                        // is constant, relative to font size, rotated with the text and has a
                        // constant color.
                        // shadow parameter values
                        static double fFactor(1.0 / 24.0);
                        const double fTextShadowOffset(aDecTrans.getScale().getY() * fFactor);
                        static basegfx::BColor aShadowColor(0.3, 0.3, 0.3);

                        // preapare shadow transform matrix
                        basegfx::B2DHomMatrix aShadowTransform;
                        aShadowTransform.translate(fTextShadowOffset, fTextShadowOffset);

                        // create shadow primitive
                        aShadow = Primitive2DReference(new ShadowPrimitive2D(
                            aShadowTransform,
                            aShadowColor,
                            aRetval));
                    }

                    if(bHasFontRelief)
                    {
                        // create emboss using an own helper primitive since this will
                        // be view-dependent
						const basegfx::BColor aBBlack(0.0, 0.0, 0.0);
						const bool bDefaultTextColor(aBBlack == getFontColor());
						TextEffectStyle2D aTextEffectStyle2D(TEXTEFFECTSTYLE2D_RELIEF_EMBOSSED);

						if(bDefaultTextColor)
						{
							if(FONT_RELIEF_ENGRAVED == getFontRelief())
							{
								aTextEffectStyle2D = TEXTEFFECTSTYLE2D_RELIEF_ENGRAVED_DEFAULT;
							}
							else
							{
								aTextEffectStyle2D = TEXTEFFECTSTYLE2D_RELIEF_EMBOSSED_DEFAULT;
							}
						}
						else
						{
							if(FONT_RELIEF_ENGRAVED == getFontRelief())
							{
								aTextEffectStyle2D = TEXTEFFECTSTYLE2D_RELIEF_ENGRAVED;
							}
							else
							{
								aTextEffectStyle2D = TEXTEFFECTSTYLE2D_RELIEF_EMBOSSED;
							}
						}

						Primitive2DReference aNewTextEffect(new TextEffectPrimitive2D(
							aRetval,
							aDecTrans.getTranslate(),
							aDecTrans.getRotate(),
                            aTextEffectStyle2D));
                        aRetval = Primitive2DSequence(&aNewTextEffect, 1);
                    }
                    else if(bHasOutline)
                    {
                        // create outline using an own helper primitive since this will
                        // be view-dependent
                        Primitive2DReference aNewTextEffect(new TextEffectPrimitive2D(
							aRetval,
							aDecTrans.getTranslate(),
							aDecTrans.getRotate(),
							TEXTEFFECTSTYLE2D_OUTLINE));
                        aRetval = Primitive2DSequence(&aNewTextEffect, 1);
                    }

                    if(aShadow.is())
                    {
                        // put shadow in front if there is one to paint timely before
                        // but placed behind content
                        const Primitive2DSequence aContent(aRetval);
                        aRetval = Primitive2DSequence(&aShadow, 1);
                        appendPrimitive2DSequenceToPrimitive2DSequence(aRetval, aContent);
                    }
                }
            }

            return aRetval;
        }

        TextDecoratedPortionPrimitive2D::TextDecoratedPortionPrimitive2D(

            // TextSimplePortionPrimitive2D parameters
			const basegfx::B2DHomMatrix& rNewTransform,
			const String& rText,
			xub_StrLen aTextPosition,
			xub_StrLen aTextLength,
			const ::std::vector< double >& rDXArray,
			const FontAttributes& rFontAttributes,
            const ::com::sun::star::lang::Locale& rLocale,
			const basegfx::BColor& rFontColor,

            // local parameters
            const basegfx::BColor& rOverlineColor,
            const basegfx::BColor& rTextlineColor,
            FontUnderline eFontOverline,
            FontUnderline eFontUnderline,
			bool bUnderlineAbove,
			FontStrikeout eFontStrikeout,
			bool bWordLineMode,
			FontEmphasisMark eFontEmphasisMark,
			bool bEmphasisMarkAbove,
			bool bEmphasisMarkBelow,
			FontRelief eFontRelief,
			bool bShadow)
		:	TextSimplePortionPrimitive2D(rNewTransform, rText, aTextPosition, aTextLength, rDXArray, rFontAttributes, rLocale, rFontColor),
            maOverlineColor(rOverlineColor),
            maTextlineColor(rTextlineColor),
            meFontOverline(eFontOverline),
            meFontUnderline(eFontUnderline),
			meFontStrikeout(eFontStrikeout),
			meFontEmphasisMark(eFontEmphasisMark),
			meFontRelief(eFontRelief),
			mbUnderlineAbove(bUnderlineAbove),
			mbWordLineMode(bWordLineMode),
			mbEmphasisMarkAbove(bEmphasisMarkAbove),
			mbEmphasisMarkBelow(bEmphasisMarkBelow),
			mbShadow(bShadow)
		{
		}

		bool TextDecoratedPortionPrimitive2D::operator==(const BasePrimitive2D& rPrimitive) const
		{
			if(TextSimplePortionPrimitive2D::operator==(rPrimitive))
			{
				const TextDecoratedPortionPrimitive2D& rCompare = (TextDecoratedPortionPrimitive2D&)rPrimitive;

                return (getOverlineColor() == rCompare.getOverlineColor()
                    && getTextlineColor() == rCompare.getTextlineColor()
                    && getFontOverline() == rCompare.getFontOverline()
                    && getFontUnderline() == rCompare.getFontUnderline()
					&& getFontStrikeout() == rCompare.getFontStrikeout()
					&& getFontEmphasisMark() == rCompare.getFontEmphasisMark()
					&& getFontRelief() == rCompare.getFontRelief()
					&& getUnderlineAbove() == rCompare.getUnderlineAbove()
					&& getWordLineMode() == rCompare.getWordLineMode()
					&& getEmphasisMarkAbove() == rCompare.getEmphasisMarkAbove()
					&& getEmphasisMarkBelow() == rCompare.getEmphasisMarkBelow()
					&& getShadow() == rCompare.getShadow());
			}

			return false;
		}

        // #i96475#
        // Added missing implementation. Decorations may (will) stick out of the text's
        // inking area, so add them if needed
		basegfx::B2DRange TextDecoratedPortionPrimitive2D::getB2DRange(const geometry::ViewInformation2D& rViewInformation) const
		{
			const bool bDecoratedIsNeeded(
                FONT_UNDERLINE_NONE != getFontOverline()
             || FONT_UNDERLINE_NONE != getFontUnderline()
             || FONT_STRIKEOUT_NONE != getFontStrikeout()
             || FONT_EMPHASISMARK_NONE != getFontEmphasisMark()
             || FONT_RELIEF_NONE != getFontRelief()
             || getShadow());

            if(bDecoratedIsNeeded)
            {
                // decoration is used, fallback to BasePrimitive2D::getB2DRange which uses
                // the own local decomposition for computation and thus creates all necessary
                // geometric objects
                return BasePrimitive2D::getB2DRange(rViewInformation);
            }
            else
            {
                // no relevant decoration used, fallback to TextSimplePortionPrimitive2D::getB2DRange
                return TextSimplePortionPrimitive2D::getB2DRange(rViewInformation);
            }
        }

		// provide unique ID
		ImplPrimitrive2DIDBlock(TextDecoratedPortionPrimitive2D, PRIMITIVE2D_ID_TEXTDECORATEDPORTIONPRIMITIVE2D)

	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////
// eof
