#ifndef WIFI_H
#define WIFI_H

bool searchForNetwork(char target[]);
void printEncryptionType(int thisType);
void print2Digits(byte thisByte);
void printMacAddress(byte mac[]);
void initWifi();
void printCurrentNet();

#endif
