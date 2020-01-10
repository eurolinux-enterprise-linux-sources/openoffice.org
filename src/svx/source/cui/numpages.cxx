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

#ifdef SVX_DLLIMPLEMENTATION
#undef SVX_DLLIMPLEMENTATION
#endif

#include <com/sun/star/text/HoriOrientation.hpp>
#include <com/sun/star/text/VertOrientation.hpp>
#include <com/sun/star/text/RelOrientation.hpp>

#include <numpages.hxx>
#include <numpages.hrc>
#include <svx/dialmgr.hxx>
#ifndef _SVX_DIALOGS_HRC
#include <svx/dialogs.hrc>
#endif
#include <tools/shl.hxx>
#include <i18npool/mslangid.hxx>
#include <svtools/valueset.hxx>
#ifndef _SVX_HELPID_HRC
#include <helpid.hrc>
#endif
#include <svx/numitem.hxx>
#include <svtools/eitem.hxx>
#include <vcl/svapp.hxx>
#include <gallery.hxx>
#include <svtools/urihelper.hxx>
#include <svx/brshitem.hxx>
#include <svtools/intitem.hxx>
#include <sfx2/objsh.hxx>
#include <vcl/graph.hxx>
#include <vcl/msgbox.hxx>
#include "cuicharmap.hxx"
#include <flstitem.hxx>
#include <dlgutil.hxx>
#include <svx/xtable.hxx>
#include <drawitem.hxx>
#include <numvset.hxx>
#include <htmlmode.hxx>
#include <svtools/pathoptions.hxx>
#include <svtools/ctrltool.hxx>
#include <unolingu.hxx>
#include <com/sun/star/style/NumberingType.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/container/XIndexAccess.hpp>
#include <com/sun/star/text/XDefaultNumberingProvider.hpp>
#include <com/sun/star/text/XNumberingFormatter.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <comphelper/processfactory.hxx>
#include <com/sun/star/text/XNumberingTypeInfo.hpp>

#include <algorithm>
#include <vector>
#include "opengrf.hxx"

#include <svx/dialogs.hrc> //CHINA001
#include <sfx2/request.hxx> //CHINA001
#include <svtools/aeitem.hxx> //add CHINA001
#include <svtools/stritem.hxx>//add CHINA001
#include <svtools/slstitm.hxx> //add CHINA001

using namespace com::sun::star;
using namespace com::sun::star::uno;
using namespace com::sun::star::beans;
using namespace com::sun::star::lang;
using namespace com::sun::star::i18n;
using namespace com::sun::star::text;
using namespace com::sun::star::container;
using namespace com::sun::star::style;
using rtl::OUString;
#define C2U(cChar) OUString::createFromAscii(cChar)

SV_IMPL_PTRARR(SvxNumSettingsArr_Impl,SvxNumSettings_ImplPtr);

/*-----------------07.02.97 15.37-------------------

--------------------------------------------------*/
#define NUM_PAGETYPE_BULLET			0
#define NUM_PAGETYPE_SINGLENUM      1
#define NUM_PAGETYPE_NUM            2
#define NUM_PAGETYPE_BMP            3
#define PAGETYPE_USER_START         10

#define SHOW_NUMBERING				0
#define SHOW_BULLET					1
#define SHOW_BITMAP					2

#define MAX_BMP_WIDTH				16
#define MAX_BMP_HEIGHT				16

static BOOL bLastRelative =			FALSE;
static const sal_Char cNumberingType[] = "NumberingType";
static const sal_Char cValue[] = "Value";
static const sal_Char cParentNumbering[] = "ParentNumbering";
static const sal_Char cPrefix[] = "Prefix";
static const sal_Char cSuffix[] = "Suffix";
static const sal_Char cBulletChar[] = "BulletChar";
static const sal_Char cBulletFontName[] = "BulletFontName";
/* -----------------------------31.01.01 10:23--------------------------------

 ---------------------------------------------------------------------------*/
Reference<XDefaultNumberingProvider> lcl_GetNumberingProvider()
{
	Reference< XMultiServiceFactory > xMSF = ::comphelper::getProcessServiceFactory();
	Reference < XInterface > xI = xMSF->createInstance(
		::rtl::OUString::createFromAscii( "com.sun.star.text.DefaultNumberingProvider" ) );
	Reference<XDefaultNumberingProvider> xRet(xI, UNO_QUERY);
	DBG_ASSERT(xRet.is(), "service missing: \"com.sun.star.text.DefaultNumberingProvider\"");

	return xRet;
}
/* -----------------------------31.01.01 11:40--------------------------------

 ---------------------------------------------------------------------------*/
SvxNumSettings_ImplPtr lcl_CreateNumSettingsPtr(const Sequence<PropertyValue>& rLevelProps)
{
	const PropertyValue* pValues = rLevelProps.getConstArray();
	SvxNumSettings_ImplPtr pNew = new SvxNumSettings_Impl;
	for(sal_Int32 j = 0; j < rLevelProps.getLength(); j++)
	{
		if(pValues[j].Name.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM(cNumberingType)))
			pValues[j].Value >>= pNew->nNumberType;
		else if(pValues[j].Name.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM(cPrefix)))
			pValues[j].Value >>= pNew->sPrefix;
		else if(pValues[j].Name.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM(cSuffix)))
			pValues[j].Value >>= pNew->sSuffix;
		else if(pValues[j].Name.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM(cParentNumbering)))
			pValues[j].Value >>= pNew->nParentNumbering;
		else if(pValues[j].Name.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM(cBulletChar)))
			pValues[j].Value >>= pNew->sBulletChar;
		else if(pValues[j].Name.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM(cBulletFontName)))
			pValues[j].Value >>= pNew->sBulletFont;
	}
	return pNew;
}
/* -----------------28.10.98 08:32-------------------
 *
 * --------------------------------------------------*/
// Die Auswahl an Bullets aus den StarSymbol
static const sal_Unicode aBulletTypes[] =
{
	0x2022,
	0x25cf,
	0xe00c,
	0xe00a,
	0x2794,
	0x27a2,
	0x2717,
	0x2714
};
/* -----------------28.10.98 09:42-------------------
 *
 * --------------------------------------------------*/
static sal_Char __READONLY_DATA aNumChar[] =
{
	'A', //CHARS_UPPER_LETTER
	'a', //CHARS_LOWER_LETTER
	'I', //ROMAN_UPPER
	'i', //ROMAN_LOWER
	'1', //ARABIC
	' '
};

/*-----------------18.03.98 08:35-------------------
	Ist eins der maskierten Formate gesetzt?
--------------------------------------------------*/
BOOL lcl_IsNumFmtSet(SvxNumRule* pNum, USHORT nLevelMask)
{
	BOOL bRet = FALSE;
	USHORT nMask = 1;
	for( USHORT i = 0; i < SVX_MAX_NUM && !bRet; i++ )
	{
		if(nLevelMask & nMask)
			bRet |= 0 != pNum->Get( i );
		nMask <<= 1 ;
	}
	return bRet;
}
/* -----------------28.10.98 08:50-------------------
 *
 * --------------------------------------------------*/

Font& lcl_GetDefaultBulletFont()
{
	static BOOL bInit = 0;
	static Font aDefBulletFont( UniString::CreateFromAscii(
		                        RTL_CONSTASCII_STRINGPARAM( "StarSymbol" ) ),
								String(), Size( 0, 14 ) );
	if(!bInit)
	{
        aDefBulletFont.SetCharSet( RTL_TEXTENCODING_SYMBOL );
		aDefBulletFont.SetFamily( FAMILY_DONTKNOW );
		aDefBulletFont.SetPitch( PITCH_DONTKNOW );
		aDefBulletFont.SetWeight( WEIGHT_DONTKNOW );
		aDefBulletFont.SetTransparent( TRUE );
		bInit = TRUE;
	}
	return aDefBulletFont;
}


/**************************************************************************/
/*                                                                        */
/*                                                                        */
/**************************************************************************/


SvxSingleNumPickTabPage::SvxSingleNumPickTabPage(Window* pParent,
							   const SfxItemSet& rSet)	:
	SfxTabPage( pParent, SVX_RES( RID_SVXPAGE_PICK_SINGLE_NUM ), rSet ),
    aValuesFL(      this, SVX_RES(FL_VALUES) ),
	pExamplesVS(	new SvxNumValueSet(this, SVX_RES(VS_VALUES), NUM_PAGETYPE_SINGLENUM )),
	pActNum(0),
	pSaveNum(0),
	nActNumLvl( USHRT_MAX ),
	bModified(FALSE),
	bPreset(FALSE),
	nNumItemId(SID_ATTR_NUMBERING_RULE)
{
	FreeResource();
	SetExchangeSupport();
	pExamplesVS->SetSelectHdl(LINK(this, SvxSingleNumPickTabPage, NumSelectHdl_Impl));
	pExamplesVS->SetDoubleClickHdl(LINK(this, SvxSingleNumPickTabPage, DoubleClickHdl_Impl));
	pExamplesVS->SetHelpId(HID_VALUESET_SINGLENUM );

	Reference<XDefaultNumberingProvider> xDefNum = lcl_GetNumberingProvider();
	if(xDefNum.is())
	{
		Sequence< Sequence< PropertyValue > > aNumberings;
        LanguageType eLang = Application::GetSettings().GetLanguage();
		Locale aLocale = SvxCreateLocale(eLang);
		try
		{
			aNumberings =
				xDefNum->getDefaultContinuousNumberingLevels( aLocale );


            sal_Int32 nLength = aNumberings.getLength() > NUM_VALUSET_COUNT ? NUM_VALUSET_COUNT :aNumberings.getLength();

			const Sequence<PropertyValue>* pValuesArr = aNumberings.getConstArray();
			for(sal_Int32 i = 0; i < nLength; i++)
			{
				SvxNumSettings_ImplPtr pNew = lcl_CreateNumSettingsPtr(pValuesArr[i]);
				aNumSettingsArr.Insert(pNew, aNumSettingsArr.Count());
			}
		}
		catch(Exception&)
		{
		}
		Reference<XNumberingFormatter> xFormat(xDefNum, UNO_QUERY);
		pExamplesVS->SetNumberingSettings(aNumberings, xFormat, aLocale);
	}
}
/*-----------------07.02.97 12.08-------------------

--------------------------------------------------*/

 SvxSingleNumPickTabPage::~SvxSingleNumPickTabPage()
{
	delete pActNum;
	delete pExamplesVS;
	delete pSaveNum;
	aNumSettingsArr.DeleteAndDestroy(0, aNumSettingsArr.Count());
}

/*-----------------07.02.97 12.13-------------------

--------------------------------------------------*/

SfxTabPage*	 SvxSingleNumPickTabPage::Create( Window* pParent,
								const SfxItemSet& rAttrSet)
{
	return new SvxSingleNumPickTabPage(pParent, rAttrSet);
}

/*-----------------07.02.97 12.09-------------------

--------------------------------------------------*/


BOOL  SvxSingleNumPickTabPage::FillItemSet( SfxItemSet& rSet )
{
	if( (bPreset || bModified) && pSaveNum)
	{
		*pSaveNum = *pActNum;
		rSet.Put(SvxNumBulletItem( *pSaveNum ), nNumItemId);
		rSet.Put(SfxBoolItem(SID_PARAM_NUM_PRESET, bPreset));
	}

	return bModified;
}

/*-----------------08.02.97 16.27-------------------

--------------------------------------------------*/

void  SvxSingleNumPickTabPage::ActivatePage(const SfxItemSet& rSet)
{
	const SfxPoolItem* pItem;
	bPreset = FALSE;
	BOOL bIsPreset = FALSE;
	const SfxItemSet* pExampleSet = GetTabDialog()->GetExampleSet();
	if(pExampleSet)
	{
		if(SFX_ITEM_SET == pExampleSet->GetItemState(SID_PARAM_NUM_PRESET, FALSE, &pItem))
			bIsPreset = ((const SfxBoolItem*)pItem)->GetValue();
		if(SFX_ITEM_SET == pExampleSet->GetItemState(SID_PARAM_CUR_NUM_LEVEL, FALSE, &pItem))
			nActNumLvl = ((const SfxUInt16Item*)pItem)->GetValue();
	}
	if(SFX_ITEM_SET == rSet.GetItemState(nNumItemId, FALSE, &pItem))
	{
		delete pSaveNum;
		pSaveNum = new SvxNumRule(*((SvxNumBulletItem*)pItem)->GetNumRule());
	}
	if(*pSaveNum != *pActNum)
	{
		*pActNum = *pSaveNum;
		pExamplesVS->SetNoSelection();
	}
	// ersten Eintrag vorselektieren
	if(pActNum && (!lcl_IsNumFmtSet(pActNum, nActNumLvl) || bIsPreset))
	{
		pExamplesVS->SelectItem(1);
		NumSelectHdl_Impl(pExamplesVS);
		bPreset = TRUE;
	}
	bPreset |= bIsPreset;

	bModified = FALSE;
}

/*-----------------08.02.97 11.28-------------------

--------------------------------------------------*/

int  SvxSingleNumPickTabPage::DeactivatePage(SfxItemSet *_pSet)
{
	if(_pSet)
		FillItemSet(*_pSet);
	return TRUE;
}

/*-----------------07.02.97 12.09-------------------

--------------------------------------------------*/


void  SvxSingleNumPickTabPage::Reset( const SfxItemSet& rSet )
{
	const SfxPoolItem* pItem;
//	nActNumLvl = ((SwNumBulletTabDialog*)GetTabDialog())->GetActNumLevel();
	//im Draw gibt es das Item als WhichId, im Writer nur als SlotId
	SfxItemState eState = rSet.GetItemState(SID_ATTR_NUMBERING_RULE, FALSE, &pItem);
	if(eState != SFX_ITEM_SET)
	{
		nNumItemId = rSet.GetPool()->GetWhich(SID_ATTR_NUMBERING_RULE);
		eState = rSet.GetItemState(nNumItemId, FALSE, &pItem);
		
		if( eState != SFX_ITEM_SET )
		{
	        pItem = &static_cast< const SvxNumBulletItem& >( rSet.Get( nNumItemId, TRUE ) );		
	        eState = SFX_ITEM_SET;
		}
	}
	DBG_ASSERT(eState == SFX_ITEM_SET, "kein Item gefunden!");
	delete pSaveNum;
	pSaveNum = new SvxNumRule(*((SvxNumBulletItem*)pItem)->GetNumRule());

	if(!pActNum)
		pActNum = new  SvxNumRule(*pSaveNum);
	else if(*pSaveNum != *pActNum)
		*pActNum = *pSaveNum;
}
/*-----------------08.02.97 11.40-------------------

--------------------------------------------------*/

IMPL_LINK(SvxSingleNumPickTabPage, NumSelectHdl_Impl, ValueSet*, EMPTYARG)
{
	if(pActNum)
	{
		bPreset = FALSE;
		bModified = TRUE;
		USHORT nIdx = pExamplesVS->GetSelectItemId() - 1;
		DBG_ASSERT(aNumSettingsArr.Count() > nIdx, "wrong index");
		if(aNumSettingsArr.Count() <= nIdx)
			return 0;
		SvxNumSettings_ImplPtr _pSet = aNumSettingsArr.GetObject(nIdx);
		sal_Int16 eNewType = _pSet->nNumberType;
        const sal_Unicode cLocalPrefix = _pSet->sPrefix.getLength() ? _pSet->sPrefix.getStr()[0] : 0;
        const sal_Unicode cLocalSuffix = _pSet->sSuffix.getLength() ? _pSet->sSuffix.getStr()[0] : 0;

		USHORT nMask = 1;
		for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
		{
			if(nActNumLvl & nMask)
			{
				SvxNumberFormat aFmt(pActNum->GetLevel(i));
				aFmt.SetNumberingType(eNewType);
				String aEmptyStr;
				if(cLocalPrefix == ' ')
					aFmt.SetPrefix( aEmptyStr );
				else
                    aFmt.SetPrefix(_pSet->sPrefix);
				if(cLocalSuffix == ' ')
					aFmt.SetSuffix( aEmptyStr );
				else
                    aFmt.SetSuffix(_pSet->sSuffix);
				aFmt.SetCharFmtName(sNumCharFmtName);
                // #62069# // #92724#
                aFmt.SetBulletRelSize(100);
                pActNum->SetLevel(i, aFmt);
			}
			nMask <<= 1 ;
		}
	}
	return 0;
}

/*-----------------06.06.97 11.15-------------------

--------------------------------------------------*/
IMPL_LINK(SvxSingleNumPickTabPage, DoubleClickHdl_Impl, ValueSet*, EMPTYARG)
{
	NumSelectHdl_Impl(pExamplesVS);
	OKButton& rOk = GetTabDialog()->GetOKButton();
	rOk.GetClickHdl().Call(&rOk);
	return 0;
}

/**************************************************************************/
/*                                                                        */
/*                                                                        */
/**************************************************************************/


SvxBulletPickTabPage::SvxBulletPickTabPage(Window* pParent,
							   const SfxItemSet& rSet)	:
	SfxTabPage( pParent, SVX_RES( RID_SVXPAGE_PICK_BULLET ), rSet ),
    aValuesFL(      this, SVX_RES(FL_VALUES) ),
	pExamplesVS(	new SvxNumValueSet(this, SVX_RES(VS_VALUES), NUM_PAGETYPE_BULLET )),
	pActNum(0),
	pSaveNum(0),
	nActNumLvl( USHRT_MAX ),
	bModified(FALSE),
	bPreset(FALSE),
	nNumItemId(SID_ATTR_NUMBERING_RULE)
{
	FreeResource();
	SetExchangeSupport();
	pExamplesVS->SetSelectHdl(LINK(this, SvxBulletPickTabPage, NumSelectHdl_Impl));
	pExamplesVS->SetDoubleClickHdl(LINK(this, SvxBulletPickTabPage, DoubleClickHdl_Impl));
	pExamplesVS->SetHelpId(HID_VALUESET_BULLET    );

}
/*-----------------07.02.97 12.10-------------------

--------------------------------------------------*/


 SvxBulletPickTabPage::~SvxBulletPickTabPage()
{
	delete pActNum;
	delete pExamplesVS;
	delete pSaveNum;
}
/*-----------------07.02.97 12.10-------------------

--------------------------------------------------*/


SfxTabPage*	 SvxBulletPickTabPage::Create( Window* pParent,
								const SfxItemSet& rAttrSet)
{
	return new SvxBulletPickTabPage(pParent, rAttrSet);
}

/*-----------------07.02.97 12.10-------------------

--------------------------------------------------*/


BOOL  SvxBulletPickTabPage::FillItemSet( SfxItemSet& rSet )
{
	if( (bPreset || bModified) && pActNum)
	{
		*pSaveNum = *pActNum;
		rSet.Put(SvxNumBulletItem( *pSaveNum ), nNumItemId);
		rSet.Put(SfxBoolItem(SID_PARAM_NUM_PRESET, bPreset));
	}
	return bModified;
}
/*-----------------08.02.97 16.28-------------------

--------------------------------------------------*/

void  SvxBulletPickTabPage::ActivatePage(const SfxItemSet& rSet)
{
	const SfxPoolItem* pItem;
	bPreset = FALSE;
	BOOL bIsPreset = FALSE;
	const SfxItemSet* pExampleSet = GetTabDialog()->GetExampleSet();
	if(pExampleSet)
	{
		if(SFX_ITEM_SET == pExampleSet->GetItemState(SID_PARAM_NUM_PRESET, FALSE, &pItem))
			bIsPreset = ((const SfxBoolItem*)pItem)->GetValue();
		if(SFX_ITEM_SET == pExampleSet->GetItemState(SID_PARAM_CUR_NUM_LEVEL, FALSE, &pItem))
			nActNumLvl = ((const SfxUInt16Item*)pItem)->GetValue();
	}
	if(SFX_ITEM_SET == rSet.GetItemState(nNumItemId, FALSE, &pItem))
	{
		delete pSaveNum;
		pSaveNum = new SvxNumRule(*((SvxNumBulletItem*)pItem)->GetNumRule());
	}
	if(*pSaveNum != *pActNum)
	{
		*pActNum = *pSaveNum;
		pExamplesVS->SetNoSelection();
	}
	// ersten Eintrag vorselektieren
	if(pActNum && (!lcl_IsNumFmtSet(pActNum, nActNumLvl) || bIsPreset))
	{
		pExamplesVS->SelectItem(1);
		NumSelectHdl_Impl(pExamplesVS);
		bPreset = TRUE;
	}
	bPreset |= bIsPreset;
	bModified = FALSE;
}
/*-----------------08.02.97 11.28-------------------

--------------------------------------------------*/

int  SvxBulletPickTabPage::DeactivatePage(SfxItemSet *_pSet)
{
	if(_pSet)
		FillItemSet(*_pSet);
	return TRUE;
}

/*-----------------07.02.97 12.11-------------------

--------------------------------------------------*/


