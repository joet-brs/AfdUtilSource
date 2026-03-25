#include <windows.h>
#include <windef.h>
#include <wincon.h>
#include <conio.h>
#include <stdio.h>
#include "CommHoldVidio.h"

HANDLE hStdIn;       /* standard input */
HANDLE hStdOut;      /* standard output */
COORD coord;  

/* ************************************************************************* */
/*  clear screen                                                             */
  
void clrscr()
{
   COORD coordScreen = { 0, 0 }; /* here's where we'll home the cursor */
   DWORD cCharsWritten;
   CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */
   DWORD dwConSize; /* number of character cells in the current buffer */

   /* get the number of character cells in the current buffer */
   GetConsoleScreenBufferInfo(hStdOut, &csbi);
   dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
   /* fill the entire screen with blanks */
   FillConsoleOutputCharacter(hStdOut, (TCHAR) ' ',
                              dwConSize, coordScreen, &cCharsWritten);
   /* get the current text attribute */
   GetConsoleScreenBufferInfo(hStdOut, &csbi);
   /* now set the buffer's attributes accordingly */
   FillConsoleOutputAttribute(hStdOut, csbi.wAttributes,
                              dwConSize, coordScreen, &cCharsWritten);
}
  
/* ************************************************************************* */
/*  position cursor                                                          */
  
void gotoyx(WORD x, WORD y)
{
   coord.X = y;
   coord.Y = x;	
   SetConsoleCursorPosition(hStdOut, coord);
}
  
/* ************************************************************************* */
/*  read a single key from keyboard                                          */
  
void readkey(char *key)
{
   *key = '\n';
   *key = getch();
}
  
/* ************************************************************************* */
/*  read a string from keyboard                                              */
  
void readkeys(char keys[])
{
   gets(keys);
}
  
/* ************************************************************************* */
/*  check for <Esc> or <Enter> key press                                     */
  
WORD check_key()
{

   return(0);
}
  
/* ************************************************************************* */
/*  display prompt and await key press                                       */
  
void presscont(WORD x, WORD y)
{
   char chr[2];
  
   gotoyx(x, y); printf("Press any key to continue");
   readkey(chr);
}
  
 
/* ************************************************************************* */
/*  clear right half of screen                                               */
  
void clrhlf()
{
   WORD i;
  
   for (i = 4; i < 24; i++)
   {
      gotoyx(i, 40);
      printf("                                      ");
   }
}
  