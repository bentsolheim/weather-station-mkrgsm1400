#include <MKRGSM.h>
#include <Arduino.h>
#include "secrets.h"

const char PINNUMBER[]     = SECRET_PINNUMBER;
const char GPRS_APN[]      = SECRET_GPRS_APN;
const char GPRS_LOGIN[]    = SECRET_GPRS_LOGIN;
const char GPRS_PASSWORD[] = SECRET_GPRS_PASSWORD;

// initialize the library instance
GSMSSLClient client;
GPRS gprs;
GSM gsmAccess;

// URL, path and port (for example: arduino.cc)
char server[] = "httpbin.org";
char path[] = "/anything";
int port = 443; // port 443 is the default for HTTPS

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
    for (int i=0; i<count; i++) {
        blinkLed(ms);
    }
}

void setup() {
    initLed();
    blinkLed(1, 2000);

    bool connected = false;

    while (!connected) {
        blinkLed(2, 250);
        if ((gsmAccess.begin(PINNUMBER) == GSM_READY) &&
            (gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD) == GPRS_READY)) {
            connected = true;
        } else {
            delay(1000);
        }
    }

    blinkLed(1, 1000);

    // if you get a connection, report back via serial:
    if (client.connect(server, port)) {
        blinkLed(1, 1000);
        client.print("GET ");
        client.print(path);
        client.println(" HTTP/1.1");
        client.print("Host: ");
        client.println(server);
        client.println("Connection: close");
        client.println();
        blinkLed(1, 1000);
    } else {
        blinkLed(4, 250);
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
        for (;;)
            ;
    }
}
