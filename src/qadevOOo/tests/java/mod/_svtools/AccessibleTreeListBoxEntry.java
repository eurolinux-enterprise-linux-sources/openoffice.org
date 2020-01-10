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
package mod._svtools;

import com.sun.star.view.XSelectionSupplier;
import java.io.PrintWriter;

import lib.StatusException;
import lib.TestCase;
import lib.TestEnvironment;
import lib.TestParameters;
import util.AccessibilityTools;
import util.DesktopTools;
import util.SOfficeFactory;

import com.sun.star.accessibility.AccessibleRole;
import com.sun.star.accessibility.XAccessible;
import com.sun.star.awt.XExtendedToolkit;
import com.sun.star.awt.XWindow;
import com.sun.star.beans.PropertyValue;
import com.sun.star.frame.XController;
import com.sun.star.frame.XDesktop;
import com.sun.star.frame.XDispatch;
import com.sun.star.frame.XDispatchProvider;
import com.sun.star.frame.XFrame;
import com.sun.star.frame.XModel;
import com.sun.star.lang.XInitialization;
import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.text.XTextDocument;
import com.sun.star.uno.UnoRuntime;
import com.sun.star.uno.XInterface;
import com.sun.star.util.URL;
import com.sun.star.util.XCloseable;


/**
 * Test for object that implements the following interfaces :
 * <ul>
 *  <li><code>::com::sun::star::accessibility::XAccessibleComponent
 *  </code></li>
 *  <li><code>::com::sun::star::accessibility::XAccessibleEventBroadcaster
 *  </code></li>
 *  <li><code>::com::sun::star::accessibility::XAccessibleContext
 *  </code></li>
 *  <li><code>::com::sun::star::accessibility::XAccessibleSelection
 *  </code></li>
 *  <li><code>::com::sun::star::accessibility::XAccessibleAction
 *  </code></li>
 *  <li><code>::com::sun::star::accessibility::XAccessibleText
 *  </code></li>
 * </ul> <p>
 *
 * @see com.sun.star.accessibility.XAccessibleEventBroadcaster
 * @see com.sun.star.accessibility.XAccessibleContext
 * @see com.sun.star.accessibility.XAccessibleComponent
 * @see com.sun.star.accessibility.XAccessibleSelection
 * @see com.sun.star.accessibility.XAccessibleAction
 * @see com.sun.star.accessibility.XAccessibleText
 * @see ifc.accessibility._XAccessibleEventBroadcaster
 * @see ifc.accessibility._XAccessibleContext
 * @see ifc.accessibility._XAccessibleComponent
 * @see ifc.accessibility._XAccessibleSelection
 * @see ifc.accessibility._XAccessibleAction
 * @see ifc.accessibility._XAccessibleText
 */
public class AccessibleTreeListBoxEntry extends TestCase {
    static XDesktop the_Desk;
    static XTextDocument xTextDoc;

    /**
     * Creates the Desktop service (<code>com.sun.star.frame.Desktop</code>).
     */
    protected void initialize(TestParameters Param, PrintWriter log) {
        the_Desk = (XDesktop) UnoRuntime.queryInterface(XDesktop.class, 
                                                        DesktopTools.createDesktop(
		(XMultiServiceFactory) Param.getMSF()));
    }

    /**
     * Disposes the document, if exists, created in
     * <code>createTestEnvironment</code> method.
     */
    protected void cleanup(TestParameters Param, PrintWriter log) {
        log.println("disposing xTextDoc");

        if (xTextDoc != null) {
            closeDoc();
        }
    }

