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
 * File:         webfuncs.cpp
 * Description:  Dynamic web server content functions.
 *----------------------------------------------------------------------------*/

#include <predef.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <startnet.h>
#include <autoupdate.h>
#include <dhcpclient.h>
#include <tcp.h>
#include <udp.h>
#include <../MOD5441X/include/sim5441x.h>
#include <pins.h>
#include "simpleAD.h"
#include "webfuncs.h"
#include <HiResTimer.h>
#include "GPS\GPSCHIP.h"



/*
 * Functions linked to web FUNCTIONCALL tags.
 */
extern "C"
{
   void WebLeds( int sock, PCSTR url );
   void DoSwitches( int sock, PCSTR url );
   void WebTickTacToe( int sock, PCSTR url );
   void DisplayFirmwareVersion( int sock, PCSTR url );
   void WebGPS( int sock, PCSTR url );
}
extern GPSCHIP gps;


char buffer[255];

//display GPS coordinates on web page
void WebGPS( int sock, PCSTR url ) {
	char latBuffer[80];
	char lonBuffer[80];
	gps.latitude.getDecimal(latBuffer);
	gps.longitude.getDecimal(lonBuffer);
	writestring(sock, "GPS Coordinates: (");
	writestring(sock, latBuffer);
	writestring(sock, ",");
	writestring(sock, lonBuffer);
	writestring(sock, ")");

	writestring(sock, "<br>");
	writestring(sock, "Num Sats: ");
	char satsBuffer[10];
	sprintf(satsBuffer, "%d", gps.sats);
	writestring(sock, satsBuffer);

	char mapHTMLBuff[1500];
	sprintf(mapHTMLBuff,"<p> <iframe width=\"600\" height=\"450\""
			"frameborder=\"0\" style=\"border:0\""
			"src=\"https://www.google.com/maps/embed/v1/place"
				"?key=AIzaSyBvtk5gC31rbCfnETbWBL8EFalLJ7esgqM&q=%s,%s&maptype=satellite\""
				"allowfullscreen></iframe></p>", latBuffer, lonBuffer);
	printf(mapHTMLBuff);
	writestring(sock,mapHTMLBuff);
}


/*-------------------------------------------------------------------
 * On the MOD-DEV-70, the LEDs are on J2 connector pins:
 * 15, 16, 31, 23, 37, 19, 20, 24 (in that order)
 * -----------------------------------------------------------------*/
void WriteLeds( BYTE LedMask )
{
   static BOOL bLedGpioInit = FALSE;
   const BYTE PinNumber[8] = { 15, 16, 31, 23, 37, 19, 20, 24 };
   BYTE BitMask = 0x01;

   if ( ! bLedGpioInit )
   {
      for ( int i = 0; i < 8; i++ )
         J2[PinNumber[i]].function( PIN_GPIO );
      bLedGpioInit = TRUE;
   }


   for ( int i = 0; i < 8; i++ )
   {
      if ( (LedMask & BitMask) == 0 )
         J2[PinNumber[i]] = 1;  // LEDs tied to 3.3V, so 1 = off
      else
         J2[PinNumber[i]] = 0;

      BitMask <<= 1;
   }
}


/*-------------------------------------------------------------------
 * This function displays a set of LED indicators on a web page that
 * can allow a user to turn the LEDs on and off through a web browser.
 * The state of the LED's is communicated from one HTML page to
 * another through the number appended on the base URL.
 * ------------------------------------------------------------------*/
void WebLeds( int sock, PCSTR url )
{
   int32_t urlEncodeOffset = 8;

   // The URL will have the format: "LED.HTM?number", where the value
   // of "number" will store the LED information. The following code
   // will test for the '?' character. If one exists, then the
   // number following the '?' is assumed to contain a value
   // indicating which LEDs are lit. If no '?' is in the URL, then
   // the LEDs are initialized to a known state.
   int v;
   if ( ( *( url + urlEncodeOffset ) ) == '?' )
      v = atoi( ( char * ) ( url + urlEncodeOffset + 1 ) );
   else
      v = 0xAA;

   WriteLeds( (BYTE) v & 0xFF );

   // Now step through each LED position and write out an HTML table element
   for ( int i = 1; i < 256; i = i << 1 )
   {
      char buffer[80];

      if ( v & i )
      {
         // The LED is currently on, so we want to output the following HTML code:
         // <td><A href="LED.HTML?#"><img src="ON.GIF"></A></td>, where '#' is the
         // value of the URL that would be identical to the current URL, but with
         // this particular LED off. That way, when the viewer of the Web page
         // clicks on this LED, it will reload a page with this LED off and all
         // other LEDs remain unchanged.
         sniprintf( buffer, 80, "<td><A href=\"LED.HTML?%d\" ><img src=\"images/ON.GIF\"> </a></td>", v & ( ~i ) );
      }
      else
      {
         // The LED is currently off, so we want to output the following HTML code:
         // <td><A href="LED.HTM?#"><img src="OFF.GIF"></A></td>, where '#' is the
         // value of the URL that would be identical to the current URL, but with
         // this particular LED on. That way, when the viewer of the web page
         // clicks on this LED, it will reload a page with this LED on and all
         // other LEDs remain unchanged.
         sniprintf( buffer, 80, "<td><A href=\"LED.HTML?%d\" ><img src=\"images/OFF.GIF\"> </a></td>", v | i );
      }
      writestring( sock, buffer );
   }
}


