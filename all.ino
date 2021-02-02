#include "Adafruit_MLX90614.h"

const byte FORCE_SENSOR_PIN = 2;
const byte DRILL_PIN = 3;
const byte LINEAR_ACTUATOR_PIN = 4; // check analog and digital pins

const double RESISTANCE_CHANGE_THRESHOLD = 0.99;

short raw_force_voltage;


const double R1=500, R2, R3=500, R4=500;
const double VD = R4/(R3+R4)*3.3;
const double force_voltage;
const double lastR2;

double tempC;

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

void setup() {
  Serial.begin(9600);
  mlx.begin();
  
  pinMode(DRILL_PIN, OUTPUT);
  pinMode(LINEAR_ACTUATOR_PIN, OUTPUT);
  analogWrite(DRILL_PIN, 255);
  analogWrite(LINEAR_ACTUATOR_PIN, 255);
  
  pinMode(FORCE_SENSOR_PIN, INPUT);
  
}

void stopDrill() {
  analogWrite(DRILL_PIN, 0);
  analogWrite(LINEAR_ACTUATOR_PIN, 0);
}

void loop() {
  //short last = raw_force_voltage;
  //raw_force_voltage = analogRead(FORCE_SENSOR_PIN);
  
  force_voltage = 3.3/3.3;
  
  R2 = (R2*(VD+force_voltage))/(3.3-(VD+force_voltage));
  
  if (R2 / lastR2 < RESISTANCE_CHANGE_THRESHOLD) { // stop on increase in resistance: decrease resistance = increase in compression
    stopDrill();
  }
  
  tempC = mlx.readObjectTempC();
  
  if (tempC > 40) {
    stopDrill();
  }
  
  Serial.println(tempC);
  
  delay(50); // 1/20 of a second. this is arbitrary
  
}
