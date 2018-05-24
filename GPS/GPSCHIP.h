/* Revision: 2.8.6 */

/******************************************************************************
* Copyright 1998-2018 NetBurner, Inc.  ALL RIGHTS RESERVED
*
*    Permission is hereby granted to purchasers of NetBurner Hardware to use or
*    modify this computer program for any use as long as the resultant program
*    is only executed on NetBurner provided hardware.
*
*    No other rights to use this program or its derivatives in part or in
*    whole are granted.
*
*    It may be possible to license this or other NetBurner software for use on
*    non-NetBurner Hardware. Contact sales@Netburner.com for more information.
*
*    NetBurner makes no representation or warranties with respect to the
*    performance of this computer program, and specifically disclaims any
*    responsibility for any damages, special or consequential, connected with
*    the use of this program.
*
* NetBurner
* 5405 Morehouse Dr.
* San Diego, CA 92121
* www.netburner.com
******************************************************************************/

#include <predef.h>
#include <stdio.h>
#include <ctype.h>
#include <startnet.h>
#include <ucos.h>
#include <serial.h>
#include <string.h>
#include <stdlib.h>
#ifdef MCF5270
#include <sim5270.h>
#define GPS_TIMER 1
#elif defined MCF5441X
#include <sim5441x.h>
#define GPS_TIMER 0
#define sim sim2
#endif
#include "GPSTime.h"
#include "GPSLocation.h"
#include "GPSSat.h"
#include "GPSNtp.h"

#ifndef GPSCHIP_H_
#define GPSCHIP_H_

// Some constants
#define PRIORITY    (MAIN_PRIO-2)
#define SERIAL_PORT (1)
#define SERIAL_BAUD (0) // Baudrate or 0 for auto-select
#define LOG_SIZE    (5)



class GPSCHIP {
public:
    // Variables
    int priority;
    int serialBaud;
    int serialPort;
    OS_MBOX taskMBOX;
    bool allDone;
    DWORD GPSTaskStack[USER_TASK_STK_SIZE/2];
    int fdIncoming;


    // Functions
    static void staticreadLoop(void *);
    void readLoop();
    BYTE GetGPSCheckSum(const char* GPSString );
    BOOL CheckGPSCheckSum( const char* GPSString );
    void resetGM862();

public:
    // Variables
    int sats;
    int GPS_Lock;
    GPSTime time;
    GPSNtp ntp;
    GPSLocation latitude;
    GPSLocation longitude;
    GPSSat sat[12];         // utilized GPS satellites
    GPSSat visibleSat[32];  // visible GPS satellites

    // Constructor
    GPSCHIP(int timerNum) :  priority(PRIORITY),
                    serialBaud(SERIAL_BAUD),
                    serialPort(SERIAL_PORT),
                    allDone(false),
                    fdIncoming(0),
                    sats(0),
                    GPS_Lock(0),
                    ntp(timerNum)
                    {}

    // Functions
    void start();
    char * getGPS();
    void setGPS(char * gps);

    void DecodeGPGGA(char* pGPSData);
    void DecodeGPGSV(char* pGPSData);
    void DecodeGPRMC(char* pGPSData);
    void DecodeGPGSA(char* pGPSData);
};

#endif /* GPSCHIP_H_ */
