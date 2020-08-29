#include <DHT.h>
#include <MKRGSM.h>
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_SleepyDog.h>
#include "secrets.h"
#include "GsmHttpClient.cpp"
#include "LedMgr.h"

bool waitForDebug = false;

const int SLEEP_TIME_MILLIS = 5 * 60 * 1000;
//const int SLEEP_TIME_MILLIS = 30 * 1000;
const int WATCHDOG_TIMEOUT = 16000;

const int PIN_SENSOR_POWER_MOSFET = 0;
const int PIN_DHT_SENSOR = 1;
const int PIN_ONE_WIRE_SENSOR = 2;
const int PIN_STATUS_LED = LED_BUILTIN;
const int PIN_ERROR_LED = 7;

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

DHT dhtSensor(PIN_DHT_SENSOR, DHT22);
OneWire oneWire(PIN_ONE_WIRE_SENSOR);
DallasTemperature sensors(&oneWire);

LedMgr statusLed(PIN_STATUS_LED);
LedMgr errorLed(PIN_ERROR_LED);
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

    pinMode(0, OUTPUT);
}

void loop() {
    Watchdog.enable(WATCHDOG_TIMEOUT);

    iteration++;
    Serial.print("Iteration ");
    Serial.println(iteration);
    statusLed.blink(1, 100);

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
    int statusCode;

    digitalWrite(PIN_SENSOR_POWER_MOSFET, HIGH);
    delay(2000);

    statusLed.on();
    for (int i = 0; i < 3; i++) {
        Watchdog.reset();
        statusCode = createSenorPayload(payload);
        if (statusCode != 1) {
            errorLed.blink(1, 1500);
            errorLed.blink(abs(statusCode), 250);
            sensorErrors++;
            Serial.println("Got error while reading sensors. Retrying...");
            delay(2000);
            continue;
        }
        success = httpClient.post("/api/v1/logger/bua/readings", payload, response);
        if (success) {
            Serial.println(response);
        } else {
            errorLed.blink(3, 500);
        }
        break;
    }

    Watchdog.reset();
    createLoggerPayload(payload, timeSpent);
    success = httpClient.post("/api/v1/logger/bua/debug", payload, response);
    if (success) {
        Serial.println(response);
    } else {
        errorLed.blink(4, 250);
    }

    Watchdog.reset();
    gsmHelper.disconnect();

    statusLed.blink(2, 100);
    statusLed.off();

    digitalWrite(PIN_SENSOR_POWER_MOSFET, LOW);

    Watchdog.disable();

    Watchdog.sleep(SLEEP_TIME_MILLIS);
}

void readBatteryStatus(BatteryReading *reading) {
    reading->analogValue = analogRead(ADC_BATTERY);
    double batteryMaxVoltage = 4.2;
    double analogMaxValue = 1009.0;
    double analogMinValue = 810.0;
    reading->voltage = reading->analogValue / analogMaxValue * batteryMaxVoltage;
    reading->level = std::fmax(
            std::fmin(lround((reading->analogValue - analogMinValue) / (analogMaxValue - analogMinValue) * 100.0),
                      100.0), 0);
}


int createSenorPayload(char *payload) {
    unsigned long localTime = gsmAccess.getLocalTime();
    unsigned long unixTime = gsmAccess.getTime();
    float temperatureInside = dhtSensor.readTemperature();
    float temperatureOutside = dhtSensor.readHumidity();

    if (isnan(temperatureInside)) {
        return -1;
    }
    if (isnan(temperatureOutside)) {
        return -2;
    }

    sensors.requestTemperatures();
    float waterTemp = sensors.getTempCByIndex(0);
    if (waterTemp == DEVICE_DISCONNECTED_C) {
        return -3;
    }

    sprintf(payload, R"({
  "readings": [
    {
      "sensorName": "inne-temp",
      "value": %f,
      "localTime": %lu,
      "unixTime": %lu
    },
    {
      "sensorName": "inne-humidity",
      "value": %f,
      "localTime": %lu,
      "unixTime": %lu
    },
    {
      "sensorName": "vann-temp",
      "value": %f,
      "localTime": %lu,
      "unixTime": %lu
    }
  ]
})",
            temperatureInside, localTime, unixTime,
            temperatureOutside, localTime, unixTime,
            waterTemp, localTime, unixTime);
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
    })", signalStrength, timeSpent, iteration, connectionErrors, sensorErrors, millis(), battery.analogValue,
            battery.voltage, battery.level);
}
