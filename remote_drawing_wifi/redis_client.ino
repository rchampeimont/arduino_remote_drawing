// Code to connect to the Redis server
// Tested with Redis 3.0.6

#include <WiFiNINA.h>
#include "redis_client.h"
#include "serial_com.h"
#include "secrets.h"
#include "pins.h"

#define MAX_LINES_IN_SEND_BUFFER 500
#define REDIS_TIMEOUT 5000 // ms
#define SEND_BUFFER_EVERY 1000 // ms
#define PING_EVERY 30000 // ms
#define REDIS_CHANNEL "remote_drawing"

int lineSendBufferIndex = 0;
Line lineSendBuffer[MAX_LINES_IN_SEND_BUFFER];
unsigned long lastSentBufferTime = millis();
unsigned long lastPingTime = millis();

// Used to run commands
WiFiClient mainClient;

// Used to subscribe to channel
WiFiClient subClient;

void connectClient(WiFiClient *client) {
  sendStatusMessage("Connecting to Redis server...");
  if (! client->connect(REDIS_ADDR, REDIS_PORT)) {
    fatalError("Connection FAILED to Redis server");
  }

  client->setTimeout(REDIS_TIMEOUT);

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
  redisSUBSCRIBE(&subClient, REDIS_CHANNEL);
  sendStatusMessage("Subscribing to Redis channel OK.");
}

void redisSUBSCRIBE(WiFiClient *client, const char *channel) {
  char s[10];

  client->write("SUBSCRIBE ");
  client->write(channel);
  client->write("\r\n");
  expectRedisResponse(&subClient, "SUBSCRIBE", "*3");
  expectRedisResponse(&subClient, "SUBSCRIBE", "$9");
  expectRedisResponse(&subClient, "SUBSCRIBE", "subscribe");
  snprintf(s, sizeof(s), "$%d", strlen(channel));
  expectRedisResponse(&subClient, "SUBSCRIBE", s);
  expectRedisResponse(&subClient, "SUBSCRIBE", channel);
  expectRedisResponse(&subClient, "SUBSCRIBE", ":1");
}

// Checks that Redis responds with expectedResponse.
// Argument "command" is used only in the error message for diagnosis
void expectRedisResponse(WiFiClient *client, const char *command, const char *expectedResponse) {
  char buf[REDIS_RECEIVE_BUFFER_SIZE];
  redisReceive(client, buf, command);
  if (strcmp(buf, expectedResponse) != 0) {
    fatalError("Unexpected response for %s: \"%s\"", command, buf);
  }
}

int expectRedisInteger(WiFiClient *client, const char *command) {
  char buf[REDIS_RECEIVE_BUFFER_SIZE];
  redisReceive(client, buf, command);
  if (buf[0] == ':') {
    return atoi(buf + 1);
  } else {
    fatalError("Expected integer for %s but got: \"%s\"", command, buf);
    return 0; // never reached, as fatalError() never returns
  }
}

int expectRedisArrayCount(WiFiClient *client, const char *command) {
  char buf[REDIS_RECEIVE_BUFFER_SIZE];
  redisReceive(client, buf, command);
  if (buf[0] == '*') {
    return atoi(buf + 1);
  } else {
    fatalError("Expected array count for %s but got: \"%s\"", command, buf);
    return 0; // never reached, as fatalError() never returns
  }
}

int expectRedisBulkStringLength(WiFiClient *client, const char *command) {
  char buf[REDIS_RECEIVE_BUFFER_SIZE];
  redisReceive(client, buf, command);
  if (buf[0] == '$') {
    return atoi(buf + 1);
  } else {
    fatalError("Expected bulk string length for %s but got: \"%s\"", command, buf);
    return 0; // never reached, as fatalError() never returns
  }
}

