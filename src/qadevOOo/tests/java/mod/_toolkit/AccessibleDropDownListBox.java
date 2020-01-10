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

import com.sun.star.accessibility.AccessibleRole;
import com.sun.star.accessibility.XAccessible;
import com.sun.star.accessibility.XAccessibleAction;
import com.sun.star.awt.XWindow;
import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.text.XTextDocument;
import com.sun.star.uno.UnoRuntime;
import com.sun.star.uno.XInterface;

import java.io.PrintWriter;

import lib.StatusException;
import lib.TestCase;
import lib.TestEnvironment;
import lib.TestParameters;

import util.AccessibilityTools;
import util.DesktopTools;
import util.SOfficeFactory;
import util.utils;


/**
 * Test for object which is represented by style list box
 * on toolbar of a document. <p>
 *
 * Object implements the following interfaces :
 * <ul>
 *  <li> <code>::com::sun::star::accessibility::XAccessibleExtendedComponent</code></li>
 *  <li> <code>::com::sun::star::accessibility::XAccessibleEventBroadcaster</code></li>
 *  <li> <code>::com::sun::star::accessibility::XAccessibleComponent</code></li>
 *  <li> <code>::com::sun::star::accessibility::XAccessibleAction</code></li>
 *  <li> <code>::com::sun::star::accessibility::XAccessibleContext</code></li>
 * </ul> <p>
 *
 * @see com.sun.star.accessibility.XAccessibleExtendedComponent
 * @see com.sun.star.accessibility.XAccessibleEventBroadcaster
 * @see com.sun.star.accessibility.XAccessibleComponent
 * @see com.sun.star.accessibility.XAccessibleAction
 * @see com.sun.star.accessibility.XAccessibleContext
 * @see ifc.accessibility._XAccessibleExtendedComponent
 * @see ifc.accessibility._XAccessibleEventBroadcaster
 * @see ifc.accessibility._XAccessibleComponent
 * @see ifc.accessibility._XAccessibleAction
 * @see ifc.accessibility._XAccessibleContext
 */
public class AccessibleDropDownListBox extends TestCase {
    private static XTextDocument xTextDoc = null;
    private static XAccessibleAction action = null;

    /**
     * Finds AccessibleDropDownListBox walking through the
     * accessible component tree of a writer document.
     */
    protected TestEnvironment createTestEnvironment(TestParameters Param, 
                                                    PrintWriter log) {
        XInterface oObj = null;

        shortWait();

        AccessibilityTools at = new AccessibilityTools();

        XWindow xWindow = at.getCurrentWindow(
                                  (XMultiServiceFactory) Param.getMSF(), 
                                  xTextDoc);

        XAccessible xRoot = at.getAccessibleObject(xWindow);

        at.printAccessibleTree(log, xRoot, Param.getBool(util.PropertyName.DEBUG_IS_ACTIVE));
        
        oObj = at.getAccessibleObjectForRole(xRoot, AccessibleRole.COMBO_BOX, 
                                             "", "AccessibleDropDownListBox");

        log.println("ImplementationName " + utils.getImplName(oObj));

        TestEnvironment tEnv = new TestEnvironment(oObj);

        final XAccessibleAction acomp = (XAccessibleAction) UnoRuntime.queryInterface(
                                                XAccessibleAction.class, oObj);
        tEnv.addObjRelation("EventProducer", 
                            new ifc.accessibility._XAccessibleEventBroadcaster.EventProducer() {
            public void fireEvent() {
                try {
                    acomp.doAccessibleAction(0);
                } catch (com.sun.star.lang.IndexOutOfBoundsException e) {
                    throw new StatusException("Can't perform action 0", e);
                }
            }
        });

        return tEnv;
    }

    /**
     * Creates writer document.
     */
    protected void initialize(TestParameters Param, PrintWriter log) {
        try {
            SOfficeFactory SOF = SOfficeFactory.getFactory(
                                         (XMultiServiceFactory) Param.getMSF());
            DesktopTools.dockStylist((XMultiServiceFactory) Param.getMSF());
            xTextDoc = SOF.createTextDoc(null);
        } catch (com.sun.star.uno.Exception e) {
            throw new StatusException("Can't create document", e);
        }
    }

    /**
     * Disposes writer document.
     */
    protected void cleanup(TestParameters Param, PrintWriter log) {
        util.DesktopTools.closeDoc(xTextDoc);
    }

    /**
    * Sleeps for 0.5 sec. to allow StarOffice to react on <code>
    * reset</code> call.
    */
    private void shortWait() {
        try {
            Thread.sleep(500);
        } catch (InterruptedException e) {
            log.println("While waiting :" + e);
        }
    }
}