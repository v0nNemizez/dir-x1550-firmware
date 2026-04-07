/*
 * rtx-backend - JSON REST API for DIR-X1550
 *
 * Lytter på port 8080 (whttpd flyttes til 8081)
 * Eksponerer:
 *   GET  /api/metrics   → systeminfo fra /proc (ingen autentisering)
 *   GET  /api/status    → HNAP GetDeviceSettings proxy
 *   GET  /api/wan       → HNAP GetWanSettings proxy
 *   GET  /api/clients   → HNAP GetClientInfo proxy
 *   POST /api/reboot    → HNAP Reboot proxy
 *
 * Avhenger av mongoose (enkelt-fil HTTP-bibliotek for embedded)
 */

#include <stdio.h>
#include <string.h>
#include "mongoose.h"
#include "metrics.h"
#include "hnap.h"

#define LISTEN_ADDR "http://0.0.0.0:8080"

static void send_json(struct mg_connection *c, int status, const char *body) {
    mg_http_reply(c, status,
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n",
        "%s", body);
}

static void handle_request(struct mg_connection *c, struct mg_http_message *hm) {
    /* GET /api/metrics */
    if (mg_http_match_uri(hm, "/api/metrics") &&
        mg_vcasecmp(&hm->method, "GET") == 0) {
        char buf[8192];
        metrics_build_json(buf, sizeof(buf));
        send_json(c, 200, buf);
        return;
    }

    /* GET /api/status */
    if (mg_http_match_uri(hm, "/api/status") &&
        mg_vcasecmp(&hm->method, "GET") == 0) {
        char buf[4096];
        if (hnap_get_device_info(buf, sizeof(buf)) == 0) {
            send_json(c, 200, buf);
        } else {
            send_json(c, 502, "{\"error\":\"HNAP unavailable\"}");
        }
        return;
    }

    /* GET /api/wan */
    if (mg_http_match_uri(hm, "/api/wan") &&
        mg_vcasecmp(&hm->method, "GET") == 0) {
        char buf[4096];
        if (hnap_get_wan_settings(buf, sizeof(buf)) == 0) {
            send_json(c, 200, buf);
        } else {
            send_json(c, 502, "{\"error\":\"HNAP unavailable\"}");
        }
        return;
    }

    /* GET /api/clients */
    if (mg_http_match_uri(hm, "/api/clients") &&
        mg_vcasecmp(&hm->method, "GET") == 0) {
        char buf[8192];
        if (hnap_get_client_info(buf, sizeof(buf)) == 0) {
            send_json(c, 200, buf);
        } else {
            send_json(c, 502, "{\"error\":\"HNAP unavailable\"}");
        }
        return;
    }

    /* POST /api/reboot */
    if (mg_http_match_uri(hm, "/api/reboot") &&
        mg_vcasecmp(&hm->method, "POST") == 0) {
        if (hnap_reboot() == 0) {
            send_json(c, 200, "{\"status\":\"rebooting\"}");
        } else {
            send_json(c, 502, "{\"error\":\"HNAP unavailable\"}");
        }
        return;
    }

    /* OPTIONS preflight (CORS) */
    if (mg_vcasecmp(&hm->method, "OPTIONS") == 0) {
        mg_http_reply(c, 204,
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type, Authorization\r\n",
            "");
        return;
    }

    send_json(c, 404, "{\"error\":\"Not found\"}");
}

static void event_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        handle_request(c, (struct mg_http_message *) ev_data);
    }
}

int main(void) {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    printf("rtx-backend starting on %s\n", LISTEN_ADDR);

    if (mg_http_listen(&mgr, LISTEN_ADDR, event_handler, NULL) == NULL) {
        fprintf(stderr, "Failed to bind %s\n", LISTEN_ADDR);
        return 1;
    }

    for (;;) mg_mgr_poll(&mgr, 1000);

    mg_mgr_free(&mgr);
    return 0;
}
