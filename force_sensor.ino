
const byte FORCE_SENSOR_PIN = 2; // all pins on BLE
const byte DRILL_PIN = 3; // 1-13, A0-A7

const double RESISTANCE_CHANGE_THRESHOLD = 0.99;

short raw_force_voltage;
double unscaled_resistance;

const int BUFFER_SIZE = 20; // *50 = 100ms

double average_resistance;

void setup() {
  Serial.begin(9600);
  
  pinMode(DRILL_PIN, OUTPUT);
  analogWrite(DRILL_PIN, 255); // don't think the drill can be powered like this
  
  pinMode(FORCE_SENSOR_PIN, INPUT);
  
  raw_force_voltage = analogRead(FORCE_SENSOR_PIN);
  average_resistance = static_cast<double>(raw_force_voltage)/(1024-raw_force_voltage);
  
  
}

void loop() {
  //short last = raw_force_voltage;
  raw_force_voltage = analogRead(FORCE_SENSOR_PIN);
  
  // Resistance = REF/(3.3/V-1) = x*REF/(1024-x)
  double last = unscaled_resistance;
  unscaled_resistance = static_cast<double>(raw_force_voltage)/(1024-raw_force_voltage); // not multiplied by reference
  
  
  if (unscaled_resistance / last < RESISTANCE_CHANGE_THRESHOLD) { // stop on increase in resistance: decrease resistance = increase in compression
    analogWrite(DRILL_PIN, 0);
  }
  // Actually, you can just use the raw force voltage directly, assuming the variance in voltage is relatively small. Also assume reference resistance is approximately equal to measuring resistance.
  // not going to do that right now though.
  
  Serial.print(raw_force_voltage); // for data collection stage only, may affect timing
  Serial.print(' ');
  Serial.println(unscaled_resistance);
  delay(50); // 1/20 of a second. this is arbitrary
  
}
