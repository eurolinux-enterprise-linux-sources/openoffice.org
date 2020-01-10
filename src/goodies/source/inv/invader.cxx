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
#include "invader.hxx"
#include "monst.hxx"
#include "shapes.hxx"
#include "expl.hxx"
#include "invader.hrc"
#include "strings.hrc"
#include "score.hxx"
#include <svtools/solar.hrc>
#include <cppuhelper/servicefactory.hxx>
#include <comphelper/processfactory.hxx>
#include <vcl/msgbox.hxx>
#include <vcl/virdev.hxx>

MyWindow::MyWindow(Window* pParent, ResMgr *pResMgr ) :
			FloatingWindow(pParent, WB_3DLOOK | WB_CLOSEABLE | WB_MOVEABLE ),
			pRes(pResMgr),
			ProgStatus(FALSE),
			bEndLevel(TRUE),
			bFightDest(FALSE),
			bTimeHigh(TRUE),
			bPause(FALSE),
			bAuseModus(FALSE),
			bWaitDlg(FALSE),
			StartPhase(1),
			nLevel(1),
			nScore(0L),
			nHighScore(0L),
			nFighter(3),
            nTimeOut(TIMEHIGH),
			nAuseCount(0),
			pBitWelcome2(0L),
			pBitHeros(0L),
			pBitStarWars(0L),
			pGegner(0L),
			pFighter(0L),
			pMunition(0L),
			pExplosion(0L),
			pBombe(0L),
			pWall(0L),
			pScoreWindow(0L),
			pVirtualDevice(0L),
			pPauseWindow(0L),
			pBox(0L),
			nDirection(0L),
			bMouseMooving(FALSE)
{
	pBitWelcome2 = ImplLoadImage( WELCOME2, GetResMgr() );
	pBitHeros = ImplLoadImage( HEROS, GetResMgr() );
	pBitStarWars = ImplLoadImage( STARWARS, GetResMgr() );
	pBombe = new Bombe(GetResMgr());
	pMunition = new Munition(GetResMgr());
	pExplosion = new Explosion(GetResMgr());
	pWall = new Wall(GetResMgr());
	pScoreWindow = new ScoreWindow(this,WB_BORDER, GetResMgr());
	pPauseWindow = new Window(this, 0);

	SetOutputSizePixel(Size(WINWIDTH,WINHEIGHT));
	SetText(String(ResId(STR_APP_TITLE,*GetResMgr())));

	pPauseWindow->SetPosSizePixel(Point(100,190),Size(500,50));
	pPauseWindow->SetBackground();

	pScoreWindow->SetPosSizePixel(Point(0,0),Size(WINWIDTH,22));
	Size aSize(WINWIDTH,WINHEIGHT);
	pFighter = new Fighter(aSize, GetResMgr());
	pGegner  = new Gegner(pFighter, pBombe, GetResMgr());

	pVirtualDevice = new VirtualDevice(*this);
	pVirtualDevice->SetOutputSizePixel(Size( WINWIDTH, WINHEIGHT ));

	SetBackground();

	aPaintTimer = Timer();
	aPaintTimer.SetTimeoutHdl(LINK(this, MyWindow, PaintTimer));
	aPaintTimer.SetTimeout(nTimeOut);
	aPaintTimer.Stop();

	aWaitDlgTimer = Timer();
	aWaitDlgTimer.SetTimeoutHdl(LINK(this, MyWindow, StartDlgTimer));
	aWaitDlgTimer.SetTimeout(10);
	aWaitDlgTimer.Stop();

	aPointer = GetPointer();

	Show();

}

MyWindow::~MyWindow()
{
	aPaintTimer.Stop();
	delete pVirtualDevice;
	delete pGegner;
	delete pFighter;
	delete pMunition;
	delete pExplosion;
	delete pBombe;
	delete pWall;
	delete pBitHeros;
	delete pBitStarWars;
	delete pBitWelcome2;
	delete pScoreWindow;
	delete pPauseWindow;
}

