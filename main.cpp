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
 * MOD54415 FACTORY DEMO PROGRAM
 *----------------------------------------------------------------------------*/

#include <predef.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <startnet.h>
#include <autoupdate.h>
#include <dhcpclient.h>
#include <ethernet.h>
#include <networkdebug.h>
#include <taskmon.h>
#include <serial.h>
#include <effs_fat/multi_drive_mmc_mcf.h>
#include "webfuncs.h"
#include "tests.h"
#include "ip_util.h"
#include "GPS\GPSCHIP.h"

const char *AppName = "DavidTest";

extern "C"
{
    void UserMain(void *pd);
    void OSDumpTCBs(void);
}

/*
 * Diagnostic functions used in the command dispatcher
 */
void dEnet(void);
void ShowArp(void);
void ShowCounters(void);
void DumpTcpDebug(void);
void EnableTcpDebug(WORD);
void FTPServerInit(int prio);


/**
 * This function pings the address given in the buffer.
 */
void processPing(char *buffer)
{
    IPADDR addr_to_ping;
    char *cp = buffer;

    // Trim leading white space
    while (*cp && isspace(*cp)) { cp++; }

    // Get the address or use the default
    if (cp[0])
        addr_to_ping = AsciiToIp(cp);
    else
        addr_to_ping = EthernetIpGate;

    iprintf("\r\nPinging:  ");
    ShowIP(addr_to_ping);
    iprintf("\r\n");

    int rv = Ping(addr_to_ping, 1 /* ID */, 1 /* Seq */, 100 /* Max ticks */);

    if (rv == -1)
        iprintf("Failed!\r\n");
    else
        iprintf("Response took %d ticks.\r\n", rv);
}



/**
 * Display the Ethernet link status - this code is only valid in release builds
 * because the network builds use Ethernet debugging.
 */
void showEthernetLinkStatus(void)
{
    if (EtherLink())
    {
        iprintf("Link Status:  UP, ");

        if (EtherSpeed100())
            iprintf("100 Mbps, ");
        else
            iprintf("10 Mbps, ");

        if (EtherDuplex())
            iprintf("Full Duplex\r\n");
        else
            iprintf("Half Duplex\r\n");
    }
    else
    {
        iprintf("Link Status:  DOWN\r\n");
    }
}




/**
 * Display debug menu
 */
void displayMenu(void)
{
    iprintf("\r\n----- Main Menu -----\r\n");
    iprintf("A - Show ARP Cache\r\n");
    iprintf("B - Show IP Addresses\r\n");
    iprintf("C - Show Counters\r\n");
    iprintf("E - Show Ethernet Registers\r\n");

#ifdef UCOS_TASKLIST
    iprintf( "H - Show Task Changes\r\n" );
#endif // UCOS_TASKLIST

    iprintf("I - Change static IP settings\r\n");

#ifdef _DEBUG
    iprintf( "L - Set Up Debug Log Enable(s)\r\n" );
#endif // _DEBUG

    iprintf("P - Ping (Example: \"P 192.168.1.1\")\r\n");

#ifdef UCOS_STACKCHECK
    iprintf( "S - Show OS Task Stack Info\r\n" );
    iprintf( "U - Show OS Task Info\r\n" );
#endif // UCOS_STACKCHECK

#ifdef _DEBUG
    iprintf( "T - DumpTcpDebug\r\n" );
#endif // _DEBUG

    iprintf("W - Show OS Seconds Counter\r\n");
    iprintf("? - Display Menu\r\n");

    showEthernetLinkStatus();
}


/**
 * Handle commands with a simple command dispatcher
 */
void processCommand(char *buffer)
{
    switch (toupper(buffer[0]))
    {
      case 'A':
        ShowArp();
        break;

      case 'B':
        showIpAddresses();
        break;

      case 'C':
        ShowCounters();
        break;

      case 'E':
        dEnet();
        break;

#ifdef UCOS_TASKLIST
      case 'H':
        ShowTaskList();
        break;
#endif // UCOS_TASKLIST

      case 'I':
        SetupDialog();
        break;

#ifdef _DEBUG
      case 'L':
        SetLogLevel();
        break;
#endif // _DEBUG

      case 'P':
        processPing(buffer + 1);
        break;

      case '~':
        doManfTest();
        break;

      case '`':
        doDualMemCardTest();
        break;

#ifdef UCOS_STACKCHECK
      case 'U':
        OSDumpTasks();
        break;

      case 'S':
        OSDumpTCBStacks();
        break;

#endif // UCOS_STACKCHECK

#ifdef _DEBUG
      case 'T':
        DumpTcpDebug();
        break;
#endif // _DEBUG

      case 'W':
        iprintf("Tick Count = 0x%lX = %ld (%ld seconds)\r\n",
                TimeTick, TimeTick, TimeTick / TICKS_PER_SECOND);
        break;
      default: // '?'
        displayMenu();
    }
}

extern const char *default_page;

/**
 * The main task
 */
void UserMain(void *pd)
{
    InitializeStack();          // Initialize TCP/IP stack
    OSChangePrio(MAIN_PRIO);    // Set main task priority to the default (50)
    EnableAutoUpdate();

    // Wait for link before sending DHCP request
    uint32_t linkCount = 0;
    while ( !EtherLink() && ( linkCount < 10) )
    {
        OSTimeDly(TICKS_PER_SECOND);
        linkCount++;
    }

    GetDHCPAddressIfNecessary();
    showIpAddresses();
    //iprintf("Primary IP address: %I\r\n", (IPADDR)EthernetIP);

    StartHTTP();
    default_page = "index.html";
    EnableTaskMonitor();

    /*
     * The InitializeNetworkGDB functions are only valid when the application is
     * compiled in debug mode, which can be detected with the preprocessor
     * "#ifdef _DEBUG" definition.  InitializeNetworkGDB() will tell the GDB
     * setup on the NetBurner device to listen and then allow the application to
     * run normally.  InitializeNetworkGDB_and_Wait() will pause the application
     * until the debugger connects.
     */
#ifdef _DEBUG
    InitializeNetworkGDB();
    //InitializeNetworkGDB_and_Wait();
#endif // _DEBUG

    /*
     * The following call to f_enterFS() must be called in every task that
     * accesses the file system.  This must only be called once in each task
     * and must be done before any other file system functions are used.  Up to
     * 10 tasks can be assigned to use the file system.  Any task may also be
     * removed from accessing the file system with a call to the function
     * f_releaseFS().
     */
    f_enterFS();

    FTPServerInit(MAIN_PRIO - 2);   // Start the FTP server

    GPSCHIP gps(GPS_TIMER); //Initialize GPS
    gps.start();

    iprintf("%s\r\n", FirmwareVersion);
    displayMenu();

    char buffer[255];
    buffer[0] = '\0';

    while (1)
    {
        if (charavail())
        {
            gets(buffer);
            processCommand(buffer);
            buffer[0] = '\0';
        }
    }
}
