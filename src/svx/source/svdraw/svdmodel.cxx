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

#include <svx/svdmodel.hxx>

#include <rtl/uuid.h>
#include <com/sun/star/lang/XComponent.hpp>
#include <osl/endian.h>
#include <rtl/logfile.hxx>
#include <math.h>
#include <tools/urlobj.hxx>
#include <unotools/ucbstreamhelper.hxx>

#include <tools/string.hxx>
#include <svtools/whiter.hxx>
#include <svx/xit.hxx>
#include <svx/xbtmpit.hxx>
#include <svx/xlndsit.hxx>
#include <svx/xlnedit.hxx>
#include <svx/xflgrit.hxx>
#include <svx/xflftrit.hxx>
#include <svx/xflhtit.hxx>
#include <svx/xlnstit.hxx>

#include "svditext.hxx"
#include <svx/editeng.hxx>   // Fuer EditEngine::CreatePool()

#include <svx/xtable.hxx>

#include "svditer.hxx"
#include <svx/svdtrans.hxx>
#include <svx/svdpage.hxx>
#include <svx/svdlayer.hxx>
#include <svx/svdundo.hxx>
#include <svx/svdpool.hxx>
#include <svx/svdobj.hxx>
#include <svx/svdotext.hxx>  // fuer ReformatAllTextObjects und CalcFieldValue
#include <svx/svdetc.hxx>
#include <svx/svdoutl.hxx>
#include <svx/svdoole2.hxx>
#include "svdglob.hxx"  // Stringcache
#include "svdstr.hrc"   // Objektname
#include "svdoutlinercache.hxx"


#include "asiancfg.hxx"
#include "fontitem.hxx"
#include <svx/colritem.hxx>
#include <svx/fhgtitem.hxx>
#include <svtools/style.hxx>
#include <tools/bigint.hxx>
#include <svx/numitem.hxx>
#include <bulitem.hxx>
#include <svx/outlobj.hxx>
#include "forbiddencharacterstable.hxx"
#include <svtools/zforlist.hxx>
#include <comphelper/processfactory.hxx>

// #90477#
#include <tools/tenccvt.hxx>
#include <svtools/syslocale.hxx>

