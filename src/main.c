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

#include "webengine/webengine.h"

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
bool outchar_scroll_up = true;

struct netif *ethif = NULL;

bool run_main = false;
bool wait_for_http = false;

void ethif_status_callback_fn(struct netif *netif)
{
    if (netif->flags & NETIF_FLAG_LINK_UP)
    {
        printf("Link up\n");
    }
    else
    {
        printf("Link down\n");
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

static void newline(void)
{
    if (outchar_scroll_up)
    {
        memmove(gfx_vram, gfx_vram + (LCD_WIDTH * 10), LCD_WIDTH * (LCD_HEIGHT - 30));
        gfx_SetColor(255);
        gfx_FillRectangle_NoClip(0, LCD_HEIGHT - 30, LCD_WIDTH, 10);
        gfx_SetTextXY(2, LCD_HEIGHT - 30);
    }
    else
        gfx_SetTextXY(2, gfx_GetTextY() + 10);
}

void outchar(char c)
{
    if (c == '\n')
    {
        newline();
    }
    else if (c < ' ' || c > '~')
    {
        return;
    }
    else
    {
        if (gfx_GetTextX() >= LCD_WIDTH - 16)
        {
            newline();
        }
        gfx_PrintChar(c);
    }
}

static void http_content_received(struct pbuf *p, err_t err) {
    printf("HTTP rcv:\n%.*s", p->len, (char *)p->payload);
    render_html((char *)p->payload);
    wait_for_http = false;
}

int main(void)
{
    sk_key_t key = 0;

    os_ClrHome();
    gfx_Begin();
    lwip_init();

    if (usb_Init(eth_handle_usb_event, NULL, NULL, USB_DEFAULT_INIT_FLAGS))
        goto exit;

    printf("Waiting for interface...\n");
    while (ethif == NULL)
    {
        key = os_GetCSC();
        if (key == sk_Clear) {
            goto exit;
        }
        ethif = netif_find("en0");
        handle_all_events();
    }
    printf("interface found\n");
    printf("DHCP starting...\n");
    netif_set_status_callback(ethif, ethif_status_callback_fn);
    if (dhcp_start(ethif) != ERR_OK)
    {
        printf("dhcp_start failed\n");
        goto exit;
    }
    printf("dhcp started\n");

    printf("waiting for DHCP to complete...\n");
    while (!dhcp_supplied_address(ethif))
    {
        key = os_GetCSC();
        if (key == sk_Clear) {
            goto exit;
        }
        handle_all_events();
    }
    printf("DHCP completed\n");

    while (1)
    {
        key = os_GetCSC();
        if (key == sk_Clear)
        {
            goto exit;
        }
        else if (key == sk_Enter)
        {
            printf("HTTP GET\n");
            err_t http_get_err = http_request(HTTP_GET, "merthsoft.com", 80, "/", "", "", 4096, http_content_received);
            if (http_get_err != ERR_OK)
            {
                printf("HTTP GET failed, code: %d\n", http_get_err);
                goto exit;
            }
            printf("wait HTTP\n");
            while (wait_for_http)
            {
                key = os_GetCSC();
                if (key == sk_Clear) {
                    goto exit;
                }
                handle_all_events();
            }
        }
        handle_all_events();
    }
    goto exit;
exit:
    msleep(2000);
    exit_funcs();
    exit(0);
}
