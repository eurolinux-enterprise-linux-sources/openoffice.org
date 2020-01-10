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

package com.sun.star.lib.sandbox;

import java.io.File;
import java.io.IOException;
import java.io.FileDescriptor;
import java.net.URL;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.StringTokenizer;
import java.util.Vector;
import java.util.Hashtable;
import java.security.*;
import sun.security.provider.*;

/**
 * This class defines an applet security policy
 *
 * @version 	1.68, 01/27/97
 */
public class SandboxSecurity extends SecurityManager //implements SecurityManagerExtension
{

    private static boolean debug = false;

    public final static int NETWORK_NONE = 1;
    public final static int NETWORK_HOST = 2;
    public final static int NETWORK_UNRESTRICTED = 3;

    private final static int PRIVELEGED_PORT = 1024;

	boolean bNoExit;
    boolean initACL;
    String	readACL[];
    String	writeACL[];
    int		networkMode;
	boolean bCheckSecurity;
    RecursionCounter InCheck = new RecursionCounter();
    RecursionCounter InIsSecureLoader = new RecursionCounter();
    RecursionCounter InInClassLoader = new RecursionCounter();
    RecursionCounter InClassLoaderDepth = new RecursionCounter();
    java.security.AllPermission allPerm = new java.security.AllPermission();
    
    // where we look for identities
    IdentityScope scope;
    // local cache for network-loaded classes
    Hashtable loadedClasses;

    public int getNetworkMode(){ return networkMode; }

    /**
     * Construct and initialize.
     */
    public SandboxSecurity() {
		reset();
    }

    /**
     * Construct and initialize.
     */
    public SandboxSecurity( boolean bNoExit_ ) {
		reset();
		bNoExit = bNoExit_;
    }

    /**
     * Reset from Properties
     */
    public void reset() {
		String str = System.getProperty("appletviewer.security.mode");
//  		System.err.println("#### SandboxSecurity.reset:" + str); 
		if( str == null )
			networkMode = NETWORK_HOST;
		else if( str.equals("unrestricted") )
			networkMode = NETWORK_UNRESTRICTED;
		else if( str.equals("none") )
			networkMode = NETWORK_NONE;
		else
			networkMode = NETWORK_HOST;

  		bCheckSecurity = !Boolean.getBoolean( "stardiv.security.disableSecurity" );
		// see if security is disabled
//		String prop = System.getProperty("Security", "1" );
//		bCheckSecurity = true;
//		if(prop.equals("0"))
//			bCheckSecurity = false;

		// see if the system scope is one we know.
		IdentityScope scope = IdentityScope.getSystemScope();
		
		if (scope instanceof IdentityDatabase) {
			this.scope = (IdentityDatabase)scope;
			debug("installing " + scope + " as the scope for signers.");
		} else  {
			debug("no signer scope found.");	    
		}
		loadedClasses = new Hashtable();
    }

//      /**
//       * True if called directly from an applet.
//       */
//      boolean fromApplet() {
//  	return classLoaderDepth() == 1;
//      }

//      /**
//       * This method takes a set of signers and returns true if 
//       * this set of signers implies that a class is trusted.
//       * In this implementation, it returns true if any of the
//       * signers is a SystemIdentity which is trusted.
//       */
//      protected boolean assessTrust(Object[] signers) {
//  	/* Remind: do we want to actually look into the scope here? */

//  	for (int i = 0; i < signers.length; i++) {

//  	    if (signers[i] instanceof SystemIdentity) {
//  		SystemIdentity sysid = (SystemIdentity)signers[i];
//  		if (sysid.isTrusted()) {
//  		    return true;
//  		}

//  	    } else if (signers[i] instanceof SystemSigner) {
//  		SystemSigner sysid = (SystemSigner)signers[i];
//  			if (sysid.isTrusted()) {
//  			    return true;
//  			}
//  	    }
//  	}

//  	return false;
//      }

//      /**
//       * True if called indirectly from an <it>untrusted</it> applet.
//       */
    synchronized boolean inApplet() {
        boolean ret = false;
        try {
            InCheck.acquire();
            ret = inClassLoader();
        } finally {
            InCheck.release();
        }
        return ret;
    }