void  SvxBulletPickTabPage::Reset( const SfxItemSet& rSet )
{
	const SfxPoolItem* pItem;
	//im Draw gibt es das Item als WhichId, im Writer nur als SlotId
	SfxItemState eState = rSet.GetItemState(SID_ATTR_NUMBERING_RULE, FALSE, &pItem);
	if(eState != SFX_ITEM_SET)
	{
		nNumItemId = rSet.GetPool()->GetWhich(SID_ATTR_NUMBERING_RULE);
		eState = rSet.GetItemState(nNumItemId, FALSE, &pItem);
		
		if( eState != SFX_ITEM_SET )
		{
	        pItem = &static_cast< const SvxNumBulletItem& >( rSet.Get( nNumItemId, TRUE ) );		
	        eState = SFX_ITEM_SET;
		}
		
	}
	DBG_ASSERT(eState == SFX_ITEM_SET, "kein Item gefunden!");
	delete pSaveNum;
	pSaveNum = new SvxNumRule(*((SvxNumBulletItem*)pItem)->GetNumRule());

//	nActNumLvl = ((SwNumBulletTabDialog*)GetTabDialog())->GetActNumLevel();

	if(!pActNum)
		pActNum = new  SvxNumRule(*pSaveNum);
	else if(*pSaveNum != *pActNum)
		*pActNum = *pSaveNum;
}
/*-----------------08.02.97 11.58-------------------

--------------------------------------------------*/

IMPL_LINK(SvxBulletPickTabPage, NumSelectHdl_Impl, ValueSet*, EMPTYARG)
{
	if(pActNum)
	{
		bPreset = FALSE;
		bModified = TRUE;
		sal_Unicode cChar = aBulletTypes[pExamplesVS->GetSelectItemId() - 1];
		Font& rActBulletFont = lcl_GetDefaultBulletFont();

		USHORT nMask = 1;
		for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
		{
			if(nActNumLvl & nMask)
			{
				SvxNumberFormat aFmt(pActNum->GetLevel(i));
				aFmt.SetNumberingType( SVX_NUM_CHAR_SPECIAL );
				aFmt.SetBulletFont(&rActBulletFont);
				aFmt.SetBulletChar(cChar );
				aFmt.SetCharFmtName(sBulletCharFmtName);
                // #62069# // #92724#
                aFmt.SetBulletRelSize(45);
				pActNum->SetLevel(i, aFmt);
			}
			nMask <<= 1;
		}
	}

	return 0;
}

/*-----------------06.06.97 11.16-------------------

--------------------------------------------------*/
IMPL_LINK(SvxBulletPickTabPage, DoubleClickHdl_Impl, ValueSet*, EMPTYARG)
{
	NumSelectHdl_Impl(pExamplesVS);
	OKButton& rOk = GetTabDialog()->GetOKButton();
	rOk.GetClickHdl().Call(&rOk);
	return 0;
}

//Add CHINA001
void SvxBulletPickTabPage::PageCreated(SfxAllItemSet aSet)
{

	SFX_ITEMSET_ARG	(&aSet,pBulletCharFmt,SfxStringItem,SID_BULLET_CHAR_FMT,sal_False);

	if (pBulletCharFmt)
		SetCharFmtName( pBulletCharFmt->GetValue());


}
//end of add CHINA001
/**************************************************************************/
/*                                                                        */
/*                                                                        */
/**************************************************************************/


SvxNumPickTabPage::SvxNumPickTabPage(Window* pParent,
							   const SfxItemSet& rSet) :
	SfxTabPage( pParent, SVX_RES( RID_SVXPAGE_PICK_NUM ), rSet ),
    aValuesFL(      this, SVX_RES(FL_VALUES) ),
	pExamplesVS(	new SvxNumValueSet(this, SVX_RES(VS_VALUES), NUM_PAGETYPE_NUM )),
	pActNum(0),
	pSaveNum(0),
	nActNumLvl( USHRT_MAX ),
	nNumItemId(SID_ATTR_NUMBERING_RULE),
	bModified(FALSE),
	bPreset(FALSE)
{

	FreeResource();

	SetExchangeSupport();

	pExamplesVS->SetSelectHdl(LINK(this, SvxNumPickTabPage, NumSelectHdl_Impl));
	pExamplesVS->SetDoubleClickHdl(LINK(this, SvxNumPickTabPage, DoubleClickHdl_Impl));
	pExamplesVS->SetHelpId(HID_VALUESET_NUM       );

	Reference<XDefaultNumberingProvider> xDefNum = lcl_GetNumberingProvider();
	if(xDefNum.is())
	{
		Sequence<Reference<XIndexAccess> > aOutlineAccess;
        LanguageType eLang = Application::GetSettings().GetLanguage();
		Locale aLocale = SvxCreateLocale(eLang);
		try
		{
			aOutlineAccess = xDefNum->getDefaultOutlineNumberings( aLocale );

			for(sal_Int32 nItem = 0;
				nItem < aOutlineAccess.getLength() && nItem < NUM_VALUSET_COUNT;
				nItem++ )
			{
				SvxNumSettingsArr_Impl& rItemArr = aNumSettingsArrays[ nItem ];

				Reference<XIndexAccess> xLevel = aOutlineAccess.getConstArray()[nItem];
				for(sal_Int32 nLevel = 0; nLevel < xLevel->getCount() && nLevel < 5; nLevel++)
				{
					Any aValueAny = xLevel->getByIndex(nLevel);
					Sequence<PropertyValue> aLevelProps;
					aValueAny >>= aLevelProps;
					SvxNumSettings_ImplPtr pNew = lcl_CreateNumSettingsPtr(aLevelProps);
					rItemArr.Insert( pNew, rItemArr.Count() );
				}
			}
		}
		catch(Exception&)
		{
		}
		Reference<XNumberingFormatter> xFormat(xDefNum, UNO_QUERY);
		pExamplesVS->SetOutlineNumberingSettings(aOutlineAccess, xFormat, aLocale);
	}
}
/*-----------------07.02.97 12.12-------------------

--------------------------------------------------*/


 SvxNumPickTabPage::~SvxNumPickTabPage()
{
	delete pActNum;
	delete pExamplesVS;
	delete pSaveNum;
}

/*-----------------07.02.97 12.12-------------------

--------------------------------------------------*/


SfxTabPage*	 SvxNumPickTabPage::Create( Window* pParent,
								const SfxItemSet& rAttrSet)
{
	return new SvxNumPickTabPage(pParent, rAttrSet);
}

/*-----------------07.02.97 12.12-------------------

--------------------------------------------------*/


BOOL  SvxNumPickTabPage::FillItemSet( SfxItemSet& rSet )
{
	if( (bPreset || bModified) && pActNum)
	{
		*pSaveNum = *pActNum;
		rSet.Put(SvxNumBulletItem( *pSaveNum ), nNumItemId);
		rSet.Put(SfxBoolItem(SID_PARAM_NUM_PRESET, bPreset));
	}
	return bModified;
}
/*-----------------08.02.97 16.28-------------------

--------------------------------------------------*/

void  SvxNumPickTabPage::ActivatePage(const SfxItemSet& rSet)
{
	const SfxPoolItem* pItem;
	bPreset = FALSE;
	BOOL bIsPreset = FALSE;
	const SfxItemSet* pExampleSet = GetTabDialog()->GetExampleSet();
	if(pExampleSet)
	{
		if(SFX_ITEM_SET == pExampleSet->GetItemState(SID_PARAM_NUM_PRESET, FALSE, &pItem))
			bIsPreset = ((const SfxBoolItem*)pItem)->GetValue();
		if(SFX_ITEM_SET == pExampleSet->GetItemState(SID_PARAM_CUR_NUM_LEVEL, FALSE, &pItem))
			nActNumLvl = ((const SfxUInt16Item*)pItem)->GetValue();
	}
	if(SFX_ITEM_SET == rSet.GetItemState(nNumItemId, FALSE, &pItem))
	{
		delete pSaveNum;
		pSaveNum = new SvxNumRule(*((SvxNumBulletItem*)pItem)->GetNumRule());
	}
	if(*pSaveNum != *pActNum)
	{
		*pActNum = *pSaveNum;
		pExamplesVS->SetNoSelection();
	}
	// ersten Eintrag vorselektieren
	if(pActNum && (!lcl_IsNumFmtSet(pActNum, nActNumLvl) || bIsPreset))
	{
		pExamplesVS->SelectItem(1);
		NumSelectHdl_Impl(pExamplesVS);
		bPreset = TRUE;
	}
	bPreset |= bIsPreset;
	bModified = FALSE;
}

/* -----------------08.02.97 11.29-------------------

--------------------------------------------------*/

int  SvxNumPickTabPage::DeactivatePage(SfxItemSet *_pSet)
{
	if(_pSet)
		FillItemSet(*_pSet);
	return TRUE;
}

/*-----------------07.02.97 12.12-------------------

--------------------------------------------------*/

void  SvxNumPickTabPage::Reset( const SfxItemSet& rSet )
{
	const SfxPoolItem* pItem;
	//im Draw gibt es das Item als WhichId, im Writer nur als SlotId
	SfxItemState eState = rSet.GetItemState(SID_ATTR_NUMBERING_RULE, FALSE, &pItem);
	if(eState != SFX_ITEM_SET)
	{
		nNumItemId = rSet.GetPool()->GetWhich(SID_ATTR_NUMBERING_RULE);
		eState = rSet.GetItemState(nNumItemId, FALSE, &pItem);
		
		if( eState != SFX_ITEM_SET )
		{
	        pItem = &static_cast< const SvxNumBulletItem& >( rSet.Get( nNumItemId, TRUE ) );		
	        eState = SFX_ITEM_SET;
		}
		
	}
	DBG_ASSERT(eState == SFX_ITEM_SET, "kein Item gefunden!");
	delete pSaveNum;
	pSaveNum = new SvxNumRule(*((SvxNumBulletItem*)pItem)->GetNumRule());

//	nActNumLvl = ((SwNumBulletTabDialog*)GetTabDialog())->GetActNumLevel();
	if(!pActNum)
		pActNum = new  SvxNumRule(*pSaveNum);
	else if(*pSaveNum != *pActNum)
		*pActNum = *pSaveNum;

}

/*-----------------08.02.97 11.58-------------------
	Hier werden alle Ebenen veraendert,
--------------------------------------------------*/

IMPL_LINK(SvxNumPickTabPage, NumSelectHdl_Impl, ValueSet*, EMPTYARG)
{
	if(pActNum)
	{
		bPreset = FALSE;
		bModified = TRUE;

        const FontList*  pList = 0;

        SvxNumSettingsArr_Impl& rItemArr = aNumSettingsArrays[pExamplesVS->GetSelectItemId() - 1];

		Font& rActBulletFont = lcl_GetDefaultBulletFont();
		SvxNumSettings_ImplPtr pLevelSettings = 0;
		for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
		{
			if(rItemArr.Count() > i)
				pLevelSettings = rItemArr[i];
			if(!pLevelSettings)
				break;
			SvxNumberFormat aFmt(pActNum->GetLevel(i));
			aFmt.SetNumberingType( pLevelSettings->nNumberType );
			USHORT nUpperLevelOrChar = (USHORT)pLevelSettings->nParentNumbering;
			if(aFmt.GetNumberingType() == SVX_NUM_CHAR_SPECIAL)
			{
				if( pLevelSettings->sBulletFont.getLength() &&
					pLevelSettings->sBulletFont.compareTo(
							rActBulletFont.GetName()))
				{
                    //search for the font
                    if(!pList)
                    {
                        SfxObjectShell* pCurDocShell = SfxObjectShell::Current();
                        const SvxFontListItem* pFontListItem =
                                (const SvxFontListItem* )pCurDocShell
													->GetItem( SID_ATTR_CHAR_FONTLIST );
                        pList = pFontListItem ? pFontListItem->GetFontList() : 0;
                    }
                    if(pList && pList->IsAvailable( pLevelSettings->sBulletFont ) )
                    {
						FontInfo aInfo = pList->Get(
                            pLevelSettings->sBulletFont,WEIGHT_NORMAL, ITALIC_NONE);
						Font aFont(aInfo);
						aFmt.SetBulletFont(&aFont);
                    }
                    else
                    {
                        //if it cannot be found then create a new one
                        Font aCreateFont( pLevelSettings->sBulletFont,
                                                String(), Size( 0, 14 ) );
                        aCreateFont.SetCharSet( RTL_TEXTENCODING_DONTKNOW );
                        aCreateFont.SetFamily( FAMILY_DONTKNOW );
                        aCreateFont.SetPitch( PITCH_DONTKNOW );
                        aCreateFont.SetWeight( WEIGHT_DONTKNOW );
                        aCreateFont.SetTransparent( TRUE );
                        aFmt.SetBulletFont( &aCreateFont );
                    }
				}
				else
                    aFmt.SetBulletFont( &rActBulletFont );

				aFmt.SetBulletChar( pLevelSettings->sBulletChar.getLength()
										? pLevelSettings->sBulletChar.getStr()[0]
										: 0 );
				aFmt.SetCharFmtName( sBulletCharFmtName );
                // #62069# // #92724#
                aFmt.SetBulletRelSize(45);
            }
			else
			{
				aFmt.SetIncludeUpperLevels(sal::static_int_cast< BYTE >(0 != nUpperLevelOrChar ? pActNum->GetLevelCount() : 0));
				aFmt.SetCharFmtName(sNumCharFmtName);
                // #62069# // #92724#
                aFmt.SetBulletRelSize(100);
            }
			aFmt.SetPrefix(pLevelSettings->sPrefix);
			aFmt.SetSuffix(pLevelSettings->sSuffix);
			pActNum->SetLevel(i, aFmt);
		}
	}
	return 0;
}

/*-----------------06.06.97 11.16-------------------

--------------------------------------------------*/
IMPL_LINK(SvxNumPickTabPage, DoubleClickHdl_Impl, ValueSet*, EMPTYARG)
{
	NumSelectHdl_Impl(pExamplesVS);
	OKButton& rOk = GetTabDialog()->GetOKButton();
	rOk.GetClickHdl().Call(&rOk);
	return 0;
}

//add CHINA001 begin
void SvxNumPickTabPage::PageCreated(SfxAllItemSet aSet)
{
	SFX_ITEMSET_ARG	(&aSet,pNumCharFmt,SfxStringItem,SID_NUM_CHAR_FMT,sal_False);
	SFX_ITEMSET_ARG	(&aSet,pBulletCharFmt,SfxStringItem,SID_BULLET_CHAR_FMT,sal_False);


	if (pNumCharFmt &&pBulletCharFmt)
		SetCharFmtNames( pNumCharFmt->GetValue(),pBulletCharFmt->GetValue());
}
//end of CHINA001
/*-----------------07.02.97 15.59-------------------

--------------------------------------------------*/
void lcl_PaintLevel(OutputDevice* pVDev, sal_Int16 nNumberingType,
                        const OUString& rBulletChar, const OUString& rText, const OUString& rFontName,
                        Point& rLeft, Font& rRuleFont, const Font& rTextFont)
{

    if(NumberingType::CHAR_SPECIAL == nNumberingType )
    {
        rRuleFont.SetStyleName(rFontName);
        pVDev->SetFont(rRuleFont);
        pVDev->DrawText(rLeft, rBulletChar);
        rLeft.X() += pVDev->GetTextWidth(rBulletChar);
    }
    else
    {
        pVDev->SetFont(rTextFont);
        pVDev->DrawText(rLeft, rText);
        rLeft.X() += pVDev->GetTextWidth(rText);
    }
}

/**************************************************************************/
/*                                                                        */
/*                                                                        */
/**************************************************************************/

SvxBitmapPickTabPage::SvxBitmapPickTabPage(Window* pParent,
							   const SfxItemSet& rSet) :
	SfxTabPage( pParent, SVX_RES( RID_SVXPAGE_PICK_BMP ), rSet ),
    aValuesFL(      this, SVX_RES(FL_VALUES) ),
    pExamplesVS(    new SvxBmpNumValueSet(this, SVX_RES(VS_VALUES)/*, aGrfNames*/ )),
	aErrorText(		this, SVX_RES(FT_ERROR)),
	aLinkedCB(		this, SVX_RES(CB_LINKED)),
	pActNum(0),
	pSaveNum(0),
	nActNumLvl( USHRT_MAX ),
	nNumItemId(SID_ATTR_NUMBERING_RULE),
	bModified(FALSE),
	bPreset(FALSE)
{
	FreeResource();
	SetExchangeSupport();
	eCoreUnit = rSet.GetPool()->GetMetric(rSet.GetPool()->GetWhich(SID_ATTR_NUMBERING_RULE));
	pExamplesVS->SetSelectHdl(LINK(this, SvxBitmapPickTabPage, NumSelectHdl_Impl));
	pExamplesVS->SetDoubleClickHdl(LINK(this, SvxBitmapPickTabPage, DoubleClickHdl_Impl));
	aLinkedCB.SetClickHdl(LINK(this, SvxBitmapPickTabPage, LinkBmpHdl_Impl));

	// Grafiknamen ermitteln

	GalleryExplorer::FillObjList(GALLERY_THEME_BULLETS, aGrfNames);
	pExamplesVS->SetHelpId(HID_VALUESET_NUMBMP    );
	for(USHORT i = 0; i < aGrfNames.Count(); i++)
	{
		pExamplesVS->InsertItem( i + 1, i);
		String* pGrfNm = (String*) aGrfNames.GetObject(i);
        INetURLObject aObj(*pGrfNm);
        if(aObj.GetProtocol() == INET_PROT_FILE)
            *pGrfNm = aObj.PathToFileName();
		pExamplesVS->SetItemText( i + 1, *pGrfNm );
	}
	if(!aGrfNames.Count())
	{
		aErrorText.Show();
	}
	else
	{
		pExamplesVS->Show();
		pExamplesVS->Format();
	}

}

/*-----------------12.02.97 07.46-------------------

--------------------------------------------------*/

 SvxBitmapPickTabPage::~SvxBitmapPickTabPage()
{
	String* pStr = (String*)aGrfNames.First();
	while( pStr )
	{
		delete pStr;
		pStr = (String*)aGrfNames.Next();
	}
	delete pExamplesVS;
	delete pActNum;
	delete pSaveNum;
}

/*-----------------12.02.97 07.46-------------------

--------------------------------------------------*/

SfxTabPage*	 SvxBitmapPickTabPage::Create( Window* pParent,
								const SfxItemSet& rAttrSet)
{
	return new SvxBitmapPickTabPage(pParent, rAttrSet);
}

/*-----------------12.02.97 07.46-------------------

--------------------------------------------------*/

void  SvxBitmapPickTabPage::ActivatePage(const SfxItemSet& rSet)
{
	const SfxPoolItem* pItem;
	bPreset = FALSE;
	BOOL bIsPreset = FALSE;
//	nActNumLvl = ((SwNumBulletTabDialog*)GetTabDialog())->GetActNumLevel();
	const SfxItemSet* pExampleSet = GetTabDialog()->GetExampleSet();
	if(pExampleSet)
	{
		if(SFX_ITEM_SET == pExampleSet->GetItemState(SID_PARAM_NUM_PRESET, FALSE, &pItem))
			bIsPreset = ((const SfxBoolItem*)pItem)->GetValue();
		if(SFX_ITEM_SET == pExampleSet->GetItemState(SID_PARAM_CUR_NUM_LEVEL, FALSE, &pItem))
			nActNumLvl = ((const SfxUInt16Item*)pItem)->GetValue();
	}
	if(SFX_ITEM_SET == rSet.GetItemState(nNumItemId, FALSE, &pItem))
	{
		delete pSaveNum;
		pSaveNum = new SvxNumRule(*((SvxNumBulletItem*)pItem)->GetNumRule());
	}
	if(*pSaveNum != *pActNum)
	{
		*pActNum = *pSaveNum;
		pExamplesVS->SetNoSelection();
	}
	// ersten Eintrag vorselektieren
	if(aGrfNames.Count() &&
		(pActNum && (!lcl_IsNumFmtSet(pActNum, nActNumLvl) || bIsPreset)))
	{
		pExamplesVS->SelectItem(1);
		NumSelectHdl_Impl(pExamplesVS);
		bPreset = TRUE;
	}
	bPreset |= bIsPreset;
	bModified = FALSE;
}
/*-----------------12.02.97 07.46-------------------

--------------------------------------------------*/

int  SvxBitmapPickTabPage::DeactivatePage(SfxItemSet *_pSet)
{
	if(_pSet)
		FillItemSet(*_pSet);
	return TRUE;
}
/*-----------------12.02.97 07.46-------------------

--------------------------------------------------*/

BOOL  SvxBitmapPickTabPage::FillItemSet( SfxItemSet& rSet )
{
	if ( !aGrfNames.Count() )
	{
// das ist im SfxItemSet leider nicht zulaessig #52134#
//		rSet.DisableItem(SID_ATTR_NUMBERING_RULE);
		return FALSE;
	}
	if( (bPreset || bModified) && pActNum)
	{
		*pSaveNum = *pActNum;
		rSet.Put(SvxNumBulletItem( *pSaveNum ), nNumItemId);
		rSet.Put(SfxBoolItem(SID_PARAM_NUM_PRESET, bPreset));
	}

	return bModified;
}
/*-----------------12.02.97 07.46-------------------

--------------------------------------------------*/

