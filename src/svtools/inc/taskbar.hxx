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

#ifndef _TASKBAR_HXX
#define _TASKBAR_HXX

#include "svtools/svtdllapi.h"
#include <tools/time.hxx>
#ifndef _TOOLS_LIST_HXX
#include <tools/list.hxx>
#endif
#include <vcl/timer.hxx>
#include <vcl/toolbox.hxx>
#include <vcl/status.hxx>

class TaskBar;
class TaskStatusFieldItem;
class ImplTaskItemList;
class ImplTaskSBItemList;
class ImplTaskBarFloat;
struct ImplTaskSBFldItem;

// -----------------
// - Dokumentation -
// -----------------

/*

TaskToolBox
===========

StartUpdateTask()/UpdateTask()/EndUpdateTask()
Diese muessen gerufen werden, wenn die Task upgedatet werden muessen.
Dann muss StartUpdateTask() gerufen werden, dann UpdateTask() fuer alle
Task's und danach EndUpdateTask() wo dann die TaskButtons entsprechend
neu angeordnet werden.

ActivateTask()
Handler der gerufen wird, wenn ein Task aktiviert werden muss. Mit
GetTaskItem() kann abgefragt werden, welcher Task aktiviert werden muss.

ContextMenu()
Dieser Handler wird gerufen, wenn ein ContextMenu angezeigt werden soll.
Mit GetTaskMode() kann abgefragt werden, ob fuer einen Task oder ein
Item.

GetTaskItem()
Diese Methode liefert das Item zurueck, welches bei UpdateTask an der
entsprechenden Position eingefuegt wurde.

GetContextMenuPos()
Liefert die Position zurueck, wo das Contextmenu angezeigt werden soll.


TaskStatusBar
=============

InsertStatusField()/RemoveStatusField()
Fuegt ein Statusfeld ein, wo die aktuelle Uhrzeit angezeigt wird. In
dieses Feld koennen dann mit AddStatusFielItem(), ModifyStatusFielItem()
und RemoveStatusFielItem() Status-Items eingefuegt werden. Bei diesen
muss man ein Image angeben, welches dann angezeigt wird. Ausserdem kann
man bei diesen noch Hilfe-Texte angeben oder sagen, ob sie blinken
sollen und ein Notify-Object, worueber man informiert wird, wenn ein
Kontextmenu angezeigt wird oder das Item angeklickt wird. Am
TaskStatusBar kann auch ein Notify-Object gesetzt werden, wenn man
benachrichtigt werden will, wenn die Uhrzeit oder die TaskStatusBar
angeklickt wird. Wenn der Notify fuer die Uhrzeit kommt, ist die
Id TASKSTATUSBAR_CLOCKID, wenn er fuer die TaskStatusBar kommt, ist
die Id 0. Mit SetFieldFlags() kann am TaskStatusBar auch die Flags
hinterher umgesetzt werden, um zum Beispiel die Uhrzeit ein- und
auszuschalten.


TaskBar
=======

Erlaubte StyleBits
------------------

WB_BORDER       - Border an der oberen Kante
WB_SIZEABLE     - Zwischen TaskToolBox und TaskStatusBar kann der Anwender
                  die Groesse aendern.

Wenn WB_SIZEABLE gesetzt ist, kann die Breite des StatusBars gesetzt und
abgefragt werden. Dazu kann man SetStatusSize()/GetStatusSize() aufrufen.
0 steht dabei fuer optimale Groesse, was auch der Default ist. Bei einem
Doppelklick auf den Trenner kann der Anwender auch wieder die optimale
Groesse einstellen.

Wichtige Methoden
------------------

virtual TaskToolBox* TaskBar::CreateButtonBar();
virtual TaskToolBox* TaskBar::CreateTaskToolBox();
virtual TaskStatusBar* TaskBar::CreateTaskStatusBar();

Diese Methoden muesste man ueberladen, wenn man eine eigene Klasse anlegen
will.

void TaskBar::ShowStatusText( const String& rText );
void TaskBar::HideStatusText();

Blendet den ButtonBar und die TaskBar ein bzw. aus um den Hilfetexte in der
gesammten Zeile anzuzeigen.
*/

// -----------------
// - TaskButtonBar -
// -----------------

class TaskButtonBar : public ToolBox
{
    friend class TaskBar;

private:
    TaskBar*            mpNotifyTaskBar;
    void*               mpDummy1;
    void*               mpDummy2;
    void*               mpDummy3;
    void*               mpDummy4;

public:
                        TaskButtonBar( Window* pParent, WinBits nWinStyle = 0 );
                        ~TaskButtonBar();

