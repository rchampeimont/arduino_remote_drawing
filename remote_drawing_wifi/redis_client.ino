// Code to connect to the Redis server
// Tested with Redis 3.0.6

#include <WiFiNINA.h>
#include "redis_client.h"
#include "serial_com.h"
#include "secrets.h"
#include "system.h"

#define MAX_LINES_IN_SEND_BUFFER 200
#define SEND_BUFFER_EVERY 500 // ms
#define REDIS_TIMEOUT 5000 // ms
#define PING_EVERY 30000 // ms
#define REDIS_CHANNEL "remote_drawing"
#define REDIS_LINES_KEY "remote_drawing_lines"

// Send buffer stuff
volatile byte currentBufferForWrite = 0;
volatile Line lineSendBuffer0[MAX_LINES_IN_SEND_BUFFER];
volatile byte linesInBuffer0 = 0;
volatile Line lineSendBuffer1[MAX_LINES_IN_SEND_BUFFER];
volatile byte linesInBuffer1 = 0;
volatile bool askedClear = false;

// Ping-related stuff
unsigned long subPingSentAt = 0; // Time when we sent the last PING in the subscription connection
int lastMainPing = 0; // Last ping latency measured in ms

// Used to run commands
WiFiClient mainClient;

// Used to subscribe to channel
WiFiClient subClient;