void MyWindow::Paint(const Rectangle&)
{
	if(bWaitDlg)
		return;

	Size aVSize = pVirtualDevice->GetOutputSizePixel();

	if (!ProgStatus)
	{
		if (StartPhase < 3)
		{
			pScoreWindow->Hide();
			bMouseMooving = FALSE;

			if( StartPhase == 1 )
			{
				const Font aOldFont( GetFont() );
				Font aFont( aOldFont );

				SetFillColor( COL_BLACK );
				DrawRect(Rectangle(Point(0,0),Point(640,480)));

				DrawImage(Point(55,78), *pBitStarWars);
				DrawImage(Point(53,207), *pBitHeros);
				DrawImage(Point(470,400), *(pFighter->pFight1));
				DrawImage(Point(460,290), *(pMunition->pMunition1));
				DrawImage(Point(480,320), *(pMunition->pMunition1));
				DrawImage(Point(470,250), *(pMunition->pMunition1));

				aFont.SetColor(COL_WHITE);
				aFont.SetFillColor(COL_BLACK);
				SetFont( aFont );

				DrawText(Point(175,170),String(ResId(STR_CHOOSEHERO, *GetResMgr())));
				DrawText(Point(98,208),String(ResId(STR_NAME1, *GetResMgr())));
				DrawText(Point(98,252),String(ResId(STR_NAME2, *GetResMgr())));
				DrawText(Point(98,296),String(ResId(STR_NAME3, *GetResMgr())));
				DrawText(Point(98,340),String(ResId(STR_NAME4, *GetResMgr())));
				DrawText(Point(98,384),String(ResId(STR_NAME5, *GetResMgr())));

				SetFont( aOldFont );
			}
			else if(StartPhase == 2)
			{
				SetFillColor( COL_WHITE );
				DrawRect(Rectangle(Point(0,0),Point(640,480)));
				DrawImage(Point(19,147), *pBitWelcome2 );
			}
		}
	}
	else if(!bPause)
	{
		aPaintTimer.Start();

		Point aPoint(0,20);

		pVirtualDevice->SetFillColor( COL_WHITE );
		pVirtualDevice->SetLineColor();
		pVirtualDevice->DrawRect(Rectangle(Point(0,0), Point(aVSize.Width(),aVSize.Height())));

		if(!bStartLevel)
		{
			if ((bMouseMooving)&&(nDirection > pFighter->GetPoint().X()+32))
			{
				pFighter->Move(1);
				pFighter->Move(1);
				pGegner->SetAuseMode(FALSE);
				nAuseCount = 0;
			}
			else if ((bMouseMooving)&&(nDirection < pFighter->GetPoint().X()))
			{
				pFighter->Move(-1);
				pFighter->Move(-1);
				pGegner->SetAuseMode(FALSE);
				nAuseCount = 0;
			}
			nAuseCount++;
			if(nAuseCount > AUSEMODE)
				pGegner->SetAuseMode(TRUE);
			pWall->Paint(*pVirtualDevice);
			pGegner->DrawGegner(pVirtualDevice,&aPoint);
			pFighter->Paint(*pVirtualDevice);
			pMunition->Paint(*pVirtualDevice);
			pBombe->Paint(*pVirtualDevice);
			pExplosion->Paint(*pVirtualDevice);
		}
		else
		{
			bMouseMooving = FALSE;
			Font aOldFont = pVirtualDevice->GetFont();
			Font aFont = aOldFont;
			switch(nStartLevel)
			{
				case 0:
					aFont.SetColor(COL_RED);
					break;
				case 1:
					aFont.SetColor(COL_YELLOW);
					break;
				case 2:
					aFont.SetColor(COL_GREEN);
					break;
			}
			pVirtualDevice->SetFont(*&aFont);
			String aString = String(ResId(STR_LEVELSTART, *GetResMgr()));
			aString += ' ';
			aString += String::CreateFromInt32(nLevel);
			if(nStartLevel < 3)
				pVirtualDevice->DrawText(Point(LEVELTEXTX,LEVELTEXTY),aString);
			nStartLevel++;
			if(nStartLevel > 3)
			{
				bStartLevel = FALSE;
				bEndLevel    = FALSE;
				bFightDest    = FALSE;
				aPaintTimer.SetTimeout(nTimeOut);
			}
			pVirtualDevice->SetFont(*&aOldFont);

		}

		DrawOutDev( Point( 0, 22), aVSize, Point( 0, 22), aVSize, *pVirtualDevice);

		if(!bStartLevel)
			Kollision();
	}
	else pPauseWindow->DrawText(Point(20,20),String(ResId(STR_PAUSE, *GetResMgr())));
}

