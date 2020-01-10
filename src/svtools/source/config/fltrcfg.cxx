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
#include "precompiled_svtools.hxx"

#include "fltrcfg.hxx"
#include <tools/debug.hxx>

#include <rtl/logfile.hxx>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Sequence.hxx>

using namespace utl;
using namespace rtl;
using namespace com::sun::star::uno;

#define C2U(cChar) OUString::createFromAscii(cChar)

// -----------------------------------------------------------------------
#define FILTERCFG_WORD_CODE 			0x0001
#define FILTERCFG_WORD_STORAGE 			0x0002
#define FILTERCFG_EXCEL_CODE 			0x0004
#define FILTERCFG_EXCEL_STORAGE 		0x0008
#define FILTERCFG_PPOINT_CODE 			0x0010
#define FILTERCFG_PPOINT_STORAGE 		0x0020
#define FILTERCFG_MATH_LOAD				0x0100
#define FILTERCFG_MATH_SAVE				0x0200
#define FILTERCFG_WRITER_LOAD			0x0400
#define FILTERCFG_WRITER_SAVE			0x0800
#define FILTERCFG_CALC_LOAD				0x1000
#define FILTERCFG_CALC_SAVE				0x2000
#define FILTERCFG_IMPRESS_LOAD			0x4000
#define FILTERCFG_IMPRESS_SAVE			0x8000
#define FILTERCFG_EXCEL_EXECTBL			0x10000
#define FILTERCFG_ENABLE_PPT_PREVIEW    0x20000
#define FILTERCFG_ENABLE_EXCEL_PREVIEW  0x40000
#define FILTERCFG_ENABLE_WORD_PREVIEW   0x80000
#define FILTERCFG_USE_ENHANCED_FIELDS	0x100000

static SvtFilterOptions* pOptions=0;

/* -----------------------------22.01.01 10:23--------------------------------

 ---------------------------------------------------------------------------*/
class SvtAppFilterOptions_Impl : public utl::ConfigItem
{
	sal_Bool				bLoadVBA;
	sal_Bool				bSaveVBA;
public:
	SvtAppFilterOptions_Impl(const OUString& rRoot) :
		utl::ConfigItem(rRoot),
		bLoadVBA(sal_False),
		bSaveVBA(sal_False)	 {}
	~SvtAppFilterOptions_Impl();
	virtual void			Commit();
	void					Load();

	sal_Bool				IsLoad() const {return bLoadVBA;}
	void					SetLoad(sal_Bool bSet)
							{
								if(bSet != bLoadVBA)
									SetModified();
								bLoadVBA = bSet;
							}
	sal_Bool				IsSave() const {return bSaveVBA;}
	void					SetSave(sal_Bool bSet)
							{
								if(bSet != bSaveVBA)
									SetModified();
								bSaveVBA = bSet;
							}
};

/* -----------------------------22.01.01 11:08--------------------------------

 ---------------------------------------------------------------------------*/
SvtAppFilterOptions_Impl::~SvtAppFilterOptions_Impl()
{
	if(IsModified())
		Commit();
}
/* -----------------------------22.01.01 10:38--------------------------------

 ---------------------------------------------------------------------------*/
void	SvtAppFilterOptions_Impl::Commit()
{
	Sequence<OUString> aNames(2);
	OUString* pNames = aNames.getArray();
	pNames[0] = C2U("Load");
	pNames[1] = C2U("Save");
	Sequence<Any> aValues(aNames.getLength());
	Any* pValues = aValues.getArray();

	const Type& rType = ::getBooleanCppuType();
	pValues[0].setValue(&bLoadVBA, rType);
	pValues[1].setValue(&bSaveVBA, rType);

	PutProperties(aNames, aValues);
}
/* -----------------------------22.01.01 10:38--------------------------------

 ---------------------------------------------------------------------------*/
void	SvtAppFilterOptions_Impl::Load()
{
	Sequence<OUString> aNames(2);
	OUString* pNames = aNames.getArray();
	pNames[0] = C2U("Load");
	pNames[1] = C2U("Save");

	Sequence<Any> aValues = GetProperties(aNames);
	const Any* pValues = aValues.getConstArray();

	if(pValues[0].hasValue())
		bLoadVBA = *(sal_Bool*)pValues[0].getValue();
	if(pValues[1].hasValue())
		bSaveVBA = *(sal_Bool*)pValues[1].getValue();
}

