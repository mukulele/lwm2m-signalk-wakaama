#ifndef SIGNALK_WS_H
#define SIGNALK_WS_H

#include <stdbool.h>

int signalk_ws_start(const char *server, int port, const char *settings_file);
void signalk_ws_stop(void);
bool signalk_ws_is_connected(void);

#endif