void MyWindow::KeyInput( const KeyEvent& rKEvent)
{
	if(bEndLevel || bFightDest)
	{
		rKEvent.GetKeyCode().GetCode();
			FloatingWindow::KeyInput(rKEvent);

		return;
	}

	bMouseMooving = FALSE;
	switch(rKEvent.GetKeyCode().GetCode())
	{
		case KEY_LEFT:
			pFighter->Move(-1);
			nAuseCount = 0;
				pGegner->SetAuseMode(FALSE);
			break;
		case KEY_RIGHT:
			pFighter->Move(1);
			nAuseCount = 0;
				pGegner->SetAuseMode(FALSE);
			break;
		case KEY_SPACE:
			pMunition->Start(pFighter->GetPoint());
			break;
		case KEY_L:
			if(nLevel > 1)
				return;
			if(bTimeHigh)
			{
				bTimeHigh     = FALSE;
				nTimeOut    = TIMELOW;
			}
			else
			{
				bTimeHigh     = TRUE;
				nTimeOut    = TIMEHIGH;
			}
			aPaintTimer.SetTimeout(nTimeOut);
			break;
		case KEY_ADD:
			if(nLevel > 1)
				return;
			nTimeOut++;
			aPaintTimer.SetTimeout(nTimeOut);
			break;
		case KEY_SUBTRACT:
			if(nLevel > 1)
				return;
			nTimeOut--;
			if(nTimeOut <= 0)
				nTimeOut = 1;
			aPaintTimer.SetTimeout(nTimeOut);
			aPaintTimer.Start();
			break;
		case KEY_P:
			if(!ProgStatus)
				return;
			if(bPause)
			{
				aPaintTimer.Start();
				bPause = FALSE;
				SetBackground();
				pPauseWindow->Hide();
			}
			else
			{
				aPaintTimer.Stop();
				bPause = TRUE;
				SetBackground( Wallpaper( COL_WHITE ) );
				pPauseWindow->Show();
				pPauseWindow->DrawText(Point(20,20),String(ResId(STR_PAUSE, *GetResMgr())));
			}
			break;
		default:
			FloatingWindow::KeyInput(rKEvent);
	}
}

IMPL_LINK( MyWindow, PaintTimer, Timer*, EMPTYARG)
{

	Invalidate();

	return 0;
}

