#include <Adafruit_MLX90614.h>
#include <HX711.h>
#include <L298N.h>

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
Scenario scenario;

// Pin constants
namespace Pin {
    using p = const uint8_t;
    p LOADCELL_DOUT_PIN = 0;
    p LOADCELL_SCK_PIN = 1;
    p LINEAR_ACTUATOR_EN = 6;
    p LINEAR_ACTUATOR_IN1 = 7;
    p LINEAR_ACTUATOR_IN2 = 8;
}

// Constants

const float STARTING_VOLTAGE_THRESHOLD = 0.1f;
const double ENTER_TRABECULAR_THRESHOLD_FACTOR = 0.5;
const double ENTER_CORTICAL_THRESHOLD_FACTOR = 2.0;

// 10 mm/s full speed
const unsigned short INITIAL_SPEED = 96;
const unsigned short CORTICAL_SPEED = 96;
const unsigned short TRABECULAR_SPEED = 96;

// Variables
double cortical_total = 0;
uint32_t cortical_len = 0; // better safe than sorry

double trabecular_total = 0;
uint32_t trabecular_len = 0;

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
HX711 loadcell;
L298N linear_actuator(Pin::LINEAR_ACTUATOR_EN, Pin::LINEAR_ACTUATOR_IN1, Pin::LINEAR_ACTUATOR_IN2);

unsigned long timer_end; // 70 minutes overflow

void choose_scenario() {
    Serial.println("What scenario would you like to use?");
    Serial.println("Press 't' to stop at the trabecular bone and 'f' to stop at the end of the whole bone.");
    while (Serial.available() <= 0) {}
    switch(Serial.read()) {
        case 't':
            scenario = Scenario::STOP_TRABECULAR;
            break;
        case 'f':
            scenario = Scenario::FULL;
            break;
        default:
            Serial.println("Disallowed scenario.");
            scenario = Scenario::FULL;
            stage = Stage::IDLE;
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
    loadcell.tare();

    setSignedSpeed(INITIAL_SPEED);

}

void setSignedSpeed(int speed) {
  linear_actuator.setSpeed(abs(speed)); // need to set speed and then run
  linear_actuator.run(speed == 0 ? L298N::STOP : (speed > 0 ? L298N::FORWARD : L298N::BACKWARD));
}

void stopDrill() {
    linear_actuator.run(-255);
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
                timer_end = millis() + 2000;
            }
            break;
        case Stage::INITIAL_FIRST_CORTICAL:
            if (millis() > timer_end) {
                stage = Stage::MAIN_FIRST_CORTICAL;
                setSignedSpeed(CORTICAL_SPEED);
            }
            break;
        case Stage::MAIN_FIRST_CORTICAL:
            if (update(force_voltage, cortical_total, cortical_len, ENTER_TRABECULAR_THRESHOLD_FACTOR)) {
                if (scenario == Scenario::STOP_TRABECULAR) {
                    stopDrill();
                } else {
                    stage = Stage::TRABECULAR;
                    setSignedSpeed(TRABECULAR_SPEED);
                }
            }
            break;
        case Stage::TRABECULAR:
            if (update(force_voltage, trabecular_total, trabecular_len, ENTER_CORTICAL_THRESHOLD_FACTOR)) {
                stage = Stage::SECOND_CORTICAL;
                setSignedSpeed(CORTICAL_SPEED);
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
    
    Serial.print(tempC);
    Serial.print(" ");
    Serial.println(millis());
}