    /**
     * The only variable that currently affects whether an applet can
     * perform certain operations is the host it came from.
     */
    public synchronized Object getSecurityContext() {
        Object ret = null;
        try {
            InCheck.acquire();
            ClassLoader loader = currentClassLoader();
            
            if (loader != null) {
                if (loader instanceof ClassContextImpl) {
                    ClassContext appletLoader = (ClassContextImpl)loader;
                    ret = appletLoader.getBase();
                } else {
                    throw(new SandboxSecurityException("getsecuritycontext.unknown"));
                }
            }
        }
        finally {
            InCheck.release();
        }
        return ret;
    }       

    /**
     * Applets are not allowed to create class loaders, or even execute any
     * of ClassLoader's methods. The name of this method should be changed to
     * checkClassLoaderOperation or somesuch.
     */
    public synchronized void checkCreateClassLoader() {
        try {
            InCheck.acquire();
            if( bCheckSecurity && !isSecureLoader() ) {
                if (classLoaderDepth() == 2)
                    throw(new SandboxSecurityException("checkcreateclassloader"));
            }
        } finally {
            InCheck.release();
        }
    }

    /**
     * Returns true if this threadgroup is in the applet's own thread
     * group. This will return false if there is no current class
     * loader.
     */
    protected synchronized boolean inThreadGroup(ThreadGroup g) {
        boolean ret = false;
        try {
            InCheck.acquire();
            ClassLoader loader = currentClassLoader();
		
            /* If this class wasn't loaded by an AppletClassLoader, we have
               not eay of telling, for now. */
            
            if (loader instanceof ClassContextImpl) {
                ClassContext appletLoader = (ClassContextImpl)loader;
                ThreadGroup appletGroup = appletLoader.getThreadGroup();
                ret = appletGroup.parentOf(g);
            }
        } finally {
            InCheck.release();
        }
		return ret;
    }

    /**
     * Returns true of the threadgroup of thread is in the applet's
     * own threadgroup.
     */
    protected synchronized boolean inThreadGroup(Thread thread) {
        boolean ret = false;
        try {
            InCheck.acquire();
            ret = inThreadGroup(thread.getThreadGroup());
        }
        finally {
            InCheck.release();
        }
        return ret;
    }

    /**
     * Applets are not allowed to manipulate threads outside
     * applet thread groups.
     */
    public synchronized void checkAccess(Thread t) {
        try {
            InCheck.acquire();
            if( bCheckSecurity && !isSecureLoader() ) {
                if (classLoaderDepth()==3 && (! inThreadGroup(t))) { 
                    throw(new SandboxSecurityException("checkaccess.thread"));
                }
            }
        } finally {
            InCheck.release();
        }
    }

    /**
     * Applets are not allowed to manipulate thread groups outside
     * applet thread groups.
     */
    public synchronized void checkAccess(ThreadGroup g) {
        try {
            InCheck.acquire();
            if( bCheckSecurity && !isSecureLoader() ) {
                if (classLoaderDepth() == 4 && (! inThreadGroup(g))) {
                    throw(new SandboxSecurityException("checkaccess.threadgroup", g.toString()));
                }
            }
        } finally {
            InCheck.release();
        }
        
    }

    /**
     * Applets are not allowed to exit the VM.
     */
    public synchronized void checkExit(int status) {
        try {
            InCheck.acquire();
            if( bNoExit ) {
                throw(new SandboxSecurityException("checkexit", String.valueOf(status)));
            }
            if( bCheckSecurity && !isSecureLoader() ) {
                if( inApplet() ) {
                    throw(new SandboxSecurityException("checkexit", String.valueOf(status)));
                }
            }
		} finally {
            InCheck.release();
        }
    }