    virtual void        RequestHelp( const HelpEvent& rHEvt );

    void                InsertButton( USHORT nItemId,
                                      const Image& rImage, const String& rText,
                                      USHORT nPos = TOOLBOX_APPEND )
                            { InsertItem( nItemId, rImage, rText, TIB_LEFT | TIB_AUTOSIZE, nPos ); }
    void                RemoveButton( USHORT nItemId )
                            { RemoveItem( nItemId ); }
};

// ---------------------
// - TaskToolBox-Types -
// ---------------------

#define TASKTOOLBOX_TASK_NOTFOUND       ((USHORT)0xFFFF)

// ---------------
// - TaskToolBox -
// ---------------

class SVT_DLLPUBLIC TaskToolBox : public ToolBox
{
    friend class TaskBar;

private:
    ImplTaskItemList*   mpItemList;
    TaskBar*            mpNotifyTaskBar;
    Point               maContextMenuPos;
    ULONG               mnOldItemCount;
    long                mnMaxTextWidth;
    long                mnDummy1;
    USHORT              mnUpdatePos;
    USHORT              mnUpdateNewPos;
    USHORT              mnActiveItemId;
    USHORT              mnNewActivePos;
    USHORT              mnTaskItem;
    USHORT              mnSmallItem;
    USHORT              mnDummy2;
    BOOL                mbMinActivate;
    BOOL                mbDummy1;
    Link                maActivateTaskHdl;
    Link                maContextMenuHdl;

#ifdef _TASKBAR_CXX
    SVT_DLLPRIVATE void                ImplFormatTaskToolBox();
#endif

	// Forbidden and not implemented.
	TaskToolBox (const TaskToolBox &);
	TaskToolBox & operator= (const TaskToolBox &);

public:
                        TaskToolBox( Window* pParent, WinBits nWinStyle = 0 );
                        ~TaskToolBox();

    void                ActivateTaskItem( USHORT nItemId,
                                          BOOL bMinActivate = FALSE );
    USHORT              GetTaskItem( const Point& rPos ) const;

    virtual void        ActivateTask();
    virtual void        ContextMenu();

    virtual void        Select();

    virtual void        MouseButtonDown( const MouseEvent& rMEvt );
    virtual void        Resize();
    virtual void        Command( const CommandEvent& rCEvt );
    virtual void        RequestHelp( const HelpEvent& rHEvt );

    void                StartUpdateTask();
    void                UpdateTask( const Image& rImage, const String& rText,
                                    BOOL bActive = FALSE );
    void                EndUpdateTask();

    const Point&        GetContextMenuPos() const { return maContextMenuPos; }
    USHORT              GetTaskItem() const { return mnTaskItem; }
    BOOL                IsMinActivate() const { return mbMinActivate; }

    void                SetActivateTaskHdl( const Link& rLink ) { maActivateTaskHdl = rLink; }
    const Link&         GetActivateTaskHdl() const { return maActivateTaskHdl; }
    void                SetContextMenuHdl( const Link& rLink ) { maContextMenuHdl = rLink; }
    const Link&         GetContextMenuHdl() const { return maContextMenuHdl; }
};

inline USHORT TaskToolBox::GetTaskItem( const Point& rPos ) const
{
    USHORT nId = GetItemId( rPos );
    if ( nId )
        return nId-1;
    else
        return TASKTOOLBOX_TASK_NOTFOUND;
}

// ---------------------
// - ITaskStatusNotify -
// ---------------------

class ITaskStatusNotify
{
public:
    virtual BOOL        MouseButtonDown( USHORT nItemd, const MouseEvent& rMEvt );
    virtual BOOL        MouseButtonUp( USHORT nItemd, const MouseEvent& rMEvt );
    virtual BOOL        MouseMove( USHORT nItemd, const MouseEvent& rMEvt );
    virtual BOOL        Command( USHORT nItemd, const CommandEvent& rCEvt );
    virtual BOOL        UpdateHelp( USHORT nItemd );
};

// -----------------------
// - TaskStatusFieldItem -
// -----------------------

#define TASKSTATUSFIELDITEM_FLASH           ((USHORT)0x0001)

class TaskStatusFieldItem
{
private:
    ITaskStatusNotify*  mpNotify;
    Image               maImage;
    XubString           maQuickHelpText;
    XubString           maHelpText;
    ULONG               mnHelpId;
    USHORT              mnFlags;

public:
                        TaskStatusFieldItem();
                        TaskStatusFieldItem( const TaskStatusFieldItem& rItem );
                        TaskStatusFieldItem( ITaskStatusNotify* pNotify,
                                             const Image& rImage,
                                             const XubString& rQuickHelpText,
                                             const XubString& rHelpText,
                                             USHORT nFlags );
                        ~TaskStatusFieldItem();

