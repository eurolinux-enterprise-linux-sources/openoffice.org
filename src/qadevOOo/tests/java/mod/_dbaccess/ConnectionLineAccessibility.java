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
package mod._dbaccess;

import java.io.PrintWriter;

import lib.Status;
import lib.StatusException;
import lib.TestCase;
import lib.TestEnvironment;
import lib.TestParameters;
import util.AccessibilityTools;

import com.sun.star.accessibility.AccessibleRole;
import com.sun.star.accessibility.XAccessible;
import com.sun.star.awt.PosSize;
import com.sun.star.awt.Rectangle;
import com.sun.star.awt.XExtendedToolkit;
import com.sun.star.awt.XWindow;
import com.sun.star.beans.PropertyValue;
import com.sun.star.beans.XPropertySet;
import com.sun.star.container.XNameAccess;
import com.sun.star.container.XNameContainer;
import com.sun.star.frame.XStorable;
import com.sun.star.lang.XComponent;
import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.sdb.XDocumentDataSource;
import com.sun.star.sdb.XQueryDefinitionsSupplier;
import com.sun.star.sdbc.XConnection;
import com.sun.star.sdbc.XIsolatedConnection;
import com.sun.star.sdbc.XStatement;
import com.sun.star.ucb.XSimpleFileAccess;
import com.sun.star.uno.UnoRuntime;
import com.sun.star.uno.XInterface;
import util.DesktopTools;
import util.utils;


/**
 * Object implements the following interfaces :
 * <ul>
 *   <li><code>::com::sun::star::accessibility::XAccessible</code></li>
 *   <li><code>::com::sun::star::accessibility::XAccessibleContext
 *   </code></li>
 *   <li><code>::com::sun::star::accessibility::XAccessibleEventBroadcaster
 *   </code></li>
 * </ul><p>
 * @see com.sun.star.accessibility.XAccessible
 * @see com.sun.star.accessibility.XAccessibleContext
 * @see com.sun.star.accessibility.XAccessibleEventBroadcaster
 * @see ifc.accessibility._XAccessible
 * @see ifc.accessibility._XAccessibleContext
 * @see ifc.accessibility._XAccessibleEventBroadcaster
 */
public class ConnectionLineAccessibility extends TestCase
{
    XWindow xWindow = null;
    Object oDBSource = null;
    String aFile = "";
    XConnection connection = null;
    XIsolatedConnection isolConnection = null;
    XComponent QueryComponent = null;
    String user = "";
    String password="";
    
