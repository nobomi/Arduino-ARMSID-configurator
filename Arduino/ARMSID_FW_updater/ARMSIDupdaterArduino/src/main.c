#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <windows.h>
#include <time.h>
#include <conio.h>

extern int main_dwl(void);
extern int open_seriak(char *name);
extern void close_seriak();
extern void delay(unsigned int milliseconds);

int arm2sid;

unsigned char FWver;
unsigned char FWrev;
unsigned char FWbeta;
unsigned short FWlen;
unsigned char FWdata[0xC000];

int open_binfile(char *name) {
    unsigned char s[8];
    FILE *f_in = fopen(name,"rb");
    if (!f_in) {
        fprintf(stderr, "Error open binfile\n");
        return 1;
    }
    if (1!=fread(s,8,1,f_in) || s[0]!='A' || s[4] || s[5] || s[6]>=192) {
        fprintf(stderr, "Error binfile header format\n");
        fclose(f_in);
        return 1;
    }
    FWver=s[1];FWrev=s[2];FWbeta=s[3];FWlen=(s[6]<<8)|s[7];
    if (1!=fread(FWdata,FWlen,1,f_in)) {
        fprintf(stderr, "Error binfile too short\n");
        fclose(f_in);
        return 1;
    }
    fclose(f_in);
    arm2sid=0;
    if (!memcmp(FWdata,"ARM2SID_",8)) {
        arm2sid=1;
    } else if (memcmp(FWdata,"ARMSID1_",8)) {
        fprintf(stderr, "Error binfile for unsupported device\n");
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc<3) {
            printf("usage: %s COMx binfile\r\n",argv[0]);
            return 0;
    }
    if (open_binfile(argv[2])) { return -2; }
    if (open_seriak(argv[1])) { return -1; }
    delay(300);
    main_dwl();
    close_seriak();
    return 0;
}

