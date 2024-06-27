/*
 *--------------------------------------
 * Program Name: Clear Renderer
 * Author: Log4Jake
 * License: GNU GPL v3
 * Description: Handles rendering pages
 *--------------------------------------
*/

#ifndef RENDERER_H
#define RENDERER_H

#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <string.h>
#include <debug.h>

#ifdef __cplusplus
extern "C" {
#endif

void render_html(const char *html);

#ifdef __cplusplus
}
#endif

#endif
