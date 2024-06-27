/*
 *--------------------------------------
 * Program Name: lwIP-CE HTTP Client
 * Author: TKB Studios
 * License: Apache License 2.0
 * Description: An HTTP Client for lwIP-CE
 *--------------------------------------
*/

#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <stdbool.h>
#include <time.h>

#include "lwip/altcp.h"
#include "lwip/altcp_tcp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*http_client_callback_t)(struct pbuf *p, err_t err);

typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_HEAD
} http_method_t;

/// @brief Sends an HTTP request
/// @param method HTTP method, GET, POST, PUT, DELETE, HEAD
/// @param netif Network interface to use
/// @param server_ip The ASCII representation of the IP address of the server, example: "237.84.2.178"
/// @param server_port The port of the server, example: 80
/// @param path URL to request, example: "/index.html"
/// @param headers Request headers, example: "Content-Type: text/html\r\nAccept: */*\r\n"
/// @param body Request body, example: "Hello, World!"
/// @param buf_len Size of the buffer
/// @param http_client_callback Callback function for received HTTP data.
/// @return 
err_t http_request(
    http_method_t method,
    struct netif *netif,
    const char *server_ip,
    uint16_t server_port,
    const char *path,
    char *headers,
    const char *body,
    unsigned int buf_len,
    http_client_callback_t http_client_callback
);

#ifdef __cplusplus
}
#endif

#endif