void connectClient(WiFiClient *client) {
  byte failures = 0;
  sendStatusMessage("Connecting to Redis server...");
  while (! client->connect(REDIS_ADDR, REDIS_PORT)) {
    failures++;
    if (failures < 3) {
      // Initial connection to Redis often fails (like 1 every 10 times),
      // so retry a few times.
      sendStatusMessage("Connection failed to Redis server. Retrying...");
    } else {
      fatalError("Connection FAILED to Redis server.");
    }
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

long expectRedisArrayCount(WiFiClient *client, const char *command) {
  char buf[REDIS_RECEIVE_BUFFER_SIZE];
  redisReceive(client, buf, command);
  if (buf[0] == '*') {
    return atol(buf + 1);
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
int redisReceiveMessage(RedisMessage *redisMessage) {
  byte buf[REDIS_RECEIVE_BUFFER_SIZE];
  char s[10];

  if (! subClient.available()) return 0;

  redisReceive(&subClient, (char *) buf, "subscription");
  if (strcmp((char *) buf, "*3") == 0) {
    // We are receiving a message from a publisher
    expectRedisResponse(&subClient, "message", "$7");
    expectRedisResponse(&subClient, "message", "message");
    snprintf(s, sizeof(s), "$%d", strlen(REDIS_CHANNEL));
    expectRedisResponse(&subClient, "message", s);
    expectRedisResponse(&subClient, "message", REDIS_CHANNEL);

    int dataLength = expectRedisBulkStringLength(&subClient, "message");

    if (dataLength != sizeof(RedisMessage)) {
      fatalError("Received Redis message has an unexpected size of %d bytes", dataLength);
    }

    // Read the actual message
    redisReceiveBinary(&subClient, (byte *) redisMessage, dataLength, "message");

    return 1;
  } else if (strcmp((char *) buf, "*2") == 0) {
    // We are receiving the response for a PING we sent
    expectRedisResponse(&subClient, "pong", "$4");
    expectRedisResponse(&subClient, "pong", "pong");
    expectRedisResponse(&subClient, "pong", "$0");
    expectRedisResponse(&subClient, "pong", "");
    int subPing = millis() - subPingSentAt;
    subPingSentAt = 0;

    sendStatusMessageFormat("Redis ping: %d ms (main) %d ms (sub.)         Uptime: %d min", lastMainPing, subPing, millis() / 60000);

    // Return 0 since we didn't write an actual message to the passed variable
    return 0;
  } else {
    fatalError("Unexpected data in subscription: \"%s\"", buf);
    return 0;
  }
}

// Execute the Redis DEL command
int redisDEL(const char *key) {
  mainClient.write("*2\r\n");
  mainClient.write("$3\r\n");
  mainClient.write("DEL\r\n");
  mainClient.write("$"); mainClient.print(strlen(key)); mainClient.write("\r\n");
  mainClient.write(key); mainClient.write("\r\n");

  return expectRedisInteger(&mainClient, "DEL");
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
long redisLRANGE(const char *key, long start, long stop) {
  char startAsString[12];
  char stopAsString[12];
  snprintf(startAsString, 12, "%ld", start);
  snprintf(stopAsString, 12, "%ld", stop);

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

// This function is called from interrupt
void redisAddLineToSendBuffer(Line line) {
  volatile Line* targetAddress;
  volatile byte *linesInBufferAddress;
  if (currentBufferForWrite == 0) {
    targetAddress = lineSendBuffer0 + linesInBuffer0;
    linesInBufferAddress = &linesInBuffer0;
  } else {
    targetAddress = lineSendBuffer1 + linesInBuffer1;
    linesInBufferAddress = &linesInBuffer1;
  }

  // Add line in send buffer
  *((Line *) targetAddress) = line;
  (*linesInBufferAddress)++;

  if ((*linesInBufferAddress) >= MAX_LINES_IN_SEND_BUFFER) {
    fatalError("Send buffer %d full", currentBufferForWrite);
  }
}

void sendLinesInBuffer() {
  if (linesInBuffer0 == 0 && linesInBuffer1 == 0) {
    // nothing to send
    return;
  }

  // First, change the write pointer so that received data now goes to the other buffer.
  // Since the writes are triggered by interrupt they can happen while this function is running.
  currentBufferForWrite = !currentBufferForWrite;

  volatile Line *bufferToRead;
  volatile byte *linesInBufferAddress;
  if (currentBufferForWrite == 0) {
    bufferToRead = lineSendBuffer1;
    linesInBufferAddress = &linesInBuffer1;
  } else {
    bufferToRead = lineSendBuffer0;
    linesInBufferAddress = &linesInBuffer0;
  }

  if (*linesInBufferAddress == 0) {
    fatalError("Internal bug: non-empty buffer is the wrong one");
  }

  Serial.write("Sending buffer ");
  Serial.print(!currentBufferForWrite);
  Serial.write(" (");
  Serial.print(*linesInBufferAddress);
  Serial.println(" lines)...");
  unsigned long start = millis();
  int newSize = redisBatchRPUSH(
                  REDIS_LINES_KEY,
                  (byte *) bufferToRead,
                  sizeof(Line),
                  *linesInBufferAddress);
  unsigned long end = millis();
  Serial.write("Buffer sent (took ");
  Serial.print(end - start);
  Serial.println(" ms).");

  // Compute the array index interval on the server where we just wrote our data
  int newLinesStartIndex = newSize - *linesInBufferAddress;
  int newLinesStopIndex = newSize - 1;

  // Now tell the other client where the new data we addded is
  RedisMessage redisMessage;
  redisInitMessage(&redisMessage);
  redisMessage.opcode = REDIS_LINE_OPCODE;
  redisMessage.data.lineInterval.newLinesStartIndex = newLinesStartIndex;
  redisMessage.data.lineInterval.newLinesStopIndex = newLinesStopIndex;
  redisTransmitMessage(redisMessage);

  // Mark the sent buffer as empty
  *linesInBufferAddress = 0;
}

void redisInitMessage(RedisMessage *redisMessage) {
  memset(redisMessage, 0, sizeof(RedisMessage));
  redisMessage->fromClientId = myClientId;
}

void redisTransmitMessage(RedisMessage redisMessage) {
  unsigned long start = millis();
  redisPUBLISH(REDIS_CHANNEL, (byte *) &redisMessage, sizeof(RedisMessage));
  unsigned long end = millis();
  Serial.write("Sent message on pub/sub (took ");
  Serial.print(end - start);
  Serial.println(" ms).");
}

void redisPlanClearDrawing() {
  // We are going to clear the drawing anyway, so get rid of any lines waiting to be sent
  linesInBuffer0 = 0;
  linesInBuffer1 = 0;

  // Mark
  askedClear = true;
}

void redisClearDrawing() {
  Serial.println("Clearing drawing on server...");

  redisDEL(REDIS_LINES_KEY);

  RedisMessage redisMessage;
  redisInitMessage(&redisMessage);
  redisMessage.opcode = REDIS_CLEAR_OPCODE;
  redisTransmitMessage(redisMessage);

  // Reset askedClear flag
  askedClear = false;
}

void runRedisPeriodicTasks() {
  static unsigned long lastSentBufferTime = millis();
  static unsigned long lastPingTime = millis();

  unsigned long now = millis();

  // Send lines in buffer
  if (now >= lastSentBufferTime + SEND_BUFFER_EVERY) {
    if (askedClear) {
      redisClearDrawing();
    }
    sendLinesInBuffer();

    lastSentBufferTime = now;
  }

  // Ping the server once in a while to check connection still works
  if (now >= lastPingTime + PING_EVERY) {
    // Ping both the main client and the subscription client
    redisPingMainClient();
    redisPingSubClient();

    lastPingTime = now;
  }
}

void redisPingMainClient() {
  unsigned long start = millis();
  mainClient.write("PING\r\n");
  expectRedisResponse(&mainClient, "PING", "+PONG");
  lastMainPing = millis() - start;
}

void redisPingSubClient() {
  if (subPingSentAt == 0) {
    subPingSentAt = millis();
    subClient.write("PING\r\n");
  } else {
    // If subPingSent is non-zero, we are still waiting for a response to a previous ping
    if (millis() > subPingSentAt + REDIS_TIMEOUT) {
      fatalError("No response to PING on Redis subscription connection");
    }
  }
}

long redisDownloadLinesBegin(long start, long stop) {
  return redisLRANGE(REDIS_LINES_KEY, start, stop);
}

void redisDownloadLine(Line *line) {
  byte buf[REDIS_RECEIVE_BUFFER_SIZE];

  redisReadArrayElement(&mainClient, buf, "LRANGE");
  *line = *((Line *) buf);
}
