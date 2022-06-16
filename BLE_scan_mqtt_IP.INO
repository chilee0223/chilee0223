#include <WiFi.h>
#include <PubSubClient.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

int scanTime = 5; //In seconds
BLEScan* pBLEScan;

//const char* ssid = "papa10241";
//const char* password = "papa710319";

const char* ssid = "ML6A01";
const char* password = "0989212698";
const char* mqtt_server = "192.168.50.150";

const char* HostName = "ESP32BLE_03";

const char* topic_status = "BLE/03/status";
const char* topic_BLE = "BLE/03/ask";
const char* topic_mac = "BLE_mac";


const char* mqttUser = "MQTT USER";
const char* mqttPassword = "MQTT PASSWORD";


IPAddress local_IP(192,168,50,103);
IPAddress gateway(192,168,50,1);
IPAddress subnet(255,255,0,0);
IPAddress primaryDNS(192,168,50,1);
IPAddress secondaryDNS(192,168,50,4);

const int ledPin = 2 ;

unsigned int count;
long int ScanTime_init;
char clientId[50];
char ble_status[50];
String ble_server_id;
String  resp_msg ="";
String  resp_msg_info ="";

WiFiClient espClient;
PubSubClient mqttclient(espClient);


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      //Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());

      //String  resp_msg_info ="";
      int BeaconRSSI = advertisedDevice.getRSSI();
      String BeaconAddress = advertisedDevice.getAddress().toString().c_str();


      if  (ble_server_id == BeaconAddress ) {
        //Serial.print("Addr :") ;
        //Serial.print(BeaconAddress);

        //Serial.print("RSSI :") ;
       
        Serial.println(BeaconRSSI);
        
      int BeaconTXpower = advertisedDevice.getTXPower();
      String BeaconUUID= advertisedDevice.getServiceUUID().toString().c_str();
      String Device_data = advertisedDevice.toString().c_str();
      String MF_data = advertisedDevice.getManufacturerData().c_str();
      String Device_name = advertisedDevice.getName().c_str();
     
      // 組合MQTT訊息

          if (count  > 3) {
           resp_msg=String(HostName)  +","+String(BeaconRSSI)+","+","+","+",";
          }
          if (count  < 3) {
           resp_msg=String(HostName) +","+String(BeaconRSSI)+","+BeaconAddress +","+BeaconUUID+","+MF_data+","+Device_name;
          }
    
      //resp_msg=String(HostName)  +","+","+","+","+",";

      //resp_msg_info=String(HostName) +","+String(BeaconRSSI)+","+BeaconAddress +","+BeaconUUID+","+MF_data+","+Device_name;
      // 宣告字元陣列
      byte arrSize = resp_msg.length() + 1;
      char msg[arrSize];
      
      resp_msg.toCharArray(msg, arrSize);   // 把String字串轉換成字元陣列格式   
      mqttclient.publish(topic_BLE, msg);   // 發布MQTT主題與訊息
      //Serial.println(resp_msg);
     digitalWrite(ledPin,HIGH);
        //Serial.println("--------  detect  --------");
     //Serial.println(count);
      }
    }
};

void reconnect() {
  while (!mqttclient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttclient.connect(HostName, mqttUser, mqttPassword)) {
      //if (client.connect(HostName, mqttUser, mqttPassword)) {
      Serial.println("connected");
      mqttclient.subscribe(topic_status);
      mqttclient.subscribe(topic_mac);
      mqttclient.subscribe(topic_BLE);
      

    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttclient.state());
      Serial.println(" try again in 5 seconds");
      delay(2000);
    }
  }
}

void callback(String topic, byte* message, unsigned int length) {
  String messageTemp;


  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }
  if (topic == topic_mac) {

    ble_server_id = messageTemp;

  }


  if (topic == topic_status) {

    if (messageTemp == "scan") {
      //Serial.println("PING");

      sprintf(ble_status,  "scan");   

    }

    else if (messageTemp == "stop") {
      sprintf(ble_status, "stop");
      digitalWrite(ledPin,LOW);
      
    }


  }

}
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  
 if (!WiFi.config(local_IP,gateway,subnet,primaryDNS,secondaryDNS)){
  Serial.println("STA Failed to configure ... ");
 }

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HostName);
  WiFi.begin(ssid, password);

  long int StartTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    if ((StartTime + 10000) < millis())
      ESP.restart();

  }
  ScanTime_init = millis();
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());


}




void setup() {

  Serial.begin(115200);
  setup_wifi();

  
  mqttclient.setServer(mqtt_server, 1883);
  mqttclient.setCallback(callback);

  BLEDevice::init(HostName);
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value

  pinMode(ledPin,OUTPUT);
}

void loop() {
  
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }
  if (!mqttclient.connected()) {
    reconnect();
  }

 

  bool result = strcmp (ble_status, "scan");
  if (result == 0)  {
    count += 1;

    if (count  == 0) {
      ScanTime_init = millis();

    }

    if ((ScanTime_init + 100000) < millis()) {

      sprintf(ble_status,  "stop");
      count = 0;
      ScanTime_init = millis();
      mqttclient.publish(topic_status, "stop");
      
    }
    //Serial.println(WiFi.localIP());
    //Serial.println(ble_server_id);
    delay(100);


    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
    //Serial.print("Devices found: ");
    //Serial.println(foundDevices.getCount());
    //Serial.println("Scan done!");
    pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
    delay(1000);
  }
  


  //Serial.println(millis());

  mqttclient.loop();

}
