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
#include <svx/svdview.hxx>
#include <svx/svddrag.hxx>

void SdrDragStat::Clear(FASTBOOL bLeaveOne)
{
	void* pP=aPnts.First();
	while (pP!=NULL) {
		delete (Point*)pP;
		pP=aPnts.Next();
	}
	if (pUser!=NULL) delete pUser;
	pUser=NULL;
	aPnts.Clear();
	if (bLeaveOne) {
		aPnts.Insert(new Point,CONTAINER_APPEND);
	}
}

void SdrDragStat::Reset()
{
	pView=NULL;
	pPageView=NULL;
	bShown=FALSE;
	nMinMov=1;
	bMinMoved=FALSE;
	bHorFixed=FALSE;
	bVerFixed=FALSE;
	bWantNoSnap=FALSE;
	pHdl=NULL;
	bOrtho4=FALSE;
	bOrtho8=FALSE;
	pDragMethod=NULL;
	bEndDragChangesAttributes=FALSE;
	bEndDragChangesGeoAndAttributes=FALSE;
	bMouseIsUp=FALSE;
	Clear(TRUE);
	aActionRect=Rectangle();
}

void SdrDragStat::Reset(const Point& rPnt)
{
	Reset();
	Start()=rPnt;
	aPos0=rPnt;
	aRealPos0=rPnt;
	RealNow()=rPnt;
}

void SdrDragStat::NextMove(const Point& rPnt)
{
	aRealPos0=GetRealNow();
	aPos0=GetNow();
	RealNow()=rPnt;
	Point aBla=KorregPos(GetRealNow(),GetPrev());
	Now()=aBla;
}

void SdrDragStat::NextPoint(FASTBOOL bSaveReal)
{
	Point aPnt(GetNow());
	if (bSaveReal) aPnt=aRealNow;
	aPnts.Insert(new Point(KorregPos(GetRealNow(),aPnt)),CONTAINER_APPEND);
	Prev()=aPnt;
}

void SdrDragStat::PrevPoint()
{
	if (aPnts.Count()>=2) { // einer muss immer da bleiben
		Point* pPnt=(Point*)(aPnts.GetObject(aPnts.Count()-2));
		aPnts.Remove(aPnts.Count()-2);
		delete pPnt;
		Now()=KorregPos(GetRealNow(),GetPrev());
	}
}

Point SdrDragStat::KorregPos(const Point& rNow, const Point& /*rPrev*/) const
{
	Point aRet(rNow);
	return aRet;
}

FASTBOOL SdrDragStat::CheckMinMoved(const Point& rPnt)
{
	if (!bMinMoved) {
		long dx=rPnt.X()-GetPrev().X(); if (dx<0) dx=-dx;
		long dy=rPnt.Y()-GetPrev().Y(); if (dy<0) dy=-dy;
		if (dx>=long(nMinMov) || dy>=long(nMinMov))
			bMinMoved=TRUE;
	}
	return bMinMoved;
}

Fraction SdrDragStat::GetXFact() const
{
	long nMul=GetNow().X()-aRef1.X();
	long nDiv=GetPrev().X()-aRef1.X();
	if (nDiv==0) nDiv=1;
	if (bHorFixed) { nMul=1; nDiv=1; }
	return Fraction(nMul,nDiv);
}

Fraction SdrDragStat::GetYFact() const
{
	long nMul=GetNow().Y()-aRef1.Y();
	long nDiv=GetPrev().Y()-aRef1.Y();
	if (nDiv==0) nDiv=1;
	if (bVerFixed) { nMul=1; nDiv=1; }
	return Fraction(nMul,nDiv);
}

void SdrDragStat::TakeCreateRect(Rectangle& rRect) const
{
	rRect=Rectangle(GetStart(),GetNow());
	if (GetPointAnz()>=2) {
		Point aBtmRgt(GetPoint(1));
		rRect.Right()=aBtmRgt.X();
		rRect.Bottom()=aBtmRgt.Y();
	}
	if (pView!=NULL && pView->IsCreate1stPointAsCenter()) {
		rRect.Top()+=rRect.Top()-rRect.Bottom();
		rRect.Left()+=rRect.Left()-rRect.Right();
	}
}

