/*
 * Header file for ems.cpp
 *
 */

#pragma once

#include <Arduino.h>

// EMS IDs
#define EMS_ID_NONE 0x00 // Fixed - used as a dest in broadcast messages and empty type IDs
#define EMS_ID_ME 0x0B   // Fixed - our device, hardcoded as the "Service Key"

#define EMS_MIN_TELEGRAM_LENGTH 6 // minimal length for a validation telegram, including CRC

// max length of a telegram, including CRC, for Rx and Tx.
// This can differs per firmware version and typically 32 is the max
#define EMS_MAX_TELEGRAM_LENGTH 99

// Common for all EMS devices
#define EMS_TYPE_Version 0x02 // version of the UBA controller (boiler)

// default values
#define EMS_VALUE_INT_ON 1             // boolean true
#define EMS_VALUE_INT_OFF 0            // boolean false
#define EMS_VALUE_INT_NOTSET 0xFF      // for 8-bit ints
#define EMS_VALUE_LONG_NOTSET 0xFFFFFF // for 3-byte longs
#define EMS_VALUE_FLOAT_NOTSET -255    // float unset

#define EMS_THERMOSTAT_READ_YES true
#define EMS_THERMOSTAT_READ_NO false
#define EMS_THERMOSTAT_WRITE_YES true
#define EMS_THERMOSTAT_WRITE_NO false

// ANSI Colors
#define COLOR_RESET "\x1B[0m"
#define COLOR_BLACK "\x1B[0;30m"
#define COLOR_RED "\x1B[0;31m"
#define COLOR_GREEN "\x1B[0;32m"
#define COLOR_YELLOW "\x1B[0;33m"
#define COLOR_BLUE "\x1B[0;34m"
#define COLOR_MAGENTA "\x1B[0;35m"
#define COLOR_CYAN "\x1B[0;36m"
#define COLOR_WHITE "\x1B[0;37m"
#define COLOR_BOLD_ON "\x1B[1m"
#define COLOR_BOLD_OFF "\x1B[21m"

// trigger settings to determine if hot tap water or the heating is active
#define EMS_BOILER_BURNPOWER_TAPWATER 100
#define EMS_BOILER_SELFLOWTEMP_HEATING 70

//define maximum settable tapwater temperature, not every installation supports 90 degrees
#define EMS_BOILER_TAPWATER_TEMPERATURE_MAX 60

#define EMS_TX_TELEGRAM_QUEUE_MAX 50 // max size of Tx FIFO queue


/* EMS UART transfer status */
typedef enum {
    EMS_RX_IDLE,
    EMS_RX_ACTIVE // Rx package is being sent
} _EMS_RX_STATUS;

typedef enum {
    EMS_TX_IDLE,
    EMS_TX_ACTIVE, // Tx package being sent, no break sent
    EMS_TX_SUCCESS,
    EMS_TX_ERROR
} _EMS_TX_STATUS;

typedef enum {
    EMS_TX_TELEGRAM_INIT,     // just initialized
    EMS_TX_TELEGRAM_READ,     // doing a read request
    EMS_TX_TELEGRAM_WRITE,    // doing a write request
    EMS_TX_TELEGRAM_VALIDATE, // do a read but only to validate the last write
    EMS_TX_TELEGRAM_RAW       // sending in raw mode
} _EMS_TX_TELEGRAM_ACTION;

/* EMS logging */
typedef enum {
    EMS_SYS_LOGGING_NONE,       // no messages
    EMS_SYS_LOGGING_RAW,        // raw data mode
    EMS_SYS_LOGGING_BASIC,      // only basic read/write messages
    EMS_SYS_LOGGING_THERMOSTAT, // only telegrams sent from thermostat
    EMS_SYS_LOGGING_VERBOSE     // everything
} _EMS_SYS_LOGGING;

