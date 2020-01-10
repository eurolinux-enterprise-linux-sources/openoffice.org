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

#include <tools/string.hxx>
#include <tools/link.hxx>
#include <vcl/smartid.hxx>
#include <vcl/timer.hxx>

class ToolBox;
class Window;
class VclSimpleEvent;

class MacroRecorder
{
private:
    Window* GetParentWithID( Window* pThis );
    SmartId GetParentID( Window* pThis );

    Link aEventListenerHdl;
    DECL_LINK( EventListener, VclSimpleEvent* );

    Window* pLastWin;
    Window* pEditModify;
    String aEditModifyString;

    ToolBox *pActionParent;      // toolbox from which a tearoff or OpenMenu might happen

    // record keys
    String aKeyString;
    SmartId aKeyUniqueID;     // has to be remembered seperately since Window might be gone when needed
    Window* pKeyWin;
    BOOL bKeyFollowFocus;

    AutoTimer aHookRefresh;
    void AddEventHooks();
    void RemoveEventHooks();
    DECL_LINK( HookRefreshHdl, void* );

    void LogVCL( SmartId aParentID, USHORT nVCLWindowType, SmartId aID, String aMethod, USHORT aParam );
    void LogVCL( SmartId aParentID, USHORT nVCLWindowType, SmartId aID, String aMethod );

    static MacroRecorder *pMacroRecorder;

    MacroRecorder();
    ~MacroRecorder();
    void CheckDelete(); 

	// Actions to perform
	BOOL m_bRecord;
	BOOL m_bLog;

public:

    void SetActionRecord( BOOL bRecord = TRUE ) { m_bRecord = bRecord; CheckDelete(); };
    void SetActionLog( BOOL bLog = TRUE ) { m_bLog = bLog; CheckDelete(); };

    static MacroRecorder* GetMacroRecorder();
    static BOOL HasMacroRecorder();
};

