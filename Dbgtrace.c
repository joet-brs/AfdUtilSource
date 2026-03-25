/*
****************************************************************************
**
** Project:  Development utilities
**
** Module:  DbgTrace.c
** 
** Purpose:  Contains all "debug trace" functions
** 
** Functions:  
**      GLOBAL WORD PLM CloseDiskTrace (void);
**      GLOBAL WORD PLM FlushDiskTrace (void);
**      GLOBAL WORD PLM OpenDiskTrace (char FARPTR pFSpec, WORD cbFSpec);
**      GLOBAL void PLM SetDfltTraceFile (char FARPTR pFSpec, WORD cbFSpec);
**      GLOBAL WORD PLM SetTraceLevel (BYTE NewLevel);
**      GLOBAL void PLM InitDbgTrace (void);
**      GLOBAL void PLM TraceWord (BYTE TraceCode, WORD wVal);
**      GLOBAL void PLM DbgTrace (BYTE TraceCode);
**  
** Author:  W. J. Sappington
**  
** History: 10/20/84, Wjs - Created, originally in PL/M
**          06/06/89, Wjs - Converted from PL/M to C
**          01/15/92, Wjs - Changed OpenDiskTrace to take filename arg
**          01/30/92, Wjs - Frozen as the version that will be installed
**                          on the live system 
**          09/11/92, Wjs - released to Sunshine as V2.0.0
**          06/14/94, Wjs - added stuff to allow the debug trace to be
**                          controlled from the debugger by changing the
**                          value of "DbgLvl".
**          06/17/94, Wjs - added function SetDfltTraceFile() so programs
**                          can set their own default debug trace file. 
****************************************************************************  
*/
/*
=============================================================================

                           Revision history

$Header:   D:/PROJ/CTUTILS/vcs/dbgtrace.c_v   1.1   31 Mar 1996 12:20:08   Wjs  $

$Log:   D:/PROJ/CTUTILS/vcs/dbgtrace.c_v  $
   
      Rev 1.1   31 Mar 1996 12:20:08   Wjs
   > made the trace code pointer (pTCodes) in TraceFnc a static because as
       an automatic it was being assigned as a register variable and then the
       optimizer would screw up (whadaya 'spect, it's Microsoft) and not
       restore the register (SI) after the call to the CTOS function PutByte().
   
      Rev 1.0   09 Mar 1996 11:37:16   Wjs
   > initial checkin as a library object. 

============================================================================
                           Previous history

      Rev 1.2   17 Feb 1996 23:44:54   Wjs
   > removed the TraceErc() function.  Identical to TraceWord()
   
      Rev 1.1   14 Feb 1996 16:17:14   Wjs
   > changed the switch (DbgLvl) in DbgTrace() to a call to SetTraceLevel()
   > also added the TraceWord() and TraceErc() functions to reduce the
       multiple calls to DbgTrace() necessary to trace word values
   > made the functions PLM callable so this module can be called from PLM

     Rev 1.0   18 Jan 1996 10:39:38   Wjs
   > initial checkin.  same as other current dbgtrace modules except standard
       C stream I/O added for use in a DOS executable.  CTOS and DOS disk I/O
       controlled by compile flags CTOS_RUN and DOS_EXE.

=============================================================================
*/

/*::::::::::::::::::::::::::::::::::::::::
:: General Includes/defines
::::::::::::::::::::::::::::::::::::::::*/
/*
defs()
*/

/*
** set up portability keywords and bring in MSL's standard defs and typedefs
** first so they take precedence.
**
** Force compilation for CTOS target since CPE will never be a DOS exe
*/
 
#define   __DOS_EXE
#define   PLM 
#define   PUBLIC 
#define   PRIVATE

#include <keywords.h>

#define __OPEN_MODES
#define __SB_STRINGS
#include <Tracemsldefs.h>
#include <string.h>



#include <stdio.h>

#define TRACE_SIZE  2048

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: GLOBAL data and functions (publics and externs) specific to DbgTrace
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

/*
** trace macro definitions and public functions and data:
*/

#define fmemcpy	  memcpy
#define DbgTrace
#define PUBLIC_DATA
#include <ctutils.h>
#undef PUBLIC_DATA

