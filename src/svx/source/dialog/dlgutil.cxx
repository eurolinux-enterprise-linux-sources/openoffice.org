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

// include ---------------------------------------------------------------

#include <limits.h>
#include <tools/shl.hxx>
#include <tools/urlobj.hxx>
#include <unolingu.hxx>

#include <svtools/langtab.hxx>
#include <svtools/itemset.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/objsh.hxx>

#include <svx/dialogs.hrc>

#include "dlgutil.hxx"
#include <svx/dialmgr.hxx>

// -----------------------------------------------------------------------

String GetLanguageString( LanguageType eType )
{
    static const SvtLanguageTable aLangTable;
	return aLangTable.GetString( eType );
}

// -----------------------------------------------------------------------

String GetDicInfoStr( const String& rName, const USHORT nLang, const BOOL bNeg )
{
	INetURLObject aURLObj;
	aURLObj.SetSmartProtocol( INET_PROT_FILE );
	aURLObj.SetSmartURL( rName, INetURLObject::ENCODE_ALL );
	String aTmp( aURLObj.GetBase() );
	aTmp += sal_Unicode( ' ' );

	if ( bNeg )
	{
		sal_Char const sTmp[] = " (-) ";
		aTmp.AppendAscii( sTmp );
	}

	if ( LANGUAGE_NONE == nLang )
		aTmp += String( ResId( RID_SVXSTR_LANGUAGE_ALL, DIALOG_MGR() ) );
	else
	{
		aTmp += sal_Unicode( '[' );
		aTmp += ::GetLanguageString( (LanguageType)nLang );
		aTmp += sal_Unicode( ']' );
	}

	return aTmp;
}

// -----------------------------------------------------------------------

void SetFieldUnit( MetricField& rField, FieldUnit eUnit, BOOL bAll )
{
	sal_Int64 nFirst	= rField.Denormalize( rField.GetFirst( FUNIT_TWIP ) );
	sal_Int64 nLast = rField.Denormalize( rField.GetLast( FUNIT_TWIP ) );
	sal_Int64 nMin = rField.Denormalize( rField.GetMin( FUNIT_TWIP ) );
	sal_Int64 nMax = rField.Denormalize( rField.GetMax( FUNIT_TWIP ) );

	if ( !bAll )
	{
		switch ( eUnit )
		{
			case FUNIT_M:
			case FUNIT_KM:
				eUnit = FUNIT_CM;
				break;

			case FUNIT_FOOT:
			case FUNIT_MILE:
				eUnit = FUNIT_INCH;
				break;
            default: ;//prevent warning
		}
	}
	rField.SetUnit( eUnit );
	switch( eUnit )
	{
		case FUNIT_MM:
			rField.SetSpinSize( 50 );
			break;

		case FUNIT_INCH:
			rField.SetSpinSize( 2 );
			break;

		default:
			rField.SetSpinSize( 10 );
	}

	if ( FUNIT_POINT == eUnit )
    {
        if( rField.GetDecimalDigits() > 1 )
		    rField.SetDecimalDigits( 1 );
    }
	else
		rField.SetDecimalDigits( 2 );

	if ( !bAll )
	{
		rField.SetFirst( rField.Normalize( nFirst ), FUNIT_TWIP );
		rField.SetLast( rField.Normalize( nLast ), FUNIT_TWIP );
		rField.SetMin( rField.Normalize( nMin ), FUNIT_TWIP );
		rField.SetMax( rField.Normalize( nMax ), FUNIT_TWIP );
	}
}

// -----------------------------------------------------------------------

void SetFieldUnit( MetricBox& rBox, FieldUnit eUnit, BOOL bAll )
{
	sal_Int64 nMin = rBox.Denormalize( rBox.GetMin( FUNIT_TWIP ) );
	sal_Int64 nMax = rBox.Denormalize( rBox.GetMax( FUNIT_TWIP ) );

	if ( !bAll )
	{
		switch ( eUnit )
		{
			case FUNIT_M:
			case FUNIT_KM:
				eUnit = FUNIT_CM;
				break;

			case FUNIT_FOOT:
			case FUNIT_MILE:
				eUnit = FUNIT_INCH;
				break;
            default: ;//prevent warning
        }
	}
	rBox.SetUnit( eUnit );

	if ( FUNIT_POINT == eUnit && rBox.GetDecimalDigits() > 1 )
		rBox.SetDecimalDigits( 1 );
	else
		rBox.SetDecimalDigits( 2 );

	if ( !bAll )
	{
		rBox.SetMin( rBox.Normalize( nMin ), FUNIT_TWIP );
		rBox.SetMax( rBox.Normalize( nMax ), FUNIT_TWIP );
	}
}

