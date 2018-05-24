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


#include "GPSNtp.h"

extern const DWORD NTPsecs[1117];


char * GPSNtp::getFormatedTime() {
    static char formattedTime[32];
    sniprintf(formattedTime, 32, "%d:%.2d:%.2d", hours, minutes, seconds);
    return formattedTime;
}

char * GPSNtp::getFormatedDate() {
    static char formattedTime[32];
    sniprintf(formattedTime, 32, "%.2d/%.2d/%.2d", month, day, year);
    return formattedTime;
}

void GPSNtp::setTimeFromGPGGAStr(char * timeValue) {
    char TempStr[10];
    memcpy(TempStr, timeValue, 2);
    hours = strtoul(TempStr, NULL, 10);
    memcpy(TempStr, timeValue + 2, 2);
    minutes = strtoul(TempStr, NULL, 10);
    memcpy(TempStr, timeValue + 4, 2);
    seconds = strtoul(TempStr, NULL, 10);
}

void GPSNtp::setDateFromGPGGAStr(char * dateValue) {
    char TempStr[10];
    memcpy(TempStr, dateValue, 2);
    day = strtoul(TempStr, NULL, 10);
    memcpy(TempStr, dateValue + 2, 2);
    month = strtoul(TempStr, NULL, 10);
    memcpy(TempStr, dateValue + 4, 2);
    year = strtoul(TempStr, NULL, 10);
}


BOOL GPSNtp::CaptureNtpTime(DWORD & dwSecs, DWORD & dwFrac) {
    DWORD capture;
    DWORD last_edge;
    DWORD last_edge_sec;
    DWORD interval;
    BOOL bValid;
    USER_ENTER_CRITICAL()
    capture = timer.tcn;
    last_edge = EdgeCountAtTime;
    last_edge_sec = NtpTimeSecs;
    interval = AverageDifference;
    bValid = ((SamplesInARow > 10) && (bTimeIsValid));
    USER_EXIT_CRITICAL();
    if (!bValid) {
        if (SamplesInARow < 10)
            whyfail = "Not Enough samples<BR>";
        return FALSE;
    }

    //Now it gets hard.
    //Step one adjust for rollover….
    //We roll over about once a minute so it won't be uncommon.
    if (capture < last_edge) {
        capture += 0x40000000;
        last_edge += 0x40000000;
    }

    DWORD difference = capture - last_edge;

    if (difference > interval) {
        difference -= interval;
        last_edge_sec++;

    }
    //We only want to correct a maximum of two seconds.
    if (difference > interval) {
        difference -= interval;
        last_edge_sec++;

    }
    //If we are still out something is wrong
    if (difference > interval) {
        whyfail = "Difference failure<BR>";

        return FALSE;
    }

    //The last step is to calculate the following:
    //Assuming infinite floating point precision
    //fraction=difference/averageDifference
    //We want it to be a 32 bit fraction
    //fraction = (2^32)*fraction;
    //Doing this in floating point will not give us enough precision
    //So we do it with a longlong union

    UnsignedLongLong ull;
    ull.unsignedInt[0] = 1;//MSB
    ull.unsignedInt[1] = 0; //LSB

    ull.unsignedLongLong *= difference;
    ull.unsignedLongLong /= interval;
    dwSecs = last_edge_sec;
    dwFrac = ull.unsignedInt[1];
    return true;
}

void GPSNtp::ProcessNtpSecs(DWORD ParseIsr, DWORD ParseTime, int hour, int min, int sec, int day, int month, int year)
{
    DWORD edge = LastEdge;
    DWORD delta = ParseTime - edge;

    // Skip time updates that are more then 1 second old
    if (delta > 73000000) {
        whyfail = "TC Delta too high<BR>";
        bTimeIsValid = false;
        return; //We don't want to handle late strings.
    }


    // Year is in YY format
    year+=2000;
    if ((year > 2100) || (year < 2008)) {
        whyfail = "Year Parse fails<BR>";
        bTimeIsValid = false;
        return;
    }

    // Calculating seconds since epoch
    year = (year - 2008) * 12 + (month - 1);
    hour += (day - 1) * 24;
    min += 60 * hour;
    sec += 60 * min;
    DWORD ntp_sec = NTPsecs[year];
    ntp_sec += sec;

    // Recording epoch seconds and last edge reading
    NtpTimeSecs = ntp_sec;
    EdgeCountAtTime = edge;

    whyfail = "TimeValid";
    bTimeIsValid = true;
    if (valid)
        setLED(LED1_GREEN);
    else
        setLED(LED1_RED);

}


void GPSNtp::setLED(BYTE led) {
#ifdef PK70
    putleds(led);
#endif
}
