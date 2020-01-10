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
package mod._sc;

import java.io.PrintWriter;

import lib.Status;
import lib.StatusException;
import lib.TestCase;
import lib.TestEnvironment;
import lib.TestParameters;
import util.AccessibilityTools;
import util.SOfficeFactory;
import util.utils;

import com.sun.star.accessibility.AccessibleRole;
import com.sun.star.accessibility.XAccessible;
import com.sun.star.awt.XWindow;
import com.sun.star.beans.XPropertySet;
import com.sun.star.container.XIndexAccess;
import com.sun.star.container.XNameAccess;
import com.sun.star.frame.XController;
import com.sun.star.frame.XDispatch;
import com.sun.star.frame.XDispatchProvider;
import com.sun.star.frame.XModel;
import com.sun.star.lang.XComponent;
import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.sheet.XSpreadsheet;
import com.sun.star.sheet.XSpreadsheetDocument;
import com.sun.star.sheet.XSpreadsheets;
import com.sun.star.style.XStyleFamiliesSupplier;
import com.sun.star.table.XCell;
import com.sun.star.uno.AnyConverter;
import com.sun.star.uno.Type;
import com.sun.star.uno.UnoRuntime;
import com.sun.star.uno.XInterface;
import com.sun.star.util.URL;
import com.sun.star.util.XCloseable;
import com.sun.star.util.XURLTransformer;


/**
 * Object implements the following interfaces:
 * <ul>
 *  <li> <code>::com::sun::star::accessibility::XAccessibleComponent</code>
 *  </li>
 *  <li> <code>::com::sun::star::accessibility::XAccessibleContext</code>
 *  </li>
 *  <li> <code>::com::sun::star::accessibility::XAccessibleSelection
 *  </code></li>
 *  <li><code>::com::sun::star::accessibility::XAccessibleValue</code>
 *  </li>
 *  <li><code>::com::sun::star::accessibility::XAccessibleEventBroadcaster
 *  </code></li>
 * </ul>
 * @see com.sun.star.accessibility.XAccessibleComponent
 * @see com.sun.star.accessibility.XAccessibleContext
 * @see com.sun.star.accessibility.XAccessibleSelection
 * @see com.sun.star.accessibility.XAccessibleValue
 * @see com.sun.star.accessibility.XAccessibleEventBroadcaster
 * @see ifc.accessibility._XAccessibleEventBroadcaster
 * @see ifc.accessibility._XAccessibleComponent
 * @see ifc.accessibility._XAccessibleContext
 * @see ifc.accessibility._XAccessibleSelection
 * @see ifc.accessibility._XAccessibleTable
 */
public class ScAccessiblePreviewHeaderCell extends TestCase {
    static XSpreadsheetDocument xSheetDoc = null;

    /**
     * Creates a spreadsheet document.
     */
    protected void initialize(TestParameters tParam, PrintWriter log) {
    }

    /**
     * Disposes a spreadsheet document.
     */
    protected void cleanup(TestParameters tParam, PrintWriter log) {
        log.println("    disposing xSheetDoc ");

        if (xSheetDoc != null) {
            try {
                XCloseable oComp = (XCloseable) UnoRuntime.queryInterface(
                                           XCloseable.class, xSheetDoc);
                oComp.close(true);
                xSheetDoc = null;
            } catch (com.sun.star.util.CloseVetoException e) {
                e.printStackTrace(log);
            } catch (com.sun.star.lang.DisposedException e) {
                log.println("document already disposed");
                xSheetDoc = null;
            }
        }
    }

