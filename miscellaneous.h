/*
 *  ============================================================================= 
 *  ALADDIN Version 1.0 :
 *      miscellaneous.h : Header file for Miscellaneous Routines
 *                                                                     
 *  Copyright (C) 1995 by Mark Austin, Xiaoguang Chen, and Wane-Jang Lin
 *  Institute for Systems Research,                                           
 *  University of Maryland, College Park, MD 20742                                   
 *                                                                     
 *  This software is provided "as is" without express or implied warranty.
 *  Permission is granted to use this software for any on any computer system
 *  and to redistribute it freely, subject to the following restrictions:
 * 
 *  1. The authors are not responsible for the consequences of use of
 *     this software, even if they arise from defects in the software.
 *  2. The origin of this software must not be misrepresented, either
 *     by explicit claim or by omission.
 *  3. Altered versions must be plainly marked as such, and must not
 *     be misrepresented as being the original software.
 *  4. This notice is to remain intact.
 *                                                                    
 *  Written by: Mark Austin                                         December 1995
 *  ============================================================================= 
 */

#ifndef MISCELLANEOUS_H
#define MISCELLANEOUS_H

/* Miscellaneous Functions */

#ifdef __STDC__

extern char   *MyMalloc( unsigned int );
extern char   *MyCalloc( unsigned int, unsigned int );
extern char   *SaveString( char * );
extern void    FatalError( char * , ... );

#else  /* case not STDC */

extern char    *MyMalloc();
extern char    *MyCalloc();
extern char    *SaveString();
extern void    FatalError();

#endif /* end case STDC */

#endif /* end case MISCELLANEOUS_H */