/*-------------------------------------------------------------------
 * On the MOD-DEV-70, the switches are on J2 connector pins:
 * 8, 6, 7, 10, 9, 11, 12, 13 (in that order). These signals are
 * Analog to Digital, not GPIO, so we read the analog value and
 * determine the switch position from it.
 * ------------------------------------------------------------------*/
BYTE ReadSwitch()
{
   static BOOL bReadSwitchInit = FALSE;
   const BYTE PinNumber[8] = { 7, 6, 5, 3, 4, 1, 0, 2 };  // map J2 conn signals pins to A/D number 0-7

   BYTE BitMask = 0;

   if ( ! bReadSwitchInit )
   {
      InitSingleEndAD();
      bReadSwitchInit = TRUE;
   }

   StartAD();
   while( !ADDone() )  asm("nop");

   for ( int BitPos = 0x01, i = 0; BitPos < 256; BitPos <<= 1, i++ )
   {
      // if greater than half the 16-bit range, consider it logic high
      if ( GetADResult(PinNumber[i]) > ( 0x7FFF / 2) )
         BitMask |= (BYTE)(0xFF & BitPos);

      //iprintf("ADC[%d] = %04X, BitPos: %02X, BitMask: %02X\r\n", PinNumber[i], GetADResult(PinNumber[i]), BitPos, BitMask );
   }

   return BitMask;
}




/*-------------------------------------------------------------------
 This function displays a set of dip switch indicators on a web page.
 The state of each switch is represented by a bit in a 8-bit
 register. A bit value of 0 = on, and 1 = off.
  ------------------------------------------------------------------*/
void DoSwitches( int sock, PCSTR url )
{
   const int NumSwitches = 8;

   // Get the value of the switches
   BYTE sw = ReadSwitch();
   iprintf( "Switch Register: 0x%02X\r\n", sw );


   // Write out each row of the table
   for ( int i = 1; i <= NumSwitches ; i++ )
   {
      char buffer[80];
      if ( sw & ( 0x80 >> ( i - 1 ) ) )
      {
         // Switch is on
         sniprintf( buffer, 80, "<tr><td>Switch %d: <b>OFF</b> </td></tr>", i );
      }
      else
      {
         // Switch is off
         sniprintf( buffer, 80, "<tr><td>Switch %d: <b>ON</b> </td></tr>", i );
      }
      writestring( sock, buffer );
   }

   // End the table
   //writestring( sock, "</table><br>" );

   // Put in a link that reloads the page
   writestring( sock, "<A HREF=\"" );
   writestring( sock, url );
   writestring( sock, "\"> Refresh Switches </A>" );
}





/******************************************************************************
 *
 *
 *
 *                TICTACTOE from here on
 *
 *
 *General Notes:
 * The comments all refer to he and I he is the human playing the game and
 * I is this program
 * The state of the Game is stored in the URL. The url will read like:
 *
 *           TT.HTM?number
 *
 * Where is a number that encodes the state of the Game...
 * This number is an or of all the square values...
 * X's (His values)
 *           0x01    0x04   0x10
 *           0x40    0x100  0x400
 *           0x1000  0x4000 0x10000
 *
 *
 *
 * O's (My values)
 *
 *           0x02    0x08  0x20
 *           0x80    0x200  0x800
 *           0x2000  0x8000 0x20000
 *
 *
 *
 *
******************************************************************************/

//Returns true if the User wins.... Should never happen!
int hewins( int i )
{
   return ( ( ( i & 0x1041 ) == 0x1041 ) ||
            ( ( i & 0x4104 ) == 0x4104 ) ||
            ( ( i & 0x10410 ) == 0x10410 ) ||
            ( ( i & 0x15 ) == 0x15 ) ||
            ( ( i & 0x540 ) == 0x540 ) ||
            ( ( i & 0x15000 ) == 0x15000 ) ||
            ( ( i & 0x10101 ) == 0x10101 ) ||
            ( ( i & 0x1110 ) == 0x1110 ) );
}


//Test to see if I (The computer wins)
int iwin( int i )
{
   return hewins( i >> 1 );
}




//The prefered moves....
const int perfmovesm[] =
{
  512, 8, 128, 2048, 32768, 2, 32, 8192, 0x20000
};


/*The cell numbers
1,2          4,8           16,32

64,128     256,512,      1024,2048

4096,8192 16384,32768, 0x10000,0x20000


x..             x.x
.*.      -> .*.  65794 -> |=32
..*             ..*

x..             x.*
.*.     and .*.  Automatic block
*..             ...


*/