    /**
     * Creating a Testenvironment for the interfaces to be tested.
     * Sets a value of the cell 'A1'. Sets the property 'PrintHeaders'
     * of the style 'Default' of the family 'PageStyles' to true.
     * Switchs the document to preview mode and then obtains the
     * accessible object for the header cell.
     */
    protected synchronized TestEnvironment createTestEnvironment(TestParameters Param, 
                                                                 PrintWriter log) {
        XInterface oObj = null;

        if (xSheetDoc != null) {
            XComponent oComp = (XComponent) UnoRuntime.queryInterface(
                                       XComponent.class, xSheetDoc);
            util.DesktopTools.closeDoc(oComp);
        }

        SOfficeFactory SOF = SOfficeFactory.getFactory( (XMultiServiceFactory) Param.getMSF());

        try {
            log.println("creating a Spreadsheet document");
            xSheetDoc = SOF.createCalcDoc(null);
        } catch (com.sun.star.uno.Exception e) {
            // Some exception occures.FAILED
            e.printStackTrace(log);
            throw new StatusException("Couldn't create document", e);
        }

        XModel xModel = (XModel) UnoRuntime.queryInterface(XModel.class, 
                                                           xSheetDoc);

        XController xController = xModel.getCurrentController();

        //setting value of cell A1
        XCell xCell = null;

        try {
            log.println("Getting spreadsheet");

            XSpreadsheets oSheets = xSheetDoc.getSheets();
            XIndexAccess oIndexSheets = (XIndexAccess) UnoRuntime.queryInterface(
                                                XIndexAccess.class, oSheets);
            XSpreadsheet oSheet = (XSpreadsheet) AnyConverter.toObject(
                                          new Type(XSpreadsheet.class), 
                                          oIndexSheets.getByIndex(0));

            log.println("Getting a cell from sheet");
            xCell = oSheet.getCellByPosition(0, 0);
        } catch (com.sun.star.lang.WrappedTargetException e) {
            e.printStackTrace(log);
            throw new StatusException(
                    "Error getting cell object from spreadsheet document", e);
        } catch (com.sun.star.lang.IndexOutOfBoundsException e) {
            e.printStackTrace(log);
            throw new StatusException(
                    "Error getting cell object from spreadsheet document", e);
        } catch (com.sun.star.lang.IllegalArgumentException e) {
            e.printStackTrace(log);
            throw new StatusException(
                    "Error getting cell object from spreadsheet document", e);
        }

        xCell.setFormula("Value");

        //setting property 'PrintHeaders' of the style 'Default'
        XStyleFamiliesSupplier xSFS = (XStyleFamiliesSupplier) UnoRuntime.queryInterface(
                                              XStyleFamiliesSupplier.class, 
                                              xSheetDoc);
        XNameAccess xNA = xSFS.getStyleFamilies();
        XPropertySet xPropSet = null;

        try {
            Object oPageStyles = xNA.getByName("PageStyles");
            xNA = (XNameAccess) UnoRuntime.queryInterface(XNameAccess.class, 
                                                          oPageStyles);

            Object oDefStyle = xNA.getByName("Default");
            xPropSet = (XPropertySet) UnoRuntime.queryInterface(
                               XPropertySet.class, oDefStyle);
        } catch (com.sun.star.lang.WrappedTargetException e) {
            e.printStackTrace(log);
            throw new StatusException(Status.failed("Couldn't get element"));
        } catch (com.sun.star.container.NoSuchElementException e) {
            e.printStackTrace(log);
            throw new StatusException(Status.failed("Couldn't get element"));
        }

        try {
            xPropSet.setPropertyValue("PrintHeaders", new Boolean(true));
        } catch (com.sun.star.lang.WrappedTargetException e) {
            e.printStackTrace(log);
            throw new StatusException(Status.failed(
                                              "Couldn't set property 'PrintHeaders'"));
        } catch (com.sun.star.lang.IllegalArgumentException e) {
            e.printStackTrace(log);
            throw new StatusException(Status.failed(
                                              "Couldn't set property 'PrintHeaders'"));
        } catch (com.sun.star.beans.PropertyVetoException e) {
            e.printStackTrace(log);
            throw new StatusException(Status.failed(
                                              "Couldn't set property 'PrintHeaders'"));
        } catch (com.sun.star.beans.UnknownPropertyException e) {
            e.printStackTrace(log);
            throw new StatusException(Status.failed(
                                              "Couldn't set property 'PrintHeaders'"));
        }

        //switching to 'Print Preview' mode
        try {
            XDispatchProvider xDispProv = (XDispatchProvider) UnoRuntime.queryInterface(
                                                  XDispatchProvider.class, 
                                                  xController);
            XURLTransformer xParser = (com.sun.star.util.XURLTransformer) UnoRuntime.queryInterface(
                                              XURLTransformer.class, 
                                              ( (XMultiServiceFactory) Param.getMSF())
                                                   .createInstance("com.sun.star.util.URLTransformer"));
            URL[] aParseURL = new URL[1];
            aParseURL[0] = new URL();
            aParseURL[0].Complete = ".uno:PrintPreview";
            xParser.parseStrict(aParseURL);

            URL aURL = aParseURL[0];
            XDispatch xDispatcher = xDispProv.queryDispatch(aURL, "", 0);

            if (xDispatcher != null) {
                xDispatcher.dispatch(aURL, null);
            }
        } catch (com.sun.star.uno.Exception e) {
            log.println("Couldn't change mode");
            throw new StatusException(Status.failed("Couldn't change mode"));
        }

        shortWait();

        AccessibilityTools at = new AccessibilityTools();

        XWindow xWindow = at.getCurrentWindow( (XMultiServiceFactory) Param.getMSF(), xModel);
        XAccessible xRoot = at.getAccessibleObject(xWindow);

        oObj = at.getAccessibleObjectForRole(xRoot, AccessibleRole.TABLE, "A");

        if (oObj == null) {
            log.println("Version with a fixed #103863#");
            oObj = at.getAccessibleObjectForRole(xRoot, 
                                                 AccessibleRole.TABLE_CELL, 
                                                 true);
        }

        log.println("ImplementationName " + utils.getImplName(oObj));

        TestEnvironment tEnv = new TestEnvironment(oObj);

        tEnv.addObjRelation("EventProducer", 
                            new ifc.accessibility._XAccessibleEventBroadcaster.EventProducer() {
            public void fireEvent() {
            }
        });

        return tEnv;
    }

    /**
    * Sleeps for 0.5 sec. to allow StarOffice to react on <code>
    * reset</code> call.
    */
    private void shortWait() {
        try {
            Thread.currentThread().sleep(500);
        } catch (InterruptedException e) {
            System.out.println("While waiting :" + e);
        }
    }
}