void  SvxBitmapPickTabPage::Reset( const SfxItemSet& rSet )
{
	const SfxPoolItem* pItem;
	//im Draw gibt es das Item als WhichId, im Writer nur als SlotId
	SfxItemState eState = rSet.GetItemState(SID_ATTR_NUMBERING_RULE, FALSE, &pItem);
	if(eState != SFX_ITEM_SET)
	{
		nNumItemId = rSet.GetPool()->GetWhich(SID_ATTR_NUMBERING_RULE);
		eState = rSet.GetItemState(nNumItemId, FALSE, &pItem);
		
		if( eState != SFX_ITEM_SET )
		{
	        pItem = &static_cast< const SvxNumBulletItem& >( rSet.Get( nNumItemId, TRUE ) );		
	        eState = SFX_ITEM_SET;
		}
		
	}
	DBG_ASSERT(eState == SFX_ITEM_SET, "kein Item gefunden!");
	delete pSaveNum;
	pSaveNum = new SvxNumRule(*((SvxNumBulletItem*)pItem)->GetNumRule());

	if(!pActNum)
		pActNum = new  SvxNumRule(*pSaveNum);
	else if(*pSaveNum != *pActNum)
		*pActNum = *pSaveNum;
	if(!pActNum->IsFeatureSupported(NUM_ENABLE_LINKED_BMP))
	{
		aLinkedCB.Check(FALSE);
		aLinkedCB.Enable(FALSE);
	}
	else if(!pActNum->IsFeatureSupported(NUM_ENABLE_EMBEDDED_BMP))
	{
		aLinkedCB.Check(TRUE);
		aLinkedCB.Enable(FALSE);
	}
}

/*-----------------12.02.97 07.53-------------------

--------------------------------------------------*/

IMPL_LINK(SvxBitmapPickTabPage, NumSelectHdl_Impl, ValueSet*, EMPTYARG)
{
	if(pActNum)
	{
		bPreset = FALSE;
		bModified = TRUE;
		USHORT nIdx = pExamplesVS->GetSelectItemId() - 1;

		String* pGrfName = 0;
		if(aGrfNames.Count() > nIdx)
			pGrfName = (String*)aGrfNames.GetObject(nIdx);

		USHORT nMask = 1;
		String aEmptyStr;
		USHORT nSetNumberingType = SVX_NUM_BITMAP;
		if(aLinkedCB.IsChecked())
			nSetNumberingType |= LINK_TOKEN;
		for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
		{
			if(nActNumLvl & nMask)
			{
				SvxNumberFormat aFmt(pActNum->GetLevel(i));
				aFmt.SetNumberingType(nSetNumberingType);
				aFmt.SetPrefix( aEmptyStr );
				aFmt.SetSuffix( aEmptyStr );
				aFmt.SetCharFmtName( sNumCharFmtName );

                Graphic aGraphic;
                if(GalleryExplorer::GetGraphicObj( GALLERY_THEME_BULLETS, nIdx, &aGraphic))
				{
                    Size aSize = SvxNumberFormat::GetGraphicSizeMM100(&aGraphic);
                    sal_Int16 eOrient = text::VertOrientation::LINE_CENTER;
					aSize = OutputDevice::LogicToLogic(aSize, MAP_100TH_MM, (MapUnit)eCoreUnit);
                    SvxBrushItem aBrush(aGraphic, GPOS_AREA, SID_ATTR_BRUSH );
                    aFmt.SetGraphicBrush( &aBrush, &aSize, &eOrient );
				}
				else if(pGrfName)
					aFmt.SetGraphic( *pGrfName );
				pActNum->SetLevel(i, aFmt);
			}
			nMask <<= 1 ;
		}
	}

	return 0;
}

/*-----------------06.06.97 11.17-------------------

--------------------------------------------------*/
IMPL_LINK(SvxBitmapPickTabPage, DoubleClickHdl_Impl, ValueSet*, EMPTYARG)
{
	NumSelectHdl_Impl(pExamplesVS);
	OKButton& rOk = GetTabDialog()->GetOKButton();
	rOk.GetClickHdl().Call(&rOk);
	return 0;
}
/* -----------------03.11.99 13:46-------------------

 --------------------------------------------------*/
IMPL_LINK(SvxBitmapPickTabPage, LinkBmpHdl_Impl, CheckBox*, EMPTYARG )
{
	if(!pExamplesVS->IsNoSelection())
	{
		NumSelectHdl_Impl(pExamplesVS);
	}
	return 0;
}
/*-----------------13.02.97 09.40-------------------

--------------------------------------------------*/

//CHINA001 SvxBmpNumValueSet::SvxBmpNumValueSet( Window* pParent, const ResId& rResId/*, const List& rStrNames*/ ) :
//CHINA001
//CHINA001 SvxNumValueSet( pParent, rResId, NUM_PAGETYPE_BMP ),
//CHINA001 //    rStrList    ( rStrNames ),
//CHINA001 bGrfNotFound( FALSE )
//CHINA001
//CHINA001 {
//CHINA001 GalleryExplorer::BeginLocking(GALLERY_THEME_BULLETS);
//CHINA001 SetStyle( GetStyle() | WB_VSCROLL );
//CHINA001 SetLineCount( 3 );
//CHINA001 aFormatTimer.SetTimeout(300);
//CHINA001 aFormatTimer.SetTimeoutHdl(LINK(this, SvxBmpNumValueSet, FormatHdl_Impl));
//CHINA001 }
//CHINA001
//CHINA001 /*-----------------13.02.97 09.41-------------------
//CHINA001
//CHINA001 --------------------------------------------------*/
//CHINA001
//CHINA001 SvxBmpNumValueSet::~SvxBmpNumValueSet()
//CHINA001 {
//CHINA001 GalleryExplorer::EndLocking(GALLERY_THEME_BULLETS);
//CHINA001 aFormatTimer.Stop();
//CHINA001 }
//CHINA001 /*-----------------13.02.97 09.41-------------------
//CHINA001
//CHINA001 --------------------------------------------------*/
//CHINA001
//CHINA001 void		SvxBmpNumValueSet::UserDraw( const UserDrawEvent& rUDEvt )
//CHINA001 {
//CHINA001 SvxNumValueSet::UserDraw(rUDEvt);
//CHINA001
//CHINA001 Rectangle aRect = rUDEvt.GetRect();
//CHINA001 OutputDevice*  pDev = rUDEvt.GetDevice();
//CHINA001 USHORT	nItemId = rUDEvt.GetItemId();
//CHINA001 Point aBLPos = aRect.TopLeft();
//CHINA001
//CHINA001 int nRectHeight = aRect.GetHeight();
//CHINA001 Size aSize(nRectHeight/8, nRectHeight/8);
//CHINA001
//CHINA001 Graphic aGraphic;
//CHINA001 if(!GalleryExplorer::GetGraphicObj( GALLERY_THEME_BULLETS, nItemId - 1,
//CHINA001 &aGraphic, NULL))
//CHINA001 {
//CHINA001 bGrfNotFound = TRUE;
//CHINA001	}
//CHINA001	else
//CHINA001 {
//CHINA001 Point aPos(aBLPos.X() + 5, 0);
//CHINA001 for( USHORT i = 0; i < 3; i++ )
//CHINA001 {
//CHINA001 USHORT nY = 11 + i * 33;
//CHINA001 aPos.Y() = aBLPos.Y() + nRectHeight  * nY / 100;
//CHINA001 aGraphic.Draw( pDev, aPos, aSize );
//CHINA001		}
//CHINA001	}
//CHINA001 }
//CHINA001
//CHINA001 /*-----------------14.02.97 07.34-------------------
//CHINA001
//CHINA001 --------------------------------------------------*/
//CHINA001
//CHINA001 IMPL_LINK(SvxBmpNumValueSet, FormatHdl_Impl, Timer*, EMPTYARG)
//CHINA001 {
//CHINA001 // nur, wenn eine Grafik nicht da war, muss formatiert werden
//CHINA001 if(bGrfNotFound)
//CHINA001 {
//CHINA001 bGrfNotFound = FALSE;
//CHINA001 Format();
//CHINA001	}
//CHINA001 Invalidate();
//CHINA001 return 0;
//CHINA001 }
/*-----------------01.12.97 16:15-------------------
	Tabpage Numerierungsoptionen
--------------------------------------------------*/
#define NUM_NO_GRAPHIC 1000
SvxNumOptionsTabPage::SvxNumOptionsTabPage(Window* pParent,
							   const SfxItemSet& rSet) :
	SfxTabPage( pParent, SVX_RES( RID_SVXPAGE_NUM_OPTIONS ), rSet ),

    aFormatFL(      this, SVX_RES(FL_FORMAT   )),
    aLevelFT(       this, SVX_RES(FT_LEVEL    )),
	aLevelLB(		this, SVX_RES(LB_LEVEL	)),
	aFmtFT(			this, SVX_RES(FT_FMT		)),
	aFmtLB(			this, SVX_RES(LB_FMT		)),
	aPrefixFT(		this, SVX_RES(FT_PREFIX	)),
	aPrefixED(		this, SVX_RES(ED_PREFIX	)),
	aSuffixFT(		this, SVX_RES(FT_SUFFIX	)),
	aSuffixED(		this, SVX_RES(ED_SUFFIX	)),
	aCharFmtFT(		this, SVX_RES(FT_CHARFMT	)),
	aCharFmtLB(		this, SVX_RES(LB_CHARFMT	)),
	aBulColorFT(	this, SVX_RES(FT_BUL_COLOR)),
	aBulColLB(		this, SVX_RES(LB_BUL_COLOR)),
	aBulRelSizeFT(	this, SVX_RES(FT_BUL_REL_SIZE)),
	aBulRelSizeMF(	this, SVX_RES(MF_BUL_REL_SIZE)),
	aAllLevelFT(	this, SVX_RES(FT_ALL_LEVEL)),
	aAllLevelNF(	this, SVX_RES(NF_ALL_LEVEL)),
	aStartFT(		this, SVX_RES(FT_START	)),
	aStartED(		this, SVX_RES(ED_START	)),
	aBulletPB(		this, SVX_RES(PB_BULLET	)),
	aAlignFT(		this, SVX_RES(FT_ALIGN	)),
	aAlignLB(		this, SVX_RES(LB_ALIGN	)),
	aBitmapFT(		this, SVX_RES(FT_BITMAP	)),
	aBitmapMB(		this, SVX_RES(MB_BITMAP	)),
	aSizeFT(		this, SVX_RES(FT_SIZE		)),
	aWidthMF(		this, SVX_RES(MF_WIDTH	)),
	aMultFT(		this, SVX_RES(FT_MULT		)),
	aHeightMF(		this, SVX_RES(MF_HEIGHT	)),
	aRatioCB(		this, SVX_RES(CB_RATIO	)),
	aOrientFT(		this, SVX_RES(FT_ORIENT	)),
	aOrientLB(		this, SVX_RES(LB_ORIENT	)),
	aSameLevelFL(   this, SVX_RES(FL_SAME_LEVEL)),
	aSameLevelCB(	this, SVX_RES(CB_SAME_LEVEL)),
	pPreviewWIN(	new SvxNumberingPreview(this, SVX_RES(WIN_PREVIEW	))),
	pActNum(0),
	pSaveNum(0),
	bLastWidthModified(FALSE),
	bModified(FALSE),
	bPreset(FALSE),
	bAutomaticCharStyles(TRUE),
	bHTMLMode(FALSE),
	bMenuButtonInitialized(FALSE),
	sBullet(SVX_RES(STR_BULLET)),
	nBullet(0xff),
	nActNumLvl(USHRT_MAX),
	nNumItemId(SID_ATTR_NUMBERING_RULE)
{
	sStartWith = aStartFT.GetText();
	pPreviewWIN->SetBackground(Wallpaper(Color(COL_TRANSPARENT)));
	SetExchangeSupport();
	aActBulletFont = lcl_GetDefaultBulletFont();

	aBulletPB.SetClickHdl(LINK(this, SvxNumOptionsTabPage, BulletHdl_Impl));
	aFmtLB.SetSelectHdl(LINK(this, SvxNumOptionsTabPage, NumberTypeSelectHdl_Impl));
	aBitmapMB.SetSelectHdl(LINK(this, SvxNumOptionsTabPage, GraphicHdl_Impl));
	aLevelLB.SetSelectHdl(LINK(this, SvxNumOptionsTabPage, LevelHdl_Impl));
	aCharFmtLB.SetSelectHdl(LINK(this, SvxNumOptionsTabPage, CharFmtHdl_Impl));
	aWidthMF.SetModifyHdl(LINK(this, SvxNumOptionsTabPage, SizeHdl_Impl));
	aHeightMF.SetModifyHdl(LINK(this, SvxNumOptionsTabPage, SizeHdl_Impl));
	aRatioCB.SetClickHdl(LINK(this, SvxNumOptionsTabPage, RatioHdl_Impl));
	aStartED.SetModifyHdl(LINK(this, SvxNumOptionsTabPage, EditModifyHdl_Impl));
	aPrefixED.SetModifyHdl(LINK(this, SvxNumOptionsTabPage, EditModifyHdl_Impl));
	aSuffixED.SetModifyHdl(LINK(this, SvxNumOptionsTabPage, EditModifyHdl_Impl));
	aAllLevelNF.SetModifyHdl(LINK(this,SvxNumOptionsTabPage, AllLevelHdl_Impl));
	aOrientLB.SetSelectHdl(LINK(this, SvxNumOptionsTabPage, OrientHdl_Impl));
	aSameLevelCB.SetClickHdl(LINK(this, SvxNumOptionsTabPage, SameLevelHdl_Impl));
	aBulRelSizeMF.SetModifyHdl(LINK(this,SvxNumOptionsTabPage, BulRelSizeHdl_Impl));
	aBulColLB.SetSelectHdl(LINK(this, SvxNumOptionsTabPage, BulColorHdl_Impl));
    aInvalidateTimer.SetTimeoutHdl(LINK(this, SvxNumOptionsTabPage, PreviewInvalidateHdl_Impl));
    aInvalidateTimer.SetTimeout(50);

    aBitmapMB.GetPopupMenu()->SetHighlightHdl(LINK(this, SvxNumOptionsTabPage, PopupActivateHdl_Impl));
	PopupMenu* pPopup = new PopupMenu;
	aBitmapMB.GetPopupMenu()->SetPopupMenu( MN_GALLERY, pPopup );

	pPopup->InsertItem(	NUM_NO_GRAPHIC, String(SVX_RES(ST_POPUP_EMPTY_ENTRY)) );
	pPopup->EnableItem( NUM_NO_GRAPHIC, FALSE );

	eCoreUnit = rSet.GetPool()->GetMetric(rSet.GetPool()->GetWhich(SID_ATTR_NUMBERING_RULE));

	FreeResource();

	//get advanced numbering types from the component
	Reference<XDefaultNumberingProvider> xDefNum = lcl_GetNumberingProvider();
	Reference<XNumberingTypeInfo> xInfo(xDefNum, UNO_QUERY);

    // Extended numbering schemes present in the resource but not offered by
    // the i18n framework per configuration must be removed from the listbox.
    // Watch out for the ugly 0x88/*SVX_NUM_BITMAP|0x80*/ to not remove that.
    const USHORT nDontRemove = 0xffff;
    ::std::vector< USHORT> aRemove( aFmtLB.GetEntryCount(), nDontRemove);
    for (size_t i=0; i<aRemove.size(); ++i)
    {
        USHORT nEntryData = (USHORT)(ULONG)aFmtLB.GetEntryData(
            sal::static_int_cast< USHORT >(i));
        if (nEntryData > NumberingType::CHARS_LOWER_LETTER_N &&
                nEntryData != (SVX_NUM_BITMAP | 0x80))
            aRemove[i] = nEntryData;
    }
	if(xInfo.is())
	{
		Sequence<sal_Int16> aTypes = xInfo->getSupportedNumberingTypes(  );
		const sal_Int16* pTypes = aTypes.getConstArray();
		for(sal_Int32 nType = 0; nType < aTypes.getLength(); nType++)
		{
			sal_Int16 nCurrent = pTypes[nType];
			if(nCurrent > NumberingType::CHARS_LOWER_LETTER_N)
			{
				sal_Bool bInsert = sal_True;
				for(USHORT nEntry = 0; nEntry < aFmtLB.GetEntryCount(); nEntry++)
				{
					USHORT nEntryData = (USHORT)(ULONG)aFmtLB.GetEntryData(nEntry);
					if(nEntryData == (USHORT) nCurrent)
					{
						bInsert = sal_False;
                        aRemove[nEntry] = nDontRemove;
						break;
					}
				}
				if(bInsert)
				{
    				OUString aIdent = xInfo->getNumberingIdentifier( nCurrent );
					USHORT nPos = aFmtLB.InsertEntry(aIdent);
					aFmtLB.SetEntryData(nPos,(void*)(ULONG)nCurrent);
				}
			}
		}
	}
    for (size_t i=0; i<aRemove.size(); ++i)
    {
        if (aRemove[i] != nDontRemove)
        {
            USHORT nPos = aFmtLB.GetEntryPos( (void*)(ULONG)aRemove[i]);
            aFmtLB.RemoveEntry( nPos);
        }
    }
}

/*-----------------01.12.97 16:30-------------------

--------------------------------------------------*/
SvxNumOptionsTabPage::~SvxNumOptionsTabPage()
{
	delete aBitmapMB.GetPopupMenu()->GetPopupMenu( MN_GALLERY );
	String* pStr = (String*)aGrfNames.First();
	while( pStr )
	{
		delete pStr;
		pStr = (String*)aGrfNames.Next();
	}
    delete pActNum;
	delete pPreviewWIN;
	delete pSaveNum;
}

/*-----------------03.12.97 07:52-------------------

--------------------------------------------------*/
void SvxNumOptionsTabPage::SetMetric(FieldUnit eMetric)
{
	if(eMetric == FUNIT_MM)
	{
		aWidthMF     .SetDecimalDigits(1);
		aHeightMF     .SetDecimalDigits(1);
	}
	aWidthMF .SetUnit( eMetric );
	aHeightMF .SetUnit( eMetric );
}

