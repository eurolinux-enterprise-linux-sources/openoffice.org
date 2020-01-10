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
// System - Includes -----------------------------------------------------



#include <sot/formats.hxx>
#include <sfx2/app.hxx>
#include <svx/linkmgr.hxx>
#include "servobj.hxx"
#include "docsh.hxx"
#include "impex.hxx"
#include "brdcst.hxx"
#include "rangenam.hxx"
#include "sc.hrc"				// SC_HINT_AREAS_CHANGED

// -----------------------------------------------------------------------

BOOL lcl_FillRangeFromName( ScRange& rRange, ScDocShell* pDocSh, const String& rName )
{
	if (pDocSh)
	{
		ScDocument* pDoc = pDocSh->GetDocument();
		ScRangeName* pNames = pDoc->GetRangeName();
		if (pNames)
		{
			USHORT nPos;
			if( pNames->SearchName( rName, nPos ) )
			{
				ScRangeData* pData = (*pNames)[ nPos ];
				if ( pData->IsValidReference( rRange ) )
					return TRUE;
			}
		}
	}
	return FALSE;
}

ScServerObjectSvtListenerForwarder::ScServerObjectSvtListenerForwarder(
        ScServerObject* pObjP)
    : pObj(pObjP)
{
}

ScServerObjectSvtListenerForwarder::~ScServerObjectSvtListenerForwarder()
{
    //! do NOT access pObj
}

void ScServerObjectSvtListenerForwarder::Notify( SvtBroadcaster& /* rBC */, const SfxHint& rHint)
{
    pObj->Notify( aBroadcaster, rHint);
}

ScServerObject::ScServerObject( ScDocShell* pShell, const String& rItem ) :
    aForwarder( this ),
	pDocSh( pShell ),
	bRefreshListener( FALSE )
{
	//	parse item string

	if ( lcl_FillRangeFromName( aRange, pDocSh, rItem ) )
	{
		aItemStr = rItem;				// must be parsed again on ref update
	}
	else
	{
		//	parse ref
		ScDocument* pDoc = pDocSh->GetDocument();
		SCTAB nTab = pDocSh->GetCurTab();
		aRange.aStart.SetTab( nTab );

		if ( aRange.Parse( rItem, pDoc ) & SCA_VALID )
		{
			// area reference
		}
		else if ( aRange.aStart.Parse( rItem, pDoc, pDoc->GetAddressConvention() ) & SCA_VALID )
		{
			// cell reference
			aRange.aEnd = aRange.aStart;
		}
		else
		{
			DBG_ERROR("ScServerObject: invalid item");
		}
	}

	pDocSh->GetDocument()->GetLinkManager()->InsertServer( this );
	pDocSh->GetDocument()->StartListeningArea( aRange, &aForwarder );

	StartListening(*pDocSh);		// um mitzubekommen, wenn die DocShell geloescht wird
	StartListening(*SFX_APP());		// for SC_HINT_AREAS_CHANGED
}

__EXPORT ScServerObject::~ScServerObject()
{
	Clear();
}

void ScServerObject::Clear()
{
	if (pDocSh)
	{
		ScDocShell* pTemp = pDocSh;
		pDocSh = NULL;

		pTemp->GetDocument()->EndListeningArea( aRange, &aForwarder );
		pTemp->GetDocument()->GetLinkManager()->RemoveServer( this );
		EndListening(*pTemp);
		EndListening(*SFX_APP());
	}
}

void ScServerObject::EndListeningAll()
{
    aForwarder.EndListeningAll();
    SfxListener::EndListeningAll();
}