// status/counters since last power on
typedef struct {
    _EMS_RX_STATUS   emsRxStatus;
    _EMS_TX_STATUS   emsTxStatus;
    uint16_t         emsRxPgks;       // received
    uint16_t         emsTxPkgs;       // sent
    uint16_t         emxCrcErr;       // CRC errors
    bool             emsPollEnabled;  // flag enable the response to poll messages
    bool             emsTxEnabled;    // flag if we're allowing sending of Tx packages
    _EMS_SYS_LOGGING emsLogging;      // logging
    bool             emsRefreshed;    // fresh data, needs to be pushed out to MQTT
    bool             emsBusConnected; // is there an active bus
    unsigned long    emsRxTimestamp;  // timestamp of last EMS poll
} _EMS_Sys_Status;

// The Tx send package
typedef struct {
    _EMS_TX_TELEGRAM_ACTION action; // read or write
    uint8_t                 dest;
    uint8_t                 type;
    uint8_t                 offset;
    uint8_t                 length;
    uint8_t                 dataValue;          // value to validate against
    uint8_t                 type_validate;      // type to call after a successful Write command
    uint8_t                 comparisonValue;    // value to compare against during a validate
    uint8_t                 comparisonOffset;   // offset of where the byte is we want to compare too later
    uint8_t                 comparisonPostRead; // after a successful write call this to read
    bool                    forceRefresh;       // should we send to MQTT after a successful Tx?
    unsigned long           timestamp;          // when created
    uint8_t                 data[EMS_MAX_TELEGRAM_LENGTH];
} _EMS_TxTelegram;

// default empty Tx
const _EMS_TxTelegram EMS_TX_TELEGRAM_NEW = {
    EMS_TX_TELEGRAM_INIT, // action
    EMS_ID_NONE,          // dest
    EMS_ID_NONE,          // type
    0,                    // offset
    0,                    // length
    0,                    // data value
    EMS_ID_NONE,          // type_validate
    0,                    // comparisonValue
    0,                    // comparisonOffset
    EMS_ID_NONE,          // comparisonPostRead
    false,                // forceRefresh
    0,                    // timestamp
    {0x00}                // data
};

typedef struct {
    uint8_t model_id;
    uint8_t product_id;
    uint8_t type_id;
    char    model_string[50];
} _Model_Type;

/*
 * Telegram package defintions
 */
typedef struct {           // UBAParameterWW
    uint8_t wWActivated;   // Warm Water activated
    uint8_t wWSelTemp;     // Warm Water selected temperature
    uint8_t wWCircPump;    // Warm Water circulation pump Available
    uint8_t wWDesiredTemp; // Warm Water desired temperature
    uint8_t wWComfort;     // Warm water comfort or ECO mode

    // UBAMonitorFast
    uint8_t selFlowTemp;        // Selected flow temperature
    float   curFlowTemp;        // Current flow temperature
    float   retTemp;            // Return temperature
    uint8_t burnGas;            // Gas on/off
    uint8_t fanWork;            // Fan on/off
    uint8_t ignWork;            // Ignition on/off
    uint8_t heatPmp;            // Circulating pump on/off
    uint8_t wWHeat;             // 3-way valve on WW
    uint8_t wWCirc;             // Circulation on/off
    uint8_t selBurnPow;         // Burner max power
    uint8_t curBurnPow;         // Burner current power
    float   flameCurr;          // Flame current in micro amps
    float   sysPress;           // System pressure
    char    serviceCodeChar[2]; // 2 character status/service code

    // UBAMonitorSlow
    float    extTemp;     // Outside temperature
    float    boilTemp;    // Boiler temperature
    uint8_t  pumpMod;     // Pump modulation
    uint32_t burnStarts;  // # burner restarts
    uint32_t burnWorkMin; // Total burner operating time
    uint32_t heatWorkMin; // Total heat operating time

    // UBAMonitorWWMessage
    float    wWCurTmp;  // Warm Water current temperature:
    uint32_t wWStarts;  // Warm Water # starts
    uint32_t wWWorkM;   // Warm Water # minutes
    uint8_t  wWOneTime; // Warm Water one time function on/off
    uint8_t  wWCurFlow; // Warm Water current flow in l/min

    // UBATotalUptimeMessage
    uint32_t UBAuptime; // Total UBA working hours

    // calculated values
    uint8_t tapwaterActive; // Hot tap water is on/off
    uint8_t heatingActive;  // Central heating is on/off

    // settings
    char    version[10];
    uint8_t type_id;
    uint8_t model_id;
} _EMS_Boiler;