// -----------------------------------------------------------------------
class SvtCalcFilterOptions_Impl : public SvtAppFilterOptions_Impl
{
	sal_Bool				bLoadExecutable;
public:
	SvtCalcFilterOptions_Impl(const OUString& rRoot) :
		SvtAppFilterOptions_Impl(rRoot),
		bLoadExecutable(sal_False)
	{}
	virtual void			Commit();
	void					Load();

	sal_Bool				IsLoadExecutable() const {return bLoadExecutable;}
	void					SetLoadExecutable(sal_Bool bSet)
							{
								if(bSet != bLoadExecutable)
									SetModified();
								bLoadExecutable = bSet;
							}
};

void SvtCalcFilterOptions_Impl::Commit()
{
	SvtAppFilterOptions_Impl::Commit();

	Sequence<OUString> aNames(1);
	aNames[0] = C2U("Executable");
	Sequence<Any> aValues(1);
	aValues[0] <<= bLoadExecutable;

	PutProperties(aNames, aValues);
}

void SvtCalcFilterOptions_Impl::Load()
{
	SvtAppFilterOptions_Impl::Load();

	Sequence<OUString> aNames(1);
	aNames[0] = C2U("Executable");

	Sequence<Any> aValues = GetProperties(aNames);
	const Any* pValues = aValues.getConstArray();
	if(pValues[0].hasValue())
		bLoadExecutable = *(sal_Bool*)pValues[0].getValue();
}

/* -----------------------------22.01.01 10:32--------------------------------

 ---------------------------------------------------------------------------*/
struct SvtFilterOptions_Impl
{
    ULONG nFlags;
    SvtAppFilterOptions_Impl aWriterCfg;
    SvtCalcFilterOptions_Impl aCalcCfg;
    SvtAppFilterOptions_Impl aImpressCfg;

    SvtFilterOptions_Impl() :
        aWriterCfg(C2U("Office.Writer/Filter/Import/VBA")),
        aCalcCfg(C2U("Office.Calc/Filter/Import/VBA")),
        aImpressCfg(C2U("Office.Impress/Filter/Import/VBA"))
    {
        nFlags = FILTERCFG_WORD_CODE |
            FILTERCFG_WORD_STORAGE |
            FILTERCFG_EXCEL_CODE |
            FILTERCFG_EXCEL_STORAGE |
            FILTERCFG_PPOINT_CODE |
            FILTERCFG_PPOINT_STORAGE |
            FILTERCFG_MATH_LOAD |
            FILTERCFG_MATH_SAVE |
            FILTERCFG_WRITER_LOAD |
            FILTERCFG_WRITER_SAVE |
            FILTERCFG_CALC_LOAD |
            FILTERCFG_CALC_SAVE |
            FILTERCFG_IMPRESS_LOAD |
            FILTERCFG_IMPRESS_SAVE | 
            FILTERCFG_USE_ENHANCED_FIELDS;
        Load();
    }

    void SetFlag( ULONG nFlag, BOOL bSet );
    BOOL IsFlag( ULONG nFlag ) const;
    void Load()
    {
        aWriterCfg.Load();
        aCalcCfg.Load();
        aImpressCfg.Load();
    }
};
/* -----------------------------22.01.01 10:34--------------------------------

 ---------------------------------------------------------------------------*/
void SvtFilterOptions_Impl::SetFlag( ULONG nFlag, BOOL bSet )
{
	switch(nFlag)
	{
		case FILTERCFG_WORD_CODE:		aWriterCfg.SetLoad(bSet);break;
		case FILTERCFG_WORD_STORAGE:	aWriterCfg.SetSave(bSet);break;
		case FILTERCFG_EXCEL_CODE:		aCalcCfg.SetLoad(bSet);break;
		case FILTERCFG_EXCEL_STORAGE:	aCalcCfg.SetSave(bSet);break;
		case FILTERCFG_EXCEL_EXECTBL:	aCalcCfg.SetLoadExecutable(bSet);break;
		case FILTERCFG_PPOINT_CODE:		aImpressCfg.SetLoad(bSet);break;
		case FILTERCFG_PPOINT_STORAGE:	aImpressCfg.SetSave(bSet);break;
		default:
			if( bSet )
				nFlags |= nFlag;
			else
				nFlags &= ~nFlag;
	}
}
/* -----------------------------22.01.01 10:35--------------------------------

 ---------------------------------------------------------------------------*/
