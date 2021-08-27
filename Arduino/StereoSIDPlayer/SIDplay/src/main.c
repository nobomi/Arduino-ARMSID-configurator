#define CHIPS_IMPL
#include "m6502.h"
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h> // clock_t, clock, CLOCKS_PER_SEC

#ifdef _WIN32
  #include <windows.h>
  #include <conio.h>
#else
  // to build on unix/macOS/linux:
  // gcc -Wall -o sidplay main.c
  #include <sys/types.h>
  #include <sys/uio.h>
  #include <unistd.h>
  #include <stdlib.h>
  #include <termios.h>
  typedef int HANDLE;
  typedef long int DWORD;
  #define CloseHandle(h) close(h)
#endif

void delay(unsigned int milliseconds){

    clock_t start = clock();

    while((clock() - start) * 1000 / CLOCKS_PER_SEC < milliseconds);
}

HANDLE hSerial;

static int stereo=0;
static int state_on=1;

void serout(unsigned char *data, int len) {
    DWORD bytes_read;
    //printf("serout\n");
    do {
        char x;
#ifdef _WIN32
        ReadFile(hSerial, &x, 1, &bytes_read, NULL);
#else
        bytes_read = read(hSerial, &x, 1);
#endif
        if (bytes_read>0) {
            if (x==0x11) {
                    state_on=1;
                    printf("o");
            }
            if (x==0x13) {
                    state_on=0;
                    printf("f");
            }
        }
    }
    while (bytes_read>0 || state_on==0);
#ifdef _WIN32
    if(!WriteFile(hSerial, data, len, &bytes_written, NULL))
#else
    if(write(hSerial, data, len) != len)
#endif
    {
        fprintf(stderr, "Error\n");
        CloseHandle(hSerial);
        exit(1);
    }
#ifndef _WIN32
    tcdrain(hSerial); // wait for written data to finish transmitting
#endif
}

int __ssat(int a,int b) {
    if (a<-((1<<(b-1))-1)) a=-((1<<(b-1))-1);
    if (a>((1<<(b-1))-1)) a=((1<<(b-1))-1);
    return a;
}

unsigned char readbyte(FILE *f)
{
  unsigned char res;

  fread(&res, 1, 1, f);
  return res;
}

unsigned short readword(FILE *f)
{
  unsigned char res[2];

  fread(&res, 2, 1, f);
  return (res[0] << 8) | res[1];
}

int playaddress;

int load_sid(const char *sidname, char *mem) {
  FILE *in;
  int dataoffset,loadaddress,initaddress,loadpos,loadend,loadsize;

  // Open SID file
  if (!sidname)
  {
    printf("Error: no SID file specified.\n");
    return -1;
  }

  in = fopen(sidname, "rb");
  if (!in)
  {
    printf("Error: couldn't open SID file.\n");
    return -1;
  }

  // Read interesting parts of the SID header
  fseek(in, 6, SEEK_SET);
  dataoffset = readword(in);
  loadaddress = readword(in);
  initaddress = readword(in);
  playaddress = readword(in);

  fseek(in, 0x7A, SEEK_SET);
  unsigned char second_sid=readbyte(in);
  if ((second_sid&1)==0 && ( (second_sid>=0x42 && second_sid<=0x7E) || (second_sid>=0xE0 && second_sid<=0xFE) )) stereo=1;
  else stereo=0;

  fseek(in, dataoffset, SEEK_SET);
  if (loadaddress == 0)
    loadaddress = readbyte(in) | (readbyte(in) << 8);

  // Load the C64 data
  loadpos = ftell(in);
  fseek(in, 0, SEEK_END);
  loadend = ftell(in);
  fseek(in, loadpos, SEEK_SET);
  loadsize = loadend - loadpos;
  if (loadsize + loadaddress >= 0x10000)
  {
    printf("Error: SID data continues past end of C64 memory.\n");
    fclose(in);
    return -1;
  }
  fread(&mem[loadaddress], loadsize, 1, in);
  fclose(in);
  return initaddress;
}

#define MAX_INSTR 0x100000

volatile int instr;
volatile int last_instr;
unsigned char last_byte;

void SIDwrite(int addr, uint8_t data) {
//    databus[dataindexwrite/databus_N]=data | ((addr<<8)&0x1F00) | ((instr<<16)&0xFF0000);
//    dataindexwrite+=4;
    unsigned char b[4]={0x00,0x7D,255,0};
    while (last_instr-instr>32000) {
        last_instr-=32000;
        serout(b,4);
    }
    b[0]=(last_instr-instr)&255;
    b[1]=(last_instr-instr)>>8;
    b[2]=addr&127;b[3]=data;
    serout(b,4);
    last_instr=instr;
    last_byte=data;
}

int runcpu(m6502_t *cpu, uint64_t *pins, uint8_t *mem) {
    *pins = m6502_tick(cpu, *pins);
    const uint16_t addr = M6502_GET_ADDR(*pins);
    if (*pins & M6502_RW) {
        if ((addr&0xFFE0)==0xD400) {
            M6502_SET_DATA(*pins, last_byte);
        }
        else M6502_SET_DATA(*pins, mem[addr]);
    }
    else {
        if ((addr&0xFFE0)==0xD400) {
            if (stereo) {
                SIDwrite(addr,M6502_GET_DATA(*pins));
            } else {
                SIDwrite(addr|0x40,M6502_GET_DATA(*pins));
            }
        }
        else if ((addr&0xFFE0)==0xD420) {
            SIDwrite(addr,M6502_GET_DATA(*pins));
        }
        else if ((addr&0xFFE0)==0xD500) {
            SIDwrite(addr|0x20,M6502_GET_DATA(*pins));
        }
        else if ((addr&0xFEE0)==0xDE00) {
            SIDwrite(addr|0x20,M6502_GET_DATA(*pins));
        }
        else mem[addr] = M6502_GET_DATA(*pins);
    }
    return 0;
}

