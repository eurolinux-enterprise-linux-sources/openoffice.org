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
#include "precompiled_goodies.hxx"
#include "shapes.hxx"
#include "invader.hrc"
#include "expl.hxx"
#include <vcl/outdev.hxx>
#include <vcl/virdev.hxx>
#include <vcl/bitmap.hxx>

Image* ImplLoadImage( USHORT nId, ResMgr* pRes )
{
	return new Image( Bitmap( ResId( nId, *pRes ) ), COL_WHITE );
}

// ------------------------------------------------------------------------

Fighter::Fighter(Size& rOut, ResMgr* pRes) :
			pFightr(NULL),
			pFightl(NULL),
			eMode(FIGHT_NORM),
			bNoKey(FALSE),
			nDelay(0),
			pFight1(NULL)
{
	pFight1 = ImplLoadImage( FIGHTER1, pRes );
	pFightr = ImplLoadImage( FIGHTERR, pRes );
	pFightl = ImplLoadImage( FIGHTERL, pRes );

	aSize = pFight1->GetSizePixel();

	aOut.X() = rOut.Width()/2-aSize.Width()/2;
	aOut.Y() = rOut.Height()-aSize.Height()-10;

	aStart = aOut;

	nMax = rOut.Width() - aSize.Width();
}

Fighter::~Fighter()
{

	delete pFight1;
	delete pFightr;
	delete pFightl;
}

void Fighter::Init()
{
	aOut  = aStart;
	eMode = FIGHT_NORM;
}

void Fighter::Paint(OutputDevice& rDev)
{
	if(eMode == FIGHT_DEST)
		return;

	switch(eMode)
	{
		case FIGHT_NORM:
			rDev.DrawImage(aOut,*pFight1);
			break;
		case FIGHT_RIGHT:
			rDev.DrawImage(aOut,*pFightr);
			break;
		case FIGHT_LEFT:
			rDev.DrawImage(aOut,*pFightl);
			break;
		case FIGHT_DEST:
			break;
	}

	if(nDelay <= 0)
		eMode = FIGHT_NORM;
	else
		nDelay--;

	bNoKey = FALSE;
}

void Fighter::Move(long nDirect)
{
	if(eMode == FIGHT_DEST)
		return;

	if(nDirect < 0)
	{
		aOut.X() -= KANONEX;
		eMode = FIGHT_LEFT;
	}
	else
	{
		aOut.X() += KANONEX;
		eMode = FIGHT_RIGHT;
	}

	nDelay = 5;

	if(aOut.X() <= 0)
		aOut.X() = 1;
	if(aOut.X() > nMax)
		aOut.X() = nMax;

	bNoKey = TRUE;
}

BOOL Fighter::Kollision(Rectangle aRect, Explosion* pExpl)
{
		if((aOut.X() <= aRect.Left() && aOut.X()+aSize.Width() >= aRect.Right()) &&
			(aOut.Y() <= aRect.Top() && aOut.Y()+aSize.Height() >= aRect.Bottom()))
		{
			pExpl->InsertExpl(aOut);
			eMode = FIGHT_DEST;
			return TRUE;
		}

	return FALSE;
}

Munition::Munition(ResMgr* pRes) :
			MunitionListe(0,1),
			pMunition2( NULL ),
			pMunition1( NULL )
{
	pMunition1 = ImplLoadImage( MUNITION1,pRes );
	pMunition2 = ImplLoadImage( MUNITION2,pRes );

	aSize = pMunition1->GetSizePixel();
}

Munition::~Munition()
{
	ClearAll();

	delete pMunition1;
	delete pMunition2;
}

void Munition::Start(Point& rPoint)
{
	if( Count() >= MUNMAX)
		return;


	Munition_Impl* pWork = new Munition_Impl();

	pWork->aPoint = rPoint;
	pWork->eMode = MUNI_MODE1;

	Insert(pWork);
}

