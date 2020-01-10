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
#include "precompiled_formula.hxx"

#include "formula/formulahelper.hxx"
#include <unotools/charclass.hxx>
#include <svtools/syslocale.hxx>

namespace formula
{

    namespace 
    {
        //============================================================================
        class OEmptyFunctionDescription : public IFunctionDescription
        {
        public:
            OEmptyFunctionDescription(){}
            virtual ~OEmptyFunctionDescription(){}

            virtual ::rtl::OUString getFunctionName() const { return ::rtl::OUString(); }
            virtual const IFunctionCategory* getCategory() const { return NULL; }
            virtual ::rtl::OUString getDescription() const { return ::rtl::OUString(); }
            virtual xub_StrLen getSuppressedArgumentCount() const { return 0; }
            virtual ::rtl::OUString getFormula(const ::std::vector< ::rtl::OUString >& ) const { return ::rtl::OUString(); }
            virtual void fillVisibleArgumentMapping(::std::vector<USHORT>& ) const {}
            virtual void initArgumentInfo()  const {}
            virtual ::rtl::OUString getSignature() const { return ::rtl::OUString(); }
            virtual long getHelpId() const { return 0; }
            virtual sal_uInt32 getParameterCount() const { return 0; }
            virtual ::rtl::OUString getParameterName(sal_uInt32 ) const { return ::rtl::OUString(); }
            virtual ::rtl::OUString getParameterDescription(sal_uInt32 ) const { return ::rtl::OUString(); }
            virtual bool isParameterOptional(sal_uInt32 ) const { return sal_False; }
        };
    }
//===================================================================
//	class FormulaHelper - statische Methoden
//===================================================================

#define FUNC_NOTFOUND 0xffff

FormulaHelper::FormulaHelper(const IFunctionManager* _pFunctionManager) 
    :m_pSysLocale(new SvtSysLocale)
    ,m_pFunctionManager(_pFunctionManager)
    ,open(_pFunctionManager->getSingleToken(IFunctionManager::eOk))
    ,close(_pFunctionManager->getSingleToken(IFunctionManager::eClose))
    ,sep(_pFunctionManager->getSingleToken(IFunctionManager::eSep))
    ,arrayOpen(_pFunctionManager->getSingleToken(IFunctionManager::eArrayOpen))
    ,arrayClose(_pFunctionManager->getSingleToken(IFunctionManager::eArrayClose))
{
    m_pCharClass = m_pSysLocale->GetCharClassPtr();
}
BOOL FormulaHelper::GetNextFunc( const String&	rFormula,
								 BOOL			bBack,
								 xub_StrLen&	rFStart,   // Ein- und Ausgabe
								 xub_StrLen*	pFEnd, 	   // = NULL
								 const IFunctionDescription**	ppFDesc,   // = NULL
                                 ::std::vector< ::rtl::OUString>*	pArgs )  const // = NULL
{
	BOOL		bFound = FALSE;
	xub_StrLen	nOldStart = rFStart;
	String		aFname;

	rFStart = GetFunctionStart( rFormula, rFStart, bBack, ppFDesc ? &aFname : NULL );
	bFound  = ( rFStart != FUNC_NOTFOUND );

	if ( bFound )
	{
		if ( pFEnd )
			*pFEnd = GetFunctionEnd( rFormula, rFStart );

		if ( ppFDesc )
		{
            *ppFDesc = NULL;
            const ::rtl::OUString sTemp( aFname );
            const sal_uInt32 nCategoryCount = m_pFunctionManager->getCount();
            for(sal_uInt32 j= 0; j < nCategoryCount && !*ppFDesc; ++j)
            {
                const IFunctionCategory* pCategory = m_pFunctionManager->getCategory(j);
                const sal_uInt32 nCount = pCategory->getCount();
                for(sal_uInt32 i = 0 ; i < nCount; ++i)
                {
                    const IFunctionDescription* pCurrent = pCategory->getFunction(i);
	                if ( pCurrent->getFunctionName().equalsIgnoreAsciiCase(sTemp) )
                    {
                        *ppFDesc = pCurrent;
                        break;
                    }
                } // for(sal_uInt32 i = 0 ; i < nCount; ++i)
            }
			if ( *ppFDesc && pArgs )
			{
				GetArgStrings( *pArgs,rFormula, rFStart, static_cast<USHORT>((*ppFDesc)->getParameterCount() ));
			}
            else
            {
                static OEmptyFunctionDescription s_aFunctionDescription;
                *ppFDesc = &s_aFunctionDescription;
            }
		}
	}
	else
		rFStart = nOldStart;

	return bFound;
}

//------------------------------------------------------------------------

void FormulaHelper::FillArgStrings( const String&	rFormula,
									xub_StrLen		nFuncPos,
									USHORT			nArgs,
									::std::vector< ::rtl::OUString >& _rArgs ) const
{
	xub_StrLen	nStart	= 0;
	xub_StrLen	nEnd	= 0;
	USHORT		i;
	BOOL		bLast	= FALSE;

	for ( i=0; i<nArgs && !bLast; i++ )
	{
		nStart = GetArgStart( rFormula, nFuncPos, i );

		if ( i+1<nArgs ) // letztes Argument?
		{
			nEnd = GetArgStart( rFormula, nFuncPos, i+1 );

			if ( nEnd != nStart )
                _rArgs.push_back(rFormula.Copy( nStart, nEnd-1-nStart ));
			else
				_rArgs.push_back(String()), bLast = TRUE;
		}
		else
		{
			nEnd = GetFunctionEnd( rFormula, nFuncPos )-1;
			if ( nStart < nEnd )
				_rArgs.push_back( rFormula.Copy( nStart, nEnd-nStart ) );
			else
				_rArgs.push_back(String());
		}
	}

	if ( bLast )
		for ( ; i<nArgs; i++ )
			_rArgs.push_back(String());
}

//------------------------------------------------------------------------

void FormulaHelper::GetArgStrings( ::std::vector< ::rtl::OUString >& _rArgs
                                      ,const String& rFormula,
									   xub_StrLen nFuncPos,
									   USHORT nArgs ) const
{
	if (nArgs)
	{
		FillArgStrings( rFormula, nFuncPos, nArgs, _rArgs );
	}
}

//------------------------------------------------------------------------

inline BOOL IsFormulaText( const CharClass* _pCharClass,const String& rStr, xub_StrLen nPos )
{
	if( _pCharClass->isLetterNumeric( rStr, nPos ) )
		return TRUE;
	else
	{	// In internationalized versions function names may contain a dot
		//  and in every version also an underscore... ;-)
		sal_Unicode c = rStr.GetChar(nPos);
		return c == '.' || c == '_';
	}

}

xub_StrLen FormulaHelper::GetFunctionStart( const String&	rFormula,
										xub_StrLen		nStart,
										BOOL			bBack,
										String*			pFuncName ) const
{
	xub_StrLen nStrLen = rFormula.Len();

	if ( nStrLen < nStart )
		return nStart;

	xub_StrLen	nFStart = FUNC_NOTFOUND;
	xub_StrLen	nParPos	= nStart;

	BOOL bRepeat, bFound;
	do
	{
		bFound  = FALSE;
		bRepeat = FALSE;

		if ( bBack )
		{
			while ( !bFound && (nParPos > 0) )
			{
				if ( rFormula.GetChar(nParPos) == '"' )
				{
					nParPos--;
					while ( (nParPos > 0) && rFormula.GetChar(nParPos) != '"' )
						nParPos--;
					if (nParPos > 0)
						nParPos--;
				}
                else if ( (bFound = ( rFormula.GetChar(nParPos) == '(' ) ) == FALSE )
					nParPos--;
			}
		}
		else
		{
			while ( !bFound && (nParPos < nStrLen) )
			{
				if ( rFormula.GetChar(nParPos) == '"' )
				{
					nParPos++;
					while ( (nParPos < nStrLen) && rFormula.GetChar(nParPos) != '"' )
						nParPos++;
					nParPos++;
				}
                else if ( (bFound = ( rFormula.GetChar(nParPos) == '(' ) ) == FALSE )
					nParPos++;
			}
		}

		if ( bFound && (nParPos > 0) )
		{
			nFStart = nParPos-1;

			while ( (nFStart > 0) && IsFormulaText(m_pCharClass, rFormula, nFStart ))
				nFStart--;
		}

		nFStart++;

		if ( bFound )
		{
			if ( IsFormulaText( m_pCharClass,rFormula, nFStart ) )
			{
									//	Funktion gefunden
				if ( pFuncName )
					*pFuncName = rFormula.Copy( nFStart, nParPos-nFStart );
			}
			else					// Klammern ohne Funktion -> weitersuchen
			{
				bRepeat = TRUE;
				if ( !bBack )
					nParPos++;
				else if (nParPos > 0)
					nParPos--;
				else
					bRepeat = FALSE;
			}
		}
		else						// keine Klammern gefunden
		{
			nFStart = FUNC_NOTFOUND;
			if ( pFuncName )
				pFuncName->Erase();
		}
	}
	while(bRepeat);

	return nFStart;
}

//------------------------------------------------------------------------

xub_StrLen	FormulaHelper::GetFunctionEnd( const String& rStr, xub_StrLen nStart ) const
{
	xub_StrLen nStrLen = rStr.Len();

	if ( nStrLen < nStart )
		return nStart;

	short	nParCount = 0;
    bool    bInArray = false;
	BOOL	bFound = FALSE;

	while ( !bFound && (nStart < nStrLen) )
	{
		sal_Unicode c = rStr.GetChar(nStart);

		if ( c == '"' )
		{
			nStart++;
			while ( (nStart < nStrLen) && rStr.GetChar(nStart) != '"' )
				nStart++;
		}
		else if ( c == open )
			nParCount++;
		else if ( c == close )
		{
			nParCount--;
			if ( nParCount == 0 )
				bFound = TRUE;
			else if ( nParCount < 0 )
			{
				bFound = TRUE;
				nStart--;	// einen zu weit gelesen
			}
		}
        else if ( c == arrayOpen )
        {
            bInArray = true;
        }
        else if ( c == arrayClose )
        {
            bInArray = false;
        }
		else if ( c == sep )
		{
			if ( !bInArray && nParCount == 0 )
			{
				bFound = TRUE;
				nStart--;	// einen zu weit gelesen
			}
		}
		nStart++; // hinter gefundene Position stellen
	}

	return nStart;
}

//------------------------------------------------------------------

xub_StrLen FormulaHelper::GetArgStart( const String& rStr, xub_StrLen nStart, USHORT nArg ) const
{
	xub_StrLen nStrLen = rStr.Len();

	if ( nStrLen < nStart )
		return nStart;

	short	nParCount	= 0;
    bool    bInArray    = false;
	BOOL	bFound		= FALSE;

	while ( !bFound && (nStart < nStrLen) )
	{
		sal_Unicode c = rStr.GetChar(nStart);

		if ( c == '"' )
		{
			nStart++;
			while ( (nStart < nStrLen) && rStr.GetChar(nStart) != '"' )
				nStart++;
		}
		else if ( c == open )
		{
			bFound = ( nArg == 0 );
			nParCount++;
		}
		else if ( c == close )
		{
			nParCount--;
			bFound = ( nParCount == 0 );
		}
        else if ( c == arrayOpen )
        {
            bInArray = true;
        }
        else if ( c == arrayClose )
        {
            bInArray = false;
        }
		else if ( c == sep )
		{
			if ( !bInArray && nParCount == 1 )
			{
				nArg--;
				bFound = ( nArg == 0  );
			}
		}
		nStart++;
	}

	return nStart;
}
// =============================================================================
} // formula
// =============================================================================
