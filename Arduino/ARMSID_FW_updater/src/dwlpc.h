// API ////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include <string.h>

#define BLUE    1
#define RED     2
#define LIGHTGREEN 12
#define YELLOW  14
#define WHITE   15

extern int arm2sid;

static void textcolor(int c) { ; }
static void textbackground(int c) { ; }
#define cprintf printf
#define clrscr() printf("\r\n========================================\r\n");

// ------------------------ ASCII letters constants ----------------------------

#define CHAR_A	'A'
#define CHAR_B	'B'
#define CHAR_C	'C'
#define CHAR_D	'D'
#define CHAR_E	'E'
#define CHAR_F	'F'
#define CHAR_G	'G'
#define CHAR_H	'H'
#define CHAR_I	'I'
#define CHAR_J	'J'
#define CHAR_K	'K'
#define CHAR_L	'L'
#define CHAR_M	'M'
#define CHAR_N	'N'
#define CHAR_O	'O'
#define CHAR_P	'P'
#define CHAR_Q	'Q'
#define CHAR_R	'R'
#define CHAR_S	'S'
#define CHAR_T	'T'
#define CHAR_U	'U'
#define CHAR_V	'V'
#define CHAR_W	'W'
#define CHAR_X	'X'
#define CHAR_Y	'Y'
#define CHAR_Z	'Z'

// -------------------------- ARMSID addressing --------------------------------

static int SIDs[2]={0x0,0x20};

volatile int SIDaddr;

void SIDsetaddr(int i)
{
	SIDaddr=SIDs[i];
}

int SIDgetaddr(void)
{
	return SIDaddr;
}

extern void SIDwrite(int, unsigned char);
extern unsigned char  SIDread(int);

void SIDwr(unsigned char reg, unsigned char value)
{
	SIDwrite(SIDaddr+reg,value);
}

unsigned char SIDrd(unsigned char reg)
{
	return SIDread(SIDaddr+reg);
}

// ------------------------- GRAFIK AND TIMING ---------------------------------

void delay(unsigned int milliseconds);

// wait for at least 100us
#define delay100us() delay_ms(1)
#define delay0  delay(0)

// no border color
#define bordercolor(a)	(0)

// background color of the text
static int c_mem=BLUE;
void bgcolor(int c) {
	textbackground((c)&7);
	c_mem=c;
}

#define COLOR_BLUE BLUE
#define COLOR_LIGHTBLUE LIGHTBLUE
#define COLOR_GREEN GREEN
#define COLOR_LIGHTGREEN LIGHTGREEN
#define COLOR_RED RED
#define COLOR_LIGHTRED LIGHTRED
#define COLOR_YELLOW YELLOW
#define COLOR_WHITE WHITE

static int kurzor_x=0;

#define cputc_(a) cprintf("%c",a)
void cputc(char ch) {
    if (ch=='^') {
        if (arm2sid) printf("ARM2SID");
        else printf("ARMSID");
        return;
    }
    if (ch=='\b') kurzor_x--;
    else kurzor_x++;
	if ((ch>='a')&&(ch<='z')) cputc_(ch+'A'-'a');
    else cputc_(ch);
}

void cputs(const char *s) {
    while(*s) {
        cputc(*s);
        s++;
    }
}

// inverse color of the text
void revers(int a) {
	if (a==1) {
		textcolor(c_mem);
		textbackground(WHITE);
	} else {
		textcolor(WHITE);
		bgcolor(c_mem);
	}
}

// print color inversed character
void cputc_revers(char ch) {
        revers(1);
        cputc(ch);
        revers(0);
}

// x,y coordinates start from 0 on the top left in C64 (from 1 in DOS)
#define wherex() kurzor_x
#define wherey() 0
#define gotoxy(a,b) do { (void)b; while (kurzor_x>(a)) cputc('\b'); } while (0)

// ------------------------------ KEYBOARD -------------------------------------

// return non zero when a key is pressed
int keypressed(void)
{
    return _kbhit();
}

// reads the key code of the pressed key from buffer
int rkey(void)
{
    int res=_getch();
    return res;
}

// reads the key and convert to C64 cursor keys
int getkey(void)
{
int key;
 while (!keypressed());
 {
	 key=rkey();
	 if (key==0x4800) key=17+128;
	 if (key==0x4B00) key=29+128;
	 if (key==0x4D00) key=29;
	 if (key==0x5000) key=17;
	 if (key&0xFF) return key&0xFF; else return key;
 }
}

// a pressed key reading
#define cgetc()	getkey()

////////////////////////////////////////////////////////////////////////////////