IMPL_LINK( MyWindow, StartDlgTimer, Timer*, EMPTYARG )
{
	bWaitDlg = TRUE;

	aWaitDlgTimer.Stop();

	String	aString;
	if(!bFightDest)
	{
		aString = String(ResId(STR_GRAT1,*GetResMgr()));
		aString += String::CreateFromInt32(nLevel);
		aString += String(ResId(STR_GRAT2,*GetResMgr()));

		pBox = new MessBox(this,WinBits(WB_OK),String(ResId(STR_APP_TITLE,*GetResMgr())),aString);
		PlaceDialog(pBox);
		ProgStatus=FALSE;
		SetBackground(Wallpaper( Color( COL_WHITE ) ) );

		pBox->Execute();

		SetBackground();
		ProgStatus=TRUE;
		delete pBox;
		nLevel++;
		GrabFocus();
	}
	else
	{
		nFighter--;
		pScoreWindow->SetLives(nFighter);
		if(nFighter > 0)
		{
			aString = String(ResId(STR_FIGHTDEST1,*GetResMgr()));
			aString += String::CreateFromInt32(nFighter);
			aString += String(ResId(STR_FIGHTDEST2,*GetResMgr()));
		}
		else
		{
			aString = String(ResId(STR_GAMEOVER,*GetResMgr()));
			ProgStatus = FALSE;
			nLevel	   = 1;
			nFighter   = 3;
			pGegner->SetRandWert(200);
			pGegner->SetDown(10);

			if(bTimeHigh)
				nTimeOut = TIMEHIGH;
			else
				nTimeOut = TIMELOW;
		}

		pBox = new MessBox(this,WinBits(WB_OK),String(ResId(STR_APP_TITLE,*GetResMgr())),aString);
		PlaceDialog(pBox);
		BOOL aDummyStatus = ProgStatus;
		ProgStatus=FALSE;
		SetBackground(Wallpaper( Color( COL_WHITE ) ) );

		pBox->Execute();

		SetBackground();
		ProgStatus=aDummyStatus;
		delete pBox;
		GrabFocus();

		if(!ProgStatus)
		{
			StartPhase = 1;
			if (nScore > nHighScore)
			{
				String	aHSString;
				aHSString = String(ResId(STR_HIGHSCORE1,*GetResMgr()));
				aHSString += String::CreateFromInt32(nScore);
				aHSString += String(ResId(STR_HIGHSCORE2,*GetResMgr()));
				aHSString += String::CreateFromInt32(nHighScore);
				aHSString += String(ResId(STR_HIGHSCORE3,*GetResMgr()));
				nHighScore = nScore;
				pBox = new MessBox(this,WinBits(WB_OK),String(ResId(STR_APP_TITLE,*GetResMgr())),aHSString);
				PlaceDialog(pBox);
				Paint(Rectangle(Point(0,0),Point(640,480)));
				SetBackground(Wallpaper( Color( COL_WHITE ) ) );
				pBox->Execute();
				SetBackground();
				delete pBox;
				GrabFocus();
			}
			nScore	   = 0;
		}

	}

	InitLevel();
	Invalidate();

	bWaitDlg = FALSE;

	return 0;
}

void MyWindow::MouseButtonDown(const MouseEvent& rMEvt)
{
	MessBox* pMessBox = 0L;

	if (!ProgStatus)
	{
		TheHero = 0;

		if (StartPhase == 1)
		{
			if ((rMEvt.GetPosPixel().X() >= 57) && (rMEvt.GetPosPixel().X() <=90))
			{
				if ((rMEvt.GetPosPixel().Y() >= 211) && (rMEvt.GetPosPixel().Y() <= 244))
					TheHero = STR_NAME1;
				else if ((rMEvt.GetPosPixel().Y() >= 255) && (rMEvt.GetPosPixel().Y() <= 288))
					TheHero = STR_NAME2;
				else if ((rMEvt.GetPosPixel().Y() >= 299) && (rMEvt.GetPosPixel().Y() <= 332))
					TheHero = STR_NAME3;
				else if ((rMEvt.GetPosPixel().Y() >= 343) && (rMEvt.GetPosPixel().Y() <= 376))
					TheHero = STR_NAME4;
				else if ((rMEvt.GetPosPixel().Y() >= 387) && (rMEvt.GetPosPixel().Y() <= 420))
					TheHero = STR_NAME5;
			}
			if (TheHero)
			{
				SetPointer(aPointer);
				switch (TheHero)
				{
					case STR_NAME1:
						{
							pMessBox = new MessBox(this,WinBits(WB_OK),String(ResId(STR_APP_TITLE,*GetResMgr())),String(ResId(STR_HERO1, *GetResMgr())));
						}
						break;
					case STR_NAME2:
						{
							pMessBox = new MessBox(this,WinBits(WB_OK),String(ResId(STR_APP_TITLE,*GetResMgr())), String(ResId(STR_HERO2, *GetResMgr())));
						}
						break;
					case STR_NAME3:
						{
							pMessBox = new MessBox(this,WinBits(WB_OK),String(ResId(STR_APP_TITLE,*GetResMgr())), String(ResId(STR_HERO3, *GetResMgr())));
						}
						break;
					case STR_NAME4:
						{
							pMessBox = new MessBox(this,WinBits(WB_OK),String(ResId(STR_APP_TITLE,*GetResMgr())), String(ResId(STR_HERO4, *GetResMgr())));
						}
						break;
					case STR_NAME5:
						{
							pMessBox = new MessBox(this,WinBits(WB_OK),String(ResId(STR_APP_TITLE,*GetResMgr())), String(ResId(STR_HERO5, *GetResMgr())));
						}
						break;
				}
				PlaceDialog(pMessBox);
				pMessBox->Execute();
				delete pMessBox;
				StartPhase++;
				pScoreWindow->SetHero(TheHero);
				Invalidate();
			}
		}
		else if (StartPhase == 2)
		{
			ProgStatus = TRUE;
			StartPhase++;
			pScoreWindow->ShowMe();
			InitLevel();
			Invalidate();
			aPaintTimer.Start();
		}
	}
	else
	{
		if((!bStartLevel) && (!bPause))
			pMunition->Start(pFighter->GetPoint());
	}
}

