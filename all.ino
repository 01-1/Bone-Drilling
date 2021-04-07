#include <Adafruit_MLX90614.h>
#include <HX711.h>
#include <L298N.h>

enum class Stage {
  BEFORE_DRILLING,
  FIRST_CORTICAL,
  MAIN_FIRST_CORTICAL,
  TRABECULAR,
  SECOND_CORTICAL,
  AFTER_DRILLING
};

enum class Scenario {
  STOP_TRABECULAR,
  FULL
};

Stage stage = Stage::BEFORE_DRILLING;
Scenario scenario;

// Pin constants
namespace Pin {
  using p = const uint8_t;
  p LOADCELL_DOUT_PIN = 3;
  p LOADCELL_SCK_PIN = 2;
  p LINEAR_ACTUATOR_EN = 6;
  p LINEAR_ACTUATOR_IN1 = 7;
  p LINEAR_ACTUATOR_IN2 = 8;
}

// Constants

const float VOLTAGE_THRESHOLD = 1500000.0f; 

const float ZERO_OFFSET = 380000.0f;

// 10 mm/s = full speed (255)
const unsigned short ACTUATOR_SPEED = 102; // 102 = 4mm/s

// Objects / Variables

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
HX711 loadcell;
L298N linear_actuator(Pin::LINEAR_ACTUATOR_EN, Pin::LINEAR_ACTUATOR_IN1, Pin::LINEAR_ACTUATOR_IN2);

void choose_scenario() {
  Serial.println("Choose a scenario:");
  Serial.println("Single cortical: '1' or 's'");
  Serial.println("Whole bone: '2' or 'f'");
  Serial.println("No movement: Anything else");
  while (Serial.available() == 0) {}
  char c = Serial.read();
  Serial.print("Input received: ");
  Serial.println(c);
  switch (c) {
    case '1':
    case 's':
      scenario = Scenario::STOP_TRABECULAR;
      break;
    case '2':
    case 'f':
      scenario = Scenario::FULL;
      break;
    default:
      Serial.println("Disallowed scenario.");
      scenario = Scenario::FULL;
      stage = Stage::AFTER_DRILLING;
      break;
  }
}

void setup() {
  Serial.begin(9600);

  choose_scenario();

  mlx.begin();

  pinMode(Pin::LINEAR_ACTUATOR_EN, OUTPUT);
  pinMode(Pin::LINEAR_ACTUATOR_IN1, OUTPUT);
  pinMode(Pin::LINEAR_ACTUATOR_IN2, OUTPUT);

  pinMode(Pin::LOADCELL_DOUT_PIN, INPUT);
  pinMode(Pin::LOADCELL_SCK_PIN, INPUT);

  loadcell.begin(Pin::LOADCELL_DOUT_PIN, Pin::LOADCELL_SCK_PIN);
  loadcell.set_scale();
  loadcell.set_gain(128);
  
  if (stage == Stage::AFTER_DRILLING) return; // no scenario inputted
  setSignedSpeed(ACTUATOR_SPEED);

}

void setSignedSpeed(int speed) {
  linear_actuator.setSpeed(abs(speed)); // need to set speed and then run
  linear_actuator.run(speed == 0 ? L298N::STOP : (speed > 0 ? L298N::FORWARD : L298N::BACKWARD));
}

void stopDrill() {
  setSignedSpeed(-255);
  stage = Stage::AFTER_DRILLING;
  Serial.print("\tStage: AFTER_DRILLING");
}

void loop() {
  float force_voltage = loadcell.get_units(1); // modify
  // library for HX711 uses float
  
  double tempC = mlx.readObjectTempC();
  
  Serial.print(millis());
  Serial.print('\t');
  Serial.print(tempC);
  Serial.print('\t');
  Serial.print((force_voltage - ZERO_OFFSET)/68171.4f);
  Serial.print('\t');
  Serial.print(force_voltage);
  
  if (tempC > 25) {
    stopDrill();
  }
  
  if (force_voltage >= 0) { // remove fluctuation/error values
    switch (stage) {
      case Stage::BEFORE_DRILLING:
        if (force_voltage >= VOLTAGE_THRESHOLD) {
          stage = Stage::FIRST_CORTICAL;
          Serial.print("\tStage: FIRST_CORTICAL");
        }
        break;
      case Stage::FIRST_CORTICAL:
        if (force_voltage < VOLTAGE_THRESHOLD) {
          if (scenario == Scenario::STOP_TRABECULAR) {
            stopDrill();
          } else {
            stage = Stage::TRABECULAR;
            Serial.print("\tStage: TRABECULAR");
          }
        }
        break;
      case Stage::TRABECULAR:
        if (force_voltage >= VOLTAGE_THRESHOLD) {
          stage = Stage::SECOND_CORTICAL;
          Serial.print("\tStage: SECOND_CORTICAL");
        }
        break;
      case Stage::SECOND_CORTICAL:
        if (force_voltage < VOLTAGE_THRESHOLD) {
          stopDrill();
        }
        break;
      case Stage::AFTER_DRILLING:
        break;
    }
  }
  if (Serial.available() > 0) {
    char input = Serial.read();
    Serial.print("\tInput received: ");
    Serial.print(input);
    switch (input) {
      case '0': // self-explanatory
        stage = Stage::AFTER_DRILLING;
        setSignedSpeed(0);
        break;
      case 'q': // quit
        stopDrill();
        break;
      case 'f': // forward
        setSignedSpeed(255);
        break;
      case '4': // 4 mm/s
        setSignedSpeed(102);
      default:
        Serial.print(" - Unknown input.");
        break;
    }
  }
  Serial.println();
}
