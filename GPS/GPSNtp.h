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
#include <basictypes.h>
#include <startnet.h>
#include <ip.h>
#include <udp.h>
#include <bsp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef MCF5270
#include <sim5270.h>
#elif defined MCF5441X
#include <sim5441x.h>
#define sim sim2
#endif
#include <pinconstant.h>
#include <pins.h>
#include <cfinter.h>


#ifndef GPSNTP_H_
#define GPSNTP_H_

#define PRECISON (0xE9)
#define DISPERSION (0)
#define ROOT_DELAY (0)
#define NTPPRIORITY (MAIN_PRIO-2)
#define NTP_PORT (123)
#define LED1_OFF     (0x00)
#define LED1_RED     (0x01)
#define LED1_GREEN   (0x02)
#define LED2_OFF     (0x00)
#define LED2_RED     (0x04)
#define LED2_GREEN   (0x08)

class GPSNtp {

public:
    int hours;
    int minutes;
    int seconds;
    int month;
    int day;
    int year;
    bool valid;
    volatile DWORD LastEdge;
    volatile DWORD PreviousEdge;
    volatile DWORD Difference;
    volatile DWORD AverageDifference;
    volatile DWORD IsrCount;
    volatile DWORD mindiff;
    volatile DWORD maxdiff;
    volatile DWORD SamplesInARow;
    volatile DWORD dwIntervalErrors;
    volatile DWORD ValidRMCInARow;
    DWORD ParseIsr;
    DWORD ParseTime;
    volatile timerstruct &timer;


    GPSNtp(int timerNum) :  hours(0),
                minutes(0),
                seconds(0),
                month(0),
                day(0),
                year(0),
                valid(false),
                timer(sim.timer[timerNum]),
                priority(NTPPRIORITY)
                {setLED(LED1_RED);}


    char * getFormatedTime();
    char * getFormatedDate();
    void setTimeFromGPGGAStr(char * timeValue);
    void setDateFromGPGGAStr(char * dateValue);
    void StartNTP();
    void ProcessNtpSecs(DWORD ParseIsr, DWORD ParseTime, int hour, int min, int sec, int day, int month, int year);
    BOOL CaptureNtpTime(DWORD & dwSecs, DWORD & dwFrac);
    void setLED(BYTE led);
private:
    int priority;

    volatile DWORD EdgeCountAtTime;
    volatile DWORD NtpTimeSecs;
    volatile BOOL bTimeIsValid;

    volatile const char * whyfail;

    typedef union _UnsignedLongLong {
        unsigned long long unsignedLongLong;
        unsigned int unsignedInt[2];

    }__attribute__(( packed) ) UnsignedLongLong;

};

#endif /* GPSNTP_H_ */