BOOL SvtFilterOptions_Impl::IsFlag( ULONG nFlag ) const
{
	BOOL bRet;
	switch(nFlag)
	{
		case FILTERCFG_WORD_CODE 		: bRet = aWriterCfg.IsLoad();break;
		case FILTERCFG_WORD_STORAGE   	: bRet = aWriterCfg.IsSave();break;
		case FILTERCFG_EXCEL_CODE 	    : bRet = aCalcCfg.IsLoad();break;
		case FILTERCFG_EXCEL_STORAGE    : bRet = aCalcCfg.IsSave();break;
		case FILTERCFG_EXCEL_EXECTBL	: bRet = aCalcCfg.IsLoadExecutable();break;
		case FILTERCFG_PPOINT_CODE 	 	: bRet = aImpressCfg.IsLoad();break;
		case FILTERCFG_PPOINT_STORAGE	: bRet = aImpressCfg.IsSave();break;
		default:
			bRet = 0 != (nFlags & nFlag );
	}
	return bRet;
}

// -----------------------------------------------------------------------

SvtFilterOptions::SvtFilterOptions() :
	ConfigItem( C2U("Office.Common/Filter/Microsoft") ),
	pImp(new SvtFilterOptions_Impl)
{
	RTL_LOGFILE_CONTEXT(aLog, "svtools SvtFilterOptions::SvtFilterOptions()");
	EnableNotification(GetPropertyNames());
	Load();
}
// -----------------------------------------------------------------------
SvtFilterOptions::~SvtFilterOptions()
{
	delete pImp;
}
/* -----------------------------22.01.01 08:45--------------------------------

 ---------------------------------------------------------------------------*/
const Sequence<OUString>& SvtFilterOptions::GetPropertyNames()
{
	static Sequence<OUString> aNames;
	if(!aNames.getLength())
	{
		int nCount = 12;
		aNames.realloc(nCount);
		static const char* aPropNames[] =
		{
			"Import/MathTypeToMath",			//  0
			"Import/WinWordToWriter",			//  1
			"Import/PowerPointToImpress",		//  2
			"Import/ExcelToCalc",				//  3
			"Export/MathToMathType",            //  4
			"Export/WriterToWinWord",           //  5
			"Export/ImpressToPowerPoint",       //  6
			"Export/CalcToExcel",            	//  7
			"Export/EnablePowerPointPreview",	//	8
			"Export/EnableExcelPreview",		//	9
			"Export/EnableWordPreview",			// 10
            "Import/ImportWWFieldsAsEnhancedFields" // 11
		};
		OUString* pNames = aNames.getArray();
		for(int i = 0; i < nCount; i++)
			pNames[i] = C2U(aPropNames[i]);
	}
	return aNames;
}
//-----------------------------------------------------------------------
static ULONG lcl_GetFlag(sal_Int32 nProp)
{
	ULONG nFlag = 0;
	switch(nProp)
	{
		case  0: nFlag = FILTERCFG_MATH_LOAD; break;
		case  1: nFlag = FILTERCFG_WRITER_LOAD; break;
		case  2: nFlag = FILTERCFG_IMPRESS_LOAD; break;
		case  3: nFlag = FILTERCFG_CALC_LOAD; break;
		case  4: nFlag = FILTERCFG_MATH_SAVE; break;
		case  5: nFlag = FILTERCFG_WRITER_SAVE; break;
		case  6: nFlag = FILTERCFG_IMPRESS_SAVE; break;
		case  7: nFlag = FILTERCFG_CALC_SAVE; break;
		case  8: nFlag = FILTERCFG_ENABLE_PPT_PREVIEW; break;
		case  9: nFlag = FILTERCFG_ENABLE_EXCEL_PREVIEW; break;
		case 10: nFlag = FILTERCFG_ENABLE_WORD_PREVIEW; break;
		case 11: nFlag = FILTERCFG_USE_ENHANCED_FIELDS; break;

		default: DBG_ERROR("illegal value");
	}
	return nFlag;
}
/*-- 22.01.01 08:53:03---------------------------------------------------

  -----------------------------------------------------------------------*/