/*-----------------01.12.97 16:30-------------------

--------------------------------------------------*/
SfxTabPage*	SvxNumOptionsTabPage::Create( Window* pParent,
								const SfxItemSet& rAttrSet)
{
	return new SvxNumOptionsTabPage(pParent, rAttrSet);
};
/*-----------------01.12.97 16:29-------------------

--------------------------------------------------*/
void 	SvxNumOptionsTabPage::ActivatePage(const SfxItemSet& rSet)
{
	const SfxPoolItem* pItem;
	const SfxItemSet* pExampleSet = GetTabDialog()->GetExampleSet();
	UINT16 nTmpNumLvl = USHRT_MAX;
	if(pExampleSet)
	{
		if(SFX_ITEM_SET == pExampleSet->GetItemState(SID_PARAM_NUM_PRESET, FALSE, &pItem))
			bPreset = ((const SfxBoolItem*)pItem)->GetValue();
		if(SFX_ITEM_SET == pExampleSet->GetItemState(SID_PARAM_CUR_NUM_LEVEL, FALSE, &pItem))
			nTmpNumLvl = ((const SfxUInt16Item*)pItem)->GetValue();
	}
	if(SFX_ITEM_SET == rSet.GetItemState(nNumItemId, FALSE, &pItem))
	{
		delete pSaveNum;
		pSaveNum = new SvxNumRule(*((SvxNumBulletItem*)pItem)->GetNumRule());
	}
	//
	bModified = (!pActNum->Get( 0 ) || bPreset);
	if(*pActNum != *pSaveNum ||
		nActNumLvl != nTmpNumLvl)
	{
		nActNumLvl = nTmpNumLvl;
		USHORT nMask = 1;
		aLevelLB.SetUpdateMode(FALSE);
		aLevelLB.SetNoSelection();
		aLevelLB.SelectEntryPos( pActNum->GetLevelCount(), nActNumLvl == USHRT_MAX);
		if(nActNumLvl != USHRT_MAX)
			for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
			{
				if(nActNumLvl & nMask)
					aLevelLB.SelectEntryPos( i, TRUE);
				nMask <<= 1 ;
			}
		aLevelLB.SetUpdateMode(TRUE);
		*pActNum = *pSaveNum;
		InitControls();
	}

}
/*-----------------01.12.97 16:29-------------------

--------------------------------------------------*/
int 	SvxNumOptionsTabPage::DeactivatePage(SfxItemSet * _pSet)
{
	if(_pSet)
		FillItemSet(*_pSet);
	return TRUE;
}
/*-----------------01.12.97 16:29-------------------

--------------------------------------------------*/
BOOL 	SvxNumOptionsTabPage::FillItemSet( SfxItemSet& rSet )
{
	rSet.Put(SfxUInt16Item(SID_PARAM_CUR_NUM_LEVEL, nActNumLvl));
	if(bModified && pActNum)
	{
		*pSaveNum = *pActNum;
		rSet.Put(SvxNumBulletItem( *pSaveNum ), nNumItemId);
		rSet.Put(SfxBoolItem(SID_PARAM_NUM_PRESET, FALSE));
	}
	return bModified;
};
/*-----------------01.12.97 16:29-------------------

--------------------------------------------------*/
void 	SvxNumOptionsTabPage::Reset( const SfxItemSet& rSet )
{
	const SfxPoolItem* pItem;
	//im Draw gibt es das Item als WhichId, im Writer nur als SlotId
	SfxItemState eState = rSet.GetItemState(SID_ATTR_NUMBERING_RULE, FALSE, &pItem);
	if(eState != SFX_ITEM_SET)
	{
		nNumItemId = rSet.GetPool()->GetWhich(SID_ATTR_NUMBERING_RULE);
		eState = rSet.GetItemState(nNumItemId, FALSE, &pItem);
		
		if( eState != SFX_ITEM_SET )
		{
	        pItem = &static_cast< const SvxNumBulletItem& >( rSet.Get( nNumItemId, TRUE ) );		
	        eState = SFX_ITEM_SET;
		}
		
	}
	DBG_ASSERT(eState == SFX_ITEM_SET, "kein Item gefunden!");
	delete pSaveNum;
	pSaveNum = new SvxNumRule(*((SvxNumBulletItem*)pItem)->GetNumRule());

	// Ebenen einfuegen
	if(!aLevelLB.GetEntryCount())
	{
		for(USHORT i = 1; i <= pSaveNum->GetLevelCount(); i++)
			aLevelLB.InsertEntry( UniString::CreateFromInt32(i));
		if(pSaveNum->GetLevelCount() > 1)
		{
			String sEntry( UniString::CreateFromAscii( RTL_CONSTASCII_STRINGPARAM( "1 - " ) ) );
			sEntry += UniString::CreateFromInt32( pSaveNum->GetLevelCount() );
			aLevelLB.InsertEntry(sEntry);
			aLevelLB.SelectEntry(sEntry);
		}
		else
			aLevelLB.SelectEntryPos(0);
	}
	else
		aLevelLB.SelectEntryPos(aLevelLB.GetEntryCount() - 1);

//	nActNumLvl = ((SwNumBulletTabDialog*)GetTabDialog())->GetActNumLevel();
	USHORT nMask = 1;
	aLevelLB.SetUpdateMode(FALSE);
	aLevelLB.SetNoSelection();
	if(nActNumLvl == USHRT_MAX)
	{
		aLevelLB.SelectEntryPos( pSaveNum->GetLevelCount(), TRUE);
	}
	else
		for(USHORT i = 0; i < pSaveNum->GetLevelCount(); i++)
		{
			if(nActNumLvl & nMask)
				aLevelLB.SelectEntryPos( i, TRUE);
			nMask <<= 1 ;
		}
	aLevelLB.SetUpdateMode(TRUE);

	if(!pActNum)
		pActNum = new  SvxNumRule(*pSaveNum);
	else if(*pSaveNum != *pActNum)
		*pActNum = *pSaveNum;
	pPreviewWIN->SetNumRule(pActNum);
	aSameLevelCB.Check(pActNum->IsContinuousNumbering());

	//ColorListBox bei Bedarf fuellen
	if ( pActNum->IsFeatureSupported( NUM_BULLET_COLOR ) )
	{
		SfxObjectShell* pDocSh = SfxObjectShell::Current();
		DBG_ASSERT( pDocSh, "DocShell not found!" );
		XColorTable* pColorTable = NULL;
		FASTBOOL bKillTable = FALSE;
		if ( pDocSh )
        {
            pItem = pDocSh->GetItem( SID_COLOR_TABLE );
            if ( pItem )
                pColorTable = ( (SvxColorTableItem*)pItem )->GetColorTable();
        }

		if ( !pColorTable )
		{
			pColorTable = new XColorTable( SvtPathOptions().GetPalettePath() );
			bKillTable = TRUE;
		}

   		aBulColLB.InsertEntry( Color( COL_AUTO ), SVX_RESSTR( RID_SVXSTR_AUTOMATIC ));

		for ( long i = 0; i < pColorTable->Count(); i++ )
		{
			XColorEntry* pEntry = pColorTable->GetColor(i);
			aBulColLB.InsertEntry( pEntry->GetColor(), pEntry->GetName() );
		}

		if ( bKillTable )
			delete pColorTable;
	}

	SfxObjectShell* pShell;
	if ( SFX_ITEM_SET == rSet.GetItemState( SID_HTML_MODE, FALSE, &pItem )
		 || ( 0 != ( pShell = SfxObjectShell::Current()) &&
			  0 != ( pItem = pShell->GetItem( SID_HTML_MODE ) ) ) )
	{
		USHORT nHtmlMode = ((SfxUInt16Item*)pItem)->GetValue();
		bHTMLMode = 0 != (nHtmlMode&HTMLMODE_ON);
	}

	BOOL bCharFmt = pActNum->IsFeatureSupported(NUM_CHAR_STYLE);
	aCharFmtFT.Show(bCharFmt);
	aCharFmtLB.Show(bCharFmt);

	BOOL bContinuous = pActNum->IsFeatureSupported(NUM_CONTINUOUS);

	BOOL bAllLevel = bContinuous && !bHTMLMode;
	aAllLevelFT.Show(bAllLevel);
	aAllLevelNF.Show(bAllLevel);

    aSameLevelFL.Show(bContinuous);
	aSameLevelCB.Show(bContinuous);
	//wieder Missbrauch: im Draw gibt es die Numerierung nur bis zum Bitmap
	// without SVX_NUM_NUMBER_NONE
	//remove types that are unsupported by Draw/Impress
	if(!bContinuous)
	{
		USHORT nFmtCount = aFmtLB.GetEntryCount();
		for(USHORT i = nFmtCount; i; i--)
		{
			USHORT nEntryData = (USHORT)(ULONG)aFmtLB.GetEntryData(i - 1);
			if(/*SVX_NUM_NUMBER_NONE == nEntryData ||*/
                ((SVX_NUM_BITMAP|LINK_TOKEN) ==  nEntryData))
				aFmtLB.RemoveEntry(i - 1);
		}
	}
	//one must be enabled
	if(!pActNum->IsFeatureSupported(NUM_ENABLE_LINKED_BMP))
	{
		long nData = SVX_NUM_BITMAP|LINK_TOKEN;
		USHORT nPos = aFmtLB.GetEntryPos((void*)nData);
		if(LISTBOX_ENTRY_NOTFOUND != nPos)
			aFmtLB.RemoveEntry(nPos);
	}
	else if(!pActNum->IsFeatureSupported(NUM_ENABLE_EMBEDDED_BMP))
	{
		long nData = SVX_NUM_BITMAP;
		USHORT nPos = aFmtLB.GetEntryPos((void*)nData);
		if(LISTBOX_ENTRY_NOTFOUND != nPos)
			aFmtLB.RemoveEntry(nPos);
	}
	if(pActNum->IsFeatureSupported(NUM_SYMBOL_ALIGNMENT))
	{
		aAlignFT.Show();
		aAlignLB.Show();
        Size aSz(aFormatFL.GetSizePixel());
        aSz.Height() = aLevelFT.GetSizePixel().Height();
        aFormatFL.SetSizePixel(aSz);
		aAlignLB.SetSelectHdl(LINK(this, SvxNumOptionsTabPage, EditModifyHdl_Impl));
	}

	//MegaHack: Aufgrund eines nicht fixbaren 'designfehlers' im Impress
	//Alle arten der numerischen Aufzaehlungen loeschen
	if(pActNum->IsFeatureSupported(NUM_NO_NUMBERS))
	{
		USHORT nFmtCount = aFmtLB.GetEntryCount();
		for(USHORT i = nFmtCount; i; i--)
		{
			USHORT nEntryData = (USHORT)(ULONG)aFmtLB.GetEntryData(i - 1);
			if( /*nEntryData >= SVX_NUM_CHARS_UPPER_LETTER &&*/  nEntryData <= SVX_NUM_NUMBER_NONE)
				aFmtLB.RemoveEntry(i - 1);
		}
	}

	InitControls();
	bModified = FALSE;

}
/*-----------------02.12.97 13:47-------------------

--------------------------------------------------*/
void SvxNumOptionsTabPage::InitControls()
{
	BOOL bShowBullet 	= TRUE;
	BOOL bShowBitmap 	= TRUE;
	BOOL bSameType 		= TRUE;
	BOOL bSameStart 	= TRUE;
	BOOL bSamePrefix 	= TRUE;
	BOOL bSameSuffix 	= TRUE;
	BOOL bAllLevel 		= TRUE;
	BOOL bSameCharFmt	= TRUE;
	BOOL bSameVOrient	= TRUE;
	BOOL bSameSize		= TRUE;
	BOOL bSameBulColor 	= TRUE;
	BOOL bSameBulRelSize= TRUE;
	BOOL bSameAdjust 	= TRUE;

	const SvxNumberFormat* aNumFmtArr[SVX_MAX_NUM];
	String sFirstCharFmt;
    sal_Int16 eFirstOrient = text::VertOrientation::NONE;
	Size aFirstSize(0,0);
	USHORT nMask = 1;
	USHORT nLvl = USHRT_MAX;
	USHORT nHighestLevel = 0;
	String aEmptyStr;

	BOOL bBullColor = pActNum->IsFeatureSupported(NUM_BULLET_COLOR);
	BOOL bBullRelSize = pActNum->IsFeatureSupported(NUM_BULLET_REL_SIZE);
	for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
	{
		if(nActNumLvl & nMask)
		{
			aNumFmtArr[i] = &pActNum->GetLevel(i);
			bShowBullet &= aNumFmtArr[i]->GetNumberingType() == SVX_NUM_CHAR_SPECIAL;
			bShowBitmap &= (aNumFmtArr[i]->GetNumberingType()&(~LINK_TOKEN)) == SVX_NUM_BITMAP;
			if(USHRT_MAX == nLvl)
			{
				nLvl = i;
				sFirstCharFmt = aNumFmtArr[i]->GetCharFmtName();
				eFirstOrient = aNumFmtArr[i]->GetVertOrient();
				if(bShowBitmap)
					aFirstSize = aNumFmtArr[i]->GetGraphicSize();
			}
			if( i > nLvl)
			{
				bSameType &=   aNumFmtArr[i]->GetNumberingType() == aNumFmtArr[nLvl]->GetNumberingType();
				bSameStart = aNumFmtArr[i]->GetStart() == aNumFmtArr[nLvl]->GetStart();

				bSamePrefix = aNumFmtArr[i]->GetPrefix() == aNumFmtArr[nLvl]->GetPrefix();
				bSameSuffix = aNumFmtArr[i]->GetSuffix() == aNumFmtArr[nLvl]->GetSuffix();
				bAllLevel &= aNumFmtArr[i]->GetIncludeUpperLevels() == aNumFmtArr[nLvl]->GetIncludeUpperLevels();
				bSameCharFmt 	&= 	sFirstCharFmt == aNumFmtArr[i]->GetCharFmtName();
				bSameVOrient    &= eFirstOrient == aNumFmtArr[i]->GetVertOrient();
				if(bShowBitmap && bSameSize)
					bSameSize &= aNumFmtArr[i]->GetGraphicSize() == aFirstSize;
				bSameBulColor &= aNumFmtArr[i]->GetBulletColor() == aNumFmtArr[nLvl]->GetBulletColor();
				bSameBulRelSize &= aNumFmtArr[i]->GetBulletRelSize() == aNumFmtArr[nLvl]->GetBulletRelSize();
				bSameAdjust 	&= aNumFmtArr[i]->GetNumAdjust() == aNumFmtArr[nLvl]->GetNumAdjust();
			}
			nHighestLevel = i;
		}
		else
			aNumFmtArr[i] = 0;

		nMask <<= 1 ;

	}
	SwitchNumberType(bShowBullet ? 1 : bShowBitmap ? 2 : 0);
    CheckForStartValue_Impl(aNumFmtArr[nLvl]->GetNumberingType());
	if(bShowBitmap)
	{
        if(!bSameVOrient || eFirstOrient == text::VertOrientation::NONE)
			aOrientLB.SetNoSelection();
		else
			aOrientLB.SelectEntryPos(
                sal::static_int_cast< USHORT >(eFirstOrient - 1));
                // kein text::VertOrientation::NONE

		if(bSameSize)
		{
			SetMetricValue(aHeightMF, aFirstSize.Height(), eCoreUnit);
			SetMetricValue(aWidthMF, aFirstSize.Width(), eCoreUnit);
		}
		else
		{
			aHeightMF.SetText(aEmptyStr);
			aWidthMF.SetText(aEmptyStr);
		}
	}

	if(bSameType)
	{
		USHORT nLBData = (USHORT) aNumFmtArr[nLvl]->GetNumberingType();
		aFmtLB.SelectEntryPos(aFmtLB.GetEntryPos( (void*)sal::static_int_cast<sal_uIntPtr>( nLBData ) ));
	}
	else
		aFmtLB.SetNoSelection();

	aAllLevelNF.Enable(nHighestLevel > 0 && !aSameLevelCB.IsChecked());
    aAllLevelNF.SetMax(nHighestLevel + 1);
	if(bAllLevel)
	{
		aAllLevelNF.SetValue(aNumFmtArr[nLvl]->GetIncludeUpperLevels());
	}
	else
	{
		aAllLevelNF.SetText(aEmptyStr);
	}
	if(bSameAdjust)
	{
		USHORT nPos = 1; // zentriert
		if(aNumFmtArr[nLvl]->GetNumAdjust() == SVX_ADJUST_LEFT)
			nPos = 0;
		else if(aNumFmtArr[nLvl]->GetNumAdjust() == SVX_ADJUST_RIGHT)
			nPos = 2;
		aAlignLB.SelectEntryPos(nPos);
	}
	else
	{
		aAlignLB.SetNoSelection();
	}

	if(bBullRelSize)
	{
		if(bSameBulRelSize)
			aBulRelSizeMF.SetValue(aNumFmtArr[nLvl]->GetBulletRelSize());
		else
			aBulRelSizeMF.SetText(aEmptyStr);
	}
	if(bBullColor)
	{
		if(bSameBulColor)
			aBulColLB.SelectEntry(aNumFmtArr[nLvl]->GetBulletColor());
		else
			aBulColLB.SetNoSelection();
	}
	switch(nBullet)
	{
		case SHOW_NUMBERING:
			if(bSameStart)
			{
				aStartED.SetValue(aNumFmtArr[nLvl]->GetStart());
			}
			else
				aStartED.SetText(aEmptyStr);
		break;
		case SHOW_BULLET:
		{
//				const Font* pFnt = aNumFmtArr[Lvl]->GetBulletFont();
//				if(pFnt)
//					ChgTxtFont(aBulletFT, *pFnt);
//				aBulletFT.SetText(String((char)aNumFmtArr[nLvl]->GetBulletChar()));
		}
		break;
		case SHOW_BITMAP:
		break;
	}

	if(bSamePrefix)
		aPrefixED.SetText(aNumFmtArr[nLvl]->GetPrefix());
	else
		aPrefixED.SetText(aEmptyStr);
	if(bSameSuffix)
		aSuffixED.SetText(aNumFmtArr[nLvl]->GetSuffix());
	else
		aSuffixED.SetText(aEmptyStr);

	if(bSameCharFmt)
	{
		if(sFirstCharFmt.Len())
				aCharFmtLB.SelectEntry(sFirstCharFmt);
		else
			aCharFmtLB.SelectEntryPos( 0 );
	}
	else
		aCharFmtLB.SetNoSelection();

    pPreviewWIN->SetLevel(nActNumLvl);
	pPreviewWIN->Invalidate();
}

/*-----------------02.12.97 14:01-------------------
	 0 - Nummer; 1 - Bullet; 2 - Bitmap
--------------------------------------------------*/

void SvxNumOptionsTabPage::SwitchNumberType( BYTE nType, BOOL )
{
	if(nBullet == nType)
		return;
	nBullet = nType;
	BOOL bBitmap = FALSE;
	BOOL bBullet = FALSE;
	BOOL bEnableBitmap = FALSE;
	if(nType == SHOW_NUMBERING)
	{
		// Label umschalten, alten Text merken
		aStartFT.SetText(sStartWith);

	}
	else if(nType == SHOW_BULLET)
	{
		// Label umschalten, alten Text merken
		aStartFT.SetText(sBullet);
		bBullet = TRUE;
	}
	else
	{
		bBitmap = TRUE;
		bEnableBitmap = TRUE;
	}
	BOOL bNumeric = !(bBitmap||bBullet);
	aPrefixFT.Show(bNumeric);
	aPrefixED.Show(bNumeric);
	aSuffixFT.Show(bNumeric);
	aSuffixED.Show(bNumeric);

	BOOL bCharFmt = pActNum->IsFeatureSupported(NUM_CHAR_STYLE);
	aCharFmtFT.Show(!bBitmap && bCharFmt);
	aCharFmtLB.Show(!bBitmap && bCharFmt);

	// das ist eigentlich Missbrauch, da fuer die vollst. Numerierung kein
	// eigenes Flag existiert
	BOOL bAllLevelFeature = pActNum->IsFeatureSupported(NUM_CONTINUOUS);
	BOOL bAllLevel = bNumeric && bAllLevelFeature && !bHTMLMode;
	aAllLevelFT.Show(bAllLevel);
	aAllLevelNF.Show(bAllLevel);

	aStartFT.Show(!bBitmap);
	aStartED.Show(!(bBullet||bBitmap));

	aBulletPB.Show(bBullet);
	BOOL bBullColor = pActNum->IsFeatureSupported(NUM_BULLET_COLOR);
	aBulColorFT.Show( 	!bBitmap && bBullColor );
	aBulColLB.Show(		!bBitmap && bBullColor );
	BOOL bBullResSize = pActNum->IsFeatureSupported(NUM_BULLET_REL_SIZE);
	aBulRelSizeFT.Show(	!bBitmap && bBullResSize );
	aBulRelSizeMF.Show(	!bBitmap && bBullResSize );

	aBitmapFT	.Show(bBitmap);
	aBitmapMB	.Show(bBitmap);

	aSizeFT		.Show(bBitmap);
	aWidthMF	.Show(bBitmap);
	aMultFT	    .Show(bBitmap);
	aHeightMF	.Show(bBitmap);
	aRatioCB	.Show(bBitmap);

	aOrientFT	.Show(bBitmap &&  bAllLevelFeature);
	aOrientLB	.Show(bBitmap &&  bAllLevelFeature);

	aSizeFT		.Enable(bEnableBitmap);
	aWidthMF	.Enable(bEnableBitmap);
	aMultFT	    .Enable(bEnableBitmap);
	aHeightMF	.Enable(bEnableBitmap);
	aRatioCB	.Enable(bEnableBitmap);
	aOrientFT	.Enable(bEnableBitmap);
	aOrientLB	.Enable(bEnableBitmap);

}
/*-----------------02.12.97 13:51-------------------

--------------------------------------------------*/
IMPL_LINK( SvxNumOptionsTabPage, LevelHdl_Impl, ListBox *, pBox )
{
	USHORT nSaveNumLvl = nActNumLvl;
	nActNumLvl = 0;
	if(pBox->IsEntryPosSelected( pActNum->GetLevelCount() ) &&
		(pBox->GetSelectEntryCount() == 1 || nSaveNumLvl != 0xffff))
	{
		nActNumLvl = 0xFFFF;
		pBox->SetUpdateMode(FALSE);
		for( USHORT i = 0; i < pActNum->GetLevelCount(); i++ )
			pBox->SelectEntryPos( i, FALSE );
		pBox->SetUpdateMode(TRUE);
	}
	else if(pBox->GetSelectEntryCount())
	{
		USHORT nMask = 1;
		for( USHORT i = 0; i < pActNum->GetLevelCount(); i++ )
		{
			if(pBox->IsEntryPosSelected( i ))
				nActNumLvl |= nMask;
			nMask <<= 1;
		}
		pBox->SelectEntryPos( pActNum->GetLevelCount(), FALSE );
	}
	else
	{
		nActNumLvl = nSaveNumLvl;
		USHORT nMask = 1;
		for( USHORT i = 0; i < pActNum->GetLevelCount(); i++ )
		{
			if(nActNumLvl & nMask)
			{
				pBox->SelectEntryPos(i);
				break;
			}
			nMask <<=1;
		}
	}
	InitControls();
	return 0;
}
/* -----------------------------05.04.2002 15:30------------------------------

 ---------------------------------------------------------------------------*/
