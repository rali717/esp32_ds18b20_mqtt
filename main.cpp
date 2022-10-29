// ----------------------------------------------------------------------------
// ---   OneWire DS18B20 to MQTT   --------------------------------------------
// ----------------------------------------------------------------------------

#include <Arduino.h>
#include <OneWire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DallasTemperature.h>

void connect_wifi();
void mqtt_callback(char *topic, byte *message, unsigned int length);
void mqtt_reconnect();
String get_DeviceAddress_as_String(DeviceAddress deviceAddress);

WiFiClient espClient;

char *ssid = (char *)"WLAN-Name";
const char *password = (char *)"Password";

PubSubClient mqtt_client(espClient);
// const char *mqtt_server = "192.168.178.200";
// const char *mqtt_server = "x1"; 
const char *mqtt_server = "The_MQTT_Broker"; 
long lastMsgSend_ms = 0;

#define TEMPERATURE_PRECISION 12 // 9
#define ONE_WIRE_BUS_PIN 19
OneWire oneWire_Bus(ONE_WIRE_BUS_PIN);
DallasTemperature sensors_OneWireBus(&oneWire_Bus);

int deviceCount = 0;
DeviceAddress oneWire_DeviceAddress;
float tempC = 0.0f;

//-----------------------------------------------------------------------------
void setup(void)
{
  Serial.begin(115200);
  mqtt_client.setServer(mqtt_server, 1883);
  mqtt_client.setCallback(mqtt_callback);
  delay(1000);
}
//-----------------------------------------------------------------------------

void loop(void) // Main-Loop
{
  if (WiFi.status() != WL_CONNECTED)
    connect_wifi();

  if (!mqtt_client.connected())
    mqtt_reconnect();

  mqtt_client.loop();

  long now_ms = millis();
  if (now_ms - lastMsgSend_ms > 5000)
  {
    lastMsgSend_ms = now_ms;

    byte present = 0;
    byte sensor_type;
    byte data[12];
    byte addr[8];
    float celsius;
    char tempString[8];

    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    String ip_string = WiFi.localIP().toString();
  

    String json_string = "{\"name\":\"DS18B20-Temp-Sensors\",\"ip\":\"";
    json_string.concat(ip_string);

    json_string.concat("\",\"mac\":\"");

    json_string.concat(WiFi.macAddress());
    json_string.concat("\"");


    deviceCount = 0;

    sensors_OneWireBus.begin();
    delay(1000);
    deviceCount = sensors_OneWireBus.getDeviceCount();

    Serial.print("\n");
    Serial.print(deviceCount, DEC);
    Serial.print("  Devices found:");
    
    delay(500);

    for (int i = 0; i < deviceCount; i++)
    {
      //json_string.concat("\",");

      sensors_OneWireBus.setResolution(oneWire_DeviceAddress, TEMPERATURE_PRECISION);
      delay(100);
      sensors_OneWireBus.requestTemperatures();
      delay(100);
      sensors_OneWireBus.getAddress(oneWire_DeviceAddress, i);

      String deviceAdress_String = get_DeviceAddress_as_String(oneWire_DeviceAddress);

      Serial.print("\nSensor ");
      Serial.print(i + 1);
      Serial.print(":  Read from Device: ");
      Serial.print(deviceAdress_String);
      delay(100);
      tempC = sensors_OneWireBus.getTempC(oneWire_DeviceAddress);
      if (tempC == -127)
      {
        Serial.print("\n\nError in Temperatur value!\nSensor connection faulty?\n\n");
        return;
      }

      Serial.print("  the Temperature: ");
      Serial.print(tempC);

      json_string.concat(",\"");
      json_string.concat(deviceAdress_String);
      json_string.concat("\":");

      dtostrf(tempC, 1, 2, tempString);

      json_string.concat(tempString);
    }

    json_string.concat("}\0");
    mqtt_client.publish("sensors/DS18B20", json_string.c_str());

    Serial.print("\n\nSend MQTT-JSON-String:   \n");
    Serial.print(json_string);
    Serial.print("\n\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
  }
} //--- End of loop ---
//-----------------------------------------------------------------------------

void connect_wifi()
{

  delay(500);
  Serial.print("\n\nConnecting to ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
}
//-----------------------------------------------------------------------------

void mqtt_callback(char *topic, byte *message, unsigned int length)
{

  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off".
  // Changes the output state according to the message
  if (String(topic) == "esp32/output")
  {
    Serial.print("Changing output to ");
    if (messageTemp == "on")
    {
      Serial.println("on");
      //    digitalWrite(ledPin, HIGH);
    }
    else if (messageTemp == "off")
    {
      Serial.println("off");
      //   digitalWrite(ledPin, LOW);
    }
  }
} //--- End of mqtt_callback ---
//-----------------------------------------------------------------------------

void mqtt_reconnect()
{
  // Loop until we're reconnected
  while (!mqtt_client.connected())
  {
    if (WiFi.status() != WL_CONNECTED)
      connect_wifi();

    Serial.print("Attempting MQTT connection...");
    if (mqtt_client.connect("ESP8266Client"))
    {
      Serial.println("connected");
      mqtt_client.subscribe("esp32/output");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
} // End mqtt_reconnect
//-----------------------------------------------------------------------------

String get_DeviceAddress_as_String(DeviceAddress deviceAddress)
{
  String address_string = "";

  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 0x10)
    {
      address_string += "0";
    }
    address_string += String(deviceAddress[i], HEX);
  }
  address_string.toUpperCase();
  return address_string;
}
//-----------------------------------------------------------------------------
