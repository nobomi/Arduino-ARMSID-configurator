/*
** Arduino Stereo SID Player
** 
** Copyright (c) 2021 nobomi (Bohumil Novacek, dzin@post.cz)
** 
** MIT License
** 
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
** 
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
*/


/*****************************************************************************/
/*                                   Code                                    */
/*****************************************************************************/

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
  init();
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

// ----------------------------- IO operations ---------------------------------

byte SIDrd(byte addr) {
  byte pd, pb;
  // TCCR1B = 0b00001000;
  DDRD = DDRD & B00000011; // digital pins 2-7 input (data)
  DDRB = DDRB & B11110011; // didigtal pins 10-11 input (data)
  PORTD = PORTD & B00000000; // no pullups on data digital pins 2-7
  PORTB = PORTB & B11110011; // no pullups on data digital pins 10-11
  PORTB = PORTB | B00000001; // RW HIGH
  PORTC = (addr&31) | B00100000; // set address, keep CS HIGH
  if (addr>=32) {
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
  if (addr>=32) PORTB |= B00010000; // CS2 HIGH
  else PORTC = PORTC | B00100000; // CS HIGH
  byte out = (pd & B11111100) | ((pb >> 2) & B00000011);
  PORTB = PORTB & B11111110; // RW LOW
  DDRD = DDRD | B11111100; // digital pins 2-7 output (data)
  DDRB = DDRB | B00001100; // didigtal pins 10-11 output (data)
  // TCCR1B = 0b00001001;
  return out;
}

void SIDwr(byte addr, byte dta) {
  // TCCR1B = 0b00001000;
  PORTD = dta & B11111100;
  PORTB = (PORTB & B11110011) | ((dta & B00000011) << 2);
  PORTC = (addr&31) | B00100000; // set address, keep CS HIGH
  if (addr>=32) {
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
  if (addr>=32) PORTB |= B00010000; // CS2 HIGH
  else PORTC = PORTC | B00100000; // CS HIGH
  return;
}

void SIDwr2(byte addr, byte dta) {
  if (addr&64) {
    SIDwr(addr&31,dta);
    SIDwr(addr|32,dta);
  } else SIDwr(addr,dta);
}

////////////////////////////////////////////////////////////////////////////////

static char ison=1;

void Xoff(void) {
  if (ison==1) Serial.write(0x13);
  ison=0;
}

void Xon(void) {
  if (ison==0) Serial.write(0x11);
  ison=1;
}

void wait16(void) {
  char i;
  for(i=0;i<16;i++) {
    while (PINB&2);
    while ((PINB&2)==0);
  }
}

void nosound(void) {
  int i;
  wait16();
  for(i=0;i<16;i++) SIDwr(i,0);
  wait16();
  for(i=0;i<32;i++) SIDwr(i,0);
  wait16();
  for(i=0;i<48;i++) SIDwr(i,0);
  wait16();
  for(i=0;i<64;i++) SIDwr(i,0);
  wait16();
}

////////////////////////////////////////////////////////////////////////////////

static unsigned long us=0;

#define T1khz  16777U
#define T2khz (16777U*2U)
#define T3khz (16777U*3U)

void tone(unsigned short i) {
  SIDwr(0+14,i);
  SIDwr(1+14,i>>8);
  SIDwr(4+14,17);
  delay(200);
  SIDwr(4+14,16);
  delay(300);
}

static void  measure() {
  unsigned char i;
    tone(T2khz);
    tone(T1khz);
    tone(T1khz);
    tone(T3khz);
}

void play() {
    SIDwr(0,255);
    SIDwr(1,255);
    SIDwr(6,240);
    SIDwr(6+14,240);
    SIDwr(23,0xF1); //1ch filter
    SIDwr(24,0x0F); //volume
    measure();
}

static byte rx_buf[1024];
static int rx_top=0;
static int rx_bottom=0;

int rx_push(byte val) {
  int rx_next=(rx_top+1)&1023;
  if (rx_next!=rx_bottom) {
    rx_buf[rx_top]=val;
    rx_top=rx_next;
  }
}

byte rx_pop() {
  if (rx_top!=rx_bottom) {
    byte res=rx_buf[rx_bottom];
    rx_bottom=(rx_bottom+1)&1023;
    return res;
  }
  return 0;
}

int rx_push_all() {
  int i=Serial.available();
  while (i>0) {
    rx_push(Serial.read());
    i--;
  }
}

void rx_pop4(byte *b) {
  *b++=rx_pop();
  *b++=rx_pop();
  *b++=rx_pop();
  *b=rx_pop();
}

int rx_cnt() {
  return (rx_top-rx_bottom)&1023;
}

int main(void)
{
  int i;
  unsigned char b[4];
  setup();
  reset_SID();
  nosound();
  play();
  nosound();
  while (1) {

    // utekla sekunda ? neni co resit, preskocime
    long mic=micros();
    if (us-mic>1000000 || mic-us>1000000) us=mic;
    
    rx_push_all();
    
    if (rx_cnt()>512) {
      Xoff();
      while (rx_cnt()>256) {
        rx_pop4(b);
        if (b[0]==0 && b[1]==0 && b[2]==255 && b[3]==254) us=micros();
        else {
          unsigned long x=b[0]+b[1]*256;
          us+=x;
          while (((signed long)(us-micros()))>0) {
            ;
          }
          if (b[2]!=255) {
            SIDwr2(b[2],b[3]);
          }
        }
        rx_push_all();
      }
      Xon();
    } else if (rx_cnt()>=4) {
      rx_pop4(b);
      if (b[0]==255 && b[1]==255 && b[2]==255 && b[3]==255) {
        char rst=1;
        while (rx_cnt()<=1 && rst) {
          rx_push_all();
          while (rx_cnt()>0) {
            if (rx_pop()==0) {
              nosound();
              rst=0;
              break;
            }
          }
        }
        continue;
      }
      else if (b[0]==0 && b[1]==0 && b[2]==255 && b[3]==254) us=micros();
      else {
        unsigned long x=b[0]+b[1]*256;
        us+=x;
        while (((signed long)(us-micros()))>0) {
          rx_push_all();
          if (rx_cnt()>512) {
            Xoff();
            while (((signed long)(us-micros()))>0) { ; }
            break;
          }
        }
        if (b[2]!=255) {
          SIDwr2(b[2],b[3]);
        }        
      }
    }
  }
  return 0;
}
