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
#include "precompiled_connectivity.hxx"
#include "file/fcode.hxx"
#include <osl/diagnose.h>
#include "connectivity/sqlparse.hxx"
#include <i18npool/mslangid.hxx>
#include <vcl/svapp.hxx>
#include <tools/debug.hxx>
#include "TConnection.hxx"
#include <com/sun/star/sdb/SQLFilterOperator.hpp>
#include <comphelper/types.hxx>
#include <com/sun/star/sdb/SQLFilterOperator.hpp>
#include <rtl/logfile.hxx>

using namespace ::comphelper;
using namespace connectivity;
using namespace connectivity::file;
//using namespace ::com::sun::star::uno;
//using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::sdb;
//using namespace ::com::sun::star::container;
//using namespace ::com::sun::star::beans;
//using namespace ::com::sun::star::sdbcx;

TYPEINIT0(OCode);
TYPEINIT1(OOperand, OCode);
TYPEINIT1(OOperandRow, OOperand);
TYPEINIT1(OOperandAttr, OOperandRow);
TYPEINIT1(OOperandParam, OOperandRow);
TYPEINIT1(OOperandValue, OOperand);
TYPEINIT1(OOperandConst, OOperandValue);
TYPEINIT1(OOperandResult, OOperandValue);
TYPEINIT1(OStopOperand, OOperandValue);

TYPEINIT1(OOperator, OCode);
TYPEINIT1(OBoolOperator,OOperator);
TYPEINIT1(OOp_NOT, OBoolOperator);
TYPEINIT1(OOp_AND, OBoolOperator);
TYPEINIT1(OOp_OR, OBoolOperator);
TYPEINIT1(OOp_ISNULL, OBoolOperator);
TYPEINIT1(OOp_ISNOTNULL, OOp_ISNULL);
TYPEINIT1(OOp_LIKE, OBoolOperator);
TYPEINIT1(OOp_NOTLIKE, OOp_LIKE);
TYPEINIT1(OOp_COMPARE, OBoolOperator);
TYPEINIT1(ONumOperator, OOperator);
TYPEINIT1(ONthOperator, OOperator);
TYPEINIT1(OBinaryOperator, OOperator);
TYPEINIT1(OUnaryOperator, OOperator);

//------------------------------------------------------------------
DBG_NAME(OCode )
OCode::OCode()
{
	DBG_CTOR(OCode ,NULL);
}
// -----------------------------------------------------------------------------
OCode::~OCode()
{
	DBG_DTOR(OCode,NULL);
}

//------------------------------------------------------------------
OEvaluateSet* OOperand::preProcess(OBoolOperator* /*pOp*/, OOperand* /*pRight*/)
{
	return NULL;
}
// -----------------------------------------------------------------------------
OOperandRow::OOperandRow(sal_uInt16 _nPos, sal_Int32 _rType)
	: OOperand(_rType)
	, m_nRowPos(_nPos)
{}
//------------------------------------------------------------------
void OOperandRow::bindValue(const OValueRefRow& _pRow)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOperandRow::OOperandRow" );
	OSL_ENSURE(_pRow.isValid(),"NO EMPTY row allowed!");
	m_pRow = _pRow;
	OSL_ENSURE(m_pRow.isValid() && m_nRowPos < m_pRow->get().size(),"Invalid RowPos is >= vector.size()");
	(m_pRow->get())[m_nRowPos]->setBound(sal_True);
}
// -----------------------------------------------------------------------------
void OOperandRow::setValue(const ORowSetValue& _rVal)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOperandRow::setValue" );
	OSL_ENSURE(m_pRow.isValid() && m_nRowPos < m_pRow->get().size(),"Invalid RowPos is >= vector.size()");
	(*(m_pRow->get())[m_nRowPos]) = _rVal;
}
//------------------------------------------------------------------
const ORowSetValue& OOperandRow::getValue() const
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOperandRow::getValue" );
	OSL_ENSURE(m_pRow.isValid() && m_nRowPos < m_pRow->get().size(),"Invalid RowPos is >= vector.size()");
	return (m_pRow->get())[m_nRowPos]->getValue();
}

