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
#include <stdarg.h>

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
#include "lwip/dns.h"

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

/* customize printf and input functions */
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

void display_mode_indicators()
{
    gfx_SetTextFGColor(11);
    gfx_PrintChar(mode_indic[INPUT_LOWER]);
    gfx_SetTextFGColor(0);
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
    uint32_t start_time = clock() * 1000 / CLOCKS_PER_SEC;
    while (1)
    {
        if ((clock() * 1000 / CLOCKS_PER_SEC) - start_time > msec)
        {
            break;
        }
        handle_all_events();
    }
}

/* logging stuff */
typedef enum {
    LOGLEVEL_DEBUG = 0,
    LOGLEVEL_INFO = 1,
    LOGLEVEL_WARN = 2,
    LOGLEVEL_ERROR = 3
} log_level_t;

#define MIN_LOGLEVEL LOGLEVEL_INFO

void printf_log(log_level_t level, const char *file, int line, const char *format, ...)
{
    if (level >= MIN_LOGLEVEL)
    {
        va_list args;
        va_start(args, format);
        printf("%s:%04d: ", file, line);
        vprintf(format, args);
        printf("\n");
        va_end(args);
    }
}

void ethif_status_callback_fn(struct netif *netif)
{
    if (netif->flags & NETIF_FLAG_LINK_UP)
    {
        printf("Link up");
    }
    else
    {
        printf("Link down");
    }
}

void set_static_dns_server_to_cloudflare()
{
    printf("Set Static DNS Server 1.1.1.1 Start.\n");
    ip_addr_t dnsserver;
    ipaddr_aton("1.1.1.1", &dnsserver);
    dns_setserver(0, &dnsserver);
    printf("Set DNS Server %s Done.\n", ipaddr_ntoa(dns_getserver(0)));
}

void http_content_received(struct pbuf *p, err_t err) {
    if (err != ERR_OK) {
        printf("HTTP err: %d", err);
        return;
    }
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

    printf("Interface Wait.\n");
    while (ethif == NULL)
    {
        key = os_GetCSC();
        if (key == sk_Clear) {
            goto exit;
        }
        ethif = netif_find("en0");
        handle_all_events();
    }
    printf("interface Found.\n");

    printf("DHCP Start.\n");
    netif_set_status_callback(ethif, ethif_status_callback_fn);
    if (dhcp_start(ethif) != ERR_OK)
    {
        printf("DHCP Start Fail.\n");
        goto exit;
    }
    printf("DHCP Done.\n");

    printf("DHCP Wait.\n");
    while (!dhcp_supplied_address(ethif))
    {
        key = os_GetCSC();
        if (key == sk_Clear) { goto exit; }
        handle_all_events();
    }
    printf("DHCP Done.\n");

    printf("DNS Init Start.\n");
    dns_init();
    printf("DNS Init Done.\n");

    const ip_addr_t *dhcp_nameserver = dns_getserver(0);
    if (dhcp_nameserver != NULL)
    {
        printf("DHCP DNS Server: %s\n", ipaddr_ntoa(dhcp_nameserver));
    }
    else
    {
        printf("DHCP Provided no DNS Server.\n");
        set_static_dns_server_to_cloudflare();
    }

    while (1)
    {
        key = os_GetCSC();
        if (key == sk_Clear) { goto exit; }
        else if (key == sk_Enter)
        {
            printf("HTTP GET\n");
            err_t http_get_err = http_request(HTTP_GET, "152.228.162.35", 80, "/", "", "", 2048, http_content_received);
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
        else if (key == sk_TglExact) { set_static_dns_server_to_cloudflare(); }
        handle_all_events();
    }
    goto exit;
exit:
    msleep(2000);
    exit_funcs();
    exit(0);
}
