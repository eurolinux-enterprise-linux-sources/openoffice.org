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

#ifndef _OBJTEST_HXX
#define _OBJTEST_HXX

#include <tools/fsys.hxx>


#include <testapp.hxx>
#include <testtool.hxx>
#include "cmdstrm.hxx"
#include <basic/basicrt.hxx>
#include <vcl/smartid.hxx>
#include "httprequest.hxx"

#include <map>

//	#define ID_BeginBlock	1
//	#define ID_EndBlock		2
#define ID_Kontext		3
#define ID_GetError		4
#define ID_ErrorDummy	5            // Intern zum Behandlen von Fehlenden Controls	und Methoden
#define ID_RecordError	6
#define ID_Start		7
//#define ID_Kill			8
//#define ID_Reset		9
//	#define ID_AppAbort		10
#define ID_Dispatch		11
#define ID_UNODispatch	12
#define ID_Control		13
#define ID_StartUse		14
#define ID_Use			15
#define ID_FinishUse	16
#define ID_CaseLog		17
#define ID_ExceptLog	18
#define ID_PrintLog		19
#define ID_WarnLog		20
#define ID_ErrorLog		21
#define ID_EnableQaErrors 22
#define ID_QAErrorLog	23
#define ID_MaybeAddErr	24
#define ID_ClearError	25
#define ID_GetNextCloseWindow	26
#define ID_RemoteCommand 27
#define ID_SaveIDs		28
#define ID_AutoExecute	29
#define ID_Execute		30
#define ID_DialogHandler 31
#define ID_GetUnoApp	32
#define ID_GetIServer	33
#define ID_RemoteCommandDelay 34
#define ID_GetApplicationPath 35
#define ID_GetCommonApplicationPath 36
#define ID_MakeIniFileName 37
#define ID_StringControl   38
#define ID_Wait            39
#define ID_GetErrorCount   40
#define ID_GetWarningCount 41
#define ID_GetQAErrorCount 42
#define ID_GetUseFileWarningCount 43
#define ID_GetErrorList    44
#define ID_GetWarningList  45
#define ID_GetQAErrorList  46
#define ID_GetUseFileWarningList 47
#define ID_GetTestCaseName 48
#define ID_GetTestCaseFileName 49
#define ID_GetTestCaseLineNr 50
#define ID_StopOnSyntaxError 51
#define ID_SetChildEnv     52
#define ID_GetChildEnv     53
#define ID_GetLinkDestination 54
#define ID_GetRegistryValue 55
#define ID_KillApp         56
#define ID_HTTPSend        57
#define ID_HTTPSetProxy    58
#define ID_DoNothing       99

void ReadFlatArray( const ControlDefLoad arWas [], CNames *&pNames );

class ControlData
{
public:
	String Kurzname;
	SmartId aUId;
};

DBG_NAMEEX( ControlItem )
DBG_NAMEEX( ControlDef )
class ControlItem
{
private:
	void InitData() { pData = new ControlData; }
public:
	ControlData *pData;

	ControlItem( const char *Name, SmartId aUIdP );
	ControlItem( const String &Name, SmartId aUIdP );
//	ControlItem( const String &Name, const String &URL, const URLType aType );
//    ControlItem( const String &Name, const String &URL, const ULONG nUId );
//    ControlItem( const char *Name, const String &URL, const ULONG nUId );
	ControlItem( ControlData *pDataP );
	virtual ~ControlItem() {
DBG_DTOR(ControlItem,0);
		delete pData;
	}
	virtual BOOL operator < (const ControlItem &rPar)=0;
	virtual BOOL operator == (const ControlItem &rPar)=0;
//	void Write( SvStream &aStream );
};

SV_DECL_PTRARR_SORT_DEL(CNames, ControlItem*, 50, 10)

#define MK_SON_ACCESS( ClassName )\
	BOOL SonInsert( const ClassName *pNewEntry ) { return pSons->C40_PTR_INSERT( ControlItem, (ControlItem*&)pNewEntry ); }\
	BOOL SonSeek_Entry( const ClassName *pSearchEntry, USHORT *nRes = NULL) { return pSons->Seek_Entry( pSearchEntry, nRes ); }\
	ClassName* SonGetObject( USHORT nNr ) { return (ClassName*)pSons->GetObject( nNr ); }

class ControlSon
{
protected:
	CNames *pSons;		// um sicherzustelle, da� nur S�hne des richtien Type reinkommen

public:
	ControlSon() : pSons( NULL ) {};
	~ControlSon();
//	void Write( SvStream &aStream );

	USHORT Son_Count() { return pSons->Count(); }
	void Sons( CNames *pNewSons ) { pSons = pNewSons; }
	CNames*& GetSons() { return pSons; }
};

class ControlItemSon : public ControlItem, public ControlSon
{
public:
	ControlItemSon(const char *Name, SmartId aUIdP )
		: ControlItem( Name, aUIdP ) {}
	ControlItemSon(const String &Name, SmartId aUIdP );
//	ControlItemSon(const String &Name, const String &URL, const URLType aType );
//    ControlItemSon(const String &Name, const String &URL, const ULONG nUId );
//    ControlItemSon(const char *Name, const String &URL, const ULONG nUId );
//	void Write( SvStream &aStream );
};

