#ifndef REDIS_CLIENT_H
#define REDIS_CLIENT_H

#define REDIS_RECEIVE_BUFFER_SIZE 100

void connectToRedisServer();

// Receive Redis data from the subscription channel if there is any
int redisReceiveMessage(int *fromClientId, int *newLinesStartIndex, int *newLinesStopIndex);

// Send lines in buffer if there are any
void sendLinesInBuffer();

// Send a line drawn by the local user to the Redis server (can be called from ISR)
void redisAddLineToSendBuffer(Line line);

// Clear the entire drawing (can be called from ISR)
void redisPlanClearDrawing();

// Download the drawing lines in interval [start, stop].
// Pass [0,-1] to get all lines.
// Call redisDownloadLine() to then get each line
int redisDownloadLinesBegin(int start, int stop);

// To call after redisDownloadLinesStart(),
// as many times as the value redisDownloadLinesStart() returned.
void redisDownloadLine(Line *line);

void runRedisPeriodicTasks();

#endif
