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
#include <vcl/metaact.hxx>
#include <svtools/zforlist.hxx>
#include <tools/urlobj.hxx>

#define _SVX_FLDITEM_CXX
#include <unotools/localfilehelper.hxx>

#include <svx/flditem.hxx>

#include <svx/svdfield.hxx>

// #90477#
#include <tools/tenccvt.hxx>

#define FRAME_MARKER	(sal_uInt32)0x21981357
#define CHARSET_MARKER	(FRAME_MARKER+1)

// -----------------------------------------------------------------------

TYPEINIT1( SvxFieldItem, SfxPoolItem );

SV_IMPL_PERSIST1( SvxFieldData, SvPersistBase );

// -----------------------------------------------------------------------

SvxFieldData::SvxFieldData()
{
}

// -----------------------------------------------------------------------

SvxFieldData::~SvxFieldData()
{
}

// -----------------------------------------------------------------------

SvxFieldData* SvxFieldData::Clone() const
{
	return new SvxFieldData;
}

// -----------------------------------------------------------------------

int SvxFieldData::operator==( const SvxFieldData& rFld ) const
{
	DBG_ASSERT( Type() == rFld.Type(), "==: Verschiedene Typen" );
	(void)rFld;
	return TRUE;	// Basicklasse immer gleich.
}

// -----------------------------------------------------------------------

void SvxFieldData::Load( SvPersistStream & /*rStm*/ )
{
}

// -----------------------------------------------------------------------

void SvxFieldData::Save( SvPersistStream & /*rStm*/ )
{
}


MetaAction* SvxFieldData::createBeginComment() const
{
	return new MetaCommentAction( "FIELD_SEQ_BEGIN" );
}

MetaAction* SvxFieldData::createEndComment() const
{
	return new MetaCommentAction( "FIELD_SEQ_END" );
}

// -----------------------------------------------------------------------

SvxFieldItem::SvxFieldItem( SvxFieldData* pFld, const USHORT nId ) :
	SfxPoolItem( nId )
{
	pField = pFld;	// gehoert direkt dem Item
}

// -----------------------------------------------------------------------

SvxFieldItem::SvxFieldItem( const SvxFieldData& rField, const USHORT nId ) :
	SfxPoolItem( nId )
{
	pField = rField.Clone();
}

// -----------------------------------------------------------------------

SvxFieldItem::SvxFieldItem( const SvxFieldItem& rItem ) :
	SfxPoolItem	( rItem )
{
	pField = rItem.GetField() ? rItem.GetField()->Clone() : 0;
}

// -----------------------------------------------------------------------

SvxFieldItem::~SvxFieldItem()
{
	delete pField;
}

// -----------------------------------------------------------------------

SfxPoolItem* SvxFieldItem::Clone( SfxItemPool* ) const
{
	return new SvxFieldItem(*this);
}

// -----------------------------------------------------------------------

SfxPoolItem* SvxFieldItem::Create( SvStream& rStrm, USHORT ) const
{
	SvxFieldData* pData = 0;
	SvPersistStream aPStrm( GetClassManager(), &rStrm );
	aPStrm >> pData;

	if( aPStrm.IsEof() )
		aPStrm.SetError( SVSTREAM_GENERALERROR );

	if ( aPStrm.GetError() == ERRCODE_IO_NOFACTORY )
		aPStrm.ResetError();	// Eigentlich einen Code, dass nicht alle Attr gelesen wurden...

	return new SvxFieldItem( pData, Which() );
}

// -----------------------------------------------------------------------

SvStream& SvxFieldItem::Store( SvStream& rStrm, USHORT /*nItemVersion*/ ) const
{
	DBG_ASSERT( pField, "SvxFieldItem::Store: Feld?!" );
	SvPersistStream aPStrm( GetClassManager(), &rStrm );
	// Das ResetError in der obigen Create-Methode gab es in 3.1 noch nicht,
	// deshalb duerfen beim 3.x-Export neuere Items nicht gespeichert werden!
	if ( ( rStrm.GetVersion() <= SOFFICE_FILEFORMAT_31 ) && pField &&
			pField->GetClassId() == 50 /* SdrMeasureField */ )
	{
		// SvxFieldData reicht nicht, weil auch nicht am ClassMgr angemeldet
		SvxURLField aDummyData;
		aPStrm << &aDummyData;
	}
	else
		aPStrm << pField;

	return rStrm;
}

