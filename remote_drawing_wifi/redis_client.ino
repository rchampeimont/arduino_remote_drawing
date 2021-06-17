// Code to connect to the Redis server
// Tested with Redis 3.0.6

#include <WiFiNINA.h>
#include "redis_client.h"
#include "serial_com.h"
#include "secrets.h"
#include "pins.h"

// Used to run commands
WiFiClient mainClient;

// Used to subscribe to channel
WiFiClient subClient;

void connectClient(WiFiClient *client) {
  sendStatusMessage("Connecting to Redis server...");
  if (! client->connect(REDIS_ADDR, REDIS_PORT)) {
    fatalError("Connection FAILED to Redis server");
  }

  sendStatusMessage("Authenticating with Redis server...");
  client->write("AUTH ");
  client->write(REDIS_PASSWORD);
  client->write("\r\n");

  expectRedisResponse(client, "AUTH", "+OK");
}

void connectToRedisServer() {
  connectClient(&subClient);
  connectClient(&mainClient);

  sendStatusMessage("Subscribing to Redis channel...");
  subClient.write("SUBSCRIBE remote_drawing\r\n");
  expectRedisResponse(&subClient, "SUBSCRIBE", "*3");
  expectRedisResponse(&subClient, "SUBSCRIBE", "$9");
  expectRedisResponse(&subClient, "SUBSCRIBE", "subscribe");
  expectRedisResponse(&subClient, "SUBSCRIBE", "$14");
  expectRedisResponse(&subClient, "SUBSCRIBE", "remote_drawing");
  expectRedisResponse(&subClient, "SUBSCRIBE", ":1");
  sendStatusMessage("Subscribing to Redis channel OK.");
}

// Checks that Redis responds with expectedResponse.
// Argument "command" is used only in the error message for diagnosis
void expectRedisResponse(WiFiClient *client, const char *command, const char *expectedResponse) {
  char buf[REDIS_RECEIVE_BUFFER_SIZE];
  if (! redisReceive(client, buf)) {
    fatalError("Got no response from Redis for command %s", command);
  } else {
    if (strcmp(buf, expectedResponse) != 0) {
      fatalError("Unexpected response for %s: \"%s\"", command, buf);
    }
  }
}

int expectRedisInteger(WiFiClient *client, const char *command) {
  char buf[REDIS_RECEIVE_BUFFER_SIZE];
  if (! redisReceive(client, buf)) {
    fatalError("Got no response from Redis for command %s", command);
    return 0; // never reached, as fatalError() never returns
  } else {
    if (buf[0] == ':') {
      return atoi(buf + 1);
    } else {
      fatalError("Expected integer for %s but got: \"%s\"", command, buf);
      return 0; // never reached, as fatalError() never returns
    }
  }
}

int expectRedisArrayCount(WiFiClient *client, const char *command) {
  char buf[REDIS_RECEIVE_BUFFER_SIZE];
  if (! redisReceive(client, buf)) {
    fatalError("Got no response from Redis for command %s", command);
    return 0; // never reached, as fatalError() never returns
  } else {
    if (buf[0] == '*') {
      return atoi(buf + 1);
    } else {
      fatalError("Expected array count for %s but got: \"%s\"", command, buf);
      return 0; // never reached, as fatalError() never returns
    }
  }
}

int expectRedisBulkStringLength(WiFiClient *client, const char *command) {
  char buf[REDIS_RECEIVE_BUFFER_SIZE];
  if (! redisReceive(client, buf)) {
    fatalError("Got no response from Redis for command %s", command);
    return 0; // never reached, as fatalError() never returns
  } else {
    if (buf[0] == '$') {
      return atoi(buf + 1);
    } else {
      fatalError("Expected bulk string length for %s but got: \"%s\"", command, buf);
      return 0; // never reached, as fatalError() never returns
    }
  }
}

// Wait to receive Redis data until timeout (1 second by default)
int redisReceive(WiFiClient *client, char buf[REDIS_RECEIVE_BUFFER_SIZE]) {
  size_t readChars = client->readBytesUntil('\n', buf, REDIS_RECEIVE_BUFFER_SIZE - 1);
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

// Receive binary data with Redis with specified sized.
// Typically used because Redis tells the same of data before sending it.
void redisReceiveBinary(WiFiClient *client, char buf[REDIS_RECEIVE_BUFFER_SIZE], int length, const char *command) {
  if (length > REDIS_RECEIVE_BUFFER_SIZE) {
    fatalError("Cannot receive Redis data of size %d which exceed buffer size", length);
  }
  int readChars = client->readBytes(buf, length);
  if (readChars != length) {
    sendStatusMessageFormat("Received %d bytes instead of %d for command %s", readChars, length, command);
  }
}

// Try to receive Redis data if there is any, but don't wait for it
int redisTryReceiveSub(char buf[REDIS_RECEIVE_BUFFER_SIZE]) {
  if (! subClient.available()) return 0;
  return redisReceive(&subClient, buf);
}

// Execute the Redis LPUSH command.
// Value is contained in "buf" of size "bufsize"
// Returns the number of elements in the array after the push
int redisLPUSH(const char *key, byte buf[], int bufsize) {
  mainClient.write("*3\r\n");
  mainClient.write("$5\r\n");
  mainClient.write("LPUSH\r\n");
  mainClient.write("$"); mainClient.print(strlen(key)); mainClient.write("\r\n");
  mainClient.write(key); mainClient.write("\r\n");
  mainClient.write("$");  mainClient.print(bufsize); mainClient.write("\r\n");
  mainClient.write(buf, bufsize); mainClient.write("\r\n");

  return expectRedisInteger(&mainClient, "LPUSH");
}

// Execute the Redis LRANGE command.
// Returns the number of elements in the array
// You should then read them with reduisReadArrayElement()
int redisLRANGE(const char *key, int start, int stop) {
  char startAsString[10];
  char stopAsString[10];
  snprintf(startAsString, 10, "%d", start);
  snprintf(stopAsString, 10, "%d", stop);
  
  mainClient.write("*4\r\n");
  mainClient.write("$6\r\n");
  mainClient.write("LRANGE\r\n");
  mainClient.write("$"); mainClient.print(strlen(key)); mainClient.write("\r\n");
  mainClient.write(key); mainClient.write("\r\n");
  mainClient.write("$"); mainClient.print(strlen(startAsString)); mainClient.write("\r\n");
  mainClient.write(startAsString); mainClient.write("\r\n");
  mainClient.write("$"); mainClient.print(strlen(stopAsString)); mainClient.write("\r\n");
  mainClient.write(stopAsString); mainClient.write("\r\n");

  return expectRedisArrayCount(&mainClient, "LRANGE");
}

// Download one array element, for used after LRANGE for instance.
// To call for each array element to read
void reduisReadArrayElement(WiFiClient *client, const char *command) {
  char buf[REDIS_RECEIVE_BUFFER_SIZE];
  int dataLength = expectRedisBulkStringLength(client, command);
  redisReceiveBinary(client, buf, dataLength, command);
  Serial.print("Read array element of size ");
  Serial.println(dataLength);
}

void redisTransmitLine(int x0, int y0, int x1, int y1) {
  int a[4] = { x0, y0, x1, y1 };
  redisLPUSH("remote_drawing_lines", (byte *) a, sizeof(a));
}

int redisDownloadLinesBegin() {
  return redisLRANGE("remote_drawing_lines", 0, -1);
}

// Goes with redisDownloadLinesBegin()
void redisDownloadLine(int *x0, int *y0, int *x1, int *y1) {
  reduisReadArrayElement(&mainClient, "LRANGE");
}
