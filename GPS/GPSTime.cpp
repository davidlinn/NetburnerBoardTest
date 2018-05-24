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


#include "GPSTime.h"

void GPSTime::getFormatedTime(char * buffer) {
    siprintf(buffer, "%d:%.2d:%.2d", hours, minutes, seconds);
}

void GPSTime::setTimeFromGPGGAStr(const char * timeValue) {
    char TempStr[10];
    memcpy(TempStr, timeValue, 2);
    hours = strtoul(TempStr, NULL, 10);
    memcpy(TempStr, timeValue + 2, 2);
    minutes = strtoul(TempStr, NULL, 10);
    memcpy(TempStr, timeValue + 4, 2);
    seconds = strtoul(TempStr, NULL, 10);
}

