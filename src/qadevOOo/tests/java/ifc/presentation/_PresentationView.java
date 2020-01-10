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

package ifc.presentation;

import lib.MultiPropertyTest;

public class _PresentationView extends MultiPropertyTest {

    /**
     * Property tester which changes DrawPage.
     */
    protected PropertyTester PageTester = new PropertyTester() {
        protected Object getNewValue(String propName, Object oldValue)
                throws java.lang.IllegalArgumentException {
            if (oldValue.equals(tEnv.getObjRelation("FirstPage")))
                return tEnv.getObjRelation("SecondPage"); else
                return tEnv.getObjRelation("FirstPage");
        }
    } ;

    /**
     * This property must be an XDrawPage
     */
    public void _CurrentPage() {
        log.println("Testing with custom Property tester") ;
        testProperty("CurrentPage", PageTester) ;
    }

}  // finish class _PresentationView