// -----------------------------------------------------------------------

int SvxFieldItem::operator==( const SfxPoolItem& rItem ) const
{
	DBG_ASSERT( SfxPoolItem::operator==( rItem ), "unequal which or type" );

	const SvxFieldData* pOtherFld = ((const SvxFieldItem&)rItem).GetField();
	if ( !pField && !pOtherFld )
		return TRUE;

	if ( ( !pField && pOtherFld ) || ( pField && !pOtherFld ) )
		return FALSE;

	return ( ( pField->Type() == pOtherFld->Type() )
				&& ( *pField == *pOtherFld ) );
}

// =================================================================
// Es folgen die Ableitungen von SvxFieldData...
// =================================================================

SV_IMPL_PERSIST1( SvxDateField, SvxFieldData );

// -----------------------------------------------------------------------

SvxDateField::SvxDateField()
{
	nFixDate = Date().GetDate();
	eType = SVXDATETYPE_VAR;
	eFormat = SVXDATEFORMAT_STDSMALL;
}

// -----------------------------------------------------------------------

SvxDateField::SvxDateField( const Date& rDate, SvxDateType eT, SvxDateFormat eF )
{
	nFixDate = rDate.GetDate();
	eType = eT;
	eFormat = eF;
}

// -----------------------------------------------------------------------

SvxFieldData* SvxDateField::Clone() const
{
	return new SvxDateField( *this );
}

// -----------------------------------------------------------------------

int SvxDateField::operator==( const SvxFieldData& rOther ) const
{
	if ( rOther.Type() != Type() )
		return FALSE;

	const SvxDateField& rOtherFld = (const SvxDateField&) rOther;
	return ( ( nFixDate == rOtherFld.nFixDate ) &&
				( eType == rOtherFld.eType ) &&
				( eFormat == rOtherFld.eFormat ) );
}

// -----------------------------------------------------------------------

void SvxDateField::Load( SvPersistStream & rStm )
{
	USHORT nType, nFormat;

	rStm >> nFixDate;
	rStm >> nType;
	rStm >> nFormat;

	eType = (SvxDateType)nType;
	eFormat= (SvxDateFormat)nFormat;
}

// -----------------------------------------------------------------------

void SvxDateField::Save( SvPersistStream & rStm )
{
	rStm << nFixDate;
	rStm << (USHORT)eType;
	rStm << (USHORT)eFormat;
}

// -----------------------------------------------------------------------

String SvxDateField::GetFormatted( SvNumberFormatter& rFormatter, LanguageType eLang ) const
{
    Date aDate; // current date
	if ( eType == SVXDATETYPE_FIX )
		aDate.SetDate( nFixDate );

	return GetFormatted( aDate, eFormat, rFormatter, eLang );
}

