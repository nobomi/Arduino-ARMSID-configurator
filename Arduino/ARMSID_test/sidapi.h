/*
** ArmSID & Arm2SID tester/configurator using cc65 or Borland Turbo C++ V1.0. or Arduino and others
**
** Copyright (c) 2019-2021 Bohumil Novacek (dzin@post.cz)
**
** This program is development version. Not to be shared or distributed!
*/

#define SOCKETS_NAMED

#define ARM2SID
#define UID
#define DIGIFIX
//#define DEMO
#define ASCII_FRAMES 0      // 0 = CP852 frames, 1 = 7bit ascii
#define TERMINAL_HEIGHT 24  // 25 if not used, 23 minimal

// ----------------------------- arduino setup ---------------------------------

#define PHI2 9
#define RW 8
#define CS1 A5
#define CS2 12
#define RST 13 // together with Arduino UNO LED
#define D1 11
#define D0 10

void Set_PHI2_1MHz() {
  TCCR1A = 0b01000000;
  TCCR1B = 0b00001001;
  OCR1AH = 0;
  OCR1AL = 7; // 1 MHz
  TIMSK1 = 0;
}

void setup() {
  // put your setup code here, to run once:
  PORTD = B00000000;
  PORTB = B00110000; // bit 5 RST HIGH, bit 4 CS2 HIGH, bit 0 R/W LOW
  PORTC = B00100000; // no pullups, but pin 5 CS1 HIGH
  DDRD = B11111111; // digital pins 2-7 output (data)
  DDRB = B11111111; // didigtal pins 10-11 output (data)
  DDRC = B00111111; // analog pins 0-4 output (register address), bit 5 CS
  Set_PHI2_1MHz();
  Serial.begin(115200);
}

void reset_SID() {
  digitalWrite(RST, LOW);
  delay(10);
  digitalWrite(RST, HIGH);
  delay(750);
}

// -------------------------- ARMSID addressing --------------------------------

#define SIDaddrtype unsigned short
static SIDaddrtype SIDaddr;

#define SIDaddresses  2
//static SIDaddrtype SIDs[1]={0};

SIDaddrtype SIDgetaddr(int i)
{
  return (SIDaddrtype)(0xD400+((i&1)<<5)+((i&2)<<7)+((i&4)*0x0280));  //SIDs[i];
}

// ----------------------------- IO operations ---------------------------------

byte SIDrd(byte addr) {
  byte pd, pb;
  // TCCR1B = 0b00001000;
  DDRD = DDRD & B00000011; // digital pins 2-7 input (data)
  DDRB = DDRB & B11110011; // didigtal pins 10-11 input (data)
  PORTD = PORTD & B00000000; // no pullups on data digital pins 2-7
  PORTB = PORTB & B11110011; // no pullups on data digital pins 10-11
  PORTB = PORTB | B00000001; // RW HIGH
  PORTC = addr | B00100000; // set address, keep CS HIGH
  if (SIDaddr&0x3FF) {
    register byte cs = PORTB & ~(B00010000); // CS2 LOW
    while (PINB&2);
    while ((PINB&2)==0);
    PORTB = cs;
  }
  else {
    register byte cs = addr & B00011111; // write address and CS LOW
    while (PINB&2);
    while ((PINB&2)==0);
    PORTC = cs;
  }
  while (PINB&2);
  while ((PINB&2)==0);
  pd = PIND;
  pb = PINB;
  if (SIDaddr&0x3FF) PORTB |= B00010000; // CS2 HIGH
  else PORTC = PORTC | B00100000; // CS HIGH
  byte out = (pd & B11111100) | ((pb >> 2) & B00000011);
  PORTB = PORTB & B11111110; // RW LOW
  DDRD = DDRD | B11111100; // digital pins 2-7 output (data)
  DDRB = DDRB | B00001100; // didigtal pins 10-11 output (data)
  return out;
}

void SIDwr(byte addr, byte dta) {
  // TCCR1B = 0b00001000;
  PORTD = dta & B11111100;
  PORTB = (PORTB & B11110011) | ((dta & B00000011) << 2);
  PORTC = addr | B00100000; // set address, keep CS HIGH
  if (SIDaddr&0x3FF) {
    register byte cs = PORTB & ~(B00010000); // CS2 LOW
    while (PINB&2);
    while ((PINB&2)==0);
    PORTB = cs;
  }
  else {
    register byte cs = addr & B00011111; // write address and CS LOW
    while (PINB&2);
    while ((PINB&2)==0);
    PORTC = cs;
  }
  while (PINB&2);
  while ((PINB&2)==0);
  if (SIDaddr&0x3FF) PORTB |= B00010000; // CS2 HIGH
  else PORTC = PORTC | B00100000; // CS HIGH
  return;
}

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