IMPL_LINK( SvxNumOptionsTabPage, PreviewInvalidateHdl_Impl, Timer*, EMPTYARG )
{
    pPreviewWIN->Invalidate();
    return 0;
}
/*-----------------03.12.97 12:01-------------------

--------------------------------------------------*/
IMPL_LINK( SvxNumOptionsTabPage, AllLevelHdl_Impl, NumericField*, pBox )
{
	for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
	{
		USHORT nMask = 1;
		for(USHORT e = 0; e < pActNum->GetLevelCount(); e++)
		{
			if(nActNumLvl & nMask)
			{
				SvxNumberFormat aNumFmt(pActNum->GetLevel(e));
				aNumFmt.SetIncludeUpperLevels((BYTE) std::min(pBox->GetValue(), sal_Int64(e + 1)) );
				pActNum->SetLevel(e, aNumFmt);
			}
			nMask <<= 1;
		}
	}
	SetModified();
	return 0;
}

/*-----------------02.12.97 08:56-------------------

--------------------------------------------------*/
IMPL_LINK( SvxNumOptionsTabPage, NumberTypeSelectHdl_Impl, ListBox *, pBox )
{
	String sSelectStyle;
	sal_Int16 eOldType;
	BOOL bShowOrient = FALSE;
	BOOL bBmp = FALSE;
	String aEmptyStr;
	USHORT nMask = 1;
	for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
	{
		if(nActNumLvl & nMask)
		{
			SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
			eOldType = aNumFmt.GetNumberingType();
			// PAGEDESC gibt es nicht
			USHORT nNumType = (USHORT)(ULONG)pBox->GetEntryData(pBox->GetSelectEntryPos());
			aNumFmt.SetNumberingType((sal_Int16)nNumType);
            sal_uInt16 nNumberingType = aNumFmt.GetNumberingType();
            if(SVX_NUM_BITMAP == (nNumberingType&(~LINK_TOKEN)))
			{
				bBmp |= 0 != aNumFmt.GetBrush();
				aNumFmt.SetIncludeUpperLevels( FALSE );
				aNumFmt.SetSuffix( aEmptyStr );
				aNumFmt.SetPrefix( aEmptyStr );
				if(!bBmp)
					aNumFmt.SetGraphic(aEmptyStr);
				pActNum->SetLevel(i, aNumFmt);
				SwitchNumberType(SHOW_BITMAP, bBmp );
				bShowOrient = TRUE;
			}
            else if( SVX_NUM_CHAR_SPECIAL == nNumberingType )
			{
				aNumFmt.SetIncludeUpperLevels( FALSE );
				aNumFmt.SetSuffix( aEmptyStr );
				aNumFmt.SetPrefix( aEmptyStr );
				if( !aNumFmt.GetBulletFont() )
					aNumFmt.SetBulletFont(&aActBulletFont);
				if( !aNumFmt.GetBulletChar() )
					aNumFmt.SetBulletChar( SVX_DEF_BULLET );
				pActNum->SetLevel(i, aNumFmt);
				SwitchNumberType(SHOW_BULLET);
				//ChgTxtFont(aBulletFT, *aNumFmt.GetBulletFont());
				//aBulletFT.SetText( aNumFmt.GetBulletChar() );
				// Zuweisung der Zeichenvorlage automatisch
				if(bAutomaticCharStyles)
				{
					sSelectStyle = sBulletCharFmtName;
				}
			}
			else
			{
				aNumFmt.SetPrefix( aPrefixED.GetText() );
				aNumFmt.SetSuffix( aSuffixED.GetText() );
//				aNumFmt.SetBulletFont(0);
				SwitchNumberType(SHOW_NUMBERING);
				pActNum->SetLevel(i, aNumFmt);
                CheckForStartValue_Impl(nNumberingType);

				// Zuweisung der Zeichenvorlage automatisch
				if(bAutomaticCharStyles)
				{
					sSelectStyle = sNumCharFmtName;
				}
			}
		}
		nMask <<= 1;
	}
	BOOL bAllLevelFeature = pActNum->IsFeatureSupported(NUM_CONTINUOUS);
	if(bShowOrient && bAllLevelFeature)
	{
		aOrientFT.Show();
		aOrientLB.Show();
	}
	else
	{
		aOrientFT.Hide();
		aOrientLB.Hide();
	}
	SetModified();
	if(sSelectStyle.Len())
	{
		aCharFmtLB.SelectEntry(sSelectStyle);
		CharFmtHdl_Impl(&aCharFmtLB);
		// bAutomaticCharStyles wird im CharFmtHdl_Impl zurueckgesetzt
		bAutomaticCharStyles = TRUE;
	}
	return 0;
}
/* -----------------06.11.2002 14:27-----------------
 *
 * --------------------------------------------------*/
void SvxNumOptionsTabPage::CheckForStartValue_Impl(sal_uInt16 nNumberingType)
{
    BOOL bIsNull = aStartED.GetValue() == 0;
    BOOL bNoZeroAllowed = nNumberingType < SVX_NUM_ARABIC ||
                        SVX_NUM_CHARS_UPPER_LETTER_N == nNumberingType ||
                        SVX_NUM_CHARS_LOWER_LETTER_N == nNumberingType;
    aStartED.SetMin(bNoZeroAllowed ? 1 : 0);
    if(bIsNull && bNoZeroAllowed)
        aStartED.GetModifyHdl().Call(&aStartED);
}
/*-----------------03.12.97 16:43-------------------

--------------------------------------------------*/
IMPL_LINK( SvxNumOptionsTabPage, OrientHdl_Impl, ListBox *, pBox )
{
	USHORT nPos = pBox->GetSelectEntryPos();
	nPos ++; // kein VERT_NONE

	USHORT nMask = 1;
	for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
	{
		if(nActNumLvl & nMask)
		{
			SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
			if(SVX_NUM_BITMAP == (aNumFmt.GetNumberingType()&(~LINK_TOKEN)))
			{
				const SvxBrushItem* pBrushItem =  aNumFmt.GetBrush();
				const Size& rSize = aNumFmt.GetGraphicSize();
                sal_Int16 eOrient = (sal_Int16)nPos;
				aNumFmt.SetGraphicBrush( pBrushItem, &rSize, &eOrient );
				pActNum->SetLevel(i, aNumFmt);
			}
		}
		nMask <<= 1;
	}
	SetModified(FALSE);
	return 0;

}

/*-----------------06.12.97 12:00-------------------

--------------------------------------------------*/
IMPL_LINK( SvxNumOptionsTabPage, SameLevelHdl_Impl, CheckBox *, pBox )
{
	BOOL bSet = pBox->IsChecked();
	pActNum->SetContinuousNumbering(bSet);
	BOOL bRepaint = FALSE;
	for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
	{
		SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
		if(aNumFmt.GetNumberingType() != SVX_NUM_NUMBER_NONE)
		{
			bRepaint = TRUE;
			break;
		}
	}
	SetModified(bRepaint);
	InitControls();
	return 0;
}
/* -----------------16.11.98 14:20-------------------
 *
 * --------------------------------------------------*/
IMPL_LINK( SvxNumOptionsTabPage, BulColorHdl_Impl, ColorListBox*, pBox )
{
	Color nSetColor = pBox->GetSelectEntryColor();

	USHORT nMask = 1;
	for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
	{
		if(nActNumLvl & nMask)
		{
			SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
			aNumFmt.SetBulletColor(nSetColor);
			pActNum->SetLevel(i, aNumFmt);
		}
		nMask <<= 1;
	}
	SetModified();
	return 0;
}
/* -----------------16.11.98 14:20-------------------
 *
 * --------------------------------------------------*/
IMPL_LINK( SvxNumOptionsTabPage, BulRelSizeHdl_Impl, MetricField *, pField)
{
	USHORT nRelSize = (USHORT)pField->GetValue();

	USHORT nMask = 1;
	for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
	{
		if(nActNumLvl & nMask)
		{
			SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
			aNumFmt.SetBulletRelSize(nRelSize);
			pActNum->SetLevel(i, aNumFmt);
		}
		nMask <<= 1;
	}
	SetModified();
	return 0;
}

/*-----------------02.12.97 10:50-------------------

--------------------------------------------------*/
IMPL_LINK( SvxNumOptionsTabPage, GraphicHdl_Impl, MenuButton *, pButton )
{
	USHORT 					nItemId = pButton->GetCurItemId();
	String 					aGrfName;
	Size 					aSize;
	sal_Bool				bSucc(sal_False);
	SvxOpenGraphicDialog	aGrfDlg( SVX_RES(RID_STR_EDIT_GRAPHIC) );
//	aGrfDlg.SetDialogHelpId( HID_NUMBERINGOPT_SEL_GRF_FROM_FILE );

	if(MN_GALLERY_ENTRY <= nItemId )
	{
		aGrfName = *((String*)aGrfNames.GetObject( nItemId - MN_GALLERY_ENTRY));
        Graphic aGraphic;
        if(GalleryExplorer::GetGraphicObj( GALLERY_THEME_BULLETS, nItemId - MN_GALLERY_ENTRY, &aGraphic))
        {
            aSize = SvxNumberFormat::GetGraphicSizeMM100(&aGraphic);
            bSucc = sal_True;
		}
	}
	else
	{
		aGrfDlg.EnableLink( sal_False );
		aGrfDlg.AsLink( sal_False );
		if ( !aGrfDlg.Execute() )
		{
			// ausgewaehlten Filter merken
			aGrfName = aGrfDlg.GetPath();

			Graphic	aGraphic;
			if( !aGrfDlg.GetGraphic(aGraphic) )
			{
				aSize = SvxNumberFormat::GetGraphicSizeMM100(&aGraphic);
				bSucc = sal_True;
			}
		}
	}
	if(bSucc)
	{
		aSize = OutputDevice::LogicToLogic(aSize, MAP_100TH_MM, (MapUnit)eCoreUnit);

		USHORT nMask = 1;
		for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
		{
			if(nActNumLvl & nMask)
			{
				SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
				aNumFmt.SetCharFmtName(sNumCharFmtName);
				aNumFmt.SetGraphic(aGrfName);

				// Size schon mal fuer spaeteren Groessenabgleich setzen
				const SvxBrushItem* pBrushItem = aNumFmt.GetBrush();
                // initiate asynchronous loading
                sal_Int16 eOrient = aNumFmt.GetVertOrient();
				aNumFmt.SetGraphicBrush( pBrushItem, &aSize, &eOrient );
				aInitSize[i] = aNumFmt.GetGraphicSize();

				pActNum->SetLevel(i, aNumFmt);
			}
			nMask <<= 1;
		}
		aRatioCB .Enable();
		aSizeFT .Enable();
		aMultFT.Enable();
		aWidthMF .Enable();
		aHeightMF.Enable();
		SetMetricValue(aWidthMF, aSize.Width(), eCoreUnit);
		SetMetricValue(aHeightMF, aSize.Height(), eCoreUnit);
		aOrientFT.Enable();
		aOrientLB.Enable();
		SetModified();
        //needed due to asynchronous loading of graphics in the SvxBrushItem
        aInvalidateTimer.Start();
	}
	return 0;
}
/* -----------------27.07.99 12:20-------------------

 --------------------------------------------------*/
IMPL_LINK( SvxNumOptionsTabPage, PopupActivateHdl_Impl, Menu *, EMPTYARG )
{
	if(!bMenuButtonInitialized)
	{
		bMenuButtonInitialized = TRUE;
		EnterWait();
		PopupMenu* pPopup = aBitmapMB.GetPopupMenu()->GetPopupMenu( MN_GALLERY );
		GalleryExplorer::FillObjList(GALLERY_THEME_BULLETS, aGrfNames);
		if(aGrfNames.Count())
		{
			pPopup->RemoveItem( pPopup->GetItemPos( NUM_NO_GRAPHIC ));
			String aEmptyStr;
            GalleryExplorer::BeginLocking(GALLERY_THEME_BULLETS);

            for(USHORT i = 0; i < aGrfNames.Count(); i++)
			{
                Graphic aGraphic;
                String sGrfName = *(const String*)aGrfNames.GetObject(i);
                INetURLObject aObj(sGrfName);
                if(aObj.GetProtocol() == INET_PROT_FILE)
                    sGrfName = aObj.PathToFileName();
                if(GalleryExplorer::GetGraphicObj( GALLERY_THEME_BULLETS, i, &aGraphic))
                {
                    Bitmap aBitmap(aGraphic.GetBitmap());
					Size aSize(aBitmap.GetSizePixel());
					if(aSize.Width() > MAX_BMP_WIDTH ||
						aSize.Height() > MAX_BMP_HEIGHT)
					{
						BOOL bWidth = aSize.Width() > aSize.Height();
						double nScale = bWidth ?
											(double)MAX_BMP_WIDTH / (double)aSize.Width():
												(double)MAX_BMP_HEIGHT / (double)aSize.Height();
						aBitmap.Scale(nScale, nScale);
					}
					Image aImage(aBitmap);

                    pPopup->InsertItem(MN_GALLERY_ENTRY + i, sGrfName, aImage );
				}
				else
				{
					Image aImage;
					pPopup->InsertItem(
                        MN_GALLERY_ENTRY + i, sGrfName, aImage );
				}
			}
            GalleryExplorer::EndLocking(GALLERY_THEME_BULLETS);
        }
		LeaveWait();
	}
	return 0;
}

/*-----------------02.12.97 10:58-------------------

--------------------------------------------------*/
IMPL_LINK( SvxNumOptionsTabPage, BulletHdl_Impl, Button *, EMPTYARG )
{
	SvxCharacterMap* pMap = new SvxCharacterMap( this, TRUE );

	USHORT nMask = 1;
	const Font* pFmtFont = 0;
	BOOL bSameBullet = TRUE;
	sal_Unicode cBullet = 0;
	BOOL bFirst = TRUE;
	for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
	{
		if(nActNumLvl & nMask)
		{
			const SvxNumberFormat& 	rCurFmt = pActNum->GetLevel(i);
			if(bFirst)
			{
				 cBullet = rCurFmt.GetBulletChar();
			}
			else if(rCurFmt.GetBulletChar() != cBullet )
			{
				bSameBullet = FALSE;
				break;
			}
			if(!pFmtFont)
				pFmtFont = rCurFmt.GetBulletFont();
			bFirst = FALSE;
		}
		nMask <<= 1;

	}

	if(pFmtFont)
		pMap->SetCharFont(*pFmtFont);
	else
		pMap->SetCharFont(aActBulletFont);
	if(bSameBullet)
		pMap->SetChar( cBullet );
	if(pMap->Execute() == RET_OK)
	{
		// Font Numrules umstellen
		aActBulletFont = pMap->GetCharFont();

		USHORT _nMask = 1;
		for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
		{
			if(nActNumLvl & _nMask)
			{
				SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
				aNumFmt.SetBulletFont(&aActBulletFont); ;
				aNumFmt.SetBulletChar( (sal_Unicode) pMap->GetChar() );
				pActNum->SetLevel(i, aNumFmt);
			}
			_nMask <<= 1;
		}

		SetModified();
	}
	delete pMap;
	return 0;
}

/*-----------------03.03.97 15:21-------------------

--------------------------------------------------*/

IMPL_LINK( SvxNumOptionsTabPage, SizeHdl_Impl, MetricField *, pField)
{
	BOOL bWidth = pField == &aWidthMF;
	bLastWidthModified = bWidth;
	BOOL bRatio = aRatioCB.IsChecked();
	long nWidthVal = static_cast<long>(aWidthMF.Denormalize(aWidthMF.GetValue(FUNIT_100TH_MM)));
	long nHeightVal = static_cast<long>(aHeightMF.Denormalize(aHeightMF.GetValue(FUNIT_100TH_MM)));
	nWidthVal = OutputDevice::LogicToLogic( nWidthVal ,
												MAP_100TH_MM, (MapUnit)eCoreUnit );
	nHeightVal = OutputDevice::LogicToLogic( nHeightVal,
												MAP_100TH_MM, (MapUnit)eCoreUnit);
	double	fSizeRatio;

	BOOL bRepaint = FALSE;
	USHORT nMask = 1;
	for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
	{
		if(nActNumLvl & nMask)
		{
			SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
			if(SVX_NUM_BITMAP == (aNumFmt.GetNumberingType()&(~LINK_TOKEN)))
			{
				Size aSize(aNumFmt.GetGraphicSize() );
				Size aSaveSize(aSize);

				if (aInitSize[i].Height())
					fSizeRatio = (double)aInitSize[i].Width() / (double)aInitSize[i].Height();
				else
					fSizeRatio = (double)1;

				if(bWidth)
				{
					long nDelta = nWidthVal - aInitSize[i].Width();
					aSize.Width() = nWidthVal;
					if (bRatio)
					{
						aSize.Height() = aInitSize[i].Height() + (long)((double)nDelta / fSizeRatio);
						aHeightMF.SetUserValue(aHeightMF.Normalize(
							OutputDevice::LogicToLogic( aSize.Height(), (MapUnit)eCoreUnit, MAP_100TH_MM )),
								FUNIT_100TH_MM);
					}
				}
				else
				{
					long nDelta = nHeightVal - aInitSize[i].Height();
					aSize.Height() = nHeightVal;
					if (bRatio)
					{
						aSize.Width() = aInitSize[i].Width() + (long)((double)nDelta * fSizeRatio);
						aWidthMF.SetUserValue(aWidthMF.Normalize(
							OutputDevice::LogicToLogic( aSize.Width(), (MapUnit)eCoreUnit, MAP_100TH_MM )),
								FUNIT_100TH_MM);
					}
				}
				const SvxBrushItem* pBrushItem =  aNumFmt.GetBrush();
                sal_Int16 eOrient = aNumFmt.GetVertOrient();
				if(aSize != aSaveSize)
					bRepaint = TRUE;
				aNumFmt.SetGraphicBrush( pBrushItem, &aSize, &eOrient );
				pActNum->SetLevel(i, aNumFmt);
			}
		}
		nMask <<= 1;
	}
	SetModified(bRepaint);
	return 0;
}

/*------------------------------------------------------------------------
 Beschreibung:
------------------------------------------------------------------------*/

IMPL_LINK( SvxNumOptionsTabPage, RatioHdl_Impl, CheckBox *, pBox )
{
	if (pBox->IsChecked())
	{
		if (bLastWidthModified)
			SizeHdl_Impl(&aWidthMF);
		else
			SizeHdl_Impl(&aHeightMF);
	}
	return 0;
}

/*-----------------02.12.97 16:07-------------------

--------------------------------------------------*/
IMPL_LINK( SvxNumOptionsTabPage, CharFmtHdl_Impl, ListBox *, EMPTYARG )
{
	bAutomaticCharStyles = FALSE;
	USHORT nEntryPos = aCharFmtLB.GetSelectEntryPos();
	String sEntry = aCharFmtLB.GetSelectEntry();
	USHORT nMask = 1;
	String aEmptyStr;
	for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
	{
		if(nActNumLvl & nMask)
		{
			SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
			if( 0 == nEntryPos )
				aNumFmt.SetCharFmtName(aEmptyStr);
			else
			{
				if(SVX_NUM_BITMAP != (aNumFmt.GetNumberingType()&(~LINK_TOKEN)))
					aNumFmt.SetCharFmtName(sEntry);
			}
			pActNum->SetLevel(i, aNumFmt);
		}
		nMask <<= 1;
	}
	SetModified(FALSE);
	return 0;

};

/*-----------------03.12.97 11:01-------------------

--------------------------------------------------*/
IMPL_LINK( SvxNumOptionsTabPage, EditModifyHdl_Impl, Edit *, pEdit )
{
	BOOL bPrefix = pEdit == &aPrefixED;
	BOOL bSuffix = pEdit == &aSuffixED;
	BOOL bStart = pEdit == &aStartED;
	USHORT nMask = 1;
	for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
	{
		if(nActNumLvl & nMask)
		{
			SvxNumberFormat aNumFmt(pActNum->GetLevel(i));
			if(bPrefix)
				aNumFmt.SetPrefix( aPrefixED.GetText() );
			else if(bSuffix)
				aNumFmt.SetSuffix( aSuffixED.GetText() );
			else if(bStart)
				aNumFmt.SetStart( (USHORT)aStartED.GetValue() );
			else //align
			{
				USHORT nPos = aAlignLB.GetSelectEntryPos();
				SvxAdjust eAdjust = SVX_ADJUST_CENTER;
				if(nPos == 0)
					eAdjust = SVX_ADJUST_LEFT;
				else if(nPos == 2)
					eAdjust = SVX_ADJUST_RIGHT;
				aNumFmt.SetNumAdjust( eAdjust );
			}
			pActNum->SetLevel(i, aNumFmt);
		}
		nMask <<= 1;
	}
	SetModified();

	return 0;
}

