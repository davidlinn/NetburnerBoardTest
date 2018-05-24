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

#include "GPSLocation.h"

void GPSLocation::getFormattedLocation(char *buffer ) {
        siprintf(buffer, "%d, %d.%d %c", degrees, minutes,
                        minutesDecimal, direction);
}

void GPSLocation::getDecimal(char *buffer) {
        float d = degrees;
        float m = minutes;
        float s = minutesDecimal;
        float loc = d + ((m + (s/100000))/60);
        sprintf(buffer,"%c%f",sign,loc);
}


/* Converts a gps GGA location string in to decimal representations.
 * This string may be formatted in several different ways. Some examples:
 *  11723.2
 *   4501.665784
 *   8145.0050
 *  00845.1234
 *  Standard would be dddmm.fffff (decimal minutes, fractional). This function
 *  will account for some variance from GPS chips and attempt to parse them all
 *  correctly.
 */
void GPSLocation::setLocationFromGPGGAStr(const char * locationValue) {
        char buffer[32];
        strcpy(buffer,locationValue);
        char* decimal = strchr(buffer,'.');
        int degree_minutes = strtoul(buffer,&decimal,10);
        degrees = degree_minutes / 100;
        minutes = degree_minutes - (degrees*100);
        minutesDecimal = static_cast<int>(strtof(decimal,0)*100000)%100000;
}
