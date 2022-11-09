/*
 * Project: Semesterprojekt
 * Description:
 * Author: Mikkel Pavia
 * Date:
 */
#include "..\lib\RTClibraryDS3231_DL\src\RTClibraryDS3231_DL.h"
#include "WiFiCredentials.h"
#define DBG

void interruptHandler();

#define LED D7
#define SENSOR A3
#define INT_INPUT D5
#define GET_WEATHER_DATA "hook-response/GET_WEATHER_DATA"
#define VBAT_ADC A2
#define VBAT_CTRL D2
#define SENSOR_DRY 3400
#define SENSOR_WET 1400

RTC_DS3231 rtc;
LEDStatus status;
SystemSleepConfiguration config;

// Serial1LogHandler logHandler(9600, LOG_LEVEL_NONE);
Timer timeoutTimer(30000, forceSleep, true);
SYSTEM_MODE(AUTOMATIC); // Call setup() and loop() before cloud connection is up
SYSTEM_THREAD(ENABLED);

unsigned int timeStart = 0;
retained float lastPercipitation;
retained int soilMoisture;
bool state = false;
volatile bool gotWeatherData = false;
String requestString = "\r";

pin_t PINS[] = {D3, D6, D9};

void setup()
{
    // Set WiFi credentials to office WiFi
    WiFi.setCredentials(WiFiSSID, WiFiPW);
    // Redacted
    timeStart = millis();
    timeoutTimer.start();
    gotWeatherData = false;
    status.setPriority(LED_PRIORITY_CRITICAL);
    status.setActive();
    status.off();
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

    while (!rtc.begin())
        ;

    if (rtc.lostPower())
    {
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        rtc.disable32K();
        rtc.clearAlarm(1);
        rtc.clearAlarm(2);
        rtc.writeSqwPinMode(DS3231_OFF);
        rtc.disableAlarm(2);

        DateTime dt = DateTime((F(__DATE__), "16:00:00"));
        // DS3231_A1_Hour to turn on once a day.
        while (!rtc.setAlarm1(dt, DS3231_A1_Second))
            ;
        dt = DateTime((F(__DATE__), "08:00:00"));
        while (!rtc.setAlarm2(dt, DS3231_A2_Hour))
            ;
    }
    config.mode(SystemSleepMode::HIBERNATE);
    config.gpio(INT_INPUT, FALLING);
    if (rtc.alarmFired(1) || rtc.alarmFired(2))
    {
        rtc.clearAlarm(1);
        rtc.clearAlarm(2);
    }
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
        unsigned int totalTime = millis() - timeStart;
        char str[80];
        sprintf(str, "P%.1f, SM%d, UP%u", lastPercipitation, soilMoisture, totalTime);
        Particle.publish("DATA", str, PRIVATE);
        delay(200);
        // Particle.disconnect();
        // WiFi.off();
        // NFC.off();
        // Wire.end();
        // SPI.end();
        System.sleep(config);
    }
}

void myHandler(const char *event, const char *data)
{
    // Get weather data
    // https://api.met.no/weatherapi/locationforecast/2.0/compact?lat=x&lon=y
    char d[100];
    strcpy(d, data);
    lastPercipitation = atof(data);
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