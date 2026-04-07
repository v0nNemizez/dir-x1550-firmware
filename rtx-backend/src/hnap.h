#ifndef HNAP_H
#define HNAP_H

/*
 * HNAP proxy-lag
 * Alle funksjoner returnerer 0 ved suksess, -1 ved feil.
 * JSON-respons skrives til buf.
 */

/* Initialiserer HNAP-session mot whttpd (kall én gang ved oppstart) */
int hnap_init(const char *whttpd_url, const char *password);

/* System */
int hnap_get_device_info(char *buf, int maxlen);
int hnap_reboot(void);

/* WAN */
int hnap_get_wan_settings(char *buf, int maxlen);
int hnap_get_wan_status(char *buf, int maxlen);

/* Klienter */
int hnap_get_client_info(char *buf, int maxlen);

/* WiFi */
int hnap_get_wifi_settings(char *buf, int maxlen);

#endif