    /**
     * Creates a new DataSource and stores it.
     * Creates a connection and using it
     * creates two tables in database.
     * Creates a new query and adds it to DefinitionContainer.
     * Opens the QueryComponent.with loadComponentFromURL
     * and gets the object with the role UNKNOWN and the Impplementation
     * name that contains ConnectionLine
     * @param Param test parameters
     * @param log writer to log information while testing
     * @return
     * @throws StatusException
     * @see TestEnvironment
     */
    protected TestEnvironment createTestEnvironment(TestParameters Param,
            PrintWriter log)
    {
        XInterface oObj = null;
        
        Object oDBContext = null;
        Object oDBSource = null;
        Object newQuery = null;
        Object toolkit = null;
        XStorable store = null;
        
        try
        {
            oDBContext = ((XMultiServiceFactory) Param.getMSF())
            .createInstance("com.sun.star.sdb.DatabaseContext");
            oDBSource = ((XMultiServiceFactory) Param.getMSF())
            .createInstance("com.sun.star.sdb.DataSource");
            newQuery = ((XMultiServiceFactory) Param.getMSF())
            .createInstance("com.sun.star.sdb.QueryDefinition");
            toolkit = ((XMultiServiceFactory) Param.getMSF())
            .createInstance("com.sun.star.awt.Toolkit");
        }
        catch (com.sun.star.uno.Exception e)
        {
            e.printStackTrace(log);
            throw new StatusException(Status.failed("Couldn't create instance"));
        }
        
        String mysqlURL = (String) Param.get("mysql.url");
        
        if (mysqlURL == null)
        {
            throw new StatusException(Status.failed(
                    "Couldn't get 'mysql.url' from ini-file"));
        }
        
        user = (String) Param.get("jdbc.user");
        password = (String) Param.get("jdbc.password");
        
        if ((user == null) || (password == null))
        {
            throw new StatusException(Status.failed(
                    "Couldn't get 'jdbc.user' or 'jdbc.password' from ini-file"));
        }
        
        PropertyValue[] info = new PropertyValue[2];
        info[0] = new PropertyValue();
        info[0].Name = "user";
        info[0].Value = user;
        info[1] = new PropertyValue();
        info[1].Name = "password";
        info[1].Value = password;
        
        XPropertySet propSetDBSource = (XPropertySet) UnoRuntime.queryInterface(
                XPropertySet.class, oDBSource);
        
        try
        {
            propSetDBSource.setPropertyValue("URL", mysqlURL);
            propSetDBSource.setPropertyValue("Info", info);
        }
        catch (com.sun.star.lang.WrappedTargetException e)
        {
            e.printStackTrace(log);
            throw new StatusException(Status.failed(
                    "Couldn't set property value"));
        }
        catch (com.sun.star.lang.IllegalArgumentException e)
        {
            e.printStackTrace(log);
            throw new StatusException(Status.failed(
                    "Couldn't set property value"));
        }
        catch (com.sun.star.beans.PropertyVetoException e)
        {
            e.printStackTrace(log);
            throw new StatusException(Status.failed(
                    "Couldn't set property value"));
        }
        catch (com.sun.star.beans.UnknownPropertyException e)
        {
            e.printStackTrace(log);
            throw new StatusException(Status.failed(
                    "Couldn't set property value"));
        }
        
        try
        {
            log.println("writing database file ...");
            XDocumentDataSource xDDS = (XDocumentDataSource)
            UnoRuntime.queryInterface(XDocumentDataSource.class, oDBSource);
            store = (XStorable) UnoRuntime.queryInterface(XStorable.class,
                    xDDS.getDatabaseDocument());
            
            aFile = utils.getOfficeTemp((XMultiServiceFactory) Param.getMSF())+"ConnectionLine.odb";
            log.println("... filename will be "+aFile);
            store.storeAsURL(aFile,new PropertyValue[]
            {});
            log.println("... done");
        }
        catch (com.sun.star.uno.Exception e)
        {
            e.printStackTrace(log);
            throw new StatusException(Status.failed("Couldn't register object"));
        }
        
        isolConnection = (XIsolatedConnection) UnoRuntime.queryInterface(
                XIsolatedConnection.class,
                oDBSource);
        
        XConnection connection = null;
        XStatement statement = null;
        
        final String tbl_name1 = "tst_table1";
        final String tbl_name2 = "tst_table2";
        final String col_name1 = "id1";
        final String col_name2 = "id2";
        
        try
        {
            connection = isolConnection.getIsolatedConnection(user, password);
            statement = connection.createStatement();
            statement.executeUpdate("drop table if exists " + tbl_name1);
            statement.executeUpdate("drop table if exists " + tbl_name2);
            statement.executeUpdate("create table " + tbl_name1 + " (" +
                    col_name1 + " int)");
            statement.executeUpdate("create table " + tbl_name2 + " (" +
                    col_name2 + " int)");
        }
        catch (com.sun.star.sdbc.SQLException e)
        {
            try
            {
                shortWait();
                connection = isolConnection.getIsolatedConnection(user,
                        password);
                statement = connection.createStatement();
                statement.executeUpdate("drop table if exists " + tbl_name1);
                statement.executeUpdate("drop table if exists " + tbl_name2);
                statement.executeUpdate("create table " + tbl_name1 + " (" +
                        col_name1 + " int)");
                statement.executeUpdate("create table " + tbl_name2 + " (" +
                        col_name2 + " int)");
            }
            catch (com.sun.star.sdbc.SQLException e2)
            {
                e2.printStackTrace(log);
                throw new StatusException(Status.failed("SQLException"));
            }
        }
        
        XQueryDefinitionsSupplier querySuppl = (XQueryDefinitionsSupplier) UnoRuntime.queryInterface(
                XQueryDefinitionsSupplier.class,
                oDBSource);
        
        XNameAccess defContainer = querySuppl.getQueryDefinitions();
        
        XPropertySet queryProp = (XPropertySet) UnoRuntime.queryInterface(
                XPropertySet.class, newQuery);
        
        try
        {
            final String query = "select * from " + tbl_name1 + ", " +
                    tbl_name2 + " where " + tbl_name1 + "." +
                    col_name1 + "=" + tbl_name2 + "." +
                    col_name2;
            queryProp.setPropertyValue("Command", query);
        }
        catch (com.sun.star.lang.WrappedTargetException e)
        {
            e.printStackTrace(log);
            throw new StatusException(Status.failed(
                    "Couldn't set property value"));
        }
        catch (com.sun.star.lang.IllegalArgumentException e)
        {
            e.printStackTrace(log);
            throw new StatusException(Status.failed(
                    "Couldn't set property value"));
        }
        catch (com.sun.star.beans.PropertyVetoException e)
        {
            e.printStackTrace(log);
            throw new StatusException(Status.failed(
                    "Couldn't set property value"));
        }
        catch (com.sun.star.beans.UnknownPropertyException e)
        {
            e.printStackTrace(log);
            throw new StatusException(Status.failed(
                    "Couldn't set property value"));
        }
        
        XNameContainer queryContainer = (XNameContainer) UnoRuntime.queryInterface(
                XNameContainer.class,
                defContainer);
        
        try
        {
            queryContainer.insertByName("Query1", newQuery);
            store.store();
            connection.close();
        }
        catch (com.sun.star.lang.WrappedTargetException e)
        {
            e.printStackTrace(log);
            throw new StatusException(Status.failed("Couldn't insert query"));
        }
        catch (com.sun.star.container.ElementExistException e)
        {
            e.printStackTrace(log);
            throw new StatusException(Status.failed("Couldn't insert query"));
        }
        catch (com.sun.star.lang.IllegalArgumentException e)
        {
            e.printStackTrace(log);
            throw new StatusException(Status.failed("Couldn't insert query"));
        }
        catch (com.sun.star.io.IOException e)
        {
            e.printStackTrace(log);
            throw new StatusException(Status.failed("Couldn't insert query"));
        }
        catch (com.sun.star.sdbc.SQLException e)
        {
            e.printStackTrace(log);
            throw new StatusException(Status.failed("Couldn't insert query"));
        }
        
        PropertyValue[] loadProps = new PropertyValue[3];
        loadProps[0] = new PropertyValue();
        loadProps[0].Name = "QueryDesignView";
        loadProps[0].Value = Boolean.TRUE;
        
        loadProps[1] = new PropertyValue();
        loadProps[1].Name = "CurrentQuery";
        loadProps[1].Value = "Query1";
        
        loadProps[2] = new PropertyValue();
        loadProps[2].Name = "DataSource";
        loadProps[2].Value = oDBSource;
        
        QueryComponent = DesktopTools.loadDoc((XMultiServiceFactory) Param.getMSF(),".component:DB/QueryDesign",loadProps);
        
        util.utils.shortWait(1000);
        
        XExtendedToolkit tk = (XExtendedToolkit) UnoRuntime.queryInterface(
                XExtendedToolkit.class, toolkit);
        
        Object atw = tk.getActiveTopWindow();
        
        xWindow = (XWindow) UnoRuntime.queryInterface(XWindow.class, atw);
        
        XAccessible xRoot = AccessibilityTools.getAccessibleObject(xWindow);
        
        AccessibilityTools.printAccessibleTree (log,xRoot, Param.getBool(util.PropertyName.DEBUG_IS_ACTIVE));
        
        oObj = AccessibilityTools.getAccessibleObjectForRoleIgnoreShowing(xRoot, AccessibleRole.UNKNOWN, "", "ConnectionLine");
        
        log.println("ImplementationName " + util.utils.getImplName(oObj));
        
        log.println("creating TestEnvironment");
        
        TestEnvironment tEnv = new TestEnvironment(oObj);
        
        shortWait();
        
        final XWindow queryWin = xWindow;
        
        tEnv.addObjRelation("EventProducer",
                new ifc.accessibility._XAccessibleEventBroadcaster.EventProducer()
        {
            public void fireEvent()
            {
                Rectangle rect = queryWin.getPosSize();
                queryWin.setPosSize(rect.X, rect.Y, rect.Height-5, rect.Width-5, PosSize.POSSIZE);
            }
        });
        
        return tEnv;
    } // finish method getTestEnvironment
    
    /**
     * Closes the DatasourceAdministration dialog and Query Dialog.
     */
    protected void cleanup(TestParameters Param, PrintWriter log)
    {
        try
        {
            
            log.println("closing QueryComponent ...");
            DesktopTools.closeDoc(QueryComponent);
            log.println("... done");
            XMultiServiceFactory xMSF = (XMultiServiceFactory)Param.getMSF();
            Object sfa = xMSF.createInstance("com.sun.star.comp.ucb.SimpleFileAccess");
            XSimpleFileAccess xSFA = (XSimpleFileAccess) UnoRuntime.queryInterface(XSimpleFileAccess.class, sfa);
            log.println("deleting database file");
            xSFA.kill(aFile);
            log.println("Could delete file "+aFile+": "+!xSFA.exists(aFile));
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }
    
    /**
     * Sleeps for 1.5 sec. to allow StarOffice to react on <code>
     * reset</code> call.
     */
    private void shortWait()
    {
        try
        {
            Thread.sleep(1500);
        }
        catch (InterruptedException e)
        {
            log.println("While waiting :" + e);
        }
    }
}