String SvxDateField::GetFormatted( Date& aDate, SvxDateFormat eFormat, SvNumberFormatter& rFormatter, LanguageType eLang )
{
	if ( eFormat == SVXDATEFORMAT_SYSTEM )
	{
		DBG_ERROR( "SVXDATEFORMAT_SYSTEM nicht implementiert!" );
		eFormat = SVXDATEFORMAT_STDSMALL;
	}
	else if ( eFormat == SVXDATEFORMAT_APPDEFAULT )
	{
		DBG_ERROR( "SVXDATEFORMAT_APPDEFAULT: Woher nehmen?" );
		eFormat = SVXDATEFORMAT_STDSMALL;
	}

    ULONG nFormatKey;

	switch( eFormat )
	{
		case SVXDATEFORMAT_STDSMALL:
            // short
            nFormatKey = rFormatter.GetFormatIndex( NF_DATE_SYSTEM_SHORT, eLang );
		break;
		case SVXDATEFORMAT_STDBIG:
            // long
            nFormatKey = rFormatter.GetFormatIndex( NF_DATE_SYSTEM_LONG, eLang );
		break;
		case SVXDATEFORMAT_A:
			// 13.02.96
            nFormatKey = rFormatter.GetFormatIndex( NF_DATE_SYS_DDMMYY, eLang );
		break;
		case SVXDATEFORMAT_B:
			// 13.02.1996
            nFormatKey = rFormatter.GetFormatIndex( NF_DATE_SYS_DDMMYYYY, eLang );
		break;
		case SVXDATEFORMAT_C:
            // 13. Feb 1996
            nFormatKey = rFormatter.GetFormatIndex( NF_DATE_SYS_DMMMYYYY, eLang );
		break;
		case SVXDATEFORMAT_D:
            // 13. Februar 1996
            nFormatKey = rFormatter.GetFormatIndex( NF_DATE_SYS_DMMMMYYYY, eLang );
		break;
		case SVXDATEFORMAT_E:
            // Die, 13. Februar 1996
            nFormatKey = rFormatter.GetFormatIndex( NF_DATE_SYS_NNDMMMMYYYY, eLang );
		break;
		case SVXDATEFORMAT_F:
            // Dienstag, 13. Februar 1996
            nFormatKey = rFormatter.GetFormatIndex( NF_DATE_SYS_NNNNDMMMMYYYY, eLang );
		break;
        default:
            nFormatKey = rFormatter.GetStandardFormat( NUMBERFORMAT_DATE, eLang );
	}

    double fDiffDate = aDate - *(rFormatter.GetNullDate());
    String aStr;
   	Color* pColor = NULL;
    rFormatter.GetOutputString( fDiffDate, nFormatKey, aStr, &pColor );
    return aStr;
}

MetaAction* SvxDateField::createBeginComment() const
{
	return new MetaCommentAction( "FIELD_SEQ_BEGIN" );
}

SV_IMPL_PERSIST1( SvxURLField, SvxFieldData );

// -----------------------------------------------------------------------

SvxURLField::SvxURLField()
{
	eFormat = SVXURLFORMAT_URL;
}

// -----------------------------------------------------------------------

SvxURLField::SvxURLField( const XubString& rURL, const XubString& rRepres, SvxURLFormat eFmt )
	: aURL( rURL ), aRepresentation( rRepres )
{
	eFormat = eFmt;
}

// -----------------------------------------------------------------------

SvxFieldData* SvxURLField::Clone() const
{
	return new SvxURLField( *this );
}

// -----------------------------------------------------------------------

int SvxURLField::operator==( const SvxFieldData& rOther ) const
{
	if ( rOther.Type() != Type() )
		return FALSE;

	const SvxURLField& rOtherFld = (const SvxURLField&) rOther;
	return ( ( eFormat == rOtherFld.eFormat ) &&
				( aURL == rOtherFld.aURL ) &&
				( aRepresentation == rOtherFld.aRepresentation ) &&
				( aTargetFrame == rOtherFld.aTargetFrame ) );
}

// -----------------------------------------------------------------------

void SvxURLField::Load( SvPersistStream & rStm )
{
	USHORT nFormat;
	sal_uInt32 nFrameMarker, nCharSetMarker;
	long nUlongSize = (long)sizeof(sal_uInt32);
	String aTmpURL;

	rStm >> nFormat;

	// UNICODE: rStm >> aTmpURL;
	rStm.ReadByteString(aTmpURL);

	// UNICODE: rStm >> aRepresentation;
	// read to a temp string first, read text encoding and
	// convert later to stay compatible to fileformat
	ByteString aTempString;
	rtl_TextEncoding aTempEncoding = RTL_TEXTENCODING_MS_1252;  // #101493# Init for old documents
	rStm.ReadByteString(aTempString);

	rStm >> nFrameMarker;
	if ( nFrameMarker == FRAME_MARKER )
	{
		// UNICODE: rStm >> aTargetFrame;
		rStm.ReadByteString(aTargetFrame);

		rStm >> nCharSetMarker;
		if ( nCharSetMarker == CHARSET_MARKER )
		{
			USHORT nCharSet;
			rStm >> nCharSet;

			// remember encoding
			aTempEncoding = (rtl_TextEncoding)nCharSet;
		}
		else
			rStm.SeekRel( -nUlongSize );
	}
	else
		rStm.SeekRel( -nUlongSize );

	// now build representation string due to known encoding
	aRepresentation = String(aTempString, aTempEncoding);

	eFormat= (SvxURLFormat)nFormat;

	// Relatives Speichern => Beim laden absolut machen.
    DBG_ERROR("No BaseURL!");
    // TODO/MBA: no BaseURL
    aURL = INetURLObject::GetAbsURL( String(), aTmpURL );
}

