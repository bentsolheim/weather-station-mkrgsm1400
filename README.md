# weather-station-mkrgsm1400
Code for the Arduino MKR GSM 1400 for reading water temperature and dispatching it to a server

## Instructions

Create file src/secrets.h with contents

    #define SECRET_PINNUMBER "XXXX"
    #define SECRET_GPRS_APN "telenor.smart"
    #define SECRET_GPRS_LOGIN ""
    #define SECRET_GPRS_PASSWORD ""
    
    #define SECRET_HOSTNAME "datalogger.hostname.com"
    #define SECRET_PORT 80
    
to build.

## Notes about the sensors

### Waterproof temperature sensor

Needs to be run quite a few meters. See https://www.maximintegrated.com/en/design/technical-documents/tutorials/1/148.html .

Trying this unit first: https://www.digitalimpuls.no/diverse/145322/waterproof-digital-temperature-sensor-ds18b20-3m-kabel .
A waterproofed ds18b20 unit.