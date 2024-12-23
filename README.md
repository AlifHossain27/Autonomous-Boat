# Autonomous-Boat
A simple boat which receives coordinates via flutter app and moves towards the target coordinates while avoiding obstacle.

# Arduino Connection:

### GPS Module (NEO-6M):
- VCC → 5V
- GND → GND
- TX (Transmit) → Arduino Pin 4
- RX (Receive) → Arduino Pin 3
- Connect RX/TX pins to Arduino TX/RX (crossed connection).
### Bluetooth (HC-05):
- VCC → 5V
- GND → GND
- TXD → Arduino Pin 10
- RXD → Arduino Pin 11
### Compass (HMC5883L):
- VCC → 5V
- GND → GND
- SDA → Arduino Pin A4
- SCL → Arduino Pin A5
### Ultrasonic Sensor:
- VCC → 5V
- GND → GND
- TRIG_PIN → Arduino Pin 7
- ECHO_PIN → Arduino Pin 8
### Servo Motor:
- VCC → 5V
- GND → GND
- Signal Pin → Arduino Pin 9
### Brushless Motor with ESC:
- ESC signal wire → Arduino PWM Pin 6.
- ESC power input connected to a LiPo battery.
- Brushless motor connected to ESC.
### LCD Display:
- VCC → 5V
- GND → GND
- SDA → Arduino Pin A4
- SCL → Arduino Pin A5