    void                SetNotifyObject( ITaskStatusNotify* pNotify ) { mpNotify = pNotify; }
    ITaskStatusNotify*  GetNotifyObject() const { return mpNotify; }
    void                SetImage( const Image& rImage ) { maImage = rImage; }
    const Image&        GetImage() const { return maImage; }
    void                SetQuickHelpText( const XubString& rStr ) { maQuickHelpText = rStr; }
    const XubString&    GetQuickHelpText() const { return maQuickHelpText; }
    void                SetHelpText( const XubString& rStr ) { maHelpText = rStr; }
    const XubString&    GetHelpText() const { return maHelpText; }
    void                SetHelpId( ULONG nHelpId ) { mnHelpId = nHelpId; }
    ULONG               GetHelpId() const { return mnHelpId; }
    void                SetFlags( USHORT nFlags ) { mnFlags = nFlags; }
    USHORT              GetFlags() const { return mnFlags; }

    const TaskStatusFieldItem& operator=( const TaskStatusFieldItem& rItem );
};

// -----------------
// - TaskStatusBar -
// -----------------

#define TASKSTATUSBAR_STATUSFIELDID         ((USHORT)61000)

#define TASKSTATUSBAR_CLOCKID               ((USHORT)61000)
#define TASKSTATUSFIELD_CLOCK               ((USHORT)0x0001)

class SVT_DLLPUBLIC TaskStatusBar : public StatusBar
{
    friend class TaskBar;

private:
    ImplTaskSBItemList* mpFieldItemList;
    TaskBar*            mpNotifyTaskBar;
    ITaskStatusNotify*  mpNotify;
    Time                maTime;
    XubString           maTimeText;
    AutoTimer           maTimer;
    long                mnClockWidth;
    long                mnItemWidth;
    long                mnFieldWidth;
    USHORT              mnFieldFlags;
    USHORT              mnDummy1;
    BOOL                mbFlashItems;
    BOOL                mbOutInterval;
    BOOL                mbDummy1;
    BOOL                mbDummy2;

#ifdef _TASKBAR_CXX
    SVT_DLLPRIVATE ImplTaskSBFldItem*  ImplGetFieldItem( USHORT nItemId ) const;
    SVT_DLLPRIVATE ImplTaskSBFldItem*  ImplGetFieldItem( const Point& rPos, BOOL& rFieldRect ) const;
    SVT_DLLPRIVATE BOOL                ImplUpdateClock();
    SVT_DLLPRIVATE BOOL                ImplUpdateFlashItems();
    SVT_DLLPRIVATE void                ImplUpdateField( BOOL bItems );
                        DECL_DLLPRIVATE_LINK( ImplTimerHdl, Timer* );
#endif

public:
                        TaskStatusBar( Window* pParent, WinBits nWinStyle = WB_LEFT );
                        ~TaskStatusBar();

    virtual void        MouseButtonDown( const MouseEvent& rMEvt );
    virtual void        MouseButtonUp( const MouseEvent& rMEvt );
    virtual void        MouseMove( const MouseEvent& rMEvt );
    virtual void        Command( const CommandEvent& rCEvt );
    virtual void        RequestHelp( const HelpEvent& rHEvt );
    virtual void        UserDraw( const UserDrawEvent& rUDEvt );

    void                InsertStatusField( long nOffset = STATUSBAR_OFFSET,
                                           USHORT nPos = STATUSBAR_APPEND,
                                           USHORT nFlags = TASKSTATUSFIELD_CLOCK );
    void                RemoveStatusField()
                            { maTimer.Stop(); RemoveItem( TASKSTATUSBAR_STATUSFIELDID ); }
    void                SetFieldFlags( USHORT nFlags );
    USHORT              GetFieldFlags() const { return mnFieldFlags; }
    void                SetNotifyObject( ITaskStatusNotify* pNotify ) { mpNotify = pNotify; }
    ITaskStatusNotify*  GetNotifyObject() const { return mpNotify; }

    void                AddStatusFieldItem( USHORT nItemId, const TaskStatusFieldItem& rItem,
                                            USHORT nPos = 0xFFFF );
    void                ModifyStatusFieldItem( USHORT nItemId, const TaskStatusFieldItem& rItem );
    void                RemoveStatusFieldItem( USHORT nItemId );
    BOOL                GetStatusFieldItem( USHORT nItemId, TaskStatusFieldItem& rItem ) const;
};

