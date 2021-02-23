#ifdef DEBUG
#include "Arduino.h"
#include "Adafruit_MLX90614.h"
#include "HX711.h"
#include "L298N.h"
#else
#include <Adafruit_MLX90614.h>
#include <HX711.h>
#include <L298N.h>
#endif

enum class Stage {
    BEFORE_DRILLING,
    INITIAL_FIRST_CORTICAL,
    MAIN_FIRST_CORTICAL,
    TRABECULAR,
    SECOND_CORTICAL,
    IDLE
};

enum class Scenario {
    STOP_TRABECULAR,
    FULL
};

Stage stage = Stage::BEFORE_DRILLING;
Scenario scenario = Scenario::FULL;

// Pin constants
namespace Pin {
    using p = const uint8_t;
    p LOADCELL_DOUT_PIN = 1;
    p LOADCELL_SCK_PIN = 2;
    p LINEAR_ACTUATOR_EN = 3;
    p LINEAR_ACTUATOR_IN1 = 4;
    p LINEAR_ACTUATOR_IN2 = 5;
}

// Constants

const float STARTING_VOLTAGE_THRESHOLD = 0.1f;
const double ENTER_TRABECULAR_THRESHOLD_FACTOR = 0.5;
const double ENTER_CORTICAL_THRESHOLD_FACTOR = 2.0;

// 10 mm/s full speed
const unsigned short INITIAL_SPEED = 13; // 0.5 mm/s
const unsigned short CORTICAL_SPEED = 26; // 1 mm/s
const unsigned short TRABECULAR_SPEED = 69; // 0.5 mm/s

// Variables
double cortical_total = 0;
uint32_t cortical_len = 0; // better safe than sorry

double trabecular_total = 0;
uint32_t trabecular_len = 0;

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
HX711 loadcell;
L298N linear_actuator(Pin::LINEAR_ACTUATOR_EN, Pin::LINEAR_ACTUATOR_IN1, Pin::LINEAR_ACTUATOR_IN2);

unsigned long timer_end; // 70 minutes overflow

void setup() {
    Serial.begin(9600);
    mlx.begin();

    // TODO: Add serial scenario input

    pinMode(Pin::LINEAR_ACTUATOR_EN, OUTPUT);
    pinMode(Pin::LINEAR_ACTUATOR_IN1, OUTPUT);
    pinMode(Pin::LINEAR_ACTUATOR_IN2, OUTPUT);

    pinMode(Pin::LOADCELL_DOUT_PIN, INPUT);
    pinMode(Pin::LOADCELL_SCK_PIN, INPUT);

    loadcell.begin(Pin::LOADCELL_DOUT_PIN, Pin::LOADCELL_SCK_PIN);
    loadcell.set_scale();
    loadcell.tare();
    
    linear_actuator.setSpeed(INITIAL_SPEED);
}

void stopDrill() {
    // linear_actuator.stop();
    linear_actuator.backward();
    stage = Stage::IDLE;
}

bool update(float force_voltage, double& total, uint32_t& len, double factor) {
    total += force_voltage;
    len++;
    return force_voltage < (total / len) * factor;
}

void loop() {
    float force_voltage = loadcell.get_units(10); // modify
    // library for HX711 uses float

    switch (stage) {
        case Stage::BEFORE_DRILLING:
            if (force_voltage > STARTING_VOLTAGE_THRESHOLD) {
                stage = Stage::INITIAL_FIRST_CORTICAL;
                timer_end = micros() + 2000000;
            }
            break;
        case Stage::INITIAL_FIRST_CORTICAL:
            if (micros() > timer_end) {
                stage = Stage::MAIN_FIRST_CORTICAL;
                linear_actuator.setSpeed(CORTICAL_SPEED);
            }
            break;
        case Stage::MAIN_FIRST_CORTICAL:
            if (update(force_voltage, cortical_total, cortical_len, ENTER_TRABECULAR_THRESHOLD_FACTOR)) {
                if (scenario == Scenario::STOP_TRABECULAR) {
                    stopDrill();
                } else {
                    stage = Stage::TRABECULAR;
                    linear_actuator.setSpeed(TRABECULAR_SPEED);
                }
            }
            break;
        case Stage::TRABECULAR:
            if (update(force_voltage, trabecular_total, trabecular_len, ENTER_CORTICAL_THRESHOLD_FACTOR)) {
                stage = Stage::SECOND_CORTICAL;
                linear_actuator.setSpeed(CORTICAL_SPEED);
            }

            break;
        case Stage::SECOND_CORTICAL:
            if (update(force_voltage, cortical_total, cortical_len, ENTER_TRABECULAR_THRESHOLD_FACTOR)) {
                stopDrill();
            }
            break;
        case Stage::IDLE:
            break;
    }

    double tempC = mlx.readObjectTempC();

    if (tempC > 40) {
        stopDrill();
    }

}
