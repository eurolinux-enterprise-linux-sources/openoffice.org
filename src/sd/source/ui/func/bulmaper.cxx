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

#ifdef SD_DLLIMPLEMENTATION
#undef SD_DLLIMPLEMENTATION
#endif

#ifndef _SVX_SVXIDS_HRC
#include <svx/svxids.hrc>
#endif

//-> Fonts & Items
#include <vcl/font.hxx>
#include <svx/fontitem.hxx>
#include <svx/fhgtitem.hxx>
#include <svx/wghtitem.hxx>
#include <svx/udlnitem.hxx>
#include <svx/crsditem.hxx>
#include <svx/postitem.hxx>
#include <svx/cntritem.hxx>
#include <svx/shdditem.hxx>

//<- Fonts & Items
#include <svx/bulitem.hxx>
#include <svx/brshitem.hxx>
#include <vcl/graph.hxx>
#include <svtools/itemset.hxx>
#include <svtools/itempool.hxx>
#include <svx/numitem.hxx>
#include <svx/eeitem.hxx>

#include "bulmaper.hxx"


#define GetWhich(nSlot) rSet.GetPool()->GetWhich( nSlot )

/* #i35937#

void SdBulletMapper::PreMapNumBulletForDialog( SfxItemSet& rSet )
{
	if( SFX_ITEM_SET == rSet.GetItemState( EE_PARA_NUMBULLET, FALSE ) )
	{
		SvxNumRule* pRule = ((SvxNumBulletItem*)rSet.GetItem( EE_PARA_NUMBULLET ))->GetNumRule();

		if(pRule && pRule->GetNumRuleType() == SVX_RULETYPE_PRESENTATION_NUMBERING)
		{
			// 10er Bullet Item auf 9er Item mappen
			SvxNumRule aNewRule( pRule->GetFeatureFlags(), 9, FALSE, SVX_RULETYPE_PRESENTATION_NUMBERING );

			for( USHORT i = 0; i < 9; i++ )
				aNewRule.SetLevel(i, pRule->GetLevel(i));

			rSet.Put( SvxNumBulletItem( aNewRule, EE_PARA_NUMBULLET ) );
		}
	}
}

void SdBulletMapper::PostMapNumBulletForDialog( SfxItemSet& rSet )
{
	if( SFX_ITEM_SET == rSet.GetItemState( EE_PARA_NUMBULLET, FALSE ) )
	{
		SvxNumRule* pRule = ((SvxNumBulletItem*)rSet.GetItem( EE_PARA_NUMBULLET ))->GetNumRule();

		if(pRule)
		{
			pRule->UnLinkGraphics();
			if(pRule->GetNumRuleType() == SVX_RULETYPE_PRESENTATION_NUMBERING)
			{
				// 9er Bullet Item auf 10er Item mappen
				SvxNumRule aNewRule( pRule->GetFeatureFlags(), 10, FALSE, SVX_RULETYPE_PRESENTATION_NUMBERING );

				for( USHORT i = 0; i < 9; i++ )
					aNewRule.SetLevel(i, pRule->GetLevel(i));

				rSet.Put( SvxNumBulletItem( aNewRule, EE_PARA_NUMBULLET ) );
			}
		}
	}
}
*/