// -----------------------------------------------------------------------

FieldUnit GetModuleFieldUnit( const SfxItemSet* pSet )
{
	FieldUnit eUnit = FUNIT_INCH;
	const SfxPoolItem* pItem = NULL;
	if ( pSet && SFX_ITEM_SET == pSet->GetItemState( SID_ATTR_METRIC, FALSE, &pItem ) )
		eUnit = (FieldUnit)( (const SfxUInt16Item*)pItem )->GetValue();
	else
	{
		SfxViewFrame* pFrame = SfxViewFrame::Current();
		SfxObjectShell* pSh = NULL;
		if ( pFrame )
			pSh = pFrame->GetObjectShell();
        if ( pSh )  // #93209# the object shell is not always available during reload
        {
		    SfxModule* pModule = pSh->GetModule();
		    if ( pModule )
		    {
                const SfxPoolItem* _pItem = pModule->GetItem( SID_ATTR_METRIC );
                if ( _pItem )
                    eUnit = (FieldUnit)( (SfxUInt16Item*)_pItem )->GetValue();
		    }
		    else
		    {
			    DBG_ERRORFILE( "GetModuleFieldUnit(): no module found" );
		    }
        }
	}
	return eUnit;
}

// -----------------------------------------------------------------------
void SetMetricValue( MetricField& rField, long nCoreValue, SfxMapUnit eUnit )
{
	sal_Int64 nVal = OutputDevice::LogicToLogic( nCoreValue, (MapUnit)eUnit, MAP_100TH_MM );
	nVal = rField.Normalize( nVal );
	rField.SetValue( nVal, FUNIT_100TH_MM );

/*
	if ( SFX_MAPUNIT_100TH_MM == eUnit )
	{
		FieldUnit eFUnit = ( (MetricField&)rField ).GetUnit();
		USHORT nDigits = rField.GetDecimalDigits();

		if ( FUNIT_MM == eFUnit )
		{
			if ( 0 == nDigits )
				lCoreValue /= 100;
			else if ( 1 == nDigits )
				lCoreValue /= 10;
			else if ( nDigits > 2 )
			{
				DBG_ERROR( "too much decimal digits" );
				return;
			}
			rField.SetValue( lCoreValue, FUNIT_MM );
			return;
		}
		else if ( FUNIT_CM == eFUnit )
		{
			if ( 0 == nDigits )
				lCoreValue /= 1000;
			else if ( 1 == nDigits )
				lCoreValue /= 100;
			else if ( 2 == nDigits )
				lCoreValue /= 10;
			else if ( nDigits > 3 )
			{
				DBG_ERROR( "too much decimal digits" );
				return;
			}
			rField.SetValue( lCoreValue, FUNIT_CM );
			return;
		}
	}
	rField.SetValue( rField.Normalize(
		ConvertValueToUnit( lCoreValue, eUnit ) ), MapToFieldUnit( eUnit ) );
*/
}

// -----------------------------------------------------------------------

