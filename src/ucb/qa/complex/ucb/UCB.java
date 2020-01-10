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

package complex.ucb;

/**
 * @author ab106281
 *
 * To change the template for this generated type comment go to
 * Window&gt;Preferences&gt;Java&gt;Code Generation&gt;Code and Comments
 */

import complexlib.ComplexTestCase;
import java.util.List;
import java.util.Vector;

import com.sun.star.beans.Property;
import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.sdbc.XResultSet;
import com.sun.star.sdbc.XRow;
import com.sun.star.uno.XComponentContext;
import com.sun.star.ucb.*;
import com.sun.star.bridge.XUnoUrlResolver;
import com.sun.star.uno.UnoRuntime;
import com.sun.star.uno.XComponentContext;
import com.sun.star.lang.XMultiComponentFactory;
import com.sun.star.beans.XPropertySet;
/**
 * @author rpiterman
 * This class is used to copy the content of a folder to
 * another folder.
 * There is an incosistency with argument order.
 * It should be always: dir,filename.
 */
public class UCB extends ComplexTestCase {
	private Object ucb;

	public String[] getTestMethodNames() {
		return new String[] {"checkWrongFtpConnection"};
	}

	public void init(XMultiServiceFactory xmsf) throws Exception {
		String[] keys = new String[2];
		keys[0] = "Local";
		keys[1] = "Office";
		ucb =
			xmsf.createInstanceWithArguments(
				"com.sun.star.ucb.UniversalContentBroker",
				keys);
	}

	public void delete(String filename) throws Exception {
		executeCommand(getContent(filename), "delete", Boolean.TRUE);
	}

	/**
	 * target name can be "", in which case the name stays lige the source name
	 * @param sourceDir
	 * @param filename
	 * @param targetDir
	 * @param targetName
	 * @return
	 */

	public Object executeCommand(
		Object xContent,
		String aCommandName,
		Object aArgument)
		throws com.sun.star.ucb.CommandAbortedException, com.sun.star.uno.Exception {
		XCommandProcessor xCmdProcessor =
			(XCommandProcessor) UnoRuntime.queryInterface(
				XCommandProcessor.class,
				xContent);
		Command aCommand = new Command();
		aCommand.Name = aCommandName;
		aCommand.Handle = -1; // not available
		aCommand.Argument = aArgument;
		return xCmdProcessor.execute(aCommand, 0, null);
	}

	private List listFiles(String path, Verifier verifier) throws Exception {
		Object xContent = getContent(path);

		OpenCommandArgument2 aArg = new OpenCommandArgument2();
		aArg.Mode = OpenMode.ALL;
		aArg.Priority = 32768;

		// Fill info for the properties wanted.
		aArg.Properties = new Property[] { new Property()};

		aArg.Properties[0].Name = "Title";
		aArg.Properties[0].Handle = -1;

		XDynamicResultSet xSet;

		xSet =
			(XDynamicResultSet) UnoRuntime.queryInterface(
				XDynamicResultSet.class,
				executeCommand(xContent, "open", aArg));

		XResultSet xResultSet = xSet.getStaticResultSet();

		List files = new Vector();

		if (xResultSet.first()) {
			// obtain XContentAccess interface for child content access and XRow for properties
			XContentAccess xContentAccess =
				(XContentAccess) UnoRuntime.queryInterface(
					XContentAccess.class,
					xResultSet);
			XRow xRow =
				(XRow) UnoRuntime.queryInterface(XRow.class, xResultSet);
			do {
				// Obtain URL of child.
				String aId = xContentAccess.queryContentIdentifierString();
				// First column: Title (column numbers are 1-based!)
				String aTitle = xRow.getString(1);
				if (aTitle.length() == 0 && xRow.wasNull());
				//ignore
				else
					files.add(aTitle);
			} while (xResultSet.next()); // next child
		}

		if (verifier != null)
			for (int i = 0; i < files.size(); i++)
				if (!verifier.verify(files.get(i)))
					files.remove(i--);

		return files;
	}

	public Object getContentProperty(
		Object content,
		String propName,
		Class type)
		throws Exception {
		Property[] pv = new Property[1];
		pv[0] = new Property();
		pv[0].Name = propName;
		pv[0].Handle = -1;

		Object row = executeCommand(content, "getPropertyValues", pv);
		XRow xrow = (XRow) UnoRuntime.queryInterface(XRow.class, row);
		if (type.equals(String.class))
			return xrow.getString(1);
		else if (type.equals(Boolean.class))
			return xrow.getBoolean(1) ? Boolean.TRUE : Boolean.FALSE;
		else if (type.equals(Integer.class))
			return new Integer(xrow.getInt(1));
		else if (type.equals(Short.class))
			return new Short(xrow.getShort(1));
		else
			return null;

	}

	public Object getContent(String path) throws Exception {
		XContentIdentifier id =
			(
				(XContentIdentifierFactory) UnoRuntime.queryInterface(
					XContentIdentifierFactory.class,
					ucb)).createContentIdentifier(
				path);

		return (
			(XContentProvider) UnoRuntime.queryInterface(
				XContentProvider.class,
				ucb)).queryContent(
			id);
	}

	public static interface Verifier {
		public boolean verify(Object object);
	}

	public void checkWrongFtpConnection() {
		//localhost  ;Lo-1.Germany.sun.com; 10.16.65.155
		try {
			XMultiServiceFactory xLocMSF = (XMultiServiceFactory)param.getMSF();
			String acountUrl = "ftp://noname:nopasswd@nohost";
			log.println(acountUrl);
			init(xLocMSF);
			Object content = getContent(acountUrl);

			OpenCommandArgument2 aArg = new OpenCommandArgument2();
			aArg.Mode = OpenMode.ALL; // FOLDER, DOCUMENTS -> simple filter
			aArg.Priority = 32768; // Ignored by most implementations

			log.println("now executing open");
			executeCommand(content, "open", aArg);
			failed("Expected 'IllegalArgumentException' was not thrown.");
		} catch (com.sun.star.lang.IllegalArgumentException ex) {
			//TODO error message;
			log.println("Correct exception thrown: " + ex.getClass().toString());
		} catch(com.sun.star.ucb.InteractiveNetworkResolveNameException ex) {
			log.println("This Exception is correctly thrown when no Proxy in StarOffice is used.");
			log.println("To reproduce the bug behaviour, use a Proxy and try again.");
		} catch (Exception ex) {
			ex.printStackTrace((java.io.PrintWriter)log);
			String exceptionName = ex.toString();
			log.println("ExName: '"+exceptionName+"'");
			failed("Wrong exception thrown: " + exceptionName);
		}
//		System.exit(0);
	}

}