// -----------------------------------------------------------------------

void SvxURLField::Save( SvPersistStream & rStm )
{
	// Relatives Speichern der URL
    DBG_ERROR("No BaseURL!");
    // TODO/MBA: no BaseURL
    String aTmpURL = INetURLObject::GetRelURL( String(), aURL );

	rStm << (USHORT)eFormat;

	// UNICODE: rStm << aTmpURL;
	rStm.WriteByteString(aTmpURL);

	// UNICODE: rStm << aRepresentation;
	rStm.WriteByteString(aRepresentation);

	rStm << FRAME_MARKER;

	// UNICODE: rStm << aTargetFrame;
	rStm.WriteByteString(aTargetFrame);

	rStm << CHARSET_MARKER;

	// #90477# rStm << (USHORT)GetStoreCharSet(gsl_getSystemTextEncoding(), rStm.GetVersion());
	rStm << (USHORT)GetSOStoreTextEncoding(gsl_getSystemTextEncoding(), (sal_uInt16)rStm.GetVersion());
}

MetaAction* SvxURLField::createBeginComment() const
{
    // #i46618# Adding target URL to metafile comment
	return new MetaCommentAction( "FIELD_SEQ_BEGIN",
                                  0, 
                                  reinterpret_cast<const BYTE*>(aURL.GetBuffer()),
                                  2*aURL.Len() );
}

// =================================================================
// Die Felder, die aus Calc ausgebaut wurden:
// =================================================================

SV_IMPL_PERSIST1( SvxPageField, SvxFieldData );

SvxFieldData* __EXPORT SvxPageField::Clone() const
{
	return new SvxPageField;		// leer
}

int __EXPORT SvxPageField::operator==( const SvxFieldData& rCmp ) const
{
	return ( rCmp.Type() == TYPE(SvxPageField) );
}

void __EXPORT SvxPageField::Load( SvPersistStream & /*rStm*/ )
{
}

void __EXPORT SvxPageField::Save( SvPersistStream & /*rStm*/ )
{
}

MetaAction* SvxPageField::createBeginComment() const
{
    return new MetaCommentAction( "FIELD_SEQ_BEGIN;PageField" );
}


SV_IMPL_PERSIST1( SvxPagesField, SvxFieldData );

SvxFieldData* __EXPORT SvxPagesField::Clone() const
{
	return new SvxPagesField;	// leer
}

int __EXPORT SvxPagesField::operator==( const SvxFieldData& rCmp ) const
{
	return ( rCmp.Type() == TYPE(SvxPagesField) );
}

void __EXPORT SvxPagesField::Load( SvPersistStream & /*rStm*/ )
{
}

void __EXPORT SvxPagesField::Save( SvPersistStream & /*rStm*/ )
{
}

SV_IMPL_PERSIST1( SvxTimeField, SvxFieldData );

SvxFieldData* __EXPORT SvxTimeField::Clone() const
{
	return new SvxTimeField;	// leer
}

int __EXPORT SvxTimeField::operator==( const SvxFieldData& rCmp ) const
{
	return ( rCmp.Type() == TYPE(SvxTimeField) );
}

void __EXPORT SvxTimeField::Load( SvPersistStream & /*rStm*/ )
{
}

void __EXPORT SvxTimeField::Save( SvPersistStream & /*rStm*/ )
{
}

MetaAction* SvxTimeField::createBeginComment() const
{
	return new MetaCommentAction( "FIELD_SEQ_BEGIN" );
}

SV_IMPL_PERSIST1( SvxFileField, SvxFieldData );

SvxFieldData* __EXPORT SvxFileField::Clone() const
{
	return new SvxFileField;	// leer
}

int __EXPORT SvxFileField::operator==( const SvxFieldData& rCmp ) const
{
	return ( rCmp.Type() == TYPE(SvxFileField) );
}

void __EXPORT SvxFileField::Load( SvPersistStream & /*rStm*/ )
{
}

void __EXPORT SvxFileField::Save( SvPersistStream & /*rStm*/ )
{
}

