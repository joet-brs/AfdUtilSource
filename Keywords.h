/* 
****************************************************************************
**
** Project:  CTOS utilities      
**
** File: KeyWords.h
** 
** Purpose:  defines some keywords that are different between the MSDOS and
**          CTOS MSC compilers.
** 
** Special Considerations: 
**  
** Author:  W. J. Sappington - Momentum Systems Limited
**  
** History: Date      Who   Description
**          ---------------------------
**          05/17/94, Wjs - Created
**  
**  
****************************************************************************  
*/

/*
=============================================================================

                           Revision history

$Header:   D:/PROJ/COMMON/CINC/vcs/keywords.h_v   1.3   18 Feb 1996 22:38:42   Wjs  $

$Log:   D:/PROJ/COMMON/CINC/vcs/keywords.h_v  $
   
      Rev 1.3   18 Feb 1996 22:38:42   Wjs
   > added NEARPTR keyword
   
      Rev 1.2   04 Feb 1996 22:57:52   Wjs
   > added FASTCALL keyword 
   
      Rev 1.1   07 Sep 1995 14:30:52   Wjs
   > converted keywords for compiling with CTOS MSC 6.2
   
      Rev 1.0   17 May 1994 09:48:32   Wjs
   Initial checkin.  As copied from a utility directory

=============================================================================
*/

#if !defined(__KEYWORDS)
#define  __KEYWORDS

#if defined (__MSDOS__)                     /* MsDos MSC 6.0 keywords */
#define FAR             _far

#if defined (_lint)
#define PLM
#else
#define PLM             _pascal
#endif

#define NEARPTR         _near *
#define FARPTR          _far *
#define FASTCALL        _fastcall

#elif defined (__CTOS__)                     /* CTOS MSC 6.2 keywords */
#define FAR             _far
#define PLM             _plm
#define NEARPTR         _near *
#define FARPTR          _far *
#define FASTCALL        _fastcall

#endif                                      /* if defined __MSDOS__ */
#endif                                      /* if !defined __KEYWORDS */
