// Sistem monitoring penyiraman arus dan pH air
// pada tanaman Hydroponic secara otomatis

#include <string.h>
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADS1015.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WString.h>

using namespace std;
// Custom libraries
#include <utlgbotlib.h>

#define WIFI_SSID "Pe"
#define WIFI_PASS "1w3r5y7i"

#define MAX_CONN_FAIL 50
#define MAX_LENGTH_WIFI_SSID 31
#define MAX_LENGTH_WIFI_PASS 63

#define TLG_TOKEN "6755409088:AAFGWuf04qveLhHiXkmwHTrrsnVZaNm6CYA" // token bot telegram

#define DEBUG_LEVEL_UTLGBOT 0

#define PIN_PH_UP 18  // ESP32 RELAYnya di pin gpio18
#define PIN_PH_DOWN 5 // ESP32 RELAYnya di pin gpio05
#define PH_PIN 35
#define WATER_LEVEL_PIN 34

Adafruit_ADS1015 ads; // Objek untuk ADS1115 (analog-to-digital converter)

float calibration = 130.00; // change this value to calibrate
int sensorValue = 0;
unsigned long int avgValue;
float b;
int buf[10], temp;

const int WATER_LEVEL_THRESHOLD = 500;

const char TEXT_START[] =
    "Hello, perkenalkan saya bot arus dan pH air pada tanaman hidroponik.\n"
    "\n"
    "silahkan ketik /help perintah untuk melihat bagaimana cara menggunakan saya.";

// Telegram Bot /help text message
const char TEXT_HELP[] =
    "Perintah yang dapat digunakan:\n"
    "\n"
    "/start - ▶ Memulai program hidroponik.\n"
    "/help - 📗 Bantuan selengkapnya.\n"
    "/up_ph - ⬆ Menaikkan nilai pH air.\n"
    "/down_ph - ⬇ Menurunkan nilai pH air.\n"
    "/cek_status - 📊 Cek status air pada hidroponik.";

float readPH();
float readWaterLevel();
void wifi_init_stat(void);
bool wifi_handle_connection(void);

// Create Bot object
uTLGBot Bot(TLG_TOKEN);

// RELAY status
uint8_t relay_status_ph_UP;
uint8_t relay_status_ph_down;

void setup(void)
{
  Bot.set_debug(DEBUG_LEVEL_UTLGBOT);
  Serial.begin(9600);

  digitalWrite(PIN_PH_UP, HIGH);
  digitalWrite(PIN_PH_DOWN, HIGH);

  pinMode(PIN_PH_UP, OUTPUT);
  pinMode(PIN_PH_DOWN, OUTPUT);

  relay_status_ph_UP = 1;
  relay_status_ph_down = 1;

  wifi_init_stat();

  Serial.println("Waiting for WiFi connection.");
  while (!wifi_handle_connection())
  {
    Serial.print(".");
    delay(500);
  }

  Bot.getMe();
}

void loop()
{
  float phValue = readPH();
  float waterLevel = readWaterLevel();

  if (!wifi_handle_connection())
  {
    // Wait 100ms and check again
    delay(100);
    return;
  }


  // Check for Bot menerima pesans
  while (Bot.getUpdates())
  {
    Serial.println("");
    Serial.println("menerima pesan: ");
    Serial.println(Bot.received_msg.text);
    Serial.println("");

    if (strncmp(Bot.received_msg.text, "/start", strlen("/start")) == 0)
    {
      Bot.sendMessage(Bot.received_msg.chat.id, TEXT_START);
    }

    else if (strncmp(Bot.received_msg.text, "/help", strlen("/help")) == 0)
    {
      Bot.sendMessage(Bot.received_msg.chat.id, TEXT_HELP);
    }

    else if (strncmp(Bot.received_msg.text, "/up_ph", strlen("/up_ph")) == 0)
    {
      relay_status_ph_UP = 0;
      // Show command reception through Serial
      Serial.println("Perintah /up_ph diterima.");
      Serial.println("Menyalakan pH Up.");

      // turn on relay
      digitalWrite(PIN_PH_UP, relay_status_ph_UP);
      Bot.sendMessage(Bot.received_msg.chat.id, "Menaikkan nilai pH air...");
      delay(5000);

      // turn off relay
      relay_status_ph_UP = 1;
      digitalWrite(PIN_PH_UP, relay_status_ph_UP);
      Bot.sendMessage(Bot.received_msg.chat.id, "pH air telah dinaikkan.");
    }

    else if (strncmp(Bot.received_msg.text, "/down_ph", strlen("/down_ph")) == 0)
    {
      relay_status_ph_down = 0;
      // Show command reception through Serial
      Serial.println("Perintah /down_ph diterima.");
      Serial.println("Menyalakan pH Dowm.");

      // turn on relay
      digitalWrite(PIN_PH_DOWN, relay_status_ph_down);
      // Send a Telegram message to notify that the RELAY has been turned off
      Bot.sendMessage(Bot.received_msg.chat.id, "Menurunkan nilai pH air...");
      delay(5000);

      // turn off relay
      relay_status_ph_down = 1;
      digitalWrite(PIN_PH_DOWN, relay_status_ph_down);
      Bot.sendMessage(Bot.received_msg.chat.id, "pH air telah diturunkan.");
    }

    else if (strncmp(Bot.received_msg.text, "/cek_status", strlen("/cek_status")) == 0)
    {
      Serial.print("nilai pH air         : ");
      Serial.println(phValue, 2);
      Serial.print("level ketinggian air : ");
      Serial.println(waterLevel);

      String pesan = "----------- status saat ini -----------\nnilai pH air                : " + String(phValue) + "\n" + "level ketinggian air : " + String(waterLevel) + "\n==========================\n    pH air normal berada di rentang\n                         6,5 - 7,5\n==========================";
      Bot.sendMessage(Bot.received_msg.chat.id, pesan.c_str());
    }

    // Feed the Watchdog
    yield();
  }

  // Wait 1s for next iteration
  // delay(1000);
}

float readPH()
{
  for (int i = 0; i < 10; i++)
  {
    buf[i] = analogRead(PH_PIN);
    delay(30);
  }

  for (int i = 0; i < 9; i++)
  {
    for (int j = i + 1; j < 10; j++)
    {
      if (buf[i] > buf[j])
      {
        temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
      }
    }
  }
  avgValue = 0;

  for (int i = 2; i < 8; i++)
    avgValue += buf[i];
  float pHVol = (float)avgValue * 5.0 / 1024 / 6;
  float phValue = -5.70 * pHVol + calibration;
  delay(500);

  return phValue;
}

float readWaterLevel()
{
  int waterLevel = analogRead(WATER_LEVEL_PIN);
  return waterLevel;
}

// Init WiFi interface
void wifi_init_stat(void)
{
  Serial.println("Initializing TCP-IP adapter...");
  Serial.print("Wifi connecting to SSID: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.println("TCP-IP adapter successfuly initialized.");
}

bool wifi_handle_connection(void)
{
  static bool wifi_connected = false;

  // Device is not connected
  if (WiFi.status() != WL_CONNECTED)
  {
    // Was connected
    if (wifi_connected)
    {
      Serial.println("WiFi disconnected.");
      wifi_connected = false;
    }

    return false;
  }
  // Device connected
  else
  {
    // Wasn't connected
    if (!wifi_connected)
    {
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());

      wifi_connected = true;
    }

    return true;
  }
}