    /**
     * Applets are not allowed to fork processes.
     */
    public synchronized void checkExec(String cmd) {
        try {
            InCheck.acquire();
            if( bCheckSecurity && !isSecureLoader()) {
                if (inApplet()) {
                    throw(new SandboxSecurityException("checkexec", cmd));
                }
            }
		} finally {
            InCheck.release();
        }   
    }

    /**
     * Applets are not allowed to link dynamic libraries.
     */
    public synchronized void checkLink(String lib) {
        try {
            InCheck.acquire();
            if( bCheckSecurity && !isSecureLoader() ) {
                switch (classLoaderDepth()) {
                case 2: // Runtime.load
                case 3: // System.loadLibrary
					throw(new SandboxSecurityException("checklink", lib));
                default:
                    break;
                }
            }
        } finally {
            InCheck.release();
        }
    }

    /**
     * Applets are not allowed to access the entire system properties
     * list, only properties explicitly labeled as accessible to applets.
     */
    public synchronized void checkPropertiesAccess() {
        try {
            InCheck.acquire();
            if( bCheckSecurity && !isSecureLoader() ) {
                if (classLoaderDepth() == 2) {
                    throw(new SandboxSecurityException("checkpropsaccess"));
                }
            }
        } finally {
            InCheck.release();
        }
    }

    /**
     * Applets can access the system property named by <i>key</i>
     * only if its twin <i>key.applet</i> property is set to true.
     * For example, the property <code>java.home</code> can be read by
     * applets only if <code>java.home.applet</code> is <code>true</code>.
     */
    public synchronized void checkPropertyAccess(String key) {
        try {
            InCheck.acquire();
            if( bCheckSecurity && !isSecureLoader() ) {
                if (classLoaderDepth() == 2) {
                    String prop = System.getProperty(key + ".applet");
                    boolean allow = new Boolean(prop).booleanValue();
                    if ( !allow ) {
                        throw(new SandboxSecurityException("checkpropsaccess.key", prop));
                    }
                }
            }
        } finally {
            InCheck.release();
        }
    }

    /**
     * Parse an ACL. Deals with "~" and "+"
     */
    void parseACL(Vector v, String path, String defaultPath) {
		String sep = System.getProperty("path.separator");
		StringTokenizer t = new StringTokenizer(path, sep);

		while (t.hasMoreTokens()) {
			String dir = t.nextToken();
			if (dir.startsWith("~")) {
			v.addElement(System.getProperty("user.home") + 
					 dir.substring(1));
			} else if (dir.equals("+")) {
			if (defaultPath != null) {
				parseACL(v, defaultPath, null);
			}
			} else {
			v.addElement(dir);
			}
		}
    }

    /**
     * Parse an ACL.
     */
    String[] parseACL(String path, String defaultPath) {
		if (path == null) {
			return new String[0];
		}
		if (path.equals("*")) {
			return null;
		}
		Vector v = new Vector();
		parseACL(v, path, defaultPath);

		String acl[] = new String[v.size()];
		v.copyInto(acl);
		return acl;
    }

    /**
     * Initialize ACLs. Called only once.
     */
    void initializeACLs() {
		readACL = parseACL(System.getProperty("acl.read"), 
				   System.getProperty("acl.read.default"));
		writeACL = parseACL(System.getProperty("acl.write"), 
					System.getProperty("acl.write.default"));
		initACL = true;
    }

    /**
     * Check if an applet can read a particular file.
     */
    public synchronized void checkRead(String file) {
        try {
            InCheck.acquire();
            if( bCheckSecurity && !isSecureLoader() ) {
                ClassLoader loader = currentClassLoader();

                /* If no class loader, it's a system class. */
                if (loader != null)
                {
                    /* If not an AppletClassLoader, we don't know what to do */
                    if (! (loader instanceof ClassContextImpl))
                        throw(new SandboxSecurityException("checkread.unknown", file));
                    ClassContext appletLoader = (ClassContextImpl)loader;
                    URL base = appletLoader.getBase();
                    checkRead(file, base);
                }
            }
		} finally {
            InCheck.release();
        }
    }

