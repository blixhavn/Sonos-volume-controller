#define ROOM_NAME "<room name>"
#define SSID "<wifi name>"
#define PASSWORD "<wifi password>"

#include "WiFiConnection.h"
#include "SONOSControl.h"

#define ENCODER_CLK D5 // GPIO4
#define ENCODER_DT D6 // GPIO0
#define BUTTON_PIN D7 // GPIO2

volatile long encoderValue = 0;
volatile bool buttonPressed = false;
volatile long lastPressTime = 0;
volatile long lastRotationTime = 0;
volatile int volumeDelta = 0;


void ICACHE_RAM_ATTR updateEncoder() {
  volatile long currentTime = millis();


  // Debounce: only register a press if it's been more than 200ms since the last press
  if (currentTime - lastRotationTime > 50) {
    int DT_VAL = digitalRead(ENCODER_DT);

    if (DT_VAL == 0) volumeDelta -= 3;
    else volumeDelta += 3;

    lastRotationTime = currentTime;
  }

}

void ICACHE_RAM_ATTR handleButtonPress() {
  volatile long currentPressTime = millis();

  // Debounce: only register a press if it's been more than 200ms since the last press
  if (currentPressTime - lastPressTime > 200) {
    buttonPressed = true;
    lastPressTime = currentPressTime;
  }
}

void setup() {
  Serial.begin(115200);

    // Connect to Wi-Fi
  connectToWiFi();

  // Discover Sonos devices
  discoverSONOSDevices();

  pinMode(ENCODER_CLK, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), updateEncoder, RISING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, FALLING);

  Serial.println("Setup complete");
}

void loop() {

  // Check if button was pressed
  if (buttonPressed) {
    Serial.println("Button pressed");
    togglePlayPause();
    buttonPressed = false;
  }

  if (volumeDelta != 0) {
    adjustVolume(volumeDelta);
    volumeDelta = 0;
  }

  delay(100);
}