void MyWindow::MouseMove(const MouseEvent& rMEvt)
{
	long Hero;

	if ((!ProgStatus) && ( StartPhase == 1 ))
	{
		Hero = 0;
		if ((rMEvt.GetPosPixel().X() >= 57) && (rMEvt.GetPosPixel().X() <=90))
		{
			if ((rMEvt.GetPosPixel().Y() >= 211) && (rMEvt.GetPosPixel().Y() <= 244))
				Hero = 1;
			else if ((rMEvt.GetPosPixel().Y() >= 255) && (rMEvt.GetPosPixel().Y() <= 288))
				Hero = 2;
			else if ((rMEvt.GetPosPixel().Y() >= 299) && (rMEvt.GetPosPixel().Y() <= 332))
				Hero = 3;
			else if ((rMEvt.GetPosPixel().Y() >= 343) && (rMEvt.GetPosPixel().Y() <= 376))
				Hero = 4;
			else if ((rMEvt.GetPosPixel().Y() >= 387) && (rMEvt.GetPosPixel().Y() <= 420))
				Hero = 5;
		}
		if ((Hero) && (GetPointer() != POINTER_REFHAND)) SetPointer(POINTER_REFHAND);
		else if ((!Hero) && (GetPointer() != aPointer)) SetPointer(aPointer);
	}
	else if ( ProgStatus )
	{
		nDirection = rMEvt.GetPosPixel().X();
		bMouseMooving = TRUE;
	}
}

BOOL MyWindow::Close()
{
	Hide();
#ifdef TEST
	pApplication->Quit();
#else
	delete this;
#endif

	return TRUE;
}

void MyWindow::PlaceDialog(MessBox* pMessBox)
{
	if(pMessBox)
    {
        Point aPos = GetPosPixel();
        Size aSize = GetSizePixel();
        Size aBoxSize = pMessBox->GetOutputSizePixel();
        aSize = Size(aSize.Width()/2 - aBoxSize.Width()/2,
                        aSize.Height()/2 - aBoxSize.Height()/2);
		pMessBox->SetPosPixel(
				Point(aPos.X()+aSize.Width(),aPos.Y()+aSize.Height()));
    }
}

#ifdef TEST
void MyApp::Main()
{
	::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >
         xMSF = cppu::createRegistryServiceFactory(
             rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "applicat.rdb" ) ), sal_True );

    ::comphelper::setProcessServiceFactory( xMSF );

	ByteString aRes = "tfu";
    ResMgr *pMyResMgr = ResMgr::CreateResMgr(aRes.GetBuffer());

	MyWindow aWindow( NULL, pMyResMgr);

#ifdef TEST
	aWindow.setApplication(this);
#endif

	Execute();

	delete pMyResMgr;
}

MyApp aMyApp;
#else

#ifdef WNT
extern "C" void __cdecl StartInvader(Window* pParent , ResMgr* pRes)
#else
extern "C" void StartInvader(Window* pParent, ResMgr* pRes )
#endif
{
	MyWindow *pInvaderWin = new MyWindow(pParent, pRes);
	pInvaderWin->Show();
}
#endif