long GetCoreValue( const MetricField& rField, SfxMapUnit eUnit )
{
	sal_Int64 nVal = rField.GetValue( FUNIT_100TH_MM );
    // avoid rounding issues
    const sal_Int64 nSizeMask = 0xffffffffff000000LL;
    bool bRoundBefore = true;
    if( nVal >= 0 )
    {
        if( (nVal & nSizeMask) == 0 )
            bRoundBefore = false;
    }
    else
    {
        if( ((-nVal) & nSizeMask ) == 0 )
            bRoundBefore = false;
    }
    if( bRoundBefore )
        nVal = rField.Denormalize( nVal );
	sal_Int64 nUnitVal = OutputDevice::LogicToLogic( static_cast<long>(nVal), MAP_100TH_MM, (MapUnit)eUnit );
    if( ! bRoundBefore )
        nUnitVal = rField.Denormalize( nUnitVal );
	return static_cast<long>(nUnitVal);

/*
	long nRet = rField.GetValue( MapToFieldUnit( eUnit ) );
	FieldUnit eFUnit = ( (MetricField&)rField ).GetUnit();
	USHORT nDigits = rField.GetDecimalDigits();
	DBG_ASSERT( nDigits <= 2, "decimal digits > 2!" );

	switch ( eUnit )
	{
		case SFX_MAPUNIT_100TH_MM:
		{
			if ( 2 == nDigits )
				return nRet;
			else if ( 1 == nDigits )
				return nRet * 10;
			else
				return nRet * 100;
		}

		case SFX_MAPUNIT_TWIP:
		{
			if ( 2 == nDigits )
			{
				long nMod = 100;
				long nTmp = nRet % nMod;

				if ( nTmp >= 49 )
					nRet += 100 - nTmp;
				return nRet / 100;
			}
			else if ( 1 == nDigits )
			{
				long nMod = 10;
				long nTmp = nRet % nMod;

				if ( nTmp >= 4 )
					nRet += 10 - nTmp;
				return nRet / 10;
			}
			else
				return nRet;
		}

		default:
			DBG_ERROR( "this unit is not implemented" );
			return 0;
	}
*/

/*!!!
	return ConvertValueToMap( rField.Denormalize(
		rField.GetValue( MapToFieldUnit( eUnit ) ) ), eUnit );
*/
}

// -----------------------------------------------------------------------

long CalcToUnit( float nIn, SfxMapUnit eUnit )
{
	// nIn ist in Points

	DBG_ASSERT( eUnit == SFX_MAPUNIT_TWIP 		||
				eUnit == SFX_MAPUNIT_100TH_MM 	||
				eUnit == SFX_MAPUNIT_10TH_MM 	||
				eUnit == SFX_MAPUNIT_MM 		||
				eUnit == SFX_MAPUNIT_CM, "this unit is not implemented" );

	float nTmp = nIn;

	if ( SFX_MAPUNIT_TWIP != eUnit )
		nTmp = nIn * 10 / 567;

	switch ( eUnit )
	{
		case SFX_MAPUNIT_100TH_MM:	nTmp *= 100; break;
		case SFX_MAPUNIT_10TH_MM:	nTmp *= 10;	 break;
		case SFX_MAPUNIT_MM:					 break;
		case SFX_MAPUNIT_CM:		nTmp /= 10;	 break;
        default: ;//prevent warning
    }

	nTmp *= 20;
	long nRet = (long)nTmp;
	return nRet;
//!	return (long)(nTmp * 20);
}

// -----------------------------------------------------------------------

long ItemToControl( long nIn, SfxMapUnit eItem, SfxFieldUnit eCtrl )
{
	long nOut = 0;

	switch ( eItem )
	{
		case SFX_MAPUNIT_100TH_MM:
		case SFX_MAPUNIT_10TH_MM:
		case SFX_MAPUNIT_MM:
		{
			if ( eItem == SFX_MAPUNIT_10TH_MM )
				nIn /= 10;
			else if ( eItem == SFX_MAPUNIT_100TH_MM )
				nIn /= 100;
			nOut = TransformMetric( nIn, FUNIT_MM, (FieldUnit)eCtrl );
		}
		break;

		case SFX_MAPUNIT_CM:
		{
			nOut = TransformMetric( nIn, FUNIT_CM, (FieldUnit)eCtrl );
		}
		break;

		case SFX_MAPUNIT_1000TH_INCH:
		case SFX_MAPUNIT_100TH_INCH:
		case SFX_MAPUNIT_10TH_INCH:
		case SFX_MAPUNIT_INCH:
		{
			if ( eItem == SFX_MAPUNIT_10TH_INCH )
				nIn /= 10;
			else if ( eItem == SFX_MAPUNIT_100TH_INCH )
				nIn /= 100;
			else if ( eItem == SFX_MAPUNIT_1000TH_INCH )
				nIn /= 1000;
			nOut = TransformMetric( nIn, FUNIT_INCH, (FieldUnit)eCtrl );
		}
		break;

		case SFX_MAPUNIT_POINT:
		{
			nOut = TransformMetric( nIn, FUNIT_POINT, (FieldUnit)eCtrl );
		}
		break;

		case SFX_MAPUNIT_TWIP:
		{
			nOut = TransformMetric( nIn, FUNIT_TWIP, (FieldUnit)eCtrl );
		}
		break;
        default: ;//prevent warning
    }
	return nOut;
}

// -----------------------------------------------------------------------