// Receive data with Redis until newline is reached
// Argument "command" is used only in the error message for diagnosis
void redisReceive(WiFiClient *client, char buf[REDIS_RECEIVE_BUFFER_SIZE], const char *command) {
  size_t readChars = client->readBytesUntil('\n', buf, REDIS_RECEIVE_BUFFER_SIZE - 1);
  if (readChars >= REDIS_RECEIVE_BUFFER_SIZE - 1) {
    // The message is too big, so we need to discard it until we reach NUL
    fatalError("Redis sent data that exceeded buffer size for %s", command);
  } else if (readChars > 0) {
    // LF is automatically discarded by readBytesUntil(), but there is also a CR to get rid of
    buf[readChars - 1] = '\0';
  } else {
    fatalError("Redis did not give a response for %s", command);
  }
}

// Receive binary data with Redis with specified length.
// Note: you always need to provide a buffer of REDIS_RECEIVE_BUFFER_SIZE.
// Length can at most be REDIS_RECEIVE_BUFFER_SIZE + 2.
// Typically used because Redis tells the same of data before sending it.
void redisReceiveBinary(WiFiClient *client, byte buf[REDIS_RECEIVE_BUFFER_SIZE], int length, const char *command) {
  // Redis sends the binary data followed by CRLF (so there are 2 extra bytes)
  if (length + 2 > REDIS_RECEIVE_BUFFER_SIZE) {
    fatalError("Cannot receive Redis data of size %d which exceeds buffer size", length);
  }
  int readChars = client->readBytes(buf, length + 2);
  if (readChars != length + 2) {
    fatalError("Received %d bytes instead of %d for command %s", readChars, length, command);
  }
}

// Try to receive Redis data if there is any, but don't wait for it
int redisReceiveMessage(int *fromClientId, int *newLinesStartIndex, int *newLinesStopIndex) {
  byte buf[REDIS_RECEIVE_BUFFER_SIZE];
  char s[10];

  if (! subClient.available()) return 0;

  expectRedisResponse(&subClient, "message", "*3");
  expectRedisResponse(&subClient, "message", "$7");
  expectRedisResponse(&subClient, "message", "message");
  snprintf(s, sizeof(s), "$%d", strlen(REDIS_CHANNEL));
  expectRedisResponse(&subClient, "message", s);
  expectRedisResponse(&subClient, "message", REDIS_CHANNEL);

  int dataLength = expectRedisBulkStringLength(&subClient, "message");

  if (dataLength != 6) {
    fatalError("Received Redis message has an unexpected size of %d bytes", dataLength);
  }

  // Read the actual message
  redisReceiveBinary(&subClient, buf, dataLength, "message");

  *fromClientId = *((int *) buf);
  *newLinesStartIndex = *((int *) buf + 1);
  *newLinesStopIndex = *((int *) buf + 2);

  return 1;
}

// Execute the Redis RPUSH command.
// Value is contained in "buf" of size "bufsize"
// Returns the number of elements in the array after the push
int redisRPUSH(const char *key, byte buf[], int bufsize) {
  mainClient.write("*3\r\n");
  mainClient.write("$5\r\n");
  mainClient.write("RPUSH\r\n");
  mainClient.write("$"); mainClient.print(strlen(key)); mainClient.write("\r\n");
  mainClient.write(key); mainClient.write("\r\n");
  mainClient.write("$");  mainClient.print(bufsize); mainClient.write("\r\n");
  mainClient.write(buf, bufsize); mainClient.write("\r\n");

  return expectRedisInteger(&mainClient, "RPUSH");
}


int redisBatchRPUSH(const char *key, byte buf[], int elementSize, int numberOfElements) {
  mainClient.write("*"); mainClient.print(2 + numberOfElements); mainClient.write("\r\n");
  // Send command
  mainClient.write("$5\r\n");
  mainClient.write("RPUSH\r\n");
  // Send key
  mainClient.write("$"); mainClient.print(strlen(key)); mainClient.write("\r\n");
  mainClient.write(key); mainClient.write("\r\n");
  // Send value
  for (int i = 0; i < numberOfElements; i++) {
    mainClient.write("$");  mainClient.print(elementSize); mainClient.write("\r\n");
    mainClient.write(buf + i * elementSize, elementSize); mainClient.write("\r\n");
  }

  return expectRedisInteger(&mainClient, "RPUSH");
}

// Execute the Redis LRANGE command.
// Returns the number of elements in the array
// You should then read them with redisReadArrayElement()
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

