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

#ifndef _CALENDAR_HXX
#define _CALENDAR_HXX

#include "svtools/svtdllapi.h"
#include <unotools/calendarwrapper.hxx>
#ifndef _COM_SUN_STAR_I18N_WEEKDAYS_HPP
#include <com/sun/star/i18n/Weekdays.hpp>
#endif

#ifndef _CTRL_HXX
#include <vcl/ctrl.hxx>
#endif
#include <vcl/timer.hxx>
#ifndef _FIELD_HXX
#include <vcl/field.hxx>
#endif

class Table;
class MouseEvent;
class TrackingEvent;
class KeyEvent;
class HelpEvent;
class DataChangedEvent;
class FloatingWindow;
class PushButton;
struct ImplDateInfo;
class ImplDateTable;
class ImplCFieldFloatWin;

/*************************************************************************

Beschreibung
============

class Calendar

Diese Klasse erlaubt die Auswahl eines Datum. Der Datumsbereich der
angezeigt wird, ist der, der durch die Klasse Date vorgegeben ist.
Es werden soviele Monate angezeigt, wie die Ausgabeflaeche des
Controls vorgibt. Der Anwender kann zwischen den Monaten ueber ein
ContextMenu (Bei Click auf den Monatstitel) oder durch 2 ScrollButtons
zwischen den Monaten wechseln.

--------------------------------------------------------------------------

WinBits

WB_BORDER					Um das Fenster wird ein Border gezeichnet.
WB_TABSTOP					Tastatursteuerung ist moeglich. Der Focus wird
							sich geholt, wenn mit der Maus in das
							Control geklickt wird.
WB_QUICKHELPSHOWSDATEINFO	DateInfo auch bei QuickInfo als BalloonHelp zeigen
WB_BOLDTEXT 				Formatiert wird nach fetten Texten und
							DIB_BOLD wird bei AddDateInfo() ausgewertet
WB_FRAMEINFO				Formatiert wird so, das Frame-Info angezeigt
							werden kann und die FrameColor bei AddDateInfo()
							ausgewertet wird
WB_RANGESELECT				Es koennen mehrere Tage selektiert werden, die
							jedoch alle zusammenhaengend sein muessen
WB_MULTISELECT				Es koennen mehrere Tage selektiert werden
WB_WEEKNUMBER				Es werden die Wochentage mit angezeigt

--------------------------------------------------------------------------

Mit SetCurDate() / GetCurDate() wird das ausgewaehlte Datum gesetzt und
abgefragt. Wenn der Anwnder ein Datum selektiert hat, wird Select()
gerufen. Bei einem Doppelklick auf ein Datum wird DoubleClick() gerufen.

--------------------------------------------------------------------------

Mit CalcWindowSizePixel() kann die Groesse des Fensters in Pixel fuer
die Darstellung einer bestimmte Anzahl von Monaten berechnet werden.

--------------------------------------------------------------------------

Mit SetSaturdayColor() kann eine spezielle Farbe fuer Sonnabende gesetzt
werden und mit SetSundayColor() eine fuer Sonntage. Mit AddDateInfo()
koennen Tage speziell gekennzeichnet werden. Dabei kann man einem
einzelnen Datum eine andere Farbe geben (zum Beispiel fuer Feiertage)
oder diese Umranden (zum Beispiel fuer Termine). Wenn beim Datum
kein Jahr angegeben wird, wird der Tag in jedem Jahr benutzt. Mit
AddDateInfo() kann auch jedem Datum ein Text mitgegeben werden, der
dann angezeigt wird, wenn Balloon-Hilfe an ist. Um nicht alle Jahre
mit entsprechenden Daten zu versorgen, wird der RequestDateInfo()-
Handler gerufen, wenn ein neues Jahr angezeigt wird. Es kann dann
im Handler mit GetRequestYear() das Jahr abgefragt werden.

--------------------------------------------------------------------------

Um ein ContextMenu zu einem Datum anzuzeigen, muss man den Command-Handler
ueberlagern. Mit GetDate() kann zur Mouse-Position das Datum ermittelt
werden. Bei Tastaturausloesung sollte das aktuelle Datum genommen werden.
Wenn ein ContextMenu angezeigt wird, darf der Handler der Basisklasse nicht
gerufen werden.

--------------------------------------------------------------------------

Bei Mehrfachselektion WB_RANGESELECT oder WB_MULTISELECT kann mit
SelectDate()/SelectDateRange() Datumsbereiche selektiert/deselektiert
werden. SelectDateRange() gilt inkl. EndDatum. Mit SetNoSelection() kann
alles deselektiert werden. SetCurDate() selektiert bei Mehrfachselektion
jedoch nicht das Datum mit, sondern gibt nur das Focus-Rechteck vor.

Den selektierten Bereich kann man mit GetSelectDateCount()/GetSelectDate()
abgefragt werden oder der Status von einem Datum kann mit IsDateSelected()
abgefragt werden.

Waehrend der Anwender am selektieren ist, wird der SelectionChanging()-
Handler gerufen. In diesem kann der selektierte Bereich angepasst werden,
wenn man beispielsweise den Bereich eingrenzen oder erweitern will. Der
Bereich wird mit SelectDate()/SelectDateRange() umgesetzt und mit
GetSelectDateCount()/GetSelectDate() abgefragt. Wenn man wissen moechte,
in welche Richtung selektiert wird, kann dies ueber IsSelectLeft()
abgefragt werden. TRUE bedeutet eine Selektion nach links oder oben,
FALSE eine Selektion nach rechts oder unten.

--------------------------------------------------------------------------

Wenn sich der Date-Range-Bereich anpasst und man dort die Selektion
uebernehmen will, sollte dies nur gemacht werden, wenn
IsScrollDateRangeChanged() TRUE zurueckliefert. Denn diese Methode liefert
TRUE zurueck, wenn der Bereich durch Betaetigung von den Scroll-Buttons
ausgeloest wurde. Bei FALSE wurde dies durch Resize(), Methoden-Aufrufen
oder durch Beendigung einer Selektion ausgeloest.

*************************************************************************/