void SdBulletMapper::MapFontsInNumRule( SvxNumRule& aNumRule, const SfxItemSet& rSet )
{
	const USHORT nCount = aNumRule.GetLevelCount();
	for( USHORT nLevel = 0; nLevel < nCount; nLevel++ )
	{
		const SvxNumberFormat& rSrcLevel = aNumRule.GetLevel(nLevel);
		SvxNumberFormat aNewLevel( rSrcLevel );

		if(rSrcLevel.GetNumberingType() != com::sun::star::style::NumberingType::CHAR_SPECIAL &&
		   rSrcLevel.GetNumberingType() != com::sun::star::style::NumberingType::NUMBER_NONE )
		{
			// wenn Aufzaehlung statt Bullet gewaehlt wurde, wird der Bullet-Font
			// dem Vorlagen-Font angeglichen

			// to be implemented if module supports CJK
            long nFontID = SID_ATTR_CHAR_FONT;
            long nFontHeightID = SID_ATTR_CHAR_FONTHEIGHT;
            long nWeightID = SID_ATTR_CHAR_WEIGHT;
            long nPostureID = SID_ATTR_CHAR_POSTURE;

			if( 0 )
			{
				nFontID = EE_CHAR_FONTINFO_CJK;
			 	nFontHeightID = EE_CHAR_FONTHEIGHT_CJK;
				nWeightID = EE_CHAR_WEIGHT_CJK;
				nPostureID = EE_CHAR_ITALIC_CJK;
			}
			else if( 0 )
			{
				nFontID = EE_CHAR_FONTINFO_CTL;
			 	nFontHeightID = EE_CHAR_FONTHEIGHT_CTL;
				nWeightID = EE_CHAR_WEIGHT_CTL;
				nPostureID = EE_CHAR_ITALIC_CTL;
			}

			Font aMyFont;
			const SvxFontItem& rFItem =
				(SvxFontItem&)rSet.Get(GetWhich( (USHORT)nFontID ));
			aMyFont.SetFamily(rFItem.GetFamily());
			aMyFont.SetName(rFItem.GetFamilyName());
			aMyFont.SetCharSet(rFItem.GetCharSet());
			aMyFont.SetPitch(rFItem.GetPitch());

			const SvxFontHeightItem& rFHItem =
				(SvxFontHeightItem&)rSet.Get(GetWhich( (USHORT)nFontHeightID ));
			aMyFont.SetSize(Size(0, rFHItem.GetHeight()));

			const SvxWeightItem& rWItem =
				(SvxWeightItem&)rSet.Get(GetWhich( (USHORT)nWeightID ));
			aMyFont.SetWeight(rWItem.GetWeight());

			const SvxPostureItem& rPItem =
				(SvxPostureItem&)rSet.Get(GetWhich( (USHORT)nPostureID ));
			aMyFont.SetItalic(rPItem.GetPosture());

			const SvxUnderlineItem& rUItem = (SvxUnderlineItem&)rSet.Get(GetWhich(SID_ATTR_CHAR_UNDERLINE));
			aMyFont.SetUnderline(rUItem.GetLineStyle());

			const SvxOverlineItem& rOItem = (SvxOverlineItem&)rSet.Get(GetWhich(SID_ATTR_CHAR_OVERLINE));
			aMyFont.SetOverline(rOItem.GetLineStyle());

			const SvxCrossedOutItem& rCOItem = (SvxCrossedOutItem&)rSet.Get(GetWhich(SID_ATTR_CHAR_STRIKEOUT));
			aMyFont.SetStrikeout(rCOItem.GetStrikeout());

			const SvxContourItem& rCItem = (SvxContourItem&)rSet.Get(GetWhich(SID_ATTR_CHAR_CONTOUR));
			aMyFont.SetOutline(rCItem.GetValue());

			const SvxShadowedItem& rSItem = (SvxShadowedItem&)rSet.Get(GetWhich(SID_ATTR_CHAR_SHADOWED));
			aMyFont.SetShadow(rSItem.GetValue());

			aNewLevel.SetBulletFont(&aMyFont);
//			aNewLevel.SetBulletRelSize( 75 );
			aNumRule.SetLevel(nLevel, aNewLevel );
		}
		else if( rSrcLevel.GetNumberingType() == com::sun::star::style::NumberingType::CHAR_SPECIAL )
		{
			String aEmpty;
			aNewLevel.SetPrefix( aEmpty );
			aNewLevel.SetSuffix( aEmpty );
			aNumRule.SetLevel(nLevel, aNewLevel );
		}
	}
}
