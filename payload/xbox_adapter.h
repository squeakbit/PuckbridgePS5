#ifndef PUCKBRIDGE_XBOX_ADAPTER_H
#define PUCKBRIDGE_XBOX_ADAPTER_H

#include <stddef.h>

int xbox_adapter_open(const char *node, char *why, size_t whylen);
void xbox_adapter_close(void);
int xbox_adapter_read(const unsigned char **message);
int xbox_adapter_send_gip(const unsigned char *message, size_t length);
int xbox_adapter_controller_connected(void);

#endif