class ControlDef : public ControlItemSon
{
public:
	ControlDef(const char *Name, SmartId aUIdP )
		: ControlItemSon( Name, aUIdP ) {DBG_CTOR(ControlDef,0);}
	ControlDef(const String &Name, SmartId aUIdP );
//	ControlDef(const String &Name, const String &URL, const URLType aType );
	ControlDef(const String &aOldName, const String &aNewName, ControlDef *pOriginal, BOOL bWithSons = FALSE );
	~ControlDef() {DBG_DTOR(ControlDef,0);}
	virtual BOOL operator < (const ControlItem &rPar);
	virtual BOOL operator == (const ControlItem &rPar);
	void Write( SvStream &aStream );
	MK_SON_ACCESS( ControlDef )
};

class ControlItemUId : public ControlItem
{
public:
	ControlItemUId(String Name, SmartId aUIdP)
        : ControlItem( Name, aUIdP){}
	virtual BOOL operator < (const ControlItem &rPar);
	virtual BOOL operator == (const ControlItem &rPar);
};

class ControlItemUIdSon : public ControlItemUId, public ControlSon
{
public:
	ControlItemUIdSon(String Name, SmartId aUIdP) : ControlItemUId( Name, aUIdP) {}
	MK_SON_ACCESS( ControlItemUId )
};

class ReverseName : public ControlItemUId
{
public:
	ULONG LastSequence;

	ReverseName(String Name, SmartId aUIdP, ULONG nSeq) : ControlItemUId( Name, aUIdP), LastSequence(nSeq) {}
};

class CRevNames: public CNames
{
public:
	void Insert( String aName, SmartId aUId, ULONG nSeq );
	String GetName( SmartId aUId );
	void Invalidate ( ULONG nSeq );
};


class SbxTransportMethod: public SbxMethod
{
public:
	SbxTransportMethod( SbxDataType );
	ULONG nValue;
	String aUnoSlot;
};
SV_DECL_IMPL_REF(SbxTransportMethod);

class Controls: public SbxObject
{
public:
	Controls( String aCName );
	~Controls();
	void ChangeListener( SbxObject* pParent );

	void SFX_NOTIFY( SfxBroadcaster&, const TypeId&, const SfxHint& rHint, const TypeId& );
	virtual SbxVariable* Find( const String&, SbxClassType );
	SbxTransportMethodRef pMethodVar;	  // zum Transport von Find nach Notify
	static CNames  *pClasses;
	static ControlDefLoad __READONLY_DATA arClasses [];
};
SV_DECL_IMPL_REF(Controls);

typedef std::map< String, String > Environment;

class ImplTestToolObj
{
public:
//	ImplTestToolObj()
//	~ImplTestToolObj()

	String ProgParam;			// Parameter der zu Testenden APP; Gesetzt �ber Start

	DirEntry aFileBase;			// Grundpfad f�r die *.sid und *.win Dateien (Aus Configdatei)
	DirEntry aLogFileBase;		// Grundpfad f�r die *.res Dateien (Aus Configdatei)
	DirEntry aHIDDir;			// Verzeichnis, in dem die hid.lst gesucht wird

	SbxTransportMethodRef pNextReturn;	// Verweis auf die Var, die den Returnwert aufnimmt.

	ControlsRef pControlsObj;			// Jeweiliges Objekt, an dem Methoden aufgerufen weden.

#define VAR_POOL_SIZE 8
	SbxTransportMethodRef pMyVars[VAR_POOL_SIZE];  // Falls in Ausdr�cken mehrere verwendet werden

	Time aServerTimeout;


// Profiling Datenfelder
	ULONG LocalStarttime;
	USHORT nNumBorders;
	ULONG naValBorders[4];
	ULONG naNumEntries[5];
	ULONG naRemoteTime[5];
	ULONG naLocalTime[5];

	ULONG nMinRemoteCommandDelay;
	ULONG nMaxRemoteCommandDelay;
	BOOL bDoRemoteCommandDelay;

	BOOL bLnaguageExtensionLoaded;		// Wurde �ber 'use' was geladen? F�r syntax highlighting
	SfxBroadcaster *pTTSfxBroadcaster;

	ULONG nErrorCount;
	ULONG nWarningCount;
	ULONG nQAErrorCount;
	ULONG nIncludeFileWarningCount;

    SbxDimArrayRef xErrorList;
    SbxDimArrayRef xWarningList;
    SbxDimArrayRef xQAErrorList;
    SbxDimArrayRef xIncludeFileWarningList;

    BOOL bIsStart;                      // set tu TRUE while command Start is initiating the communication

    MyBasic* pMyBasic;

    String aTestCaseName;       // holds name of current TestCase
    String aTestCaseFileName;   // holds FileName of current TestCase
    USHORT nTestCaseLineNr;     // holds Line of current TestCase

