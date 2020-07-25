#include "LedMgr.h"
#include <MKRGSM.h>

class GsmHelper {

public:
    GsmHelper(GSM *gsm, GPRS *gprs, LedMgr *statusLed, const char *pin, const char *gprsApn,
              const char *gprsLogin, const char *gprsPassword) {
        this->gsmAccess = gsm;
        this->gprs = gprs;
        this->statusLed = statusLed;

        this->pin = pin;
        this->gprsApn = gprsApn;
        this->gprsLogin = gprsLogin;
        this->gprsPassword = gprsPassword;
    }

    long connect(unsigned long timeoutMillis) {
        unsigned long start = millis();

        statusLed->on();
        gsmAccess->setTimeout(timeoutMillis);
        gsmAccess->begin(this->pin, true, false);
        while (gsmAccess->ready() != 1) {
            long errorState = checkConnectionStatus();
            if (errorState != 0) {
                return errorState;
            }
        }

        gprs->setTimeout(timeoutMillis - (long)(millis() - start));
        gprs->attachGPRS(this->gprsApn, this->gprsLogin, this->gprsPassword, false);
        while (gprs->ready() != 1) {
            long errorState = checkConnectionStatus();
            if (errorState != 0) {
                return errorState;
            }
        }
        statusLed->off();

        return (long) (millis() - start);
    }

    void disconnect() {
        gprs->detachGPRS();
        gsmAccess->shutdown();
    }

private:
    GSM *gsmAccess;
    GPRS *gprs;
    LedMgr *statusLed;

    const char *pin;
    const char *gprsApn;
    const char *gprsLogin;
    const char *gprsPassword;

    long checkConnectionStatus() const {
        statusLed->blink(1, 100);
        GSM3_NetworkStatus_t status = gsmAccess->status();
        if (status == ERROR) {
            return -1;
        }
        return 0;
    }
};

class GsmHttpClient {
private:
    GSMClient *client;
    const char *server;
    int port;

public:
    explicit GsmHttpClient(GSMClient *client, const char *server, int port) {
        this->client = client;
        this->server = server;
        this->port = port;
    }

    int post(const char *path, const char *payload, char *response) {
        int connected = client->connect(server, port);
        if (!connected) {
            return 0;
        }
        char request[strlen(payload) + 200];

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
        itoa(strlen(payload), contentLength, 10);
        strcat(request, contentLength);
        strcat(request, "\n\n");
        strcat(request, payload);
        strcat(request, "\n");

        Serial.println(request);

        client->print(request);

        Serial.println("Data sent");

        int i = 0;
        while (true) {
            if (client->available()) {
                char c = client->read();
                response[i++] = c;
            }
            if (!client->available() && !client->connected()) {
                response[i] = '\0';
                break;
            }
            delayMicroseconds(100);
        }

        client->stop();

        return 1;
    }
};
