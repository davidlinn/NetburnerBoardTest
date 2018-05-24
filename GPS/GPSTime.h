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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef GPSTIME_H_
#define GPSTIME_H_

class GPSTime {
public:
    int hours;
    int minutes;
    int seconds;

    GPSTime() : hours(0), minutes(0), seconds(0) {}

    void getFormatedTime(char * buffer);
    void setTimeFromGPGGAStr(const char * timeValue);
};

#endif /* GPSTIME_H_ */