/*-----------------09.12.97 11:49-------------------

--------------------------------------------------*/
USHORT lcl_DrawGraphic(VirtualDevice* pVDev, const SvxNumberFormat &rFmt, USHORT nXStart,
						USHORT nYStart, USHORT nDivision)
{
	const SvxBrushItem* pBrushItem = rFmt.GetBrush();
	USHORT nRet = 0;
	if(pBrushItem)
	{
		const Graphic* pGrf = pBrushItem->GetGraphic();
		if(pGrf)
		{
			Size aGSize( rFmt.GetGraphicSize() );
			aGSize.Width() /= nDivision;
			nRet = (USHORT)aGSize.Width();
			aGSize.Height() /= nDivision;
			pGrf->Draw( pVDev, Point(nXStart,nYStart),
					pVDev->PixelToLogic( aGSize ) );
		}
	}
	return nRet;

}

/*-----------------09.12.97 11:54-------------------

--------------------------------------------------*/
USHORT lcl_DrawBullet(VirtualDevice* pVDev,
			const SvxNumberFormat& rFmt, USHORT nXStart,
			USHORT nYStart, const Size& rSize)
{
	Font aTmpFont(pVDev->GetFont());

	//per Uno kann es sein, dass kein Font gesetzt ist!
	Font aFont(rFmt.GetBulletFont() ? *rFmt.GetBulletFont() : aTmpFont);
	Size aTmpSize(rSize);
	aTmpSize.Width() *= rFmt.GetBulletRelSize();
	aTmpSize.Width() /= 100 ;
	aTmpSize.Height() *= rFmt.GetBulletRelSize();
	aTmpSize.Height() /= 100 ;
	// bei einer Hoehe von Null wird in Ursprungshoehe gezeichnet
	if(!aTmpSize.Height())
		aTmpSize.Height() = 1;
	aFont.SetSize(aTmpSize);
	aFont.SetTransparent(TRUE);
	Color aBulletColor = rFmt.GetBulletColor();
    if(aBulletColor.GetColor() == COL_AUTO)
        aBulletColor = Color(pVDev->GetFillColor().IsDark() ? COL_WHITE : COL_BLACK);
    else if(aBulletColor == pVDev->GetFillColor())
        aBulletColor.Invert();
    aFont.SetColor(aBulletColor);
	pVDev->SetFont( aFont );
	String aText(sal_Unicode(rFmt.GetBulletChar()));
	long nY = nYStart;
	nY -= ((aTmpSize.Height() - rSize.Height())/ 2);
	pVDev->DrawText( Point(nXStart, nY), aText );
	USHORT nRet = (USHORT)pVDev->GetTextWidth(aText);

	pVDev->SetFont(aTmpFont);
	return nRet;
}
/*-----------------02.12.97 10:34-------------------
	Vorschau der Numerierung painten
--------------------------------------------------*/
void	SvxNumberingPreview::Paint( const Rectangle& /*rRect*/ )
{
	Size aSize(PixelToLogic(GetOutputSizePixel()));
	Rectangle aRect(Point(0,0), aSize);

    const StyleSettings& rStyleSettings = GetSettings().GetStyleSettings();
    const Color aBackColor = rStyleSettings.GetFieldColor();
    const Color aTextColor = rStyleSettings.GetFieldTextColor();

	VirtualDevice* pVDev = new VirtualDevice(*this);
	pVDev->EnableRTL( IsRTLEnabled() );
	pVDev->SetMapMode(GetMapMode());
	pVDev->SetOutputSize( aSize );

    Color aLineColor(COL_LIGHTGRAY);
    if(aLineColor == aBackColor)
        aLineColor.Invert();
    pVDev->SetLineColor(aLineColor);
    pVDev->SetFillColor( aBackColor );
	pVDev->DrawRect(aRect);

	if(pActNum)
	{
		USHORT nWidthRelation;
		if(nPageWidth)
		{
			nWidthRelation = USHORT (nPageWidth / aSize.Width());
			if(bPosition)
				nWidthRelation = nWidthRelation * 2 / 3;
			else
				nWidthRelation = nWidthRelation / 4;
		}
		else
			nWidthRelation = 30; // Kapiteldialog

		//Hoehe pro Ebene
		USHORT nXStep = sal::static_int_cast< USHORT >(aSize.Width() / (3 * pActNum->GetLevelCount()));
		if(pActNum->GetLevelCount() < 10)
			nXStep /= 2;
		USHORT nYStart = 4;
		// fuer ein einziges Level darf nicht die gesamte Hoehe benutzt werden
		USHORT nYStep = sal::static_int_cast< USHORT >((aSize.Height() - 6)/ (pActNum->GetLevelCount() > 1 ? pActNum->GetLevelCount() : 5));
        aStdFont = OutputDevice::GetDefaultFont(
                DEFAULTFONT_UI_SANS, MsLangId::getSystemLanguage(), DEFAULTFONT_FLAGS_ONLYONE);
		aStdFont.SetColor(aTextColor);
		aStdFont.SetFillColor(aBackColor);

		//
		USHORT nFontHeight = nYStep * 6 / 10;
		if(bPosition)
			nFontHeight = nYStep * 15 / 10;
		aStdFont.SetSize(Size( 0, nFontHeight ));

		SvxNodeNum aNum( (BYTE)0 );
		USHORT nPreNum = pActNum->GetLevel(0).GetStart();

		if(bPosition)
		{
			USHORT nLineHeight = nFontHeight * 8 / 7;
			BYTE nStart = 0;
			while( !(nActLevel & (1<<nStart)) )
			{
				nStart++;
			}
			if(nStart)
				nStart--;
			BYTE nEnd = std::min( (BYTE)(nStart + 3), (BYTE)pActNum->GetLevelCount() );
			for( BYTE nLevel = nStart; nLevel < nEnd; ++nLevel )
			{
				const SvxNumberFormat &rFmt = pActNum->GetLevel(nLevel);
				aNum.GetLevelVal()[ nLevel ] = rFmt.GetStart();

                // --> OD 2008-01-16 #newlistlevelattrs#
                USHORT nXStart( 0 );
                short nTextOffset( 0 );
                USHORT nNumberXPos( 0 );
                if ( rFmt.GetPositionAndSpaceMode() == SvxNumberFormat::LABEL_WIDTH_AND_POSITION )
                {
                    nXStart = rFmt.GetAbsLSpace() / nWidthRelation;
                    nTextOffset = rFmt.GetCharTextDistance() / nWidthRelation;
                    nNumberXPos = nXStart;
                    USHORT nFirstLineOffset = (-rFmt.GetFirstLineOffset()) / nWidthRelation;

                    if(nFirstLineOffset <= nNumberXPos)
                        nNumberXPos = nNumberXPos - nFirstLineOffset;
                    else
                        nNumberXPos = 0;
                    //im draw ist das zulaeesig
                    if(nTextOffset < 0)
                        nNumberXPos = nNumberXPos + nTextOffset;
                }
                else if ( rFmt.GetPositionAndSpaceMode() == SvxNumberFormat::LABEL_ALIGNMENT )
                {
                    const long nTmpNumberXPos( ( rFmt.GetIndentAt() +
                                                 rFmt.GetFirstLineIndent() ) /
                                               nWidthRelation );
                    if ( nTmpNumberXPos < 0 )
                    {
                        nNumberXPos = 0;
                    }
                    else
                    {
                        nNumberXPos = static_cast<USHORT>( nTmpNumberXPos );
                    }
                }
                // <--

				USHORT nBulletWidth = 0;
				if( SVX_NUM_BITMAP == (rFmt.GetNumberingType() &(~LINK_TOKEN)))
				{
					nBulletWidth = rFmt.IsShowSymbol() ? lcl_DrawGraphic(pVDev, rFmt,
										nNumberXPos,
											nYStart, nWidthRelation) : 0;
				}
				else if( SVX_NUM_CHAR_SPECIAL == rFmt.GetNumberingType() )
				{
					nBulletWidth =  rFmt.IsShowSymbol() ?
					 lcl_DrawBullet(pVDev, rFmt, nNumberXPos, nYStart, aStdFont.GetSize()) : 0;
				}
				else
				{
					pVDev->SetFont(aStdFont);
					aNum.SetLevel( nLevel );
					if(pActNum->IsContinuousNumbering())
						aNum.GetLevelVal()[nLevel] = nPreNum;
					String aText(pActNum->MakeNumString( aNum ));
					Font aSaveFont = pVDev->GetFont();
					Font aColorFont(aSaveFont);
                    Color aTmpBulletColor = rFmt.GetBulletColor();
                    if(aTmpBulletColor.GetColor() == COL_AUTO)
                        aTmpBulletColor = Color(aBackColor.IsDark() ? COL_WHITE : COL_BLACK);
                    else if(aTmpBulletColor == aBackColor)
                        aTmpBulletColor.Invert();
                    aColorFont.SetColor(aTmpBulletColor);
					pVDev->SetFont(aColorFont);
					pVDev->DrawText( Point(nNumberXPos, nYStart), aText );
					pVDev->SetFont(aSaveFont);
					nBulletWidth = (USHORT)pVDev->GetTextWidth(aText);
					nPreNum++;
				}
                // --> OD 2008-01-16 #newlistlevelattrs#
                if ( rFmt.GetPositionAndSpaceMode() == SvxNumberFormat::LABEL_ALIGNMENT &&
                     rFmt.GetLabelFollowedBy() == SvxNumberFormat::SPACE )
                {
                    pVDev->SetFont(aStdFont);
                    String aText(' ');
                    pVDev->DrawText( Point(nNumberXPos, nYStart), aText );
                    nBulletWidth = nBulletWidth + (USHORT)pVDev->GetTextWidth(aText);
                }

                // --> OD 2008-01-16 #newlistlevelattrs#
                USHORT nTextXPos( 0 );
                if ( rFmt.GetPositionAndSpaceMode() == SvxNumberFormat::LABEL_WIDTH_AND_POSITION )
                {
                    nTextXPos = nXStart;
                    if(nTextOffset < 0)
                         nTextXPos = nTextXPos + nTextOffset;
                    if(nNumberXPos + nBulletWidth + nTextOffset > nTextXPos )
                        nTextXPos = nNumberXPos + nBulletWidth + nTextOffset;
                }
                else if ( rFmt.GetPositionAndSpaceMode() == SvxNumberFormat::LABEL_ALIGNMENT )
                {
                    switch ( rFmt.GetLabelFollowedBy() )
                    {
                        case SvxNumberFormat::LISTTAB:
                        {
                            nTextXPos = static_cast<USHORT>(
                                            rFmt.GetListtabPos() / nWidthRelation );
                            if ( nTextXPos < nNumberXPos + nBulletWidth )
                            {
                                nTextXPos = nNumberXPos + nBulletWidth;
                            }
                        }
                        break;
                        case SvxNumberFormat::SPACE:
                        case SvxNumberFormat::NOTHING:
                        {
                            nTextXPos = nNumberXPos + nBulletWidth;
                        }
                        break;
                    }

                    nXStart = static_cast<USHORT>( rFmt.GetIndentAt() / nWidthRelation );
                }
                // <--

				Rectangle aRect1(Point(nTextXPos, nYStart + nFontHeight / 2), Size(aSize.Width() / 2, 2));
                pVDev->SetFillColor( aBackColor );
				pVDev->DrawRect( aRect1 );

				Rectangle aRect2(Point(nXStart, nYStart + nLineHeight + nFontHeight / 2 ), Size(aSize.Width() / 2, 2));
				pVDev->DrawRect( aRect2 );
				nYStart += 2 * nLineHeight;
			}
		}
		else
		{
            //#i5153# painting gray or black rectangles as 'normal' numbering text
            String sMsg( RTL_CONSTASCII_USTRINGPARAM( "Preview") );
            long nWidth = pVDev->GetTextWidth(sMsg);
            long nTextHeight = pVDev->GetTextHeight();
            long nRectHeight = nTextHeight * 2 / 3;
            long nTopOffset = nTextHeight - nRectHeight;
            Color aBlackColor(COL_BLACK);
            if(aBlackColor == aBackColor)
                aBlackColor.Invert();

			for( BYTE nLevel = 0; nLevel < pActNum->GetLevelCount();
							++nLevel, nYStart = nYStart + nYStep )
			{
				const SvxNumberFormat &rFmt = pActNum->GetLevel(nLevel);
				aNum.GetLevelVal()[ nLevel ] = rFmt.GetStart();
                // --> OD 2008-01-31 #newlistlevelattrs#
                USHORT nXStart( 0 );
                if ( rFmt.GetPositionAndSpaceMode() == SvxNumberFormat::LABEL_WIDTH_AND_POSITION )
                {
                    nXStart = rFmt.GetAbsLSpace() / nWidthRelation;
                }
                else if ( rFmt.GetPositionAndSpaceMode() == SvxNumberFormat::LABEL_ALIGNMENT )
                {
                    const long nTmpXStart( ( rFmt.GetIndentAt() +
                                             rFmt.GetFirstLineIndent() ) /
                                           nWidthRelation );
                    if ( nTmpXStart < 0 )
                    {
                        nXStart = 0;
                    }
                    else
                    {
                        nXStart = static_cast<USHORT>(nTmpXStart);
                    }
                }
                nXStart /= 2;
                nXStart += 2;
                // <--
				USHORT nTextOffset = 2 * nXStep;
				if( SVX_NUM_BITMAP == (rFmt.GetNumberingType()&(~LINK_TOKEN)) )
				{
					if(rFmt.IsShowSymbol())
					{
						nTextOffset = lcl_DrawGraphic(pVDev, rFmt, nXStart, nYStart, nWidthRelation);
						nTextOffset = nTextOffset + nXStep;
					}
				}
				else if( SVX_NUM_CHAR_SPECIAL == rFmt.GetNumberingType() )
				{
					if(rFmt.IsShowSymbol())
					{
						nTextOffset =  lcl_DrawBullet(pVDev, rFmt, nXStart, nYStart, aStdFont.GetSize());
						nTextOffset = nTextOffset + nXStep;
					}
				}
				else
				{
					Font aColorFont(aStdFont);
                    Color aTmpBulletColor = rFmt.GetBulletColor();
                    if(aTmpBulletColor.GetColor() == COL_AUTO)
                        aTmpBulletColor = Color(aBackColor.IsDark() ? COL_WHITE : COL_BLACK);
                    else if(aTmpBulletColor == aBackColor)
                        aTmpBulletColor.Invert();
                    aColorFont.SetColor(aTmpBulletColor);
					pVDev->SetFont(aColorFont);
					aNum.SetLevel( nLevel );
					if(pActNum->IsContinuousNumbering())
						aNum.GetLevelVal()[nLevel] = nPreNum;
					String aText(pActNum->MakeNumString( aNum ));
					pVDev->DrawText( Point(nXStart, nYStart), aText );
					pVDev->SetFont(aStdFont);
					nTextOffset = (USHORT)pVDev->GetTextWidth(aText);
					nTextOffset = nTextOffset + nXStep;
					nPreNum++;
				}
                if(pOutlineNames)
                {
                    //#i5153# outline numberings still use the style names as text
                    pVDev->SetFont(aStdFont);
                    sMsg = pOutlineNames[nLevel];
                    pVDev->DrawText( Point(nXStart + nTextOffset, nYStart), sMsg );
                }
                else
                {
                    //#i5153# the selected rectangle(s) should be black
                    if( 0 != (nActLevel & (1<<nLevel)))
                    {
                        pVDev->SetFillColor( aBlackColor );
                        pVDev->SetLineColor( aBlackColor );
                    }
                    else
                    {
                        //#i5153# unselected levels are gray
                        pVDev->SetFillColor( aLineColor );
                        pVDev->SetLineColor( aLineColor );
                    }
                    Rectangle aRect1(Point(nXStart + nTextOffset, nYStart + nTopOffset), Size(nWidth, nRectHeight));
                    pVDev->DrawRect(aRect1);
                }
			}
		}
	}
	DrawOutDev(	Point(0,0), aSize,
				Point(0,0), aSize,
						*pVDev );
	delete pVDev;

}

/*-----------------03.12.97 10:02-------------------

--------------------------------------------------*/
SvxNumPositionTabPage::SvxNumPositionTabPage(Window* pParent,
							   const SfxItemSet& rSet) :
	SfxTabPage( pParent, SVX_RES( RID_SVXPAGE_NUM_POSITION ), rSet ),
    aPositionFL(    this, SVX_RES(FL_POSITION )),
    aLevelFT(       this, SVX_RES(FT_LEVEL    )),
	aLevelLB(		this, SVX_RES(LB_LEVEL	)),
	aDistBorderFT(	this, SVX_RES(FT_BORDERDIST	)),
	aDistBorderMF(	this, SVX_RES(MF_BORDERDIST	)),
	aRelativeCB(	this, SVX_RES(CB_RELATIVE		)),
	aIndentFT(		this, SVX_RES(FT_INDENT		)),
	aIndentMF(		this, SVX_RES(MF_INDENT		)),
	aDistNumFT(		this, SVX_RES(FT_NUMDIST		)),
	aDistNumMF(		this, SVX_RES(MF_NUMDIST		)),
	aAlignFT(		this, SVX_RES(FT_ALIGN	)),
	aAlignLB(		this, SVX_RES(LB_ALIGN	)),
    // --> OD 2008-01-10 #newlistlevelattrs#
    aLabelFollowedByFT( this, SVX_RES(FT_LABEL_FOLLOWED_BY) ),
    aLabelFollowedByLB( this, SVX_RES(LB_LABEL_FOLLOWED_BY) ),
    aListtabFT( this, SVX_RES(FT_LISTTAB) ),
    aListtabMF( this, SVX_RES(MF_LISTTAB) ),
    aAlign2FT( this, SVX_RES(FT_ALIGN_2) ),
    aAlign2LB( this, SVX_RES(LB_ALIGN_2) ),
    aAlignedAtFT( this, SVX_RES(FT_ALIGNED_AT) ),
    aAlignedAtMF( this, SVX_RES(MF_ALIGNED_AT) ),
    aIndentAtFT( this, SVX_RES(FT_INDENT_AT) ),
    aIndentAtMF( this, SVX_RES(MF_INDENT_AT) ),
    // <--
    aStandardPB(    this, SVX_RES(PB_STANDARD       )),
	pPreviewWIN(	new SvxNumberingPreview(this, SVX_RES(WIN_PREVIEW	))),
	pActNum(0),
	pSaveNum(0),
	nActNumLvl( USHRT_MAX ),
	nNumItemId(SID_ATTR_NUMBERING_RULE),
        bModified(false),
        bPreset(false),
    bInInintControl(FALSE),
    // --> OD 2008-01-11 #newlistlevelattrs#
    bLabelAlignmentPosAndSpaceModeActive( false )
    // <--
{
	FreeResource();
	SetExchangeSupport();
	pPreviewWIN->SetBackground(Wallpaper(Color(COL_TRANSPARENT)));

	aRelativeCB.Check();
	aAlignLB.SetSelectHdl(LINK(this, SvxNumPositionTabPage, EditModifyHdl_Impl));
    // --> OD 2008-01-10 #newlistlevelattrs#
    aAlign2LB.SetSelectHdl(LINK(this, SvxNumPositionTabPage, EditModifyHdl_Impl));
    for ( USHORT i = 0; i < aAlignLB.GetEntryCount(); ++i )
    {
        aAlign2LB.InsertEntry( aAlignLB.GetEntry( i ) );
    }
    aAlign2LB.SetDropDownLineCount( aAlign2LB.GetEntryCount() );
    aAlign2FT.SetText( aAlignFT.GetText() );
    // <--

	Link aLk = LINK(this, SvxNumPositionTabPage, DistanceHdl_Impl);

	aDistBorderMF.SetUpHdl(aLk);
	aDistBorderMF.SetDownHdl(aLk);
	aDistBorderMF.SetLoseFocusHdl(aLk);

	aDistNumMF.SetUpHdl(aLk);
	aDistNumMF.SetDownHdl(aLk);
	aDistNumMF.SetLoseFocusHdl(aLk);

	aIndentMF.SetUpHdl(aLk);
	aIndentMF.SetDownHdl(aLk);
	aIndentMF.SetLoseFocusHdl(aLk);

    // --> OD 2008-01-10 #newlistlevelattrs#
    aLabelFollowedByLB.SetDropDownLineCount( aLabelFollowedByLB.GetEntryCount() );
    aLabelFollowedByLB.SetSelectHdl( LINK(this, SvxNumPositionTabPage, LabelFollowedByHdl_Impl) );

    aLk = LINK(this, SvxNumPositionTabPage, ListtabPosHdl_Impl);
    aListtabMF.SetUpHdl(aLk);
    aListtabMF.SetDownHdl(aLk);
    aListtabMF.SetLoseFocusHdl(aLk);

    aLk = LINK(this, SvxNumPositionTabPage, AlignAtHdl_Impl);
    aAlignedAtMF.SetUpHdl(aLk);
    aAlignedAtMF.SetDownHdl(aLk);
    aAlignedAtMF.SetLoseFocusHdl(aLk);

    aLk = LINK(this, SvxNumPositionTabPage, IndentAtHdl_Impl);
    aIndentAtMF.SetUpHdl(aLk);
    aIndentAtMF.SetDownHdl(aLk);
    aIndentAtMF.SetLoseFocusHdl(aLk);
    // <--

	aLevelLB.SetSelectHdl(LINK(this, SvxNumPositionTabPage, LevelHdl_Impl));
	aRelativeCB.SetClickHdl(LINK(this, SvxNumPositionTabPage, RelativeHdl_Impl));
	aStandardPB.SetClickHdl(LINK(this, SvxNumPositionTabPage, StandardHdl_Impl));


	aRelativeCB.Check(bLastRelative);
	pPreviewWIN->SetPositionMode();
	eCoreUnit = rSet.GetPool()->GetMetric(rSet.GetPool()->GetWhich(SID_ATTR_NUMBERING_RULE));

    //HACK("der Wert sollte mal sinnvol gesetzt werden")
    long nWidth = 10000;
    nWidth = OutputDevice::LogicToLogic( nWidth,
                                            (MapUnit)eCoreUnit, MAP_100TH_MM );

    aDistBorderMF.SetMax(aDistBorderMF.Normalize( nWidth ), FUNIT_100TH_MM );
    aDistNumMF   .SetMax(aDistNumMF   .Normalize( nWidth ), FUNIT_100TH_MM );
    aIndentMF    .SetMax(aIndentMF    .Normalize( nWidth ), FUNIT_100TH_MM );
    // --> OD 2008-02-18 #newlistlevelattrs#
    aListtabMF.SetMax(aListtabMF.Normalize( nWidth ), FUNIT_100TH_MM );
    aAlignedAtMF.SetMax(aAlignedAtMF.Normalize( nWidth ), FUNIT_100TH_MM );
    aIndentAtMF.SetMax(aIndentAtMF.Normalize( nWidth ), FUNIT_100TH_MM );
    // <--
    long nLast2 = nWidth /2;
    aDistBorderMF.SetLast( aDistBorderMF.Normalize(   nLast2 ), FUNIT_100TH_MM );
    aDistNumMF   .SetLast( aDistNumMF     .Normalize( nLast2 ), FUNIT_100TH_MM );
    aIndentMF    .SetLast( aIndentMF      .Normalize( nLast2 ), FUNIT_100TH_MM );
    // --> OD 2008-02-18 #newlistlevelattrs#
    aListtabMF.SetLast(aListtabMF.Normalize( nLast2 ), FUNIT_100TH_MM );
    aAlignedAtMF.SetLast(aAlignedAtMF.Normalize( nLast2 ), FUNIT_100TH_MM );
    aIndentAtMF.SetLast(aIndentAtMF.Normalize( nLast2 ), FUNIT_100TH_MM );
    // <--

#if OSL_DEBUG_LEVEL > 1
	pDebugFixedText = new FixedText(this, 0);
	pDebugFixedText->Show();
	Size aSize(200, 20);
	Point aPos(250,0);

	pDebugFixedText->SetPosSizePixel(aPos, aSize);
	pDebugFixedText->SetText( UniString::CreateFromAscii( RTL_CONSTASCII_STRINGPARAM( "Das ist ein Debug-Text" ) ) );
#endif
}
/*-----------------03.12.97 10:02-------------------

--------------------------------------------------*/
SvxNumPositionTabPage::~SvxNumPositionTabPage()
{
	delete pActNum;
	delete pPreviewWIN;
	delete pSaveNum;
#if OSL_DEBUG_LEVEL > 1
	delete pDebugFixedText;
#endif
}
/*-------------------------------------------------------*/