// ------------------
// - Calendar-Types -
// ------------------

#define WB_QUICKHELPSHOWSDATEINFO	((WinBits)0x00004000)
#define WB_BOLDTEXT 				((WinBits)0x00008000)
#define WB_FRAMEINFO				((WinBits)0x00010000)
#define WB_WEEKNUMBER				((WinBits)0x00020000)
// Muss mit den WinBits beim TabBar uebereinstimmen oder mal
// nach \vcl\inc\wintypes.hxx verlagert werden
#ifndef WB_RANGESELECT
#define WB_RANGESELECT				((WinBits)0x00200000)
#endif
#ifndef WB_MULTISELECT
#define WB_MULTISELECT				((WinBits)0x00400000)
#endif

#define DIB_BOLD					((USHORT)0x0001)

// ------------
// - Calendar -
// ------------

class SVT_DLLPUBLIC Calendar : public Control
{
private:
	ImplDateTable*	mpDateTable;
	Table*			mpSelectTable;
	Table*			mpOldSelectTable;
	Table*			mpRestoreSelectTable;
	XubString*		mpDayText[31];
	XubString		maDayText;
	XubString		maWeekText;
	CalendarWrapper maCalendarWrapper;
	Rectangle		maPrevRect;
	Rectangle		maNextRect;
	String			maDayOfWeekText;
	sal_Int32		mnDayOfWeekAry[7];
	Date			maOldFormatFirstDate;
	Date			maOldFormatLastDate;
	Date			maFirstDate;
	Date			maOldFirstDate;
	Date			maCurDate;
	Date			maOldCurDate;
	Date			maAnchorDate;
	Date			maDropDate;
	Color			maSelColor;
	Color			maOtherColor;
	Color*			mpStandardColor;
	Color*			mpSaturdayColor;
	Color*			mpSundayColor;
	ULONG			mnDayCount;
	long			mnDaysOffX;
	long			mnWeekDayOffY;
	long			mnDaysOffY;
	long			mnMonthHeight;
	long			mnMonthWidth;
	long			mnMonthPerLine;
	long			mnLines;
	long			mnDayWidth;
	long			mnDayHeight;
	long			mnWeekWidth;
	long			mnDummy2;
	long			mnDummy3;
	long			mnDummy4;
	WinBits 		mnWinStyle;
	USHORT			mnFirstYear;
	USHORT			mnLastYear;
	USHORT			mnRequestYear;
	BOOL			mbCalc:1,
					mbFormat:1,
					mbDrag:1,
					mbSelection:1,
					mbMultiSelection:1,
					mbWeekSel:1,
					mbUnSel:1,
					mbMenuDown:1,
					mbSpinDown:1,
					mbPrevIn:1,
					mbNextIn:1,
					mbDirect:1,
					mbInSelChange:1,
					mbTravelSelect:1,
					mbScrollDateRange:1,
					mbSelLeft:1,
					mbAllSel:1,
					mbDropPos:1;
	Link			maSelectionChangingHdl;
	Link			maDateRangeChangedHdl;
	Link			maRequestDateInfoHdl;
	Link			maDoubleClickHdl;
	Link			maSelectHdl;
	Timer			maDragScrollTimer;
	USHORT			mnDragScrollHitTest;

#ifdef _SV_CALENDAR_CXX
    using Window::ImplInit;
	SVT_DLLPRIVATE void			ImplInit( WinBits nWinStyle );
	SVT_DLLPRIVATE void			ImplInitSettings();
	SVT_DLLPRIVATE void			ImplGetWeekFont( Font& rFont ) const;
	SVT_DLLPRIVATE void			ImplFormat();
    using Window::ImplHitTest;
	SVT_DLLPRIVATE USHORT			ImplHitTest( const Point& rPos, Date& rDate ) const;
	SVT_DLLPRIVATE void			ImplDrawSpin( BOOL bDrawPrev = TRUE, BOOL bDrawNext = TRUE );
	SVT_DLLPRIVATE void			ImplDrawDate( long nX, long nY,
								  USHORT nDay, USHORT nMonth, USHORT nYear,
								  DayOfWeek eDayOfWeek,
								  BOOL bBack = TRUE, BOOL bOther = FALSE,
								  ULONG nToday = 0 );
	SVT_DLLPRIVATE void			ImplDraw( BOOL bPaint = FALSE );
	SVT_DLLPRIVATE void			ImplUpdateDate( const Date& rDate );
	SVT_DLLPRIVATE void			ImplUpdateSelection( Table* pOld );
	SVT_DLLPRIVATE void			ImplMouseSelect( const Date& rDate, USHORT nHitTest,
									 BOOL bMove, BOOL bExpand, BOOL bExtended );
	SVT_DLLPRIVATE void			ImplUpdate( BOOL bCalcNew = FALSE );
    using Window::ImplScroll;
	SVT_DLLPRIVATE void			ImplScroll( BOOL bPrev );
	SVT_DLLPRIVATE void			ImplInvertDropPos();
	SVT_DLLPRIVATE void			ImplShowMenu( const Point& rPos, const Date& rDate );
	SVT_DLLPRIVATE void			ImplTracking( const Point& rPos, BOOL bRepeat );
	SVT_DLLPRIVATE void			ImplEndTracking( BOOL bCancel );
    SVT_DLLPRIVATE DayOfWeek    ImplGetWeekStart() const;
#endif

protected:
	BOOL			ShowDropPos( const Point& rPos, Date& rDate );
	void			HideDropPos();

