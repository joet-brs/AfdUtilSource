#define NO_SHOW  0
#define NO_CLEAR 0
#define NO_PAUSE 0
#define SHOW  1
#define CLEAR 1
#define PAUSE 1
#define SHOW_SOME 2

extern void readkey(char *);
extern void readkeys(char *);
extern void gotoyx(WORD, WORD);
extern void clrscr(void);
extern void clrhlf(void);
extern BYTE await_io(void);
extern WORD check_key(void);
extern void presscont(WORD, WORD);

 
#define printw printf

#define CR  (unsigned char) 13