void setPC(uint16_t next_pc, m6502_t *cpu, uint64_t *pins, uint8_t *mem) {
    *pins = M6502_SYNC;
    M6502_SET_ADDR(*pins, next_pc);
    M6502_SET_DATA(*pins, mem[next_pc]);
    m6502_set_pc(cpu, next_pc);
}

int open_seriak(char *name) {
#ifdef _WIN32
    char namefull[100];
    DCB dcbSerialParams = {0};
    COMMTIMEOUTS timeouts = {0};

    if (strlen(name)>80) return -1;
    sprintf(namefull,"\\\\.\\%s",name);

    fprintf(stderr, "Opening serial port...");
    hSerial = CreateFile(
                namefull, GENERIC_READ|GENERIC_WRITE, 0, NULL,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if (hSerial == INVALID_HANDLE_VALUE)
    {
            fprintf(stderr, "Error\n");
            return 1;
    }
    else fprintf(stderr, "OK\n");

    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (GetCommState(hSerial, &dcbSerialParams) == 0)
    {
        fprintf(stderr, "Error getting device state\n");
        CloseHandle(hSerial);
        return 1;
    }

    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    dcbSerialParams.fDtrControl = FALSE;
    dcbSerialParams.fBinary = TRUE;
    if(SetCommState(hSerial, &dcbSerialParams) == 0)
    {
        fprintf(stderr, "Error setting device parameters\n");
        CloseHandle(hSerial);
        return 1;
    }

    // Set COM port timeout settings
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    if(SetCommTimeouts(hSerial, &timeouts) == 0)
    {
        fprintf(stderr, "Error setting timeouts\n");
        CloseHandle(hSerial);
        return 1;
    }
    return 0;
#else
    struct termios term;

    fprintf(stderr, "Opening serial port...");
    hSerial = open(name, O_RDWR | O_NOCTTY);
    if (hSerial == -1)
    {
            fprintf(stderr, "Error\n");
            return 1;
    }
    else fprintf(stderr, "OK\n");

    if (tcgetattr(hSerial, &term) != 0)
    {
        fprintf(stderr, "Error getting device state\n");
        CloseHandle(hSerial);
        return 1;
    }

    cfmakeraw(&term);
    cfsetspeed(&term, B115200);
    term.c_cc[VMIN] = 0; // non-blocking read()
    term.c_cc[VTIME] = 0; // should be zero by default, but just to be safe.
    // timing of writes is handled by tcdrain() after write()

    if(tcsetattr(hSerial, TCSANOW, &term) != 0)
    {
        fprintf(stderr, "Error setting device parameters\n");
        CloseHandle(hSerial);
        return 1;
    }

    return 0;
#endif
}

int main(int argc, char **argv) {
    int initaddress;
    // 64 KB zero-initialized memory
    uint8_t mem[(1<<16)] = { };

#ifdef _WIN32
    _setmode( _fileno( stdin ), _O_BINARY );
    _setmode( _fileno( stdout ), _O_BINARY );
#endif

    if (argc<3) {
            printf("usage: %s COMx sound_file.sid\r\n",argv[0]);
            return 0;
    }
    initaddress=load_sid(argv[2],(char*)mem);
    if (initaddress<=0) return 0;
    mem[0x01] = 0x37;

    if (open_seriak(argv[1])) return -1;

    delay(300);

    unsigned char brst[13]={255,255,255,255,255,255,255,255,255,255,255,255,0};
    serout(brst,13);

    // initialize a 6502 instance:
    m6502_t cpu;
    uint64_t pins = m6502_init(&cpu, &(m6502_desc_t){ });

    // run for 7 ticks reset sequence
    for (int i = 0; i < 7; i++) pins = m6502_tick(&cpu, pins);

    setPC(initaddress,&cpu,&pins,mem);
    instr = 0; last_instr = 0;
    unsigned char b[4]={0,0,255,254};
    serout(b,4);
      while (1)
      {
        runcpu(&cpu, &pins, mem);
        instr--;
        if (cpu.brk_flags&M6502_BRK_IRQ) break;
        if (cpu.PC<4) break;
        if (instr < -MAX_INSTR)
        {
          break;
        }
      }

      if (playaddress == 0)
      {
        if ((mem[0x01] & 0x07) == 0x5)
          playaddress = mem[0xfffe] | (mem[0xffff] << 8);
        else
          playaddress = mem[0x314] | (mem[0x315] << 8);
      }

      while(instr>-985250*300) {
          int localcnt=(985250/50)-2;
          printf(".");fflush(stdout);
        pins = m6502_init(&cpu, &(m6502_desc_t){ });

        // run for 7 ticks reset sequence
        for (int i = 0; i < 7; i++) pins = m6502_tick(&cpu, pins);
        setPC(playaddress,&cpu,&pins,mem);
          while(localcnt--) {
            runcpu(&cpu, &pins, mem);
            instr--;
            if (cpu.brk_flags&M6502_BRK_IRQ) break;
            if (cpu.PC<4) break;
            if ((mem[0x01] & 0x07) != 0x5 && (cpu.PC == 0xea31 || cpu.PC == 0xea81)) break;
          }
          do {
            instr--;
          } while (instr%(985250/50));

#ifdef _WIN32
        if (kbhit()) {
            break;
        }
#endif

      }
            serout(brst,13);
            delay(200);
            CloseHandle(hSerial);

    return 0;
}

