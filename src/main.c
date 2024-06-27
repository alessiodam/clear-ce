/*
 *--------------------------------------
 * Program Name: Clear Client
 * Author: TKB Studios
 * License: GNU GPL v3
 * Description: Clear. A web browser for the TI-84 Plus CE.
 *--------------------------------------
 * Wasted hours counter: 3 hours
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <usbdrvce.h>
#include <ti/getcsc.h>
#include <ti/screen.h>
#include <sys/timers.h>

#include "lwip/init.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/netif.h"

// #include "lwip/altcp_tcp.h"
// #include "lwip/altcp.h"
#include "lwip/dhcp.h"

#include "drivers/usb-ethernet.h"

#include "http-client/http-client.h"

#include "renderer.h"

enum
{
    INPUT_LOWER,
    INPUT_UPPER,
    INPUT_NUMBER
};
char *chars_lower = "\0\0\0\0\0\0\0\0\0\0\"wrmh\0\0?[vqlg\0\0.zupkfc\0 ytojeb\0\0xsnida\0\0\0\0\0\0\0\0";
char *chars_upper = "\0\0\0\0\0\0\0\0\0\0\"WRMH\0\0?[VQLG\0\0:ZUPKFC\0 YTOJEB\0\0XSNIDA\0\0\0\0\0\0\0\0";
char *chars_num = "\0\0\0\0\0\0\0\0\0\0+-*/^\0\0?359)\0\0\0.258(\0\0\0000147,\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
char mode_indic[] = {'a', 'A', '1'};

struct netif *ethif = NULL;

bool run_main = false;

const char *TEST_HTML = "<p>Merthsoft Creations</p>"
"       <p>Links:</p>"
"       <ul>"
"           <li>TokenIDE:"
"               <ul>"
"                   <li><a href=\"Tokens.zip\">Tokens.zip</a></li>"
"                   <li><a href=\"Tokens.tar.xz\">Tokens.tar.xz</a></li>"
"                   <li><a href=\"http://www.cemetech.net/forum/viewtopic.php?t=4798\">Topic on Cemetech</a></li>"
"                   <li><a href=\"https://bitbucket.org/merthsoft/tokenide\">BitBucket page</a></li>"
"               </ul>"
"           </li>"
"           <li>DecBot:"
"               <ul>"
"                   <li><a href=\"DecBot.php\">Scores</a></li>"
"                   <li><a href=\"DecBotTab.php\">TSV</a></li>"
"                   <li><a href=\"http://www.cemetech.net/forum/viewtopic.php?t=5543\">Topic on Cemetech</a></li>"
"                   <li><a href=\"https://bitbucket.org/merthsoft/decbot\">BitBucket page</a></li>"
"               </ul>"
"           </li>"
"           <li><a href=\"vardump.php\">Variable dumper</a></li>"
"           <li><a href=\"binsprite.html\">Binary/hex sprite renderer</a></li>"
"           <li><a href=\"gol\">HTML5 Game of Life</a></li>"
"           <li><a href=\"https://github.com/merthsoft\">GitHub page</a></li>"
"           <li><a href=\"linkguide\">TI Link Protocol Guide</a></li>"
"           <li><a href=\"http://www.cemetech.net/forum/viewforum.php?f=69\">Projects on Cemetech</a>"
"               <a href=\"http://www.cemetech.net/forum/profile.php?mode=viewprofile&amp;u=merthsoft\"> (profile)</a></li>"
"           <li><a href=\"http://www.ticalc.org/archives/files/authors/75/7522.html\">ticalc.org profile</a></li>"
"           <li><a href=\"http://thisaintopera.com\">This Ain't Opera</a></li>"
"       </ul>";

void ethif_status_callback_fn(struct netif *netif)
{
    if (netif->flags & NETIF_FLAG_LINK_UP)
    {
        dbg_printf("Link up\n");
    }
    else
    {
        dbg_printf("Link down\n");
    }
}

void exit_funcs(void)
{
    usb_Cleanup();
    gfx_End();
}

void handle_all_events()
{
    usb_HandleEvents();       // usb events
    sys_check_timeouts();     // lwIP timers/event callbacks
}

void events_msleep(uint16_t msec)
{
    uint32_t start_time = sys_now();
    while ((sys_now() - start_time) < msec)
    {
        handle_all_events();
        usleep(1000);
    }
}

void display_mode_indicators()
{
    gfx_SetTextFGColor(11);
    gfx_PrintChar(mode_indic[INPUT_LOWER]);
    gfx_SetTextFGColor(0);
}

static void http_get_recv(struct pbuf *p, err_t err) {
    dbg_printf("HTTP rcv:\n%.*s", p->len, (char *)p->payload);
    render_html((char *)p->payload);
}

int main(void)
{
    sk_key_t key = 0;

    os_ClrHome();
    gfx_Begin();
    lwip_init();

    if (usb_Init(eth_handle_usb_event, NULL, NULL, USB_DEFAULT_INIT_FLAGS))
        goto exit;

    dbg_printf("Waiting for interface...\n");
    printf(".");
    while (ethif == NULL)
    {
        key = os_GetCSC();
        if (key == sk_Clear) {
            goto exit;
        }
        ethif = netif_find("en0");
        handle_all_events();
    }
    dbg_printf("interface found\n");
    printf(".");

    dbg_printf("DHCP starting...\n");
    printf(".");
    netif_set_status_callback(ethif, ethif_status_callback_fn);
    if (dhcp_start(ethif) != ERR_OK)
    {
        dbg_printf("dhcp_start failed\n");
        goto exit;
    }
    dbg_printf("dhcp started\n");
    printf(".");

    dbg_printf("waiting for DHCP to complete...\n");
    printf(".");
    while (!dhcp_supplied_address(ethif))
    {
        key = os_GetCSC();
        if (key == sk_Clear) {
            goto exit;
        }
        handle_all_events();
    }
    dbg_printf("DHCP completed\n");
    printf(".");

    // events_msleep(1000);

    // dbg_printf("Testing HTTP GET...\n");
    // if (http_request(HTTP_GET, ethif, "152.228.162.35", 80, "/", "", "", 1024, http_get_recv) != ERR_OK)
    // {
    //     dbg_printf("HTTP GET failed\n");
    //     msleep(1000);
    //     goto exit;
    // }
    // 
    // dbg_printf("Waiting for HTTP response...\n");
    // while (1)
    // {
    //     key = os_GetCSC();
    //     if (key == sk_Clear) {
    //         break;
    //     }
    //     handle_all_events();
    // }

    printf(".\n");
    printf("Init done\n");

    while (1)
    {
        key = os_GetCSC();
        if (key == sk_Clear)
        {
            break;
        }
        else if (key == sk_Enter)
        {
            os_ClrLCD();
            render_html(TEST_HTML);
        }
        handle_all_events();
    }
    goto exit;
exit:
    exit_funcs();
    exit(0);
}
