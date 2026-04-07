#ifndef METRICS_H
#define METRICS_H

/* Bygger komplett metrics JSON og skriver til buf (maxlen bytes) */
void metrics_build_json(char *buf, int maxlen);

#endif
