// --- ESP32-C6 HARDWARE PINS ---
const int pinX = 6;        // VRx connected to GPIO 6
const int pinY = 5;        // VRy connected to GPIO 5
const int buzzerPin = 12;  // Active Buzzer Positive connected to GPIO 12
 
// --- JOYSTICK CALIBRATION ---
// Based on your diagnostic screenshot, resting is ~1682
const int centerVal = 1680; 
const int threshold = 600;  // Must push past 1080 or 2280 to move
char lastDir = 'S';         
 
void setup() {
  Serial.begin(115200);       
  pinMode(buzzerPin, OUTPUT); 
  digitalWrite(buzzerPin, LOW); 
}
 
void loop() {
  // ==========================================
  // 1. READ JOYSTICK & SEND TO MZ_APO
  // ==========================================
  int xVal = analogRead(pinX); 
  int yVal = analogRead(pinY); 
  char currDir = 'S'; 
 
  // Check which way the stick is pushed
  if (yVal < (centerVal - threshold)) currDir = 'U';
  else if (yVal > (centerVal + threshold)) currDir = 'D';
  else if (xVal < (centerVal - threshold)) currDir = 'L';
  else if (xVal > (centerVal + threshold)) currDir = 'R';
 
  // Only send data over the USB if the direction changed
  if (currDir != lastDir) {
    if (currDir != 'S') Serial.print(currDir); 
    lastDir = currDir;
  }
 
  // ==========================================
  // 2. LISTEN FOR SOUND TRIGGERS FROM MZ_APO
  // ==========================================
  if (Serial.available() > 0) { 
    char cmd = Serial.read();   
 
    if (cmd == 'A') { 
      // APPLE EATEN: Short, crisp chirp
      digitalWrite(buzzerPin, HIGH);
      delay(30);
      digitalWrite(buzzerPin, LOW);
    } 
    else if (cmd == 'W') { 
      // WIN: Three rapid victory beeps
      for(int i=0; i<3; i++) {
        digitalWrite(buzzerPin, HIGH); delay(75);
        digitalWrite(buzzerPin, LOW);  delay(75);
      }
    } 
    else if (cmd == 'L') { 
      // LOSE: One long, sad beep
      digitalWrite(buzzerPin, HIGH);
      delay(600);
      digitalWrite(buzzerPin, LOW);
    }
  }
 
  delay(20); // Keep the polling rate stable
}