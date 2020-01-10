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
import java.util.Vector;

import lib.Status;
import lib.StatusException;
import lib.TestCase;
import lib.TestEnvironment;
import lib.TestParameters;
import util.DBTools;
import util.utils;

import com.sun.star.beans.PropertyValue;
import com.sun.star.beans.XPropertySet;
import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.sdb.CommandType;
import com.sun.star.sdb.ParametersRequest;
import com.sun.star.sdb.RowChangeEvent;
import com.sun.star.sdb.XInteractionSupplyParameters;
import com.sun.star.sdbc.SQLException;
import com.sun.star.sdbc.XConnection;
import com.sun.star.sdbc.XResultSet;
import com.sun.star.sdbc.XResultSetUpdate;
import com.sun.star.sdbc.XRow;
import com.sun.star.sdbc.XRowSet;
import com.sun.star.sdbc.XRowUpdate;
import com.sun.star.task.XInteractionAbort;
import com.sun.star.task.XInteractionContinuation;
import com.sun.star.task.XInteractionRequest;
import com.sun.star.ucb.AuthenticationRequest;
import com.sun.star.ucb.XSimpleFileAccess;
import com.sun.star.uno.AnyConverter;
import com.sun.star.uno.Type;
import com.sun.star.uno.UnoRuntime;
import com.sun.star.uno.XInterface;
import ifc.sdb._XCompletedExecution;

/**
 * Test for object which is represented by service
 * <code>com.sun.star.sdb.DataSource</code>. <p>
 *
 * Object implements the following interfaces :
 * <ul>
 *  <li> <code>com::sun::star::sdbc::RowSet</code></li>
 *  <li> <code>com::sun::star::sdbcx::XRowLocate</code></li>
 *  <li> <code>com::sun::star::sdbc::XResultSetUpdate</code></li>
 *  <li> <code>com::sun::star::util::XCancellable</code></li>
 *  <li> <code>com::sun::star::sdbc::XParameters</code></li>
 *  <li> <code>com::sun::star::sdbc::XResultSetMetaDataSupplier</code></li>
 *  <li> <code>com::sun::star::sdbcx::XDeleteRows</code></li>
 *  <li> <code>com::sun::star::sdbc::XCloseable</code></li>
 *  <li> <code>com::sun::star::sdbcx::XColumnsSupplier</code></li>
 *  <li> <code>com::sun::star::sdb::XResultSetAccess</code></li>
 *  <li> <code>com::sun::star::sdbc::XResultSet</code></li>
 *  <li> <code>com::sun::star::sdbc::XColumnLocate</code></li>
 *  <li> <code>com::sun::star::sdbc::XRowSet</code></li>
 *  <li> <code>com::sun::star::sdb::RowSet</code></li>
 *  <li> <code>com::sun::star::sdbc::XRowUpdate</code></li>
 *  <li> <code>com::sun::star::sdb::XRowSetApproveBroadcaster</code></li>
 *  <li> <code>com::sun::star::beans::XPropertySet</code></li>
 *  <li> <code>com::sun::star::sdbc::XRow</code></li>
 *  <li> <code>com::sun::star::sdbc::XWarningsSupplier</code></li>
 *  <li> <code>com::sun::star::lang::XComponent</code></li>
 *  <li> <code>com::sun::star::sdbcx::ResultSet</code></li>
 *  <li> <code>com::sun::star::sdbc::ResultSet</code></li>
 * </ul> <p>
 * The following files used by this test :
 * <ul>
 *  <li><b> TestDB/TestDB.dbf </b> : the database file with some
 *    predefined fields described in <code>util.DBTools</code>.
 *    The copy of this file is always made in temp directory for
 *    testing purposes.</li>
 * </ul> <p>
 * The following parameters in ini-file used by this test:
 * <ul>
 *   <li><code>test.db.url</code> - URL to MySQL database.
 *   For example: <code>mysql://mercury:3306/api_current</code></li>
 *   <li><code>test.db.user</code> - user for MySQL database</li>
 *   <li><code>test.db.password</code> - password for MySQL database</li>
 * </ul><p>
 *
 * @see com.sun.star.sdbc.RowSet
 * @see com.sun.star.sdbcx.XRowLocate
 * @see com.sun.star.sdbc.XResultSetUpdate
 * @see com.sun.star.util.XCancellable
 * @see com.sun.star.sdbc.XParameters
 * @see com.sun.star.sdbc.XResultSetMetaDataSupplier
 * @see com.sun.star.sdbcx.XDeleteRows
 * @see com.sun.star.sdbc.XCloseable
 * @see com.sun.star.sdbcx.XColumnsSupplier
 * @see com.sun.star.sdb.XResultSetAccess
 * @see com.sun.star.sdbc.XResultSet
 * @see com.sun.star.sdbc.XColumnLocate
 * @see com.sun.star.sdbc.XRowSet
 * @see com.sun.star.sdb.RowSet
 * @see com.sun.star.sdbc.XRowUpdate
 * @see com.sun.star.sdb.XRowSetApproveBroadcaster
 * @see com.sun.star.beans.XPropertySet
 * @see com.sun.star.sdbc.XRow
 * @see com.sun.star.sdbc.XWarningsSupplier
 * @see com.sun.star.lang.XComponent
 * @see com.sun.star.sdbcx.ResultSet
 * @see com.sun.star.sdbc.ResultSet
 * @see ifc.sdbc._RowSet
 * @see ifc.sdbcx._XRowLocate
 * @see ifc.sdbc._XResultSetUpdate
 * @see ifc.util._XCancellable
 * @see ifc.sdbc._XParameters
 * @see ifc.sdbc._XResultSetMetaDataSupplier
 * @see ifc.sdbcx._XDeleteRows
 * @see ifc.sdbc._XCloseable
 * @see ifc.sdbcx._XColumnsSupplier
 * @see ifc.sdb._XResultSetAccess
 * @see ifc.sdbc._XResultSet
 * @see ifc.sdbc._XColumnLocate
 * @see ifc.sdbc._XRowSet
 * @see ifc.sdb._RowSet
 * @see ifc.sdbc._XRowUpdate
 * @see ifc.sdb._XRowSetApproveBroadcaster
 * @see ifc.beans._XPropertySet
 * @see ifc.sdbc._XRow
 * @see ifc.sdbc._XWarningsSupplier
 * @see ifc.lang._XComponent
 * @see ifc.sdbcx._ResultSet
 * @see ifc.sdbc._ResultSet
 */
