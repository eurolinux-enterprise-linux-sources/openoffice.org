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

import java.awt.Image;
import java.awt.Dimension;
import java.awt.Container;
import java.awt.BorderLayout;

import java.applet.Applet;
import java.applet.AppletStub;
import java.applet.AppletContext;
import java.applet.AudioClip;

import java.io.IOException;
import java.io.InputStream;
import java.io.ByteArrayOutputStream;

import java.net.URL;
import java.net.MalformedURLException;

import java.text.MessageFormat;

import java.util.Hashtable;
import java.util.Observable;
import java.util.MissingResourceException;
import java.util.ResourceBundle;

public abstract class ExecutionContext extends Observable {
	private static final boolean DEBUG = false;
	
	private static int instances;

    /* message ids */
    protected final static int CMD_LOAD    = 1;
    protected final static int CMD_INIT    = 2;
    protected final static int CMD_START   = 3;
    protected final static int CMD_STOP    = 4;
    protected final static int CMD_DESTROY = 5;
    protected final static int CMD_DISPOSE = 6;

    protected final static int LOADED    = 1;
    protected final static int INITED    = 2;
    protected final static int STARTED   = 3;
    protected final static int STOPPED   = 4;
    protected final static int DESTROYED = 5;
    protected final static int DISPOSED  = 6;

    private int status = DISPOSED;
    private Object statusLock= new Object();
    private boolean bDispatchException;
    
	protected ClassContext classContext;

    private Thread dispatchThread = null;
    private SandboxThreadGroup threadGroup = null;

    private String name;

	protected ResourceBundle resourceBundle;

	private Object synObj = new Object();
	private Message head;
	private Message tail;
	private boolean loop = true;

	protected ExecutionContext() {
		instances ++;
	}

	public void finalize() {
		instances --;
	}

	public int getStatus() {
		return status;
	}

	Object getSynObject() {
		return synObj;
	}

	class Message {
		Message next;
		int id;

		Message(int id) {
			this.id = id;
		}
	}

    public void init(String name, ClassContext classContext) throws MissingResourceException {
		this.name = name;

		resourceBundle = ResourceBundle.getBundle("sun.applet.resources.MsgAppletViewer");

		this.classContext = classContext;

		threadGroup = new SandboxThreadGroup(classContext.getThreadGroup(), name, classContext.getClassLoader());
		threadGroup.setDaemon(true); 

		dispatchThread = new Thread( threadGroup, new Runnable() {
			public void run() {
				while( loop ) {
                    if( head != null) {
                        if (DEBUG) System.err.println("#### ExecutionContext dispatchThread " + dispatchThread.toString() + " -dispatching: " + head.id);
                        dispatch( head.id );
                        if (DEBUG) System.err.println("#### ExecutionContext dispatchThread " + dispatchThread.toString() + " get next head - current state is " +head.id );
                        synchronized( getSynObject() ) {
                            head = head.next;
                            getSynObject().notify();
                        }
                    }

                    synchronized( getSynObject() ) {
						if (head == null) {
							try {
								getSynObject().wait();
							}
							catch (InterruptedException e ) {
								if (DEBUG) System.err.println("#### ExecutionContext - dispatchThread " + dispatchThread.toString()  + " -interrupted");
  								break;
							}				
						}
					}
				}
  				if(DEBUG) System.err.println("#### ExecutionContext - dispatchThread  -terminating");
			}
		});

        dispatchThread.setDaemon(true);
		dispatchThread.start();
    }
    public void sendEvent(int id) {
		sendEvent(id, 0);
	}

    public void sendEvent(int id, int timeout) {
  		synchronized( getSynObject() ) {
			try {
				Message message = new Message(id);
				if(tail != null)
					tail.next = message;
				
				tail = message;
				
				if(head == null)
					head = tail;
				
				getSynObject().notify();
				
				if ( timeout != 0 )
					getSynObject().wait( timeout );
			}
			catch( InterruptedException e ) {
			}
  		}
	}

	public void dispose() {
  		//if(DEBUG) System.err.println("#### ExecutionContext.disposing");
		dispose(1000);
	}

	public void dispose( long timeout ) {
		if(DEBUG) System.err.println("#### ExecutionContext "+ dispatchThread.toString() +"disposing:" + timeout);
		try {
			try {
				synchronized( getSynObject() ) {
					while( head != null )
						getSynObject().wait( timeout ); // wait at most one second for each queued command     
                      loop = false;
                   	getSynObject().notifyAll();
 				}
              dispatchThread.join(timeout);
			}
			catch(InterruptedException ee) {
				if(DEBUG) System.err.println("#### ExecutionContext " + dispatchThread.toString() + " - dispose 1:" + ee);
			}
		
			if(DEBUG) threadGroup.list();
			if ( !threadGroup.isDestroyed() )
				threadGroup.destroy();
		}
		catch (Exception ie) {
			if(DEBUG) System.err.println("#### ExecutionContext "+ threadGroup.toString() + " - destroyThreadGroup:" + ie);
			try {
				threadGroup.stop();
			} catch (Exception se) {
				if(DEBUG) System.err.println("#### ExecutionContext "+ threadGroup.toString() + " - stop ThreadGroup:" + se);
			}
		}

		classContext = null;

		dispatchThread = null;
		threadGroup.dispose();
		threadGroup = null;

		name = null;

		resourceBundle = null;

		synObj = null;
		head = null;
		tail = null;
	}


