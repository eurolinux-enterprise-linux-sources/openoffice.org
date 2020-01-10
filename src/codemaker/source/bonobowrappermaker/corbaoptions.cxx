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

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_codemaker.hxx"
#include 	<stdio.h> 

#include	"corbaoptions.hxx"

using namespace rtl;

sal_Bool CorbaOptions::initOptions(int ac, char* av[], sal_Bool bCmdFile) 
	throw( IllegalArgument )
{
	sal_Bool 	ret = sal_True;
	sal_uInt16	i=0;

	if (!bCmdFile)
	{
		bCmdFile = sal_True;
		
		m_program = av[0];

		if (ac < 2)
		{
			fprintf(stderr, "%s", prepareHelp().getStr());
			ret = sal_False;
		}

		i = 1;
	} else
	{
		i = 0;
	}

	char	*s=NULL;
	for (i; i < ac; i++)
	{
		if (av[i][0] == '-')
		{
			switch (av[i][1])
			{
				case 'O':
					if (av[i][2] == '\0')
					{
						if (i < ac - 1 && av[i+1][0] != '-')
						{
							i++;
							s = av[i];
						} else
						{
							OString tmp("'-O', please check");
							if (i <= ac - 1)
							{
								tmp += " your input '" + OString(av[i+1]) + "'";
							}
							
							throw IllegalArgument(tmp);
						}
					} else
					{
						s = av[i] + 2;
					}
					
					m_options["-O"] = OString(s);
					break;
				case 'H':
					if (av[i][2] == '\0')
					{
						if (i < ac - 1 && av[i+1][0] != '-')
						{
							i++;
							s = av[i];
						} else
						{
							OString tmp("'-H', please check");
							if (i <= ac - 1)
							{
								tmp += " your input '" + OString(av[i+1]) + "'";
							}
							
							throw IllegalArgument(tmp);
						}
					} else
					{
						s = av[i] + 2;
					}
					
					m_options["-H"] = OString(s);
					break;
				case 'B':
					if (av[i][2] == '\0')
					{
						if (i < ac - 1 && av[i+1][0] != '-')
						{
							i++;
							s = av[i];
						} else
						{
							OString tmp("'-B', please check");
							if (i <= ac - 1)
							{
								tmp += " your input '" + OString(av[i+1]) + "'";
							}
							
							throw IllegalArgument(tmp);
						}
					} else
					{
						s = av[i] + 2;
					}
					
					m_options["-B"] = OString(s);
					break;
				case 'T':
					if (av[i][2] == '\0')
					{
						if (i < ac - 1 && av[i+1][0] != '-')
						{
							i++;
							s = av[i];
						} else
						{
							OString tmp("'-T', please check");
							if (i <= ac - 1)
							{
								tmp += " your input '" + OString(av[i+1]) + "'";
							}
							
							throw IllegalArgument(tmp);
						}
					} else
					{
						s = av[i] + 2;
					}
					
					if (m_options.count("-T") > 0)
					{
						OString tmp(m_options["-T"]);
						tmp = tmp + ";" + s;
						m_options["-T"] = tmp;
					} else
					{
						m_options["-T"] = OString(s);
					}
					break;
				case 'G':
					if (av[i][2] != '\0')
					{
						OString tmp("'-G', please check");
						if (i <= ac - 1)
						{
							tmp += " your input '" + OString(av[i]) + "'";
						}

						throw IllegalArgument(tmp);
					}
					
					m_options["-G"] = OString("");
					break;
				default:
					throw IllegalArgument("the option is unknown" + OString(av[i]));
					break;					
			}
		} else
		{
			if (av[i][0] == '@')
			{
				FILE* cmdFile = fopen(av[i]+1, "r");
		  		if( cmdFile == NULL )
      			{
					fprintf(stderr, "%s", prepareHelp().getStr());
					ret = sal_False;
				} else
				{
					int rargc=0;
					char* rargv[512];
					char  buffer[512];

					while ( fscanf(cmdFile, "%s", buffer) != EOF )
					{
						rargv[rargc]= strdup(buffer);
						rargc++;
					}
					fclose(cmdFile);
					
					ret = initOptions(rargc, rargv, bCmdFile);
					
					for (long i=0; i < rargc; i++) 
					{
						free(rargv[i]);
					}
				}		
			} else
			{
				m_inputFiles.push_back(av[i]);
			}		
		}
	}
	printf("-T: %s\n", 						m_options["-T"].getStr());

	return ret;	
}	

OString	CorbaOptions::prepareHelp()
{
	OString help("\nusing: ");
	help += m_program + " [-options] file_1 ... file_n\nOptions:\n";
	help += "    -O<file>   = file name for the generated output.\n";
	help += "                 The output directory tree is generated under this directory.\n";
	help += "    -T<name>   = name specifies a type or a list of types. The output for this\n";
	help += "      [t1;...]   type is generated. If no '-T' option is specified,\n";
	help += "                 then output for all types is generated.\n";
	help += "                 Example: 'com.sun.star.uno.XInterface' is a valid type.\n";		
	help += "    -B<name>   = name specifies the base node. All types are searched under this\n";
	help += "                 node. Default is the root '/' of the registry files.\n";
	help += "    -G         = generate only target files which does not exists.\n";
	help += "    -H<header> = include CORBA generated <header>.\n";
	help += prepareVersion();
	
	return help;
}	

OString	CorbaOptions::prepareVersion()
{
	OString version("\nSun Microsystems (R) ");
	version += m_program + " Version 2.0\n\n";

	return version;
}	

	
