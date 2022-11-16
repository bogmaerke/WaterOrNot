/*
 * Project: Semesterprojekt
 * Description:
 * Author: Mikkel Pavia
 * Date:
 *
 * Notifications: https://www.hackster.io/gusgonnet/add-push-notifications-to-your-hardware-41fa5e
 */
#include "..\lib\RTClibraryDS3231_DL\src\RTClibraryDS3231_DL.h"
#include "WiFiCredentials.h"
//#define DBG

void interruptHandler();

#define LED D7
#define SENSOR A3
#define INT_INPUT D5
#define GET_WEATHER_DATA "hook-response/GET_WEATHER_DATA"
#define VBAT_ADC A2
#define VBAT_CTRL D2
#define SENSOR_DRY 3400
#define SENSOR_WET 1400
#define LOW_MOISTURE 50
#define SIGNIFICANT_PERCIPITATION 2.0
#define EXTENDED_INFO 1
#define DS3231 // Uncomment or delete to use ULP

RTC_DS3231 rtc;
LEDStatus statusOff;
LEDStatus blinkRGB(RGB_COLOR_RED, LED_PATTERN_BLINK, LED_SPEED_NORMAL, LED_PRIORITY_CRITICAL);
SystemSleepConfiguration config;

Timer timeoutTimer(30000, forceSleep, true);
SYSTEM_MODE(AUTOMATIC); // Call setup() before cloud connection is up
SYSTEM_THREAD(ENABLED);

unsigned int timeStart = 0;
unsigned int alarmFired = 0;
retained float futurePercipitation;
retained int soilMoisture;
bool state = false;
volatile bool gotWeatherData = false;
String requestString = "\r";

pin_t PINS[] = {D3, D6, D9};

void setup()
{
    // Set WiFi credentials to office WiFi
    WiFi.setCredentials(WiFiSSID, WiFiPW);
#ifdef DBG
    timeStart = millis();
#endif
    timeoutTimer.start();
    gotWeatherData = false;
    statusOff.setPriority(LED_PRIORITY_CRITICAL);
    statusOff.setActive();
    statusOff.off();
    // Set unused pins as output
    for (unsigned int i = 0; i < sizeof(PINS) / sizeof(PINS[0]); i++)
    {
        pinMode(PINS[i], OUTPUT);
        digitalWrite(PINS[i], LOW);
    }
    // Configure interrupt pin
    pinMode(INT_INPUT, INPUT);

    // Control pin for VBAT
    pinMode(VBAT_CTRL, OUTPUT);

    // Setup sensors
    pinMode(SENSOR, INPUT);
    pinMode(VBAT_ADC, INPUT);

#ifdef DBG
    blinkRGB.setActive();
#endif
#ifdef DS3231
    while (!rtc.begin())
        ;
    while (rtc.lostPower())
    {

        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        rtc.disable32K();
        rtc.clearAlarm(1);
        rtc.clearAlarm(2);
        rtc.writeSqwPinMode(DS3231_OFF);
        rtc.disableAlarm(1);
        rtc.disableAlarm(2);
        DateTime dt = DateTime((F(__DATE__), "16:00:00"));
        // DS3231_A1_Hour to turn on once a day.
        while (!rtc.setAlarm1(dt, DS3231_A1_Hour)) // Debug: DS3231_A1_Second
            ;
        dt = DateTime((F(__DATE__), "08:00:00"));
        while (!rtc.setAlarm2(dt, DS3231_A2_Hour))
            ;
    }
#ifdef DBG
    blinkRGB.setActive(false);
#endif
    if (rtc.alarmFired(1))
    {
        alarmFired = 1;
        rtc.clearAlarm(1);
    }
    if (rtc.alarmFired(2))
    {
        alarmFired = 2;
        rtc.clearAlarm(2);
    }
    config.mode(SystemSleepMode::HIBERNATE);
#else
    config.mode(SystemSleepMode::ULTRA_LOW_POWER);
    config.duration(480min);
#endif
    config.gpio(INT_INPUT, FALLING);
    soilMoisture = analogRead(SENSOR);
    soilMoisture = map(soilMoisture, SENSOR_DRY, SENSOR_WET, 0, 100);
    // Wait for cloud connection
    while (!Particle.connected())
        ;

    // Subscribe to weather data hook
    bool subRes;
    subRes = Particle.subscribe(GET_WEATHER_DATA, myHandler, MY_DEVICES);
    if (!subRes)
    {
    }
    bool res;
    res = Particle.publish("GET_WEATHER_DATA", requestString, PRIVATE);
    if (!res)
    {
    }
}

void loop()
{
    if (gotWeatherData)
    {
#ifdef DBG
        unsigned int totalTime = millis() - timeStart;
        char str[80];
        sprintf(str, "P%.1f, SM%d, UP%u", futurePercipitation, soilMoisture, totalTime);
        Particle.publish("DATA", str, PRIVATE);
#endif
        // Notify user to water plants
        if (soilMoisture <= LOW_MOISTURE && futurePercipitation <= SIGNIFICANT_PERCIPITATION)
        {
            if (EXTENDED_INFO)
            {
                Particle.publish("pushbullet", "Low chance of percipitation, water plants!\nR" + String(futurePercipitation, 1) + " M" + String(soilMoisture) + " A" + String(alarmFired), 60, PRIVATE);
            }
            else
            {
                Particle.publish("pushbullet", "Low chance of percipitation, water plants!", 60, PRIVATE);
            }
        }
        System.sleep(config);
    }
}

void myHandler(const char *event, const char *data)
{
    // Get weather data
    // https://api.met.no/weatherapi/locationforecast/2.0/compact?lat=x&lon=y
    char d[100];
    strcpy(d, data);
    futurePercipitation = atof(data);
    gotWeatherData = true;
}

void forceSleep()
{
    Particle.publish("FORCE_SLEEP", "\r", PRIVATE);
    delay(200);
    Particle.disconnect();
    WiFi.off();
    System.sleep(config);
}