    public synchronized void checkRead(String file, URL base) {
        try {
            InCheck.acquire();
            if( bCheckSecurity && (base != null) && !isSecureLoader() ) {
                if (!initACL) 
                    initializeACLs();
                if (readACL == null)
                {
                    InCheck.release();
                    return;
                }
		
                String realPath = null;
                try {
                    realPath = (new File(file)).getCanonicalPath();
                } catch (IOException e) {
                    throw(new SandboxSecurityException("checkread.exception1", e.getMessage(), file));
                }
                
                for (int i = readACL.length ; i-- > 0 ;) {
                    if (realPath.startsWith(readACL[i]))
                    {
                        InCheck.release();
                        return;
                    }
                }
                
                // if the applet is loaded from a file URL, allow reading
                // in that directory
                if (base.getProtocol().equals("file")) {
                    String dir = null;
                    try {
                        // If the file url contains spaces (i.e. %20) then URL.getFile() still contains %20
                        // File.getCanonicalPath does not replace %20
                        // create a string with real spaces instead of %20
                        StringBuffer buf= new StringBuffer(256);
                        String sSpace= "%20";
                        String sFile= base.getFile();
                        int begin= 0;
                        int end= 0;
                        while((end= sFile.indexOf(sSpace, begin)) != -1) {
                            buf.append( sFile.substring(begin, end));
                            buf.append(" ");
                            begin= end + sSpace.length();
                        }
                        buf.append(sFile.substring(begin));
                        
                        String sWithSpaces= buf.toString();
                        dir = (new File(sWithSpaces).getCanonicalPath());
                    } catch (IOException e) { // shouldn't happen
                        throw(new SandboxSecurityException("checkread.exception2", e.toString()));
                    }
                    if (realPath.startsWith(dir))
                    {
                        InCheck.release();
                        return;
                    }
                }
                throw new SandboxSecurityException("checkread", file, realPath);
            }
        } finally {
            InCheck.release();
        }
        
    }
        /**
     * Checks to see if the current context or the indicated context are
     * both allowed to read the given file name.
     * @param file the system dependent file name
     * @param context the alternate execution context which must also
     * be checked
     * @exception  SecurityException If the file is not found.
     */
    public synchronized void checkRead(String file, Object context) {
        try {
            InCheck.acquire();
            checkRead(file);
            if (context != null) 
                checkRead(file, (URL) context);
        } finally {
            InCheck.release();
        }
    }

    /**
     * Check if an applet can write a particular file.
     */
    public synchronized void checkWrite(String file) {
        try {
            InCheck.acquire();
            if( bCheckSecurity && inApplet() && !isSecureLoader() ) {
                if (!initACL)
                    initializeACLs();
                if (writeACL == null)
                {
                    InCheck.release();
                    return;
                }

                String realPath = null;
                try {
                    realPath = (new File(file)).getCanonicalPath();
                } catch (IOException e) {
                    throw(new SandboxSecurityException("checkwrite.exception", e.getMessage(), file));
                }

                for (int i = writeACL.length ; i-- > 0 ;) {
                    if (realPath.startsWith(writeACL[i]))
                    {
                        InCheck.release();
                        return;
                    }
			}
			throw(new SandboxSecurityException("checkwrite", file, realPath));
		}
        } finally {
            InCheck.release();
        }
    }   

    /**
     * Applets are not allowed to open file descriptors unless
     * it is done through a socket, in which case other access
     * restrictions still apply.
     */
    public synchronized void checkRead(FileDescriptor fd) {
        try {
            InCheck.acquire();
            if( bCheckSecurity && !isSecureLoader() ) {
                if( (inApplet() && !inClass("java.net.SocketInputStream") ) || (!fd.valid()) ) 
                    throw(new SandboxSecurityException("checkread.fd"));
            }
		} finally {
            InCheck.release();
        }
	}

