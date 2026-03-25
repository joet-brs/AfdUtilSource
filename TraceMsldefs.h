/*
*****************************************************************************
**
**  Project:  General C utilities
**  
**  File:  MslDefs.h
**
**  Purpose:  Standard definitions used for C programs
**
**  History:    Date    Who  Description
**            --------  ---  ------------
**            05/05/94  Wjs  Created from some #defs in GDPLit.edf
**                               and my own
**
******************************************************************************
*/

/*
=============================================================================

                           Revision history

$Header:   D:/PROJ/COMMON/CINC/vcs/msldefs.h_v   1.7   29 Jun 1996 16:09:58   Wjs  $

$Log:   D:/PROJ/COMMON/CINC/vcs/msldefs.h_v  $
   
      Rev 1.7   29 Jun 1996 16:09:58   Wjs
   > slightly modified the SEGMENTOF and OFFSETOF macros and changed their
       names to SEGMENT_OF and OFFSET_OF to make them more readable.
   
      Rev 1.6   18 Feb 1996 22:38:42   Wjs
   > added generic STR_BUF structure template
   > added PTR_OVERLAY struct/typedef
   > added B_NULL for specifying null BYTE pointers
   
      Rev 1.5   04 Feb 1996 22:57:52   Wjs
   > added STR_CONST and STR_BUF string structure types
   > added CTOS file open mode macros
   > bracketted some of the defines and the structure types with compile flags
   > added V_NULL for specifying a null void pointer.
   > added STREQU for comparing strings
   > added F_NULL for comparing FILE pointers to null (0)
   > added E_OK so programs wouldn't have to include cterc.h to get it.
   
      Rev 1.4   07 Sep 1995 14:30:52   Wjs
   > converted stuff for compiling with CTOS MSC 6.2
   > changed to new standard for PUBLIC and PRIVATE, PRIVATE only means static
       when using Lint for intermodule cross-referencing
   
      Rev 1.3   12 Jul 1995 12:51:32   Wjs
   > added DO_FOREVER macro 
   
      Rev 1.2   09 Jan 1995 04:49:34   Wjs
   added some more defines and some data manipulation macros 
   
      Rev 1.1   24 May 1994 12:30:20   Wjs
   added #define for S_FSPEC. 
   
      Rev 1.0   17 May 1994 13:07:24   Wjs
   Initial checkin.

=============================================================================
*/

/* additional data types */ 

#if !defined(__BYTE)
typedef unsigned char       BYTE;
#define __BYTE
#endif 

#if !defined(__FLAG)
typedef unsigned char       FLAG;
#define __FLAG
#endif 

#if !defined(__WORD)
typedef unsigned int        WORD;
#define __WORD
#endif 

#if !defined(__ERC_T)
typedef unsigned int        ERC_T;
#define __ERC_T
#endif 

#if !defined(__BOOL)
typedef unsigned int        BOOL;
#define __BOOL
#endif 

#if !defined(__DWORD)
typedef unsigned long       DWORD;
#define __DWORD
#endif 

#define __SB_STRINGS


#define UI      unsigned int
#define UC      unsigned char
#define UL      unsigned long
#define DBL     double


/* General 'C' defines */

/*
** Scope declarators:  PUBLIC and PRIVATE are used to define the visibility
** (scope) of functions and data as being either globally visible (PUBLIC)
** or visible only to the module being compiled (PRIVATE).  PRIVATE
** is analogous to "static" with the exception that "static" causes a symbol
** to be removed from the CTOS symbol file (.sym is publics only).  When
** compiling a module, PRIVATE is #define'd as nothing (a null #define) which
** causes the symbol to default to public.  When "linting" the code, PRIVATE
** is #define'd as "static" to allow Lint to do its scope checking.  Lint
** checks all references to a symbol, including intermodule references in a
** multi-module program and identifies any public symbols whose scope is
** limited to 1 module and so can be made static.
*/
 
#ifndef PUBLIC
#define PUBLIC
#endif

#ifndef PRIVATE

#ifdef _lint
#define PRIVATE         static
#else
#define PRIVATE
#endif
#endif

#ifndef E_OK
#define E_OK            0
#endif

#ifndef NULL
#define NULL            (char *)0
#endif

#ifndef V_NULL
#define V_NULL          (void *)0
#endif

#ifndef B_NULL
#define B_NULL          (BYTE *)0
#endif

#ifndef F_NULL
#define F_NULL          (FILE *)0
#endif

#ifndef  L_NULL
#define  L_NULL         0L
#endif

#ifndef TRUE
#define TRUE            1
#endif

#ifndef FALSE
#define FALSE           0
#endif

#ifndef SPACE
#define SPACE           0x20
#endif

#ifndef  NEW_LINE
#define  NEW_LINE       '\n'
#endif

#ifndef  BACKSLASH
#define  BACKSLASH      '\\'
#endif

/* size of filespec is 12 each for node, vol, dir, pswd = 48 + 50 for 
    filename + 4 for []<> + 1 for ^(pswd) = 103 + 1 for length or
    null terminator = 104 */