void SvtFilterOptions::Notify( const Sequence<OUString>& )
{
	Load();
}
/*-- 22.01.01 08:53:04---------------------------------------------------

  -----------------------------------------------------------------------*/
void SvtFilterOptions::Commit()
{
	const Sequence<OUString>& aNames = GetPropertyNames();
	Sequence<Any> aValues(aNames.getLength());
	Any* pValues = aValues.getArray();

	const Type& rType = ::getBooleanCppuType();
	for(int nProp = 0; nProp < aNames.getLength(); nProp++)
	{
		ULONG nFlag = lcl_GetFlag(nProp);
		sal_Bool bVal = pImp->IsFlag( nFlag);
		pValues[nProp].setValue(&bVal, rType);

	}
	PutProperties(aNames, aValues);
}
/*-- 22.01.01 08:53:04---------------------------------------------------

  -----------------------------------------------------------------------*/
void SvtFilterOptions::Load()
{
	pImp->Load();
	const Sequence<OUString>& rNames = GetPropertyNames();
	Sequence<Any> aValues = GetProperties(rNames);
	const Any* pValues = aValues.getConstArray();
	DBG_ASSERT(aValues.getLength() == rNames.getLength(), "GetProperties failed");
	if(aValues.getLength() == rNames.getLength())
	{
		for(int nProp = 0; nProp < rNames.getLength(); nProp++)
		{
			if(pValues[nProp].hasValue())
			{
				sal_Bool bVal = *(sal_Bool*)pValues[nProp].getValue();
				ULONG nFlag = lcl_GetFlag(nProp);
				pImp->SetFlag( nFlag, bVal);
			}
		}
	}
}
// -----------------------------------------------------------------------

void SvtFilterOptions::SetLoadWordBasicCode( BOOL bFlag )
{
	pImp->SetFlag( FILTERCFG_WORD_CODE, bFlag );
	SetModified();
}

BOOL SvtFilterOptions::IsLoadWordBasicCode() const
{
	return pImp->IsFlag( FILTERCFG_WORD_CODE );
}

void SvtFilterOptions::SetLoadWordBasicStorage( BOOL bFlag )
{
	pImp->SetFlag( FILTERCFG_WORD_STORAGE, bFlag );
	SetModified();
}

BOOL SvtFilterOptions::IsLoadWordBasicStorage() const
{
	return pImp->IsFlag( FILTERCFG_WORD_STORAGE );
}

// -----------------------------------------------------------------------

void SvtFilterOptions::SetLoadExcelBasicCode( BOOL bFlag )
{
	pImp->SetFlag( FILTERCFG_EXCEL_CODE, bFlag );
	SetModified();
}

BOOL SvtFilterOptions::IsLoadExcelBasicCode() const
{
	return pImp->IsFlag( FILTERCFG_EXCEL_CODE );
}

void SvtFilterOptions::SetLoadExcelBasicExecutable( BOOL bFlag )
{
	pImp->SetFlag( FILTERCFG_EXCEL_EXECTBL, bFlag );
	SetModified();
}

BOOL SvtFilterOptions::IsLoadExcelBasicExecutable() const
{
	return pImp->IsFlag( FILTERCFG_EXCEL_EXECTBL );
}

void SvtFilterOptions::SetLoadExcelBasicStorage( BOOL bFlag )
{
	pImp->SetFlag( FILTERCFG_EXCEL_STORAGE, bFlag );
	SetModified();
}

BOOL SvtFilterOptions::IsLoadExcelBasicStorage() const
{
	return pImp->IsFlag( FILTERCFG_EXCEL_STORAGE );
}

// -----------------------------------------------------------------------

void SvtFilterOptions::SetLoadPPointBasicCode( BOOL bFlag )
{
	pImp->SetFlag( FILTERCFG_PPOINT_CODE, bFlag );
	SetModified();
}

BOOL SvtFilterOptions::IsLoadPPointBasicCode() const
{
	return pImp->IsFlag( FILTERCFG_PPOINT_CODE );
}

void SvtFilterOptions::SetLoadPPointBasicStorage( BOOL bFlag )
{
	pImp->SetFlag( FILTERCFG_PPOINT_STORAGE, bFlag );
	SetModified();
}