    /**
     * Applets are not allowed to open file descriptors unless
     * it is done through a socket, in which case other access
     * restrictions still apply.
     */
    public synchronized void checkWrite(FileDescriptor fd) {
        try {
            InCheck.acquire();
            if( bCheckSecurity && !isSecureLoader() ) {
                if( (inApplet() && !inClass("java.net.SocketOutputStream")) || (!fd.valid()) )
                    throw(new SandboxSecurityException("checkwrite.fd"));
            }
        } finally {
            InCheck.release();
        }
    }

    /**
     * Applets can only listen on unpriveleged ports > 1024
     * A port of 0 denotes an ephemeral system-assigned port
     * Which will be outside this range.  Note that java sockets
     * take an int and ports are really a u_short, but range
     * checking is done in ServerSocket & DatagramSocket, so the port policy
     * cannot be subverted by ints that wrap around to an illegal u_short.
     */
    public synchronized void checkListen(int port) {
        try {
            InCheck.acquire();
            if( bCheckSecurity && !isSecureLoader() ) {
                if (inApplet() && port > 0 && port < PRIVELEGED_PORT)
                    throw(new SandboxSecurityException("checklisten", String.valueOf(port)));
            }
		} finally {
            InCheck.release();
        }
    }

    /**
     * Applets can accept connectionions on unpriveleged ports, from
     * any hosts they can also connect to (typically host-of-origin
     * only, depending on the network security setting).
     */
    public synchronized void checkAccept(String host, int port) {
        try {
            InCheck.acquire();
            if( bCheckSecurity && !isSecureLoader() ) {
                if( inApplet() && port < PRIVELEGED_PORT )
                    throw(new SandboxSecurityException("checkaccept", host, String.valueOf(port)));
                checkConnect(host, port);
            }
        } finally {
            InCheck.release();
        }
    }

    /**
     * Check if an applet can connect to the given host:port.
     */
    public synchronized void checkConnect(String host, int port) {
        try {
            InCheck.acquire();
            if(bCheckSecurity && !isSecureLoader() ) {
                ClassLoader loader = currentClassLoader();
                if (loader == null)
                {
                    InCheck.release();
                    return; // Not called from an applet, so it is ok
                }

                // REMIND: This is only appropriate for our protocol handlers.
                int depth = classDepth("sun.net.www.http.HttpClient");
                if (depth > 1)
                {
                    InCheck.release();
                    return; // Called through our http protocol handler
                }
			
                if(getInCheck())
                {
                    InCheck.release();
                    return;
                }

                if (loader instanceof ClassContextImpl) {
                    ClassContext appletLoader = (ClassContextImpl)loader;
                    checkConnect(appletLoader.getBase().getHost(), host);
                } else {
                    throw(new SandboxSecurityException("checkconnect.unknown"));
                }
            }
        } finally {
            InCheck.release();
        }
    }

    /**
     * Checks to see if the applet and the indicated execution context
     * are both allowed to connect to the indicated host and port.
     */
    public synchronized void checkConnect(String host, int port, Object context) {
        try {
            InCheck.acquire();
            checkConnect(host, port);
            if (context != null)
                checkConnect(((URL) context).getHost(), host);
        } finally {
            InCheck.release();
        }
    }

