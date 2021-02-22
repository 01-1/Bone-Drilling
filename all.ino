#include "Adafruit_MLX90614.h"
#include <cstdint>

using byte8 = std::int8_t;
using int32 = std::int32_t;

enum class stage {
  before_drilling,
  first_cortical,
  trabecular,
  second_cortical
};

stage current_stage = stage::before_drilling;

// Pin constants
namespace Pin {
  const byte8 DRILL = 3;
  const byte8 LINEAR_ACTUATOR = 4; // check analog and digital pins
}

// Constants
namespace Threshold {
  const float START_VOLTAGE = 0.1;
  const double STOP_FACTOR = 0.5;
}

// Variables


Adafruit_MLX90614 mlx = Adafruit_MLX90614();

void setup() {
  Serial.begin(9600);
  mlx.begin();
  
  pinMode(Pin::DRILL, OUTPUT);
  pinMode(Pin::LINEAR_ACTUATOR, OUTPUT);
  analogWrite(Pin::DRILL, 255);
  analogWrite(Pin::LINEAR_ACTUATOR, 255);
  
}

void stopDrill() {
  analogWrite(Pin::DRILL, 0);
  analogWrite(Pin::LINEAR_ACTUATOR, 0);
}

void loop() {
  float force_voltage = 0; // modify
  switch (current_stage) {
    case stage::before_drilling:
      if (force_voltage > starting_threshold) {
        stage = stage::first_cortical;
      }
      break;
    case stage::first_cortical:
      // setup average
      break;
    case stage::trabecular:
      break;
    case stage::second_cortical:
      break;
  }
  
  tempC = mlx.readObjectTempC();
  
  if (tempC > 40) {
    stopDrill();
  }
  
}