SV_IMPL_PERSIST1( SvxTableField, SvxFieldData );

SvxFieldData* __EXPORT SvxTableField::Clone() const
{
	return new SvxTableField;	// leer
}

int __EXPORT SvxTableField::operator==( const SvxFieldData& rCmp ) const
{
	return ( rCmp.Type() == TYPE(SvxTableField) );
}

void __EXPORT SvxTableField::Load( SvPersistStream & /*rStm*/ )
{
}

void __EXPORT SvxTableField::Save( SvPersistStream & /*rStm*/ )
{
}

//----------------------------------------------------------------------------
//		SvxExtTimeField
//----------------------------------------------------------------------------

SV_IMPL_PERSIST1( SvxExtTimeField, SvxFieldData );

//----------------------------------------------------------------------------

SvxExtTimeField::SvxExtTimeField()
{
	nFixTime = Time().GetTime();
	eType = SVXTIMETYPE_VAR;
	eFormat = SVXTIMEFORMAT_STANDARD;
}

//----------------------------------------------------------------------------

SvxExtTimeField::SvxExtTimeField( const Time& rTime, SvxTimeType eT, SvxTimeFormat eF )
{
	nFixTime = rTime.GetTime();
	eType = eT;
	eFormat = eF;
}

//----------------------------------------------------------------------------

SvxFieldData* SvxExtTimeField::Clone() const
{
	return new SvxExtTimeField( *this );
}

//----------------------------------------------------------------------------

int SvxExtTimeField::operator==( const SvxFieldData& rOther ) const
{
	if ( rOther.Type() != Type() )
		return FALSE;

	const SvxExtTimeField& rOtherFld = (const SvxExtTimeField&) rOther;
	return ( ( nFixTime == rOtherFld.nFixTime ) &&
				( eType == rOtherFld.eType ) &&
				( eFormat == rOtherFld.eFormat ) );
}

//----------------------------------------------------------------------------

void SvxExtTimeField::Load( SvPersistStream & rStm )
{
	USHORT nType, nFormat;

	rStm >> nFixTime;
	rStm >> nType;
	rStm >> nFormat;

	eType = (SvxTimeType) nType;
	eFormat= (SvxTimeFormat) nFormat;
}

//----------------------------------------------------------------------------

void SvxExtTimeField::Save( SvPersistStream & rStm )
{
	rStm << nFixTime;
	rStm << (USHORT) eType;
	rStm << (USHORT) eFormat;
}

//----------------------------------------------------------------------------

String SvxExtTimeField::GetFormatted( SvNumberFormatter& rFormatter, LanguageType eLang ) const
{
    Time aTime; // current time
	if ( eType == SVXTIMETYPE_FIX )
		aTime.SetTime( nFixTime );
	return GetFormatted( aTime, eFormat, rFormatter, eLang );
}

String SvxExtTimeField::GetFormatted( Time& aTime, SvxTimeFormat eFormat, SvNumberFormatter& rFormatter, LanguageType eLang )
{
	switch( eFormat )
	{
        case SVXTIMEFORMAT_SYSTEM :
            DBG_ERROR( "SVXTIMEFORMAT_SYSTEM: not implemented" );
            eFormat = SVXTIMEFORMAT_STANDARD;
        break;
        case SVXTIMEFORMAT_APPDEFAULT :
            DBG_ERROR( "SVXTIMEFORMAT_APPDEFAULT: not implemented" );
            eFormat = SVXTIMEFORMAT_STANDARD;
        break;
        default: ;//prevent warning
    }

    sal_uInt32 nFormatKey;

	switch( eFormat )
	{
		case SVXTIMEFORMAT_12_HM:
            nFormatKey = rFormatter.GetFormatIndex( NF_TIME_HHMMAMPM, eLang );
		break;
        case SVXTIMEFORMAT_12_HMSH:
        {   // no builtin format available, try to insert or reuse
            String aFormatCode( RTL_CONSTASCII_USTRINGPARAM( "HH:MM:SS.00 AM/PM" ) );
            xub_StrLen nCheckPos;
            short nType;
            /*BOOL bInserted = */rFormatter.PutandConvertEntry( aFormatCode,
                nCheckPos, nType, nFormatKey, LANGUAGE_ENGLISH_US, eLang );
            DBG_ASSERT( nCheckPos == 0, "SVXTIMEFORMAT_12_HMSH: could not insert format code" );
            if ( nCheckPos )
                nFormatKey = rFormatter.GetFormatIndex( NF_TIME_HH_MMSS00, eLang );
        }
        break;
		case SVXTIMEFORMAT_24_HM:
            nFormatKey = rFormatter.GetFormatIndex( NF_TIME_HHMM, eLang );
		break;
		case SVXTIMEFORMAT_24_HMSH:
            nFormatKey = rFormatter.GetFormatIndex( NF_TIME_HH_MMSS00, eLang );
		break;
		case SVXTIMEFORMAT_12_HMS:
            nFormatKey = rFormatter.GetFormatIndex( NF_TIME_HHMMSSAMPM, eLang );
		break;
		case SVXTIMEFORMAT_24_HMS:
            nFormatKey = rFormatter.GetFormatIndex( NF_TIME_HHMMSS, eLang );
		break;
		case SVXTIMEFORMAT_STANDARD:
        default:
            nFormatKey = rFormatter.GetStandardFormat( NUMBERFORMAT_TIME, eLang );
	}

    double fFracTime = aTime.GetTimeInDays();
    String aStr;
   	Color* pColor = NULL;
    rFormatter.GetOutputString( fFracTime, nFormatKey, aStr, &pColor );
    return aStr;
}