	DECL_STATIC_LINK( Calendar, ScrollHdl, Timer *);

public:
					Calendar( Window* pParent, WinBits nWinStyle = 0 );
					Calendar( Window* pParent, const ResId& rResId );
					~Calendar();

	virtual void	MouseButtonDown( const MouseEvent& rMEvt );
	virtual void	MouseButtonUp( const MouseEvent& rMEvt );
	virtual void	MouseMove( const MouseEvent& rMEvt );
	virtual void	Tracking( const TrackingEvent& rMEvt );
	virtual void	KeyInput( const KeyEvent& rKEvt );
	virtual void	Paint( const Rectangle& rRect );
	virtual void	Resize();
	virtual void	GetFocus();
	virtual void	LoseFocus();
	virtual void	RequestHelp( const HelpEvent& rHEvt );
	virtual void	Command( const CommandEvent& rCEvt );
	virtual void	StateChanged( StateChangedType nStateChange );
	virtual void	DataChanged( const DataChangedEvent& rDCEvt );

	virtual void	SelectionChanging();
	virtual void	DateRangeChanged();
	virtual void	RequestDateInfo();
	virtual void	DoubleClick();
	virtual void	Select();

    const CalendarWrapper& GetCalendarWrapper() const { return maCalendarWrapper; }

