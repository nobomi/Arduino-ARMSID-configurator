/*
** ARMxSID Arduino updater
**
*/

#include <stdlib.h>
#include <stdio.h>
#include "dwlpc.h"

/*****************************************************************************/
/*                                   Data                                    */
/*****************************************************************************/

extern unsigned short FWlen;
extern unsigned char FWdata[];
extern unsigned char FWver;
extern unsigned char FWrev;
extern unsigned char FWbeta;

#ifdef BLIND
static const char Nadpis [] = "\r\n  nobomi ^ blind update to v";
#else
static const char Nadpis [] = "\r\n     nobomi ^ update to v";
#endif
static const char Notfou [] = "^ not found";
static const char Founda [] = "^ found in arduino";

/*****************************************************************************/
/*                                   Code                                    */
/*****************************************************************************/

//static const int SIDs[5]={0xD400,0xD420,0xD500,0xDE00,0xDF00};
//volatile unsigned char *SIDaddr;

void cputbval(unsigned char x) {
	char sto=0;
	char deset=0;
	if (x>=100) {
		do {
			x-=100;sto++;
		} while (x>=100);
		cputc(sto+'0');
	}
	while (x>=10) {
		x-=10;deset++;
	}
	if (sto|deset) cputc(deset+'0');
	cputc(x+'0');
}

void cput1000(unsigned short x) {
	char t=0;
	char m;
	while (x>=1000) {
		x-=1000;t++;
	}
	cputbval(t);
	cputc('.');
	t='0';while (x>=100) { x-=100;t++; } cputc(t);
	m=x;
	t='0';while (m>=10) { m-=10;t++; } cputc(t);
	cputc(m+'0');
}

void cputnibble(unsigned char x) {
	x&=15;
	if(x>9) x=('a'-10)+x;
	else x+='0';
	cputc(x);
}

void cputhex(unsigned short x) {
//	cputc('$');
	cputnibble(x>>12);
	cputnibble(x>>8);
	cputnibble(x>>4);
	cputnibble(x);
}

void uvod() {
    (void) textcolor (COLOR_WHITE);
    (void) bordercolor (COLOR_LIGHTBLUE);
    (void) bgcolor (COLOR_BLUE);
    /* Clear the screen, put cursor in upper left corner */
    clrscr ();
    cputs (Nadpis);
		cputbval(FWver);
	  cputc('.');
	  cputbval(FWrev);
		if (FWbeta) {
			cputc('b');
			cputbval(FWbeta);
		}
    cputs ("\r\n\r\n");
}

char vyber_sid(char i) {
    return 1;
}

int najdi2() {
	  cputs(Founda);
//	  cputhex((int)SIDaddr);
	  cputs("\r\n");
	SIDwr(29,0);
	SIDwr(29,0);
	SIDwr(29,0);
    delay(10);
	SIDwr(31,CHAR_D);
	SIDwr(30,CHAR_I);
	SIDwr(29,CHAR_S);
	delay(1);
	SIDwr(31,CHAR_I);	//typ a kanal
	SIDwr(30,CHAR_I);
#ifndef BLIND
    delay(1);
		if ((arm2sid && SIDrd(27)!=2) || (!arm2sid && SIDrd(27)==2)) {
			cputs("incompatible version of armsid\r\n");
			return 0;
		}
#endif
    return 1;
}

int najdi() {
    return 1;
}

static unsigned char read_ver;
static unsigned char read_rev;
static unsigned char read_beta;

#ifdef BLIND
static unsigned char detail_count=0;
#endif