MetaAction* SvxExtTimeField::createBeginComment() const
{
	return new MetaCommentAction( "FIELD_SEQ_BEGIN" );
}

//----------------------------------------------------------------------------
//		SvxExtFileField
//----------------------------------------------------------------------------

SV_IMPL_PERSIST1( SvxExtFileField, SvxFieldData );

//----------------------------------------------------------------------------

SvxExtFileField::SvxExtFileField()
{
	eType = SVXFILETYPE_VAR;
	eFormat = SVXFILEFORMAT_FULLPATH;
}

//----------------------------------------------------------------------------

SvxExtFileField::SvxExtFileField( const XubString& rStr, SvxFileType eT, SvxFileFormat eF )
{
	aFile = rStr;
	eType = eT;
	eFormat = eF;
}

//----------------------------------------------------------------------------

SvxFieldData* SvxExtFileField::Clone() const
{
	return new SvxExtFileField( *this );
}

//----------------------------------------------------------------------------

int SvxExtFileField::operator==( const SvxFieldData& rOther ) const
{
	if ( rOther.Type() != Type() )
		return FALSE;

	const SvxExtFileField& rOtherFld = (const SvxExtFileField&) rOther;
	return ( ( aFile == rOtherFld.aFile ) &&
				( eType == rOtherFld.eType ) &&
				( eFormat == rOtherFld.eFormat ) );
}

//----------------------------------------------------------------------------

void SvxExtFileField::Load( SvPersistStream & rStm )
{
	USHORT nType, nFormat;

	// UNICODE: rStm >> aFile;
	rStm.ReadByteString(aFile);

	rStm >> nType;
	rStm >> nFormat;

	eType = (SvxFileType) nType;
	eFormat= (SvxFileFormat) nFormat;
}

//----------------------------------------------------------------------------

void SvxExtFileField::Save( SvPersistStream & rStm )
{
	// UNICODE: rStm << aFile;
	rStm.WriteByteString(aFile);

	rStm << (USHORT) eType;
	rStm << (USHORT) eFormat;
}

//----------------------------------------------------------------------------