long ControlToItem( long nIn, SfxFieldUnit eCtrl, SfxMapUnit eItem )
{
	return ItemToControl( nIn, eItem, eCtrl );
}

// -----------------------------------------------------------------------

FieldUnit MapToFieldUnit( const SfxMapUnit eUnit )
{
	switch ( eUnit )
	{
		case SFX_MAPUNIT_100TH_MM:
		case SFX_MAPUNIT_10TH_MM:
		case SFX_MAPUNIT_MM:
			return FUNIT_MM;

		case SFX_MAPUNIT_CM:
			return FUNIT_CM;

		case SFX_MAPUNIT_1000TH_INCH:
		case SFX_MAPUNIT_100TH_INCH:
		case SFX_MAPUNIT_10TH_INCH:
		case SFX_MAPUNIT_INCH:
			return FUNIT_INCH;

		case SFX_MAPUNIT_POINT:
			return FUNIT_POINT;

		case SFX_MAPUNIT_TWIP:
			return FUNIT_TWIP;
        default: ;//prevent warning
    }
	return FUNIT_NONE;
}

// -----------------------------------------------------------------------

MapUnit FieldToMapUnit( const SfxFieldUnit /*eUnit*/ )
{
	return MAP_APPFONT;
}

// -----------------------------------------------------------------------

long ConvertValueToMap( long nVal, SfxMapUnit eUnit )
{
	long nNew = nVal;

	switch ( eUnit )
	{
		case SFX_MAPUNIT_10TH_MM:
		case SFX_MAPUNIT_10TH_INCH:
			nNew *= 10;
			break;

		case SFX_MAPUNIT_100TH_MM:
		case SFX_MAPUNIT_100TH_INCH:
			nNew *= 100;
			break;

		case SFX_MAPUNIT_1000TH_INCH:
			nNew *= 1000;
        default: ;//prevent warning
    }
	return nNew;
}

// -----------------------------------------------------------------------

long ConvertValueToUnit( long nVal, SfxMapUnit eUnit )
{
	long nNew = nVal;

	switch ( eUnit )
	{
		case SFX_MAPUNIT_10TH_MM:
		case SFX_MAPUNIT_10TH_INCH:
			nNew /= 10;
			break;

		case SFX_MAPUNIT_100TH_MM:
		case SFX_MAPUNIT_100TH_INCH:
			nNew /= 100;
			break;

		case SFX_MAPUNIT_1000TH_INCH:
			nNew /= 1000;
        break;
        default: ;//prevent warning
	}
	return nNew;
}

// -----------------------------------------------------------------------

long CalcToPoint( long nIn, SfxMapUnit eUnit, USHORT nFaktor )
{
	DBG_ASSERT( eUnit == SFX_MAPUNIT_TWIP 		||
				eUnit == SFX_MAPUNIT_100TH_MM 	||
				eUnit == SFX_MAPUNIT_10TH_MM 	||
				eUnit == SFX_MAPUNIT_MM 		||
				eUnit == SFX_MAPUNIT_CM, "this unit is not implemented" );

	long nRet = 0;

	if ( SFX_MAPUNIT_TWIP == eUnit )
		nRet = nIn;
	else
		nRet = nIn * 567;

	switch ( eUnit )
	{
		case SFX_MAPUNIT_100TH_MM:	nRet /= 100; break;
		case SFX_MAPUNIT_10TH_MM:	nRet /= 10;	 break;
		case SFX_MAPUNIT_MM:					 break;
		case SFX_MAPUNIT_CM:		nRet *= 10;	 break;
        default: ;//prevent warning
    }

	// ggf. aufrunden
	if ( SFX_MAPUNIT_TWIP != eUnit )
	{
		long nMod = 10;
		long nTmp = nRet % nMod;

		if ( nTmp >= 4 )
			nRet += 10 - nTmp;
		nRet /= 10;
	}
	return nRet * nFaktor / 20;
}

// -----------------------------------------------------------------------

long CMToTwips( long nIn )
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 567 ) && nIn >= ( LONG_MIN / 567 ) )
		nRet = nIn * 567;
	return nRet;
}

// -----------------------------------------------------------------------

long MMToTwips( long nIn )
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 567 ) && nIn >= ( LONG_MIN / 567 ) )
		nRet = nIn * 567 / 10;
	return nRet;
}

// -----------------------------------------------------------------------

long InchToTwips( long nIn )
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 1440 ) && nIn >= ( LONG_MIN / 1440 ) )
		nRet = nIn * 1440;
	return nRet;
}

