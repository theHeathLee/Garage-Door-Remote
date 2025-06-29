#include "config.h"  // Include the configuration file

#include <ESP32Servo.h>
#include <BlynkSimpleEsp32.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// WiFi credentials
char ssid[] = WIFI_SSID;
char pass[] = WIFI_PASS;

// Telegram bot settings
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

Servo myServo;
bool sweepActive = false;  // Controlled by Blynk or Telegram
int servoPin = 13;
int pos = 0;
int increment = 1;
unsigned long lastUpdate = 0;
const unsigned long interval = 15;

bool moveTriggered = false;
bool returningHome = false;
unsigned long moveStartTime = 0;
const unsigned long moveDelay = 500;

unsigned long lastTelegramCheck = 0;
const unsigned long telegramInterval = 1000;  // Check every 1 second

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  secured_client.setInsecure();  // For quick setup, skips TLS cert verification

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  myServo.setPeriodHertz(50);
  myServo.attach(servoPin, 500, 2400);
  myServo.write(0);  // Start at home
}

BLYNK_WRITE(V0) {
  int value = param.asInt();
  if (value == 1 && !moveTriggered) {
    triggerServo();
  }
}

void triggerServo() {
  moveTriggered = true;
  myServo.write(25);  // Rotate to 30 degrees
  moveStartTime = millis();
  returningHome = true;
}

void checkTelegram() {
  int newMessages = bot.getUpdates(bot.last_message_received + 1);
  for (int i = 0; i < newMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    if (chat_id == CHAT_ID) {
      if (text == "/open") {
        bot.sendMessage(chat_id, "Opening garage!", "");
        if (!moveTriggered) {
          triggerServo();
        }
      } else {
        bot.sendMessage(chat_id, "Unknown command. Send /open to activate the servo.", "");
      }
    }
  }
}

void loop() {
  unsigned long currentMillis = millis();

  if (returningHome && (currentMillis - moveStartTime >= moveDelay)) {
    myServo.write(0);  // Return to home
    returningHome = false;
    moveTriggered = false;
  }

  if (currentMillis - lastTelegramCheck >= telegramInterval) {
    checkTelegram();
    lastTelegramCheck = currentMillis;
  }

  Blynk.run();
}