void Munition::Paint(OutputDevice& rDev)
{
	unsigned long i;
	for(i=0; i<Count();i++)
	{
		switch(GetMode(i))
		{
			case MUNI_MODE1:
				rDev.DrawImage(GetPoint(i),*pMunition1);
				SetMode(i,MUNI_MODE2);
				break;
			case MUNI_MODE2:
				rDev.DrawImage(GetPoint(i),*pMunition2);
				SetMode(i,MUNI_MODE1);
				break;
			case MUNI_DEL:
				break;
		}

		SetKoll(i,Rectangle(Point(GetPoint(i).X()+aSize.Width()/2,GetPoint(i).Y()),
					Point(GetPoint(i).X()+aSize.Width()/2,
							GetPoint(i).Y())));


		SetPoint(i,Point(GetPoint(i).X(),GetPoint(i).Y() - MUNIY));
		if(GetPoint(i).Y() <= aSize.Height()*-1)
		{
			SetMode(i,MUNI_DEL);
		}
	}
}

long Munition::RemoveMunition()
{
	for(long i=Count()-1; i>=0; i--)
	{
		if(GetMode(i) == MUNI_DEL)
		{
			Munition_Impl* pWork = GetObject(i);
			Remove(pWork);
			delete pWork;
		}
	}

	return 5-Count();
}

void Munition::ClearAll()
{
	for(long i=Count()-1; i>=0; i--)
		delete	GetObject(i);

	Clear();
}

Bombe::Bombe(ResMgr* pRes) :
		BombenListe(0,1)
{
	pBombe = ImplLoadImage( BOMBE,pRes );

	aSize = pBombe->GetSizePixel();
	nSpeed = BOMBEY;
}

Bombe::~Bombe()
{
	ClearAll();

	delete pBombe;
}

void Bombe::Paint(OutputDevice& rDev)
{
	unsigned long i;
	for(i=0; i<Count();i++)
	{
		rDev.DrawImage(GetPoint(i),*pBombe);
		SetKoll(i,Rectangle(Point(GetPoint(i).X()+aSize.Width()/2,
								GetPoint(i).Y()+aSize.Height()),
						Point(GetPoint(i).X()+aSize.Width()/2,
							GetPoint(i).Y()+aSize.Height())));

		SetPoint(i,Point(GetPoint(i).X(),GetPoint(i).Y() + MUNIY));
	}
}

void Bombe::InsertBombe(const Point& rPoint)
{
	Bombe_Impl* pWork = new Bombe_Impl;
	pWork->aXY = rPoint;
	pWork->bDelete = FALSE;
	Insert(pWork);
}

void Bombe::RemoveBomben()
{
	for(long i=Count()-1; i>=0; i--)
	{
		Bombe_Impl* pWork = GetObject(i);
		if(pWork->bDelete)
		{
			Remove(pWork);
			delete pWork;
		}
	}
}

void Bombe::ClearAll()
{
	unsigned long i;
	for(i=0;i<Count();i++)
		delete GetObject(i);
	Clear();
}

BOOL Bombe::Kollision(Rectangle aRect, Explosion* pExpl)
{
	unsigned long i;
	for(i=0;i<Count();i++)
	{
		if((GetPoint(i).X() <= aRect.Left() && GetPoint(i).X()+aSize.Width() >= aRect.Right()) &&
			(GetPoint(i).Y() <= aRect.Top() && GetPoint(i).Y()+aSize.Height() >= aRect.Bottom()))
		{
			pExpl->InsertExpl(GetPoint(i));
			SetDelete(i);
			return TRUE;
		}
	}

	return FALSE;
}

Wall::Wall(ResMgr* pRes) :
			WallListe(0,1),
			pWall1(0L),
			pWall2(0L),
			pWall3(0L),
			pWall4(0L),
			pWall5(0L),
			pWall6(0L),
			pWall7(0L),
			pWall8(0L),
			pWall9(0L),
			pWall10(0L)
{
	pWall1 	= ImplLoadImage( WALL1,pRes );
	pWall2 	= ImplLoadImage( WALL2,pRes );
	pWall3 	= ImplLoadImage( WALL3,pRes );
	pWall4 	= ImplLoadImage( WALL4,pRes );
	pWall5 	= ImplLoadImage( WALL5,pRes );
	pWall6 	= ImplLoadImage( WALL6,pRes );
	pWall7 	= ImplLoadImage( WALL7,pRes );
	pWall8 	= ImplLoadImage( WALL8,pRes );
	pWall9 	= ImplLoadImage( WALL9,pRes );
	pWall10	= ImplLoadImage( WALL10,pRes );

	aSize = pWall1->GetSizePixel();
}

