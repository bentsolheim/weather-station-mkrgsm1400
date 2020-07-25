#include <DHT.h>
#include <MKRGSM.h>
#include <Arduino.h>
#include "secrets.h"
#include "GsmHttpClient.cpp"
#include "LedMgr.h"

const char PINNUMBER[] = SECRET_PINNUMBER;
const char GPRS_APN[] = SECRET_GPRS_APN;
const char GPRS_LOGIN[] = SECRET_GPRS_LOGIN;
const char GPRS_PASSWORD[] = SECRET_GPRS_PASSWORD;

GSMClient client;
GPRS gprs;
GSM gsmAccess(false);
GSMScanner gsmScanner;

DHT dht(1, DHT22);

LedMgr statusLed(LED_BUILTIN);
GsmHelper gsmHelper(&gsmAccess, &gprs, &statusLed, PINNUMBER, GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD);
GsmHttpClient httpClient(&client, "hw1.kilsundvaeret.no", 80);

void setup() {
    Serial.begin(9600);
    while (!Serial) { ; // wait for serial port to connect. Needed for native USB port only
    }
    dht.begin();
    statusLed.blink(1, 2000);
}

int iteration = 0;
int errors = 0;

void createSenorPayload(char *payload);
void createLoggerPayload(char *payload, long timeSpent);

void loop() {
    iteration++;
    Serial.print("Iteration ");
    Serial.println(iteration);
    statusLed.blink(3, 333);

    Serial.println("Connecting...");
    long timeSpent = gsmHelper.connect(60000);
    if (timeSpent == -1) {
        errors++;
        Serial.println("Error while connecting. Taking a break...");
        delay(60000);
        Serial.println("Resetting modem");
        gsmScanner.begin();
        Serial.println("Modem reset");
        return;
    }

    Serial.print("Connected in ");
    Serial.print(timeSpent);
    Serial.print(" millis");
    Serial.println();

    char payload[400];
    char response[400];
    int success;

    createSenorPayload(payload);
    success = httpClient.post("/api/v1/data-log-request", payload, response);
    if (success) {
        Serial.println(response);
    } else {
        statusLed.blink(3, 1000);
    }

    createLoggerPayload(payload, timeSpent);
    success = httpClient.post("/api/v1/logger/bua/debug", payload, response);
    if (success) {
        Serial.println(response);
    } else {
        statusLed.blink(3, 1000);
    }

    gsmHelper.disconnect();

    statusLed.blink(3, 333);

    delay(10000);
}


void createSenorPayload(char *payload) {
    unsigned long localTime = gsmAccess.getLocalTime();
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    sprintf(payload, R"({
  "data": [
    {
      "loggerId": "bua",
      "sensorName": "inne-temp",
      "value": %f,
      "localTime": %lu
    },
    {
      "loggerId": "bua",
      "sensorName": "inne-humidity",
      "value": %f,
      "localTime": %lu
    }
  ]
})", t, localTime, h, localTime);
}

void createLoggerPayload(char *payload, long timeSpent) {
    char signalStrength[3];
    gsmScanner.getSignalStrength().toCharArray(signalStrength, 3);
    sprintf(payload, R"({
      "signalStrength": "%s",
      "timeSpent": %ld,
      "iteration": %d,
      "errors": %d,
      "millisSinceStart": %lu
    })", signalStrength, timeSpent, iteration, errors, millis());
}