// #95114#
#include <vcl/svapp.hxx>
#include <svx/sdr/properties/properties.hxx>
#include <svx/eeitem.hxx>
#include <svtools/itemset.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SdrModelImpl
{
	SfxUndoManager*	mpUndoManager;
	SdrUndoFactory* mpUndoFactory;
	bool mbAllowShapePropertyChangeListener;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

DBG_NAME(SdrModel)
TYPEINIT1(SdrModel,SfxBroadcaster);
void SdrModel::ImpCtor(SfxItemPool* pPool, ::comphelper::IEmbeddedHelper* _pEmbeddedHelper,
	bool bUseExtColorTable, bool bLoadRefCounts)
{
	mpImpl = new SdrModelImpl;
	mpImpl->mpUndoManager=0;
	mpImpl->mpUndoFactory=0;
	mpImpl->mbAllowShapePropertyChangeListener=false;
	mbInDestruction=false;
	aObjUnit=SdrEngineDefaults::GetMapFraction();
	eObjUnit=SdrEngineDefaults::GetMapUnit();
	eUIUnit=FUNIT_MM;
	aUIScale=Fraction(1,1);
	nUIUnitKomma=0;
	bUIOnlyKomma=FALSE;
	pLayerAdmin=NULL;
	pItemPool=pPool;
	bMyPool=FALSE;
	m_pEmbeddedHelper=_pEmbeddedHelper;
	pDrawOutliner=NULL;
	pHitTestOutliner=NULL;
	pRefOutDev=NULL;
	nProgressAkt=0;
	nProgressMax=0;
	nProgressOfs=0;
	pDefaultStyleSheet=NULL;
	pLinkManager=NULL;
	pUndoStack=NULL;
	pRedoStack=NULL;
	nMaxUndoCount=16;
	pAktUndoGroup=NULL;
	nUndoLevel=0;
	mbUndoEnabled=true;
	nProgressPercent=0;
	nLoadVersion=0;
	bExtColorTable=FALSE;
	mbChanged = sal_False;
	bInfoChanged=FALSE;
	bPagNumsDirty=FALSE;
	bMPgNumsDirty=FALSE;
	bPageNotValid=FALSE;
	bSavePortable=FALSE;
	bSaveCompressed=FALSE;
	bSaveNative=FALSE;
	bSwapGraphics=FALSE;
	nSwapGraphicsMode=SDR_SWAPGRAPHICSMODE_DEFAULT;
	bSaveOLEPreview=FALSE;
	bPasteResize=FALSE;
	bNoBitmapCaching=FALSE;
	bReadOnly=FALSE;
	nStreamCompressMode=COMPRESSMODE_NONE;
	nStreamNumberFormat=NUMBERFORMAT_INT_BIGENDIAN;
	nDefaultTabulator=0;
	pColorTable=NULL;
	pDashList=NULL;
	pLineEndList=NULL;
	pHatchList=NULL;
	pGradientList=NULL;
	pBitmapList=NULL;
	mpNumberFormatter = NULL;
	bTransparentTextFrames=FALSE;
	bStarDrawPreviewMode = FALSE;
	nStarDrawPreviewMasterPageNum = SDRPAGE_NOTFOUND;
	pModelStorage = NULL;
	mpForbiddenCharactersTable = NULL;
	mbModelLocked = FALSE;
	mpOutlinerCache = NULL;
	mbKernAsianPunctuation = sal_False;
    mbAddExtLeading = sal_False;

    SvxAsianConfig aAsian;
	mnCharCompressType = aAsian.GetCharDistanceCompression();

#ifdef OSL_LITENDIAN
	nStreamNumberFormat=NUMBERFORMAT_INT_LITTLEENDIAN;
#endif
	bExtColorTable=bUseExtColorTable;

	if ( pPool == NULL )
    {
		pItemPool=new SdrItemPool(0L, bLoadRefCounts);
		// Der Outliner hat keinen eigenen Pool, deshalb den der EditEngine
		SfxItemPool* pOutlPool=EditEngine::CreatePool( bLoadRefCounts );
		// OutlinerPool als SecondaryPool des SdrPool
		pItemPool->SetSecondaryPool(pOutlPool);
		// Merken, dass ich mir die beiden Pools selbst gemacht habe
		bMyPool=TRUE;
	}
	pItemPool->SetDefaultMetric((SfxMapUnit)eObjUnit);

// SJ: #95129# using static SdrEngineDefaults only if default SvxFontHeight item is not available
    const SfxPoolItem* pPoolItem = pItemPool->GetPoolDefaultItem( EE_CHAR_FONTHEIGHT );
    if ( pPoolItem )
        nDefTextHgt = ((SvxFontHeightItem*)pPoolItem)->GetHeight();
    else
        nDefTextHgt = SdrEngineDefaults::GetFontHeight();

	SetTextDefaults();
	pLayerAdmin=new SdrLayerAdmin;
	pLayerAdmin->SetModel(this);
	ImpSetUIUnit();

	// den DrawOutliner OnDemand erzeugen geht noch nicht, weil ich den Pool
	// sonst nicht kriege (erst ab 302!)
	pDrawOutliner = SdrMakeOutliner( OUTLINERMODE_TEXTOBJECT, this );
	ImpSetOutlinerDefaults(pDrawOutliner, TRUE);

	pHitTestOutliner = SdrMakeOutliner( OUTLINERMODE_TEXTOBJECT, this );
	ImpSetOutlinerDefaults(pHitTestOutliner, TRUE);

	ImpCreateTables();
}

SdrModel::SdrModel(SfxItemPool* pPool, ::comphelper::IEmbeddedHelper* pPers, sal_Bool bLoadRefCounts):
	maMaPag(1024,32,32),
	maPages(1024,32,32)
{
#ifdef TIMELOG
    RTL_LOGFILE_CONTEXT_AUTHOR ( aLog, "svx", "aw93748", "SdrModel::SdrModel(...)" );
#endif

	DBG_CTOR(SdrModel,NULL);
	ImpCtor(pPool,pPers,FALSE, (FASTBOOL)bLoadRefCounts);
}

SdrModel::SdrModel(const String& rPath, SfxItemPool* pPool, ::comphelper::IEmbeddedHelper* pPers, sal_Bool bLoadRefCounts):
	maMaPag(1024,32,32),
	maPages(1024,32,32),
	aTablePath(rPath)
{
#ifdef TIMELOG
    RTL_LOGFILE_CONTEXT_AUTHOR ( aLog, "svx", "aw93748", "SdrModel::SdrModel(...)" );
#endif

	DBG_CTOR(SdrModel,NULL);
	ImpCtor(pPool,pPers,FALSE, (FASTBOOL)bLoadRefCounts);
}

SdrModel::SdrModel(SfxItemPool* pPool, ::comphelper::IEmbeddedHelper* pPers, FASTBOOL bUseExtColorTable, sal_Bool bLoadRefCounts):
	maMaPag(1024,32,32),
	maPages(1024,32,32)
{
#ifdef TIMELOG
    RTL_LOGFILE_CONTEXT_AUTHOR ( aLog, "svx", "aw93748", "SdrModel::SdrModel(...)" );
#endif

	DBG_CTOR(SdrModel,NULL);
	ImpCtor(pPool,pPers,bUseExtColorTable, (FASTBOOL)bLoadRefCounts);
}

SdrModel::SdrModel(const String& rPath, SfxItemPool* pPool, ::comphelper::IEmbeddedHelper* pPers, FASTBOOL bUseExtColorTable, sal_Bool bLoadRefCounts):
	maMaPag(1024,32,32),
	maPages(1024,32,32),
	aTablePath(rPath)
{
#ifdef TIMELOG
    RTL_LOGFILE_CONTEXT_AUTHOR ( aLog, "svx", "aw93748", "SdrModel::SdrModel(...)" );
#endif

	DBG_CTOR(SdrModel,NULL);
	ImpCtor(pPool,pPers,bUseExtColorTable, (FASTBOOL)bLoadRefCounts);
}

SdrModel::SdrModel(const SdrModel& /*rSrcModel*/):
	SfxBroadcaster(),
	tools::WeakBase< SdrModel >(),
	maMaPag(1024,32,32),
	maPages(1024,32,32)
{
#ifdef TIMELOG
    RTL_LOGFILE_CONTEXT_AUTHOR ( aLog, "svx", "aw93748", "SdrModel::SdrModel(...)" );
#endif

	// noch nicht implementiert
	DBG_ERROR("SdrModel::CopyCtor() ist noch nicht implementiert");
}

SdrModel::~SdrModel()
{
#ifdef TIMELOG
    RTL_LOGFILE_CONTEXT_AUTHOR ( aLog, "svx", "aw93748", "SdrModel::~SdrModel(...)" );
#endif

	DBG_DTOR(SdrModel,NULL);

	mbInDestruction = true;

	Broadcast(SdrHint(HINT_MODELCLEARED));

	delete mpOutlinerCache;

	ClearUndoBuffer();
#ifdef DBG_UTIL
	if(pAktUndoGroup)
	{
		ByteString aStr("Im Dtor des SdrModel steht noch ein offenes Undo rum: \"");

		aStr += ByteString(pAktUndoGroup->GetComment(), gsl_getSystemTextEncoding());
		aStr += '\"';

		DBG_ERROR(aStr.GetBuffer());
	}
#endif
	if (pAktUndoGroup!=NULL)
		delete pAktUndoGroup;

	// #116168#
	ClearModel(sal_True);

	delete pLayerAdmin;

	// Den DrawOutliner erst nach dem ItemPool loeschen, da
	// der ItemPool Items des DrawOutliners referenziert !!! (<- das war mal)
	// Wg. Problem bei Malte Reihenfolge wieder umgestellt.
	// Loeschen des Outliners vor dem loeschen des ItemPools
	delete pHitTestOutliner;
	delete pDrawOutliner;

	// delete StyleSheetPool, derived classes should not do this since
	// the DrawingEngine may need it in its destrctor (SB)
	if( mxStyleSheetPool.is() )
	{
		Reference< XComponent > xComponent( dynamic_cast< cppu::OWeakObject* >( mxStyleSheetPool.get() ), UNO_QUERY );
		if( xComponent.is() ) try
		{
			xComponent->dispose();
		}
		catch( RuntimeException& )
		{
		}
		mxStyleSheetPool.clear();
	}

	if (bMyPool)
	{
		// Pools loeschen, falls es meine sind
		SfxItemPool* pOutlPool=pItemPool->GetSecondaryPool();
        SfxItemPool::Free(pItemPool);
		// Der OutlinerPool muss nach dem ItemPool plattgemacht werden, da der
		// ItemPool SetItems enthaelt die ihrerseits Items des OutlinerPools
		// referenzieren (Joe)
        SfxItemPool::Free(pOutlPool);
	}

	if( mpForbiddenCharactersTable )
		mpForbiddenCharactersTable->release();

	// Tabellen, Listen und Paletten loeschen
	if (!bExtColorTable)
		delete pColorTable;
	delete pDashList;
	delete pLineEndList;
	delete pHatchList;
	delete pGradientList;
	delete pBitmapList;

	if(mpNumberFormatter)
		delete mpNumberFormatter;

	delete mpImpl->mpUndoFactory;
	delete mpImpl;
}

bool SdrModel::IsInDestruction() const
{
	return mbInDestruction;
}

const SvNumberFormatter& SdrModel::GetNumberFormatter() const
{
	if(!mpNumberFormatter)
	{
		// use cast here since from outside view this IS a const method
		((SdrModel*)this)->mpNumberFormatter = new SvNumberFormatter(
			::comphelper::getProcessServiceFactory(), LANGUAGE_SYSTEM);
	}

	return *mpNumberFormatter;
}

// noch nicht implementiert:
void SdrModel::operator=(const SdrModel& /*rSrcModel*/)
{
	DBG_ERROR("SdrModel::operator=() ist noch nicht implementiert");
}

FASTBOOL SdrModel::operator==(const SdrModel& /*rCmpModel*/) const
{
	DBG_ERROR("SdrModel::operator==() ist noch nicht implementiert");
	return FALSE;
}

void SdrModel::SetSwapGraphics( FASTBOOL bSwap )
{
	bSwapGraphics = bSwap;
}

FASTBOOL SdrModel::IsReadOnly() const
{
	return bReadOnly;
}

void SdrModel::SetReadOnly(FASTBOOL bYes)
{
	bReadOnly=bYes;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SdrModel::SetMaxUndoActionCount(ULONG nAnz)
{
	if (nAnz<1) nAnz=1;
	nMaxUndoCount=nAnz;
	if (pUndoStack!=NULL) {
		while (pUndoStack->Count()>nMaxUndoCount) {
			delete (SfxUndoAction*) pUndoStack->Remove(pUndoStack->Count());
		}
	}
}

void SdrModel::ClearUndoBuffer()
{
	if (pUndoStack!=NULL) {
		while (pUndoStack->Count()!=0) {
			delete (SfxUndoAction*) pUndoStack->Remove(pUndoStack->Count()-1);
		}
		delete pUndoStack;
		pUndoStack=NULL;
	}
	if (pRedoStack!=NULL) {
		while (pRedoStack->Count()!=0) {
			delete (SfxUndoAction*) pRedoStack->Remove(pRedoStack->Count()-1);
		}
		delete pRedoStack;
		pRedoStack=NULL;
	}
}

FASTBOOL SdrModel::Undo()
{
	FASTBOOL bRet=FALSE;
	if( mpImpl->mpUndoManager )
	{
		DBG_ERROR("svx::SdrModel::Undo(), method not supported with application undo manager!");
	}
	else
	{
		SfxUndoAction* pDo=(SfxUndoAction*)GetUndoAction(0);
		if(pDo!=NULL)
		{
			const bool bWasUndoEnabled = mbUndoEnabled;
			mbUndoEnabled = false;
			pDo->Undo();
			if(pRedoStack==NULL)
				pRedoStack=new Container(1024,16,16);
			pRedoStack->Insert(pUndoStack->Remove((ULONG)0),(ULONG)0);
			mbUndoEnabled = bWasUndoEnabled;
		}
	}
	return bRet;
}

FASTBOOL SdrModel::Redo()
{
	FASTBOOL bRet=FALSE;
	if( mpImpl->mpUndoManager )
	{
		DBG_ERROR("svx::SdrModel::Redo(), method not supported with application undo manager!");
	}
	else
	{
		SfxUndoAction* pDo=(SfxUndoAction*)GetRedoAction(0);
		if(pDo!=NULL)
		{
			const bool bWasUndoEnabled = mbUndoEnabled;
			mbUndoEnabled = false;
			pDo->Redo();
			if(pUndoStack==NULL)
				pUndoStack=new Container(1024,16,16);
			pUndoStack->Insert(pRedoStack->Remove((ULONG)0),(ULONG)0);
			mbUndoEnabled = bWasUndoEnabled;
		}
	}
	return bRet;
}

FASTBOOL SdrModel::Repeat(SfxRepeatTarget& rView)
{
	FASTBOOL bRet=FALSE;
	if( mpImpl->mpUndoManager )
	{
		DBG_ERROR("svx::SdrModel::Redo(), method not supported with application undo manager!");
	}
	else
	{
		SfxUndoAction* pDo=(SfxUndoAction*)GetUndoAction(0);
		if(pDo!=NULL)
		{
			if(pDo->CanRepeat(rView))
			{
				pDo->Repeat(rView);
				bRet=TRUE;
			}
		}
	}
	return bRet;
}

void SdrModel::ImpPostUndoAction(SdrUndoAction* pUndo)
{
	DBG_ASSERT( mpImpl->mpUndoManager == 0, "svx::SdrModel::ImpPostUndoAction(), method not supported with application undo manager!" );
	if( IsUndoEnabled() )
	{
		if (aUndoLink.IsSet())
		{
			aUndoLink.Call(pUndo);
		}
		else
		{
			if (pUndoStack==NULL)
				pUndoStack=new Container(1024,16,16);
			pUndoStack->Insert(pUndo,(ULONG)0);
			while (pUndoStack->Count()>nMaxUndoCount)
			{
				delete (SfxUndoAction*)pUndoStack->Remove(pUndoStack->Count()-1);
			}
			if (pRedoStack!=NULL)
				pRedoStack->Clear();
		}
	}
	else
	{
		delete pUndo;
	}
}

void SdrModel::BegUndo()
{
	if( mpImpl->mpUndoManager )
	{
		const String aEmpty;
		mpImpl->mpUndoManager->EnterListAction(aEmpty,aEmpty);
		nUndoLevel++;
	}
	else if( IsUndoEnabled() )
	{
		if(pAktUndoGroup==NULL)
		{
			pAktUndoGroup = new SdrUndoGroup(*this);
			nUndoLevel=1;
		}
		else
		{
			nUndoLevel++;
		}
	}
}

void SdrModel::BegUndo(const XubString& rComment)
{
	if( mpImpl->mpUndoManager )
	{
		const String aEmpty;
		mpImpl->mpUndoManager->EnterListAction( rComment, aEmpty );
		nUndoLevel++;
	}
	else if( IsUndoEnabled() )
	{
		BegUndo();
		if (nUndoLevel==1)
		{
			pAktUndoGroup->SetComment(rComment);
		}
	}
}

void SdrModel::BegUndo(const XubString& rComment, const XubString& rObjDescr, SdrRepeatFunc eFunc)
{
	if( mpImpl->mpUndoManager )
	{
		String aComment(rComment);
		if( aComment.Len() && rObjDescr.Len() )
		{
			String aSearchString(RTL_CONSTASCII_USTRINGPARAM("%1"));
			aComment.SearchAndReplace(aSearchString, rObjDescr);
		}
		const String aEmpty;
		mpImpl->mpUndoManager->EnterListAction( aComment,aEmpty );
		nUndoLevel++;
	}
	else if( IsUndoEnabled() )
	{
		BegUndo();
		if (nUndoLevel==1)
		{
			pAktUndoGroup->SetComment(rComment);
			pAktUndoGroup->SetObjDescription(rObjDescr);
			pAktUndoGroup->SetRepeatFunction(eFunc);
		}
	}
}

void SdrModel::BegUndo(SdrUndoGroup* pUndoGrp)
{
	if( mpImpl->mpUndoManager )
	{
		DBG_ERROR("svx::SdrModel::BegUndo(), method not supported with application undo manager!" );
		nUndoLevel++;
	}
	else if( IsUndoEnabled() )
	{
		if (pAktUndoGroup==NULL)
		{
			pAktUndoGroup=pUndoGrp;
			nUndoLevel=1;
		}
		else
		{
			delete pUndoGrp;
			nUndoLevel++;
		}
	}
	else
	{
		delete pUndoGrp;
	}
}

void SdrModel::EndUndo()
{
	DBG_ASSERT(nUndoLevel!=0,"SdrModel::EndUndo(): UndoLevel is already 0!");
	if( mpImpl->mpUndoManager )
	{
		if( nUndoLevel )
		{
			nUndoLevel--;
			mpImpl->mpUndoManager->LeaveListAction();
		}
	}
	else
	{
		if(pAktUndoGroup!=NULL && IsUndoEnabled())
		{
			nUndoLevel--;
			if(nUndoLevel==0)
			{
				if(pAktUndoGroup->GetActionCount()!=0)
				{
					SdrUndoAction* pUndo=pAktUndoGroup;
					pAktUndoGroup=NULL;
					ImpPostUndoAction(pUndo);
				}
				else
				{
					// was empty
					delete pAktUndoGroup;
					pAktUndoGroup=NULL;
				}
			}
		}
	}
}

void SdrModel::SetUndoComment(const XubString& rComment)
{
	DBG_ASSERT(nUndoLevel!=0,"SdrModel::SetUndoComment(): UndoLevel is on level 0!");

	if( mpImpl->mpUndoManager )
	{
		DBG_ERROR("svx::SdrModel::SetUndoComment(), method not supported with application undo manager!" );
	}
	else if( IsUndoEnabled() )
	{
		if(nUndoLevel==1)
		{
			pAktUndoGroup->SetComment(rComment);
		}
	}
}

void SdrModel::SetUndoComment(const XubString& rComment, const XubString& rObjDescr)
{
	DBG_ASSERT(nUndoLevel!=0,"SdrModel::SetUndoComment(): UndoLevel is 0!");
	if( mpImpl->mpUndoManager )
	{
		DBG_ERROR("svx::SdrModel::SetUndoComment(), method not supported with application undo manager!" );
	}
	else
	{
		if (nUndoLevel==1)
		{
			pAktUndoGroup->SetComment(rComment);
			pAktUndoGroup->SetObjDescription(rObjDescr);
		}
	}
}

void SdrModel::AddUndo(SdrUndoAction* pUndo)
{
	if( mpImpl->mpUndoManager )
	{
		mpImpl->mpUndoManager->AddUndoAction( pUndo );
	}
	else if( !IsUndoEnabled() )
	{
		delete pUndo;
	}
	else
	{
		if (pAktUndoGroup!=NULL)
		{
			pAktUndoGroup->AddAction(pUndo);
		}
		else
		{
			ImpPostUndoAction(pUndo);
		}
	}
}

void SdrModel::EnableUndo( bool bEnable )
{
	if( mpImpl->mpUndoManager )
	{
		mpImpl->mpUndoManager->EnableUndo( bEnable );
	}
	else
	{
		mbUndoEnabled = bEnable;
	}
}

bool SdrModel::IsUndoEnabled() const
{
	if( mpImpl->mpUndoManager )
	{
		return mpImpl->mpUndoManager->IsUndoEnabled();
	}
	else
	{
		return mbUndoEnabled;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SdrModel::ImpCreateTables()
{
	// der Writer hat seinen eigenen ColorTable
	if (!bExtColorTable) pColorTable=new XColorTable(aTablePath,(XOutdevItemPool*)pItemPool);
	pDashList    =new XDashList    (aTablePath,(XOutdevItemPool*)pItemPool);
	pLineEndList =new XLineEndList (aTablePath,(XOutdevItemPool*)pItemPool);
	pHatchList   =new XHatchList   (aTablePath,(XOutdevItemPool*)pItemPool);
	pGradientList=new XGradientList(aTablePath,(XOutdevItemPool*)pItemPool);
	pBitmapList  =new XBitmapList  (aTablePath,(XOutdevItemPool*)pItemPool);
}

// #116168#
void SdrModel::ClearModel(sal_Bool bCalledFromDestructor)
{
	if(bCalledFromDestructor)
	{
		mbInDestruction = true;
	}

	sal_Int32 i;
	// delete all drawing pages
	sal_Int32 nAnz=GetPageCount();
	for (i=nAnz-1; i>=0; i--)
	{
		DeletePage( (USHORT)i );
	}
	maPages.Clear();
	// #109538#
	PageListChanged();

	// delete all Masterpages
	nAnz=GetMasterPageCount();
	for(i=nAnz-1; i>=0; i--)
	{
		DeleteMasterPage( (USHORT)i );
	}
	maMaPag.Clear();
	// #109538#
	MasterPageListChanged();

	pLayerAdmin->ClearLayer();
}

SdrModel* SdrModel::AllocModel() const
{
	SdrModel* pModel=new SdrModel;
	pModel->SetScaleUnit(eObjUnit,aObjUnit);
	return pModel;
}

SdrPage* SdrModel::AllocPage(FASTBOOL bMasterPage)
{
	return new SdrPage(*this,bMasterPage);
}

void SdrModel::SetTextDefaults() const
{
	SetTextDefaults( pItemPool, nDefTextHgt );
}

void ImpGetDefaultFontsLanguage( SvxFontItem& rLatin, SvxFontItem& rAsian, SvxFontItem& rComplex)
{
	const USHORT nItemCnt = 3;
	static struct {
		USHORT nFntType, nLanguage;
	}  aOutTypeArr[ nItemCnt ] = {
		{  DEFAULTFONT_LATIN_TEXT, LANGUAGE_ENGLISH_US },
		{  DEFAULTFONT_CJK_TEXT, LANGUAGE_ENGLISH_US },
		{  DEFAULTFONT_CTL_TEXT, LANGUAGE_ARABIC_SAUDI_ARABIA }
	};
	SvxFontItem* aItemArr[ nItemCnt ] = { &rLatin, &rAsian, &rComplex };

	for( USHORT n = 0; n < nItemCnt; ++n )
	{
		Font aFnt( OutputDevice::GetDefaultFont(
			aOutTypeArr[ n ].nFntType, aOutTypeArr[ n ].nLanguage,
			DEFAULTFONT_FLAGS_ONLYONE, 0 ));
		SvxFontItem* pI = aItemArr[ n ];
		pI->GetFamily() = aFnt.GetFamily();
		pI->GetFamilyName() = aFnt.GetName();
		pI->GetStyleName().Erase();
		pI->GetPitch() = aFnt.GetPitch();
		pI->GetCharSet() = aFnt.GetCharSet();
	}
}

void SdrModel::SetTextDefaults( SfxItemPool* pItemPool, ULONG nDefTextHgt )
{
	// #95114# set application-language specific dynamic pool language defaults
    SvxFontItem aSvxFontItem( EE_CHAR_FONTINFO) ;
	SvxFontItem aSvxFontItemCJK(EE_CHAR_FONTINFO_CJK);
	SvxFontItem aSvxFontItemCTL(EE_CHAR_FONTINFO_CTL);
	sal_uInt16 nLanguage(Application::GetSettings().GetLanguage());

	// get DEFAULTFONT_LATIN_TEXT and set at pool as dynamic default
	Font aFont(OutputDevice::GetDefaultFont(DEFAULTFONT_LATIN_TEXT, nLanguage, DEFAULTFONT_FLAGS_ONLYONE, 0));
	aSvxFontItem.GetFamily() = aFont.GetFamily();
	aSvxFontItem.GetFamilyName() = aFont.GetName();
	aSvxFontItem.GetStyleName().Erase();
	aSvxFontItem.GetPitch() = aFont.GetPitch();
	aSvxFontItem.GetCharSet() = aFont.GetCharSet();
	pItemPool->SetPoolDefaultItem(aSvxFontItem);

	// get DEFAULTFONT_CJK_TEXT and set at pool as dynamic default
	Font aFontCJK(OutputDevice::GetDefaultFont(DEFAULTFONT_CJK_TEXT, nLanguage, DEFAULTFONT_FLAGS_ONLYONE, 0));
	aSvxFontItemCJK.GetFamily() = aFontCJK.GetFamily();
	aSvxFontItemCJK.GetFamilyName() = aFontCJK.GetName();
	aSvxFontItemCJK.GetStyleName().Erase();
	aSvxFontItemCJK.GetPitch() = aFontCJK.GetPitch();
	aSvxFontItemCJK.GetCharSet() = aFontCJK.GetCharSet();
	pItemPool->SetPoolDefaultItem(aSvxFontItemCJK);

	// get DEFAULTFONT_CTL_TEXT and set at pool as dynamic default
	Font aFontCTL(OutputDevice::GetDefaultFont(DEFAULTFONT_CTL_TEXT, nLanguage, DEFAULTFONT_FLAGS_ONLYONE, 0));
	aSvxFontItemCTL.GetFamily() = aFontCTL.GetFamily();
	aSvxFontItemCTL.GetFamilyName() = aFontCTL.GetName();
	aSvxFontItemCTL.GetStyleName().Erase();
	aSvxFontItemCTL.GetPitch() = aFontCTL.GetPitch();
	aSvxFontItemCTL.GetCharSet() = aFontCTL.GetCharSet();
	pItemPool->SetPoolDefaultItem(aSvxFontItemCTL);

	// set dynamic FontHeight defaults
	pItemPool->SetPoolDefaultItem( SvxFontHeightItem(nDefTextHgt, 100, EE_CHAR_FONTHEIGHT ) );
	pItemPool->SetPoolDefaultItem( SvxFontHeightItem(nDefTextHgt, 100, EE_CHAR_FONTHEIGHT_CJK ) );
	pItemPool->SetPoolDefaultItem( SvxFontHeightItem(nDefTextHgt, 100, EE_CHAR_FONTHEIGHT_CTL ) );

	// set FontColor defaults
    pItemPool->SetPoolDefaultItem( SvxColorItem(SdrEngineDefaults::GetFontColor(), EE_CHAR_COLOR) );
}

SdrOutliner& SdrModel::GetDrawOutliner(const SdrTextObj* pObj) const
{
	pDrawOutliner->SetTextObj(pObj);
	return *pDrawOutliner;
}

boost::shared_ptr< SdrOutliner > SdrModel::CreateDrawOutliner(const SdrTextObj* pObj)
{
	boost::shared_ptr< SdrOutliner > xDrawOutliner( SdrMakeOutliner( OUTLINERMODE_TEXTOBJECT, this ) );
	ImpSetOutlinerDefaults(xDrawOutliner.get(), TRUE);
	xDrawOutliner->SetTextObj(pObj);
	return xDrawOutliner;
}

const SdrTextObj* SdrModel::GetFormattingTextObj() const
{
	if (pDrawOutliner!=NULL) {
		return pDrawOutliner->GetTextObj();
	}
	return NULL;
}

void SdrModel::ImpSetOutlinerDefaults( SdrOutliner* pOutliner, BOOL bInit )
{
	/**************************************************************************
	* Initialisierung der Outliner fuer Textausgabe und HitTest
	**************************************************************************/
	if( bInit )
	{
		pOutliner->EraseVirtualDevice();
		pOutliner->SetUpdateMode(FALSE);
		pOutliner->SetEditTextObjectPool(pItemPool);
		pOutliner->SetDefTab(nDefaultTabulator);
	}

	pOutliner->SetRefDevice(GetRefDevice());
	pOutliner->SetForbiddenCharsTable(GetForbiddenCharsTable());
	pOutliner->SetAsianCompressionMode( mnCharCompressType );
	pOutliner->SetKernAsianPunctuation( IsKernAsianPunctuation() );
    pOutliner->SetAddExtLeading( IsAddExtLeading() );

	if ( !GetRefDevice() )
	{
		MapMode aMapMode(eObjUnit, Point(0,0), aObjUnit, aObjUnit);
		pOutliner->SetRefMapMode(aMapMode);
	}
}

void SdrModel::SetRefDevice(OutputDevice* pDev)
{
	pRefOutDev=pDev;
	ImpSetOutlinerDefaults( pDrawOutliner );
	ImpSetOutlinerDefaults( pHitTestOutliner );
	RefDeviceChanged();
}

void SdrModel::ImpReformatAllTextObjects()
{
	if( isLocked() )
		return;

	USHORT nAnz=GetMasterPageCount();
	USHORT nNum;
	for (nNum=0; nNum<nAnz; nNum++) {
		GetMasterPage(nNum)->ReformatAllTextObjects();
	}
	nAnz=GetPageCount();
	for (nNum=0; nNum<nAnz; nNum++) {
		GetPage(nNum)->ReformatAllTextObjects();
	}
}

/** #103122#
	steps over all available pages and sends notify messages to
	all edge objects that are connected to other objects so that
	they may reposition itselfs
*/
void SdrModel::ImpReformatAllEdgeObjects()
{
	if( isLocked() )
		return;

	sal_uInt16 nAnz=GetMasterPageCount();
	sal_uInt16 nNum;
	for (nNum=0; nNum<nAnz; nNum++)
	{
		GetMasterPage(nNum)->ReformatAllEdgeObjects();
	}
	nAnz=GetPageCount();
	for (nNum=0; nNum<nAnz; nNum++)
	{
		GetPage(nNum)->ReformatAllEdgeObjects();
	}
}

SvStream* SdrModel::GetDocumentStream(SdrDocumentStreamInfo& /*rStreamInfo*/) const
{
	return NULL;
}

// Die Vorlagenattribute der Zeichenobjekte in harte Attribute verwandeln.
void SdrModel::BurnInStyleSheetAttributes()
{
	USHORT nAnz=GetMasterPageCount();
	USHORT nNum;
	for (nNum=0; nNum<nAnz; nNum++) {
		GetMasterPage(nNum)->BurnInStyleSheetAttributes();
	}
	nAnz=GetPageCount();
	for (nNum=0; nNum<nAnz; nNum++) {
		GetPage(nNum)->BurnInStyleSheetAttributes();
	}
}

void SdrModel::RefDeviceChanged()
{
	Broadcast(SdrHint(HINT_REFDEVICECHG));
	ImpReformatAllTextObjects();
}

void SdrModel::SetDefaultFontHeight(ULONG nVal)
{
	if (nVal!=nDefTextHgt) {
		nDefTextHgt=nVal;
		Broadcast(SdrHint(HINT_DEFFONTHGTCHG));
		ImpReformatAllTextObjects();
	}
}

void SdrModel::SetDefaultTabulator(USHORT nVal)
{
	if (nDefaultTabulator!=nVal) {
		nDefaultTabulator=nVal;
		Outliner& rOutliner=GetDrawOutliner();
		rOutliner.SetDefTab(nVal);
		Broadcast(SdrHint(HINT_DEFAULTTABCHG));
		ImpReformatAllTextObjects();
	}
}

void SdrModel::ImpSetUIUnit()
{
	if(0 == aUIScale.GetNumerator() || 0 == aUIScale.GetDenominator()) 
    {
        aUIScale = Fraction(1,1);
    }

    // set start values
	nUIUnitKomma = 0;
	sal_Int64 nMul(1);
	sal_Int64 nDiv(1);

	// normalize on meters resp. inch
	switch (eObjUnit) 
    {
		case MAP_100TH_MM   : nUIUnitKomma+=5; break;
		case MAP_10TH_MM    : nUIUnitKomma+=4; break;
		case MAP_MM         : nUIUnitKomma+=3; break;
		case MAP_CM         : nUIUnitKomma+=2; break;
		case MAP_1000TH_INCH: nUIUnitKomma+=3; break;
		case MAP_100TH_INCH : nUIUnitKomma+=2; break;
		case MAP_10TH_INCH  : nUIUnitKomma+=1; break;
		case MAP_INCH       : nUIUnitKomma+=0; break;
		case MAP_POINT      : nDiv=72;     break;          // 1Pt   = 1/72"
		case MAP_TWIP       : nDiv=144; nUIUnitKomma++; break; // 1Twip = 1/1440"
		case MAP_PIXEL      : break;
		case MAP_SYSFONT    : break;
		case MAP_APPFONT    : break;
		case MAP_RELATIVE   : break;
		default: break;
	} // switch

	// 1 mile    =  8 furlong = 63.360" = 1.609.344,0mm
	// 1 furlong = 10 chains  =  7.920" =   201.168,0mm
	// 1 chain   =  4 poles   =    792" =    20.116,8mm
	// 1 pole    =  5 1/2 yd  =    198" =     5.029,2mm
	// 1 yd      =  3 ft      =     36" =       914,4mm
	// 1 ft      = 12 "       =      1" =       304,8mm
	switch (eUIUnit) 
    {
		case FUNIT_NONE   : break;
		// Metrisch
		case FUNIT_100TH_MM: nUIUnitKomma-=5; break;
		case FUNIT_MM     : nUIUnitKomma-=3; break;
		case FUNIT_CM     : nUIUnitKomma-=2; break;
		case FUNIT_M      : nUIUnitKomma+=0; break;
		case FUNIT_KM     : nUIUnitKomma+=3; break;
		// Inch
		case FUNIT_TWIP   : nMul=144; nUIUnitKomma--;  break;  // 1Twip = 1/1440"
		case FUNIT_POINT  : nMul=72;     break;            // 1Pt   = 1/72"
		case FUNIT_PICA   : nMul=6;      break;            // 1Pica = 1/6"  ?
		case FUNIT_INCH   : break;                         // 1"    = 1"
		case FUNIT_FOOT   : nDiv*=12;    break;            // 1Ft   = 12"
		case FUNIT_MILE   : nDiv*=6336; nUIUnitKomma++; break; // 1mile = 63360"
		// sonstiges
		case FUNIT_CUSTOM : break;
		case FUNIT_PERCENT: nUIUnitKomma+=2; break;
	} // switch

    // check if mapping is from metric to inch and adapt
	const bool bMapInch(IsInch(eObjUnit));
	const bool bUIMetr(IsMetric(eUIUnit));

    if (bMapInch && bUIMetr) 
    {
		nUIUnitKomma += 4;
		nMul *= 254;
	}
	
    // check if mapping is from inch to metric and adapt
	const bool bMapMetr(IsMetric(eObjUnit));
	const bool bUIInch(IsInch(eUIUnit));

    if (bMapMetr && bUIInch) 
    {
		nUIUnitKomma -= 4;
		nDiv *= 254;
	}

	// use temporary fraction for reduction (fallback to 32bit here),
    // may need to be changed in the future, too
    if(1 != nMul || 1 != nDiv)
    {
        const Fraction aTemp(static_cast< long >(nMul), static_cast< long >(nDiv));
        nMul = aTemp.GetNumerator();
        nDiv = aTemp.GetDenominator();
    }

    // #i89872# take Unit of Measurement into account
    if(1 != aUIScale.GetDenominator() || 1 != aUIScale.GetNumerator())
    {
        // divide by UIScale
	    nMul *= aUIScale.GetDenominator();
	    nDiv *= aUIScale.GetNumerator();
    }

    // shorten trailing zeroes for dividend
    while(0 == (nMul % 10))
    {
	    nUIUnitKomma--;
	    nMul /= 10;
    }

    // shorten trailing zeroes for divisor
    while(0 == (nDiv % 10)) 
    {
	    nUIUnitKomma++;
	    nDiv /= 10;
    }

    // end preparations, set member values
    aUIUnitFact = Fraction(sal_Int32(nMul), sal_Int32(nDiv));
	bUIOnlyKomma = (nMul == nDiv);
	TakeUnitStr(eUIUnit, aUIUnitStr);
}

void SdrModel::SetScaleUnit(MapUnit eMap, const Fraction& rFrac)
{
	if (eObjUnit!=eMap || aObjUnit!=rFrac) {
		eObjUnit=eMap;
		aObjUnit=rFrac;
		pItemPool->SetDefaultMetric((SfxMapUnit)eObjUnit);
		ImpSetUIUnit();
		ImpSetOutlinerDefaults( pDrawOutliner );
		ImpSetOutlinerDefaults( pHitTestOutliner );
		ImpReformatAllTextObjects(); // #40424#
	}
}

void SdrModel::SetScaleUnit(MapUnit eMap)
{
	if (eObjUnit!=eMap) {
		eObjUnit=eMap;
		pItemPool->SetDefaultMetric((SfxMapUnit)eObjUnit);
		ImpSetUIUnit();
		ImpSetOutlinerDefaults( pDrawOutliner );
		ImpSetOutlinerDefaults( pHitTestOutliner );
		ImpReformatAllTextObjects(); // #40424#
	}
}

void SdrModel::SetScaleFraction(const Fraction& rFrac)
{
	if (aObjUnit!=rFrac) {
		aObjUnit=rFrac;
		ImpSetUIUnit();
		ImpSetOutlinerDefaults( pDrawOutliner );
		ImpSetOutlinerDefaults( pHitTestOutliner );
		ImpReformatAllTextObjects(); // #40424#
	}
}

void SdrModel::SetUIUnit(FieldUnit eUnit)
{
	if (eUIUnit!=eUnit) {
		eUIUnit=eUnit;
		ImpSetUIUnit();
		ImpReformatAllTextObjects(); // #40424#
	}
}

void SdrModel::SetUIScale(const Fraction& rScale)
{
	if (aUIScale!=rScale) {
		aUIScale=rScale;
		ImpSetUIUnit();
		ImpReformatAllTextObjects(); // #40424#
	}
}

void SdrModel::SetUIUnit(FieldUnit eUnit, const Fraction& rScale)
{
	if (eUIUnit!=eUnit || aUIScale!=rScale) {
		eUIUnit=eUnit;
		aUIScale=rScale;
		ImpSetUIUnit();
		ImpReformatAllTextObjects(); // #40424#
	}
}

void SdrModel::TakeUnitStr(FieldUnit eUnit, XubString& rStr)
{
	switch(eUnit)
	{
		default:
		case FUNIT_NONE   :
		case FUNIT_CUSTOM :
		{
			rStr = String();
			break;
		}
		case FUNIT_100TH_MM:
		{
			sal_Char aText[] = "/100mm";
			rStr = UniString(aText, sizeof(aText-1));
			break;
		}
		case FUNIT_MM     :
		{
			sal_Char aText[] = "mm";
			rStr = UniString(aText, sizeof(aText-1));
			break;
		}
		case FUNIT_CM     :
		{
			sal_Char aText[] = "cm";
			rStr = UniString(aText, sizeof(aText-1));
			break;
		}
		case FUNIT_M      :
		{
			rStr = String();
			rStr += sal_Unicode('m');
			break;
		}
		case FUNIT_KM     :
		{
			sal_Char aText[] = "km";
			rStr = UniString(aText, sizeof(aText-1));
			break;
		}
		case FUNIT_TWIP   :
		{
			sal_Char aText[] = "twip";
			rStr = UniString(aText, sizeof(aText-1));
			break;
		}
		case FUNIT_POINT  :
		{
			sal_Char aText[] = "pt";
			rStr = UniString(aText, sizeof(aText-1));
			break;
		}
		case FUNIT_PICA   :
		{
			sal_Char aText[] = "pica";
			rStr = UniString(aText, sizeof(aText-1));
			break;
		}
		case FUNIT_INCH   :
		{
			rStr = String();
			rStr += sal_Unicode('"');
			break;
		}
		case FUNIT_FOOT   :
		{
			sal_Char aText[] = "ft";
			rStr = UniString(aText, sizeof(aText-1));
			break;
		}
		case FUNIT_MILE   :
		{
			sal_Char aText[] = "mile(s)";
			rStr = UniString(aText, sizeof(aText-1));
			break;
		}
		case FUNIT_PERCENT:
		{
			rStr = String();
			rStr += sal_Unicode('%');
			break;
		}
	}
}

void SdrModel::TakeMetricStr(long nVal, XubString& rStr, FASTBOOL bNoUnitChars, sal_Int32 nNumDigits) const
{
	// #i22167#
	// change to double precision usage to not loose decimal places after comma
	const bool bNegative(nVal < 0L);
    SvtSysLocale aSysLoc;
    const LocaleDataWrapper& rLoc(aSysLoc.GetLocaleData());
	double fLocalValue(double(nVal) * double(aUIUnitFact));

	if(bNegative)
	{
		fLocalValue = -fLocalValue;
	}

	if( -1 == nNumDigits )
	{
		nNumDigits = rLoc.getNumDigits();
	}

	sal_Int32 nKomma(nUIUnitKomma);

	if(nKomma > nNumDigits)
	{
		const sal_Int32 nDiff(nKomma - nNumDigits);
		const double fFactor(pow(10.0, static_cast<const int>(nDiff)));

		fLocalValue /= fFactor;
		nKomma = nNumDigits;
	}
	else if(nKomma < nNumDigits)
	{
		const sal_Int32 nDiff(nNumDigits - nKomma);
		const double fFactor(pow(10.0, static_cast<const int>(nDiff)));

		fLocalValue *= fFactor;
		nKomma = nNumDigits;
	}

	rStr = UniString::CreateFromInt32(static_cast<sal_Int32>(fLocalValue + 0.5));

	if(nKomma < 0)
	{
		// Negatives Komma bedeutet: Nullen dran
		sal_Int32 nAnz(-nKomma);

		for(sal_Int32 i=0; i<nAnz; i++)
			rStr += sal_Unicode('0');

		nKomma = 0;
	}

	// #83257# the second condition needs to be <= since inside this loop
	// also the leading zero is inserted.
	if(nKomma > 0 && rStr.Len() <= nKomma)
	{
		// Fuer Komma evtl. vorne Nullen dran
		sal_Int32 nAnz(nKomma - rStr.Len());

        if(nAnz >= 0 && rLoc.isNumLeadingZero())
			nAnz++;

		for(sal_Int32 i=0; i<nAnz; i++)
			rStr.Insert(sal_Unicode('0'), 0);
	}

    sal_Unicode cDec( rLoc.getNumDecimalSep().GetChar(0) );

	// KommaChar einfuegen
	sal_Int32 nVorKomma(rStr.Len() - nKomma);

	if(nKomma > 0)
        rStr.Insert(cDec, (xub_StrLen) nVorKomma);

    if(!rLoc.isNumTrailingZeros())
	{
		while(rStr.Len() && rStr.GetChar(rStr.Len() - 1) == sal_Unicode('0'))
			rStr.Erase(rStr.Len() - 1);

		if(rStr.Len() && rStr.GetChar(rStr.Len() - 1) == cDec)
			rStr.Erase(rStr.Len() - 1);
	}

	// ggf. Trennpunkte bei jedem Tausender einfuegen
    if( nVorKomma > 3 )
	{
        String aThoSep( rLoc.getNumThousandSep() );
        if ( aThoSep.Len() > 0 )
        {
            sal_Unicode cTho( aThoSep.GetChar(0) );
            sal_Int32 i(nVorKomma - 3);

            while(i > 0) // #78311#
            {
                rStr.Insert(cTho, (xub_StrLen)i);
                i -= 3;
            }
        }
	}

	if(!rStr.Len())
	{
		rStr = String();
		rStr += sal_Unicode('0');
	}

	if(bNegative)
	{
		rStr.Insert(sal_Unicode('-'), 0);
	}

	if(!bNoUnitChars)
		rStr += aUIUnitStr;
}

void SdrModel::TakeWinkStr(long nWink, XubString& rStr, FASTBOOL bNoDegChar) const
{
	BOOL bNeg(nWink < 0);

	if(bNeg)
		nWink = -nWink;

	rStr = UniString::CreateFromInt32(nWink);

    SvtSysLocale aSysLoc;
    const LocaleDataWrapper& rLoc = aSysLoc.GetLocaleData();
	xub_StrLen nAnz(2);

    if(rLoc.isNumLeadingZero())
		nAnz++;

	while(rStr.Len() < nAnz)
		rStr.Insert(sal_Unicode('0'), 0);

    rStr.Insert(rLoc.getNumDecimalSep().GetChar(0), rStr.Len() - 2);

	if(bNeg)
		rStr.Insert(sal_Unicode('-'), 0);

	if(!bNoDegChar)
		rStr += DEGREE_CHAR;
}

void SdrModel::TakePercentStr(const Fraction& rVal, XubString& rStr, FASTBOOL bNoPercentChar) const
{
	INT32 nMul(rVal.GetNumerator());
	INT32 nDiv(rVal.GetDenominator());
	BOOL bNeg(nMul < 0);

	if(nDiv < 0)
		bNeg = !bNeg;

	if(nMul < 0)
		nMul = -nMul;

	if(nDiv < 0)
		nDiv = -nDiv;

	nMul *= 100;
	nMul += nDiv/2;
	nMul /= nDiv;

	rStr = UniString::CreateFromInt32(nMul);

	if(bNeg)
		rStr.Insert(sal_Unicode('-'), 0);

	if(!bNoPercentChar)
		rStr += sal_Unicode('%');
}

void SdrModel::SetChanged(sal_Bool bFlg)
{
	mbChanged = bFlg;
}

void SdrModel::RecalcPageNums(FASTBOOL bMaster)
{
	Container& rPL=*(bMaster ? &maMaPag : &maPages);
	USHORT nAnz=USHORT(rPL.Count());
	USHORT i;
	for (i=0; i<nAnz; i++) {
		SdrPage* pPg=(SdrPage*)(rPL.GetObject(i));
		pPg->SetPageNum(i);
	}
	if (bMaster) bMPgNumsDirty=FALSE;
	else bPagNumsDirty=FALSE;
}

void SdrModel::InsertPage(SdrPage* pPage, USHORT nPos)
{
	USHORT nAnz=GetPageCount();
	if (nPos>nAnz) nPos=nAnz;
	maPages.Insert(pPage,nPos);
	// #109538#
	PageListChanged();
	pPage->SetInserted(TRUE);
	pPage->SetPageNum(nPos);
	pPage->SetModel(this);
	if (nPos<nAnz) bPagNumsDirty=TRUE;
	SetChanged();
	SdrHint aHint(HINT_PAGEORDERCHG);
	aHint.SetPage(pPage);
	Broadcast(aHint);
}

void SdrModel::DeletePage(USHORT nPgNum)
{
	SdrPage* pPg=RemovePage(nPgNum);
	delete pPg;
}

SdrPage* SdrModel::RemovePage(USHORT nPgNum)
{
	SdrPage* pPg=(SdrPage*)maPages.Remove(nPgNum);
	// #109538#
	PageListChanged();
	if (pPg!=NULL) {
		pPg->SetInserted(FALSE);
	}
	bPagNumsDirty=TRUE;
	SetChanged();
	SdrHint aHint(HINT_PAGEORDERCHG);
	aHint.SetPage(pPg);
	Broadcast(aHint);
	return pPg;
}

void SdrModel::MovePage(USHORT nPgNum, USHORT nNewPos)
{
	SdrPage* pPg=(SdrPage*)maPages.Remove(nPgNum);
	// #109538#
	PageListChanged();
	if (pPg!=NULL) {
		pPg->SetInserted(FALSE);
		InsertPage(pPg,nNewPos);
	}
}

void SdrModel::InsertMasterPage(SdrPage* pPage, USHORT nPos)
{
	USHORT nAnz=GetMasterPageCount();
	if (nPos>nAnz) nPos=nAnz;
	maMaPag.Insert(pPage,nPos);
	// #109538#
	MasterPageListChanged();
	pPage->SetInserted(TRUE);
	pPage->SetPageNum(nPos);
	pPage->SetModel(this);
	if (nPos<nAnz) {
		bMPgNumsDirty=TRUE;
	}
	SetChanged();
	SdrHint aHint(HINT_PAGEORDERCHG);
	aHint.SetPage(pPage);
	Broadcast(aHint);
}

void SdrModel::DeleteMasterPage(USHORT nPgNum)
{
	SdrPage* pPg=RemoveMasterPage(nPgNum);
	if (pPg!=NULL) delete pPg;
}

SdrPage* SdrModel::RemoveMasterPage(USHORT nPgNum)
{
	SdrPage* pRetPg=(SdrPage*)maMaPag.Remove(nPgNum);
	// #109538#
	MasterPageListChanged();

	if(pRetPg)
	{
		// Nun die Verweise der normalen Zeichenseiten auf die entfernte MasterPage loeschen
		sal_uInt16 nPageAnz(GetPageCount());

		for(sal_uInt16 np(0); np < nPageAnz; np++)
		{
			GetPage(np)->TRG_ImpMasterPageRemoved(*pRetPg);
		}

		pRetPg->SetInserted(FALSE);
	}

	bMPgNumsDirty=TRUE;
	SetChanged();
	SdrHint aHint(HINT_PAGEORDERCHG);
	aHint.SetPage(pRetPg);
	Broadcast(aHint);
	return pRetPg;
}

void SdrModel::MoveMasterPage(USHORT nPgNum, USHORT nNewPos)
{
	SdrPage* pPg=(SdrPage*)maMaPag.Remove(nPgNum);
	// #109538#
	MasterPageListChanged();
	if (pPg!=NULL) {
		pPg->SetInserted(FALSE);
		maMaPag.Insert(pPg,nNewPos);
		// #109538#
		MasterPageListChanged();
	}
	bMPgNumsDirty=TRUE;
	SetChanged();
	SdrHint aHint(HINT_PAGEORDERCHG);
	aHint.SetPage(pPg);
	Broadcast(aHint);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

FASTBOOL SdrModel::CheckConsistence() const
{
	FASTBOOL bRet=TRUE;
#ifdef DBG_UTIL
	DBG_CHKTHIS(SdrModel,NULL);
#endif
	return bRet;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// #48289#
void SdrModel::CopyPages(USHORT nFirstPageNum, USHORT nLastPageNum,
						 USHORT nDestPos,
						 FASTBOOL bUndo, FASTBOOL bMoveNoCopy)
{
	if( bUndo && !IsUndoEnabled() )
		bUndo = false;

	if( bUndo )
		BegUndo(ImpGetResStr(STR_UndoMergeModel));

	USHORT nPageAnz=GetPageCount();
	USHORT nMaxPage=nPageAnz;
	
	if (nMaxPage!=0)
		nMaxPage--;
	if (nFirstPageNum>nMaxPage)
		nFirstPageNum=nMaxPage;
	if (nLastPageNum>nMaxPage)
		nLastPageNum =nMaxPage;
	FASTBOOL bReverse=nLastPageNum<nFirstPageNum;
	if (nDestPos>nPageAnz)
		nDestPos=nPageAnz;

	// Zunaechst die Zeiger der betroffenen Seiten in einem Array sichern
	USHORT nPageNum=nFirstPageNum;
	USHORT nCopyAnz=((!bReverse)?(nLastPageNum-nFirstPageNum):(nFirstPageNum-nLastPageNum))+1;
	SdrPage** pPagePtrs=new SdrPage*[nCopyAnz];
	USHORT nCopyNum;
	for(nCopyNum=0; nCopyNum<nCopyAnz; nCopyNum++)
	{
		pPagePtrs[nCopyNum]=GetPage(nPageNum);
		if (bReverse)
			nPageNum--;
		else
			nPageNum++;
	}

	// Jetzt die Seiten kopieren
	USHORT nDestNum=nDestPos;
	for (nCopyNum=0; nCopyNum<nCopyAnz; nCopyNum++)
	{
		SdrPage* pPg=pPagePtrs[nCopyNum];
		USHORT nPageNum2=pPg->GetPageNum();
		if (!bMoveNoCopy)
		{
			const SdrPage* pPg1=GetPage(nPageNum2);
			pPg=pPg1->Clone();
			InsertPage(pPg,nDestNum);
			if (bUndo)
				AddUndo(GetSdrUndoFactory().CreateUndoCopyPage(*pPg));
			nDestNum++;
		}
		else
		{
			// Move ist nicht getestet!
			if (nDestNum>nPageNum2)
				nDestNum--;

			if(bUndo)
				AddUndo(GetSdrUndoFactory().CreateUndoSetPageNum(*GetPage(nPageNum2),nPageNum2,nDestNum));

			pPg=RemovePage(nPageNum2);
			InsertPage(pPg,nDestNum);
			nDestNum++;
		}

		if(bReverse)
			nPageNum2--;
		else
			nPageNum2++;
	}

	delete[] pPagePtrs;
	if(bUndo)
		EndUndo();
}

void SdrModel::Merge(SdrModel& rSourceModel,
					 USHORT nFirstPageNum, USHORT nLastPageNum,
					 USHORT nDestPos,
					 FASTBOOL bMergeMasterPages, FASTBOOL bAllMasterPages,
					 FASTBOOL bUndo, FASTBOOL bTreadSourceAsConst)
{
	if (&rSourceModel==this)
	{ // #48289#
		CopyPages(nFirstPageNum,nLastPageNum,nDestPos,bUndo,!bTreadSourceAsConst);
		return;
	}

	if( bUndo && !IsUndoEnabled() )
		bUndo = false;

	if (bUndo)
		BegUndo(ImpGetResStr(STR_UndoMergeModel));

	USHORT nSrcPageAnz=rSourceModel.GetPageCount();
	USHORT nSrcMasterPageAnz=rSourceModel.GetMasterPageCount();
	USHORT nDstMasterPageAnz=GetMasterPageCount();
	FASTBOOL bInsPages=(nFirstPageNum<nSrcPageAnz || nLastPageNum<nSrcPageAnz);
	USHORT nMaxSrcPage=nSrcPageAnz; if (nMaxSrcPage!=0) nMaxSrcPage--;
	if (nFirstPageNum>nMaxSrcPage) nFirstPageNum=nMaxSrcPage;
	if (nLastPageNum>nMaxSrcPage)  nLastPageNum =nMaxSrcPage;
	FASTBOOL bReverse=nLastPageNum<nFirstPageNum;

	USHORT*   pMasterMap=NULL;
	FASTBOOL* pMasterNeed=NULL;
	USHORT    nMasterNeed=0;
	if (bMergeMasterPages && nSrcMasterPageAnz!=0) {
		// Feststellen, welche MasterPages aus rSrcModel benoetigt werden
		pMasterMap=new USHORT[nSrcMasterPageAnz];
		pMasterNeed=new FASTBOOL[nSrcMasterPageAnz];
		memset(pMasterMap,0xFF,nSrcMasterPageAnz*sizeof(USHORT));
		if (bAllMasterPages) {
			memset(pMasterNeed,TRUE,nSrcMasterPageAnz*sizeof(FASTBOOL));
		} else {
			memset(pMasterNeed,FALSE,nSrcMasterPageAnz*sizeof(FASTBOOL));
			USHORT nAnf= bReverse ? nLastPageNum : nFirstPageNum;
			USHORT nEnd= bReverse ? nFirstPageNum : nLastPageNum;
			for (USHORT i=nAnf; i<=nEnd; i++) {
				const SdrPage* pPg=rSourceModel.GetPage(i);
				if(pPg->TRG_HasMasterPage())
				{
					SdrPage& rMasterPage = pPg->TRG_GetMasterPage();
					sal_uInt16 nMPgNum(rMasterPage.GetPageNum());

					if(nMPgNum < nSrcMasterPageAnz)
					{
						pMasterNeed[nMPgNum] = TRUE;
					}
				}
			}
		}
		// Nun das Mapping der MasterPages bestimmen
		USHORT nAktMaPagNum=nDstMasterPageAnz;
		for (USHORT i=0; i<nSrcMasterPageAnz; i++) {
			if (pMasterNeed[i]) {
				pMasterMap[i]=nAktMaPagNum;
				nAktMaPagNum++;
				nMasterNeed++;
			}
		}
	}

	// rueberholen der Masterpages
	if (pMasterMap!=NULL && pMasterNeed!=NULL && nMasterNeed!=0) {
		for (USHORT i=nSrcMasterPageAnz; i>0;) {
			i--;
			if (pMasterNeed[i]) {
				SdrPage* pPg=NULL;
				if (bTreadSourceAsConst) {
					const SdrPage* pPg1=rSourceModel.GetMasterPage(i);
					pPg=pPg1->Clone();
				} else {
					pPg=rSourceModel.RemoveMasterPage(i);
				}
				if (pPg!=NULL) {
					// und alle ans einstige Ende des DstModel reinschieben.
					// nicht InsertMasterPage() verwenden da die Sache
					// inkonsistent ist bis alle drin sind
					maMaPag.Insert(pPg,nDstMasterPageAnz);
					// #109538#
					MasterPageListChanged();
					pPg->SetInserted(TRUE);
					pPg->SetModel(this);
					bMPgNumsDirty=TRUE;
					if (bUndo) AddUndo(GetSdrUndoFactory().CreateUndoNewPage(*pPg));
				} else {
					DBG_ERROR("SdrModel::Merge(): MasterPage im SourceModel nicht gefunden");
				}
			}
		}
	}

	// rueberholen der Zeichenseiten
	if (bInsPages) {
		USHORT nSourcePos=nFirstPageNum;
		USHORT nMergeCount=USHORT(Abs((long)((long)nFirstPageNum-nLastPageNum))+1);
		if (nDestPos>GetPageCount()) nDestPos=GetPageCount();
		while (nMergeCount>0) {
			SdrPage* pPg=NULL;
			if (bTreadSourceAsConst) {
				const SdrPage* pPg1=rSourceModel.GetPage(nSourcePos);
				pPg=pPg1->Clone();
			} else {
				pPg=rSourceModel.RemovePage(nSourcePos);
			}
			if (pPg!=NULL) {
				InsertPage(pPg,nDestPos);
				if (bUndo) AddUndo(GetSdrUndoFactory().CreateUndoNewPage(*pPg));
				// und nun zu den MasterPageDescriptoren

				if(pPg->TRG_HasMasterPage())
				{
					SdrPage& rMasterPage = pPg->TRG_GetMasterPage();
					sal_uInt16 nMaPgNum(rMasterPage.GetPageNum());

					if (bMergeMasterPages)
					{
						sal_uInt16 nNeuNum(0xFFFF);

						if(pMasterMap)
						{
							nNeuNum = pMasterMap[nMaPgNum];
						}

						if(nNeuNum != 0xFFFF)
						{
							if(bUndo)
							{
								AddUndo(GetSdrUndoFactory().CreateUndoPageChangeMasterPage(*pPg));
							}

							pPg->TRG_SetMasterPage(*GetMasterPage(nNeuNum));
						}
						DBG_ASSERT(nNeuNum!=0xFFFF,"SdrModel::Merge(): Irgendwas ist krumm beim Mappen der MasterPages");
					} else {
						if (nMaPgNum>=nDstMasterPageAnz) {
							// Aha, die ist ausserbalb des urspruenglichen Bereichs der Masterpages des DstModel
							pPg->TRG_ClearMasterPage();
						}
					}
				}

			} else {
				DBG_ERROR("SdrModel::Merge(): Zeichenseite im SourceModel nicht gefunden");
			}
			nDestPos++;
			if (bReverse) nSourcePos--;
			else if (bTreadSourceAsConst) nSourcePos++;
			nMergeCount--;
		}
	}

	delete [] pMasterMap;
	delete [] pMasterNeed;

	bMPgNumsDirty=TRUE;
	bPagNumsDirty=TRUE;

	SetChanged();
	// Fehlt: Mergen und Mapping der Layer
	// an den Objekten sowie an den MasterPageDescriptoren
	if (bUndo) EndUndo();
}

void SdrModel::SetStarDrawPreviewMode(BOOL bPreview)
{
	if (!bPreview && bStarDrawPreviewMode && GetPageCount())
	{
		// Das Zuruecksetzen ist nicht erlaubt, da das Model ev. nicht vollstaendig geladen wurde
		DBG_ASSERT(FALSE,"SdrModel::SetStarDrawPreviewMode(): Zuruecksetzen nicht erlaubt, da Model ev. nicht vollstaendig");
	}
	else
	{
		bStarDrawPreviewMode = bPreview;
	}
}

uno::Reference< uno::XInterface > SdrModel::getUnoModel()
{
	if( !mxUnoModel.is() )
		mxUnoModel = createUnoModel();

	return mxUnoModel;
}

void SdrModel::setUnoModel( ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > xModel )
{
    mxUnoModel = xModel;
}

uno::Reference< uno::XInterface > SdrModel::createUnoModel()
{
	DBG_ERROR( "SdrModel::createUnoModel() - base implementation should not be called!" );
	::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > xInt;
	return xInt;
}

void SdrModel::setLock( BOOL bLock )
{
	if( mbModelLocked != bLock )
	{
		if( sal_False == bLock )
		{
			// ReformatAllTextObjects(); #103122# due to a typo in the above if, this code was never
			//							 executed, so I remove it until we discover that we need it here
			ImpReformatAllEdgeObjects();	// #103122#
		}
		mbModelLocked = bLock;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SdrModel::MigrateItemSet( const SfxItemSet* pSourceSet, SfxItemSet* pDestSet, SdrModel* pNewModel )
{
	if( pSourceSet && pDestSet && (pSourceSet != pDestSet ) )
	{
		if( pNewModel == NULL )
			pNewModel = this;

		SfxWhichIter aWhichIter(*pSourceSet);
		sal_uInt16 nWhich(aWhichIter.FirstWhich());
		const SfxPoolItem *pPoolItem;

		while(nWhich)
		{
			if(SFX_ITEM_SET == pSourceSet->GetItemState(nWhich, FALSE, &pPoolItem))
			{
				const SfxPoolItem* pItem = pPoolItem;

				switch( nWhich )
				{
				case XATTR_FILLBITMAP:
					pItem = ((XFillBitmapItem*)pItem)->checkForUniqueItem( pNewModel );
					break;
				case XATTR_LINEDASH:
					pItem = ((XLineDashItem*)pItem)->checkForUniqueItem( pNewModel );
					break;
				case XATTR_LINESTART:
					pItem = ((XLineStartItem*)pItem)->checkForUniqueItem( pNewModel );
					break;
				case XATTR_LINEEND:
					pItem = ((XLineEndItem*)pItem)->checkForUniqueItem( pNewModel );
					break;
				case XATTR_FILLGRADIENT:
					pItem = ((XFillGradientItem*)pItem)->checkForUniqueItem( pNewModel );
					break;
				case XATTR_FILLFLOATTRANSPARENCE:
					// #85953# allow all kinds of XFillFloatTransparenceItem to be set
					pItem = ((XFillFloatTransparenceItem*)pItem)->checkForUniqueItem( pNewModel );
					break;
				case XATTR_FILLHATCH:
					pItem = ((XFillHatchItem*)pItem)->checkForUniqueItem( pNewModel );
					break;
				}

				// set item
				if( pItem )
				{
					pDestSet->Put(*pItem);

					// delete item if it was a generated one
					if( pItem != pPoolItem)
						delete (SfxPoolItem*)pItem;
				}
			}
			nWhich = aWhichIter.NextWhich();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SdrModel::SetForbiddenCharsTable( vos::ORef<SvxForbiddenCharactersTable> xForbiddenChars )
{
	if( mpForbiddenCharactersTable )
		mpForbiddenCharactersTable->release();

	mpForbiddenCharactersTable = xForbiddenChars.getBodyPtr();

	if( mpForbiddenCharactersTable )
		mpForbiddenCharactersTable->acquire();

	ImpSetOutlinerDefaults( pDrawOutliner );
	ImpSetOutlinerDefaults( pHitTestOutliner );
}

vos::ORef<SvxForbiddenCharactersTable> SdrModel::GetForbiddenCharsTable() const
{
	return mpForbiddenCharactersTable;
}

void SdrModel::SetCharCompressType( UINT16 nType )
{
	if( nType != mnCharCompressType )
	{
		mnCharCompressType = nType;
		ImpSetOutlinerDefaults( pDrawOutliner );
		ImpSetOutlinerDefaults( pHitTestOutliner );
	}
}

void SdrModel::SetKernAsianPunctuation( sal_Bool bEnabled )
{
	if( mbKernAsianPunctuation != bEnabled )
	{
		mbKernAsianPunctuation = bEnabled;
		ImpSetOutlinerDefaults( pDrawOutliner );
		ImpSetOutlinerDefaults( pHitTestOutliner );
	}
}

void SdrModel::SetAddExtLeading( sal_Bool bEnabled )
{
    if( mbAddExtLeading != bEnabled )
    {
        mbAddExtLeading = bEnabled;
        ImpSetOutlinerDefaults( pDrawOutliner );
        ImpSetOutlinerDefaults( pHitTestOutliner );
    }
}

void SdrModel::ReformatAllTextObjects()
{
	ImpReformatAllTextObjects();
}

FASTBOOL SdrModel::HasTransparentObjects( BOOL bCheckForAlphaChannel ) const
{
	FASTBOOL	bRet = FALSE;
	USHORT		n, nCount;

	for( n = 0, nCount = GetMasterPageCount(); ( n < nCount ) && !bRet; n++ )
		if( GetMasterPage( n )->HasTransparentObjects( bCheckForAlphaChannel ) )
			bRet = TRUE;

	if( !bRet )
	{
		for( n = 0, nCount = GetPageCount(); ( n < nCount ) && !bRet; n++ )
			if( GetPage( n )->HasTransparentObjects( bCheckForAlphaChannel ) )
				bRet = TRUE;
	}

	return bRet;
}

SdrOutliner* SdrModel::createOutliner( USHORT nOutlinerMode )
{
	if( NULL == mpOutlinerCache )
		mpOutlinerCache = new SdrOutlinerCache(this);

	return mpOutlinerCache->createOutliner( nOutlinerMode );
}

void SdrModel::disposeOutliner( SdrOutliner* pOutliner )
{
	if( mpOutlinerCache )
	{
		mpOutlinerCache->disposeOutliner( pOutliner );
	}
	else
	{
		delete pOutliner;
	}
}

SvxNumType SdrModel::GetPageNumType() const
{
	return SVX_ARABIC;
}

const SdrPage* SdrModel::GetPage(sal_uInt16 nPgNum) const
{
	DBG_ASSERT(nPgNum < maPages.Count(), "SdrModel::GetPage: Access out of range (!)");
	return (SdrPage*)(maPages.GetObject(nPgNum));
}

SdrPage* SdrModel::GetPage(sal_uInt16 nPgNum)
{
	DBG_ASSERT(nPgNum < maPages.Count(), "SdrModel::GetPage: Access out of range (!)");
	return (SdrPage*)(maPages.GetObject(nPgNum));
}

sal_uInt16 SdrModel::GetPageCount() const
{
	return sal_uInt16(maPages.Count());
}

void SdrModel::PageListChanged()
{
}

const SdrPage* SdrModel::GetMasterPage(sal_uInt16 nPgNum) const
{
	DBG_ASSERT(nPgNum < maMaPag.Count(), "SdrModel::GetMasterPage: Access out of range (!)");
	return (SdrPage*)(maMaPag.GetObject(nPgNum));
}

SdrPage* SdrModel::GetMasterPage(sal_uInt16 nPgNum)
{
	DBG_ASSERT(nPgNum < maMaPag.Count(), "SdrModel::GetMasterPage: Access out of range (!)");
	return (SdrPage*)(maMaPag.GetObject(nPgNum));
}

sal_uInt16 SdrModel::GetMasterPageCount() const
{
	return sal_uInt16(maMaPag.Count());
}

void SdrModel::MasterPageListChanged()
{
}

void SdrModel::SetSdrUndoManager( SfxUndoManager* pUndoManager )
{
	mpImpl->mpUndoManager = pUndoManager;
}

SdrUndoFactory& SdrModel::GetSdrUndoFactory() const
{
	if( !mpImpl->mpUndoFactory )
		mpImpl->mpUndoFactory = new SdrUndoFactory;
	return *mpImpl->mpUndoFactory;
}

void SdrModel::SetSdrUndoFactory( SdrUndoFactory* pUndoFactory )
{
	if( pUndoFactory && (pUndoFactory != mpImpl->mpUndoFactory) )
	{
		delete mpImpl->mpUndoFactory;
		mpImpl->mpUndoFactory = pUndoFactory;
	}
}

/** cl: added this for OJ to complete his reporting engine, does not work
	correctly so only enable it for his model */
bool SdrModel::IsAllowShapePropertyChangeListener() const
{
	return mpImpl && mpImpl->mbAllowShapePropertyChangeListener;
}

void SdrModel::SetAllowShapePropertyChangeListener( bool bAllow )
{
	if( mpImpl )
	{
		mpImpl->mbAllowShapePropertyChangeListener = bAllow;
	}
}

const ::com::sun::star::uno::Sequence< sal_Int8 >& SdrModel::getUnoTunnelImplementationId()
{
	static ::com::sun::star::uno::Sequence< sal_Int8 > * pSeq = 0;
	if( !pSeq )
	{
		::osl::MutexGuard aGuard( ::osl::Mutex::getGlobalMutex() );
		if( !pSeq )
		{
			static Sequence< sal_Int8 > aSeq( 16 );
			rtl_createUuid( (sal_uInt8*)aSeq.getArray(), 0, sal_True );
			pSeq = &aSeq;
		}
	}
	return *pSeq;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TYPEINIT1(SdrHint,SfxHint);

SdrHint::SdrHint()
:	mpPage(0L),
	mpObj(0L),
	mpObjList(0L),
	meHint(HINT_UNKNOWN)
{
}

SdrHint::SdrHint(SdrHintKind eNewHint)
:	mpPage(0L),
	mpObj(0L),
	mpObjList(0L),
	meHint(eNewHint)
{
}

SdrHint::SdrHint(const SdrObject& rNewObj)
:	mpPage(rNewObj.GetPage()),
	mpObj(&rNewObj),
	mpObjList(rNewObj.GetObjList()),
	meHint(HINT_OBJCHG)
{
	maRectangle = rNewObj.GetLastBoundRect();
}

SdrHint::SdrHint(const SdrObject& rNewObj, const Rectangle& rRect)
:	mpPage(rNewObj.GetPage()),
	mpObj(&rNewObj),
	mpObjList(rNewObj.GetObjList()),
	meHint(HINT_OBJCHG)
{
	maRectangle = rRect;
}

void SdrHint::SetPage(const SdrPage* pNewPage)
{
	mpPage = pNewPage;
}

void SdrHint::SetObjList(const SdrObjList* pNewOL)
{
	mpObjList = pNewOL;
}

void SdrHint::SetObject(const SdrObject* pNewObj)
{
	mpObj = pNewObj;
}

void SdrHint::SetKind(SdrHintKind eNewKind)
{
	meHint = eNewKind;
}

void SdrHint::SetRect(const Rectangle& rNewRect)
{
	maRectangle = rNewRect;
}

const SdrPage* SdrHint::GetPage() const
{
	return mpPage;
}

const SdrObjList* SdrHint::GetObjList() const
{
	return mpObjList;
}

const SdrObject* SdrHint::GetObject() const
{
	return mpObj;
}

SdrHintKind SdrHint::GetKind() const
{
	return meHint;
}

const Rectangle& SdrHint::GetRect() const
{
	return maRectangle;
}

// eof
