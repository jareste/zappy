#ifndef WS_CLIENT_H
#define WS_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

int    ws_init(void);
int    ws_send(const char *msg);
int    ws_recv(char *buf, int max_len);
void   ws_close(void);

#ifdef __cplusplus
}
#endif

#endif
