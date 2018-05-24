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

#include "GPSSat.h"

void GPSSat::getXYvalues(int size, int * buffer) {
    int length = 90 - elevation;
    int x = 0;
    int y = 0;

    if ( azimuth >= 0 && azimuth < 90 ) {
        x = sin(azimuth*PI/180) * length;
        x = (x*size)/180;
        x = (size/2) + x;
        y = cos(azimuth*PI/180) * length;
        y = (y*size)/180;
        y = (size/2) - y;
    }
    else if ( azimuth >= 90 && azimuth < 180 ) {
        x = cos((azimuth - 90)*PI/180) * length;
        x = (x*size)/180;
        x = (size/2) + x;
        y = sin((azimuth - 90)*PI/180) * length;
        y = (y*size)/180;
        y = (size/2) + y;
    }
    else if ( azimuth >= 180 && azimuth < 270 ) {
        x = sin((azimuth - 180)*PI/180) * length;
        x = (x*size)/180;
        x = (size/2) - x;
        y = cos((azimuth - 180)*PI/180) * length;
        y = (y*size)/180;
        y = (size/2) + y;
    }
    else if ( azimuth >= 270 && azimuth <= 359 ) {
        x = cos((azimuth - 270)*PI/180) * length;
        x = (x*size)/180;
        x = (size/2) - x;
        y = sin((azimuth - 270)*PI/180) * length;
        y = (y*size)/180;
        y = (size/2) - y;
    }
    else {
        x = 0;
        y = 0;
    }
    buffer[0] = x;
    buffer[1] = y;
}
