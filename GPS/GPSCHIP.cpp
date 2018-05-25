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

#include "GPSCHIP.h"

bool GPSCHIP::webCheck() {
	return CheckGPSCheckSum(gpsBuff);
}

void GPSCHIP::readLoop() {
    printf("Got to readLoop()\n");
	/*
     * Statement values:
     * 0: No statement
     * 1: GPGGA
     * 2: GPGSV
     * 3: GPRMC
     *
     */
    int statement = 0;

    char buff[512];
    char myBuff[2];
    char identifier[8];
    if(serialBaud == 0) {
        // Auto select by attempting to connect via each baudrate. A connection
        // is successful if a 10 consecutive ASCII characters are read
        bool found = false;
        int validBaudrates[] = {9600,4800,115200,57600,56000,38400,28800,19200,14400,2400};
        for (int br : validBaudrates) {
            fdIncoming = SimpleOpenSerial(serialPort, br );
            if(fdIncoming > 0) {
                int charsValid = 0;
                int charsRead = 0;
                while ((charsRead < 82*5) && (!found)) { //nmea max length = 82.
                    char c;
                    charsRead += ReadWithTimeout(fdIncoming,&c,1,TICKS_PER_SECOND * 1);
                    if (c > 0 && c < 128) {
                        charsValid++;
                    } else {
                        charsValid--;
                    }
                    if (charsValid > 100) {
                        found = true;
                    }

                }
            }
            if(!found) {
                SerialClose(serialPort);
            } else {
                printf("\nSerial Port found w/ baud rate trial and error.\n");
                break;
            }
        }
        if(!found) {
            iprintf("ERROR: Unable to auto-detect baudrate of GPS chip\r\n");
            while(1) {};
        }
    } else {
        fdIncoming = SimpleOpenSerial(serialPort, serialBaud );
        printf("\nSerial Port found simply.\n");
    }

    int parseState = 0;
    while (!allDone) {
        int numCharsRead = read(fdIncoming, myBuff, 1);
        //printf("Read %i characters from serial\n", numCharsRead);
        myBuff[1] = 0;
        if (myBuff[0] == '$') {
            identifier[0] = myBuff[0];
            parseState = 1;
            ntp.ParseIsr = ntp.IsrCount;
            ntp.ParseTime = ntp.timer.tcn;
        } else {
            switch (parseState) {
            case 0:
                break;
            case 1:
                identifier[1] = myBuff[0];
                parseState++;
                break;
            case 2:
                identifier[2] = myBuff[0];
                identifier[3] = 0;
                parseState++;
                break;
            case 3:
                if (myBuff[0] == 'G')
                    parseState++;
                else if (myBuff[0] == 'R')
                    parseState = 20;
                else
                    parseState = 0;
                break;
            case 4:
                if (myBuff[0] == 'G')
                    parseState++;
                else if (myBuff[0] == 'S')
                    parseState = 10;
                else
                    parseState = 0;
                break;
            case 5:
                if (myBuff[0] == 'A')
                    parseState++;
                else
                    parseState = 0;
                break;
            case 6:
                strcat(buff,identifier);
                strcat(buff, "GGA,");
                statement = 1;
                parseState = 98;
                break;

            case 10:
                if (myBuff[0] == 'V')
                    parseState++;
                else if (myBuff[0] == 'A')
                     parseState=12;
                else
                    parseState = 0;
                break;
            case 11:
                strcat(buff,identifier);
                strcat(buff, "GSV,");
                statement = 2;
                parseState = 98;
                break;
            case 12:
                strcat(buff,identifier);
                strcat(buff, "GSA,");
                statement = 4;
                parseState = 98;
                break;
            case 20:
                if (myBuff[0] == 'M')
                    parseState++;
                else
                    parseState = 0;
                break;
            case 21:
                if (myBuff[0] == 'C')
                    parseState++;
                else
                    parseState = 0;
                break;
            case 22:
                strcat(buff,identifier);
                strcat(buff, "RMC,");
                statement = 3;
                parseState = 98;
                break;

            case 98:
                if (myBuff[0] == '\r')
                    parseState = 99;
                strcat(buff, myBuff);
                break;
            case 99:
                printf("case 99\n");
            	if (myBuff[0] == '\n')
                    parseState = 0;
                else
                    parseState = 98;
                strcat(buff, myBuff);
                printf("Statement: %d\n", statement);
                printf("Buffer: %s\n", buff);
                for (int i = 0; i < 512; i++) //512 is buff size
                	gpsBuff[i] = buff[i];
                switch (statement) {
                case 0:
                    break;
                case 1:
                    DecodeGPGGA(buff);
                    break;
                case 2:
                    DecodeGPGSV(buff);
                    break;
                case 3:
                    DecodeGPRMC(buff);
                    break;
                case 4: //GSA
                    DecodeGPGSA(buff);
                    break;
                }
                char latBuff[50];
                char lonBuff[50];
                latitude.getFormattedLocation(latBuff);
                longitude.getFormattedLocation(lonBuff);
                printf("Sats: %d\nLat: %s\nLon: %s",sats,latBuff, lonBuff);
                memcpy(buff, "\0", strlen(buff));
                break;
            default:
                parseState = 0;
                break;
            }
            //printf("End Parse State: %d\n", parseState);
        }
        //OSTimeDly(TICKS_PER_SECOND * 1);
    }
}

