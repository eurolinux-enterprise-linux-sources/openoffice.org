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
#include "precompiled_rsc.hxx"
/****************** I N C L U D E S **************************************/

// C and C++ Includes.

// Solar Definitionen
#include <tools/solar.h>

// Programmabhaengige Includes.
#include <rscclobj.hxx>
#include <rsctop.hxx>

/****************** C O D E **********************************************/

/****************** R e f N o d e ****************************************/
/*************************************************************************
|*
|*    RefNode::RefNode()
|*
|*    Beschreibung
|*    Ersterstellung    MM 03.05.91
|*    Letzte Aenderung  MM 03.05.91
|*
*************************************************************************/
RefNode::RefNode( Atom nTyp ){
    pObjBiTree = 0;
    nTypNameId = nTyp;
}

/*************************************************************************
|*
|*    RefNode::GetId()
|*
|*    Beschreibung
|*    Ersterstellung    MM 29.10.91
|*    Letzte Aenderung  MM 29.10.91
|*
*************************************************************************/
sal_uInt32 RefNode::GetId() const
{
    return( nTypNameId );
}

/*************************************************************************
|*
|*    RefNode::PutObjNode()
|*
|*    Beschreibung      NAME.DOC
|*    Ersterstellung    MM 21.03.90
|*    Letzte Aenderung  MM 27.06.90
|*
*************************************************************************/
BOOL RefNode::PutObjNode( ObjNode * pPutObject ){
// insert a node in the b-tree pObjBiTree
// if the node with the same name is in pObjBiTree,
// return FALSE and no insert,

    if( pObjBiTree )
        return( pObjBiTree->Insert( pPutObject ) );

    pObjBiTree = pPutObject;
    return( TRUE );
}

/****************** O b j N o d e ****************************************/
/*************************************************************************
|*
|*    RefNode::GetObjNode()
|*
|*    Beschreibung      NAME.DOC
|*    Ersterstellung    MM 21.03.90
|*    Letzte Aenderung  MM 27.06.90
|*
*************************************************************************/
ObjNode * RefNode :: GetObjNode( const RscId & rRscId ){
// insert a node in the b-tree pObjBiTree
// if the node with the same name is in pObjBiTree,
// return NULL and no insert,
// if not return the pointer to the Object

    if( pObjBiTree )
        return( pObjBiTree->Search( rRscId ) );
    return( NULL );
}

/*************************************************************************
|*
|*    ObjNode::ObjNode()
|*
|*    Beschreibung
|*    Ersterstellung    MM 15.05.91
|*    Letzte Aenderung  MM 15.05.91
|*
*************************************************************************/
ObjNode::ObjNode( const RscId & rId, CLASS_DATA pData, ULONG lKey ){
    pRscObj  = pData;
    aRscId   = rId;
    lFileKey = lKey;
}

/*************************************************************************
|*
|*    ObjNode::DelObjNode()
|*
|*    Beschreibung
|*    Ersterstellung    MM 09.12.91
|*    Letzte Aenderung  MM 09.12.91
|*
*************************************************************************/
ObjNode * ObjNode::DelObjNode( RscTop * pClass, ULONG nFileKey ){
    ObjNode * pRetNode = this;

    if( Right() )
        pRight = ((ObjNode *)Right())->DelObjNode( pClass, nFileKey );
    if( Left() )
        pLeft = ((ObjNode *)Left())->DelObjNode( pClass, nFileKey );

    if( GetFileKey() == nFileKey ){
        if( GetRscObj() ){
            pClass->Destroy( RSCINST( pClass, GetRscObj() ) );
            rtl_freeMemory( GetRscObj() );
        }
        pRetNode = (ObjNode *)Right();
        if( pRetNode ){
            if( Left() )
                pRetNode->Insert( (ObjNode *)Left() );
        }
        else
            pRetNode = (ObjNode *)Left();
        delete this;
    }
    return pRetNode;
}

/*************************************************************************
|*
|*    ObjNode::GetId()
|*
|*    Beschreibung
|*    Ersterstellung    MM 29.10.91
|*    Letzte Aenderung  MM 29.10.91
|*
*************************************************************************/
sal_uInt32 ObjNode::GetId() const
{
    return( (sal_uInt32)(long)aRscId );
}

/*************************************************************************
|*
|*    ObjNode::IsConsistent()
|*
|*    Beschreibung
|*    Ersterstellung    MM 23.09.91
|*    Letzte Aenderung  MM 23.09.91
|*
*************************************************************************/
BOOL ObjNode::IsConsistent( RscInconsList * pList )
{
    BOOL bRet = TRUE;

    if( (long)aRscId > 0x7FFF || (long)aRscId < 1 )
	{
        bRet = FALSE;
        if( pList )
            pList->Insert( new RscInconsistent( aRscId, aRscId ) );
    }
    else
	{
        if( Left() )
		{
            if( !((ObjNode *)Left())->IsConsistent( pList ) )
                bRet = FALSE;
            if( ((ObjNode *)Left())->aRscId >= aRscId )
			{
                bRet = FALSE;
                if( pList )
                    pList->Insert(
                        new RscInconsistent( ((ObjNode *)Left())->GetRscId(),
                                             GetRscId() ) );
            }
        };
        if( Right() )
		{
            if( ((ObjNode *)Right())->aRscId <= aRscId )
			{
                bRet = FALSE;
                if( pList )
                    pList->Insert(
                        new RscInconsistent( GetRscId(),
                                             ((ObjNode *)Right())->GetRscId() ) );
            }
            if( !((ObjNode *)Right())->IsConsistent( pList ) )
                bRet = FALSE;
        };
    };

    return( bRet );
}