Wall::~Wall()
{
	ClearAll();

	delete pWall1;
	delete pWall2;
	delete pWall3;
	delete pWall4;
	delete pWall5;
	delete pWall6;
	delete pWall7;
	delete pWall8;
	delete pWall9;
	delete pWall10;
}

void Wall::Paint(OutputDevice& rDev)
{
	unsigned long i;
	for(i=0; i<Count(); i++)
	{
		switch(GetMode(i))
		{
			case WALL_MOD1:
				rDev.DrawImage(GetPoint(i),*pWall1);
				break;
			case WALL_MOD2:
				rDev.DrawImage(GetPoint(i),*pWall2);
				break;
			case WALL_MOD3:
				rDev.DrawImage(GetPoint(i),*pWall3);
				break;
			case WALL_MOD4:
				rDev.DrawImage(GetPoint(i),*pWall4);
				break;
			case WALL_MOD5:
				rDev.DrawImage(GetPoint(i),*pWall5);
				break;
			case WALL_MOD6:
				rDev.DrawImage(GetPoint(i),*pWall6);
				break;
			case WALL_MOD7:
				rDev.DrawImage(GetPoint(i),*pWall7);
				break;
			case WALL_MOD8:
				rDev.DrawImage(GetPoint(i),*pWall8);
				break;
			case WALL_MOD9:
				rDev.DrawImage(GetPoint(i),*pWall9);
				break;
			case WALL_MOD10:
				rDev.DrawImage(GetPoint(i),*pWall10);
				break;
			case WALL_DEL:
				break;
		}

	}
}

void Wall::InsertWall(const Point& rPoint)
{
	Wall_Impl* pWork = new Wall_Impl();

	pWork->aXY = rPoint;
	pWork->aXY.Y() = WALLY;
	pWork->eMode = WALL_MOD1;

	Insert(pWork);
}

void Wall::ClearAll()
{
	unsigned long i;
	for(i=0; i<Count(); i++)
		delete GetObject(i);

	Clear();
}

BOOL Wall::Kollision(Rectangle& rRect, BOOL bDel)
{
	BOOL nTreffer = FALSE;

	Rectangle aWork;

	unsigned long i;
	for(i=0; i<Count();i++)
	{

		Point aPoint = GetPoint(i);
		aWork = Rectangle(Point(aPoint.X(),aPoint.Y()-WALLKOLL),
							Point(aPoint.X()+aSize.Width(),
							aPoint.Y()+aSize.Height()-WALLKOLL));

		if((aWork.Left() <= rRect.Left() &&  aWork.Right() >= rRect.Right()) &&
			(aWork.Top() <= rRect.Top() && aWork.Bottom() >= rRect.Bottom()) &&
				GetMode(i) != WALL_DEL)
		{
			switch(GetMode(i))
			{
				case WALL_MOD1:
					SetMode(i,WALL_MOD2);
					break;
				case WALL_MOD2:
					SetMode(i,WALL_MOD3);
					break;
				case WALL_MOD3:
					SetMode(i,WALL_MOD4);
					break;
				case WALL_MOD4:
					SetMode(i,WALL_MOD5);
					break;
				case WALL_MOD5:
					SetMode(i,WALL_MOD6);
					break;
				case WALL_MOD6:
					SetMode(i,WALL_MOD7);
					break;
				case WALL_MOD7:
					SetMode(i,WALL_MOD8);
					break;
				case WALL_MOD8:
					SetMode(i,WALL_MOD9);
					break;
				case WALL_MOD9:
					SetMode(i,WALL_MOD10);
					break;
				case WALL_MOD10:
					SetMode(i,WALL_DEL);
					break;
				case WALL_DEL:
					break;
			}

			if(bDel)
				SetMode(i,WALL_DEL);

			nTreffer = TRUE;
		}
	}

	return nTreffer;
}