void GPSCHIP::start() {
#ifdef NANO_NTP
    sim1.gpio.par_simp0l = 0x00;
    sim1.gpio.pddr_g |= 0x01;
    sim1.gpio.pclrr_g = ~0x01;
    OSTimeDly(2);
    sim1.gpio.ppdsdr_g = 0x01;
#endif
    OSTaskCreate(GPSCHIP::staticreadLoop, (void *) this,
            (void *) &GPSTaskStack[USER_TASK_STK_SIZE / 2],
            (void *) GPSTaskStack, priority);
    printf("Created staticreadloop task\n");
}

// staticreadLoop gives OSTaskCreate a target function to call.
void GPSCHIP::staticreadLoop(void * p) {
    printf("Entered staticreadloop\n");
	GPSCHIP * po;
    po = (GPSCHIP*) p;
    printf("About to call readLoop()\n");
    po->readLoop();
}

BYTE GPSCHIP::GetGPSCheckSum(const char* GPSString) {
    BYTE csum = 0;
    GPSString++;
    for (int i = 0; (i < 100) && (*GPSString != '*'); i++)
        csum ^= *GPSString++;
    GPSString++;
    return csum;
}

BOOL GPSCHIP::CheckGPSCheckSum(const char* GPSString) {
    BYTE csum = 0;
    GPSString++;
    for (int i = 0; (i < 100) && (*GPSString != '*'); i++)
        csum ^= *GPSString++;
    GPSString++;
    if (strtoul(GPSString, NULL, 16) == csum)
        return TRUE;
    else
        return FALSE;
}

/*
 * GPGGA is a string separated by commas. It can be described as
 * ------------------------------------------
 * | Name
 * ------------------------------------------
 * | Identifier
 * | Time
 * | Latitude
 * | Latitude Direction
 * | Longitude
 * | Longitude Direction
 * | Fix Quality
 * | Number of Satellites in use
 * | HDOP
 * | Altitude
 * | Altitude Format
 * | Height of geoID
 * | Height of geoID Format
 * | Reference Station ID+Checksum
 * ------------------------------------------
 */

void GPSCHIP::DecodeGPGGA(char* pGPSData) {
    char* t_pdata;
    if (pGPSData != NULL) {
        if (CheckGPSCheckSum(pGPSData)) {
            // Identifier
            t_pdata = strsep(&pGPSData, ",");
            // Time
            t_pdata = strsep(&pGPSData, ",");
            time.setTimeFromGPGGAStr(t_pdata);
            ntp.setTimeFromGPGGAStr(t_pdata);
            ntp.ProcessNtpSecs( ntp.ParseIsr,
                                ntp.ParseTime,
                                ntp.hours,
                                ntp.minutes,
                                ntp.seconds,
                                ntp.day,
                                ntp.month,
                                ntp.year
                               );
            // Latitude
            t_pdata = strsep(&pGPSData, ",");
            latitude.setLocationFromGPGGAStr(t_pdata);
            // Latitude Direction
            t_pdata = strsep(&pGPSData, ",");
            latitude.direction = *t_pdata;
            if (*t_pdata == 'N')
                latitude.sign = '+';
            else if (*t_pdata == 'S')
                latitude.sign = '-';
            else
                latitude.sign = '0';
            // Longitude
            t_pdata = strsep(&pGPSData, ",");
            longitude.setLocationFromGPGGAStr(t_pdata);
            // Longitude Direction
            t_pdata = strsep(&pGPSData, ",");
            longitude.direction = *t_pdata;
            if (*t_pdata == 'E')
                longitude.sign = '+';
            else if (*t_pdata == 'W')
                longitude.sign = '-';
            else
                longitude.sign = '0';
            // Fix Quality
            t_pdata = strsep(&pGPSData, ",");
            // Satellites in use
            t_pdata = strsep(&pGPSData, ",");
            sats = strtoul(t_pdata, NULL, 10);
            // HDOP
            t_pdata = strsep(&pGPSData, ",");
            // Altitude
            t_pdata = strsep(&pGPSData, ",");
            // Altitude format
            t_pdata = strsep(&pGPSData, ",");
            // Height of GeoID
            t_pdata = strsep(&pGPSData, ",");
            // Height of GeoID format
            t_pdata = strsep(&pGPSData, ",");
            // Ref Station + Checksum
            t_pdata = strsep(&pGPSData, ",");

            t_pdata = strsep(&pGPSData, ",");

        }
    }
}