// Definition for thermostat type
typedef struct {
    uint8_t model_id;
    bool    read_supported;
    bool    write_supported;
} _Thermostat_Type;

// Thermostat data
typedef struct {
    uint8_t type_id;  // the type ID of the thermostat
    uint8_t model_id; // which Thermostat type
    bool    read_supported;
    bool    write_supported;
    char    version[10];
    float   setpoint_roomTemp; // current set temp
    float   curr_roomTemp;     // current room temp
    uint8_t mode;              // 0=low, 1=manual, 2=auto
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t day;
    uint8_t month;
    uint8_t year;
} _EMS_Thermostat;

// call back function signature
typedef void (*EMS_processType_cb)(uint8_t * data, uint8_t length);

// Definition for each EMS type, including the relative callback function
typedef struct {
    uint8_t            model_id;
    uint8_t            type;
    const char         typeString[50];
    EMS_processType_cb processType_cb;
} _EMS_Type;

// function definitions
extern void ems_parseTelegram(uint8_t * telegram, uint8_t len);
void        ems_init(uint8_t boiler_modelid, uint8_t thermostat_modelid);
void        ems_doReadCommand(uint8_t type, uint8_t dest, bool forceRefresh = false);
void        ems_sendRawTelegram(char * telegram);

void ems_setThermostatTemp(float temp);
void ems_setThermostatMode(uint8_t mode);
void ems_setWarmWaterTemp(uint8_t temperature);
void ems_setWarmWaterActivated(bool activated);
void ems_setWarmTapWaterActivated(bool activated);
void ems_setPoll(bool b);
void ems_setTxEnabled(bool b);
void ems_setLogging(_EMS_SYS_LOGGING loglevel);
void ems_setEmsRefreshed(bool b);
void ems_setWarmWaterModeComfort(bool comfort);
bool ems_checkEMSBUSAlive();
void ems_setModels();

void             ems_getThermostatValues();
void             ems_getBoilerValues();
bool             ems_getPoll();
bool             ems_getTxEnabled();
bool             ems_getThermostatEnabled();
bool             ems_getBoilerEnabled();
bool             ems_getBusConnected();
_EMS_SYS_LOGGING ems_getLogging();
uint8_t          ems_getEmsTypesCount();
bool             ems_getEmsRefreshed();
uint8_t          ems_getThermostatModel();
uint8_t          ems_getBoilerModel();

void   ems_scanDevices();
void   ems_printAllTypes();
char * ems_getThermostatType(char * buffer);
void   ems_printTxQueue();
char * ems_getBoilerType(char * buffer);

// private functions
uint8_t _crcCalculator(uint8_t * data, uint8_t len);
void    _processType(uint8_t * telegram, uint8_t length);
void    _debugPrintPackage(const char * prefix, uint8_t * data, uint8_t len, const char * color);
void    _ems_clearTxData();
int     _ems_findModel(uint8_t model_id);
char *  _ems_buildModelString(char * buffer, uint8_t size, uint8_t model_id);
bool    _ems_setModel(uint8_t model_id);

// global so can referenced in other classes
extern _EMS_Sys_Status EMS_Sys_Status;
extern _EMS_Boiler     EMS_Boiler;
extern _EMS_Thermostat EMS_Thermostat;
