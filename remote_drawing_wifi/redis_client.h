#ifndef REDIS_CLIENT_H
#define REDIS_CLIENT_H

#define REDIS_RECEIVE_BUFFER_SIZE 1024

void connectToRedisServer();

// Receive Redis data from the subscription channel if there is any
int redisTryReceiveSub(char buf[REDIS_RECEIVE_BUFFER_SIZE]);

// Send a line drawn by the local user to the Redis server
void redisTransmitLine(int x0, int y0, int x1, int y1);

// Download the complete drawing stored on the server, typically used at boot
int redisDownloadLinesBegin();

// To call after redisDownloadLinesStart(),
// as many times as the value redisDownloadLinesStart() returned.
void redisDownloadLine(int *x0, int *y0, int *x1, int *y1);

#endif