public class ORowSet extends TestCase {

    private static int uniqueSuffix = 0 ;
    private DBTools dbTools     = null ;
    private static String origDB = null ;
    private PrintWriter log = null ;
    private static String tmpDir = null ;
    String tableName = null;
    DBTools.DataSourceInfo srcInf = null;
    boolean isMySQLDB = false;
    protected final static String dbSourceName = "ORowSetDataSource";
    public XConnection conn = null;
    Object dbSrc = null;
    String aFile = "";

    /**
    * Initializes some class fields. Then creates DataSource, which serves
    * as a single source for all tables created in the test.
    * This DataSource then registered in the global
    * <code>DatabaseContext</code> service. This data source's URL
    * points to SOffice temp directory where tables are copied from
    * <code>TestDocuments</code> directory on every environment
    * creation.
    * To create DataSource for MySQL database next parameters required
    * in ini-file:
    * <ul>
    *   <li><code>test.db.url</code> - URL to MySQL database.
    *   For example: <code>mysql://mercury:3306/api_current</code></li>
    *   <li><code>test.db.user</code> - user for MySQL database</li>
    *   <li><code>test.db.password</code> - password for MySQL database</li>
    * </ul>
    *
    * @throws StatusException if DataSource can not be created or
    * registered.
    */
    protected void initialize ( TestParameters Param, PrintWriter log)
        throws StatusException {

        this.log = log ;
        tmpDir = (String) Param.get("TMPURL") ;
            tmpDir = utils.getOfficeTemp((XMultiServiceFactory)Param.getMSF());

        origDB = util.utils.getFullTestDocName("TestDB/testDB.dbf");

        dbTools = new DBTools((XMultiServiceFactory)Param.getMSF()) ;

        // creating DataSource and registering it in DatabaseContext
        String dbURL = (String) Param.get("test.db.url");
        String dbUser = (String) Param.get("test.db.user");
        String dbPassword = (String) Param.get("test.db.password");

        log.println("Creating and registering DataSource ...");
        srcInf = dbTools.newDataSourceInfo();
        if (dbURL != null && dbUser != null && dbPassword != null) {
            isMySQLDB = true;
            log.println("dbURL = " + dbURL);
            log.println("dbUSER = " + dbUser);
            log.println("dbPASSWORD = " + dbPassword);
            //DataSource for mysql db
            try {
                tableName = "soffice_test_table";
                srcInf.URL = "jdbc:" + dbURL;
                srcInf.IsPasswordRequired = new Boolean(true);
                srcInf.Password = dbPassword;
                srcInf.User = dbUser;
                PropertyValue[] propInfo = new PropertyValue[1];
                propInfo[0] = new PropertyValue();
                propInfo[0].Name = "JavaDriverClass";
                propInfo[0].Value = "org.gjt.mm.mysql.Driver";
                srcInf.Info = propInfo;
                dbSrc = srcInf.getDataSourceService() ;
                if (uniqueSuffix < 1)
                    dbTools.reRegisterDB(dbSourceName, dbSrc);
                XMultiServiceFactory xMSF = (XMultiServiceFactory)Param.getMSF ();
                    aFile = utils.getOfficeTemp (xMSF)+dbSourceName+".odb";
            } catch (com.sun.star.uno.Exception e) {
                log.println("Error while object test initialization :") ;
                e.printStackTrace(log) ;
                throw new StatusException("Error while object test" +
                    " initialization", e);
            }
        } else {
            try {
                srcInf.URL = "sdbc:dbase:" + DBTools.dirToUrl(tmpDir) ;
                dbSrc = srcInf.getDataSourceService() ;
                if (uniqueSuffix < 1)
                    dbTools.reRegisterDB(dbSourceName, dbSrc) ;
            } catch (com.sun.star.uno.Exception e) {
                log.println("Error while object test initialization :") ;
                e.printStackTrace(log) ;
                throw new
                    StatusException("Error while object test initialization",e) ;
            }
        }
    }