XubString SvxExtFileField::GetFormatted() const
{
	XubString aString;

	INetURLObject aURLObj( aFile );

    if( INET_PROT_NOT_VALID == aURLObj.GetProtocol() )
    {
        // invalid? try to interpret string as system file name
        String aURLStr;

        ::utl::LocalFileHelper::ConvertPhysicalNameToURL( aFile, aURLStr );

        aURLObj.SetURL( aURLStr );
    }

    // #92009# Be somewhat liberate when trying to
    // get formatted content out of the FileField
    if( INET_PROT_NOT_VALID == aURLObj.GetProtocol() )
    {
        // still not valid? Then output as is
        aString = aFile;
    }
	else if( INET_PROT_FILE == aURLObj.GetProtocol() )
	{
		switch( eFormat )
		{
			case SVXFILEFORMAT_FULLPATH:
				aString = aURLObj.getFSysPath(INetURLObject::FSYS_DETECT);
			break;

			case SVXFILEFORMAT_PATH:
                aURLObj.removeSegment(INetURLObject::LAST_SEGMENT, false);
                // #101742# Leave trailing slash at the pathname
                aURLObj.setFinalSlash();
				aString = aURLObj.getFSysPath(INetURLObject::FSYS_DETECT);
			break;

			case SVXFILEFORMAT_NAME:
				aString = aURLObj.getBase(INetURLObject::LAST_SEGMENT,true,INetURLObject::DECODE_UNAMBIGUOUS);
			break;

			case SVXFILEFORMAT_NAME_EXT:
				aString = aURLObj.getName(INetURLObject::LAST_SEGMENT,true,INetURLObject::DECODE_UNAMBIGUOUS);
			break;
		}
	}
	else
	{
		switch( eFormat )
		{
			case SVXFILEFORMAT_FULLPATH:
				aString = aURLObj.GetMainURL( INetURLObject::DECODE_TO_IURI );
			break;

			case SVXFILEFORMAT_PATH:
                aURLObj.removeSegment(INetURLObject::LAST_SEGMENT, false);
                // #101742# Leave trailing slash at the pathname
                aURLObj.setFinalSlash();
				aString = aURLObj.GetMainURL( INetURLObject::DECODE_TO_IURI );
			break;

			case SVXFILEFORMAT_NAME:
				aString = aURLObj.getBase();
			break;

			case SVXFILEFORMAT_NAME_EXT:
				aString = aURLObj.getName();
			break;
		}
	}

	return( aString );
}

//----------------------------------------------------------------------------
//		SvxAuthorField
//----------------------------------------------------------------------------

SV_IMPL_PERSIST1( SvxAuthorField, SvxFieldData );

//----------------------------------------------------------------------------

SvxAuthorField::SvxAuthorField()
{
	eType = SVXAUTHORTYPE_VAR;
	eFormat = SVXAUTHORFORMAT_FULLNAME;
}

//----------------------------------------------------------------------------

SvxAuthorField::SvxAuthorField( const XubString& rFirstName, 
                                const XubString& rLastName,
                                const XubString& rShortName,
									SvxAuthorType eT, SvxAuthorFormat eF )
{
    aName      = rLastName;
    aFirstName = rFirstName;
    aShortName = rShortName;
	eType   = eT;
	eFormat = eF;
}

//----------------------------------------------------------------------------

SvxFieldData* SvxAuthorField::Clone() const
{
	return new SvxAuthorField( *this );
}

//----------------------------------------------------------------------------

int SvxAuthorField::operator==( const SvxFieldData& rOther ) const
{
	if ( rOther.Type() != Type() )
		return FALSE;

	const SvxAuthorField& rOtherFld = (const SvxAuthorField&) rOther;
	return ( ( aName == rOtherFld.aName ) &&
				( aFirstName == rOtherFld.aFirstName ) &&
				( aShortName == rOtherFld.aShortName ) &&
				( eType == rOtherFld.eType ) &&
				( eFormat == rOtherFld.eFormat ) );
}

//----------------------------------------------------------------------------

void SvxAuthorField::Load( SvPersistStream & rStm )
{
	USHORT nType, nFormat;

	// UNICODE: rStm >> aName;
	rStm.ReadByteString(aName);

	// UNICODE: rStm >> aFirstName;
	rStm.ReadByteString(aFirstName);

	// UNICODE: rStm >> aShortName;
	rStm.ReadByteString(aShortName);

	rStm >> nType;
	rStm >> nFormat;

	eType = (SvxAuthorType) nType;
	eFormat= (SvxAuthorFormat) nFormat;
}

//----------------------------------------------------------------------------

void SvxAuthorField::Save( SvPersistStream & rStm )
{
	// UNICODE: rStm << aName;
	rStm.WriteByteString(aName);

	// UNICODE: rStm << aFirstName;
	rStm.WriteByteString(aFirstName);

	// UNICODE: rStm << aShortName;
	rStm.WriteByteString(aShortName);

	rStm << (USHORT) eType;
	rStm << (USHORT) eFormat;
}

