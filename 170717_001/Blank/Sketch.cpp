/*Begining of Auto generated code by Atmel studio */
#include <Arduino.h>
/*End of auto generated code by Atmel studio */

#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <WiFiEspClient.h>
#include <WiFiEsp.h>
#include <WiFiEspUdp.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <MQ2.h>
#include <EEPROM.h>
//#include <TimeLib.h>
#include <SimpleDHT.h>

int pinDHT11 = A1;
SimpleDHT11 dht11;

// ID of the settings block
#define CONFIG_VERSION "ls1"

// Tell it where to store your config data in EEPROM
#define CONFIG_START 32

// Settings structure
struct StoreStruct {
	char version[4];
	char __light1, __light2, __light3;
	} storage = {
	CONFIG_VERSION,
	0, 0, 0
};

/************************* WiFi Access Point *********************************/
char WLAN_SSID[] = "G3TOFF_TP";
char WLAN_PASS[] = "royalwater@263";

/************************* Adafruit.io Setup *********************************/
const char *AIO_SERVER = "192.168.0.200";
uint16_t AIO_SERVERPORT = 1883;                   // use 8883 for SSL
const char *AIO_USERNAME = "root";
const char *AIO_KEY = "toorP@55w0rd";
const char *AIO_CHANNEL_SUB = "DEV_ID/SUB/DATA";
const char *AIO_CHANNEL_PUB = "DEV_ID/PUB/DATA";

/************ Global State (you don't need to change this!) ******************/
// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiEspClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
//Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT);

/****************************** Feeds ***************************************/
// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish client_pub = Adafruit_MQTT_Publish(&mqtt, AIO_CHANNEL_PUB);

// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe client_sub = Adafruit_MQTT_Subscribe(&mqtt, AIO_CHANNEL_SUB);

const byte __RXPIN__  = 3;
const byte __TXPIN__  = 2;
const byte pin = A0;
SoftwareSerial soft(__RXPIN__, __TXPIN__); // RX, TX
MQ2 mq2(pin);

uint8_t status = WL_IDLE_STATUS;
const byte __LM35_PIN = A1;
const byte L1 = 8;
const byte L2 = 9;
const byte L3 = 10;
const byte L4 = 11;

void InitWiFi();
void MQTT_connect();
void getAndSendData();
void loadConfig();
void saveConfig();

void setup() {
	pinMode(13, OUTPUT);
	pinMode(L1, OUTPUT);
	pinMode(L2, OUTPUT);
	pinMode(L3, OUTPUT);
	
	Serial.begin(9600);
	mq2.begin();
	 
	InitWiFi();
	mqtt.subscribe(&client_sub);
	
	loadConfig();
	
	digitalWrite(L1, storage.__light1);
	digitalWrite(L2, storage.__light2);
	digitalWrite(L3, storage.__light3);
}

uint8_t lastMillis = 0;
Adafruit_MQTT_Subscribe *subscription;
void loop() {
	MQTT_connect();
	
	if ((millis() - lastMillis) > 1400){
		digitalWrite(13, digitalRead(13)^1);
		getAndSendData();
		lastMillis = millis();
	}
	
	// Read incoming data from subscription
	while ((subscription = mqtt.readSubscription(500))) {
		digitalWrite(13, digitalRead(13)^1);
		if (subscription == &client_sub) {
			const size_t bufferSize = JSON_OBJECT_SIZE(3) + 20;
			DynamicJsonBuffer jsonBuffer(bufferSize);
			JsonObject& sub_root = jsonBuffer.parseObject((char *)client_sub.lastread);
			if (!sub_root.success()) {
				Serial.println("parseObject() failed");
			}
			sub_root.printTo(Serial);
			Serial.println();
				
			const char* cmd = sub_root["CMD"];

			if(!strcmp("LIGHT",cmd)){
				const char* SW = sub_root["SW"];
				char st = SW[3] - 0x30;
				
				if(strstr(SW, "L1") != 0){
					digitalWrite(L1, st);
					storage.__light1 = st;
				}

				if(strstr(SW, "L2") != 0){
					digitalWrite(L2, st);
					storage.__light2 = st;
				}

				if(strstr(SW, "L3") != 0){
					digitalWrite(L3, st);
					storage.__light3 = st;
				}
			}
			saveConfig();
		}
	}
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
	uint8_t ret;

	// Stop if already connected.
	if (mqtt.connected()) {
		return;
	}

	//Serial.println(F("Connecting to MQTT ... "));

	uint8_t retries = 3;
	while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
		Serial.println(mqtt.connectErrorString(ret));
		if(retries > 0) {
			Serial.println(F("Retrying MQTT connection in 5 seconds..."));
			mqtt.disconnect();
		}
		if (retries == 0) {
			// basically die and wait for WDT to reset me
			Serial.println(F("MQTT connection failed check parameters. :=("));
			while (1);
		}
		retries--;
		delay(5000);  // wait 5 seconds
	}
	Serial.println(F("MQTT Connected!"));
}

void InitWiFi() {
	// initialize serial for ESP module
	soft.begin(9600);
	
	// initialize ESP module
	WiFi.init(&soft);
	
	// check for the presence of the shield
	if (WiFi.status() == WL_NO_SHIELD) {
		Serial.println(F("WiFi shield not present"));
		// don't continue
		while (true);
	}

	// attempt to connect to WiFi network
	while (status != WL_CONNECTED) {
		status = WiFi.begin(WLAN_SSID, WLAN_PASS);
		delay(500);
	}
	Serial.println(F("Connected to AP"));
}

void loadConfig() {
	if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
	EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
	EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2])
	for (unsigned int t=0; t<sizeof(storage); t++)
	*((char*)&storage + t) = EEPROM.read(CONFIG_START + t);
}

void saveConfig() {
	for (unsigned int t=0; t<sizeof(storage); t++)
	EEPROM.write(CONFIG_START + t, *((char*)&storage + t));
}

void getAndSendData() {
	// Read Gases in the air
	float* values = mq2.read(false); //set it false if you don't want to print the values in the Serial
	
	// read without samples.
	byte temperature = 0;
	byte humidity = 0;
	int err = SimpleDHTErrSuccess;
	dht11.read(pinDHT11, &temperature, &humidity, NULL);
	
	const size_t bufferSize = JSON_OBJECT_SIZE(6) + 100;
	// Prepare a JSON payload string
	static char payload[bufferSize];
	DynamicJsonBuffer jsonBuffer(bufferSize);
	JsonObject& pub_root = jsonBuffer.createObject();
	char buffer[10];
	
	pub_root["LPG"]   = String(values[0]);
	pub_root["CO"]    = String(values[1]);
	pub_root["SMOKE"] = String(values[2]);
	pub_root["DegC"]  = String(temperature);
	pub_root["HUMI"]  = String(humidity);
	pub_root["LIGHT"] = String((digitalRead(L1) | (digitalRead(L2) << 1) | (digitalRead(L3) << 2)));

	pub_root.printTo(payload);
	Serial.println(payload);      // Take this off during release

	if (!client_pub.publish(payload)) {
		Serial.println(F("Publishing payload failed"));
	}
}