    protected void showStatus(String status) {
		if (DEBUG) System.err.println("#### ExecutionContext.showStatus:" + status);
  		setChanged();
  		notifyObservers(resourceBundle.getString("appletpanel." + status));
	}

    protected void showStatus(String status, String arg1) {
		if(DEBUG) System.err.println("#### ExecutionContext.showStatus" + status + " " + arg1);
		try {
			Object args[] = new Object[1];
			args[0] = arg1;
			setChanged();
			try {
				notifyObservers(MessageFormat.format(resourceBundle.getString("appletpanel." + status), args));
			}
			catch(MissingResourceException me) {}
		}
		catch(Exception ee) {
			if(DEBUG)System.err.println("#### ExecutionContext.showStatus:" + ee);
		}
    }


    public ThreadGroup getThreadGroup() {
		return threadGroup;
    }

    /**
     * Send an event. Queue it for execution by the handler thread.
     */
    public void dispatch(int id) {
		try {
			switch(id) {
			case CMD_LOAD:    
				if (status == DISPOSED) {
					xload();
					setStatus(LOADED);
					showStatus("loaded");
				}
				else
					showStatus("notdisposed");
				break;

			case CMD_INIT:
				if(status == LOADED || status == DESTROYED) {
					xinit();
					setStatus(INITED);
					showStatus("inited");
				}
				else
					showStatus("notloaded");
				break;
		    
			case CMD_START:   
				if (status == INITED || status == STOPPED) {
					xstart(); 
					setStatus(STARTED);
					showStatus("started");
				}
				else
					showStatus("notinited");
				break;
		    
			case CMD_STOP:    
				if (status == STARTED) {
					xstop(); 
					setStatus(STOPPED);
					showStatus("stopped");
				}
				else
					showStatus("notstarted");
				break;
		    
			case CMD_DESTROY: 
				if(status == INITED || status == STOPPED) {
					xdestroy(); 
					setStatus(DESTROYED);
					showStatus("destroyed");
				}
				else
					showStatus("notstopped");
				break;
		    
			case CMD_DISPOSE:
				if (status == LOADED || status == DESTROYED) {
					xdispose();
					//	baseResourceLoader.flush();
					showStatus("disposed");
                  setStatus(DISPOSED);  
				}
				else
					showStatus("notdestroyed");
				break;
					
			default:
				xExtended(id);
			}
		} 
		catch (ClassNotFoundException classNotFoundException) {
            setDispatchException();
			showStatus("notfound", name);
			if(DEBUG) classNotFoundException.printStackTrace();
		}
		catch (InstantiationException instantiationException) {
            setDispatchException();
			showStatus("nocreate", name);
			if(DEBUG) instantiationException.printStackTrace();
		}
		catch (IllegalAccessException illegalAccessException) {
            setDispatchException();
			showStatus("noconstruct", name);
			if(DEBUG) illegalAccessException.printStackTrace();
		}
		catch (Exception exception) {
            setDispatchException();
			showStatus("exception", exception.getMessage());
			if(DEBUG) exception.printStackTrace();
		}
		catch (ThreadDeath threadDeath) {
            setDispatchException();
			showStatus("death");
			if(DEBUG) threadDeath.printStackTrace();

			throw threadDeath;
		}
		catch (Error error) {
            setDispatchException();
			showStatus("error", error.getMessage());
			if(DEBUG) error.printStackTrace();
		}
    }

    protected abstract void xload() throws ClassNotFoundException, InstantiationException, IllegalAccessException;
    protected abstract void xinit();
    protected abstract void xstart();
    protected abstract void xstop();
    protected abstract void xdestroy();
    protected abstract void xdispose();
    
    protected void xExtended(int id) {
    }

    /*
    **
    */
    public void sendLoad() {
		sendEvent(CMD_LOAD);
    }
	
	public void sendInit() {
		sendEvent(CMD_INIT);
	}

    public void sendStart() {
  		sendEvent(CMD_START);
    }

    public void sendStop() {
		sendEvent(CMD_STOP);
    }
	
	public void sendDestroy() {
		sendEvent(CMD_DESTROY);
	}

	public void sendDispose() {
		sendEvent(CMD_DISPOSE);
	}

	public void startUp() {
		sendLoad();
		sendInit();
		sendStart();
	}

    public void shutdown() {
		sendStop();
		sendDestroy();
		sendDispose();
    }

    public void restart() {
		sendStop();
		sendDestroy();
		sendInit();
		sendStart();
    }

    public void reload() {
		sendStop();
		sendDestroy();
		sendDispose();
		sendLoad();
		sendInit();
		sendStart();
    }
    
    /** This function blocks until the status of ExecutionContext is DISPOSED or
     *      an Exeption occurred during a call to the AppletExecutionContext in dispatch.
     * @see #dispatch
     * @see #setStatus
     * @see #setDispatchException
     */
    public void waitForDispose() {
        if (status == DISPOSED || bDispatchException)
            return;
        else
        {
            // wait until status is disposed
            synchronized (statusLock) {
                while (status != DISPOSED && !bDispatchException) {
                    try {
                        statusLock.wait();
                    } catch (java.lang.InterruptedException e) {
                    }
                }
            }
        }
        System.err.println("exit");
    }

    protected void  setStatus( int newStatus) {
        synchronized (statusLock) {
            status= newStatus;
            statusLock.notifyAll();
        }
    }

    protected void setDispatchException() {
        synchronized (statusLock) {
            bDispatchException= true;
            statusLock.notifyAll();
        }
    }
}

