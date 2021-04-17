#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <windows.h>
#include <time.h>

void delay(unsigned int milliseconds){

    clock_t start;
    while (milliseconds > 500) {
        delay(500);
        printf(".");
        milliseconds-=500;
    }
    start = clock();
    while((clock() - start) * 1000 / CLOCKS_PER_SEC < milliseconds);
}

HANDLE hSerial;

void serout(char *data, int len) {
    DWORD bytes_written;
    if(!WriteFile(hSerial, data, len, &bytes_written, NULL))
    {
        fprintf(stderr, "Error\n");
        CloseHandle(hSerial);
        exit(1);
    }
}

void SIDwrite(int addr, uint8_t data) {
    char s[20];
    sprintf(s,"W%02X%02X\r\n",addr&255,data);
    serout(s,strlen(s));
}

unsigned char SIDread(int addr) {
    DWORD bytes_written;
    unsigned char r=0;
    char s[20];
    sprintf(s,"R%02X\r\n",addr&255);
    serout(s,strlen(s));
    ReadFile(hSerial, &r, 1, &bytes_written, NULL);
    return r;
}

int open_seriak(char *name) {
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

    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 50;

    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    if(SetCommTimeouts(hSerial, &timeouts) == 0)
    {
        fprintf(stderr, "Error setting timeouts\n");
        CloseHandle(hSerial);
        return 1;
    }
    return 0;
}

void close_seriak() {
    delay(200);
    CloseHandle(hSerial);
}