/*
 * GPGSV is a string separated by commas. It can be described as
 * ------------------------------------------
 * | Name
 * ------------------------------------------
 * | Identifier
 * | Total messages
 * | Message number
 * | Sats in View
 * | SV PRN Number
 * | SV Elevation [0-90]
 * | SV Azimuth [000-359]
 * | SV SNR [00-99 dB]
 * | 8-11 (Second SV, same as fields 4-7
 * | 12-15 (Third SV, same as fields 4-7
 * | 16-19 (Fourth SV, same as fields 4-7
 * | Checksum
 * ------------------------------------------
 */

void GPSCHIP::DecodeGPGSV(char* pGPSData) {

    char* t_pdata;

    if (pGPSData != NULL) {
        if (CheckGPSCheckSum(pGPSData)) {
            int satNum = 0;
            int satView = 0;

            // Identifier
            t_pdata = strsep(&pGPSData, ",");
            // Total messages
            t_pdata = strsep(&pGPSData, ",");
            // Message number
            t_pdata = strsep(&pGPSData, ",");
            satNum = (strtoul(t_pdata, NULL, 10)-1) * 4;
            // Sats in View
            t_pdata = strsep(&pGPSData, ",");
            satView = strtoul(t_pdata, NULL, 10);
            int x = (satView - satNum);
            //for(int i=0;i<=((x<3)?x:3);i++) {
            for (int i = ((x > 3) ? 3 : (x - 1)); i >= 0; i--) {
                // SV PRN
                t_pdata = strsep(&pGPSData, ",");
                int satPrn = strtoul(t_pdata, NULL, 10);
                if (satPrn >= 64 ) { // Only save GPS satellites
                    break;
                }
                visibleSat[satNum].prn = satPrn;
                // SV Elevation [0-90]
                t_pdata = strsep(&pGPSData, ",");
                visibleSat[satNum].elevation = strtoul(t_pdata, NULL, 10);
                // SV Azimuth [000-359]
                t_pdata = strsep(&pGPSData, ",");
                visibleSat[satNum].azimuth = strtoul(t_pdata, NULL, 10);
                // SV SNR [00-99 dB]
                t_pdata = strsep(&pGPSData, ",*");
                visibleSat[satNum].snr = strtoul(t_pdata, NULL, 10);
                satNum++;
            }
        }
    }

}

/*
 * GPGSV is a string separated by commas. It can be described as
 * ------------------------------------------
 * | Name
 * ------------------------------------------
 * | Identifier
 * | Time Stamp
 * | validity - A-ok, V-invalid
 * | current Latitude
 * | North/South
 * | current Longitude
 * | East/West
 * | Speed in knots
 * | True course
 * | Date Stamp
 * | Variation
 * | East/West
 * | Checksum
 * ------------------------------------------
 */