//----------------------------------------------------------------------------

XubString SvxAuthorField::GetFormatted() const
{
	XubString aString;

	switch( eFormat )
	{
		case SVXAUTHORFORMAT_FULLNAME:
			aString  = aFirstName;
			aString += sal_Unicode(' ');
			aString += aName;
		break;

		case SVXAUTHORFORMAT_NAME:
			aString = aName;
		break;

		case SVXAUTHORFORMAT_FIRSTNAME:
			aString = aFirstName;
		break;

		case SVXAUTHORFORMAT_SHORTNAME:
			aString = aShortName;
		break;
	}

	return( aString );
}

static SvClassManager* pClassMgr=0;

SvClassManager&	SvxFieldItem::GetClassManager()
{
	if ( !pClassMgr )
	{
		pClassMgr = new SvClassManager;
		pClassMgr->SV_CLASS_REGISTER( SvxFieldData );
		pClassMgr->SV_CLASS_REGISTER( SvxURLField );
		pClassMgr->SV_CLASS_REGISTER( SvxDateField );
		pClassMgr->SV_CLASS_REGISTER( SvxPageField );
		pClassMgr->SV_CLASS_REGISTER( SvxTimeField );
		pClassMgr->SV_CLASS_REGISTER( SvxExtTimeField );
		pClassMgr->SV_CLASS_REGISTER( SvxExtFileField );
		pClassMgr->SV_CLASS_REGISTER( SvxAuthorField );
	}

	return *pClassMgr;
}

///////////////////////////////////////////////////////////////////////

SV_IMPL_PERSIST1( SvxHeaderField, SvxFieldData );

SvxFieldData* __EXPORT SvxHeaderField::Clone() const
{
	return new SvxHeaderField;		// leer
}

int __EXPORT SvxHeaderField::operator==( const SvxFieldData& rCmp ) const
{
	return ( rCmp.Type() == TYPE(SvxHeaderField) );
}

void __EXPORT SvxHeaderField::Load( SvPersistStream & /*rStm*/ )
{
}

void __EXPORT SvxHeaderField::Save( SvPersistStream & /*rStm*/ )
{
}

///////////////////////////////////////////////////////////////////////

SV_IMPL_PERSIST1( SvxFooterField, SvxFieldData );

SvxFieldData* __EXPORT SvxFooterField::Clone() const
{
	return new SvxFooterField;		// leer
}

int __EXPORT SvxFooterField::operator==( const SvxFieldData& rCmp ) const
{
	return ( rCmp.Type() == TYPE(SvxFooterField) );
}

void __EXPORT SvxFooterField::Load( SvPersistStream & /*rStm*/ )
{
}

void __EXPORT SvxFooterField::Save( SvPersistStream & /*rStm*/ )
{
}

///////////////////////////////////////////////////////////////////////

SV_IMPL_PERSIST1( SvxDateTimeField, SvxFieldData );

SvxFieldData* __EXPORT SvxDateTimeField::Clone() const
{
	return new SvxDateTimeField;		// leer
}

int __EXPORT SvxDateTimeField::operator==( const SvxFieldData& rCmp ) const
{
	return ( rCmp.Type() == TYPE(SvxDateTimeField) );
}

void __EXPORT SvxDateTimeField::Load( SvPersistStream & /*rStm*/ )
{
}

void __EXPORT SvxDateTimeField::Save( SvPersistStream & /*rStm*/ )
{
}

String SvxDateTimeField::GetFormatted( Date& rDate, Time& rTime, int eFormat, SvNumberFormatter& rFormatter, LanguageType eLanguage )
{
	String aRet;

	SvxDateFormat eDateFormat = (SvxDateFormat)(eFormat & 0x0f);

	if(eDateFormat)
	{
		aRet = SvxDateField::GetFormatted( rDate, eDateFormat, rFormatter, eLanguage );
	}

	SvxTimeFormat eTimeFormat = (SvxTimeFormat)((eFormat >> 4) & 0x0f);

	if(eTimeFormat)
	{
		if(aRet.Len())
			aRet += sal_Unicode(' ');

		aRet += SvxExtTimeField::GetFormatted( rTime, eTimeFormat, rFormatter, eLanguage );
	}

	return aRet;
}

