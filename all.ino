#include "Adafruit_MLX90614.h"

const byte FORCE_SENSOR_PIN = 2;
const byte DRILL_PIN = 3;

const double RESISTANCE_CHANGE_THRESHOLD = 0.99;

short raw_force_voltage;
float unscaled_resistance;
float last;

double tempC;

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

void setup() {
  Serial.begin(9600);
  mlx.begin();
  
  pinMode(DRILL_PIN, OUTPUT);
  analogWrite(DRILL_PIN, 255);
  
  pinMode(FORCE_SENSOR_PIN, INPUT);
  
}

void stopDrill() {
  analogWrite(DRILL_PIN, 0);
  
}

void loop() {
  //short last = raw_force_voltage;
  raw_force_voltage = analogRead(FORCE_SENSOR_PIN);
  
  // Resistance = REF/(3.3/V-1) = x*REF/(1024-x)
  last = unscaled_resistance;
  unscaled_resistance = static_cast<float>(raw_force_voltage)/(1024-raw_force_voltage); // not multiplied by reference
  
  if (unscaled_resistance / last < RESISTANCE_CHANGE_THRESHOLD) { // stop on increase in resistance: decrease resistance = increase in compression
    stopDrill();
  }
  // Actually, you can just use the raw force voltage directly, assuming the variance in voltage is relatively small. Also assume reference resistance is approximately equal to measuring resistance.
  // not going to do that right now though.
  
  Serial.println(unscaled_resistance); // for data collection stage only, may affect timing
  
  tempC = mlx.readObjectTempC();
  
  if (tempC > 40) {
    stopDrill();
  }
  
  Serial.println(tempC);
  
  delay(50); // 1/20 of a second. this is arbitrary
  
}