/*::::::::::::::::::::::::::::::::::::::
:: PRIVATE (local) function prototypes
::::::::::::::::::::::::::::::::::::::*/

PRIVATE void PLM TraceFnc (BYTE nBytes);


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::
::                  Module scope data definitions
::    Everything beyond this point is "visible" only to this module.
::
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*
vars()
*/

PRIVATE WORD
     iTrace                                 /*Trace index */
    ,iTraceMax                              /*Public value for max index */
    ,sTrace                                 /*Public trace size value */
    ;

PRIVATE BYTE
     DbgLvl = 0                             /* debug level */
    ,fDiskTrace = FALSE                     /* flag for logging to disk */
    ,SaveDbgLvl = 0                         /* for detecting DbgLvl change */
    ;

PRIVATE BYTE
     TCodes [8]                             /* tmp storage for multi-codes */
    ,Trace [TRACE_SIZE]                     /*The trace array */
    ;

PRIVATE STR_BUF_80
     sbSaveSpec                             /* saved trace filespec */
    ;



PRIVATE FILE
     *pTraceFile
    ;
STR_CONST
     scDfltSpec =                           /* default log file spec */
        {11, "dbgtrace.log"}
    ;

                             /* end if msdos/ctos */


/*
**************************************************************************
**   
**  Function:  CloseDiskTrace 
**   
**  Purpose:  closes the disk file used for logging trace codes
**   
**  Interface:  Erc = CloseDiskTrace;
**   
**  Author: W. J. Sappington  
**   
**  History:   Date    Who   Description
**           10/20/84, Wjs - Created 
**           04/10/90, Wjs - converted to C 
**   
*************************************************************************
*/
PUBLIC WORD PLM CloseDiskTrace (void)
    {
    WORD     Erc = 0
            ;

    if (fDiskTrace)                         /* if disk trl enabled */
        {
#if defined (__DOS_EXE)
        fclose(pTraceFile);
#elif defined (__CTOS_RUN)
        Erc = CloseByteStream(TrcBswa);     /* close the file */
#endif
        fDiskTrace = FALSE;                 /* disable disk logging */
        }

    DbgLvl = SaveDbgLvl = 1;

    return Erc;
    }                                       /* end CloseDiskTrace */

/*
**************************************************************************
**   
**  Function:  FlushDiskTrace  (for buffered filesystems only)
**   
**  Purpose:  flushes filesystem buffers to disk
**   
**  Interface:  Erc = FlushDiskTrace();
**   
**  Author: W. J. Sappington  
**   
**  History:   Date    Who   Description
**           10/20/84, Wjs - Created
**           04/10/90, Wjs - Converted to C
**   
*************************************************************************
*/
PUBLIC WORD PLM FlushDiskTrace (void)
    {
    WORD       Erc = 0;

    if (fDiskTrace)                         /* if disktrl enabled */
        {
#if defined (__DOS_EXE)
        if (fflush(pTraceFile) != 0)
            CloseDiskTrace();
#elif defined (__CTOS_RUN)
        Erc = CheckpointBs(TrcBswa);        /* flush buffers */
        if (Erc != E_OK)                    /* if error */
            CloseDiskTrace();               /* close the trace */
#endif
        }

    return Erc;
    }                                       /* end FlushDiskTrace */

