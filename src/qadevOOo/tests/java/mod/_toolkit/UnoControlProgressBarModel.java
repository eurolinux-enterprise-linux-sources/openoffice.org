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
package mod._toolkit;

import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.uno.XInterface;

import java.io.PrintWriter;

import lib.StatusException;
import lib.TestCase;
import lib.TestEnvironment;
import lib.TestParameters;


/**
* Test for object which is represented by service
* <code>com.sun.star.awt.UnoControlProgressBarModel</code>. <p>
* Object implements the following interfaces :
* <ul>
*  <li> <code>com::sun::star::awt::UnoControlProgressBarModel</code></li>
*  <li> <code>com::sun::star::io::XPersistObject</code></li>
*  <li> <code>com::sun::star::lang::XComponent</code></li>
*  <li> <code>com::sun::star::beans::XPropertySet</code></li>
*  <li> <code>com::sun::star::beans::XMultiPropertySet</code></li>
* </ul>
* This object test <b> is NOT </b> designed to be run in several
* threads concurently.
* @see com.sun.star.awt.UnoControlProgressBarModel
* @see com.sun.star.io.XPersistObject
* @see com.sun.star.lang.XComponent
* @see com.sun.star.beans.XPropertySet
* @see com.sun.star.beans.XMultiPropertySet
* @see ifc.awt._UnoControlProgressBarModel
* @see ifc.io._XPersistObject
* @see ifc.lang._XComponent
* @see ifc.beans._XPropertySet
* @see ifc.beans._XMultiPropertySet
*/
public class UnoControlProgressBarModel extends TestCase {
    /**
    * Creating a Testenvironment for the interfaces to be tested.
    * Creates an instance of the service
    * <code>com.sun.star.awt.UnoControlProgressBarModel</code>.
    *     Object relations created :
    * <ul>
    *  <li> <code>'OBJNAME'</code> for
    *      {@link ifc.io._XPersistObject} </li>
    * </ul>
    */
    public synchronized TestEnvironment createTestEnvironment(TestParameters Param, 
                                                              PrintWriter log)
        throws StatusException {
        XInterface oObj = null;

        try {
            oObj = (XInterface) ((XMultiServiceFactory) Param.getMSF()).createInstance(
                           "com.sun.star.awt.UnoControlProgressBarModel");
        } catch (Exception e) {
        }

        log.println("creating a new environment for object");

        TestEnvironment tEnv = new TestEnvironment(oObj);

        tEnv.addObjRelation("OBJNAME", "stardiv.vcl.controlmodel.ProgressBar");

        return tEnv;
    } // finish method getTestEnvironment
}