// -----------------------------------------------------------------------

long PointToTwips( long nIn )
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 20 ) && nIn >= ( LONG_MIN / 20 ) )
		nRet = nIn * 20;
	return nRet;
}

// -----------------------------------------------------------------------

long PicaToTwips( long nIn )
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 240 ) && nIn >= ( LONG_MIN / 240 ) )
		nRet = nIn * 240;
	return nRet;
}

// -----------------------------------------------------------------------

long TwipsToCM( long nIn )
{
	long nRet = nIn / 567;
	return nRet;
}

// -----------------------------------------------------------------------

long InchToCM( long nIn )
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 254 ) && nIn >= ( LONG_MIN / 254 ) )
		nRet = nIn * 254 / 100;
	return nRet;
}

// -----------------------------------------------------------------------

long MMToCM( long nIn )
{
	long nRet = nIn / 10;
	return nRet;
}

// -----------------------------------------------------------------------

long PointToCM( long nIn )
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 20 ) && nIn >= ( LONG_MIN / 20 ) )
		nRet = nIn * 20 / 567;
	return nRet;
}

// -----------------------------------------------------------------------

long PicaToCM( long nIn)
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 12 / 20 ) && nIn >= ( LONG_MIN / 12 / 20 ) )
		nRet = nIn * 12 * 20 / 567;
	return nRet;
}

// -----------------------------------------------------------------------

long TwipsToMM( long nIn )
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 10 ) && nIn >= ( LONG_MIN / 10 ) )
		nRet = nIn * 10 / 566;
	return nRet;
}

// -----------------------------------------------------------------------

long CMToMM( long nIn )
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 10 ) && nIn >= ( LONG_MIN / 10 ) )
		nRet = nIn * 10;
	return nRet;
}

// -----------------------------------------------------------------------

long InchToMM( long nIn )
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 254 ) && nIn >= ( LONG_MIN / 254 ) )
		nRet = nIn * 254 / 10;
	return nRet;
}

// -----------------------------------------------------------------------

long PointToMM( long nIn )
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 200 ) && nIn >= ( LONG_MIN / 200 ) )
		nRet = nIn * 200 / 567;
	return nRet;
}

// -----------------------------------------------------------------------

long PicaToMM( long nIn )
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 12 / 200 ) && nIn >= ( LONG_MIN / 12 / 200 ) )
		nRet = nIn * 12 * 200 / 567;
	return nRet;
}

// -----------------------------------------------------------------------

long TwipsToInch( long nIn )
{
	long nRet = nIn / 1440;
	return nRet;
}

// -----------------------------------------------------------------------

long CMToInch( long nIn )
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 100 ) && nIn >= ( LONG_MIN / 100 ) )
		nRet = nIn * 100 / 254;
	return nRet;
}

// -----------------------------------------------------------------------

long MMToInch( long nIn )
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 10 ) && nIn >= ( LONG_MIN / 10 ) )
		nRet = nIn * 10 / 254;
	return nRet;
}

// -----------------------------------------------------------------------

long PointToInch( long nIn )
{
	long nRet = nIn / 72;
	return nRet;
}

// -----------------------------------------------------------------------

long PicaToInch( long nIn )
{
	long nRet = nIn / 6;
	return nRet;
}

// -----------------------------------------------------------------------

long TwipsToPoint( long nIn )
{
	long nRet = nIn / 20;
	return nRet;
}

// -----------------------------------------------------------------------

long InchToPoint( long nIn )
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 72 ) && nIn >= ( LONG_MIN / 72 ) )
		nRet = nIn * 72;
	return nRet;
}

// -----------------------------------------------------------------------

long CMToPoint( long nIn )
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 567 ) && nIn >= ( LONG_MIN / 567 ) )
		nRet = nIn * 567 / 20;
	return nRet;
}

// -----------------------------------------------------------------------

long MMToPoint( long nIn )
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 567 ) && nIn >= ( LONG_MIN / 567 ) )
		nRet = nIn * 567 / 200;
	return nRet;
}

// -----------------------------------------------------------------------

long PicaToPoint( long nIn )
{
	long nRet = nIn / 12;
	return nRet;
}

// -----------------------------------------------------------------------

long TwipsToPica( long nIn )
{
	long nRet = nIn / 240;
	return nRet;
}

// -----------------------------------------------------------------------