int redisPUBLISH(const char *channel, byte buf[], int bufsize) {
  mainClient.write("*3\r\n");
  mainClient.write("$7\r\n");
  mainClient.write("PUBLISH\r\n");
  mainClient.write("$"); mainClient.print(strlen(channel)); mainClient.write("\r\n");
  mainClient.write(channel); mainClient.write("\r\n");
  mainClient.write("$");  mainClient.print(bufsize); mainClient.write("\r\n");
  mainClient.write(buf, bufsize); mainClient.write("\r\n");

  return expectRedisInteger(&mainClient, "PUBLISH");
}

// Download one array element, for used after LRANGE for instance.
// To call for each array element to read
void redisReadArrayElement(WiFiClient *client, byte buf[REDIS_RECEIVE_BUFFER_SIZE], const char *command) {
  int dataLength = expectRedisBulkStringLength(client, command);
  redisReceiveBinary(client, buf, dataLength, command);
}

void redisTransmitLine(Line line) {
  // Add line in send buffer
  lineSendBuffer[lineSendBufferIndex] = line;
  lineSendBufferIndex++;

  if (lineSendBufferIndex >= MAX_LINES_IN_SEND_BUFFER) {
    Serial.println("Send buffer full");
    sendLinesInBuffer();
  }
}

void sendLinesInBuffer() {
  if (lineSendBufferIndex > 0) {
    Serial.write("Sending buffer (");
    Serial.print(lineSendBufferIndex);
    Serial.println(" lines)...");
    unsigned long start = millis();
    int newSize = redisBatchRPUSH("remote_drawing_lines", (byte *) lineSendBuffer, sizeof(Line), lineSendBufferIndex);
    unsigned long end = millis();
    Serial.write("Buffer sent (took ");
    Serial.print(end - start);
    Serial.println(" ms).");

    int newLinesStartIndex = newSize - lineSendBufferIndex;
    int newLinesStopIndex = newSize - 1;

    // Now tell tell the channel that we transmitted this data
    start = millis();
    // Our convention is to transmit the indices of the added array range
    int message[3] = { myClientId, newLinesStartIndex, newLinesStopIndex };
    redisPUBLISH(REDIS_CHANNEL, (byte *) message, sizeof(message));
    end = millis();
    Serial.write("Sent message on pub/sub (took ");
    Serial.print(end - start);
    Serial.println(" ms).");

    lineSendBufferIndex = 0;
  }
}

void runRedisPeriodicTasks() {
  unsigned long now = millis();
  if (now >= lastSentBufferTime + SEND_BUFFER_EVERY) {
    sendLinesInBuffer();
    lastSentBufferTime = now;
  }

  // Ping the server once in a while to check connection still works
  if (now >= lastPingTime + PING_EVERY) {
    redisPingAll();
    lastPingTime = now;
  }
}

void redisPingAll() {
  int mainPing = redisPing(&mainClient, false);
  int subPing = redisPing(&subClient, true);
  sendStatusMessageFormat("Ping: %d ms (main) / %d ms (subscription)", mainPing, subPing);
}

// The response format of PING is different in a subscribed connection
// vs a regular one, which is why you need to pass a boolean to
// say if the client is subscribed.
int redisPing(WiFiClient *client, bool subscribed) {
  int ping;
  unsigned long start = millis();
  client->write("PING\r\n");
  if (subscribed) {
    expectRedisResponse(client, "PING", "*2");
    expectRedisResponse(client, "PING", "$4");
    expectRedisResponse(client, "PING", "pong");
    expectRedisResponse(client, "PING", "$0");
    expectRedisResponse(client, "PING", "");
  } else {
    expectRedisResponse(client, "PING", "+PONG");
  }
  unsigned long end = millis();
  ping = end - start;
  return ping;
}

int redisDownloadLinesBegin(int start, int stop) {
  return redisLRANGE("remote_drawing_lines", start, stop);
}

void redisDownloadLine(Line *line) {
  byte buf[REDIS_RECEIVE_BUFFER_SIZE];

  redisReadArrayElement(&mainClient, buf, "LRANGE");
  *line = *((Line *) buf);
}