//Given the present board state return the next board state... (Our move)
int move( int now )
{
   //Opening book
   switch ( now )
   {
     case 1:
     case 64:
     case 4096:
     case 0x10000:
       return 512 | now;
     case 4612:
     case 1540:
     case 65794:
       return now |= 32;
     case 16984:
       return now |= 8192;
     case 16960:
       return now |= 8192;
     case 16912:
     case 17929:
       return now |= 0x20000;
     case 17920:
       return now |= 0x20000;
     case 66049:
       return now |= 32768;
     case 4672:
     case 256:
     case 532:
     case 580:
       return 2 | now;
   }

   int bm = 0;
   int pm;
   for ( int im = 0; im < 9; im++ )
   {
      pm = perfmovesm[im];
      if ( ( ( pm & now ) == 0 ) && ( ( ( pm >> 1 ) & now ) == 0 ) )
      {
         //The move is legal
         int hw = 0;
         int tnow = ( now | pm );
         if ( iwin( tnow ) )
         {
            return ( tnow );
         }
         bm = pm;
         for ( int hm = 1; hm < ( 1 << 18 ); hm = hm << 2 )
         {
            if ( ( ( hm & tnow ) == 0 ) && ( ( ( hm << 1 ) & tnow ) == 0 ) )
            {
               if ( hewins( hm | tnow ) )
               {
                  hw = 1;
               }
            }
         }//For hm
         if ( hw == 0 )
         {
            return tnow;
         }
      }//for pm
   }
   return bm | now;
}



//This function fills in an HTML table with the TicTacToe game in it.
void WebTickTacToe( int sock, PCSTR url )
{
   int v;
   iprintf( "Doing TTT for [%s]\r\n", url );

   int32_t urlEncodeOffset = 7;

   //Get a buffer to store outgoing data....
   PoolPtr pp = GetBuffer();
   pp->usedsize = 0;
   char *po = ( char * ) pp->pData;

   if ( ( *( url + urlEncodeOffset ) ) == '?' )
   {
      v = atoi( ( char * ) ( url + urlEncodeOffset + 1 ) );
   }
   else
   {
      v = 0;
   }



   //There are nine squares each with three states yours,mine,empty
   //00 ==empty
   //01 == yours
   //10 == mine
   //11 ==bad

   iprintf( "Tick Tack Toe V= from %d to ", v );
   int orgv = v;
   if ( ( v != 0 ) && ( !hewins( v ) ) )
   {
      v = move( v );
   }
   iprintf( "%d\n", v );


   int vshift = v;
   int won = 0;

   //Write out the HTML Table header stuff...
   if ( hewins( v ) )
   {
      won = 1;
      writestring( sock,
                   "<tr><td colspan=3>You win!<A HREF=\"TT.HTML\">(Again?) </A></td></tr>\n" );
   }
   else if ( iwin( v ) )
   {
      won = 1;
      writestring( sock,
                   "<tr><td colspan=3>I win!<A HREF=\"TT.HTML\">(Again?) </A></td></tr>\n" );
   }
   else if ( ( orgv ) && ( orgv == v ) )
   {
      won = 1;
      writestring( sock,
                   "<tr><td colspan=3>No One Wins! <BR><A HREF=\"TT.HTML\">Again? </A></td></tr>\n" );
   }



   //Now write out the board
   for ( int i = 0; i < 9; i++ )
   {
      //The begining of a row
      if ( ( i % 3 ) == 0 )
      {
         append( po, ( char * ) "<TR>" );
      }
      append( po, ( char * ) "<TD>" );

      switch ( vshift & 3 )
      {
        case 0:
          //IF the game is not over include the HREF for the next move...
          //Only if the square is blank
          if ( !won )
          {
             siprintf( po,
                       "<A HREF = \"TT.HTML?%d\"><IMG SRC=\"images/BLANK.GIF\"></A>",
                       v + ( 1 << ( 2 * i ) ) );
             while ( *po )
             {
                po++;
             }
          }
          else
          {
             append( po, ( char * ) "<IMG SRC=\"images/BLANK.GIF\">" );
          }
          break;
        case 1:
          //The square belongs to the evil human!
          append( po, ( char * ) "<IMG SRC=\"images/CROSS.GIF\">" );
          break;
        case 2:
          //The square is Mine all mine
          append( po, ( char * ) "<IMG SRC=\"images/NOT.GIF\">" );
          break;
      }//Switch
      //Now shift the next square into view...
      vshift = vshift >> 2;
      //Add the element end
      append( po, ( char * ) "</TD>" );
      //If it is the end of the row add the row end...
      if ( ( i % 3 ) == 2 )
      {
         append( po, ( char * ) "</TR>\n" );
      }
   }

   //We have written all of this stuff into the Pool buffer we alocated.
   //No we actually write it to the socket...
   pp->usedsize = ( ( unsigned char * ) po - ( pp->pData ) );
   write( sock, ( char * ) pp->pData, pp->usedsize );
   //Now free our storage...
   FreeBuffer( pp );
   return;
}


/*
 * Display the factory demo application revision number and compilation date
 * in index.htm.
 */
void DisplayFirmwareVersion( int sock, PCSTR url )
{
   writestring( sock, FirmwareVersion );
}
