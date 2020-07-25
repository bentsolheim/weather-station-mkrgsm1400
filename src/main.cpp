#include <DHT.h>
#include <MKRGSM.h>
#include <Arduino.h>
#include "secrets.h"
#include "GsmHttpClient.cpp"
#include "LedMgr.h"

bool waitForDebug = false;

const char PINNUMBER[] = SECRET_PINNUMBER;
const char GPRS_APN[] = SECRET_GPRS_APN;
const char GPRS_LOGIN[] = SECRET_GPRS_LOGIN;
const char GPRS_PASSWORD[] = SECRET_GPRS_PASSWORD;

void createSenorPayload(char *payload);
void createLoggerPayload(char *payload, long timeSpent);

GSMClient client;
GPRS gprs;
GSM gsmAccess(false);
GSMScanner gsmScanner;

DHT dht(1, DHT22);

LedMgr statusLed(LED_BUILTIN);
LedMgr errorLed(7);
GsmHelper gsmHelper(&gsmAccess, &gprs, &statusLed, PINNUMBER, GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD);
GsmHttpClient httpClient(&client, SECRET_HOSTNAME, SECRET_PORT);

int iteration = 0;
int errors = 0;

void setup() {
    if (waitForDebug) {
        Serial.begin(9600);
        while (!Serial) { ; // wait for serial port to connect. Needed for native USB port only
        }
    }
    dht.begin();
    statusLed.on();
    errorLed.on();
    delay(1000);
    statusLed.off();
    errorLed.off();
    delay(1000);
}

void loop() {
    iteration++;
    Serial.print("Iteration ");
    Serial.println(iteration);
    statusLed.blink(1, 100);

    Serial.println("Connecting...");
    long timeSpent = gsmHelper.connect(60000);
    if (timeSpent == -1) {
        errorLed.on();
        errors++;
        delay(60000);
        gsmScanner.begin();
        return;
    }

    errorLed.off();

    Serial.print("Connected in ");
    Serial.print(timeSpent);
    Serial.print(" millis");
    Serial.println();

    char payload[400];
    char response[400];
    int success;

    statusLed.on();
    createSenorPayload(payload);
    success = httpClient.post("/api/v1/logger/bua/readings", payload, response);
    if (success) {
        Serial.println(response);
    } else {
        errorLed.blink(2, 500);
    }

    createLoggerPayload(payload, timeSpent);
    success = httpClient.post("/api/v1/logger/bua/debug", payload, response);
    if (success) {
        Serial.println(response);
    } else {
        errorLed.blink(4, 250);
    }

    gsmHelper.disconnect();

    statusLed.blink(2, 100);
    statusLed.off();

    delay(5*60*1000);
}


void createSenorPayload(char *payload) {
    unsigned long localTime = gsmAccess.getLocalTime();
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    sprintf(payload, R"({
  "readings": [
    {
      "sensorName": "inne-temp",
      "value": %f,
      "localTime": %lu
    },
    {
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
