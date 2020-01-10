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

package mod._sw;

import java.io.PrintWriter;

import lib.StatusException;
import lib.TestCase;
import lib.TestEnvironment;
import lib.TestParameters;
import util.SOfficeFactory;

import com.sun.star.beans.XPropertySet;
import com.sun.star.container.XIndexAccess;
import com.sun.star.container.XNameAccess;
import com.sun.star.container.XNameContainer;
import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.style.XStyleFamiliesSupplier;
import com.sun.star.text.ControlCharacter;
import com.sun.star.text.XText;
import com.sun.star.text.XTextCursor;
import com.sun.star.text.XTextDocument;
import com.sun.star.uno.AnyConverter;
import com.sun.star.uno.Type;
import com.sun.star.uno.UnoRuntime;
import com.sun.star.uno.XInterface;

/**
 * Test for object which is represented by service
 * <code>com.sun.star.text.NumberingRules</code>.<p>
 * Object implements the following interfaces :
 * <ul>
 *  <li> <code>com::sun::star::beans::XPropertySet</code></li>
 *  <li> <code>com::sun::star::container::XIndexAccess</code></li>
 *  <li> <code>com::sun::star::container::XElementAccess</code></li>
 *  <li> <code>com::sun::star::text::NumberingRules</code></li>
 * </ul> <p>
 * This object test <b> is NOT </b> designed to be run in several
 * threads concurently.
 * @see com.sun.star.beans.XPropertySet
 * @see com.sun.star.container.XIndexAccess
 * @see com.sun.star.container.XElementAccess
 * @see com.sun.star.text.NumberingRules
 * @see ifc.beans._XPropertySet
 * @see ifc.container._XIndexAccess
 * @see ifc.container._XElementAccess
 * @see ifc.text._NumberingRules
 */
public class SwXNumberingRules extends TestCase {
    XTextDocument xTextDoc;

    /**
    * Creates text document.
    */
    protected void initialize( TestParameters tParam, PrintWriter log ) {
        SOfficeFactory SOF = SOfficeFactory.getFactory( (XMultiServiceFactory)tParam.getMSF() );
        try {
            log.println( "creating a textdocument" );
            xTextDoc = SOF.createTextDoc( null );
        } catch ( com.sun.star.uno.Exception e ) {
            e.printStackTrace( log );
            throw new StatusException( "Couldn't create document", e );
        }
    }

    /**
    * Disposes text document.
    */
    protected void cleanup( TestParameters tParam, PrintWriter log ) {
        log.println( "    disposing xTextDoc " );
        util.DesktopTools.closeDoc(xTextDoc);
    }

    /**
    * Creating a Testenvironment for the interfaces to be tested.
    * At first some strings are inserted to a text document, then style families
    * are gotten from text document using <code>XStyleFamiliesSupplier</code>.
    * Then style family 'NuberingRules' is retrieved, then style indexed '0'
    * is obtained. At the end property value 'NumberingRules' is obtained from
    * given style.
    */
    protected TestEnvironment createTestEnvironment(TestParameters tParam, PrintWriter log) {
        XInterface oObj = null;
        XIndexAccess NumStyleI = null;

        XText oText = xTextDoc.getText();
        XTextCursor oCursor = oText.createTextCursor();

        try {
            for (int i=0; i<5; i++) {
                oText.insertString(oCursor, "The quick brown fox jumps "+
                    "over the lazy Dog", false);
                oText.insertControlCharacter(oCursor,
                    ControlCharacter.PARAGRAPH_BREAK, false);
            }
        } catch ( com.sun.star.lang.IllegalArgumentException e ) {
            log.println("Error, exception occured...");
            e.printStackTrace(log);
        }

        XStyleFamiliesSupplier oStyleFamiliesSupplier = (XStyleFamiliesSupplier)
            UnoRuntime.queryInterface(XStyleFamiliesSupplier.class, xTextDoc);

        try {
            XNameAccess oStyleFamilies = oStyleFamiliesSupplier.getStyleFamilies();
            XNameContainer NumStyles = (XNameContainer) AnyConverter.toObject(
                new Type(XNameContainer.class),
                    oStyleFamilies.getByName("NumberingStyles"));
            NumStyleI = (XIndexAccess)
                UnoRuntime.queryInterface(XIndexAccess.class,NumStyles);
        } catch ( com.sun.star.lang.WrappedTargetException e ) {
            log.println("Error, exception occured...");
            e.printStackTrace(log);
        } catch ( com.sun.star.container.NoSuchElementException e ) {
            log.println("Error, no such style family...");
            e.printStackTrace(log);
        } catch ( com.sun.star.lang.IllegalArgumentException e ) {
            log.println("Error, exception occured...");
            e.printStackTrace(log);
        }
        
        Object instance1 = null;

        try {
            oObj = (XInterface) AnyConverter.toObject(
                    new Type(XInterface.class),NumStyleI.getByIndex(0));
            XPropertySet props = (XPropertySet)
                UnoRuntime.queryInterface(XPropertySet.class, oObj);
            oObj = (XInterface) AnyConverter.toObject(
                new Type(XInterface.class),props.getPropertyValue("NumberingRules"));
            XIndexAccess nRules = (XIndexAccess) UnoRuntime.queryInterface(XIndexAccess.class, props.getPropertyValue("NumberingRules"));
            instance1 = nRules.getByIndex(0);
        } catch ( com.sun.star.lang.WrappedTargetException e ) {
            log.println("Error, exception occured...");
            e.printStackTrace(log);
        } catch ( com.sun.star.lang.IndexOutOfBoundsException e ) {
            log.println("Error, exception occured...");
            e.printStackTrace(log);
        } catch ( com.sun.star.beans.UnknownPropertyException e ) {
            log.println("Error, exception occured...");
            e.printStackTrace(log);
        } catch ( com.sun.star.lang.IllegalArgumentException e ) {
            log.println("Error, exception occured...");
            e.printStackTrace(log);
        }        
        
        TestEnvironment tEnv = new TestEnvironment( oObj );
        
        tEnv.addObjRelation("INSTANCE1", instance1);
        return tEnv;
    }

}    // finish class SwXNumberingRules
