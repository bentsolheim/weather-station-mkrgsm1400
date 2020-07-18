#include <MKRGSM.h>
#include <Arduino.h>
#include "secrets.h"

// Please enter your sensitive data in the Secret tab or arduino_secrets.h
// PIN Number
const char PINNUMBER[] = SECRET_PINNUMBER;

GSM gsmAccess;
GSM_SMS sms;
GSMPIN gsmPin;

/*
  Read input serial
 */
int readSerial(char result[]) {
    int i = 0;
    while (1) {
        while (Serial.available() > 0) {
            char inChar = Serial.read();
            if (inChar == '\n') {
                result[i] = '\0';
                Serial.flush();
                return 0;
            }
            if (inChar != '\r') {
                result[i] = inChar;
                i++;
            }
        }
    }
}

void initLed() {
    pinMode(LED_BUILTIN, OUTPUT);
}

void blinkLed(int ms) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(ms);
    digitalWrite(LED_BUILTIN, LOW);
    delay(ms);
}

void setup() {
    initLed();
    blinkLed(500);
    // initialize serial communications and wait for port to open
    Serial.begin(9600);
    while (!Serial) { ; // wait for serial port to connect. Needed for native USB port only
    }
    blinkLed(500);

    Serial.println("SMS Messages Sender");
    //Serial.println(PINNUMBER);

    gsmPin.begin();
    Serial.println("Restarted modem");
    Serial.println(gsmPin.getPINUsed());
    Serial.println(gsmPin.checkReg());
    Serial.println(gsmPin.isPIN());
    if (gsmPin.checkPIN(PINNUMBER) == 0) {
        Serial.println("Pin OK");
    } else {
        Serial.println("Pin incorrect");
    }

    // connection state
    bool connected = false;

    // Start GSM shield
    // If your SIM has PIN, pass it as a parameter of begin() in quotes
    while (!connected) {
        Serial.println("Connecting...");
        int status = gsmAccess.begin(PINNUMBER);
        if (status == GSM_READY) {
            connected = true;
        } else {
            if (status == ERROR) {
                Serial.println("ERROR");
            } else if (status == CONNECTING) {
                Serial.println("CONNECTING");
            } else if (status == GSM_READY) {
                Serial.println("GSM_READY");
            } else if (status == GPRS_READY) {
                Serial.println("GPRS_READY");
            } else if (status == TRANSPARENT_CONNECTED) {
                Serial.println("TRANSPARENT_CONNECTED");
            } else if (status == IDLE) {
                Serial.println("IDLE");
            } else {
                Serial.println("Unknown status");
            }
            delay(1000);
        }
    }

    Serial.println("GSM initialized");
}

void loop() {

    Serial.print("Enter a mobile number: ");
    char remoteNum[20];  // telephone number to send sms
    readSerial(remoteNum);
    Serial.println(remoteNum);

    // sms text
    Serial.print("Now, enter SMS content: ");
    char txtMsg[200];
    readSerial(txtMsg);
    Serial.println("SENDING");
    Serial.println();
    Serial.println("Message:");
    Serial.println(txtMsg);

    // send the message
    sms.beginSMS(remoteNum);
    sms.print(txtMsg);
    sms.endSMS();
    Serial.println("\nCOMPLETE!\n");
}

