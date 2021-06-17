#ifndef REDIS_CLIENT_H
#define REDIS_CLIENT_H

#define REDIS_RECEIVE_BUFFER_SIZE 1024

void connectToRedisServer();
int redisReceive(char buf[REDIS_RECEIVE_BUFFER_SIZE]);

#endif
