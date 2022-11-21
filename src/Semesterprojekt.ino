/*
 * Project: Semesterprojekt
 * Description:
 * Author: Mikkel Pavia
 * Date:
 *
 * Notifications: https://www.hackster.io/gusgonnet/add-push-notifications-to-your-hardware-41fa5e
 */
#include "WiFiCredentials.h"
#define DBG

#define EXTENDED_INFO 1
#define GET_WEATHER_DATA "hook-response/GET_WEATHER_DATA"

// Thresholds
#define SENSOR_DRY 3400
#define SENSOR_WET 1400
#define SIGNIFICANT_PERCIPITATION 2.0
#define LOW_MOISTURE 65
#define VBAT_LOW 3.5

// Pins
#define SENSOR A3
#define VBAT_ADC A2
#define VBAT_CTRL D2

// Constants
#define MAX_RUNTIME 45000
#define ADC_RES (3.3 / 4096);
#define VOLTAGE_DIVIDER_SCALE_FACTOR 5.45

LEDStatus statusOff;
SystemSleepConfiguration config;

Timer timeoutTimer(MAX_RUNTIME, forceSleep, false);
SYSTEM_MODE(AUTOMATIC); // Call setup() before cloud connection is up
SYSTEM_THREAD(ENABLED);
#ifdef DBG
unsigned int totalTime = 0;
char str[80];
SerialLogHandler logHandler(115200);
const char *STATE_NAMES[8] = {
    "COLD_START",
    "READ_SENSORS",
    "RETURN_FROM_SLEEP",
    "CONNECTED",
    "AWAIT_RESPONSE",
    "HANDLE_DATA",
    "FORCE_SLEEP",
    "SLEEP"};
#endif
unsigned int timeStart = 0;
unsigned int alarmFired = 0;
float futurePercipitation;
double VBAT_ACTUAL;
int soilMoisture;
int VBAT;
volatile bool gotWeatherData = false;
typedef enum STATES
{
    COLD_START,
    READ_SENSORS,
    RETURN_FROM_SLEEP,
    CONNECTED,
    AWAIT_RESPONSE,
    HANDLE_DATA,
    FORCE_SLEEP,
    SLEEP

} state_t;
volatile state_t currentState = COLD_START, lastState;
pin_t PINS[] = {D3, D4, D5, D6, D7, D8, D9};

void setup()
{
}

void loop()
{
#ifdef DBG
    if (currentState != lastState)
    {
        Log.info("\n\t\tSTATE: " + String(STATE_NAMES[currentState]) + "\n");
        if (Particle.connected())
            Particle.publish("STATE", STATE_NAMES[currentState], PRIVATE);
    }
#endif
    switch (currentState)
    {
    case COLD_START:
        // Set WiFi credentials to office WiFi
        WiFi.setCredentials(WiFiSSID, WiFiPW);
#ifdef DBG
        timeStart = millis();
#endif
        timeoutTimer.reset();
        statusOff.setPriority(LED_PRIORITY_CRITICAL);
        statusOff.setActive();
        statusOff.off();
        // Set unused pins as output
        for (unsigned int i = 0; i < sizeof(PINS) / sizeof(PINS[0]); i++)
        {
            pinMode(PINS[i], OUTPUT);
            digitalWrite(PINS[i], LOW);
        }

        // Control pin for VBAT
        pinMode(VBAT_CTRL, OUTPUT);

        // Setup sensors
        pinMode(SENSOR, INPUT);
        pinMode(VBAT_ADC, INPUT);

        config.mode(SystemSleepMode::ULTRA_LOW_POWER);
        config.duration(1min); // 480 min for 8 hours
        currentState = READ_SENSORS;
        break;

    case READ_SENSORS:
        // Sensor read
        soilMoisture = analogRead(SENSOR);
        soilMoisture = map(soilMoisture, SENSOR_DRY, SENSOR_WET, 0, 100);
        // Read VBAT
        digitalWrite(VBAT_CTRL, HIGH);
        VBAT = analogRead(VBAT_ADC);
        digitalWrite(VBAT_CTRL, LOW);

        // Calculate voltage
        VBAT_ACTUAL = VBAT * ADC_RES;
        VBAT_ACTUAL *= VOLTAGE_DIVIDER_SCALE_FACTOR;
        // If soil moisture is high, go to sleep immediately, don't even wait for cloud connection
        if (soilMoisture >= LOW_MOISTURE && VBAT_ACTUAL > VBAT_LOW)
            currentState = SLEEP;
        else
            currentState = CONNECTED;
        break;

    case CONNECTED:
        // Wait for cloud connection
        while (!Particle.connected())
            ;

        // Subscribe to weather data hook
        bool subRes;
        subRes = Particle.subscribe(GET_WEATHER_DATA, subHandler, MY_DEVICES);
        if (!subRes)
        {
        }
        bool res;
        res = Particle.publish("GET_WEATHER_DATA", PRIVATE);
        if (!res)
        {
        }
        currentState = AWAIT_RESPONSE;
        break;

    case AWAIT_RESPONSE:
        // IDLE?
        break;

    case HANDLE_DATA:
#ifdef DBG
        totalTime = millis() - timeStart;
        sprintf(str, "%s P%.1f, SM%d, UP%u B%.2lf", Time.timeStr().c_str(), futurePercipitation, soilMoisture, totalTime, VBAT_ACTUAL);
        Particle.publish("pushbullet", str, PRIVATE);
#else
        if (soilMoisture <= LOW_MOISTURE && futurePercipitation <= SIGNIFICANT_PERCIPITATION)
        {
            if (EXTENDED_INFO)
            {

                Particle.publish("pushbullet", "Low chance of percipitation, water plants!\nR" + String(futurePercipitation, 1) + " M" + String(soilMoisture) + " B" + String(VBAT_ACTUAL, 2), PRIVATE);
            }
            else
            {
                Particle.publish("pushbullet", "Low chance of percipitation, water plants!", PRIVATE);
            }
        }
#endif
        currentState = SLEEP;
        break;

    case SLEEP:
        currentState = RETURN_FROM_SLEEP;
        timeoutTimer.stop();
        System.sleep(config);
        break;

    case FORCE_SLEEP:
        Particle.publish("FORCE_SLEEP", PRIVATE);
        currentState = SLEEP;
        break;

    case RETURN_FROM_SLEEP:
        timeStart = millis();
        timeoutTimer.reset();
        currentState = READ_SENSORS;
        break;

    default:
        break;
    }
    lastState = currentState;
}

void subHandler(const char *event, const char *data)
{
    // Get weather data
    // https://api.met.no/weatherapi/locationforecast/2.0/compact?lat=x&lon=y
    char d[100];
    strcpy(d, data);
    futurePercipitation = atof(data);
    currentState = HANDLE_DATA;
}

void forceSleep()
{
    currentState = FORCE_SLEEP;
}