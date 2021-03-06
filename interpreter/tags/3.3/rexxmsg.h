/*
 *  The Regina Rexx Interpreter
 *  Copyright (C) 2001  Mark Hessling <M.Hessling@qut.edu.au>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * $Id: rexxmsg.h,v 1.14 2004/04/15 10:07:52 mark Exp $
 */
struct textindex
{
   unsigned int errorno;
   unsigned int suberrorno;
   unsigned int textlength;
   unsigned int fileoffset;
};

/*
 * The following number is generated by running 'msgcmp'. This number MUST
 * match the first word in the binary error message files (*.mtb), or an
 * error indicating that the error message files are corrupt will be
 * displayed instead of a real error message.
 */
#define NUMBER_ERROR_MESSAGES 266

/*
 * These defines aren't really used; they are just placekeepers
 */
#define LANGUAGE_ENGLISH     0
#define LANGUAGE_GERMAN      1
#define LANGUAGE_SPANISH     2
#define LANGUAGE_NORWEGIAN   3
#define LANGUAGE_PORTUGUESE  4
#define LANGUAGE_POLISH      5

#define LANGUAGE_MAXIMUM     6