// -----------------------------------------------------------------------------
void OOperandValue::setValue(const ORowSetValue& _rVal)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOperandValue::setValue" );
	m_aValue = _rVal;
}
// -------------------------------------------------------------------------
sal_Bool OOperandAttr::isIndexed() const
{
	return sal_False;
}
//------------------------------------------------------------------
OOperandParam::OOperandParam(OSQLParseNode* pNode, sal_Int32 _nPos)
	: OOperandRow(static_cast<sal_uInt16>(_nPos), DataType::VARCHAR)		 // Standard-Typ
{
	OSL_ENSURE(SQL_ISRULE(pNode,parameter),"Argument ist kein Parameter");
	OSL_ENSURE(pNode->count() > 0,"Fehler im Parse Tree");
	OSQLParseNode *pMark = pNode->getChild(0);

	String aParameterName;
	if (SQL_ISPUNCTUATION(pMark,"?"))
		aParameterName = '?';
	else if (SQL_ISPUNCTUATION(pMark,":"))
		aParameterName = pNode->getChild(1)->getTokenValue();
	else
	{
		OSL_ASSERT("Fehler im Parse Tree");
	}

    // Parameter-Column aufsetzen mit defult typ, kann zu einem spaeteren Zeitpunkt ueber DescribeParameter
	// genauer spezifiziert werden

	// Identitaet merken (hier eigentlich nicht erforderlich, aber aus
	// Symmetriegruenden ...)

	// todo
	//	OColumn* pColumn = new OFILEColumn(aParameterName,eDBType,255,0,SQL_FLAGS_NULLALLOWED);
	//	rParamColumns->AddColumn(pColumn);

	// der Wert wird erst kurz vor der Auswertung gesetzt
}


//------------------------------------------------------------------
const ORowSetValue& OOperandValue::getValue() const
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOperandValue::getValue" );
	return m_aValue;
}

//------------------------------------------------------------------
OOperandConst::OOperandConst(const OSQLParseNode& rColumnRef, const rtl::OUString& aStrValue)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOperandConst::OOperandConst" );
	switch (rColumnRef.getNodeType())
	{
		case SQL_NODE_STRING:
			m_aValue	= aStrValue;
			m_eDBType	= DataType::VARCHAR;
			m_aValue.setBound(sal_True);
			return;
		case SQL_NODE_INTNUM:
		case SQL_NODE_APPROXNUM:
		{
			m_aValue	= aStrValue.toDouble();
			m_eDBType	= DataType::DOUBLE;
			m_aValue.setBound(sal_True);
			return;
		}
        default:
            break;
	}

	if (SQL_ISTOKEN(&rColumnRef,TRUE))
	{
		m_aValue = 1.0;
		m_eDBType = DataType::BIT;
	}
	else if (SQL_ISTOKEN(&rColumnRef,FALSE))
	{
		m_aValue = 0.0;
		m_eDBType = DataType::BIT;
	}
	else
	{
		OSL_ASSERT("Parse Error");
	}
	m_aValue.setBound(sal_True);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Implementation of the operators

//------------------------------------------------------------------
sal_uInt16 OOperator::getRequestedOperands() const {return 2;}

//------------------------------------------------------------------
sal_Bool OBoolOperator::operate(const OOperand*, const OOperand*) const
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OBoolOperator::operate" );
	return sal_False;
}


//------------------------------------------------------------------
void OBoolOperator::Exec(OCodeStack& rCodeStack)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OBoolOperator::Exec" );
	OOperand  *pRight	= rCodeStack.top();
	rCodeStack.pop();
	OOperand  *pLeft	= rCodeStack.top();
	rCodeStack.pop();

	rCodeStack.push(new OOperandResultBOOL(operate(pLeft, pRight)));
	if (IS_TYPE(OOperandResult,pLeft))
		delete pLeft;
	if (IS_TYPE(OOperandResult,pRight))
		delete pRight;
}
//------------------------------------------------------------------
sal_Bool OOp_NOT::operate(const OOperand* pLeft, const OOperand* ) const
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOp_AND::operate" );
	return !pLeft->isValid();
}
//------------------------------------------------------------------
void OOp_NOT::Exec(OCodeStack& rCodeStack)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOp_ISNULL::Exec" );
	OOperand* pOperand = rCodeStack.top();
	rCodeStack.pop();

	rCodeStack.push(new OOperandResultBOOL(operate(pOperand)));
	if (IS_TYPE(OOperandResult,pOperand))
		delete pOperand;
}
//------------------------------------------------------------------
sal_uInt16 OOp_NOT::getRequestedOperands() const 
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOp_NOT::getRequestedOperands" );
    return 1;
}

//------------------------------------------------------------------
sal_Bool OOp_AND::operate(const OOperand* pLeft, const OOperand* pRight) const
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOp_AND::operate" );
	return pLeft->isValid() && pRight->isValid();
}

