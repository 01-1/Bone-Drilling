const byte FORCE_SENSOR_PIN = 0;

unsigned short analog_force;
double force_voltage;

void setup() {
  Serial.begin(9600);
  pinMode(FORCE_SENSOR_PIN, INPUT);
}

void loop() {  
  analog_force = analogRead(FORCE_SENSOR_PIN);
  force_voltage = analog_force /1024.0 * 2;
  Serial.print(analog_force);
  Serial.print(' ');
  Serial.println(force_voltage);
  
}
