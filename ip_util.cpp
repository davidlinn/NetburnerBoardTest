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

/******************************************************************************
 * Helper functions to display IP address information on a web page or
 * the serial port
 ******************************************************************************/
#include <predef.h>
#include <constants.h>
#include <ctype.h>
#include <ucos.h>
#include <system.h>
#include <init.h>
#include <utils.h>
#include <ipv6/ipv6_interface.h>
#include <netinterface.h>
#include <tcp.h>
#include <fdprintf.h>
#include <http.h>


// Required for web page callback linking
extern "C"
{
    void webShowAddress( int socket, char *url );
}

const char * SourceName(ePrefixSource e)
{
    switch (e)
    {
    case eLinkLocal: return "Link Local";
    case eRouter: return "Router";
    case eDHCP: return "DHCP";
    case eStatic: return "Static";
    case eUnknown:
    default:
        return "Unknown";
    }
}


/*------------------------------------------------------------------------------
 * Web page function callback to display links to IPv4 and IPv6 addresses.
 * Note that IPv6 prefix addresses may take a few seconds to be assigned.
 *-----------------------------------------------------------------------------*/
void webShowAddress( int socket, char *url )
{

    IPADDR localIP = GetSocketLocalAddr(socket);
    uint32_t remotePort = GetSocketRemotePort(socket);

    fdprintf(socket, "<br>Web browser request came from:");
     localIP.fdprint(socket);
    fdprintf(socket,": %d<br>\r\n",remotePort);
    fdprintf(socket, "<br>Device Interfaces:<br>");
    int ifn=GetFirstInterface();

    while (ifn)
    {
        fdprintf(socket,"<HR>\r\n");
        fdprintf(socket,"Interface # %d:<br>\r\n",ifn);
        fdprintf(socket, "<br>IPV4 Addresses:<br>");
        if (GetInterfaceLink(ifn))
        {
            IPADDR6 i6();
            fdprintf(socket, "Make an IPv4 request: <a href=\"http://");
            InterfaceIP(ifn).fdprint(socket);
            fdprintf(socket,"\">");
            InterfaceIP(ifn).fdprint(socket);
            fdprintf(socket,"</a><br>\r\n");
        }
        IPv6Interface *pIPv6If=IPv6Interface::GetInterfaceN(ifn);
        if (pIPv6If)
        {
            fdprintf(socket,"IPV6 Addresses:<BR>\r\n");

            IPV6_PREFIX * pPrefix =pIPv6If->FirstPrefix();


            while (pPrefix)
            {
                // Check that the prefix is actually valid to use as an address
                // for our device
                if (pPrefix->bValidForInterface) {
                    ePrefixSource psrc=pPrefix->Source();
                    fdprintf(socket, "Make an IPv6 request: <a href=\"http://[");
                    pPrefix->m_IPAddress.fdprint(socket);
                    fdprintf(socket,"]\">");
                    pPrefix->m_IPAddress.fdprint(socket);
                    fdprintf(socket,"</a> (Created from %s)<br>\r\n",SourceName(psrc));
                    if (pPrefix->pDHCPD) {
                        fdiprintf(socket, "<div style=\"padding-left: 30px\">Renew: %lu</div>", pPrefix->pDHCPD->m_renewTick - TimeTick);
                        fdiprintf(socket, "<div style=\"padding-left: 30px\">Rebind: %lu</div>", pPrefix->pDHCPD->m_rebindTick - TimeTick);
                    }
                }
                pPrefix = pPrefix->GetNext();
            }
            IPV6_DNS * pDNS = pIPv6If->FirstDNS();
            if (pDNS) {
                fdiprintf(socket, "<br>DNS:<br>");
            }
            while (pDNS) {
                fdiprintf(socket, "<div style=\"padding-left: 30px\"> %I - Source: %s</div>",
                            pDNS->m_IPAddress, (pDNS->pRouter) ? "Router" : "DHCP");
                pDNS = pDNS->GetNext();
            }
        }

        ifn=GetnextInterface(ifn);
        //now skip auto ip addresses...
        while ((ifn) && (GetInterFaceBlock(ifn)) && (GetInterFaceBlock(ifn)->AutoIPC))
        {
            ifn=GetnextInterface(ifn);
        }
    }
    fdprintf(socket,"<HR>\r\n");

}


/*------------------------------------------------------------------------------
 * Print IP addresses to serial port
 *-----------------------------------------------------------------------------*/
void showIpAddresses()
{

    int ifn = GetFirstInterface();

    while (ifn)
    {
        iprintf("\r\nInteface %d MAC:", ifn);
        InterfaceMAC(ifn).print();
        iprintf("\r\nIPv4 Addresses:\r\n");
        iprintf("IP:      %HI\r\n", InterfaceIP(ifn));
        iprintf("Mask:    %HI\r\n", InterfaceMASK(ifn));
        iprintf("DNS:     %HI\r\n", InterfaceDNS(ifn));
        iprintf("Gateway: %HI\r\n", InterfaceGate(ifn));

        IPv6Interface *pIPv6If = IPv6Interface::GetInterfaceN(ifn);
        if (pIPv6If)
        {
            IPV6_PREFIX * pPrefix = pIPv6If->FirstPrefix();
            while (pPrefix)
            {
                iprintf("\r\nIPv6 Addresses:\r\n");
                // Check that the prefix is actually valid to use as an address
                // for our device
                if (pPrefix->bValidForInterface && pPrefix->AgeStillValidTest())
                {
                    ePrefixSource psrc = pPrefix->Source();
                    pPrefix->m_IPAddress.print();
                    iprintf("/%d :%s\r\n", pPrefix->PrefixLen, SourceName(psrc));
                    if (pPrefix->pDHCPD)
                    {
                        iprintf("  Renew: %lu\r\n", pPrefix->pDHCPD->m_renewTick - TimeTick);
                        iprintf("  Rebind: %lu\r\n", pPrefix->pDHCPD->m_rebindTick - TimeTick);
                    }
                }
                pPrefix = pPrefix->GetNext();
            }
            IPV6_DNS * pDNS = pIPv6If->FirstDNS();
            if (pDNS)
            {
                iprintf("\r\nDNS:\r\n");
            }
            while (pDNS)
            {
            	iprintf("  %I - Source: ", pDNS->m_IPAddress );
            	if( pDNS->pRouter != nullptr )
            	{
            		iprintf(" %s", (pDNS->pRouter) ? "Router" : "DHCP");
            	}
            	else
            	{
            		iprintf(" Router is unavailable!");
            	}
            	iprintf( "\r\n" );
                pDNS = pDNS->GetNext();
            }
        }

        ifn = GetnextInterface(ifn);
//        //now skip auto ip addresses...
//        while ((ifn) && (GetInterFaceBlock(ifn)) && (GetInterFaceBlock(ifn)->AutoIPC))
//        {
//            ifn = GetnextInterface(ifn);
//        }

    }

    iprintf("\r\n");
}