int details() {
    unsigned char p,q;
    SIDwr(31,CHAR_V);
    SIDwr(30,CHAR_I);
    delay(1);
    p=SIDrd(27);
    q=SIDrd(28);
#ifdef BLIND
	if (detail_count>1) {
		//cprintf ("fw version:%d.%d\r\n", p, q);
		cputs("fw version:");
		cputbval(p);
		cputc('.');
		cputbval(q);
		read_beta=1;
	} else cputs("fw version: unknown\r\n");
#else
//    cprintf ("fw version:%d.%d\r\n", p, q);
		cputs("fw version:");
		cputbval(p);
		cputc('.');
		cputbval(q);
#endif
    read_ver=p;
    read_rev=q;
    SIDwr(31,CHAR_R);
    SIDwr(30,CHAR_I);
    delay(1);
    p=SIDrd(27);
		read_beta=0;
		if ((p>'0')&&(p<'e')) {
			read_beta=p-'0';
			cputs(" beta ");
			cputbval(read_beta);
		}
		cputs("\r\n");
    SIDwr(31,CHAR_F);
    SIDwr(30,CHAR_I);
    delay(1);
    p=SIDrd(27);if ((p<=32)||(p>127)) p='.';
    q=SIDrd(28);if ((q<=32)||(q>127)) q='.';
#ifdef BLIND
	if (detail_count>1) {
		cputs("emulated device:");
		cputc(p);
		cputc(q);
		cputs("xx\r\n");
	} else cputs("emulated device: unknown\r\n");
#else
		cputs("emulated device:");
		cputc(p);
		cputc(q);
		cputs("xx\r\n");
#endif
    SIDwr(31,CHAR_I);
    SIDwr(30,CHAR_F);
    delay(1);
    p=SIDrd(27);if ((p<=32)||(p>127)) p='.';
    q=SIDrd(28);if ((q<=32)||(q>127)) q='.';
#ifdef BLIND
	if (detail_count>1) {
		cputs("app/boot:");
		cputc(p);
		cputc(q);
		cputs("\r\n");
	} else if (detail_count&1) cputs("app/boot:bl\r\n");
  else cputs("app/boot:ap\r\n");
#else
		cputs("app/boot:");
		cputc(p);
		cputc(q);
		cputs("\r\n");
#endif
#ifdef BLIND
	detail_count++;
	return detail_count&1;
#else
    if ((p!=CHAR_B)||(q!=CHAR_L)) return 1;
#endif
    return 0;
}

int get_SFX() {
	unsigned char i,p;
	if (!arm2sid) return 0;
	for(i=0;i<50;i++) {
    SIDwr(31,CHAR_M);
    SIDwr(30,CHAR_M);
    delay(1);
    p=SIDrd(27);
		if ((p!=0)&&(p!=8)) return 1;
	}
	return 0;
}

void erase_SFX() {
    cputs("\r\nsfx switching off .");
    SIDwr(31,'0');
    SIDwr(30,CHAR_M);
    delay(1);
    SIDwr(31,0xCF);
    SIDwr(30,CHAR_E);
    delay(2000);
#ifdef BLIND
#else
	    while (SIDrd(30)==70);
#endif
    cputs(" done\r\n");
}

int cont() {
    while (kbhit()) cgetc();
    cputs("\r\nready to update?[y/n]\r\n");
    if (cgetc()=='y') return 1;
    return 0;
}

