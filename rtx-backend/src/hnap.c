/*
 * hnap.c - HNAP SOAP proxy-lag
 *
 * Håndterer autentisering mot whttpd og oversetter
 * HNAP XML-responser til JSON.
 *
 * Autentisering (HNAP1):
 *   1. Login med brukernavn/passord → Challenge + Cookie + PublicKey
 *   2. PrivateKey = HMAC-MD5(PublicKey + password, Challenge)
 *   3. Alle requests signeres: HNAP_AUTH = HMAC-MD5(PrivateKey, timestamp + SOAPAction)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mongoose.h"
#include "hnap.h"

/* TODO: Implementeres i neste fase
 * Krever:
 *   - HMAC-MD5 (kan bruke mbedtls eller en liten inline-impl)
 *   - XML-parser (minimal, bare tag-verdi-ekstraksjon)
 *   - HTTP-klient (kan gjenbruke mongoose sin mg_http_connect)
 *
 * Inntil da returnerer alle funksjoner stub-data slik at
 * /api/metrics fungerer og kan testes uavhengig.
 */

static char g_whttpd_url[128] = "http://127.0.0.1:8081";
static char g_password[64]    = "admin";
static char g_private_key[64] = "";
static char g_cookie[64]      = "";

int hnap_init(const char *whttpd_url, const char *password) {
    if (whttpd_url) strncpy(g_whttpd_url, whttpd_url, sizeof(g_whttpd_url) - 1);
    if (password)   strncpy(g_password, password, sizeof(g_password) - 1);
    /* TODO: utfør Login-handshake og lagre PrivateKey + Cookie */
    return 0;
}

int hnap_get_device_info(char *buf, int maxlen) {
    /* TODO: HNAP GetDeviceSettings → parse XML → bygg JSON */
    snprintf(buf, maxlen,
        "{\"model\":\"DIR-X1550\",\"firmware\":\"V4.0.0\","
        "\"note\":\"HNAP proxy ikke implementert enda\"}");
    return 0;
}

int hnap_reboot(void) {
    /* TODO: HNAP Reboot */
    return -1;
}

int hnap_get_wan_settings(char *buf, int maxlen) {
    /* TODO: HNAP GetWanSettings → JSON */
    snprintf(buf, maxlen,
        "{\"type\":\"DHCP\",\"ip\":\"\","
        "\"note\":\"HNAP proxy ikke implementert enda\"}");
    return 0;
}

int hnap_get_wan_status(char *buf, int maxlen) {
    snprintf(buf, maxlen, "{\"status\":\"unknown\"}");
    return 0;
}

int hnap_get_client_info(char *buf, int maxlen) {
    /* TODO: HNAP GetClientInfo → JSON */
    snprintf(buf, maxlen, "[]");
    return 0;
}

int hnap_get_wifi_settings(char *buf, int maxlen) {
    /* TODO: HNAP GetWLanRadioSettings → JSON */
    snprintf(buf, maxlen, "{}");
    return 0;
}