    /// Set one of ::com::sun::star::i18n::Weekdays.
    void            SetWeekStart( sal_Int16 nDay );

    /// Set how many days of a week must reside in the first week of a year.
    void            SetMinimumNumberOfDaysInWeek( sal_Int16 nDays );

	void			SelectDate( const Date& rDate, BOOL bSelect = TRUE );
	void			SelectDateRange( const Date& rStartDate, const Date& rEndDate,
									 BOOL bSelect = TRUE );
	void			SetNoSelection();
	BOOL			IsDateSelected( const Date& rDate ) const;
	ULONG			GetSelectDateCount() const;
	Date			GetSelectDate( ULONG nIndex = 0 ) const;
	void			EnableCallEverySelect( BOOL bEvery = TRUE ) { mbAllSel = bEvery; }
	BOOL			IsCallEverySelectEnabled() const { return mbAllSel; }

	USHORT			GetRequestYear() const { return mnRequestYear; }
	void			SetCurDate( const Date& rNewDate );
	Date			GetCurDate() const { return maCurDate; }
	void			SetFirstDate( const Date& rNewFirstDate );
	Date			GetFirstDate() const { return maFirstDate; }
	Date			GetLastDate() const { return GetFirstDate() + mnDayCount; }
	ULONG			GetDayCount() const { return mnDayCount; }
	Date			GetFirstMonth() const;
	Date			GetLastMonth() const;
	USHORT			GetMonthCount() const;
	BOOL			GetDate( const Point& rPos, Date& rDate ) const;
	Rectangle		GetDateRect( const Date& rDate ) const;
	BOOL			GetDropDate( Date& rDate ) const;

	long			GetCurMonthPerLine() const { return mnMonthPerLine; }
	long			GetCurLines() const { return mnLines; }

	void			SetStandardColor( const Color& rColor );
	const Color&	GetStandardColor() const;
	void			SetSaturdayColor( const Color& rColor );
	const Color&	GetSaturdayColor() const;
	void			SetSundayColor( const Color& rColor );
	const Color&	GetSundayColor() const;

	void			AddDateInfo( const Date& rDate, const XubString& rText,
								 const Color* pTextColor = NULL,
								 const Color* pFrameColor = NULL,
								 USHORT nFlags = 0 );
	void			RemoveDateInfo( const Date& rDate );
	void			ClearDateInfo();
	XubString		GetDateInfoText( const Date& rDate );

	void			StartSelection();
	void			EndSelection();

	BOOL			IsTravelSelect() const { return mbTravelSelect; }
	BOOL			IsScrollDateRangeChanged() const { return mbScrollDateRange; }
	BOOL			IsSelectLeft() const { return mbSelLeft; }

	Size			CalcWindowSizePixel( long nCalcMonthPerLine = 1,
										 long nCalcLines = 1 ) const;

	void			SetSelectionChangingHdl( const Link& rLink ) { maSelectionChangingHdl = rLink; }
	const Link& 	GetSelectionChangingHdl() const { return maSelectionChangingHdl; }
	void			SetDateRangeChangedHdl( const Link& rLink ) { maDateRangeChangedHdl = rLink; }
	const Link& 	GetDateRangeChangedHdl() const { return maDateRangeChangedHdl; }
	void			SetRequestDateInfoHdl( const Link& rLink ) { maRequestDateInfoHdl = rLink; }
	const Link& 	GetRequestDateInfoHdl() const { return maRequestDateInfoHdl; }
	void			SetDoubleClickHdl( const Link& rLink ) { maDoubleClickHdl = rLink; }
	const Link& 	GetDoubleClickHdl() const { return maDoubleClickHdl; }
	void			SetSelectHdl( const Link& rLink ) { maSelectHdl = rLink; }
	const Link& 	GetSelectHdl() const { return maSelectHdl; }
};

