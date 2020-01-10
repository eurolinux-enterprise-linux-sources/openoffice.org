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

#ifndef SC_DBDOCFUN_HXX
#define SC_DBDOCFUN_HXX

#include "address.hxx"
#include <tools/solar.h>
#include <com/sun/star/uno/Sequence.hxx>

class String;

struct ScImportParam;
struct ScQueryParam;
struct ScSortParam;
struct ScSubTotalParam;

class SfxViewFrame;
class SbaSelectionList;
class ScDBData;
class ScDocShell;
class ScAddress;
class ScRange;
class ScDPObject;

namespace com { namespace sun { namespace star {
    namespace beans {
	    struct PropertyValue;
    }
    namespace sdbc {
        class XResultSet;
    }
} } }

// ---------------------------------------------------------------------------
// -----------------------------------------------------------------
class SbaSelectionList: public List , public SvRefBase
{
public:
	SbaSelectionList():
		List(CONTAINER_MAXBLOCKSIZE,100,100){}
};

SV_DECL_IMPL_REF(SbaSelectionList)


class ScDBDocFunc
{
friend class ScDBFunc;

private:
	ScDocShell&		rDocShell;

public:
					ScDBDocFunc( ScDocShell& rDocSh ): rDocShell(rDocSh) {}
					~ScDBDocFunc() {}

    void			UpdateImport( const String& rTarget, const String& rDBName,
                        const String& rTableName, const String& rStatement,
                        BOOL bNative, BYTE nType,
                        const ::com::sun::star::uno::Reference<
                        ::com::sun::star::sdbc::XResultSet >& xResultSet,
                        const SbaSelectionList* pSelection );

    BOOL			DoImport( SCTAB nTab, const ScImportParam& rParam,
                        const ::com::sun::star::uno::Reference<
                        ::com::sun::star::sdbc::XResultSet >& xResultSet,
                        const SbaSelectionList* pSelection, BOOL bRecord,
                        BOOL bAddrInsert = FALSE );

	BOOL			DoImportUno( const ScAddress& rPos,
								const com::sun::star::uno::Sequence<
									com::sun::star::beans::PropertyValue>& aArgs );

	static void		ShowInBeamer( const ScImportParam& rParam, SfxViewFrame* pFrame );

	BOOL			Sort( SCTAB nTab, const ScSortParam& rSortParam,
							BOOL bRecord, BOOL bPaint, BOOL bApi );

	SC_DLLPUBLIC BOOL			Query( SCTAB nTab, const ScQueryParam& rQueryParam,
							const ScRange* pAdvSource, BOOL bRecord, BOOL bApi );

	BOOL			DoSubTotals( SCTAB nTab, const ScSubTotalParam& rParam,
									const ScSortParam* pForceNewSort,
									BOOL bRecord, BOOL bApi );

	BOOL			AddDBRange( const String& rName, const ScRange& rRange, BOOL bApi );
	BOOL			DeleteDBRange( const String& rName, BOOL bApi );
	BOOL			RenameDBRange( const String& rOld, const String& rNew, BOOL bApi );
	BOOL			ModifyDBData( const ScDBData& rNewData, BOOL bApi );	// Name unveraendert

	BOOL			RepeatDB( const String& rDBName, BOOL bRecord, BOOL bApi );

	BOOL			DataPilotUpdate( ScDPObject* pOldObj, const ScDPObject* pNewObj,
										BOOL bRecord, BOOL bApi, BOOL bAllowMove = FALSE );
};



#endif
