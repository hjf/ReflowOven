#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>
#include <Adafruit_MAX31856.h>

#include <ArduinoOTA.h>
#include "mytypes.h"
#include "WebServer.h"
#include "config.h"
#include <PID_v1.h>
double Kp = 25, Ki = 15, Kd = 8;
bool tuningsChanged = false;


#define WITH_SERIAL
#ifdef HARDWARE_SPI
Adafruit_MAX31856 tc1 = Adafruit_MAX31856(TC1_CS);
#else
Adafruit_MAX31856 tc1 = Adafruit_MAX31856(TC1_CS, MOSI, MISO, MCLK); //CS, DI, DO, CLK
#endif
thermocouple_reading_t tc_reading;
oven_status_t oven_status = BAKING;
uint32_t triac_millis = 0;
uint32_t triac_millis_setpoint = 0;
uint32_t triac_value = 100;
uint32_t pid_millis = 0;
double Setpoint, Input, Output;
float starting_temp;
oven_status_t last_oven_status;
profile_t current_profile;
uint32_t state_millis;
int32_t remaining_seconds;
bool temp_reached;

PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

void setup() {


  WiFiManager wifiManager;
  wifiManager.autoConnect("REFLOW OVEN");
  tc1.begin();
  tc1.setThermocoupleType(MAX31856_TCTYPE_K);
  tc1.setNoiseFilter(MAX31856_NOISE_FILTER_50HZ);
  tc_reading.cold_junction_temperature = 0;
  tc_reading.thermocouple_temperature = 0;
  tc_reading.cold_junction_temperature = 0xff;
  webserver_init();

#ifdef WITH_SERIAL
  Serial.begin(115200);
#else
  pinMode(1, FUNCTION_3);  //GPIO 1 (TX) swap the pin to a GPIO.
  pinMode(3, FUNCTION_3); //GPIO 3 (RX) swap the pin to a GPIO.
#endif


  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  pinMode(TRIAC_ENABLE, OUTPUT);
  digitalWrite(TRIAC_ENABLE, LOW);
  tc1.oneShotTemperature(false);
  myPID.SetOutputLimits(0, TRIAC_PERIOD);
  myPID.SetMode(AUTOMATIC);

  current_profile.soak_target = 150;
  current_profile.soak_time = 90;
  current_profile.reflow_target = 230;
  current_profile.reflow_time = 35;
  current_profile.cooling_target = 60;

  oven_status = IDLE;
  tc_reading.fault = 1;
  remaining_seconds = -1;
}

void loop() {

  if (oven_status == IDLE) {
    digitalWrite(TRIAC_ENABLE, LOW);
    triac_value = 0;
    Output = 0;
  }
  //TC
  if (!tc_reading.fault) {
    if (last_oven_status != oven_status) {
      last_oven_status = oven_status;
      state_millis = millis();
      remaining_seconds = -1;
      starting_temp = tc_reading.thermocouple_temperature;
      temp_reached = false;
    }
    uint32_t state_secs = (millis() - state_millis) / 1000;


    if (oven_status == SOAK) {
      Setpoint = current_profile.soak_target;

      if (tc_reading.thermocouple_temperature  >= current_profile.soak_target)
        if (temp_reached == false) {
          temp_reached = true;
          state_millis = millis();
          state_secs = (millis() - state_millis) / 1000;
        }

      if (temp_reached)
        remaining_seconds = current_profile.soak_time - state_secs;

      if ( temp_reached && state_secs > current_profile.soak_time)
        oven_status = REFLOW;
    }

    else if (oven_status == REFLOW) {
      Setpoint = current_profile.reflow_target;
      if (tc_reading.thermocouple_temperature  >= current_profile.reflow_target)
        if (temp_reached == false) {
          temp_reached = true;
          state_millis = millis();
          state_secs = (millis() - state_millis) / 1000;
        }

      if (temp_reached)
        remaining_seconds = current_profile.reflow_time - state_secs;

      if ( temp_reached && state_secs > current_profile.reflow_time)
        oven_status = COOLING;
    }


    else if (oven_status == COOLING) {
      Setpoint = current_profile.cooling_target;
      if (tc_reading.thermocouple_temperature < current_profile.cooling_target)
        oven_status = IDLE;
    }
  }

  if (tuningsChanged) {
    myPID.SetTunings(Kp, Ki, Kd);
    tuningsChanged = false;
  }
  if (tc1.isConversionDone()) {
    tc_reading.cold_junction_temperature = tc1.readCJTemperature(false);
    tc_reading.thermocouple_temperature = tc1.readThermocoupleTemperature(false);
    tc_reading.fault = tc1.readFault();
    tc1.oneShotTemperature(false);
    if (oven_status != IDLE && millis() - pid_millis > PID_sampleTime ) {
      pid_millis = millis();
      Input = tc_reading.thermocouple_temperature;
      myPID.Compute();
      triac_value = Output;
    }
  }
  yield();
  webserver_handle();
  yield();
  ArduinoOTA.handle();
  yield();

  uint32_t ttime = millis() - triac_millis;

  if (ttime > TRIAC_PERIOD) {
    triac_millis = millis();
    if (oven_status && triac_value > 5)
      digitalWrite(TRIAC_ENABLE, HIGH);
  }
  else if (ttime > triac_value) {
    digitalWrite(TRIAC_ENABLE, LOW);
  }


#ifdef WITH_SERIAL
  //  Serial.print("CJ: ");
  //  Serial.println(tc_reading.cold_junction_temperature);
  //  Serial.print("TC: ");
  //  Serial.println(tc_reading.thermocouple_temperature);
  //  Serial.print("FAULT: ");
  //  Serial.println(tc_reading.cold_junction_temperature);
#endif
  yield();

}