BOOL SvtFilterOptions::IsLoadPPointBasicStorage() const
{
	return pImp->IsFlag( FILTERCFG_PPOINT_STORAGE );
}

// -----------------------------------------------------------------------

BOOL SvtFilterOptions::IsMathType2Math() const
{
	return pImp->IsFlag( FILTERCFG_MATH_LOAD );
}

void SvtFilterOptions::SetMathType2Math( BOOL bFlag )
{
	pImp->SetFlag( FILTERCFG_MATH_LOAD, bFlag );
	SetModified();
}

BOOL SvtFilterOptions::IsMath2MathType() const
{
	return pImp->IsFlag( FILTERCFG_MATH_SAVE );
}

void SvtFilterOptions::SetMath2MathType( BOOL bFlag )
{
	pImp->SetFlag( FILTERCFG_MATH_SAVE, bFlag );
	SetModified();
}


// -----------------------------------------------------------------------
BOOL SvtFilterOptions::IsWinWord2Writer() const
{
	return pImp->IsFlag( FILTERCFG_WRITER_LOAD );
}

void SvtFilterOptions::SetWinWord2Writer( BOOL bFlag )
{
	pImp->SetFlag( FILTERCFG_WRITER_LOAD, bFlag );
	SetModified();
}

BOOL SvtFilterOptions::IsWriter2WinWord() const
{
	return pImp->IsFlag( FILTERCFG_WRITER_SAVE );
}

void SvtFilterOptions::SetWriter2WinWord( BOOL bFlag )
{
	pImp->SetFlag( FILTERCFG_WRITER_SAVE, bFlag );
	SetModified();
}

BOOL SvtFilterOptions::IsUseEnhancedFields() const
{
    return false; // disable for now;
//	return pImp->IsFlag( FILTERCFG_USE_ENHANCED_FIELDS );
}

void SvtFilterOptions::SetUseEnhancedFields( BOOL bFlag )
{
	pImp->SetFlag( FILTERCFG_USE_ENHANCED_FIELDS, bFlag );
	SetModified();
}

// -----------------------------------------------------------------------
BOOL SvtFilterOptions::IsExcel2Calc() const
{
	return pImp->IsFlag( FILTERCFG_CALC_LOAD );
}

void SvtFilterOptions::SetExcel2Calc( BOOL bFlag )
{
	pImp->SetFlag( FILTERCFG_CALC_LOAD, bFlag );
	SetModified();
}

BOOL SvtFilterOptions::IsCalc2Excel() const
{
	return pImp->IsFlag( FILTERCFG_CALC_SAVE );
}

void SvtFilterOptions::SetCalc2Excel( BOOL bFlag )
{
	pImp->SetFlag( FILTERCFG_CALC_SAVE, bFlag );
	SetModified();
}


// -----------------------------------------------------------------------
BOOL SvtFilterOptions::IsPowerPoint2Impress() const
{
	return pImp->IsFlag( FILTERCFG_IMPRESS_LOAD );
}

void SvtFilterOptions::SetPowerPoint2Impress( BOOL bFlag )
{
	pImp->SetFlag( FILTERCFG_IMPRESS_LOAD, bFlag );
	SetModified();
}

BOOL SvtFilterOptions::IsImpress2PowerPoint() const
{
	return pImp->IsFlag( FILTERCFG_IMPRESS_SAVE );
}

void SvtFilterOptions::SetImpress2PowerPoint( BOOL bFlag )
{
	pImp->SetFlag( FILTERCFG_IMPRESS_SAVE, bFlag );
	SetModified();
}

SvtFilterOptions* SvtFilterOptions::Get()
{
	if ( !pOptions )
		pOptions = new SvtFilterOptions;
	return pOptions;
}

// -----------------------------------------------------------------------

BOOL SvtFilterOptions::IsEnablePPTPreview() const
{
	return pImp->IsFlag( FILTERCFG_ENABLE_PPT_PREVIEW );
}


BOOL SvtFilterOptions::IsEnableCalcPreview() const
{
	return pImp->IsFlag( FILTERCFG_ENABLE_EXCEL_PREVIEW );
}


BOOL SvtFilterOptions::IsEnableWordPreview() const
{
	return pImp->IsFlag( FILTERCFG_ENABLE_WORD_PREVIEW );
}