int main_dwl(void)
{
    char a=0;
    char last=0;
	  char first=1;
    //*(unsigned char *)0xd018 = 0x15;
    uvod();
    if (!najdi()) return EXIT_SUCCESS;
    while (a!='q') {
	uvod();
	if (!najdi2()) return EXIT_SUCCESS;
	if (details()) {
    if (last) {
			if ((FWver!=read_ver)||(FWrev!=read_rev)) {
				textcolor(COLOR_RED);
				cputs("\r\nupdate error\r\n");
			} else {
				textcolor(COLOR_LIGHTGREEN);
				cputs("\r\nupdate done\r\n");
			}
  		textcolor (COLOR_WHITE);
			break;
		}

		if (first) {
  		if (!cont()) break;
			erase_SFX();
 		  uvod();
  		if (!najdi2()) return EXIT_SUCCESS;
	  	details();
#ifdef BLIND
			while (first || SIDaddr==(char *)(SIDs[1]))
#else
			while (get_SFX())
#endif
			{
				if (first==0) {
					a='q';
		  		textcolor (COLOR_RED);
					cputs("\r\nupdate error (sfx active)\r\n");
					textcolor (COLOR_WHITE);
					break;
				}
  			first=0;
  	    cputs("sfx is active\r\n");
				erase_SFX();
				uvod();
				if (!najdi2()) return EXIT_SUCCESS;
				details();
			}
			if (a=='q') break;
 			first=0;
			continue;
		} // first
  		cputs("\r\nerasing app header .");
	    SIDwr(31,66);
	    SIDwr(30,70);
	    delay(1500);
#ifdef BLIND
#else
	    while (SIDrd(30)==70);
#endif
	    cputs(" done\r\n");
	    cputs("\r\nreseting .");
	    SIDwr(31,82);
	    SIDwr(30,70);
	    delay(1);
#ifdef BLIND
#else
	    while (SIDrd(30)==70);
#endif
	    delay(1500);
	    cputs(" done\r\n");
	    continue;
	} else { //bootloader
	    int tout;
		  unsigned int i,setina;
	    if (last) {
    		textcolor (COLOR_RED);
  			cputs("\r\ndownload error\r\n");
    		textcolor (COLOR_WHITE);
	  		break;
		  }

			if (first) {
				if (!cont()) break;
				first=0;
			}

	    cputs("\r\nerasing app .");
	    SIDwr(31,66);
	    SIDwr(30,70);
	    delay(1000);
#ifdef BLIND
#else
	    while (SIDrd(30)==70);
#endif
	    delay(1000);
	    SIDwr(30,0);
	    SIDwr(31,69);
	    SIDwr(30,70);
	    delay(5000);
#ifdef BLIND
#else
	    while (SIDrd(30)!=102);
#endif
	    cputs(" done\r\n");

		//jen nastaveni AP/BL
	    SIDwr(30,0);
	    SIDwr(31,73);
	    SIDwr(30,70);
		delay(10);

		//start programovani
	    SIDwr(30,0);
	    SIDwr(31,83);
	    SIDwr(30,70);
	    delay(100);
#ifdef BLIND
#else
	    while (SIDrd(30)!=102);
#endif

#ifdef BLIND
		tout=0;
		cputs("\r\nprogramming ... ");
#else
		{
			char p;
			p=SIDrd(27);
		    if ((p==69)||(p==79)) {
					cputs("\r\nblank check ... ");
					cputc(p);
					cputc(SIDrd(28));
					cputs("\r\n");
				}
			if (p==69) {
				tout=1000;
				i=0;
			} else {
				tout=0;
				cputs("\r\nprogramming ... ");
			}
		}
#endif

	    setina=(FWlen+99)/100;
	    if (tout<1000) for(i=0;i<FWlen;i++) {
	    	SIDwr(31,FWdata[i]);
	    	SIDwr(30,87);
	    	delay0;
//			if (((i&127)==127)||(i==FWlen-1)) {
			if (((i&127)==127)||(i>FWlen-3)) {
				int x,y;
				x=wherex();
				y=wherey();
				cputbval(i/setina);
				cputc('%');
				gotoxy(x,y);
				delay(20);
			}
	    	tout=0;
	    	while (SIDrd(30)!=119) {
				tout++;
#ifdef BLIND
				delay(5); break;
#else
				if (tout>=1000) break;
#endif
			}
			if (tout>=1000) break;
	    }
	    if (tout<1000) {
			cputs("done\r\n");
	    } else {
    		textcolor (COLOR_RED);
				cputs("error at addr ");
				cput1000(i);
				cputs("     \r\n");
    		textcolor (COLOR_WHITE);
    			break;
			}
	    delay(500);
	    cputs("\r\nreseting .");
	    delay(100);
	    SIDwr(31,82);
	    SIDwr(30,70);
	    delay(1);
#ifdef BLIND
#else
	    while (SIDrd(30)==70);
#endif
	    delay(2000);
	    cputs("done\r\n");
	    last=1;
	    continue;
	}
    }
    cputs("\r\n");
	SIDwr(29,0);
	SIDwr(29,0);
	SIDwr(29,0);
    return EXIT_SUCCESS;
}

