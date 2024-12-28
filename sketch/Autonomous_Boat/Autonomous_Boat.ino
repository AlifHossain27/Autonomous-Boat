#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <Servo.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>

// Pins for GPS
#define RXPin 4
#define TXPin 3

// Pins for Ultrasonic Sensor
#define TRIG_PIN 7
#define ECHO_PIN 8

// Pin for Servo Motor (Rudder)
#define RUDDER_PIN 9

// Pin for Brushless Motor ESC
#define ESC_PIN 6

// Bluetooth for communication
SoftwareSerial bluetoothSerial(10, 11);

// Target Coordinates
float targetLat = 0.0;
float targetLon = 0.0;
bool targetSet = false;

// Return Coordinates
float returnLat = 0.0;
float returnLon = 0.0;
bool returning = false;

// Initialize GPS
TinyGPSPlus gps;
SoftwareSerial gpsSerial(RXPin, TXPin);

// Initialize LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Initialize Servo (Rudder Control)
Servo rudder;

// Compass Module
Adafruit_HMC5883_Unified compass = Adafruit_HMC5883_Unified(12345);

// Speed Control Variables
int motorSpeed = 1200;

// Functions
void setup() {
  // Serial and GPS Initialization
  Serial.begin(9600);
  gpsSerial.begin(9600);

  // LCD Initialization
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("System Initializing");
  delay(2000);
  lcd.clear();

  // Servo Initialization
  rudder.attach(RUDDER_PIN);
  rudder.write(90);

  // ESC Initialization
  pinMode(ESC_PIN, OUTPUT);
  armESC();

  // Ultrasonic Sensor Initialization
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Compass Initialization
  if (!compass.begin()) {
    lcd.setCursor(0, 0);
    lcd.print("Compass Error!");
    while (1);
  }

  // Bluetooth communication setup
  bluetoothSerial.begin(9600);

  lcd.setCursor(0, 0);
  lcd.print("Ready...");
  delay(1000);
}

void loop() {
  // Handle Bluetooth Commands
  handleBluetooth();

  // If target is set, start handling GPS, obstacles, and speed
  if (targetSet) {
    // Handle GPS Data
    handleGPS();

    // Perform Obstacle Avoidance
    if (detectObstacle()) {
      bluetoothSerial.println("HC-05 > Obstacle Detected");
      rudder.write(45); // Turn rudder to left if obstacle detected
      controlSpeed(0);  // Stop motor briefly
      delay(1000);
    } else {
      rudder.write(90); // Continue forward
    }
  }

  delay(100);
}

// Handle Bluetooth Data
void handleBluetooth() {
  if (bluetoothSerial.available()) {
    String command = bluetoothSerial.readStringUntil('\n');

    int commaIndex = command.indexOf(',');
    if (commaIndex != -1) {
      String latString = command.substring(0, commaIndex);
      String lonString = command.substring(commaIndex + 1);

      targetLat = latString.toFloat();
      targetLon = lonString.toFloat();
      targetSet = true;

      if (gps.location.isValid()) {
        returnLat = gps.location.lat();
        returnLon = gps.location.lng();
        returning = false;
      }

      lcd.setCursor(0, 0);
      lcd.print("Target Received");
      bluetoothSerial.println("HC-05 > Target Received");
      delay(1000);
    } else {
      bluetoothSerial.println("HC-05 > Invalid Coordinates");
    }
  }
}

// GPS handling
void handleGPS() {
  if (gpsSerial.available() > 0 && gps.encode(gpsSerial.read())) {
    if (gps.location.isValid()) {
      float currentLat = gps.location.lat();
      float currentLon = gps.location.lng();
      float distance = TinyGPSPlus::distanceBetween(currentLat, currentLon, targetLat, targetLon);
      float targetDirection = TinyGPSPlus::courseTo(currentLat, currentLon, targetLat, targetLon);

      String currentCoords = "Lat: " + String(currentLat, 6) + ", Lon: " + String(currentLon, 6);
      bluetoothSerial.println(currentCoords);
      Serial.println(currentCoords);

      if (distance < 1) { // Reached the target
        controlSpeed(0); // Stop the motor
        rudder.write(90); // Straighten the rudder
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Target Reached!");
        bluetoothSerial.println("HC-05 > Target Reached!");

        if (!returning) { // If at target coordinates
          delay(5000); // Wait for 5 seconds
          targetLat = returnLat; // Set return coordinates as new target
          targetLon = returnLon;
          returning = true;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Returning...");
          bluetoothSerial.println("HC-05 > Returning...");
        } else {
          targetSet = false; // Stop navigation
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Back at Start!");
          bluetoothSerial.println("HC-05 > Back at Start!");
        }
        return;
      }

      // Display current coordinates on LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Cur: ");
      lcd.print(currentLat, 6); // Current Latitude
      lcd.setCursor(0, 1);
      lcd.print(currentLon, 6); // Current Longitude

      delay(1000);
      // Display target coordinates 
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Target: ");
      lcd.setCursor(0, 1);
      lcd.print(targetLat, 6); // Target Latitude
      lcd.setCursor(8, 1);
      lcd.print(targetLon, 6); // Target Longitude
      adjustRudder(targetDirection);

      controlSpeed(distance);
    } else {
      lcd.setCursor(0, 0);
      lcd.print("Waiting GPS...");
    }
  }
}

// Adjust Rudder Based on GPS/Compass Logic
void adjustRudder(float targetDirection) {
  float heading = getHeading();
  float rudderError = targetDirection - heading;

  int rudderAngle = 90 + rudderError;
  rudderAngle = constrain(rudderAngle, 45, 135);
  rudder.write(rudderAngle);
}

// Compass heading logic
float getHeading() {
  sensors_event_t event;
  compass.getEvent(&event);

  float heading = atan2(event.magnetic.y, event.magnetic.x);
  heading = heading * 180 / M_PI;
  if (heading < 0) {
    heading += 360;
  }
  return heading;
}

// Motor speed control logic
void controlSpeed(float distance) {
  if (distance < 10) {
    motorSpeed = 1100;
  } else if (distance < 50) {
    motorSpeed = 1300;
  } else {
    motorSpeed = 1500;
  }

  motorSpeed = constrain(motorSpeed, 1100, 2000);
  analogWrite(ESC_PIN, motorSpeed);
}

// Obstacle detection logic
bool detectObstacle() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  int distance = duration * 0.034 / 2;

  return distance < 30;
}

// ESC arming logic
void armESC() {
  digitalWrite(ESC_PIN, LOW);
  delay(1000);
  digitalWrite(ESC_PIN, HIGH);
  delay(1000);
  digitalWrite(ESC_PIN, LOW);
  delay(1000);
}