#if OSL_DEBUG_LEVEL > 1
void lcl_PrintDebugOutput(FixedText& rFixed, const SvxNumberFormat& rNumFmt)
{
#define TWIP_TO_MM100(TWIP) 	((TWIP) >= 0 ? (((TWIP)*127L+36L)/72L) : (((TWIP)*127L-36L)/72L))

	sal_Char const sHash[] = " # ";
    if ( rNumFmt.GetPositionAndSpaceMode() == SvxNumberFormat::LABEL_WIDTH_AND_POSITION )
    {
        String sDebugText( UniString::CreateFromInt32( TWIP_TO_MM100(rNumFmt.GetAbsLSpace() ) ) );
        sDebugText.AppendAscii( RTL_CONSTASCII_STRINGPARAM( sHash ) );
        sDebugText += UniString::CreateFromInt32( TWIP_TO_MM100(rNumFmt.GetCharTextDistance() ) );
        sDebugText.AppendAscii( RTL_CONSTASCII_STRINGPARAM( sHash ) );
        sDebugText += UniString::CreateFromInt32( TWIP_TO_MM100(rNumFmt.GetFirstLineOffset() ) );
        rFixed.SetText(sDebugText);
    }
    else if ( rNumFmt.GetPositionAndSpaceMode() == SvxNumberFormat::LABEL_ALIGNMENT )
    {
        String sDebugText( UniString::CreateFromInt32( TWIP_TO_MM100(rNumFmt.GetListtabPos() ) ) );
        sDebugText.AppendAscii( RTL_CONSTASCII_STRINGPARAM( sHash ) );
        sDebugText += UniString::CreateFromInt32( TWIP_TO_MM100(rNumFmt.GetFirstLineIndent() ) );
        sDebugText.AppendAscii( RTL_CONSTASCII_STRINGPARAM( sHash ) );
        sDebugText += UniString::CreateFromInt32( TWIP_TO_MM100(rNumFmt.GetIndentAt() ) );
        rFixed.SetText(sDebugText);
    }

}
#endif

/*-----------------03.12.97 10:06-------------------

--------------------------------------------------*/
void SvxNumPositionTabPage::InitControls()
{
	bInInintControl	= TRUE;
    // --> OD 2008-01-11 #newlistlevelattrs#
    const bool bRelative = !bLabelAlignmentPosAndSpaceModeActive &&
                     aRelativeCB.IsEnabled() && aRelativeCB.IsChecked();
    // <--
    const bool bSingleSelection = aLevelLB.GetSelectEntryCount() == 1 &&
                            USHRT_MAX != nActNumLvl;

    aDistBorderMF.Enable( !bLabelAlignmentPosAndSpaceModeActive &&
                          ( bSingleSelection || bRelative ) );
    aDistBorderFT.Enable( !bLabelAlignmentPosAndSpaceModeActive &&
                          ( bSingleSelection || bRelative ) );

    bool bSetDistEmpty = false;
    bool bSameDistBorderNum = !bLabelAlignmentPosAndSpaceModeActive;
    bool bSameDist      = !bLabelAlignmentPosAndSpaceModeActive;
    bool bSameIndent    = !bLabelAlignmentPosAndSpaceModeActive;
    bool bSameAdjust    = true;

    // --> OD 2008-01-11 #newlistlevelattrs#
    bool bSameLabelFollowedBy = bLabelAlignmentPosAndSpaceModeActive;
    bool bSameListtab = bLabelAlignmentPosAndSpaceModeActive;
    bool bSameAlignAt = bLabelAlignmentPosAndSpaceModeActive;
    bool bSameIndentAt = bLabelAlignmentPosAndSpaceModeActive;
    // <--

	const SvxNumberFormat* aNumFmtArr[SVX_MAX_NUM];
	USHORT nMask = 1;
	USHORT nLvl = USHRT_MAX;
	long nFirstBorderText = 0;
	long nFirstBorderTextRelative = -1;
	for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
	{
		aNumFmtArr[i] = &pActNum->GetLevel(i);
		if(nActNumLvl & nMask)
		{
			if(USHRT_MAX == nLvl)
			{
				nLvl = i;
                if ( !bLabelAlignmentPosAndSpaceModeActive )
                {
                    nFirstBorderText = nLvl > 0 ?
                        aNumFmtArr[nLvl]->GetAbsLSpace() + aNumFmtArr[nLvl]->GetFirstLineOffset() -
                        aNumFmtArr[nLvl - 1]->GetAbsLSpace() + aNumFmtArr[nLvl - 1]->GetFirstLineOffset():
                            aNumFmtArr[nLvl]->GetAbsLSpace() + aNumFmtArr[nLvl]->GetFirstLineOffset();
                }
			}

			if( i > nLvl)
			{
                // --> OD 2008-01-11 #newlistlevelattrs#
                bSameAdjust &= aNumFmtArr[i]->GetNumAdjust() == aNumFmtArr[nLvl]->GetNumAdjust();
                if ( !bLabelAlignmentPosAndSpaceModeActive )
                {
                    if(bRelative)
                    {
                        if(nFirstBorderTextRelative == -1)
                            nFirstBorderTextRelative =
                            (aNumFmtArr[i]->GetAbsLSpace() + aNumFmtArr[i]->GetFirstLineOffset() -
                            aNumFmtArr[i - 1]->GetAbsLSpace() + aNumFmtArr[i - 1]->GetFirstLineOffset());
                        else
                            bSameDistBorderNum &= nFirstBorderTextRelative ==
                            (aNumFmtArr[i]->GetAbsLSpace() + aNumFmtArr[i]->GetFirstLineOffset() -
                            aNumFmtArr[i - 1]->GetAbsLSpace() + aNumFmtArr[i - 1]->GetFirstLineOffset());
                    }
                    else
                        bSameDistBorderNum &=
                        aNumFmtArr[i]->GetAbsLSpace() - aNumFmtArr[i]->GetFirstLineOffset() ==
                        aNumFmtArr[i - 1]->GetAbsLSpace() - aNumFmtArr[i - 1]->GetFirstLineOffset();

                    bSameDist       &= aNumFmtArr[i]->GetCharTextDistance() == aNumFmtArr[nLvl]->GetCharTextDistance();
                    bSameIndent     &= aNumFmtArr[i]->GetFirstLineOffset() == aNumFmtArr[nLvl]->GetFirstLineOffset();
                }
                else
                {
                    bSameLabelFollowedBy &=
                        aNumFmtArr[i]->GetLabelFollowedBy() == aNumFmtArr[nLvl]->GetLabelFollowedBy();
                    bSameListtab &=
                        aNumFmtArr[i]->GetListtabPos() == aNumFmtArr[nLvl]->GetListtabPos();
                    bSameAlignAt &=
                        ( ( aNumFmtArr[i]->GetIndentAt() + aNumFmtArr[i]->GetFirstLineIndent() )
                            == ( aNumFmtArr[nLvl]->GetIndentAt() + aNumFmtArr[nLvl]->GetFirstLineIndent() ) );
                    bSameIndentAt &=
                        aNumFmtArr[i]->GetIndentAt() == aNumFmtArr[nLvl]->GetIndentAt();
                }
                // <--
			}
		}
		nMask <<= 1;

	}
	if(bSameDistBorderNum)
	{
		long nDistBorderNum;
		if(bRelative)
		{
			nDistBorderNum = (long)aNumFmtArr[nLvl]->GetAbsLSpace()+ aNumFmtArr[nLvl]->GetFirstLineOffset();
			if(nLvl)
				nDistBorderNum -= (long)aNumFmtArr[nLvl - 1]->GetAbsLSpace()+ aNumFmtArr[nLvl - 1]->GetFirstLineOffset();
		}
		else
		{
			nDistBorderNum = (long)aNumFmtArr[nLvl]->GetAbsLSpace()+ aNumFmtArr[nLvl]->GetFirstLineOffset();
		}
		SetMetricValue(aDistBorderMF, nDistBorderNum, eCoreUnit);
	}
	else
        bSetDistEmpty = true;

#if OSL_DEBUG_LEVEL > 1
	lcl_PrintDebugOutput(*pDebugFixedText, *aNumFmtArr[nLvl]);
#endif

    const String aEmptyStr;
	if(bSameDist)
		SetMetricValue(aDistNumMF, aNumFmtArr[nLvl]->GetCharTextDistance(), eCoreUnit);
	else
		aDistNumMF.SetText(aEmptyStr);
	if(bSameIndent)
		SetMetricValue(aIndentMF, - aNumFmtArr[nLvl]->GetFirstLineOffset(), eCoreUnit);
	else
		aIndentMF.SetText(aEmptyStr);

	if(bSameAdjust)
	{
		USHORT nPos = 1; // zentriert
		if(aNumFmtArr[nLvl]->GetNumAdjust() == SVX_ADJUST_LEFT)
			nPos = 0;
		else if(aNumFmtArr[nLvl]->GetNumAdjust() == SVX_ADJUST_RIGHT)
			nPos = 2;
		aAlignLB.SelectEntryPos(nPos);
        // --> OD 2008-01-11 #newlistlevelattrs#
        aAlign2LB.SelectEntryPos( nPos );
        // <--
	}
	else
	{
		aAlignLB.SetNoSelection();
        // --> OD 2008-01-11 #newlistlevelattrs#
        aAlign2LB.SetNoSelection();
        // <--
	}

    // --> OD 2008-01-11 #newlistlevelattrs#
    if ( bSameLabelFollowedBy )
    {
        USHORT nPos = 0; // LISTTAB
        if ( aNumFmtArr[nLvl]->GetLabelFollowedBy() == SvxNumberFormat::SPACE )
        {
            nPos = 1;
        }
        else if ( aNumFmtArr[nLvl]->GetLabelFollowedBy() == SvxNumberFormat::NOTHING )
        {
            nPos = 2;
        }
        aLabelFollowedByLB.SelectEntryPos( nPos );
    }
    else
    {
        aLabelFollowedByLB.SetNoSelection();
    }

    if ( aNumFmtArr[nLvl]->GetLabelFollowedBy() == SvxNumberFormat::LISTTAB )
    {
        aListtabFT.Enable( true );
        aListtabMF.Enable( true );
        if ( bSameListtab )
        {
            SetMetricValue( aListtabMF, aNumFmtArr[nLvl]->GetListtabPos(), eCoreUnit );
        }
        else
        {
            aListtabMF.SetText(aEmptyStr);
        }
    }
    else
    {
        aListtabFT.Enable( false );
        aListtabMF.Enable( false );
        aListtabMF.SetText(aEmptyStr);
    }

    if ( bSameAlignAt )
    {
        SetMetricValue( aAlignedAtMF,
                        aNumFmtArr[nLvl]->GetIndentAt() + aNumFmtArr[nLvl]->GetFirstLineIndent(),
                        eCoreUnit );
    }
    else
    {
        aAlignedAtMF.SetText(aEmptyStr);
    }

    if ( bSameIndentAt )
    {
        SetMetricValue( aIndentAtMF, aNumFmtArr[nLvl]->GetIndentAt(), eCoreUnit );
    }
    else
    {
        aIndentAtMF.SetText(aEmptyStr);
    }
    // <--

    if ( bSetDistEmpty )
		aDistBorderMF.SetText(aEmptyStr);

	bInInintControl	= FALSE;
}

/*-----------------03.12.97 10:02-------------------

--------------------------------------------------*/
void SvxNumPositionTabPage::ActivatePage(const SfxItemSet& rSet)
{
	const SfxPoolItem* pItem;
	UINT16 nTmpNumLvl = USHRT_MAX;
	const SfxItemSet* pExampleSet = GetTabDialog()->GetExampleSet();
	if(pExampleSet)
	{
		if(SFX_ITEM_SET == pExampleSet->GetItemState(SID_PARAM_NUM_PRESET, FALSE, &pItem))
			bPreset = ((const SfxBoolItem*)pItem)->GetValue();
		if(SFX_ITEM_SET == pExampleSet->GetItemState(SID_PARAM_CUR_NUM_LEVEL, FALSE, &pItem))
			nTmpNumLvl = ((const SfxUInt16Item*)pItem)->GetValue();
	}
	//
	if(SFX_ITEM_SET == rSet.GetItemState(nNumItemId, FALSE, &pItem))
	{
		delete pSaveNum;
		pSaveNum = new SvxNumRule(*((SvxNumBulletItem*)pItem)->GetNumRule());
	}
	bModified = (!pActNum->Get( 0 ) || bPreset);
	if(*pSaveNum != *pActNum ||
		nActNumLvl != nTmpNumLvl )
	{
		*pActNum = *pSaveNum;
		nActNumLvl = nTmpNumLvl;
		USHORT nMask = 1;
		aLevelLB.SetUpdateMode(FALSE);
		aLevelLB.SetNoSelection();
		aLevelLB.SelectEntryPos( pActNum->GetLevelCount(), nActNumLvl == USHRT_MAX);
		if(nActNumLvl != USHRT_MAX)
			for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
			{
				if(nActNumLvl & nMask)
					aLevelLB.SelectEntryPos( i, TRUE);
				nMask <<= 1 ;
			}
		aRelativeCB.Enable(nActNumLvl != 1);
		aLevelLB.SetUpdateMode(TRUE);

        // --> OD 2008-01-11 #newlistlevelattrs#
        InitPosAndSpaceMode();
        ShowControlsDependingOnPosAndSpaceMode();
        // <--

        InitControls();
	}
	pPreviewWIN->SetLevel(nActNumLvl);
	pPreviewWIN->Invalidate();
}

/*-----------------03.12.97 10:02-------------------

--------------------------------------------------*/
int  SvxNumPositionTabPage::DeactivatePage(SfxItemSet *_pSet)
{
	if(_pSet)
		FillItemSet(*_pSet);
	return TRUE;
}

/*-----------------03.12.97 10:02-------------------

--------------------------------------------------*/
BOOL SvxNumPositionTabPage::FillItemSet( SfxItemSet& rSet )
{
	rSet.Put(SfxUInt16Item(SID_PARAM_CUR_NUM_LEVEL, nActNumLvl));

    if(bModified && pActNum)
	{
		*pSaveNum = *pActNum;
		rSet.Put(SvxNumBulletItem( *pSaveNum ), nNumItemId);
		rSet.Put(SfxBoolItem(SID_PARAM_NUM_PRESET, FALSE));
	}
	return bModified;
}

/*-----------------03.12.97 10:02-------------------

--------------------------------------------------*/
void SvxNumPositionTabPage::Reset( const SfxItemSet& rSet )
{
	const SfxPoolItem* pItem;
	//im Draw gibt es das Item als WhichId, im Writer nur als SlotId
	SfxItemState eState = rSet.GetItemState(SID_ATTR_NUMBERING_RULE, FALSE, &pItem);
	if(eState != SFX_ITEM_SET)
	{
		nNumItemId = rSet.GetPool()->GetWhich(SID_ATTR_NUMBERING_RULE);
		eState = rSet.GetItemState(nNumItemId, FALSE, &pItem);
				
		if( eState != SFX_ITEM_SET )
		{
	        pItem = &static_cast< const SvxNumBulletItem& >( rSet.Get( nNumItemId, TRUE ) );		
	        eState = SFX_ITEM_SET;
		}

	}
	DBG_ASSERT(eState == SFX_ITEM_SET, "kein Item gefunden!");
	delete pSaveNum;
	pSaveNum = new SvxNumRule(*((SvxNumBulletItem*)pItem)->GetNumRule());

	// Ebenen einfuegen
	if(!aLevelLB.GetEntryCount())
	{
		for(USHORT i = 1; i <= pSaveNum->GetLevelCount(); i++)
			aLevelLB.InsertEntry(UniString::CreateFromInt32(i));
		if(pSaveNum->GetLevelCount() > 1)
		{
			String sEntry( UniString::CreateFromAscii( RTL_CONSTASCII_STRINGPARAM( "1 - ") ) );
			sEntry.Append( UniString::CreateFromInt32( pSaveNum->GetLevelCount() ) );
			aLevelLB.InsertEntry(sEntry);
			aLevelLB.SelectEntry(sEntry);
		}
		else
			aLevelLB.SelectEntryPos(0);
	}
	else
		aLevelLB.SelectEntryPos(aLevelLB.GetEntryCount() - 1);
	USHORT nMask = 1;
	aLevelLB.SetUpdateMode(FALSE);
	aLevelLB.SetNoSelection();
	if(nActNumLvl == USHRT_MAX)
	{
		aLevelLB.SelectEntryPos( pSaveNum->GetLevelCount(), TRUE);
	}
	else
		for(USHORT i = 0; i < pSaveNum->GetLevelCount(); i++)
		{
			if(nActNumLvl & nMask)
				aLevelLB.SelectEntryPos( i, TRUE);
			nMask <<= 1;
		}
	aLevelLB.SetUpdateMode(TRUE);

	if(!pActNum)
		pActNum = new  SvxNumRule(*pSaveNum);
	else if(*pSaveNum != *pActNum)
		*pActNum = *pSaveNum;
	pPreviewWIN->SetNumRule(pActNum);

    // --> OD 2008-01-11 #newlistlevelattrs#
    InitPosAndSpaceMode();
    ShowControlsDependingOnPosAndSpaceMode();
//    const BOOL bDraw = pActNum->IsFeatureSupported(NUM_CONTINUOUS);
//    aDistNumFT.Show(bDraw);
//    aDistNumMF.Show(bDraw);
    // <--

	InitControls();
	bModified = FALSE;
}