    public synchronized void checkConnect(String fromHost, String toHost, boolean trustP) {
        try {
            InCheck.acquire();
            if( bCheckSecurity && !isSecureLoader() ) {
                if (fromHost == null)
                {
                    InCheck.release();
                    return;
                }
                
                switch (networkMode) {
                case NETWORK_NONE:
					throw(new SandboxSecurityException("checkconnect.networknone", fromHost, toHost));
                    
                case NETWORK_HOST:
                    /*
                     * The policy here is as follows:
                     *
                     * - if the strings match, and we know the IP address for it
                     * we allow the connection. The calling code downstream will
                     * substitute the IP in their request to the proxy if needed.
                     * - if the strings don't match, and we can get the IP of
                     * both hosts then
                     *   - if the IPs match, we allow the connection
                     *   - if they don't we throw(an exception
                     * - if the string match works and we don't know the IP address
                     * then we consult the trustProxy property, and if that is true,
                     * we allow the connection.
                     * set inCheck so InetAddress knows it doesn't have to
                     * check security.
                     */
                    try {
                        inCheck = true;
                        InetAddress toHostAddr, fromHostAddr;
                        if (!fromHost.equals(toHost)) {
                            try {
                                // the only time we allow non-matching strings
                                // is when IPs and the IPs match.
                                toHostAddr = InetAddress.getByName(toHost);
                                fromHostAddr = InetAddress.getByName(fromHost);
                                
                                if( fromHostAddr.equals(toHostAddr) )
                                {
                                    InCheck.release();
                                    return;
                                }
                                else
                                {
                                    throw(new SandboxSecurityException(
                                              "checkconnect.networkhost1", toHost, fromHost));
                                }
                                
                            } catch (UnknownHostException e) {
                                throw(new SecurityException("checkconnect.networkhost2" + toHost + fromHost));
//  							throw(new SandboxSecurityException("checkconnect.networkhost2", toHost, fromHost));
                            }  
                        } else {
                            try {
                                toHostAddr = InetAddress.getByName(toHost);
                                InCheck.release();
                                // strings match: if we have IP, we're homefree, 
                                // otherwise we check the properties.
                                return;
                                // getBoolean really defaults to false.		
                            } catch (UnknownHostException e) {
                                if( trustP )
                                {
                                    InCheck.release();
                                    return;
                                }
                                else
                                {
                                    throw(new SandboxSecurityException(
                                              "checkconnect.networkhost3", toHost));
                                }
                            }
                        }
                    } finally {
                        inCheck = false;
                    }
                    
                case NETWORK_UNRESTRICTED:
                    InCheck.release();
                    return;
                }
                throw(new SandboxSecurityException("checkconnect", fromHost, toHost));
            } 
        } finally {
            InCheck.release();
        }
    }


 
    /**
     * check if an applet from a host can connect to another
     * host. This usually means that you need to determine whether
     * the hosts are inside or outside the firewall. For now applets
     * can only access the host they came from.
     */
    public synchronized void checkConnect(String fromHost, String toHost) {
        try {
            InCheck.acquire();
            checkConnect(fromHost, toHost, Boolean.getBoolean("trustProxy"));
        } finally {
            InCheck.release();
        }
    }
	
    /**
     * Checks to see if top-level windows can be created by the caller.
     */
    public synchronized boolean checkTopLevelWindow(Object window) {
        boolean ret = true;
        try {
            InCheck.acquire();
            if( bCheckSecurity && inClassLoader() && !isSecureLoader() ) {
                /* XXX: this used to return depth > 3. However, this lets */
                /* some applets create frames without warning strings. */
                ret = false;
            }
        } finally {
            InCheck.release();
        }
		return ret;
    } 

    /**
     * Check if an applet can access a package.
     */
    public synchronized void checkPackageAccess(String pkg) {
        try {
            InCheck.acquire();
            
            if( bCheckSecurity && inClassLoader() && !isSecureLoader() ) {
                if( pkg.equals( "stardiv.applet" )
                    // Das AWT von StarDivision
                    || pkg.equals( "stardiv.look" )
                    || pkg.equals( "netscape.javascript" ) )
                {
                    InCheck.release();
                    return;
                }

                final String forbidden[] = new String[]{
                    "com.sun.star.uno",
                    "com.sun.star.lib.uno",
                    "com.sun.star.comp.connections",
                    "com.sun.star.comp.loader",
                    "com.sun.star.comp.servicemanager"
                };


                for(int j = 0; j < forbidden.length; ++ j) {
                    if(pkg.startsWith(forbidden[j]))
                        throw(new SandboxSecurityException("checkpackageaccess2", pkg));
                }
                
                int i = pkg.indexOf('.');
                while (i > 0) {
                    String subpkg = pkg.substring(0,i);
                    if( Boolean.getBoolean("package.restrict.access." + subpkg) )
                        throw(new SandboxSecurityException("checkpackageaccess", pkg));
                    i = pkg.indexOf('.',i+1);
                }
            }
        } finally {
            InCheck.release();
        }
    }

