/* -*- Mode: C++; tab-width: 4 -*- 
 *
 *  cplusplus.cc --- C++ main for compilers/linkers/link formats that require it.
 *
 *  This file is required when you are linking in C++ code. Some older C++
 *  linkers (HP, COFF I think, etc) require main() to be in a C++ file for
 *  the C++ runtime loader to be linked in or invoked. This file does nothing
 *  but make a call to to the real main in mozilla.c, but it will link the C++
 *  runtime loader.
 *
 *  Copyright © 1996 Netscape Communications Corporation, all rights reserved.
 *  Created: David Williams <djw@netscape.com>, May-26-1996
 *
 *  RCSID: "$Id: cplusplusmain.cc,v 1.1 1996/05/27 05:42:21 djw Exp $"
 */
extern "C" int mozilla_main(int argc, char** argv);

int
main(int argc, char** argv)
{
	return mozilla_main(argc, argv);
}
