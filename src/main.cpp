#include <Arduino.h>
#include <Wire.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <IRremoteESP8266.h>

#include <ClosedCube_SHT31D.h>

#include "memory.h"
#include "irsendext.h"

#include "config.h"
#include "config-common.h"

// Config

const char wifi_ssid[]     = WIFI_SSID;
const char wifi_password[] = WIFI_PASSWORD;

const char mqtt_server[]     = MQTT_SERVER;
const char mqtt_clientid[]   = MQTT_CLIENTID;
const char mqtt_username[]   = MQTT_USERNAME;
const char mqtt_password[]   = MQTT_PASSWORD;
const char mqtt_ir_topic[]   = MQTT_IR_TOPIC;
const char mqtt_temp_topic[] = MQTT_TEMP_TOPIC;
const char mqtt_rh_topic[]   = MQTT_RH_TOPIC;

// Globals

#define IR_PIN D5
WiFiClient   wifi_client;
PubSubClient mqtt_client(wifi_client);
IRsendExt    ir_sender(IR_PIN);

#define I2C_ADDRESS 0x45
ClosedCube_SHT31D sht30;

static void led_setup() { pinMode     (LED_BUILTIN, OUTPUT); }
static void led_on()    { digitalWrite(LED_BUILTIN, LOW);    }
static void led_off()   { digitalWrite(LED_BUILTIN, HIGH);   }

static void ir_send_raw(const uint8_t *data, size_t length) {
    typedef uint16_t ir_interval_t;
    const intptr_t ALIGN_MASK = ~(1 << (sizeof(ir_interval_t) - 1));

    uint32_t frequency = read_unaligned<uint32_t>(data);

    const size_t intervals_length = length - sizeof(frequency);
    const size_t intervals_count  = intervals_length / sizeof(ir_interval_t);

    ir_interval_t* aligned_buffer = nullptr;

    const ir_interval_t * intervals = reinterpret_cast<const ir_interval_t*>(
        data + sizeof(frequency));

    if (((intptr_t) intervals) & ALIGN_MASK) {
        Serial.print("realigning buffer");
        aligned_buffer = reinterpret_cast<ir_interval_t*>(
            calloc(intervals_count, sizeof(uint32_t)));

        if (!aligned_buffer) {
            Serial.println("Failed to allocate aligned buffer");
            return;
        }

        memcpy(aligned_buffer, intervals, intervals_length);
        intervals = aligned_buffer;
    }

    Serial.print("frequency = ");
    Serial.print(frequency);
    Serial.print(" intervals = [");
    for (size_t i = 0; i < intervals_count; i++) {
        Serial.print(intervals[i]);
        Serial.print(", ");
    }
    Serial.println("]");

    ir_sender.sendRaw(const_cast<ir_interval_t*>(intervals), intervals_count,
                      frequency);

    free(aligned_buffer);
}

static void wifi_setup() {
    delay(10);

    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(wifi_ssid);

    WiFi.begin(wifi_ssid, wifi_password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

static void mqtt_reconnect() {
  while (!mqtt_client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt_client.connect(mqtt_clientid, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      mqtt_client.subscribe(mqtt_ir_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

static void ir_send_one_command(const uint8_t *data, size_t size) {
    uint8_t encoding = read_unaligned<uint8_t>(data);

    data += sizeof(encoding);
    size -= sizeof(encoding);

    switch (encoding) {
    case NEC:
    case SONY:
    case RC5:
    case RC6:
    case DISH:
    case JVC:
    case SAMSUNG:
    case LG:
    case WHYNTER:
    case COOLIX:
    case DENON:
        if (size == sizeof(uint32_t)) {
            ir_sender.send(encoding, read_unaligned<uint32_t>(data), size << 3);
        }
        else {
            Serial.print("Unexpected generic encoding size: ");
            Serial.print(size);
            Serial.println();
        }
        break;

    case DELAY:
        delay(read_unaligned<uint16_t>(data));
        break;

    case RAW:
    case 240: // Andrew's IR sender sends this, thinking it's raw.
        ir_send_raw(data, size);
        break;

    case PANASONIC_RAW:
        ir_sender.sendPanasonicRaw(data, size);
        break;

    default:
        Serial.print("Unexpected encoding: ");
        Serial.print(encoding);
        Serial.println();
    }

}

static void mqtt_handle_message(char* topic, const uint8_t* data, size_t size) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] size = ");
    Serial.println(size);

    led_on();

    const uint8_t *end = data + size;
    uint8_t nCommands = read_unaligned<uint8_t>(data);
    data += sizeof(nCommands);
    for (uint8_t cmd = 0; cmd < nCommands; cmd++) {
        uint8_t cmdSize = read_unaligned<uint8_t>(data);
        data += sizeof(cmdSize);
        if (data + cmdSize > end) {
            Serial.print("Invalid command encoding; length overruns mqtt packet\n");
            break;
        }

        ir_send_one_command(data, cmdSize);
        data += cmdSize;
    }
    if (data != end) {
        Serial.print("Invalid command encoding; parsed contents not exhaustive\n");
    }

    led_off();

    delay(100); // give all IR protocols a nice big gap
}


void setup() {
    pinMode(IR_PIN, OUTPUT);
    digitalWrite(IR_PIN, LOW);

    ir_sender.begin();

    led_setup();
    led_on();

    Serial.begin(115200);

    Wire.begin();
    sht30.begin(I2C_ADDRESS);

    wifi_setup();

    mqtt_client.setServer(mqtt_server, 1883);
    mqtt_client.setCallback(mqtt_handle_message);

    led_off();
}

void loop() {
    static unsigned long last_sht = 0;

    if (!mqtt_client.connected()) {
        mqtt_reconnect();
    }
    mqtt_client.loop();

    if ((millis() - last_sht) > 10000) {
        SHT31D result = sht30.readTempAndHumidity(SHT3XD_REPEATABILITY_HIGH,
                                                  SHT3XD_MODE_POLLING, 50);
        if (result.error == SHT3XD_NO_ERROR) {
            float temp = result.t;
            float rh = result.rh;
            char temp_s[10];
            char rh_s[10];

            snprintf(temp_s, sizeof(temp_s), "%.2f", temp);
            snprintf(rh_s, sizeof(rh_s), "%.2f", rh);

            mqtt_client.publish(mqtt_temp_topic, temp_s);
            mqtt_client.publish(mqtt_rh_topic, rh_s);

            last_sht = millis();
        }
    }
}