    /**
     * Check if an applet can define classes in a package.
     */
    public synchronized void checkPackageDefinition(String pkg) {
        try {
            InCheck.acquire();
        } finally {
            InCheck.release();
        }
		return;
/*
	if (!inClassLoader())
	    return;
    	int i = pkg.indexOf('.');

    	while (i > 0) {
	    String subpkg = pkg.substring(0,i);
	    if (Boolean.getBoolean("package.restrict.definition." + subpkg)) {
                throw(new SandboxSecurityException("checkpackagedefinition", pkg);
	    }
	    i = pkg.indexOf('.',i+1);
        }
*/
    }


    /**
     * Check if an applet can set a networking-related object factory.
     */
    public synchronized void checkSetFactory() {
        try {
            InCheck.acquire();
            if( bCheckSecurity && inApplet() && !isSecureLoader() )
                throw(new SandboxSecurityException("cannotsetfactory"));
        } finally {
            InCheck.release();
        }
	}

    /**
     * Check if client is allowed to reflective access to a member or
     * a set of members for the specified class.  Once initial access
     * is granted, the reflected members can be queried for
     * identifying information, but can only be <strong>used</strong>
     * (via get, set, invoke, or newInstance) with standard Java
     * language access control.
     *
     * <p>The policy is to deny <em>untrusted</em> clients access to
     * <em>declared</em> members of classes other than those loaded
     * via the same class loader.  All other accesses are granted.
     *
     * XXX: Should VerifyClassAccess here?  Should Class.forName do it?
     */
    public synchronized void checkMemberAccess(Class clazz, int which) {
        try {
            InCheck.acquire();
            
            if( bCheckSecurity && !isSecureLoader() ) {
                if( which != java.lang.reflect.Member.PUBLIC ) {
                    ClassLoader currentLoader = currentClassLoader();
                    if( currentLoader != null && (classLoaderDepth() <= 3) )
                        /* Client is an untrusted class loaded by currentLoader */
                        if( currentLoader != clazz.getClassLoader() )
                            throw(new SandboxSecurityException("checkmemberaccess"));
                }
            }
        } finally {
            InCheck.release();
        }
    }

    /**
     * Checks to see if an applet can initiate a print job request.
     */
    public synchronized void checkPrintJobAccess() {
        try {
            InCheck.acquire();
            if( bCheckSecurity && inApplet() && !isSecureLoader() )
                throw(new SandboxSecurityException("checkgetprintjob"));
        } finally {
            InCheck.release();
        }
    }

    /**
     * Checks to see if an applet can get System Clipboard access.
     */
    public synchronized void checkSystemClipboardAccess() {
        try {
            InCheck.acquire();
            
            if( bCheckSecurity && inApplet() && !isSecureLoader() )
                throw(new SandboxSecurityException("checksystemclipboardaccess"));
        } finally {
            InCheck.release();
        }
    }

    /**
     * Checks to see if an applet can get EventQueue access.
     */
    public synchronized void checkAwtEventQueueAccess() {
        try {
            InCheck.acquire();
            if( bCheckSecurity && inClassLoader() && !isSecureLoader() ) {
//    			throw(new SandboxSecurityException("checkawteventqueueaccess"));
            }
        } finally {
            InCheck.release();
        }
    }

    /**
     * Checks to see if an applet can perform a given operation.
     */
    public synchronized void checkSecurityAccess(String action) {
        try {
            InCheck.acquire();
            if( bCheckSecurity && inApplet() && !isSecureLoader() )
                throw(new SandboxSecurityException("checksecurityaccess", action));
        } finally {
            InCheck.release();
        }
	}

