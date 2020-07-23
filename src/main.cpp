#include <DHT.h>
#include <MKRGSM.h>
#include <Arduino.h>
#include "secrets.h"

const char PINNUMBER[] = SECRET_PINNUMBER;
const char GPRS_APN[] = SECRET_GPRS_APN;
const char GPRS_LOGIN[] = SECRET_GPRS_LOGIN;
const char GPRS_PASSWORD[] = SECRET_GPRS_PASSWORD;

GSMClient client;
GPRS gprs;
GSM gsmAccess(false);

DHT dht(1, DHT22);

char server[] = "hw1.kilsundvaeret.no";
char path[] = "/api/v1/data-log-request";
int port = 80;

void initLed() {
    pinMode(LED_BUILTIN, OUTPUT);
}

void blinkLed(int ms) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(ms);
    digitalWrite(LED_BUILTIN, LOW);
    delay(ms);
}

void blinkLed(int count, int ms) {
    for (int i = 0; i < count; i++) {
        blinkLed(ms);
    }
}

void setup() {
    //Serial.begin(9600);
    //while (!Serial) { ; // wait for serial port to connect. Needed for native USB port only
    //}
    dht.begin();
    initLed();
    blinkLed(1, 2000);

    float t = dht.readTemperature();
    Serial.print("Temp: ");
    Serial.println(t);

    gsmAccess.begin(PINNUMBER, true, false);
    while (!gsmAccess.ready()) {
        blinkLed(1, 100);
    }

    gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD, false);
    while (!gprs.ready()) {
        blinkLed(1, 100);
    }

    blinkLed(1, 2000);

    int connected = client.connect(server, port);
    if (connected) {
        char request[400];
        char payload[400];
        sprintf(payload, R"({"loggerId": "bua", "sensorName": "inne-temp", "value": %f })", t);

        strcpy(request, "POST ");
        strcat(request, path);
        strcat(request, " HTTP/1.1\n");
        strcat(request, "Host: ");
        strcat(request, server);
        strcat(request, "\n");
        strcat(request, "Connection: close\n");
        strcat(request, "Content-Type: application/json\n");
        strcat(request, "Content-Length: ");
        char contentLength[5];
        itoa(strlen(payload),contentLength,10);
        strcat(request, contentLength);
        strcat(request, "\n\n");
        strcat(request, payload);
        strcat(request, "\n");

        Serial.println(request);

        blinkLed(1, 1000);
        client.println(request);
        blinkLed(1, 1000);

        Serial.println("Data sent");
    } else {
        blinkLed(4, 250);
        blinkLed(1, 3000);
    }
}

void loop() {
    // if there are incoming bytes available
    // from the server, read them and print them:
    if (client.available()) {
        client.read();
    }

    // if the server's disconnected, stop the client:
    if (!client.available() && !client.connected()) {
        client.stop();
        blinkLed(10, 100);

        // do nothing forevermore:
        while(true) {
            delayMicroseconds(100);
        }
    }
}
