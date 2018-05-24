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
 * File:         ftps.cpp
 * Description:  FTP server functions.
 *
 * This demo program also allows a FTP client to download information.  The FTP
 * server is automatically started when the program runs.  You can then download
 * a file via FTP that contains time information.  Here is the procedure:
 *
 *    1. Start a FTP client and connect to the FTP server.  For example, open a
 *       command prompt and type "FTP <IP ADDR>", where the IP ADDR field is the
 *       IP address of the NetBurner demo board.
 *    2. Enter any user name and password.
 *    3. Type "ls" to list available files for download.
 *    4. Type "get <filename>" to retrieve a file.
 *    5. Now view the files to see the data.
 *----------------------------------------------------------------------------*/

#include <predef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <startnet.h>
#include <autoupdate.h>
#include <ftpd.h>
#include <htmlfiles.h>
#include <fdprintf.h>


typedef struct
{
      unsigned long dwBlockRamStart ;
      unsigned long dwExecutionAddr ;
      unsigned long dwBlockSize     ;
      unsigned long dwSrcBlockSize  ;
      unsigned long dwBlockSum      ;
      unsigned long dwStructSum     ;
}StartUpStruct ;


void SendFullBin( int fd );
void SendFullS19( int fd );


/*-------------------------------------------------------------------
 Initialize the FTP Server: set the listen port and priority
 -------------------------------------------------------------------*/
void FTPServerInit( int prio )
{
   FTPDStart( 21, prio );
}


/*-----------------------------------------------------------------------
 * Get Directory String
 * This function is used to format a file name into a directory string
 * that the FTP client will recognize. In this very simple case we
 * are just going to hardcode the values.
 *-----------------------------------------------------------------------*/
void getDirString( char *fileName, long fileSize, char *dirStr )
{
    char tmp[80];

    dirStr[0] = '-';  // '-' for file, 'd' for directory
    dirStr[1] = 0;

    // permissions, hard link, user, group
    strcat( dirStr, "rw-rw-rw- 1 user group " );

    sniprintf( tmp, 80, "%9ld ", fileSize );
    strcat( dirStr, tmp );

    strcat( dirStr, "JAN 01 2016 " );

    strcat( dirStr, fileName );
}


/*-------------------------------------------------------------------
  This function is called by the FTP Server in response to a FTP
  Client's request to list the files (e.g. the "ls" command).
 ------------------------------------------------------------------*/
int FTPD_ListFile( const char *current_directory, void *pSession, FTPDCallBackReportFunct *pFunc, int handle )
{
    char fileName[] = "seconds.txt";
    char dirString[256];
    char secondsString[64];

    sniprintf( secondsString, 64, "%ld\r\n", TimeTick / TICKS_PER_SECOND);   // Seconds since boot

    getDirString( fileName, strlen(secondsString), dirString );
    pFunc( handle, dirString );
    return FTPD_OK;
}


/*-------------------------------------------------------------------
  This function is called by the FTP Server in response to a FTP
  Client's request to receive a file (e.g. get).
 ------------------------------------------------------------------*/
int FTPD_SendFileToClient( const char *full_directory, const char *file_name, void *pSession, int fd )
{
   char buffer[255];

   if ( strcmp( file_name, "seconds.txt" ) == 0 ) // create and send file
   {
      sniprintf( buffer, 255, "%ld\r\n", TimeTick / TICKS_PER_SECOND );
      writestring( fd, buffer );
      return FTPD_OK;
   }
   else if ( strcmp( file_name, "image.bin" ) == 0 ) // send application as binary image
   {
      SendFullBin( fd );
      return FTPD_OK;
   }
   else if ( strcmp( file_name, "image.s19" ) == 0 ) // send application as .s19 file
   {
      SendFullS19( fd );
      return FTPD_OK;
   }

   iprintf( "Unknown file %s\r\n", file_name );
   return FTPD_FAIL;
}



/*-------------------------------------------------------------------
  This function is called by the FTP Server to determine if a file
  exists.
 ------------------------------------------------------------------*/
int FTPD_FileExists( const char *full_directory, const char *file_name, void *pSession )
{
   if ( strcmp( file_name, "seconds.txt" ) == 0 )
   {
      return FTPD_OK;
   }
   if ( strcmp( file_name, "image.bin" ) == 0 )
   {
      return FTPD_OK;
   }
   if ( strcmp( file_name, "image.s19" ) == 0 )
   {
      return FTPD_OK;
   }
   return FTPD_FAIL;
}

