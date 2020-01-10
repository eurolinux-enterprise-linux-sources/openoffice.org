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
#include "precompiled_sc.hxx"



// INCLUDE ---------------------------------------------------------------

#include <svtools/intitem.hxx>
#include <svtools/stritem.hxx>

#include "navcitem.hxx"
#include "global.hxx"
#include "navipi.hxx"
#include "sc.hrc"		// -> Item-IDs

// STATIC DATA -----------------------------------------------------------


//------------------------------------------------------------------------

ScNavigatorControllerItem::ScNavigatorControllerItem( USHORT          nIdP,
													  ScNavigatorDlg& rDlg,
													  SfxBindings&	  rBindings )
    :   SfxControllerItem   ( nIdP, rBindings ),
		rNavigatorDlg		( rDlg )
{
}

//------------------------------------------------------------------------

void __EXPORT ScNavigatorControllerItem::StateChanged( USHORT /* nSID */, SfxItemState /* eState */,
														  const SfxPoolItem* pItem )
{
	switch( GetId() )
	{
		case SID_CURRENTCELL:
			if ( pItem )
			{
//				const SfxPointItem* pCellPosItem = PTR_CAST(SfxPointItem, pItem);
				const SfxStringItem* pCellPosItem = PTR_CAST(SfxStringItem, pItem);

				DBG_ASSERT( pCellPosItem, "SfxStringItem expected!" );

				if ( pCellPosItem )
				{
					String	aAddress( pCellPosItem->GetValue() );
					ScAddress aScAddress;
					aScAddress.Parse( aAddress );

					SCCOL nCol = aScAddress.Col()+1;
					SCROW nRow = aScAddress.Row()+1;

//					SCCOL nCol = (USHORT)pCellPosItem->GetValue().X()+1;
//					SCROW nRow = (USHORT)pCellPosItem->GetValue().Y()+1;

					rNavigatorDlg.UpdateColumn( &nCol );
					rNavigatorDlg.UpdateRow   ( &nRow );
					rNavigatorDlg.CursorPosChanged();
				}
			}
			break;

		case SID_CURRENTTAB:
			if ( pItem )
			{
				const SfxUInt16Item* pTabItem = PTR_CAST(SfxUInt16Item, pItem);

				DBG_ASSERT( pTabItem, "SfxUInt16Item expected!" );

				//	Tabelle fuer Basic ist 1-basiert
				if ( pTabItem && pTabItem->GetValue() )
				{
					SCTAB nTab = pTabItem->GetValue() - 1;

					rNavigatorDlg.UpdateTable( &nTab );
					rNavigatorDlg.UpdateColumn();
					rNavigatorDlg.UpdateRow();
					rNavigatorDlg.CursorPosChanged();
				}
			}
			break;

		case SID_CURRENTDOC:
			//
			//	gar nix mehr, wird ueber SFX_HINT_DOCCHANGED erledigt
			//
			break;


		case SID_SELECT_SCENARIO:
			rNavigatorDlg.aWndScenarios.NotifyState( pItem );
			break;

		default:
			break;
	}
}



