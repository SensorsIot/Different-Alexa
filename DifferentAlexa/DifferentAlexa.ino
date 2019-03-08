/*
  Basic ESP8266 MQTT example

  This sketch demonstrates the capabilities of the pubsub library in combination
  with the ESP8266 board/library.

  It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

  It will reconnect to the server if the connection is lost using a blocking
  reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
  achieve the same result without blocking the main loop.

  To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <JQ6500_Serial.h>
#include <credentials.h>

#define BUILTIN_LED 2

#define inTopic "speechIn"
#define outTopic "speechOut"

// Update these with values suitable for your network.

const char* ssid = mySSID;
const char* password = myPASSWORD;
const char* mqtt_server = "192.168.0.203";

WiFiClient espClient;
PubSubClient client(espClient);
JQ6500_Serial mp3(14,12);

long lastMsg = 0;
char msg[50];
int value = 0;
bool sendMessage = false;
unsigned long entryPlay = millis();
int track = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  Serial.println(password);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  sendMessage = true;
  String hi = "";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    //  Serial.print((char)payload[i]);
    hi = hi + (char)payload[i];
    //  Serial.println(hi);
  }

  track = hi.toInt();
  Serial.println();
  Serial.print("Track ");
  Serial.print(" ");
  Serial.println(track);
  yield();
  mp3.playFileByIndexNumber(track);

  while (mp3.getStatus() == MP3_STATUS_PLAYING && millis() - entryPlay < 2000)  {
    yield();
  }
  entryPlay = millis();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(outTopic, "hello world");
      // ... and resubscribe
      client.subscribe(inTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);

  mp3.begin(9600);
  mp3.reset();
  mp3.setVolume(30);
  mp3.setLoopMode(MP3_LOOP_NONE);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (sendMessage == true) {
    snprintf (msg, 50, "finished #%ld", track);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(outTopic, msg);
    sendMessage = false;
  }
  yield();
}