long InchToPica( long nIn )
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 6 ) && nIn >= ( LONG_MIN / 6 ) )
		nRet = nIn * 6;
	return nRet;
}

// -----------------------------------------------------------------------

long PointToPica( long nIn )
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 12 ) && nIn >= ( LONG_MIN / 12 ) )
		nRet = nIn * 12;
	return nRet;
}

// -----------------------------------------------------------------------

long CMToPica( long nIn )
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 567 ) && nIn >= ( LONG_MIN / 567 ) )
		nRet = nIn * 567 / 20 / 12;
	return nRet;
}

// -----------------------------------------------------------------------

long MMToPica( long nIn )
{
	long nRet = 0;

	if ( nIn <= ( LONG_MAX / 567 ) && nIn >= ( LONG_MIN / 567 ) )
		nRet = nIn * 567 / 200 / 12;
	return nRet;
}

// -----------------------------------------------------------------------

long Nothing( long nIn )
{
	long nRet = nIn;
	return nRet;
}

FUNC_CONVERT ConvertTable[6][6] =
{
//  CM,			MM			INCH		 POINT		  PICAS=32	   TWIPS
	{ Nothing, 	CMToMM, 	CMToInch,    CMToPoint,   CMToPica,    CMToTwips },
	{ MMToCM,		Nothing,	MMToInch,	 MMToPoint,   MMToPica,    MMToTwips },
	{ InchToCM,	InchToMM,	Nothing,	 InchToPoint, InchToPica,  InchToTwips },
	{ PointToCM,	PointToMM,  PointToInch, Nothing,	  PointToPica, PointToTwips },
	{ PicaToCM,	PicaToMM,   PicaToInch,  PicaToPoint, Nothing,	   PicaToTwips },
	{ TwipsToCM,	TwipsToMM,  TwipsToInch, TwipsToPoint,TwipsToPica, Nothing }
};

// -----------------------------------------------------------------------

long TransformMetric( long nVal, FieldUnit aOld, FieldUnit aNew )
{
	if ( aOld == FUNIT_NONE	  || aNew == FUNIT_NONE ||
		 aOld == FUNIT_CUSTOM || aNew == FUNIT_CUSTOM )
	{
		return nVal;
	}

	USHORT nOld = 0;
	USHORT nNew = 0;

	switch ( aOld )
	{
		case FUNIT_CM:
			nOld = 0; break;
		case FUNIT_MM:
			nOld = 1; break;
		case FUNIT_INCH:
			nOld = 2; break;
		case FUNIT_POINT:
			nOld = 3; break;
		case FUNIT_PICA:
			nOld = 4; break;
		case FUNIT_TWIP:
			nOld = 5; break;
        default: ;//prevent warning
    }

	switch ( aNew )
	{
		case FUNIT_CM:
			nNew = 0; break;
		case FUNIT_MM:
			nNew = 1; break;
		case FUNIT_INCH:
			nNew = 2; break;
		case FUNIT_POINT:
			nNew = 3; break;
		case FUNIT_PICA:
			nNew = 4; break;
		case FUNIT_TWIP:
			nNew = 5; break;
        default: ;//prevent warning
    }
	return ConvertTable[nOld][nNew]( nVal );
}

String ConvertPosSizeToIniString( const Point& rPos, const Size& rSize )
{
    String aRet = String::CreateFromInt32( rPos.X() );
	aRet += '/';
    aRet += String::CreateFromInt32( rPos.Y() );
	aRet += '/';
    aRet += String::CreateFromInt32( rSize.Width() );
	aRet += '/';
    aRet += String::CreateFromInt32( rSize.Height() );
    return aRet;
}

sal_Bool ConvertIniStringToPosSize( const String& rIniStr, Point& rPos, Size& rSize )
{
	if ( rIniStr.GetTokenCount('/') != 4 )
		return sal_False;

	USHORT nIdx = 0;
	rPos.X() = rIniStr.GetToken( 0, '/', nIdx ).ToInt32();
	rPos.Y() = rIniStr.GetToken( 0, '/', nIdx ).ToInt32();
	rSize.Width() = rIniStr.GetToken( 0, '/', nIdx ).ToInt32();
	rSize.Height() = rIniStr.GetToken( 0, '/', nIdx ).ToInt32();

	// negative sizes are invalid
	if ( rSize.Width() < 0 || rSize.Height() < 0 )
		return sal_False;

	return sal_True;
}