    /**
    * Creating a Testenvironment for the interfaces to be tested.
    * The database (DBF) file is copied from test document directory
    * into SOffice temp dir with unique name for each enviroment
    * creation. If the file cann't be copied (is not released)
    * then another unique name is used (file name suffix incremented
    * by 1).<p>
    *
    * <code>com.sun.star.sdb.RowSet</code> service created and its
    * source is all rows from the current copy of the table. Then
    * row set command ("select all rows from a table") is executed
    * and cursor is positioned to the first row. <p>
    *
    *     Object relations created :
    * <ul>
    *  <li> <code>'ORowSet.Connection'</code> for
    *      internal component test usage. Is used for
    *      closing connection when cleaning up environment. </li>
    *  <li> <code>'XRowSetApproveBroadcaster.ApproveChecker'</code> for
    *      {@link ifc.sdb._XRowSetApproveBroadcaster} interface
    *      implementation which made actions required </li>
    *  <li> <code>'CurrentRowData'</code> for
    *      {@link ifc.sdbc._XRow}, {@link ifc.sdbc._XRowUpdate} :
    *      exports types and values of the current row data.</li>
    *  <li> <code>'XColumnLocate.ColumnName'</code> for
    *      {@link ifc.sdbc._XColumnLocate} :
    *      the name of the first column of the table.</li>
    *  <li> <code>'XParameters.ParamValues'</code> for
    *      {@link ifc.sdbc._XParameters} :
    *      Collection of parameter types presented in the query. </li>
    *  <li> <code>'XRowUpdate.XRow'</code> for
    *      {@link ifc.sdbc._XRowUpdate} :
    *      <code>XRow</code> interface of the current component.</li>
    *  <li> <code>'XResultSetUpdate.UpdateTester'</code> for
    *      {@link ifc.sdbc._XResultSetUpdate} </li>
    * </ul>
    *
    * @see com.sun.star.sdb.DatabaseContext
    * @see com.sun.star.sdb.DataSource
    */
    protected TestEnvironment createTestEnvironment(TestParameters Param,
            PrintWriter log) {

        XInterface oObj = null;
        Object oInterface = null;
        XMultiServiceFactory xMSF = null ;
        uniqueSuffix++;
        boolean envCreatedOK = false ;
        
        //initialize test table
        if (isMySQLDB) {
            try {
                dbTools.initTestTableUsingJDBC(tableName, srcInf);
            } catch(java.sql.SQLException e) {
                e.printStackTrace(log);
                throw new StatusException(Status.failed("Couldn't " +
                    " init test table. SQLException..."));
            } catch(java.lang.ClassNotFoundException e) {
                throw new StatusException(Status.failed("Couldn't " +
                    "register mysql driver"));
            }
        } else {
            String oldF = null ;
            String newF = null ;
            do {
                tableName = "ORowSet_tmp" + uniqueSuffix ;
                oldF = utils.getFullURL(origDB);
                newF = utils.getOfficeTemp((XMultiServiceFactory)Param.getMSF())+tableName+".dbf";
            } while (!utils.overwriteFile((XMultiServiceFactory)Param.getMSF(),oldF,newF) &&
                uniqueSuffix++ < 50);
        }
        
        XConnection connection = null ;

        try {
            xMSF = (XMultiServiceFactory)Param.getMSF();

            Object oRowSet = xMSF.createInstance("com.sun.star.sdb.RowSet") ;

            XPropertySet xSetProp = (XPropertySet) UnoRuntime.queryInterface
                (XPropertySet.class, oRowSet) ;

            log.println("Trying to open: " + tableName);

            xSetProp.setPropertyValue("DataSourceName", dbSourceName) ;
            xSetProp.setPropertyValue("Command", tableName) ;
            xSetProp.setPropertyValue("CommandType",
                new Integer(CommandType.TABLE)) ;

            com.sun.star.sdbc.XRowSet xORowSet = (com.sun.star.sdbc.XRowSet)
                UnoRuntime.queryInterface(com.sun.star.sdbc.XRowSet.class,
                oRowSet) ;                   

            xORowSet.execute() ;

            connection = null;

            try {
                connection = (XConnection) AnyConverter.toObject(
                                    new Type(XConnection.class),
                                    xSetProp.getPropertyValue("ActiveConnection"));
            } catch (com.sun.star.lang.IllegalArgumentException iae) {
                throw new StatusException("couldn't convert Any",iae);
            }

            oInterface = oRowSet ;

            XResultSet xRes = (XResultSet) UnoRuntime.queryInterface
                (XResultSet.class, oRowSet) ;

            xRes.first() ;

            if (oInterface == null) {
                log.println("Service wasn't created") ;
                throw new StatusException("Service wasn't created",
                    new NullPointerException()) ;
            }

            oObj = (XInterface) oInterface;

            log.println( "    creating a new environment for object" );
            TestEnvironment tEnv = new TestEnvironment( oObj );

            // Adding relations for disposing object
            tEnv.addObjRelation("ORowSet.Connection", connection) ;
            this.conn = (XConnection) tEnv.getObjRelation("ORowSet.Connection");


            // Adding obj relation for XRowSetApproveBroadcaster test
            {
                final XResultSet xResSet = (XResultSet)
                    UnoRuntime.queryInterface(XResultSet.class, oObj) ;
                final XResultSetUpdate xResSetUpdate = (XResultSetUpdate)
                    UnoRuntime.queryInterface(XResultSetUpdate.class, oObj) ;
                final XRowSet xRowSet = (XRowSet) UnoRuntime.queryInterface
                    (XRowSet.class, oObj) ;
                final XRowUpdate xRowUpdate = (XRowUpdate)
                    UnoRuntime.queryInterface(XRowUpdate.class, oObj) ;
                final PrintWriter logF = log ;
                tEnv.addObjRelation("XRowSetApproveBroadcaster.ApproveChecker",
                    new ifc.sdb._XRowSetApproveBroadcaster.RowSetApproveChecker() {
                        public void moveCursor() {
                            try {
                                xResSet.beforeFirst() ;
                                xResSet.afterLast() ;
                                xResSet.first() ;
                            } catch (com.sun.star.sdbc.SQLException e) {
                                logF.println("### _XRowSetApproveBroadcaster." +
                                    "RowSetApproveChecker.moveCursor() :") ;
                                e.printStackTrace(logF) ;
                            }
                        }
                        public RowChangeEvent changeRow() {
                            try {
                                xResSet.first() ;
                                xRowUpdate.updateString(1, "ORowSetTest2") ;
                                xResSetUpdate.updateRow() ;
                            } catch (com.sun.star.sdbc.SQLException e) {
                                logF.println("### _XRowSetApproveBroadcaster." +
                                    "RowSetApproveChecker.changeRow() :") ;
                                e.printStackTrace(logF) ;
                            }
                            RowChangeEvent ev = new RowChangeEvent() ;
                            ev.Action = com.sun.star.sdb.RowChangeAction.UPDATE ;
                            ev.Rows = 1 ;

                            return ev ;
                        }
                        public void changeRowSet() {
                            try {
                                xRowSet.execute() ;
                                xResSet.first() ;
                            } catch (com.sun.star.sdbc.SQLException e) {
                                logF.println("### _XRowSetApproveBroadcaster."+
                                    "RowSetApproveChecker.changeRowSet() :") ;
                                e.printStackTrace(logF) ;
                            }
                        }
                    }) ;
            }
            // Adding relations for XRow as a Vector with all data
            // of current row of RowSet.

            Vector rowData = new Vector() ;

            for (int i = 0; i < DBTools.TST_TABLE_VALUES[0].length; i++) {
                rowData.add(DBTools.TST_TABLE_VALUES[0][i]) ;
            }

            // here XRef must be added
            // here XBlob must be added
            // here XClob must be added
            // here XArray must be added

            tEnv.addObjRelation("CurrentRowData", rowData) ;

            // Adding relation for XColumnLocate ifc test
            tEnv.addObjRelation("XColumnLocate.ColumnName",
                DBTools.TST_STRING_F) ;

            // Adding relation for XCompletedExecution
            tEnv.addObjRelation("InteractionHandlerChecker", new InteractionHandlerImpl());
            XPropertySet xProp = (XPropertySet) UnoRuntime.queryInterface(XPropertySet.class, oObj) ;
            try {
                xProp.setPropertyValue("DataSourceName", dbSourceName) ;
                if(isMySQLDB) {
                    xProp.setPropertyValue("Command", "SELECT Column0  FROM soffice_test_table  WHERE ( (  Column0 = :param1 ) )");
                }
                else {
                    xProp.setPropertyValue("Command", "SELECT \"_TEXT\" FROM \"ORowSet_tmp" + uniqueSuffix + "\" WHERE ( ( \"_TEXT\" = :param1 ) )");
                }

                xProp.setPropertyValue("CommandType", new Integer(CommandType.COMMAND)) ;
            }
            catch(Exception e) {
            }

            
            
            
            
            // Adding relation for XParameters ifc test
            Vector params = new Vector() ;


            tEnv.addObjRelation("XParameters.ParamValues", params) ;

            // Adding relation for XRowUpdate
            XRow row = (XRow) UnoRuntime.queryInterface (XRow.class, oObj) ;
            tEnv.addObjRelation("XRowUpdate.XRow", row) ;

            // Adding relation for XResultSetUpdate
            {
                final XResultSet xResSet = (XResultSet)
                    UnoRuntime.queryInterface(XResultSet.class, oObj) ;
                final XRowUpdate xRowUpdate = (XRowUpdate)
                    UnoRuntime.queryInterface(XRowUpdate.class, oObj) ;
                final XRow xRow = (XRow) UnoRuntime.queryInterface
                    (XRow.class, oObj) ;

                tEnv.addObjRelation("XResultSetUpdate.UpdateTester",
                    new ifc.sdbc._XResultSetUpdate.UpdateTester() {
                        String lastUpdate = null ;

                        public int rowCount() throws SQLException {
                            int prevPos = xResSet.getRow() ;
                            xResSet.last() ;
                            int count = xResSet.getRow() ;
                            xResSet.absolute(prevPos) ;

                            return count ;
                        }

                        public void update() throws SQLException {
                            lastUpdate = xRow.getString(1) ;
                            lastUpdate += "_" ;
                            xRowUpdate.updateString(1, lastUpdate) ;
                        }

                        public boolean wasUpdated() throws SQLException {
                            String getStr = xRow.getString(1) ;
                            return lastUpdate.equals(getStr) ;
                        }

                        public int currentRow() throws SQLException {
                            return xResSet.getRow() ;
                        }
                    }) ;
            }

            envCreatedOK = true ;
            return tEnv;

        } catch(com.sun.star.uno.Exception e) {
            log.println("Can't create object" );
            e.printStackTrace(log) ;
            try {
                connection.close() ;
            } catch(Exception ex) {}
            throw new StatusException("Can't create object", e) ;
        } finally {
            if (!envCreatedOK) {
                try {
                    connection.close() ;
                } catch(Exception ex) {}
            }
        }

    } // finish method getTestEnvironment