// -----------
// - TaskBar -
// -----------

class SVT_DLLPUBLIC TaskBar : public Window
{
private:
    ImplTaskBarFloat*       mpAutoHideBar;
    TaskButtonBar*          mpButtonBar;
    TaskToolBox*            mpTaskToolBox;
    TaskStatusBar*          mpStatusBar;
    void*                   mpDummy1;
    void*                   mpDummy2;
    void*                   mpDummy3;
    void*                   mpDummy4;
    String                  maOldText;
    long                    mnStatusWidth;
    long                    mnMouseOff;
    long                    mnOldStatusWidth;
    long                    mnDummy1;
    long                    mnDummy2;
    long                    mnDummy3;
    long                    mnDummy4;
    WinBits                 mnWinBits;
    USHORT                  mnLines;
    BOOL                    mbStatusText;
    BOOL                    mbShowItems;
    BOOL                    mbAutoHide;
    BOOL                    mbAlignDummy1;
    BOOL                    mbDummy1;
    BOOL                    mbDummy2;
    BOOL                    mbDummy3;
    BOOL                    mbDummy4;
    Link                    maTaskResizeHdl;

#ifdef _TASKBAR_CXX
    SVT_DLLPRIVATE void                    ImplInitSettings();
    SVT_DLLPRIVATE void                    ImplNewHeight( long nNewHeight );
#endif

public:
                            TaskBar( Window* pParent, WinBits nWinStyle = WB_BORDER | WB_SIZEABLE );
                            ~TaskBar();

    virtual void            TaskResize();

    virtual TaskButtonBar*  CreateButtonBar();
    virtual TaskToolBox*    CreateTaskToolBox();
    virtual TaskStatusBar*  CreateTaskStatusBar();

    virtual void            MouseMove( const MouseEvent& rMEvt );
    virtual void            MouseButtonDown( const MouseEvent& rMEvt );
    virtual void            Tracking( const TrackingEvent& rMEvt );
    virtual void            Paint( const Rectangle& rRect );
    virtual void            Resize();
    virtual void            StateChanged( StateChangedType nType );
    virtual void            DataChanged( const DataChangedEvent& rDCEvt );

    void                    Format();

    void                    SetLines( USHORT nLines );
    USHORT                  GetLines() const { return mnLines; }
    void                    EnableAutoHide( BOOL bAutoHide = TRUE );
    BOOL                    IsAutoHideEnabled() const { return mbAutoHide; }

    void                    ShowStatusText( const String& rText );
    void                    HideStatusText();

    void                    SetStatusSize( long nNewSize )
                                { mnStatusWidth=nNewSize; Resize(); }
    long                    GetStatusSize() const { return mnStatusWidth; }

    Size                    CalcWindowSizePixel() const;

    TaskButtonBar*          GetButtonBar() const;
    TaskToolBox*            GetTaskToolBox() const;
    TaskStatusBar*          GetStatusBar() const;

    void                    SetTaskResizeHdl( const Link& rLink ) { maTaskResizeHdl = rLink; }
    const Link&             GetTaskResizeHdl() const { return maTaskResizeHdl; }
};

// -----------------------
// - WindowArrange-Types -
// -----------------------

#define WINDOWARRANGE_TILE      1
#define WINDOWARRANGE_HORZ      2
#define WINDOWARRANGE_VERT      3
#define WINDOWARRANGE_CASCADE   4

class ImplWindowArrangeList;

// -----------------------
// - class WindowArrange -
// -----------------------

class SVT_DLLPUBLIC WindowArrange
{
private:
    List*                   mpWinList;
    void*                   mpDummy;
    ULONG                   mnDummy;

#ifdef _TASKBAR_CXX
    SVT_DLLPRIVATE void                    ImplTile( const Rectangle& rRect );
    SVT_DLLPRIVATE void                    ImplHorz( const Rectangle& rRect );
    SVT_DLLPRIVATE void                    ImplVert( const Rectangle& rRect );
    SVT_DLLPRIVATE void                    ImplCascade( const Rectangle& rRect );
#endif

public:
                            WindowArrange();
                            ~WindowArrange();

    void                    AddWindow( Window* pWindow, ULONG nPos = LIST_APPEND )
                                { mpWinList->Insert( (void*)pWindow, nPos ); }
    void                    RemoveAllWindows()
                                { mpWinList->Clear(); }

    void                    Arrange( USHORT nType, const Rectangle& rRect );
};

#endif  // _TASKBAR_HXX
