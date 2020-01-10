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
#ifndef DBAUI_OQUERYMOVETABWINUNDOACT_HXX
#define DBAUI_OQUERYMOVETABWINUNDOACT_HXX

#ifndef DBAUI_QUERYDESIGNUNDOACTION_HXX
#include "QueryDesignUndoAction.hxx"
#endif
#ifndef _DBU_QRY_HRC_
#include "dbu_qry.hrc"
#endif
#ifndef _SV_GEN_HXX
#include <tools/gen.hxx>
#endif

namespace dbaui
{

	// ================================================================================================
	// OQueryMoveTabWinUndoAct - Undo-Klasse fuer Verschieben eines TabWins
	class OQueryTableWindow;
	class OTableWindow;
	class OJoinMoveTabWinUndoAct : public OQueryDesignUndoAction
	{
		Point			m_ptNextPosition;
		OTableWindow*	m_pTabWin;

	protected:
		void TogglePosition();

	public:
		OJoinMoveTabWinUndoAct(OJoinTableView* pOwner, const Point& ptOriginalPosition, OTableWindow* pTabWin);

		virtual void	Undo() { TogglePosition(); }
		virtual void	Redo() { TogglePosition(); }
	};

	// ------------------------------------------------------------------------------------------------
	inline OJoinMoveTabWinUndoAct::OJoinMoveTabWinUndoAct(OJoinTableView* pOwner, const Point& ptOriginalPosition, OTableWindow* pTabWin)
		:OQueryDesignUndoAction(pOwner, STR_QUERY_UNDO_MOVETABWIN)
		,m_ptNextPosition(ptOriginalPosition)
		,m_pTabWin(pTabWin)
	{
	}
}
#endif // DBAUI_OQUERYMOVETABWINUNDOACT_HXX


