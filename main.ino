// based: https://www.hackster.io/Elite_Worm/build-a-laser-tachometer-c5a320
#include <SPI.h>
#include <Wire.h>

const byte sensor = 2;

volatile unsigned long t_pulse_started_volatile = 0;
volatile unsigned long t_pulse_duration_volatile = 0;
unsigned long t_pulse_started = 0;
unsigned long t_pulse_duration = 0;

long rpm_sum = 0;
long rpm_reading[100];
long rpm_average = 0;
byte n_max = 0;
byte n = 0;

volatile bool timeout = 1;
volatile bool newpulse = 0;

void setup() {
  pinMode(sensor, INPUT);
  attachInterrupt(digitalPinToInterrupt(sensor), ISR_sensor, RISING);
}

void loop() {
  noInterrupts();
  t_pulse_started = t_pulse_started_volatile;
  t_pulse_duration = t_pulse_duration_volatile;
  interrupts();

  if (((micros() - t_pulse_started) > 2000000) && timeout == 0 && newpulse == 0) {
    timeout = 1;
    rpm_average = 0;
    n = 0;
  };

  if (timeout == 0) {
    if (newpulse) {
      
      rpm_reading[n] = (60000000 / t_pulse_duration);
      n_max = constrain(map(rpm_reading[n], 60, 100000, 0, 100), 0, 100);
      n++;
      newpulse = 0;

      if (n > n_max) {
        for (byte i = 0; i <= n_max; i++) {
          rpm_sum = rpm_sum + rpm_reading[i];
        };
        rpm_average = rpm_sum / (n_max + 1);
        rpm_sum = 0;
        n = 0;
      }
      
    }
  }
}

void ISR_sensor() {

  t_pulse_duration_volatile = micros() - t_pulse_started_volatile;
  t_pulse_started_volatile = micros();
  timeout = 0;
  newpulse = 1;

}