int FTPD_GetFileSize( const char *full_directory,  const char *file_name)
{
    char buffer[64];
    if ( strcmp( file_name, "seconds.txt" ) == 0 )
    {
        sniprintf(buffer, 64, "%ld\r\n", TimeTick / TICKS_PER_SECOND);
        return strlen(buffer);
    }
    if ( strcmp( file_name, "image.bin" ) == 0 )
    {
        StartUpStruct *ps = ( StartUpStruct * ) 0xC0040000;
        DWORD end_of_flash = 0xC0040000 + sizeof( StartUpStruct ) + ps->dwSrcBlockSize;
        PBYTE pstart = ( PBYTE ) 0xC0000000;
        PBYTE pend = ( PBYTE ) end_of_flash;
        return pend - pstart;
    }
    if ( strcmp( file_name, "image.s19" ) == 0 )
    {
        return 11;
    }
    return FTPD_FILE_SIZE_NOSUCH_FILE;
}

/*-------------------------------------------------------------------
  This function is called to determine if a FTP session is allowed to
  start. This is where username and password verification could occur.
 ------------------------------------------------------------------*/
void * FTPDSessionStart( const char *user, const char *passwd, const IPADDR hi_ip )
{
   return ( void * ) 200; /* Return a non zero value */
}



void SendFullBin( int fd )
{
   char buffer[10];
   StartUpStruct *ps = ( StartUpStruct * ) 0xC0040000;
   DWORD end_of_flash = 0xC0040000 + sizeof( StartUpStruct ) + ps->dwSrcBlockSize;
   PBYTE pstart = ( PBYTE ) 0xC0000000;
   PBYTE pend = ( PBYTE ) end_of_flash;
   iprintf( "Starting to send flash image from %p to %p", pstart, pend );

   sniprintf( buffer, 10, "%08lX", pend - pstart );
   writeall( fd, buffer, 8 );
   while ( pstart < ( pend - 1400 ) )
   {
      writeall( fd, ( const char * ) pstart, 1400 );
      pstart += 1400;
   }
   if ( pstart < pend )
   {
      writeall( fd, ( const char * ) pstart, pend - pstart );
   }
}


void SendFullS19( int fd )
{
   writestring( fd, "Not compete" );
}









/****************FTPD Functions that are not supported/used in this trivial case *******************/


/*-------------------------------------------------------------------
  This function gets called by the FTP Server when a FTP client sends
  a file.
 ------------------------------------------------------------------*/
int FTPD_GetFileFromClient( const char *full_directory, const char *file_name, void *pSession, int fd )
{
   return FTPD_FAIL;
}


/*-------------------------------------------------------------------
  This function gets called by the FTP Server to determine if it is
  ok to create a file on the system. In this case is will occur when
  a FTP client sends a file.
 ------------------------------------------------------------------*/
int FTPD_AbleToCreateFile( const char *full_directory,
                           const char *file_name,
                           void *pSession )
{
   return FTPD_FAIL;
}


void FTPDSessionEnd( void *pSession )
{
   /* Do nothing */
}

int FTPD_ListSubDirectories( const char *current_directory,
                             void *pSession,
                             FTPDCallBackReportFunct *pFunc,
                             int handle )
{
   /* No directories to list */
   return FTPD_OK;
}



int FTPD_DirectoryExists( const char *full_directory, void *pSession )
{
   return FTPD_FAIL;
}


int FTPD_CreateSubDirectory( const char *current_directory,
                             const char *new_dir,
                             void *pSession )
{
   return FTPD_FAIL;
}


int FTPD_DeleteSubDirectory( const char *current_directory,
                             const char *sub_dir,
                             void *pSession )
{
   return FTPD_FAIL;
}

int FTPD_DeleteFile( const char *current_directory, const char *file_name, void *pSession )
{
   return FTPD_FAIL;
}



int FTPD_Rename( const char *current_directory,const char *cur_file_name,const char *new_file_name,void *pSession )
{
   return FTPD_FAIL;
}

#ifdef IPV6
/*-------------------------------------------------------------------
  This function is called to determine if a FTP session is allowed to
  start. This is where username and password verification could occur.
 ------------------------------------------------------------------*/
void * FTPDSessionStart( const char *user, const char *passwd, const IPADDR4 hi_ip )
{
   return ( void * ) 1; /* Return a non zero value */
}
#endif