// ascii frames
#if ASCII_FRAMES
#define CHAR_FRAME_TOP_LEFT		  '/'
#define CHAR_FRAME_TOP			    '='
#define CHAR_FRAME_TOP_RIGHT	  '\\'
#define CHAR_FRAME_RIGHT		    '|'
#define CHAR_FRAME_BOTTOM_RIGHT	'/'
#define CHAR_FRAME_BOTTOM		    '='
#define CHAR_FRAME_BOTTOM_LEFT	'\\'
#define CHAR_FRAME_LEFT			    '|'
#define CHAR_LINE               '-'
#define CHAR_CHOICE_1           '\\'
#define CHAR_CHOICE_2           '/'
#define CHAR_DEGREE             '^'
#else
#define CHAR_FRAME_TOP_LEFT     201
#define CHAR_FRAME_TOP          205
#define CHAR_FRAME_TOP_RIGHT    187
#define CHAR_FRAME_RIGHT        186
#define CHAR_FRAME_BOTTOM_RIGHT 188
#define CHAR_FRAME_BOTTOM       205
#define CHAR_FRAME_BOTTOM_LEFT  200
#define CHAR_FRAME_LEFT         186
#define CHAR_LINE               196
#define CHAR_CHOICE_1           0x7F
#define CHAR_CHOICE_2           0xC2
#define CHAR_DEGREE             248
#endif

// ------------------------- GRAFIK AND TIMING ---------------------------------

#define COLOR_BLUE       0x204
#define COLOR_LIGHTBLUE   0x104
#define COLOR_GREEN     0x202
#define COLOR_LIGHTGREEN  0x102
#define COLOR_RED       0x201
#define COLOR_LIGHTRED    0x101
#define COLOR_YELLOW    0x103
#define COLOR_WHITE     0x107

void textcolor(short c);
void revers(int a);
void cputc_revers(char ch);

static char KX=0, KY=0;
static char KURZMOVEON=1;

#define cputc_(a) Serial.write(a&255)
void cputc(const char ch) {
  if ((ch>='a')&&(ch<='z')) cputc_(ch+'A'-'a');
  else cputc_(ch);
  if (KURZMOVEON) {
    if (ch>=' ') {
      KX++;
      if ((KX>=40) && (KY<TERMINAL_HEIGHT-1)) {
        cputc_(13);cputc_(10);
        KX=0;KY++;
      }
    } else if (ch==0x0D) KY++;
    else if (ch==0x0A) KX=0;
  }
}


//new cputs with macros for color change and inversion text
void cputs2my(const char *s) {
  while (*s) {
    if (*s=='L') {
      textcolor(COLOR_LIGHTBLUE);
    } else if (*s=='W') {
      textcolor(COLOR_WHITE);
    } else if (*s=='R') {
      revers(0);
    } else if (*s=='@') {
      s++;
      if (*s) cputc_revers(*s);
      else break;
    } else if (*s=='\\') {
      cputc(13);
      cputc(10);
    } else {
      cputc(*s);
    }
    s++;
  }
}

// wait for ms milliseconds
void delay_ms(short ms) {
     delay(ms);
}

// wait for at least 100us
#define delay100us() delay_ms(1)

// no border color
#define bordercolor(a)	(0)

// x,y coordinates start from 0 on the top left in C64 (from 1 in DOS)
int wherex() { return KX; }
int wherey() { return KY; }

static void textbackground(short i) {
  KURZMOVEON=0;
  cputs2my("\x1B[4");
  cputc('0'+(i&255));
  cputs2my(";");
  cputc('0'+(i>>8));
  cputc_('m');
  KURZMOVEON=1;
}

static void textforeground(short i) {
  KURZMOVEON=0;
  cputs2my("\x1B[3");
  cputc('0'+(i&255));
  cputs2my(";");
  cputc('0'+(i>>8));
  cputc_('m');
  KURZMOVEON=1;
}

static short c_mem=COLOR_BLUE;
static short c_txt=COLOR_WHITE;
static char inversed=0;

void textcolor(short c) {
  if (inversed) {
    textbackground(c);
  } else {
    textforeground(c);  
  }
  c_txt=c;
}

void bgcolor(short c) {
  if (inversed) {
    textforeground(c);  
  } else {
    textbackground(c);
  }
  c_mem=c;
}

