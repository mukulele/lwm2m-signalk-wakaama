#ifndef SIGNALK_WS_H
#define SIGNALK_WS_H

int signalk_ws_start(const char *server, int port, const char *settings_file);
void signalk_ws_stop(void);

#endif
