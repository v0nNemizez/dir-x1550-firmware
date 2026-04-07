/*
 * metrics.c - /api/metrics endepunkt
 * Leser systeminfo direkte fra /proc - ingen HNAP nødvendig
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "metrics.h"

/* Leser /proc/uptime → uptime i sekunder */
static double read_uptime(void) {
    FILE *f = fopen("/proc/uptime", "r");
    if (!f) return 0;
    double up = 0;
    fscanf(f, "%lf", &up);
    fclose(f);
    return up;
}

/* Leser /proc/loadavg → 1/5/15 min load */
static void read_loadavg(double *l1, double *l5, double *l15) {
    *l1 = *l5 = *l15 = 0;
    FILE *f = fopen("/proc/loadavg", "r");
    if (!f) return;
    fscanf(f, "%lf %lf %lf", l1, l5, l15);
    fclose(f);
}

/* Leser /proc/meminfo → total/free/available i kB */
static void read_meminfo(long *total, long *free, long *available) {
    *total = *free = *available = 0;
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return;
    char key[64];
    long val;
    while (fscanf(f, "%63s %ld kB\n", key, &val) == 2) {
        if (strcmp(key, "MemTotal:") == 0)     *total = val;
        else if (strcmp(key, "MemFree:") == 0) *free = val;
        else if (strcmp(key, "MemAvailable:") == 0) *available = val;
    }
    fclose(f);
}

/*
 * Leser /proc/net/dev
 * Returnerer JSON-array med interface-statistikk
 * Format: {"name":"eth0","rx_bytes":123,"tx_bytes":456, ...}
 */
static int read_net_dev(char *buf, int maxlen) {
    FILE *f = fopen("/proc/net/dev", "r");
    if (!f) {
        snprintf(buf, maxlen, "[]");
        return 0;
    }

    char line[256];
    int written = 0;
    int first = 1;
    written += snprintf(buf + written, maxlen - written, "[");

    /* Hopp over to header-linjer */
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);

    while (fgets(line, sizeof(line), f) && written < maxlen - 200) {
        char iface[32];
        unsigned long long rx_bytes, rx_pkts, rx_errs, rx_drop;
        unsigned long long tx_bytes, tx_pkts, tx_errs, tx_drop;

        /* /proc/net/dev format:
         * iface: rx_bytes rx_pkts rx_errs rx_drop rx_fifo rx_frame rx_compressed rx_multicast
         *        tx_bytes tx_pkts tx_errs tx_drop tx_fifo tx_colls tx_carrier tx_compressed */
        char *colon = strchr(line, ':');
        if (!colon) continue;
        *colon = ' ';

        if (sscanf(line, "%31s %llu %llu %llu %llu %*u %*u %*u %*u %llu %llu %llu %llu",
                   iface,
                   &rx_bytes, &rx_pkts, &rx_errs, &rx_drop,
                   &tx_bytes, &tx_pkts, &tx_errs, &tx_drop) != 9)
            continue;

        if (!first) written += snprintf(buf + written, maxlen - written, ",");
        first = 0;

        written += snprintf(buf + written, maxlen - written,
            "{\"name\":\"%s\","
            "\"rx_bytes\":%llu,\"rx_packets\":%llu,\"rx_errors\":%llu,\"rx_dropped\":%llu,"
            "\"tx_bytes\":%llu,\"tx_packets\":%llu,\"tx_errors\":%llu,\"tx_dropped\":%llu}",
            iface,
            rx_bytes, rx_pkts, rx_errs, rx_drop,
            tx_bytes, tx_pkts, tx_errs, tx_drop);
    }

    fclose(f);
    written += snprintf(buf + written, maxlen - written, "]");
    return written;
}

/*
 * Leser /proc/net/wireless
 * Returnerer JSON-array med WiFi-interface-statistikk
 */
static int read_wireless(char *buf, int maxlen) {
    FILE *f = fopen("/proc/net/wireless", "r");
    if (!f) {
        snprintf(buf, maxlen, "[]");
        return 0;
    }

    char line[256];
    int written = 0;
    int first = 1;
    written += snprintf(buf + written, maxlen - written, "[");

    /* Hopp over to header-linjer */
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);

    while (fgets(line, sizeof(line), f) && written < maxlen - 200) {
        char iface[32];
        int status;
        float link, level, noise;

        char *colon = strchr(line, ':');
        if (!colon) continue;
        *colon = ' ';

        /* Format: iface status link level noise ... */
        if (sscanf(line, "%31s %d %f. %f. %f.", iface, &status, &link, &level, &noise) < 4)
            continue;

        if (!first) written += snprintf(buf + written, maxlen - written, ",");
        first = 0;

        written += snprintf(buf + written, maxlen - written,
            "{\"name\":\"%s\",\"link\":%d,\"level\":%d,\"noise\":%d}",
            iface, (int)link, (int)level, (int)noise);
    }

    fclose(f);
    written += snprintf(buf + written, maxlen - written, "]");
    return written;
}

/*
 * Leser /proc/net/arp for tilkoblede klienter
 */
static int read_arp_clients(char *buf, int maxlen) {
    FILE *f = fopen("/proc/net/arp", "r");
    if (!f) {
        snprintf(buf, maxlen, "[]");
        return 0;
    }

    char line[256];
    int written = 0;
    int first = 1;
    written += snprintf(buf + written, maxlen - written, "[");

    fgets(line, sizeof(line), f); /* header */

    while (fgets(line, sizeof(line), f) && written < maxlen - 200) {
        char ip[20], hw_type[8], flags[8], mac[20], mask[8], iface[16];
        if (sscanf(line, "%19s %7s %7s %19s %7s %15s",
                   ip, hw_type, flags, mac, mask, iface) != 6)
            continue;

        /* Hopp over incomplete entries (flags = 0x0) */
        if (strcmp(flags, "0x0") == 0) continue;

        if (!first) written += snprintf(buf + written, maxlen - written, ",");
        first = 0;

        written += snprintf(buf + written, maxlen - written,
            "{\"ip\":\"%s\",\"mac\":\"%s\",\"iface\":\"%s\"}",
            ip, mac, iface);
    }

    fclose(f);
    written += snprintf(buf + written, maxlen - written, "]");
    return written;
}

/* Bygger komplett metrics JSON-respons */
void metrics_build_json(char *buf, int maxlen) {
    double uptime = read_uptime();
    double l1, l5, l15;
    read_loadavg(&l1, &l5, &l15);

    long mem_total, mem_free, mem_available;
    read_meminfo(&mem_total, &mem_free, &mem_available);

    char net_buf[4096];
    read_net_dev(net_buf, sizeof(net_buf));

    char wifi_buf[1024];
    read_wireless(wifi_buf, sizeof(wifi_buf));

    char arp_buf[2048];
    read_arp_clients(arp_buf, sizeof(arp_buf));

    snprintf(buf, maxlen,
        "{"
          "\"system\":{"
            "\"uptime\":%.0f,"
            "\"load\":[%.2f,%.2f,%.2f],"
            "\"memory\":{"
              "\"total_kb\":%ld,"
              "\"free_kb\":%ld,"
              "\"available_kb\":%ld,"
              "\"used_kb\":%ld"
            "}"
          "},"
          "\"network\":%s,"
          "\"wireless\":%s,"
          "\"clients\":%s"
        "}",
        uptime,
        l1, l5, l15,
        mem_total, mem_free, mem_available, mem_total - mem_free,
        net_buf,
        wifi_buf,
        arp_buf
    );
}