#define S_FSPEC         104

#define FOREVER         for (;;)
#define DO_FOREVER      for (;;)
#define ONES            0xFFFF
#define ALLFS           0xFFFF

#ifdef __OPEN_MODES

/* CTOS file open modes.  'TRX...' modes are transaction modes for ISAM
    added 12/20/95, Wjs */

#define TRXREAD         (('t' << 8) | 'r')
#define TRXMODIFY       (('t' << 8) | 'm')
#define MODEREAD        (('m' << 8) | 'r')
#define MODEWRITE       (('m' << 8) | 'w')
#define MODEAPPEND      (('m' << 8) | 'a')
#define MODEMODIFY      (('m' << 8) | 'm')

#undef __OPEN_MODES
#endif

/* some generally useful macros */ 

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: ARRAY_LEN returns the number of elements in a single-dimension array.
::  usage: n = ARRAY_LEN(AnyArray); 
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#ifndef  ARRAY_LEN
#define  ARRAY_LEN(a)   sizeof((a))/sizeof((a)[0])
#endif

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: returns the low byte of a word 
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#ifndef LOBYTE
#define LOBYTE(w)       (BYTE)((w) & 0x00FF)
#endif

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: returns the high byte of a word
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#ifndef HIBYTE
#define HIBYTE(w)       (BYTE)(((w) & 0xFF00) >> 8)
#endif

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: returns the low nybble of a byte 
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#ifndef  LONYBL    
#define  LONYBL(b)      ((b) & 0x0F)
#endif

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: returns the high nybble of a byte 
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#ifndef  HINYBL    
#define  HINYBL(b)      (((b) & 0xF0) >> 4)
#endif

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: returns the segment portion of a 32-bit pointer 
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#ifndef SEGMENT_OF
#define SEGMENT_OF(fp) ((unsigned)((unsigned long)(fp) >> 16))
#endif

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: returns the offset portion of a 32-bit pointer 
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#ifndef OFFSET_OF
#define OFFSET_OF(fp)  ((unsigned)((unsigned long)(fp)))
#endif

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: fast check for string EQUALITY
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#ifndef STREQU
#define STREQU(p1,p2,c) ((*(p1) == *(p2)) ? (ULCmpB((p1),(p2),(c)) == 0xFFFF) : 0)
#endif


/*::::::::::::::::::::::::::::::::::::::::
:: Standard structure typedefs
::::::::::::::::::::::::::::::::::::::::*/

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: error code to error message map structure.  the actual error map is
:: usually implemented as an array of these structures.
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#ifdef __ERC_MAP

typedef struct T_ERC_MAP                    /* error code/msg map table */
   {
   WORD         Erc;                        /* error code */
   char *       pszErcMsg;                  /* ptr to error msg string */
   } ERC_MAP;

#undef __ERC_MAP
#endif

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: structures for accessing the selector and offset components of a pointer
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#ifdef __PTR_STRUCT

typedef struct T_PTR_STRUCT                 /* struct for overlaying ptrs */
   {
   WORD         Offset;                     /* offset portion */
   WORD         Selector;                   /* selector/segment */
   } PTR_STRUCT;

typedef union
    {
    void FARPTR Ptr;                        /* a 32-bit far pointer */
    PTR_STRUCT  PtrStruct;                  /* overlayed with seg/offset */
    } PTR_OVERLAY;

#undef __PTR_STRUCT
#endif

#ifdef __SB_STRINGS

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: add 'sb' string structure types; 12/20/95, Wjs.  these structures
:: implement CTOS 'sb' strings as a length/string structure.
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

struct T_STR_CONST                          /* 'sb' type string CONSTANT */
    {
    BYTE    cb;
    char    *pStr;
    };
typedef const struct T_STR_CONST STR_CONST;

struct T_STR_BUF                            /* generic 'sb' string BUFFER */
    {                                       /* for passing ptr to sbstring */
    BYTE    cb;                             /* of indeterminate size */
    BYTE    Buf[1];
    };
typedef struct T_STR_BUF STR_BUF;

struct T_STR_BUF_12                         /* 12 char 'sb' string BUFFER */
    {
    BYTE    cb;
    BYTE    Buf[12];
    };
typedef struct T_STR_BUF_12 STR_BUF_12;

struct T_STR_BUF_20                         /* 20 char 'sb' string BUFFER */
    {
    BYTE    cb;
    BYTE    Buf[20];
    };
typedef struct T_STR_BUF_20 STR_BUF_20;

struct T_STR_BUF_40                         /* 40 char 'sb' string BUFFER */
    {
    BYTE    cb;
    BYTE    Buf[40];
    };
typedef struct T_STR_BUF_40 STR_BUF_40;

struct T_STR_BUF_80                         /* 80 char 'sb' string BUFFER */
    {
    BYTE    cb;
    BYTE    Buf[80];
    };
typedef struct T_STR_BUF_80 STR_BUF_80;

#undef __SB_STRINGS
#endif
