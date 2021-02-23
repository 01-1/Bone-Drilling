#include "Adafruit_MLX90614.h"
#include <cstdint>

enum class stage {
    before_drilling,
    first_cortical,
    trabecular,
    second_cortical
};

stage current_stage = stage::before_drilling;

// Pin constants
namespace Pin {
    const uint8_t DRILL = 3;
    const uint8_t LINEAR_ACTUATOR = 4; // check analog and digital pins
}

// Constants
namespace Threshold {
    const float START_VOLTAGE = 0.1f;
    const double CT_FACTOR = 0.5;
    const double TC_FACTOR = 2.0;
}

// Variables
double cortical_total = 0;
uint32_t cortical_len = 0; // better safe than sorry

double trabecular_total = 0;
uint32_t trabecular_len = 0;

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

bool update(float force_voltage, double& total, uint32_t& len, double factor) {
    total += force_voltage;
    len++;
    return force_voltage < (total / len) * factor;
}

void loop() {
    float force_voltage = 0; // modify
    // library for HX711 uses float

    switch (current_stage) {
        case stage::before_drilling:
            if (force_voltage > Threshold::START_VOLTAGE) {
                current_stage = stage::first_cortical;
            }
            break;
        case stage::first_cortical:
            if (update(force_voltage, cortical_total, cortical_len, Threshold::CT_FACTOR)) {
                current_stage = stage::trabecular;
            }
            break;
        case stage::trabecular:
            if (update(force_voltage, trabecular_total, trabecular_len, Threshold::TC_FACTOR)) {
                current_stage = stage::second_cortical;
            }
            break;
        case stage::second_cortical:
            if (update(force_voltage, cortical_total, cortical_len, Threshold::CT_FACTOR)) {
                stopDrill();
            }
            break;
    }

    double tempC = mlx.readObjectTempC();

    if (tempC > 40) {
        stopDrill();
    }

}
