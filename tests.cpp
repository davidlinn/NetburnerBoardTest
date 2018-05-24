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

/*------------------------------------------------------------------------------
 * MOD5441x Test Functions
 *----------------------------------------------------------------------------*/
#include <predef.h>
#include <stdio.h>
#include <stdlib.h>
#include <iosys.h>
#include <ctype.h>
#include <string.h>
#include <ethernet.h>
#include <tcp.h>
#include <udp.h>
#include <serial.h>
#include <effs_fat/multi_drive_mmc_mcf.h>
#include <sim5441x.h>

#define MAX_EFFS_ERRORCODE   ( 38 )
#define MMC_OFF_BOARD        (  0 )   // SD Card reader on MOD-DEV-70 and -100
#define MMC_ON_BOARD         (  1 )   // MicroSD Card reader on MOD54415


char EffsErrorCode[][80] =
{
  "F_NO_ERROR",                // 0
  "F_ERR_INVALIDDRIVE",        // 1
  "F_ERR_NOTFORMATTED",        // 2
  "F_ERR_INVALIDDIR",          // 3
  "F_ERR_INVALIDNAME",         // 4
  "F_ERR_NOTFOUND",            // 5
  "F_ERR_DUPLICATED",          // 6
  "F_ERR_NOMOREENTRY",         // 7
  "F_ERR_NOTOPEN",             // 8
  "F_ERR_EOF",                 // 9
  "F_ERR_RESERVED",            // 10
  "F_ERR_NOTUSEABLE",          // 11
  "F_ERR_LOCKED",              // 12
  "F_ERR_ACCESSDENIED",        // 13
  "F_ERR_NOTEMPTY",            // 14
  "F_ERR_INITFUNC",            // 15
  "F_ERR_CARDREMOVED",         // 16
  "F_ERR_ONDRIVE",             // 17
  "F_ERR_INVALIDSECTOR",       // 18
  "F_ERR_READ",                // 19
  "F_ERR_WRITE",               // 20
  "F_ERR_INVALIDMEDIA",        // 21
  "F_ERR_BUSY",                // 22
  "F_ERR_WRITEPROTECT",        // 23
  "F_ERR_INVFATTYPE",          // 24
  "F_ERR_MEDIATOOSMALL",       // 25
  "F_ERR_MEDIATOOLARGE",       // 26
  "F_ERR_NOTSUPPSECTORSIZE",   // 27
  "F_ERR_DELFUNC",             // 28
  "F_ERR_MOUNTED",             // 29
  "F_ERR_TOOLONGNAME",         // 30
  "F_ERR_NOTFORREAD",          // 31
  "F_ERR_DELFUNC",             // 32
  "F_ERR_ALLOCATION",          // 33
  "F_ERR_INVALIDPOS",          // 34
  "F_ERR_NOMORETASK",          // 35
  "F_ERR_NOTAVAILABLE",        // 36
  "F_ERR_TASKNOTFOUND",        // 37
  "F_ERR_UNUSABLE"             // 38
};



/**
 * Displays the string representation of standard EFFS numerical return codes
 */
void displayEffsErrorCode(int code)
{
   if (code <= MAX_EFFS_ERRORCODE)
      iprintf("%s\r\n", EffsErrorCode[code]);
   else
      iprintf("Unknown EFFS error code %d\r\n", code);
}


/**
 * Performs a simple test of either the on-board uSD or off-board SD card reader
 */
void doSingleMemCardTest(int mmc_drive)
{
    const char *driveLabel;

    if (mmc_drive == MMC_OFF_BOARD)
        driveLabel = "Off-board Card Reader";
    else if (mmc_drive == MMC_ON_BOARD)
        driveLabel = "On-board Card Reader";
    else
    {
        driveLabel = "Unidentified Card Reader";
        iprintf("ERROR  -- %s used\r\n", driveLabel);
        return;
    }

    // Card detection check
    if (get_cd(mmc_drive) != 0)
    {
        // Write protection check
        if (get_wp(mmc_drive) == 0)
        {
            int rv;

            if (mmc_drive == MMC_OFF_BOARD)
                rv = f_mountfat(mmc_drive, mmc_initfunc, F_MMC_DRIVE0);
            else // (mmc_drive == MMC_ON_BOARD)
                rv = f_mountfat(mmc_drive, mmc_initfunc, F_MMC_DRIVE1);

            if (rv == F_NO_ERROR)
            {
                rv = f_chdrive(mmc_drive);

                if (rv == F_NO_ERROR)
                {
                    F_FILE *fp = f_open( "TestFile.txt", "w+");

                    if (fp)
                    {
                        const char *cp = "Hello Memory Card!";
                        f_write(cp, 1, strlen(cp), fp);
                        f_close(fp);
                    }

                    rv = f_delvolume(mmc_drive);

                    if (rv == F_NO_ERROR)
                    {
                        iprintf("PASS   ++ %s test successful\r\n", driveLabel);
                    }
                    else
                    {
                        iprintf("ERROR  -- Failed to delete volume:  ");
                        displayEffsErrorCode(rv);
                    }
                }
                else
                {
                    iprintf("ERROR  -- Failed to change drive:  ");
                    displayEffsErrorCode(rv);
                }
            }
            else
            {
                iprintf("ERROR  -- Failed to mount drive:  ");
                displayEffsErrorCode(rv);
            }
        }
        else
        {
            iprintf("ERROR  -- %s has write-protection enabled\r\n", driveLabel);
        }
    }
    else
    {
        iprintf("ERROR  -- %s is empty\r\n", driveLabel);
    }
}


/**
 * Tests with the off-board and on-board card readers
 */
void doDualMemCardTest(void)
{
    iprintf("\r\nSTATUS -- Testing on-board uSD Card reader...\r\n");
    doSingleMemCardTest(MMC_ON_BOARD);

    iprintf("STATUS -- Testing off-board SD Card reader...\r\n");
    doSingleMemCardTest(MMC_OFF_BOARD);

    iprintf("STATUS -- End of dual memory card reader test\r\n\r\n");
}


/**
 * Used as part of a bring-up test in manufacturing.
 */
void doManfTest(void)
{
    char buffer[10];

    int fd1 = OpenSerial(1, 115200, 2, 8, eParityNone);
    buffer[0] = ' ';

    do
    {
        read(fd1, buffer, 1);
        write(fd1, buffer, 1);
    } while (buffer[0] != 'x');

    close(fd1);
}



