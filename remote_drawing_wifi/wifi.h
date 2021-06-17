#ifndef WIFI_FUNCTIONS
#define WIFI_FUNCTIONS

bool searchForNetwork(char target[]);
void printEncryptionType(int thisType);
void print2Digits(byte thisByte);
void printMacAddress(byte mac[]);
void initWifi();
void printCurrentNet();

#endif