inline const Color& Calendar::GetStandardColor() const
{
	if ( mpStandardColor )
		return *mpStandardColor;
	else
		return GetFont().GetColor();
}

inline const Color& Calendar::GetSaturdayColor() const
{
	if ( mpSaturdayColor )
		return *mpSaturdayColor;
	else
		return GetFont().GetColor();
}

inline const Color& Calendar::GetSundayColor() const
{
	if ( mpSundayColor )
		return *mpSundayColor;
	else
		return GetFont().GetColor();
}

/*************************************************************************

Beschreibung
============

class CalendarField

Bei dieser Klasse handelt es sich um ein DateField, wo ueber einen
DropDown-Button ueber das Calendar-Control ein Datum ausgewaehlt werden
kann.

--------------------------------------------------------------------------

WinBits

Siehe DateField

Die Vorgaben fuer das CalendarControl koennen ueber SetCalendarStyle()
gesetzt werden.

--------------------------------------------------------------------------

Mit EnableToday()/EnableNone() kann ein Today-Button und ein None-Button
enabled werden.

--------------------------------------------------------------------------

Wenn mit SetCalendarStyle() WB_RANGESELECT gesetzt wird, koennen im
Calendar auch mehrere Tage selektiert werden. Da immer nur das Start-Datum
in das Feld uebernommen wird, sollte dann im Select-Handler mit
GetCalendar() der Calendar abgefragt werden und an dem mit
GetSelectDateCount()/GetSelectDate() der selektierte Bereich abgefragt
werden, um beispielsweise diese dann in ein weiteres Feld zu uebernehmen.

--------------------------------------------------------------------------

Wenn ein abgeleiteter Calendar verwendet werden soll, kann am
CalendarField die Methode CreateCalendar() ueberlagert werden und
dort ein eigener Calendar erzeugt werden.

*************************************************************************/

// -----------------
// - CalendarField -
// -----------------

class SVT_DLLPUBLIC CalendarField : public DateField
{
private:
	ImplCFieldFloatWin* mpFloatWin;
	Calendar*			mpCalendar;
	WinBits 			mnCalendarStyle;
	PushButton* 		mpTodayBtn;
	PushButton* 		mpNoneBtn;
	Date				maDefaultDate;
	BOOL				mbToday;
	BOOL				mbNone;
	Link				maSelectHdl;

#ifdef _SV_CALENDAR_CXX
						DECL_DLLPRIVATE_LINK( ImplSelectHdl, Calendar* );
						DECL_DLLPRIVATE_LINK( ImplClickHdl, PushButton* );
						DECL_DLLPRIVATE_LINK( ImplPopupModeEndHdl, FloatingWindow* );
#endif

public:
						CalendarField( Window* pParent, WinBits nWinStyle );
						CalendarField( Window* pParent, const ResId& rResId );
						~CalendarField();

	virtual void		Select();

	virtual BOOL		ShowDropDown( BOOL bShow );
	virtual Calendar*	CreateCalendar( Window* pParent );
	Calendar*			GetCalendar();

	void				SetDefaultDate( const Date& rDate ) { maDefaultDate = rDate; }
	Date				GetDefaultDate() const { return maDefaultDate; }

	void				EnableToday( BOOL bToday = TRUE ) { mbToday = bToday; }
	BOOL				IsTodayEnabled() const { return mbToday; }
	void				EnableNone( BOOL bNone = TRUE ) { mbNone = bNone; }
	BOOL				IsNoneEnabled() const { return mbNone; }

	void				SetCalendarStyle( WinBits nStyle ) { mnCalendarStyle = nStyle; }
	WinBits 			GetCalendarStyle() const { return mnCalendarStyle; }

	void				SetSelectHdl( const Link& rLink ) { maSelectHdl = rLink; }
	const Link& 		GetSelectHdl() const { return maSelectHdl; }

protected:
	virtual void	StateChanged( StateChangedType nStateChange );
};

#endif	// _CALENDAR_HXX