BOOL __EXPORT ScServerObject::GetData(
		::com::sun::star::uno::Any & rData /*out param*/,
        const String & rMimeType, BOOL /* bSynchron */ )
{
	if (!pDocSh)
		return FALSE;

	// named ranges may have changed -> update aRange
	if ( aItemStr.Len() )
	{
		ScRange aNew;
		if ( lcl_FillRangeFromName( aNew, pDocSh, aItemStr ) && aNew != aRange )
		{
			aRange = aNew;
			bRefreshListener = TRUE;
		}
	}

	if ( bRefreshListener )
	{
		//	refresh the listeners now (this is called from a timer)

		EndListeningAll();
		pDocSh->GetDocument()->StartListeningArea( aRange, &aForwarder );
		StartListening(*pDocSh);
		StartListening(*SFX_APP());
		bRefreshListener = FALSE;
	}

	String aDdeTextFmt = pDocSh->GetDdeTextFmt();
	ScDocument* pDoc = pDocSh->GetDocument();

	if( FORMAT_STRING == SotExchange::GetFormatIdFromMimeType( rMimeType ))
	{
		ScImportExport aObj( pDoc, aRange );
		if( aDdeTextFmt.GetChar(0) == 'F' )
			aObj.SetFormulas( TRUE );
		if( aDdeTextFmt.EqualsAscii( "SYLK" ) ||
			aDdeTextFmt.EqualsAscii( "FSYLK" ) )
		{
			ByteString aByteData;
			if( aObj.ExportByteString( aByteData, gsl_getSystemTextEncoding(), SOT_FORMATSTR_ID_SYLK ) )
			{
				rData <<= ::com::sun::star::uno::Sequence< sal_Int8 >(
										(sal_Int8*)aByteData.GetBuffer(),
										aByteData.Len() + 1 );
				return 1;
			}
			return 0;
		}
		if( aDdeTextFmt.EqualsAscii( "CSV" ) ||
			aDdeTextFmt.EqualsAscii( "FCSV" ) )
			aObj.SetSeparator( ',' );
        aObj.SetExportTextOptions( ScExportTextOptions( ScExportTextOptions::ToSpace, ' ', false ) );
		return aObj.ExportData( rMimeType, rData ) ? 1 : 0;
	}

	ScImportExport aObj( pDoc, aRange );
    aObj.SetExportTextOptions( ScExportTextOptions( ScExportTextOptions::ToSpace, ' ', false ) );
	if( aObj.IsRef() )
		return aObj.ExportData( rMimeType, rData ) ? 1 : 0;
	return 0;
}

void __EXPORT ScServerObject::Notify( SfxBroadcaster& rBC, const SfxHint& rHint )
{
	BOOL bDataChanged = FALSE;

	//	DocShell can't be tested via type info, because SFX_HINT_DYING comes from the dtor
	if ( &rBC == pDocSh )
	{
		//	from DocShell, only SFX_HINT_DYING is interesting
		if ( rHint.ISA(SfxSimpleHint) && ((const SfxSimpleHint&)rHint).GetId() == SFX_HINT_DYING )
		{
			pDocSh = NULL;
			EndListening(*SFX_APP());
			//	don't access DocShell anymore for EndListening etc.
		}
	}
	else if (rBC.ISA(SfxApplication))
	{
		if ( aItemStr.Len() && rHint.ISA(SfxSimpleHint) &&
				((const SfxSimpleHint&)rHint).GetId() == SC_HINT_AREAS_CHANGED )
		{
			//	check if named range was modified
			ScRange aNew;
			if ( lcl_FillRangeFromName( aNew, pDocSh, aItemStr ) && aNew != aRange )
				bDataChanged = TRUE;
		}
	}
	else
	{
		//	must be from Area broadcasters

		const ScHint* pScHint = PTR_CAST( ScHint, &rHint );
		if( pScHint && (pScHint->GetId() & (SC_HINT_DATACHANGED | SC_HINT_DYING)) )
			bDataChanged = TRUE;
		else if (rHint.ISA(ScAreaChangedHint))		// position of broadcaster changed
		{
			ScRange aNewRange = ((const ScAreaChangedHint&)rHint).GetRange();
			if ( aRange != aNewRange )
			{
				bRefreshListener = TRUE;
				bDataChanged = TRUE;
			}
		}
		else if (rHint.ISA(SfxSimpleHint))
		{
            ULONG nId = ((const SfxSimpleHint&)rHint).GetId();
			if (nId == SFX_HINT_DYING)
			{
				//	If the range is being deleted, listening must be restarted
				//	after the deletion is complete (done in GetData)
				bRefreshListener = TRUE;
				bDataChanged = TRUE;
			}
		}
	}

	if ( bDataChanged && HasDataLinks() )
		SvLinkSource::NotifyDataChanged();
}