//------------------------------------------------------------------
sal_Bool OOp_OR::operate(const OOperand* pLeft, const OOperand* pRight) const
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOp_OR::operate" );
	return pLeft->isValid() || pRight->isValid();
}

//------------------------------------------------------------------
sal_uInt16 OOp_ISNULL::getRequestedOperands() const 
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOp_ISNULL::getRequestedOperands" );
    return 1;
}

//------------------------------------------------------------------
void OOp_ISNULL::Exec(OCodeStack& rCodeStack)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOp_ISNULL::Exec" );
	OOperand* pOperand = rCodeStack.top();
	rCodeStack.pop();

	rCodeStack.push(new OOperandResultBOOL(operate(pOperand)));
	if (IS_TYPE(OOperandResult,pOperand))
		delete pOperand;
}

//------------------------------------------------------------------
sal_Bool OOp_ISNULL::operate(const OOperand* pOperand, const OOperand*) const
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOp_ISNULL::operate" );
	return pOperand->getValue().isNull();
}

//------------------------------------------------------------------
sal_Bool OOp_ISNOTNULL::operate(const OOperand* pOperand, const OOperand*) const
{
	return !OOp_ISNULL::operate(pOperand);
}

//------------------------------------------------------------------
sal_Bool OOp_LIKE::operate(const OOperand* pLeft, const OOperand* pRight) const
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOp_ISNULL::operate" );
	sal_Bool bMatch;
	ORowSetValue aLH(pLeft->getValue());
	ORowSetValue aRH(pRight->getValue());

	if (aLH.isNull() || aRH.isNull())
		bMatch = sal_False;
	else
	{
		bMatch = match(aRH.getString(), aLH.getString(), cEscape);
	}
	return bMatch;
}

//------------------------------------------------------------------
sal_Bool OOp_NOTLIKE::operate(const OOperand* pLeft, const OOperand* pRight) const
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOp_NOTLIKE::operate" );
	return !OOp_LIKE::operate(pLeft, pRight);
}

//------------------------------------------------------------------
sal_Bool OOp_COMPARE::operate(const OOperand* pLeft, const OOperand* pRight) const
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOp_COMPARE::operate" );
	ORowSetValue aLH(pLeft->getValue());
	ORowSetValue aRH(pRight->getValue());

	if (aLH.isNull() || aRH.isNull()) // if (!aLH.getValue() || !aRH.getValue())
		return sal_False;

	sal_Bool bResult = sal_False;
	sal_Int32 eDBType = pLeft->getDBType();

	// Vergleich (je nach Datentyp):
	switch (eDBType)
	{
		case DataType::CHAR:
		case DataType::VARCHAR:
        case DataType::LONGVARCHAR:
		{
			rtl::OUString sLH = aLH, sRH = aRH;
            INT32 nRes = rtl_ustr_compareIgnoreAsciiCase_WithLength
                (
                 sLH.pData->buffer,
                 sLH.pData->length,
                 sRH.pData->buffer,
                 sRH.pData->length );
			switch(aPredicateType)
			{
				case SQLFilterOperator::EQUAL:			bResult = (nRes == 0); break;
				case SQLFilterOperator::NOT_EQUAL:			bResult = (nRes != 0); break;
				case SQLFilterOperator::LESS:				bResult = (nRes <  0); break;
				case SQLFilterOperator::LESS_EQUAL:		bResult = (nRes <= 0); break;
				case SQLFilterOperator::GREATER:			bResult = (nRes >  0); break;
				case SQLFilterOperator::GREATER_EQUAL:	bResult = (nRes >= 0); break;
				default:						bResult = sal_False;
			}
		} break;
		case DataType::TINYINT:
		case DataType::SMALLINT:
		case DataType::INTEGER:
		case DataType::DECIMAL:
		case DataType::NUMERIC:
		case DataType::REAL:
		case DataType::DOUBLE:
		case DataType::BIT:
		case DataType::TIMESTAMP:
		case DataType::DATE:
		case DataType::TIME:
		{
			double n = aLH ,m = aRH;

			switch (aPredicateType)
			{
				case SQLFilterOperator::EQUAL:			bResult = (n == m); break;
				case SQLFilterOperator::LIKE:				bResult = (n == m); break;
				case SQLFilterOperator::NOT_EQUAL:			bResult = (n != m); break;
				case SQLFilterOperator::NOT_LIKE:			bResult = (n != m); break;
				case SQLFilterOperator::LESS:				bResult = (n < m); break;
				case SQLFilterOperator::LESS_EQUAL:		bResult = (n <= m); break;
				case SQLFilterOperator::GREATER:			bResult = (n > m); break;
				case SQLFilterOperator::GREATER_EQUAL:	bResult = (n >= m); break;
				default:						bResult = sal_False;
			}
		} break;
		default:
			bResult = aLH == aRH;
	}
	return bResult;
}