void revers(int a) {
  inversed=a;
  bgcolor(c_mem);
  textcolor(c_txt);
}

// print color inversed character
void cputc_revers(char ch) {
        revers(1);
        cputc(ch);
        revers(0);
}

#define _NOCURSOR  'l'
#define _NORMALCURSOR 'h'
void _setcursortype(int i) {
  KURZMOVEON=0;
  cputs2my("\x1B[?25");
  cputc_(i);
  KURZMOVEON=1;
}

void gotoxy(int x, int y) {
  cputs2my("\x1B[");
  if (y+1>=10) cputc('0'+(y+1)/10);
  cputc('0'+(y+1)%10);
  cputc(';');
  if (x+1>=10) cputc('0'+(x+1)/10);
  cputc('0'+(x+1)%10);
  cputc_('H');
  KX=x;KY=y;
}

void gotoy(int y) {
  gotoxy(wherex(),y);
}

void clrscr() {
  cputs2my("\x1B[2J");
  gotoxy(0,0);
}

void fcputs2(const __FlashStringHelper *s) {
  String tmpstr(s);
  cputs2my(tmpstr.c_str());
}

void fcputs2xy(const unsigned char x, const unsigned char y, const __FlashStringHelper *s) {
  String tmpstr(s);
  gotoxy(x,y);
  cputs2my(tmpstr.c_str());
}

#define cputs2(a) fcputs2(F(a))
//#define cputs2(a) cputs2my(a)
#define cputs2xy(a,b,c) fcputs2xy(a,b,F(c))

// ------------------------------ KEYBOARD -------------------------------------

// return non zero when a key is pressed
int keypressed(void)
{
	return Serial.available();
}

static char escaped=0;

// reads the key code of the pressed key from buffer
int rkey(void)
{
	char k=0;
	while (Serial.available() > 0) {
    k = Serial.read();
    if (escaped==2) {
 	    escaped=0;
 	    switch (k) {
   		  case '0' ... '9': escaped=2; return 0;
   		  case 0x41: return 0x4800;
   		  case 0x42: return 0x5000;
   		  case 0x43: return 0x4D00;
   		  case 0x44: return 0x4B00;
   		  default: return 0;
 	    }
    }
    if ((escaped==1) && (k==91)) {
  	  escaped=2;
	    continue;
	  } else if (escaped==1) escaped=0;
      if (k==27) escaped=1;
      else {
  		  switch (k) {
            case 'I': return 0x4800;
            case 'K': return 0x5000;
            case 'L': return 0x4D00;
            case 'J': return 0x4B00;
            /*
	        case 'A': return 0x4800;
	        case 'B': return 0x5000;
	        case 'C': return 0x4D00;
	        case 'D': return 0x4B00;
	        */
	  	  }
		    return k;
	    }
    }
    return 0;
}

// reads the key and convert to C64 cursor keys
int getkey(void)
{
int key;
 if (!keypressed()) return 0;
 else {
	 key=rkey();
	 if (key==0x4800) key=17+128; // up
	 if (key==0x4B00) key=29+128; // left
	 if (key==0x4D00) key=29; // right
	 if (key==0x5000) key=17; // down
	 if (key&0xFF) return key&0xFF; else return key;
 }
}

// a pressed key reading
#define cgetc()	getkey()


// ------------------------- MAIN FUNCTION -------------------------------------

//#define main_params	int argc, char *argv[]
#define main_params	void

//int main(main_params);

void sid_off(void);

void setup2() {
  reset_SID();
    SIDwr(24, 0);  //silent
    SIDwr(24, 0);
    SIDwr(24, 0);
    SIDwr(0, 0);
    SIDwr(0, 0);
    SIDwr(0, 0);
  sid_off();
  delay(1000);
    cputs2my("\x1B[0");
    cputc_('m');
    bgcolor(COLOR_BLUE);
    textcolor(COLOR_WHITE);
    _setcursortype(_NOCURSOR);
    clrscr();
    while (keypressed()) cgetc();  
}

// macro for main start
#define  main_begin() \
  init(); \
  setup(); \
  setup2(); \
  while(1) { a=0;

// called on the end of main function
#define main_end() a=0; }
//  textmode(tinfo.currmode);
//  _setcursortype(_NORMALCURSOR);
//}

// when no sid is find or choosed
#define main_nosid() \
	  cputs2("\\press a key\\"); \
	  while (!keypressed()); \
	  cgetc();

////////////////////////////////////////////////////////////////////////////////