void GPSCHIP::DecodeGPRMC(char* pGPSData) {

    char* t_pdata;
    if (pGPSData != NULL) {
        if (CheckGPSCheckSum(pGPSData)) {
            // Identifier
            t_pdata = strsep(&pGPSData, ",");
            // Time Stamp
            t_pdata = strsep(&pGPSData, ",");
            // validity - A-ok, V-invalid
            t_pdata = strsep(&pGPSData, ",");
            if (*t_pdata == 'A') {
                ntp.valid = true;
                ntp.ValidRMCInARow++;
            } else {
                ntp.valid = false;
                ntp.ValidRMCInARow = 0;
            }
            // current Latitude
            t_pdata = strsep(&pGPSData, ",");
            // North/South
            t_pdata = strsep(&pGPSData, ",");
            // current Longitude
            t_pdata = strsep(&pGPSData, ",");
            // East/West
            t_pdata = strsep(&pGPSData, ",");
            // Speed in knots
            t_pdata = strsep(&pGPSData, ",");
            // True course
            t_pdata = strsep(&pGPSData, ",");
            // Date Stamp
            t_pdata = strsep(&pGPSData, ",");
            ntp.setDateFromGPGGAStr(t_pdata);
            // Variation
            t_pdata = strsep(&pGPSData, ",");
            // East/West
            t_pdata = strsep(&pGPSData, ",");
            // Checksum
            t_pdata = strsep(&pGPSData, "*");
        }
    }

}

/*

     GSA      Satellite status
     A        Auto selection of 2D or 3D fix (M = manual) 
     3        3D fix - values include: 1 = no fix
                                       2 = 2D fix
                                       3 = 3D fix
     04,05... PRNs of satellites used for fix (space for 12) 
     2.5      PDOP (dilution of precision) 
     1.3      Horizontal dilution of precision (HDOP) 
     2.1      Vertical dilution of precision (VDOP)
     *39      the checksum data, always begins with *
*/


/*
 * GPGSA is a string separated by commas. It can be described as
 * ------------------------------------------
 * | Name
 * ------------------------------------------
 * | Identifier
 * | Auto selection of 2D or 3D fix (M = manual)
 * | 3D fix - values include: 1 = no fix
 * |                          2 = 2D fix
 * |                          3 = 3D fix
 * | 1 PRN of satellites used for fix
 * | 2 PRN of satellites used for fix
 * | 3 PRN of satellites used for fix
 * | 4 PRN of satellites used for fix
 * | 5 PRN of satellites used for fix
 * | 6 PRN of satellites used for fix
 * | 7 PRN of satellites used for fix
 * | 8 PRN of satellites used for fix
 * | 9 PRN of satellites used for fix
 * | 10 PRN of satellites used for fix
 * | 11 PRN of satellites used for fix
 * | 12 PRN of satellites used for fix
 * | PDOP (dilution of precision)
 * | Horizontal dilution of precision (HDOP)
 * | Vertical dilution of precision (VDOP)
 * | Checksum
 * ------------------------------------------
 */


void GPSCHIP::DecodeGPGSA(char* pGPSData) {

    char* t_pdata;
    if (pGPSData != NULL) {
        if (CheckGPSCheckSum(pGPSData)) {
            // Identifier
            t_pdata = strsep(&pGPSData, ",");
            // Selection of fix
            t_pdata = strsep(&pGPSData, ",");
            // 3D Fix
            t_pdata = strsep(&pGPSData, ",");
            int satNum = 0;
            for(int i = 0;i<12;i++) {
                // Sat PRN utilitized for location data
                t_pdata = strsep(&pGPSData, ",");
                if(strlen(t_pdata) > 0) {
                    int prn = strtoul(t_pdata, NULL, 10);
                    if(prn <= 64) {
                        for (int i = 0; i < 32; i++) {
                            if(visibleSat[i].prn == prn) {
                                sat[satNum].prn = visibleSat[i].prn;
                                sat[satNum].elevation = visibleSat[i].elevation;
                                sat[satNum].azimuth = visibleSat[i].azimuth;
                                sat[satNum].snr = visibleSat[i].snr;
                                satNum++;
                                break;
                            }
                        }
                    }
                }
            }
            // PDOP
            t_pdata = strsep(&pGPSData, ",");
            // HDOP
            t_pdata = strsep(&pGPSData, ",");
            // VDOP
            t_pdata = strsep(&pGPSData, ",");
            // Checksum
            t_pdata = strsep(&pGPSData, "*");
        }
    }

}

char * GPSCHIP::getGPS() {
    static char buff[256];
    sniprintf(buff, 256, "%d,%d,%d,%c,%d,%d,%d,%c,%c,%d,%d,%d,%c", time.hours,
            time.minutes, time.seconds, latitude.sign, latitude.degrees,
            latitude.minutes, latitude.minutesDecimal, latitude.direction,
            longitude.sign, longitude.degrees, longitude.minutes,
            longitude.minutesDecimal, longitude.direction);
    return buff;
}