    BOOL bEnableQaErrors;       // include QA errors in report
    BOOL bDebugFindNoErrors;    // suppress generating errors when find of variables is called for variable viewing purposes

    BOOL bStopOnSyntaxError;    // catch syntax errors in testcases or not

    Environment *pChildEnv;     // Environment Variables for child Process

    HttpRequest *pHttpRequest;  // used for sending HTTP requests
};


class TTFormat
{
public:
	static String ms2s( ULONG nMilliSeconds );
};


#define ADD_TO_LOG( aLogTypep, aMsgp, aFilenamep, nLinep, nCol1p, nCol2p, aRevisionp, pLogList )\
{																		\
	TTLogMsg *pLogMsg = new TTLogMsg();									\
	pLogMsg->aDebugData.aMsg = aMsgp;									\
	pLogMsg->aLogFileName = (pImpl->aLogFileBase + DirEntry(aLogFileName)).GetFull();	\
	pLogMsg->aDebugData.aFilename = aFilenamep;							\
	pLogMsg->aDebugData.nLine = nLinep;									\
	pLogMsg->aDebugData.nCol1 = nCol1p;									\
	pLogMsg->aDebugData.nCol2 = nCol2p;									\
	pLogMsg->aDebugData.aLogType = aLogTypep;							\
	aLogHdl.Call( pLogMsg );											\
    void* pDummyForWarningAboutNULL = pLogList;                         \
    if( pDummyForWarningAboutNULL )														\
    {																	\
        SbxDimArray* pLogArray = (SbxDimArray*)pLogList;				\
        SbxVariable* pLogLine = new SbxVariable( SbxSTRING );				\
        String aCollect;												\
        aCollect.Append( pLogMsg->aDebugData.aFilename );				\
        aCollect.AppendAscii( ";" );									\
        aCollect.Append( String::CreateFromInt32( nLinep ) );			\
        aCollect.AppendAscii( ";" );									\
        aCollect.Append( aRevisionp );				\
        aCollect.AppendAscii( ";" );									\
        aCollect.Append( pLogMsg->aDebugData.aMsg );										\
        pLogLine->PutString( aCollect );									\
        pLogArray->Insert( pLogLine, pLogArray->Count() );	\
    }																	\
    delete pLogMsg;														\
}																		\


#define ADD_RUN_LOG()													\
	ADD_TO_LOG(LOG_RUN, String(), String(), 0, 0, 0, String(), NULL)					\

#define ADD_ERROR_LOG(aMsg, aFilename, nLine, nCol1, nCol2, aRevision)				\
{																		\
	ADD_TO_LOG(LOG_ERROR, aMsg, aFilename, nLine, nCol1, nCol2, aRevision, &pImpl->xErrorList) \
	pImpl->nErrorCount++;												\
}

#define ADD_CALL_STACK_LOG(aMsg, aFilename, nLine, nCol1, nCol2)		\
	ADD_TO_LOG(LOG_CALL_STACK, aMsg, aFilename, nLine, nCol1, nCol2, String(), NULL)	\


#define ADD_AUTO_LOG(aLogType, aMsg, pLogList)							\
{																		\
	if ( BasicRuntimeAccess::HasRuntime() )								\
	{																	\
		BasicRuntime aRun = BasicRuntimeAccess::GetRuntime();			\
		ADD_TO_LOG(aLogType, aMsg, aRun.GetModuleName(SbxNAME_SHORT_TYPES),	\
			aRun.GetLine(), aRun.GetCol1(), aRun.GetCol2(), aRun.GetSourceRevision(), pLogList)	\
	}																	\
	else																\
	{																	\
		ADD_TO_LOG(aLogType, aMsg, UniString(), 0, 0, STRING_LEN, String(), pLogList)	\
	}																	\
}																		\

#define ADD_CASE_LOG(aMsg)												\
	ADD_AUTO_LOG(LOG_TEST_CASE, aMsg, NULL)								\

#define ADD_MESSAGE_LOG(aMsg)											\
	ADD_AUTO_LOG(LOG_MESSAGE, aMsg, NULL)								\

#define ADD_WARNING_LOG(aMsg)											\
{																		\
	ADD_AUTO_LOG(LOG_WARNING, aMsg, &pImpl->xWarningList)				\
	pImpl->nWarningCount++;												\
}

#define ADD_WARNING_LOG2(aMsg, aFilename, nLine )						\
{																		\
	ADD_TO_LOG(LOG_WARNING, aMsg, aFilename, nLine, 0, STRING_LEN, String(), &pImpl->xWarningList) \
	pImpl->nWarningCount++;												\
}

#define ADD_ASSERTION_LOG(aMsg)											\
	ADD_AUTO_LOG(LOG_ASSERTION, aMsg, NULL)								\

#define ADD_QA_ERROR_LOG(aMsg)											\
    if ( pImpl->bEnableQaErrors )                                       \
    {                                                                   \
	    ADD_AUTO_LOG(LOG_QA_ERROR, aMsg, &pImpl->xQAErrorList)			\
    	pImpl->nQAErrorCount++;											\
    }

#endif