/*
**************************************************************************
**   
**  Function:  OpenDiskTrace 
**   
**  Purpose:  opens a disk file for logging trace values
**   
**  Interface:  Erc = OpenDiskTrace(pTraceFile, cbTraceFile);
**
**  Notes:  the filespec pointer must be a far pointer if this function
**      is to be called from PLM.  In PLM all data pointers are far pointers.
**   
**  Author: W. J. Sappington  
**   
**  History:   Date    Who   Description
**           10/20/84, Wjs - Created 
**           04/10/90, Wjs - Converted to C
**   
*************************************************************************
*/
PUBLIC WORD PLM OpenDiskTrace (char FARPTR pFSpec, WORD cbFSpec)
    {
    WORD     Erc
            ;

    /* if no trace spec provided, use the default.  otherwise, use what
        was passed in and save what we use for re-opening later. Slap
        a null on the saved string for passing to fopen(). */ 

    if ((pFSpec == (char FARPTR)0) || (cbFSpec == 0))
        {
        sbSaveSpec.cb = scDfltSpec.cb;
        memcpy(sbSaveSpec.Buf, scDfltSpec.pStr, scDfltSpec.cb);
        }
    else
        {
        sbSaveSpec.cb = (BYTE)cbFSpec;
        fmemcpy(sbSaveSpec.Buf, pFSpec, cbFSpec);
        }
    sbSaveSpec.Buf[sbSaveSpec.cb] = 0;

    /* open it and set flags accordingly */ 


    pTraceFile = fopen((char *)sbSaveSpec.Buf, "wb");
    if (pTraceFile == (FILE *)0)
        {
        fDiskTrace = FALSE;                 /* disable disk logging */
        Erc = ALLFS;                        /* return an error */
        }


    else                                    /* open went ok */
        {
        fDiskTrace = TRUE;                  /* enable disk logging */
        Erc = 0;                            /* return ok */
        }

    if (fDiskTrace)
        DbgLvl = SaveDbgLvl = 2;

    return (Erc);
    }                                       /* end OpenDiskTrace */


/*
**************************************************************************
**  
**  Function: SetDfltTraceFile
**  
**  Purpose: sets up the default trace filespec to be used when the trace 
**      is enabled with the DbgLvl variable
**  
**  Notes:  the filespec pointer must be a far pointer if this function
**      is to be called from PLM.  In PLM all data pointers are far pointers.
**   
**  Author: W. J. Sappington
**   
**  History:   Date    Who   Description
**           -----------------------------
**           06/17/94, Wjs - Created 
**   
****************************************************************************
*/

PUBLIC void PLM SetDfltTraceFile (char FARPTR pFSpec, WORD cbFSpec)
    {
    
    sbSaveSpec.cb = (BYTE)cbFSpec;
    fmemcpy(sbSaveSpec.Buf, pFSpec, cbFSpec);

    return;
    }


/*
**************************************************************************
**  
**  Function: SetTraceLevel
**  
**  Purpose: sets the trace to the specified level
**  
**  Author: W. J. Sappington
**   
**  History:   Date    Who   Description
**           -----------------------------
**           01/04/95, Wjs - Created 
**   
****************************************************************************
*/

PUBLIC WORD PLM SetTraceLevel (BYTE NewLevel)
    {
    WORD    Erc = 0;

    if (NewLevel > 3)
        NewLevel = 0;
    switch (NewLevel)
        {
        case 0:
            /* case 0, shut down all trace stuff */

            Erc = CloseDiskTrace();         /* Close the disk trace */
            fDbgTrace = FALSE;              /* Disable the trace */
            break;

        case 1:
            /* Case 1, in-memory trace only */

            if (SaveDbgLvl == 0)            /* previously disabled */
                InitDbgTrace();
            else                            /* currently at level 1 or 2 */
                Erc = CloseDiskTrace();     /* make sure it's closed */
            fDbgTrace = TRUE;
            break;

        case 2:
            /* Case 2, memory & disk trace. InitDbgTrace() to initialize and
                enable it, open the disk trace if it's not already */

            InitDbgTrace();          
            if (!fDiskTrace)
                {
                Erc = OpenDiskTrace((char *)sbSaveSpec.Buf, sbSaveSpec.cb);
                if (Erc !=  0)
                    DbgLvl = 1;
                }
            fDbgTrace = TRUE;
            break;

        case 3:
            /* case 3, reset the trace, enable for memory only */ 

            Erc = CloseDiskTrace();
            InitDbgTrace();
            NewLevel = 1;                   /* make sure level 1 is set */
            break;
        }                                   /* end switch */
    DbgLvl = SaveDbgLvl = NewLevel;         /* save the new level */

    return (Erc);
    }                                       /* end function */


/*
**************************************************************************
**   
**  Function:  InitDbgTrace 
**   
**  Purpose:  initializes the debug trace
**   
**  Interface:  InitDbgTrace (); 
**   
**  Author: W. J. Sappington  
**   
**  History:   Date    Who   Description
**           06/06/89, Wjs - Created 
**   
**   
*************************************************************************
*/

