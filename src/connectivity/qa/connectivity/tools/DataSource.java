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
package connectivity.tools;

import com.sun.star.container.ElementExistException;
import com.sun.star.container.NoSuchElementException;
import com.sun.star.container.XNameAccess;
import com.sun.star.container.XNameContainer;
import com.sun.star.lang.WrappedTargetException;
import com.sun.star.lang.XSingleServiceFactory;
import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.beans.XPropertySet;
import com.sun.star.sdb.XQueryDefinitionsSupplier;
import com.sun.star.sdbc.XDataSource;
import com.sun.star.sdbcx.XTablesSupplier;
import com.sun.star.uno.Exception;
import com.sun.star.uno.UnoRuntime;
import com.sun.star.util.XRefreshable;
import java.util.logging.Level;
import java.util.logging.Logger;

public class DataSource
{
    // the service factory

    private final XMultiServiceFactory m_orb;
    private XDataSource m_dataSource;

    public DataSource(final XMultiServiceFactory _orb, final String _registeredName) throws Exception
    {
        m_orb = _orb;

        final XNameAccess dbContext = (XNameAccess) UnoRuntime.queryInterface(XNameAccess.class,
                _orb.createInstance("com.sun.star.sdb.DatabaseContext"));

        m_dataSource = (XDataSource) UnoRuntime.queryInterface(XDataSource.class,
                dbContext.getByName(_registeredName));
    }

    public DataSource(final XMultiServiceFactory _orb,final XDataSource _dataSource)
    {
        m_orb = _orb;
        m_dataSource = _dataSource;
    }

    final public XDataSource getXDataSource()
    {
        return m_dataSource;
    }

    /** creates a query with a given name and SQL command
     */
    public void createQuery(final String _name, final String _sqlCommand) throws ElementExistException, WrappedTargetException, com.sun.star.lang.IllegalArgumentException
    {
        createQuery(_name, _sqlCommand, true);
    }

    /** creates a query with a given name, SQL command, and EscapeProcessing flag
     */
    public void createQuery(final String _name, final String _sqlCommand, final boolean _escapeProcessing) throws ElementExistException, WrappedTargetException, com.sun.star.lang.IllegalArgumentException
    {
        final XSingleServiceFactory queryDefsFac = (XSingleServiceFactory) UnoRuntime.queryInterface(
                XSingleServiceFactory.class, getQueryDefinitions());
        XPropertySet queryDef = null;
        try
        {
            queryDef = (XPropertySet) UnoRuntime.queryInterface(
                    XPropertySet.class, queryDefsFac.createInstance());
            queryDef.setPropertyValue("Command", _sqlCommand);
            queryDef.setPropertyValue("EscapeProcessing", Boolean.valueOf(_escapeProcessing));
        }
        catch (com.sun.star.uno.Exception e)
        {
            e.printStackTrace(System.err);
        }

        final XNameContainer queryDefsContainer = (XNameContainer) UnoRuntime.queryInterface(
                XNameContainer.class, getQueryDefinitions());
        queryDefsContainer.insertByName(_name, queryDef);
    }

    /** provides the query definition with the given name
     */
    public QueryDefinition getQueryDefinition(final String _name) throws NoSuchElementException
    {
        final XNameAccess allDefs = getQueryDefinitions();
        try
        {
            return new QueryDefinition(
                    (XPropertySet) UnoRuntime.queryInterface(XPropertySet.class, allDefs.getByName(_name)));
        }
        catch (WrappedTargetException e)
        {
        }
        throw new NoSuchElementException();
    }

    /** provides the container of query definitions of the data source
     */
    public XNameAccess getQueryDefinitions()
    {
        final XQueryDefinitionsSupplier suppQueries = (XQueryDefinitionsSupplier) UnoRuntime.queryInterface(
                XQueryDefinitionsSupplier.class, m_dataSource);
        return suppQueries.getQueryDefinitions();
    }

    /** refreshs the table container of a given connection
     *
     *  This is usually necessary if you created tables by directly executing SQL statements,
     *  bypassing the SDBCX layer.
     */
    public void refreshTables(final com.sun.star.sdbc.XConnection _connection)
    {
        final XTablesSupplier suppTables = (XTablesSupplier) UnoRuntime.queryInterface(
                XTablesSupplier.class, _connection);
        final XRefreshable refreshTables = (XRefreshable) UnoRuntime.queryInterface(
                XRefreshable.class, suppTables.getTables());
        refreshTables.refresh();
    }

    /** returns the name of the data source
     * 
     * If a data source is registered at the database context, the name is the registration
     * name. Otherwise, its the URL which the respective database document is based on.
     * 
     * Note that the above definition is from the UNO API, not from this wrapper here.
     */
    public String getName()
    {
        String name = null;
        try
        {
            final XPropertySet dataSourceProps = (XPropertySet) UnoRuntime.queryInterface(
                    XPropertySet.class, m_dataSource);
            name = (String) dataSourceProps.getPropertyValue("Name");
        }
        catch (Exception ex)
        {
            Logger.getLogger(DataSource.class.getName()).log(Level.SEVERE, null, ex);
        }
        return name;
    }
};
