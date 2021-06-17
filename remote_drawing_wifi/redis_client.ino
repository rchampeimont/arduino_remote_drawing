// Code to connect to the Redis server
// Tested with Redis 3.0.6

#include <WiFiNINA.h>
#include "redis_client.h"
#include "serial_com.h"
#include "secrets.h"
#include "pins.h"

WiFiClient client;

void connectToRedisServer() {
  char buf[REDIS_RECEIVE_BUFFER_SIZE];

  sendStatusMessage("Connecting to Redis server...");
  if (! client.connect(REDIS_ADDR, REDIS_PORT)) {
    fatalError("Connection FAILED to Redis server");
  }

  sendStatusMessage("Authenticating with Redis server...");
  client.write("AUTH ");
  client.write(REDIS_PASSWORD);
  client.write("\r\n");

  if (! redisReceive(buf)) {
    fatalError("No response from Redis when authenticating");
  }
  
  if (strcmp(buf, "+OK") == 0) {
    sendStatusMessage("Authentication with Redis server OK");
  } else {
    fatalError("Authentication with Redis server FAILED: %s", buf);
  }



  sendStatusMessage("Subscribing to Redis channel...");
  client.write("SUBSCRIBE remote_drawing\r\n");
  expectRedisResponse("SUBSCRIBE", "*3");
  expectRedisResponse("SUBSCRIBE", "$9");
  expectRedisResponse("SUBSCRIBE", "subscribe");
  expectRedisResponse("SUBSCRIBE", "$14");
  expectRedisResponse("SUBSCRIBE", "remote_drawing");
  expectRedisResponse("SUBSCRIBE", ":1");
  sendStatusMessage("Subscribing to Redis channel OK.");
}

// Checks that Redis responds with expectedResponse.
// Argument "command" is used only in the error message for diagnosis
void expectRedisResponse(const char *command, const char *expectedResponse) {
  char buf[REDIS_RECEIVE_BUFFER_SIZE];
  if (! redisReceive(buf)) {
    fatalError("Got no response from Redis for command %s", command);
  } else {
    if (strcmp(buf, expectedResponse) != 0) {
      fatalError("Unexpected response from Redis for %s: \"%s\"", command, buf);
    }
  }
}

// Wait to receive Redis data until timeout
int redisReceive(char buf[REDIS_RECEIVE_BUFFER_SIZE]) {
  size_t readChars = client.readBytesUntil('\n', buf, REDIS_RECEIVE_BUFFER_SIZE - 1);
  if (readChars >= REDIS_RECEIVE_BUFFER_SIZE - 1) {
    // The message is too big, so we need to discard it until we reach NUL
    sendStatusMessageFormat("Received too big line from Redis server with size: %d", readChars);
    return 0;
  } else if (readChars > 0) {
    // LF is automatically discarded by readBytesUntil(), but there is also a CR to get rid of
    buf[readChars - 1] = '\0';
    return 1;
  } else {
    return 0;
  }
}

// Try to receive Redis data if there is any, but don't wait for it
int tryRedisReceive(char buf[REDIS_RECEIVE_BUFFER_SIZE]) {
  if (! client.available()) return 0;
  return redisReceive(buf);
}
