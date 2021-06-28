#ifndef REDIS_CLIENT_H
#define REDIS_CLIENT_H

#define REDIS_RECEIVE_BUFFER_SIZE 100

#define REDIS_LINE_OPCODE 'L'
#define REDIS_CLEAR_OPCODE 'C'

typedef struct {
  int newLinesStartIndex;
  int newLinesStopIndex;
} RedisLinesInterval;

// This union is trivial, but it clarifies that lineInterval is not always used
typedef union {
  RedisLinesInterval lineInterval;
} RedisMessageData;

typedef struct {
  byte opcode;
  int fromClientId;
  RedisMessageData data;
} RedisMessage;

void connectToRedisServer();

// Receive Redis data from the subscription channel if there is any
int redisReceiveMessage(RedisMessage *redisMessage);

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