    /**
    * Closes connection of <code>RowSet</code> instance created.
    */
    protected void cleanup( TestParameters Param, PrintWriter log) {
        try {
            conn.close() ;
            XMultiServiceFactory xMSF = (XMultiServiceFactory)Param.getMSF ();
            Object sfa = xMSF.createInstance ("com.sun.star.comp.ucb.SimpleFileAccess");
            XSimpleFileAccess xSFA = (XSimpleFileAccess) UnoRuntime.queryInterface (XSimpleFileAccess.class, sfa);
            log.println ("deleting database file");
            xSFA.kill (aFile);
            log.println ("Could delete file "+aFile+": "+!xSFA.exists (aFile));            
        } catch (com.sun.star.uno.Exception e) {
            log.println("Can't close the connection") ;
            e.printStackTrace(log) ;
        } catch (com.sun.star.lang.DisposedException e) {
            log.println("Connection was already closed. It's OK.") ;
        }
//        try {
//            dbTools.revokeDB(dbSourceName) ;
//            XComponent db = (XComponent) UnoRuntime.queryInterface(XComponent.class,dbSrc);
//            db.dispose();
//        } catch (com.sun.star.uno.Exception e) {
//            log.println("Error while object test cleaning up :") ;
//            e.printStackTrace(log) ;
//            throw new StatusException("Error while object test cleaning up",e) ;
//        }
    }

