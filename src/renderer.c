/*
 *--------------------------------------
 * Program Name: Clear Renderer
 * Author: Log4Jake
 * License: GNU GPL v3
 * Description: Handles rendering pages
 *--------------------------------------
*/

#include "renderer.h"
#include <string.h>

#define MAX_LINKS 20
#define MAX_HREF_LENGTH 100
#define MAX_RENDERED_ITEMS 50 // This CANNOT be increased due to memory limitations.

typedef struct {
    char href[MAX_HREF_LENGTH];
} Link;

typedef struct {
    char text[100];
    int x;
    int y;
    int bold;
    int italic;
    int underline;
    int link;
} RenderedItem;

Link links[MAX_LINKS];
RenderedItem renderedItems[MAX_RENDERED_ITEMS];
int link_count = 0;
int item_count = 0;

void render_html(const char *html) {
    gfx_SetTextFGColor(0);
    gfx_SetTextBGColor(255);
    gfx_SetTextTransparentColor(255);
    gfx_FillScreen(255);

    int x = 10, y = 10;
    int bold = 0, italic = 0, underline = 0, link = 0, h1 = 0, ul = 0, li = 0;
    char buffer[2] = {0, 0};

    while (*html) {
        if (*html == '<') {
            html++;
            if (strncmp(html, "h1>", 3) == 0) {
                h1 = 1;
                html += 3;
                x = 10;
                gfx_SetTextScale(2, 2);
                gfx_SetTextFGColor(0);
            } else if (strncmp(html, "/h1>", 4) == 0) {
                h1 = 0;
                html += 4;
                y += 20;
                gfx_SetTextScale(1, 1);
                gfx_SetTextFGColor(0);
                x = 10;
            } else if (strncmp(html, "p>", 2) == 0) {
                html += 2;
                x = 10;
            } else if (strncmp(html, "/p>", 3) == 0) {
                html += 3;
                y += 10;
                x = 10;
            } else if (strncmp(html, "br/>", 4) == 0) {
                html += 4;
                y += 10;
                x = 10;
            } else if (strncmp(html, "em>", 3) == 0 || strncmp(html, "i>", 2) == 0) {
                italic = 1;
                html += (strncmp(html, "em>", 3) == 0) ? 3 : 2;
            } else if (strncmp(html, "/em>", 4) == 0 || strncmp(html, "/i>", 3) == 0) {
                italic = 0;
                html += (strncmp(html, "/em>", 4) == 0) ? 4 : 3;
            } else if (strncmp(html, "u>", 2) == 0) {
                underline = 1;
                html += 2;
            } else if (strncmp(html, "/u>", 3) == 0) {
                underline = 0;
                html += 3;
            } else if (strncmp(html, "ul>", 3) == 0) {
                ul = 1;
                html += 3;
                y += 10;
                x = 20;
            } else if (strncmp(html, "/ul>", 4) == 0) {
                ul = 0;
                html += 4;
                y += 10;
                x = 10;
            } else if (strncmp(html, "li>", 3) == 0) {
                li = 1;
                html += 3;
                x = 30;
            } else if (strncmp(html, "/li>", 4) == 0) {
                li = 0;
                html += 4;
                y += 10;
                x = 20;
            } else if (strncmp(html, "a href=\"", 8) == 0) {
                link = 1;
                html += 8;
                const char *start = html;
                while (*html && *html != '\"') html++;
                if (link_count < MAX_LINKS && html - start < MAX_HREF_LENGTH) {
                    strncpy(links[link_count].href, start, html - start);
                    links[link_count].href[html - start] = '\0';
                    link_count++;
                }
                while (*html && *html != '>') html++;
                if (*html == '>') html++;
            } else if (strncmp(html, "/a>", 3) == 0) {
                link = 0;
                html += 3;
            } else {
                while (*html && *html != '>') html++;
                if (*html == '>') html++;
            }
        } else {
            buffer[0] = *html;
            int offset = 8;
            if (h1) {
                gfx_SetTextScale(2, 2);
                gfx_SetTextFGColor(0);
                offset = 16;
            } else if (bold) {
                gfx_SetTextScale(2, 1);
                offset = 12;
            } else if (italic || link) {
                gfx_SetTextScale(1, (uint8_t)1.5);
                if (link) {
                    gfx_SetTextFGColor(gfx_RGBTo1555(0, 0, 238));
                } else {
                    gfx_SetTextFGColor(0);
                }
            } else {
                gfx_SetTextScale(1, 1);
                gfx_SetTextFGColor(0);
            }

            if (y <= LCD_HEIGHT) {
                gfx_PrintStringXY(buffer, x, y);
                if (underline) {
                    gfx_HorizLine(x, y + 8, offset); 
                }
            }

            if (item_count < MAX_RENDERED_ITEMS) {
                RenderedItem *item = &renderedItems[item_count++];
                strncpy(item->text, buffer, sizeof(item->text));
                item->x = x;
                item->y = y;
                item->bold = bold;
                item->italic = italic;
                item->underline = underline;
                item->link = link;
            }

            html++;
            x += offset;

            if (x >= LCD_WIDTH - 10) {
                x = 10;
                y += 10;
            }
        }
    }

    for (int i = 0; i < link_count; i++) {
        dbg_printf("Link %d: %s\n", i + 1, links[i].href);
    }

    gfx_SwapDraw();
}