    /**
     * Returns the thread group of the applet. We consult the classloader
     * if there is one.
     */
    public synchronized ThreadGroup getThreadGroup() {
        ThreadGroup group = null;
        try {
            InCheck.acquire();
            /* First we check if any classloaded thing is on the stack. */
            ClassLoader loader = currentClassLoader();
            if (loader != null && (loader instanceof ClassContextImpl))
            {    
                if( inThreadGroup( Thread.currentThread() ) )
                {
                    group = Thread.currentThread().getThreadGroup();
                }
                else
                {
                    ClassContextImpl appletLoader = (ClassContextImpl)loader;
                    group = appletLoader.getThreadGroup();
                }
            }
            else
            {
                group = super.getThreadGroup();
            }
        } finally {
            InCheck.release();
        }
        return group;
    }
        
    public void debug(String s) {
		if( debug )
			System.err.println(s);
    }

    // This method is called from within the checkXXX method which
    //already track if this class is on the stack by using InCheck.
    private synchronized boolean isSecureLoader() {
        InIsSecureLoader.acquire();
        try {
            boolean bReturn = false;
            ClassLoader loader = currentClassLoader();
            if (loader != null) {
                if (loader instanceof ClassContextImpl) {
                    bReturn = !((ClassContextImpl) loader).checkSecurity();
                } else {
                    bReturn = true; // fremder ClassLoader: kann machen was er will
                }
            } else {
                bReturn = true;
            }
            return bReturn;
        } finally {
            InIsSecureLoader.release();
        }
    }



    /* In checkPermission we trap calls which are not covered by the old
      (1.1) SecurityManager functions. This is necessary, because whenever
      applets are used then the SecurityManager is set and Java components
      may cause a checkPermission call. Then, if a user has not specified
      permissions for this component, a SecurityException will be thrown
      and the component fails to work.

      Calls to java.lang.SecurityManager.inClassLoader result in
      a call to  this function. For example, a checkPackageAccess with the
      package java.text resulted in a call to this function with the permission
      All_Permission. We use the member InCheck in order to determine if we
      are in one of the check functions. If we are then checkPermissions does
      nothing. The calling checkXXX does the required check anyway.

      We cannot override
      void checkPermission(Permission perm, Object context)
      because we do not know if in the thread, which is represented by context,
      one of our security functions has been called. That is, we have no
      access to InCheck or the class loaders to dertime if we grant permission.

    */
    public synchronized void checkPermission(java.security.Permission perm)
    {
        //isSecureClassLoader calls SecurityManager.currentClassLoader, which then 
        //calls this function to check if we have AllPermission. If so, then currentClassLoader
        //returns null. Therefore we must throw and exception here. This is similar with 
        //the functions inClassLoader and classLoaderDepth.
        if (InIsSecureLoader.inRecursion() 
        || InInClassLoader.inRecursion()
        || InClassLoaderDepth.inRecursion()) 
        {
            if (perm.implies(allPerm))
                throw(new SandboxSecurityException("checkPermission ", perm.toString()));
        }
        if (InCheck.inRecursion() == false
            && isSecureLoader() == false)
        {
            throw(new SandboxSecurityException("checkPermission ", perm.toString()));
        }
    }
    
    protected boolean inClassLoader() 
    {
        InInClassLoader.acquire();
        try 
        {
            return super.inClassLoader();
        } finally {
            InInClassLoader.release();
        }
    }
    
    protected int classLoaderDepth()
    {
        InClassLoaderDepth.acquire();
        try 
        {
            return super.classLoaderDepth();
        } finally {
            InClassLoaderDepth.release();
        }

    }
}

class RecursionCounter
{
    void acquire()
    {
        int count = ((Integer)refCount.get()).intValue();
        refCount.set(new Integer(++count));
    }

    void release()
    {
        int count = ((Integer)refCount.get()).intValue();
        refCount.set(new Integer(--count));
    }

    boolean inRecursion() {
        int count = ((Integer)refCount.get()).intValue();
        return count > 0;
    }

    private ThreadLocal refCount = new ThreadLocal() {
         protected synchronized Object initialValue() {
             return new Integer(0);
         }
    };
}