    /**
     * Creates a text document. Opens the DataSource browser.
     * Creates an instance of the service <code>com.sun.star.awt.Toolkit</code>
     * and gets active top window. Then obtains an accessible object with
     * the role <code>AccessibleRole.LABEL</code> and with the name
     * <code>"Bibliography"</code>.
     * Object relations created :
     * <ul>
     *  <li> <code>'EventProducer'</code> for
     *      {@link ifc.accessibility._XAccessibleEventBroadcaster}:
     *   </li>
     *  <li> <code>'XAccessibleSelection.multiSelection'</code>
     *  of type <code>Boolean</code> for
     *  {@link ifc.accessibility._XAccessibleSelection}:
     *   indicates that component supports single selection mode.</li>
     *   <li> <code>'XAccessibleText.Text'</code> for
     *   {@link ifc.accessibility._XAccessibleText}:
     *    the text of the component</li>
     * </ul>
     *
     * @param tParam test parameters
     * @param log writer to log information while testing
     *
     * @see com.sun.star.awt.Toolkit
     * @see com.sun.star.accessibility.AccessibleRole
     * @see ifc.accessibility._XAccessibleEventBroadcaster
     * @see ifc.accessibility._XAccessibleSelection
     * @see ifc.accessibility._XAccessibleText
     * @see com.sun.star.accessibility.XAccessibleEventBroadcaster
     * @see com.sun.star.accessibility.XAccessibleSelection
     * @see com.sun.star.accessibility.XAccessibleText
     */
    protected TestEnvironment createTestEnvironment(TestParameters tParam, 
                                                    PrintWriter log) {
        log.println("creating a test environment");

        if (xTextDoc != null) {
            closeDoc();
        }

        // get a soffice factory object
        SOfficeFactory SOF = SOfficeFactory.getFactory( (XMultiServiceFactory) tParam.getMSF());

        try {
            log.println("creating a text document");
            xTextDoc = SOF.createTextDoc(null);
        } catch (com.sun.star.uno.Exception e) {
            // Some exception occures.FAILED
            e.printStackTrace(log);
            throw new StatusException("Couldn't create document", e);
        }

        shortWait();

        XModel aModel1 = (XModel) UnoRuntime.queryInterface(XModel.class, 
                                                            xTextDoc);

        XController secondController = aModel1.getCurrentController();

        XDispatchProvider aProv = (XDispatchProvider) UnoRuntime.queryInterface(
                                          XDispatchProvider.class, 
                                          secondController);

        XDispatch getting = null;

        log.println("opening DatasourceBrowser");

        URL the_url = new URL();
        the_url.Complete = ".component:DB/DataSourceBrowser";
        getting = aProv.queryDispatch(the_url, "_beamer", 12);

        PropertyValue[] noArgs = new PropertyValue[0];
        getting.dispatch(the_url, noArgs);

        shortWait();

        XFrame the_frame1 = the_Desk.getCurrentFrame();

        if (the_frame1 == null) {
            log.println("Current frame was not found !!!");
        }

        XFrame the_frame2 = the_frame1.findFrame("_beamer", 4);

        the_frame2.setName("DatasourceBrowser");

        XInterface oObj = null;

        XInitialization xInit = (XInitialization) UnoRuntime.queryInterface(
                                        XInitialization.class, 
                                        the_frame2.getController());

        PropertyValue[] params = new PropertyValue[3];
        PropertyValue param1 = new PropertyValue();
        param1.Name = "DataSourceName";
        param1.Value = "Bibliography";
        params[0] = param1;

        PropertyValue param2 = new PropertyValue();
        param2.Name = "CommandType";
        param2.Value = new Integer(com.sun.star.sdb.CommandType.QUERY);
        params[1] = param2;

        PropertyValue param3 = new PropertyValue();
        param3.Name = "Command";
        param3.Value = "select * from biblio";
        params[2] = param3;
        
        XController xCont = the_frame2.getController();
        
        XSelectionSupplier xSelect = (XSelectionSupplier) UnoRuntime.queryInterface(
            XSelectionSupplier.class, xCont);
        
        try {
            xSelect.select(params);
        } catch (com.sun.star.lang.IllegalArgumentException ex) {
            throw new StatusException("Could not select Biblio-Database", ex);
        }
        

        try {
            shortWait();
            oObj = (XInterface) ( (XMultiServiceFactory) tParam.getMSF())
                                      .createInstance("com.sun.star.awt.Toolkit");
        } catch (com.sun.star.uno.Exception e) {
            throw new StatusException("Couldn't get toolkit", e);
        }
        
        shortWait();

        XExtendedToolkit tk = (XExtendedToolkit) UnoRuntime.queryInterface(
                                      XExtendedToolkit.class, oObj);

        AccessibilityTools at = new AccessibilityTools();                

        XWindow xWindow = (XWindow) UnoRuntime.queryInterface(XWindow.class, 
                                                              tk.getActiveTopWindow());

        XAccessible xRoot = at.getAccessibleObject(xWindow);
        
        at.printAccessibleTree(log, xRoot, tParam.getBool(util.PropertyName.DEBUG_IS_ACTIVE));
        
        oObj = at.getAccessibleObjectForRole(xRoot, AccessibleRole.LABEL, 
                                             "Bibliography");
        
        log.println("ImplementationName: " + util.utils.getImplName(oObj));

        TestEnvironment tEnv = new TestEnvironment(oObj);

        tEnv.addObjRelation("EditOnly", 
                            "This method isn't supported in this component");
        
        //adding ObjectRelation for XAccessibleText
        tEnv.addObjRelation("LimitedBounds", "true");


        tEnv.addObjRelation("EventProducer", 
                            new ifc.accessibility._XAccessibleEventBroadcaster.EventProducer() {
            public void fireEvent() {
            }
        });

        tEnv.addObjRelation("XAccessibleSelection.multiSelection", 
                            new Boolean(false));

        tEnv.addObjRelation("XAccessibleText.Text", "Bibliography");

        return tEnv;
    }

    /**
    * Sleeps for 0.5 sec. to allow StarOffice to react on <code>
    * reset</code> call.
    */
    private void shortWait() {
        try {
            Thread.sleep(5000);
        } catch (InterruptedException e) {
            System.out.println("While waiting :" + e);
        }
    }

    protected void closeDoc() {
        XCloseable closer = (XCloseable) UnoRuntime.queryInterface(
                                    XCloseable.class, xTextDoc);

        try {
            closer.close(true);
        } catch (com.sun.star.util.CloseVetoException e) {
            log.println("Couldn't close document " + e.getMessage());
        } catch (com.sun.star.lang.DisposedException e) {
            log.println("Couldn't close document " + e.getMessage());
        }
    }
}