//------------------------------------------------------------------
void ONumOperator::Exec(OCodeStack& rCodeStack)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "ONumOperator::Exec" );

	OOperand  *pRight	= rCodeStack.top();
	rCodeStack.pop();
	OOperand  *pLeft	= rCodeStack.top();
	rCodeStack.pop();

	rCodeStack.push(new OOperandResultNUM(operate(pLeft->getValue(), pRight->getValue())));
	if (IS_TYPE(OOperandResult,pLeft))
		delete pLeft;
	if (IS_TYPE(OOperandResult,pRight))
		delete pRight;
}
//------------------------------------------------------------------
double OOp_ADD::operate(const double& fLeft,const double& fRight) const
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOp_ADD::operate" );
	return fLeft + fRight;
}

//------------------------------------------------------------------
double OOp_SUB::operate(const double& fLeft,const double& fRight) const
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOp_SUB::operate" );
	return fLeft - fRight;
}

//------------------------------------------------------------------
double OOp_MUL::operate(const double& fLeft,const double& fRight) const
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOp_MUL::operate" );
	return fLeft * fRight;
}

//------------------------------------------------------------------
double OOp_DIV::operate(const double& fLeft,const double& fRight) const
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOp_DIV::operate" );
	return fLeft / fRight;
}
// -----------------------------------------------------------------------------
OEvaluateSet* OOperandAttr::preProcess(OBoolOperator* /*pOp*/, OOperand* /*pRight*/)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OOperandAttr::preProcess" );
	return NULL;
}
//------------------------------------------------------------------
void ONthOperator::Exec(OCodeStack& rCodeStack)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "ONthOperator::Exec" );
	::std::vector<ORowSetValue> aValues;
	::std::vector<OOperand*> aOperands;
	OOperand* pOperand;
	do
	{
		OSL_ENSURE(!rCodeStack.empty(),"Stack must be none empty!");
		pOperand	= rCodeStack.top();
		rCodeStack.pop();
		if ( !IS_TYPE(OStopOperand,pOperand) )
			aValues.push_back( pOperand->getValue() );
		aOperands.push_back( pOperand );
	}
	while ( !IS_TYPE(OStopOperand,pOperand) );

	rCodeStack.push(new OOperandResult(operate(aValues)));

	::std::vector<OOperand*>::iterator aIter = aOperands.begin();
	::std::vector<OOperand*>::iterator aEnd = aOperands.end();
	for (; aIter != aEnd; ++aIter)
	{
		if (IS_TYPE(OOperandResult,*aIter))
			delete *aIter;
	}
}
//------------------------------------------------------------------
void OBinaryOperator::Exec(OCodeStack& rCodeStack)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OBinaryOperator::Exec" );
	OOperand  *pRight	= rCodeStack.top();
	rCodeStack.pop();
	OOperand  *pLeft	= rCodeStack.top();
	rCodeStack.pop();

	if ( !rCodeStack.empty() && IS_TYPE(OStopOperand,rCodeStack.top()) )
		rCodeStack.pop();

	rCodeStack.push(new OOperandResult(operate(pLeft->getValue(),pRight->getValue())));
	if (IS_TYPE(OOperandResult,pRight))
		delete pRight;
	if (IS_TYPE(OOperandResult,pLeft))
		delete pLeft;
}
//------------------------------------------------------------------
void OUnaryOperator::Exec(OCodeStack& rCodeStack)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "file", "Ocke.Janssen@sun.com", "OUnaryOperator::Exec" );
	OSL_ENSURE(!rCodeStack.empty(),"Stack is empty!");
	OOperand* pOperand = rCodeStack.top();
	rCodeStack.pop();

	rCodeStack.push(new OOperandResult(operate(pOperand->getValue())));
	if (IS_TYPE(OOperandResult,pOperand))
		delete pOperand;
}
// -----------------------------------------------------------------------------
sal_uInt16 OUnaryOperator::getRequestedOperands() const {return 1;}