    /**
     * Implementation of interface _XCompletedExecution.CheckInteractionHandler
     * for the XCompletedExecution test
     * @see ifc.sdb._XCompletedExecution
     */
    public class InteractionHandlerImpl implements _XCompletedExecution.CheckInteractionHandler {
        private boolean handlerWasUsed = false;
        private PrintWriter log = new PrintWriter(System.out);
        
        public boolean checkInteractionHandler() {
            return handlerWasUsed;
        }
        
        public void handle(XInteractionRequest xInteractionRequest) {
            log.println("### _XCompletedExecution.InteractionHandlerImpl: handle called.");
            ParametersRequest req = null;
            boolean abort = false;
            
            Object o = xInteractionRequest.getRequest();
            if (o instanceof ParametersRequest) {
                req = (ParametersRequest)o;
            }
            else if (o instanceof AuthenticationRequest) {
                log.println("### The request in XCompletedExecution is of type 'AuthenticationRequest'");
                log.println("### This is not implemented in ORowSet.InteractionHandlerImpl test -> abort.");
                abort = true;
            }
            else {
                log.println("### Unknown request:" + o.toString());
                log.println("### This is not implemented in ORowSet.InteractionHandlerImpl test -> abort.");
                abort = true;
            }
                
            XInteractionContinuation[]xCont = xInteractionRequest.getContinuations();
            XInteractionSupplyParameters xParamCallback = null;
            for(int i=0; i<xCont.length; i++) {
                if (abort) {
                    XInteractionAbort xAbort = null;
                    xAbort = (XInteractionAbort)UnoRuntime.queryInterface(XInteractionAbort.class, xCont[i]);
                    if (xAbort != null)
                        xAbort.select();
                        return;
                }
                else {
                    xParamCallback = (XInteractionSupplyParameters)
                                    UnoRuntime.queryInterface(XInteractionSupplyParameters.class, xCont[i]);
                    if (xParamCallback != null)
                        break;
                }
            }
            if (xParamCallback != null) {
                log.println("### _XCompletedExecution.InteractionHandlerImpl: supplying parameters.");
                handlerWasUsed = true;
                PropertyValue[] prop = new PropertyValue[1];
                prop[0] = new PropertyValue();
                prop[0].Name = "param1";
                prop[0].Value = "Hi.";

                xParamCallback.setParameters(prop);
                xParamCallback.select();
            }
            else { // we should never reach this: abort has to be true first.
                log.println("### _XCompletedExecution.InteractionHandlerImpl: Got no " + 
                            "'XInteractionSupplyParameters' and no 'XInteractionAbort'.");
            }
        }
        
        public void setLog(PrintWriter log) {
            this.log = log;
        }
        
    }
}
