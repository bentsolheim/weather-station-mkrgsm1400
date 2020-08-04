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

int createSenorPayload(char *payload);
void createLoggerPayload(char *payload, long timeSpent);

struct BatteryReading {
    int analogValue;
    double voltage;
    int level;
};
void readBatteryStatus(BatteryReading *reading);

GSMClient client;
GPRS gprs;
GSM gsmAccess(false);
GSMScanner gsmScanner;

DHT dhtSensor(1, DHT22);

LedMgr statusLed(LED_BUILTIN);
LedMgr errorLed(7);
GsmHelper gsmHelper(&gsmAccess, &gprs, &statusLed, PINNUMBER, GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD);
GsmHttpClient httpClient(&client, SECRET_HOSTNAME, SECRET_PORT);

int iteration = 0;
int connectionErrors = 0;
int sensorErrors = 0;

void setup() {
    if (waitForDebug) {
        Serial.begin(9600);
        while (!Serial) { ; // wait for serial port to connect. Needed for native USB port only
        }
    }
    dhtSensor.begin();
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

    //delay(5*1000);
    //return;

    Serial.println("Connecting...");
    long timeSpent = gsmHelper.connect(60000);
    if (timeSpent == -1) {
        errorLed.on();
        connectionErrors++;
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
    int error;

    statusLed.on();
    for (int i=0; i<3; i++) {
        error = createSenorPayload(payload);
        if (!error) {
            success = httpClient.post("/api/v1/logger/bua/readings", payload, response);
            if (success) {
                Serial.println(response);
            } else {
                errorLed.blink(2, 500);
            }
            break;
        } else {
            sensorErrors ++;
            Serial.println("Got error while reading sensors. Retrying...");
            delay(2000);
        }
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

void readBatteryStatus(BatteryReading *reading) {
    reading->analogValue = analogRead(ADC_BATTERY);
    double batteryMaxVoltage = 4.2;
    double batteryMinVoltage = 3.0;
    reading->voltage = reading->analogValue / 1023.0 * batteryMaxVoltage;
    reading->level = lround((reading->voltage - batteryMinVoltage) / (batteryMaxVoltage - batteryMinVoltage) * 100.0);
}


int createSenorPayload(char *payload) {
    unsigned long localTime = gsmAccess.getLocalTime();
    float t = dhtSensor.readTemperature();
    float h = dhtSensor.readHumidity();

    if (t == NAN) {
        return -1;
    }
    if (h == NAN) {
        return -2;
    }

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
    return 1;
}

void createLoggerPayload(char *payload, long timeSpent) {

    BatteryReading battery{};
    readBatteryStatus(&battery);

    char signalStrength[3];
    gsmScanner.getSignalStrength().toCharArray(signalStrength, 3);
    sprintf(payload, R"({
      "signalStrength": "%s",
      "timeSpent": %ld,
      "iteration": %d,
      "connectionErrors": %d,
      "sensorErrors": %d,
      "millisSinceStart": %lu,
      "battery": {
        "analogReading": %d,
        "voltage": %f,
        "level": %d
      }
    })", signalStrength, timeSpent, iteration, connectionErrors, sensorErrors, millis(), battery.analogValue, battery.voltage, battery.level);
}
