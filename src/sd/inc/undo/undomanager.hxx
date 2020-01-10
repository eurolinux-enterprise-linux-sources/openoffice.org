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

#ifndef _SD_UNDOMANAGER_HXX
#define _SD_UNDOMANAGER_HXX

#include "misc/scopelock.hxx"
#include <svtools/undo.hxx>

namespace sd
{

class UndoManager : public SfxUndoManager
{
public:
	UndoManager( USHORT nMaxUndoActionCount = 20 );

	virtual void            EnterListAction(const UniString &rComment, const UniString& rRepeatComment, USHORT nId=0);
	virtual void 			LeaveListAction();

	virtual void			AddUndoAction( SfxUndoAction *pAction, BOOL bTryMerg=FALSE );

	bool					isInListAction() const { return mnListLevel != 0; }
	bool					isInUndo() const { return maIsInUndoLock.isLocked(); }

	virtual BOOL			Undo( USHORT nCount=1 );
	virtual BOOL			Redo( USHORT nCount=1 );

    /** Set or reset the undo manager linked with the called undo manager.
    */
    void SetLinkedUndoManager (SfxUndoManager* pLinkedUndoManager);

private:
	using SfxUndoManager::Undo;
	using SfxUndoManager::Redo;

	int	mnListLevel;
	ScopeLock maIsInUndoLock;

    /** Used when the outline view is visible as a last resort to
        synchronize the undo managers.
    */
    SfxUndoManager* mpLinkedUndoManager;

    /** Call ClearRedo() at the linked undo manager, when present.

        It should not be necessary to call ClearRedo() explicitly, but the
        synchronization between the under managers of the document and the
        outline view seems to have a bug.  Therefore this method is called
        whenever a new undo action is added.
    */
    void ClearLinkedRedoActions (void);
};

}

#endif	   // _SD_UNDOMANAGER_HXX