// --> OD 2008-01-11 #newlistlevelattrs#
void SvxNumPositionTabPage::InitPosAndSpaceMode()
{
    if ( pActNum == 0 )
    {
        DBG_ASSERT( false,
                "<SvxNumPositionTabPage::InitPosAndSpaceMode()> - misusage of method -> <pAktNum> has to be already set!" );
        return;
    }

    SvxNumberFormat::SvxNumPositionAndSpaceMode ePosAndSpaceMode =
                                            SvxNumberFormat::LABEL_ALIGNMENT;
    USHORT nMask = 1;
    for( USHORT i = 0; i < pActNum->GetLevelCount(); ++i )
    {
        if(nActNumLvl & nMask)
        {
            SvxNumberFormat aNumFmt( pActNum->GetLevel(i) );
            ePosAndSpaceMode = aNumFmt.GetPositionAndSpaceMode();
            if ( ePosAndSpaceMode == SvxNumberFormat::LABEL_ALIGNMENT )
            {
                break;
            }
        }
        nMask <<= 1;
    }

    bLabelAlignmentPosAndSpaceModeActive =
                    ePosAndSpaceMode == SvxNumberFormat::LABEL_ALIGNMENT;
}

void SvxNumPositionTabPage::ShowControlsDependingOnPosAndSpaceMode()
{
    aDistBorderFT.Show( !bLabelAlignmentPosAndSpaceModeActive );
    aDistBorderMF.Show( !bLabelAlignmentPosAndSpaceModeActive );
    aRelativeCB.Show( !bLabelAlignmentPosAndSpaceModeActive );
    aIndentFT.Show( !bLabelAlignmentPosAndSpaceModeActive );
    aIndentMF.Show( !bLabelAlignmentPosAndSpaceModeActive );
    aDistNumFT.Show( !bLabelAlignmentPosAndSpaceModeActive &&
                     pActNum->IsFeatureSupported(NUM_CONTINUOUS) );
    aDistNumMF.Show( !bLabelAlignmentPosAndSpaceModeActive &&
                     pActNum->IsFeatureSupported(NUM_CONTINUOUS));
    aAlignFT.Show( !bLabelAlignmentPosAndSpaceModeActive );
    aAlignLB.Show( !bLabelAlignmentPosAndSpaceModeActive );

    aLabelFollowedByFT.Show( bLabelAlignmentPosAndSpaceModeActive );
    aLabelFollowedByLB.Show( bLabelAlignmentPosAndSpaceModeActive );
    aListtabFT.Show( bLabelAlignmentPosAndSpaceModeActive );
    aListtabMF.Show( bLabelAlignmentPosAndSpaceModeActive );
    aAlign2FT.Show( bLabelAlignmentPosAndSpaceModeActive );
    aAlign2LB.Show( bLabelAlignmentPosAndSpaceModeActive );
    aAlignedAtFT.Show( bLabelAlignmentPosAndSpaceModeActive );
    aAlignedAtMF.Show( bLabelAlignmentPosAndSpaceModeActive );
    aIndentAtFT.Show( bLabelAlignmentPosAndSpaceModeActive );
    aIndentAtMF.Show( bLabelAlignmentPosAndSpaceModeActive );
}
// <--

/*-----------------03.12.97 10:02-------------------

--------------------------------------------------*/
SfxTabPage*	SvxNumPositionTabPage::Create( Window* pParent,
								const SfxItemSet& rAttrSet)
{
	return new SvxNumPositionTabPage(pParent, rAttrSet);
}

/*-----------------04.12.97 12:51-------------------

--------------------------------------------------*/
void	SvxNumPositionTabPage::SetMetric(FieldUnit eMetric)
{
	if(eMetric == FUNIT_MM)
	{
		aDistBorderMF .SetDecimalDigits(1);
		aDistNumMF	  .SetDecimalDigits(1);
		aIndentMF	  .SetDecimalDigits(1);
        // --> OD 2008-02-18 #newlistlevelattrs#
        aListtabMF.SetDecimalDigits(1);
        aAlignedAtMF.SetDecimalDigits(1);
        aIndentAtMF.SetDecimalDigits(1);
        // <--
	}
	aDistBorderMF .SetUnit( eMetric );
	aDistNumMF	  .SetUnit( eMetric );
	aIndentMF	  .SetUnit( eMetric );
    // --> OD 2008-02-18 #newlistlevelattrs#
    aListtabMF.SetUnit( eMetric );
    aAlignedAtMF.SetUnit( eMetric );
    aIndentAtMF.SetUnit( eMetric );
    // <--
}

/*-----------------03.12.97 11:06-------------------

--------------------------------------------------*/
IMPL_LINK( SvxNumPositionTabPage, EditModifyHdl_Impl, Edit *, EMPTYARG )
{
	USHORT nMask = 1;
	for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
	{
		if(nActNumLvl & nMask)
		{
			SvxNumberFormat aNumFmt(pActNum->GetLevel(i));

            // --> OD 2008-01-11 #newlistlevelattrs#
            const USHORT nPos = aAlignLB.IsVisible()
                                ? aAlignLB.GetSelectEntryPos()
                                : aAlign2LB.GetSelectEntryPos();
            // <--
			SvxAdjust eAdjust = SVX_ADJUST_CENTER;
			if(nPos == 0)
				eAdjust = SVX_ADJUST_LEFT;
			else if(nPos == 2)
				eAdjust = SVX_ADJUST_RIGHT;
			aNumFmt.SetNumAdjust( eAdjust );
			pActNum->SetLevel(i, aNumFmt);
		}
		nMask <<= 1;
	}
	SetModified();
	return 0;
}
/*-----------------03.12.97 11:11-------------------

--------------------------------------------------*/
IMPL_LINK( SvxNumPositionTabPage, LevelHdl_Impl, ListBox *, pBox )
{
	USHORT nSaveNumLvl = nActNumLvl;
	nActNumLvl = 0;
	if(pBox->IsEntryPosSelected( pActNum->GetLevelCount() ) &&
			(pBox->GetSelectEntryCount() == 1 || nSaveNumLvl != 0xffff))
	{
		nActNumLvl = 0xFFFF;
		pBox->SetUpdateMode(FALSE);
		for( USHORT i = 0; i < pActNum->GetLevelCount(); i++ )
			pBox->SelectEntryPos( i, FALSE );
		pBox->SetUpdateMode(TRUE);
	}
	else if(pBox->GetSelectEntryCount())
	{
		USHORT nMask = 1;
		for( USHORT i = 0; i < pActNum->GetLevelCount(); i++ )
		{
			if(pBox->IsEntryPosSelected( i ))
				nActNumLvl |= nMask;
			nMask <<= 1;
		}
		pBox->SelectEntryPos( pActNum->GetLevelCount(), FALSE );
	}
	else
	{
		nActNumLvl = nSaveNumLvl;
		USHORT nMask = 1;
		for( USHORT i = 0; i < pActNum->GetLevelCount(); i++ )
		{
			if(nActNumLvl & nMask)
			{
				pBox->SelectEntryPos(i);
				break;
			}
			nMask <<=1;
		}
	}
	aRelativeCB.Enable(nActNumLvl != 1);
	SetModified();
    // --> OD 2008-01-15 #newlistlevelattrs#
    InitPosAndSpaceMode();
    ShowControlsDependingOnPosAndSpaceMode();
    // <--
	InitControls();
	return 0;
}
/*-----------------03.12.97 12:24-------------------

--------------------------------------------------*/
IMPL_LINK( SvxNumPositionTabPage, DistanceHdl_Impl, MetricField *, pFld )
{
	if(bInInintControl)
		return 0;
	long nValue = GetCoreValue(*pFld, eCoreUnit);
	USHORT nMask = 1;
#if OSL_DEBUG_LEVEL > 1
	BOOL bFirst = TRUE;
#endif
	for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
	{
		if(nActNumLvl & nMask)
		{
			SvxNumberFormat aNumFmt( pActNum->GetLevel( i ) );
			if(pFld == &aDistBorderMF)
			{

				if(aRelativeCB.IsChecked())
				{
					if(0 == i)
					{
						long nTmp = aNumFmt.GetFirstLineOffset();
						aNumFmt.SetAbsLSpace( USHORT(nValue - nTmp));
					}
					else
					{
						long nTmp = pActNum->GetLevel( i - 1 ).GetAbsLSpace() +
									pActNum->GetLevel( i - 1 ).GetFirstLineOffset() -
									pActNum->GetLevel( i ).GetFirstLineOffset();

						aNumFmt.SetAbsLSpace( USHORT(nValue + nTmp));
					}
				}
				else
				{
					aNumFmt.SetAbsLSpace( (short)nValue - aNumFmt.GetFirstLineOffset());
				}
			}
			else if(pFld == &aDistNumMF)
			{
				aNumFmt.SetCharTextDistance( (short)nValue );
			}
			else if(pFld == &aIndentMF)
			{
				//jetzt muss mit dem FirstLineOffset auch der AbsLSpace veraendert werden
				long nDiff = nValue + aNumFmt.GetFirstLineOffset();
				long nAbsLSpace = aNumFmt.GetAbsLSpace();
				aNumFmt.SetAbsLSpace(USHORT(nAbsLSpace + nDiff));
				aNumFmt.SetFirstLineOffset( -(short)nValue );
			}

#if OSL_DEBUG_LEVEL > 1
			if(bFirst)
				lcl_PrintDebugOutput(*pDebugFixedText, aNumFmt);
			bFirst = FALSE;
#endif
			pActNum->SetLevel( i, aNumFmt );
		}
		nMask <<= 1;
	}

	SetModified();
	if(!aDistBorderMF.IsEnabled())
	{
		String aEmptyStr;
		aDistBorderMF.SetText(aEmptyStr);
	}

	return 0;
}

/*-----------------04.12.97 12:35-------------------

--------------------------------------------------*/
IMPL_LINK( SvxNumPositionTabPage, RelativeHdl_Impl, CheckBox *, pBox )
{
	BOOL bOn = pBox->IsChecked();
	BOOL bSingleSelection = aLevelLB.GetSelectEntryCount() == 1 && USHRT_MAX != nActNumLvl;
	BOOL bSetValue = FALSE;
	long nValue = 0;
	if(bOn || bSingleSelection)
	{
		USHORT nMask = 1;
		BOOL bFirst = TRUE;
		bSetValue = TRUE;
		for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
		{
			if(nActNumLvl & nMask)
			{
				const SvxNumberFormat &rNumFmt = pActNum->GetLevel(i);
				if(bFirst)
				{
					nValue = rNumFmt.GetAbsLSpace() + rNumFmt.GetFirstLineOffset();
					if(bOn && i)
						nValue -= (pActNum->GetLevel(i - 1).GetAbsLSpace() + pActNum->GetLevel(i - 1).GetFirstLineOffset());
				}
				else
					bSetValue = nValue ==
						(rNumFmt.GetAbsLSpace() + rNumFmt.GetFirstLineOffset()) -
							(pActNum->GetLevel(i - 1).GetAbsLSpace() + pActNum->GetLevel(i - 1).GetFirstLineOffset());
				bFirst = FALSE;
			}
			nMask <<= 1;
		}

	}
	String aEmptyStr;
	if(bSetValue)
		SetMetricValue(aDistBorderMF, nValue,	eCoreUnit);
	else
		aDistBorderMF.SetText(aEmptyStr);
	aDistBorderMF.Enable(bOn || bSingleSelection);
	aDistBorderFT.Enable(bOn || bSingleSelection);
	bLastRelative = bOn;
	return 0;
}

// --> OD 2008-01-14 #newlistlevelattrs#
IMPL_LINK( SvxNumPositionTabPage, LabelFollowedByHdl_Impl, ListBox*, EMPTYARG )
{
    // determine value to be set at the chosen list levels
    SvxNumberFormat::SvxNumLabelFollowedBy eLabelFollowedBy =
                                                    SvxNumberFormat::LISTTAB;
    {
        const USHORT nPos = aLabelFollowedByLB.GetSelectEntryPos();
        if ( nPos == 1 )
        {
            eLabelFollowedBy = SvxNumberFormat::SPACE;
        }
        else if ( nPos == 2 )
        {
            eLabelFollowedBy = SvxNumberFormat::NOTHING;
        }
    }

    // set value at the chosen list levels
    bool bSameListtabPos = true;
    USHORT nFirstLvl = USHRT_MAX;
    USHORT nMask = 1;
    for( USHORT i = 0; i < pActNum->GetLevelCount(); ++i )
    {
        if ( nActNumLvl & nMask )
        {
            SvxNumberFormat aNumFmt( pActNum->GetLevel(i) );
            aNumFmt.SetLabelFollowedBy( eLabelFollowedBy );
            pActNum->SetLevel( i, aNumFmt );

            if ( nFirstLvl == USHRT_MAX )
            {
                nFirstLvl = i;
            }
            else
            {
                bSameListtabPos &= aNumFmt.GetListtabPos() ==
                        pActNum->GetLevel( nFirstLvl ).GetListtabPos();
            }
        }
        nMask <<= 1;
    }

    // enable/disable metric field for list tab stop position depending on
    // selected item following the list label.
    aListtabFT.Enable( eLabelFollowedBy == SvxNumberFormat::LISTTAB );
    aListtabMF.Enable( eLabelFollowedBy == SvxNumberFormat::LISTTAB );
    if ( bSameListtabPos && eLabelFollowedBy == SvxNumberFormat::LISTTAB )
    {
        SetMetricValue( aListtabMF, pActNum->GetLevel( nFirstLvl ).GetListtabPos(), eCoreUnit );
    }
    else
    {
        aListtabMF.SetText( String() );
    }

    SetModified();

    return 0;
}
// <--

// --> OD 2008-01-14 #newlistlevelattrs#
IMPL_LINK( SvxNumPositionTabPage, ListtabPosHdl_Impl, MetricField*, pFld )
{
    // determine value to be set at the chosen list levels
    const long nValue = GetCoreValue( *pFld, eCoreUnit );

    // set value at the chosen list levels
    USHORT nMask = 1;
    for( USHORT i = 0; i < pActNum->GetLevelCount(); ++i )
    {
        if ( nActNumLvl & nMask )
        {
            SvxNumberFormat aNumFmt( pActNum->GetLevel(i) );
            aNumFmt.SetListtabPos( nValue );
            pActNum->SetLevel( i, aNumFmt );
#if OSL_DEBUG_LEVEL > 1
            lcl_PrintDebugOutput(*pDebugFixedText, aNumFmt);
#endif
        }
        nMask <<= 1;
    }

    SetModified();

    return 0;
}
// <--

// --> OD 2008-01-14 #newlistlevelattrs#
IMPL_LINK( SvxNumPositionTabPage, AlignAtHdl_Impl, MetricField*, pFld )
{
    // determine value to be set at the chosen list levels
    const long nValue = GetCoreValue( *pFld, eCoreUnit );

    // set value at the chosen list levels
    USHORT nMask = 1;
    for( USHORT i = 0; i < pActNum->GetLevelCount(); ++i )
    {
        if ( nActNumLvl & nMask )
        {
            SvxNumberFormat aNumFmt( pActNum->GetLevel(i) );
            const long nFirstLineIndent = nValue - aNumFmt.GetIndentAt();
            aNumFmt.SetFirstLineIndent( nFirstLineIndent );
            pActNum->SetLevel( i, aNumFmt );
#if OSL_DEBUG_LEVEL > 1
            lcl_PrintDebugOutput(*pDebugFixedText, aNumFmt);
#endif
        }
        nMask <<= 1;
    }

    SetModified();

    return 0;
}
// <--

// --> OD 2008-01-14 #newlistlevelattrs#
IMPL_LINK( SvxNumPositionTabPage, IndentAtHdl_Impl, MetricField*, pFld )
{
    // determine value to be set at the chosen list levels
    const long nValue = GetCoreValue( *pFld, eCoreUnit );

    // set value at the chosen list levels
    USHORT nMask = 1;
    for( USHORT i = 0; i < pActNum->GetLevelCount(); ++i )
    {
        if ( nActNumLvl & nMask )
        {
            SvxNumberFormat aNumFmt( pActNum->GetLevel(i) );
            const long nAlignedAt = aNumFmt.GetIndentAt() +
                                    aNumFmt.GetFirstLineIndent();
            aNumFmt.SetIndentAt( nValue );
            const long nNewFirstLineIndent = nAlignedAt - nValue;
            aNumFmt.SetFirstLineIndent( nNewFirstLineIndent );
            pActNum->SetLevel( i, aNumFmt );
#if OSL_DEBUG_LEVEL > 1
            lcl_PrintDebugOutput(*pDebugFixedText, aNumFmt);
#endif
        }
        nMask <<= 1;
    }

    SetModified();

    return 0;
}
// <--
/*-----------------05.12.97 15:33-------------------

--------------------------------------------------*/
IMPL_LINK( SvxNumPositionTabPage, StandardHdl_Impl, PushButton *, EMPTYARG )
{
	USHORT nMask = 1;
    // --> OD 2008-02-11 #newlistlevelattrs#
    SvxNumRule aTmpNumRule( pActNum->GetFeatureFlags(),
                            pActNum->GetLevelCount(),
                            pActNum->IsContinuousNumbering(),
                            SVX_RULETYPE_NUMBERING,
                            pActNum->GetLevel( 0 ).GetPositionAndSpaceMode() );
    // <--
	for(USHORT i = 0; i < pActNum->GetLevelCount(); i++)
	{
		if(nActNumLvl & nMask)
		{
			SvxNumberFormat aNumFmt( pActNum->GetLevel( i ) );
			SvxNumberFormat aTempFmt(aTmpNumRule.GetLevel( i ));
            // --> OD 2008-02-05 #newlistlevelattrs#
            aNumFmt.SetPositionAndSpaceMode( aTempFmt.GetPositionAndSpaceMode() );
            if ( aTempFmt.GetPositionAndSpaceMode() == SvxNumberFormat::LABEL_WIDTH_AND_POSITION )
            {
                aNumFmt.SetAbsLSpace( aTempFmt.GetAbsLSpace() );
                aNumFmt.SetCharTextDistance( aTempFmt.GetCharTextDistance() );
                aNumFmt.SetFirstLineOffset( aTempFmt.GetFirstLineOffset() );
            }
            else if ( aTempFmt.GetPositionAndSpaceMode() == SvxNumberFormat::LABEL_ALIGNMENT )
            {
                aNumFmt.SetNumAdjust( aTempFmt.GetNumAdjust() );
                aNumFmt.SetLabelFollowedBy( aTempFmt.GetLabelFollowedBy() );
                aNumFmt.SetListtabPos( aTempFmt.GetListtabPos() );
                aNumFmt.SetFirstLineIndent( aTempFmt.GetFirstLineIndent() );
                aNumFmt.SetIndentAt( aTempFmt.GetIndentAt() );
            }
            // <--

			pActNum->SetLevel( i, aNumFmt );
		}
		nMask <<= 1;
	}

	InitControls();
	SetModified();
	return 0;
}

void SvxNumPositionTabPage::SetModified(BOOL bRepaint)
{
	bModified = TRUE;
	if(bRepaint)
	{
		pPreviewWIN->SetLevel(nActNumLvl);
		pPreviewWIN->Invalidate();
	}
}

void SvxNumOptionsTabPage::SetModified(BOOL bRepaint)
{
	bModified = TRUE;
	if(bRepaint)
	{
		pPreviewWIN->SetLevel(nActNumLvl);
		pPreviewWIN->Invalidate();
	}
}

//Add CHINA001
void SvxNumOptionsTabPage::PageCreated(SfxAllItemSet aSet)
{
	SFX_ITEMSET_ARG	(&aSet,pListItem,SfxStringListItem,SID_CHAR_FMT_LIST_BOX,sal_False);
	SFX_ITEMSET_ARG	(&aSet,pNumCharFmt,SfxStringItem,SID_NUM_CHAR_FMT,sal_False);
	SFX_ITEMSET_ARG	(&aSet,pBulletCharFmt,SfxStringItem,SID_BULLET_CHAR_FMT,sal_False);
	SFX_ITEMSET_ARG	(&aSet,pMetricItem,SfxAllEnumItem,SID_METRIC_ITEM,sal_False);

	if (pNumCharFmt &&pBulletCharFmt)
		SetCharFmts( pNumCharFmt->GetValue(),pBulletCharFmt->GetValue());

	if (pListItem)
	{
		ListBox& myCharFmtLB = GetCharFmtListBox();
		const List *pList = (pListItem)->GetList();
		sal_uInt32 nCount = pList->Count();;
        for(sal_uInt32 i = 0; i < nCount; i++)
		{
			myCharFmtLB.InsertEntry(*(const String*)(pList->GetObject(i)) );

		}
	}
	if (pMetricItem)
		SetMetric(static_cast<FieldUnit>(pMetricItem->GetValue()));
}

//end of add CHINA001

//add CHINA001 begin

void SvxNumPositionTabPage::PageCreated(SfxAllItemSet aSet)
{
	SFX_ITEMSET_ARG	(&aSet,pMetricItem,SfxAllEnumItem,SID_METRIC_ITEM,sal_False);

	if (pMetricItem)
		SetMetric(static_cast<FieldUnit>(pMetricItem->GetValue()));
}
