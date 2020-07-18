# weather-station-mkrgsm1400
Code for the Arduino MKR GSM 1400 for reading water temperature and dispatching it to a server

## Instructions

Create file src/secrets.h with contents

    #define SECRET_PINNUMBER "XXXX"
    #define SECRET_GPRS_APN "telenor.smart"
    #define SECRET_GPRS_LOGIN ""
    #define SECRET_GPRS_PASSWORD ""
    
to build.