PUBLIC void PLM InitDbgTrace (void)
    {

    iTrace = 0;                             /* init the index */
    iTraceMax = (TRACE_SIZE-1);             /*Public value for max index */
    sTrace    = TRACE_SIZE;                 /*Public trace size value */
    memset(Trace,0xFF,sTrace);              /* init trace to FF's */

    fDbgTrace = TRUE;
    fDiskTrace = FALSE;                     /*Flag for logging to disk */

    DbgLvl = SaveDbgLvl = 1;

    return;
    }                                        /* end InitDbgTrace */

/*
**************************************************************************
**   
**  Function:  TraceFnc
**   
**  Purpose:  stores values in the debug trace.
**   
**  Interface:  TraceFnc(Code);
**   
**       Params:  Code  - code indicating whether to store location only,
**                      a word trace value, or a location and a value
**       Returns: nothing
**   
**  Author: W. J. Sappington  
**   
**  History:   Date    Who   Description
**           -------------------------------
**           01/17/96, Wjs - Created 
**   
**   
*************************************************************************
*/

PRIVATE void PLM TraceFnc (BYTE nBytes)
    {
    static BYTE  *pTCodes
                ;

#if defined (__CTOS_RUN)
    WORD     Erc = E_OK
            ;
#endif

    /* if debug level has changed, respond accordingly */ 

    if (DbgLvl != SaveDbgLvl)
        SetTraceLevel(DbgLvl);

    /* log the values sent with this call.  won't hurt if trace is disabled */ 

    pTCodes = TCodes;
    while (nBytes-- > 0)
        {
        /* insert call to disable interrupts here    Do not disturb!     */

        iTrace = ++iTrace & iTraceMax;      /* increment index, wrap to 0 */
        Trace[iTrace] = *pTCodes;           /* store the trace code */

        /* insert call to enable interrupts here     Ok to interrupt now. */

        if (fDiskTrace)                     /* if logging to disk */
            {
#if defined (__DOS_EXE)
            fputc(*pTCodes, pTraceFile);    /* write value to disk */
            if (ferror(pTraceFile))         /* if write failed  */
                CloseDiskTrace();           /* shut off disk logging */

#elif defined (__CTOS_RUN)
            Erc = WriteByte(TrcBswa,*pTCodes);  /* write value to disk */
            if (Erc != E_OK)                /* if write failed */
                CloseDiskTrace();           /* shut off disk logging */
#endif
            }                               /* end if fdisktrace */
        pTCodes += 1;
        }                                   /* end while */

    return;
    }                                       /* end TraceFnc */


/*
**************************************************************************
**  
**  Function: TraceWord
**  
**  Purpose: traces a location and a word value to go with it.
**
****************************************************************************
*/

PUBLIC void PLM TraceWord (BYTE TraceCode, WORD wVal)
    {

    TCodes[0] = TraceCode;
    *(WORD *)&TCodes[1] = wVal;
    TraceFnc(3);
                   
    return;
    }


/*
**************************************************************************
**   
**  Function:  DbgTrace
**   
**  Purpose:  stores values in the debug trace.
**            Maintains an in-memory trace of markers and data so that the  
**            execution path may be easily deciphered.  Useful for debugging
**            Also allows logging the trace to disk or CRT under control    
**            of the main program.
**   
**  Interface:  DbgTrace(value);
**   
**       Params:  value (BYTE or WORD) - the value to be stored in the trace
**       Returns: nothing
**   
**  Special Considerations: 
**       The trace must be declared as a public array in this module 
**       by including the file "dbgtrace.h".
**   
**  Author: W. J. Sappington  
**   
**  History:   Date    Who   Description
**           -------------------------------
**           10/20/84, Wjs - Created 
**   
**   
*************************************************************************
*/

PUBLIC void PLM DbgTrace (BYTE TraceCode)
    {

    TCodes[0] = TraceCode;
    TraceFnc(1);

    return;
    }                                       /* end DbgTrace */



