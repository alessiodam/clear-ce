#include "http-client.h"

static http_client_callback_t http_callback = NULL;

static err_t http_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err) {
    dbg_printf("HTTP rcv:\n%.*s\n", p->len, (char *)p->payload);

    if (http_callback) {
        http_callback(p, err);
    }

    if (p == NULL) {
        dbg_printf("HTTP close, p is NULL\n");
        altcp_close(pcb);
    } else {
        dbg_printf("HTTP close, p is not NULL\n");
        altcp_recved(pcb, p->len);
        pbuf_free(p);
    }
    return ERR_OK;
}

static err_t http_connected(void *arg, struct altcp_pcb *pcb, err_t err) {
    if (err != ERR_OK) {
        dbg_printf("Error connecting: %d\n", err);
        return err;
    }
    
    dbg_printf("HTTP connected\n");

    char *request = (char *)arg;
    dbg_printf("Sending HTTP request:\n%s\n", request);

    altcp_output(pcb);
    altcp_recv(pcb, http_recv);
    altcp_write(pcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    return ERR_OK;
}

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
) {
    struct altcp_pcb *pcb = altcp_tcp_new_ip_type(IPADDR_TYPE_V4);
    ip4_addr_t ip;
    if (!ip4addr_aton(server_ip, &ip)) {
        dbg_printf("Invalid IP address\n");
        return ERR_VAL;
    }
    
    dbg_printf("Server IP: %s, Port: %d\n", server_ip, server_port);

    http_callback = http_client_callback;

    char request[buf_len];
    const char *method_str;
    switch (method) {
        case HTTP_GET: method_str = "GET"; break;
        case HTTP_POST: method_str = "POST"; break;
        case HTTP_PUT: method_str = "PUT"; break;
        case HTTP_DELETE: method_str = "DELETE"; break;
        case HTTP_HEAD: method_str = "HEAD"; break;
        default: return ERR_VAL;
    }

    // snprintf(headers, buf_len, "User-Agent: TI-84 plus CE\r\n%s", headers ? headers : ""); // Add user agent

    if (method == HTTP_POST || method == HTTP_PUT) {
        snprintf(
            request,
            sizeof(request),
            "%s %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "%s"
            "Content-Length: %d\r\n"
            "Connection: close\r\n\r\n"
            "%s",
            method_str,
            path ? path : "/",
            server_ip,
            headers ? headers : "",
            body ? (int)strlen(body) : 0,
            body ? body : ""
        );
    } else {
        snprintf(
            request,
            sizeof(request),
            "%s %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "%s"
            "Connection: close\r\n\r\n",
            method_str,
            path ? path : "/",
            server_ip,
            headers ? headers : ""
        );
    }

    dbg_printf("HTTP Request:\n%s\n", request);

    altcp_connect(pcb, (ip_addr_t *)&ip, server_port, http_connected);
    altcp_arg(pcb, request);

    return ERR_OK;
}
