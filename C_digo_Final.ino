#include <ESP8266WiFi.h> 
#include <ESP8266HTTPClient.h>
#include "DHT.h"
#include <SPI.h>
#include <math.h>
#define DHTTYPE DHT11   
#define dht_dpin D1
#define LED_BUILTIN 2

//Helix IP Address 
const char* orionAddressPath = "143.107.145.14:1026/v2";

//Device ID (example: urn:ngsi-ld:entity:001) 
const char* deviceID = "urn:ngsi-ld:143.107.145.14:015";

//Wi-Fi Credentials
const char* ssid = "AndroidAP"; 
const char* password = "rgss3827";

//global variables
int medianGas;
float totalGas;
int gassensorAnalog;
int medianTemperature;
float totalTemperature;
int medianHumidity;
float totalHumidity;
int Buzzer = D8; 
int Gas_analog = A0; 
int leitura_sensor = 300;

  
WiFiClient espClient;
HTTPClient http;
DHT dht(dht_dpin, DHTTYPE);


void setup() {
  //setup
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  pinMode(Buzzer, OUTPUT);      
  pinMode(Gas_analog, INPUT); 
 
  
  //start sensor DHT11
  dht.begin();

  
  //Wi-Fi access
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected WiFi network!");
  delay(2000);

  Serial.println("Creating " + String(deviceID) + " entitie...");
  delay(2000);
  //creating the device in the Helix Sandbox (plug&play) 
  orionCreateEntitie(deviceID);
  
}
 
void loop(){
  
//reset variables
  medianGas=0;
  totalGas=0;
  gassensorAnalog=0;
  medianTemperature=0;
  totalTemperature=0;
  medianHumidity=0;
  totalHumidity=0;
  

//looping for calculating average temperature and humidity
  for(int i = 0; i < 3; i ++){
    totalHumidity  += dht.readHumidity();
    totalTemperature += dht.readTemperature(); 
    totalGas += analogRead(Gas_analog); 
    Serial.println("COUNT[" + String(i+1) + "] - Total Humidity: " + String(totalHumidity) + " Total Temperature: " + String(totalTemperature)+ " Total Gas: " + String(totalGas));
    delay(2000); //delay 5 seg
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(250);                       
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(250); 
  };

//calculation of average values
  medianHumidity = totalHumidity/3;
  medianTemperature = totalTemperature/3;
  medianGas = totalGas/3;
  
  Serial.println("Median after 3 reads is Humidity: " + String(medianHumidity) + " Temperature: " + String(medianTemperature)+ " Gas: " + String(medianGas));
  
  char msgHumidity[10];
  char msgTemperature[10];
  char msgGas[10]; 
  sprintf(msgHumidity,"%d",medianHumidity);
  sprintf(msgTemperature,"%d",medianTemperature);
   sprintf(msgGas,"%d",medianGas);

  //update 
  Serial.println("Updating data in orion...");
  orionUpdate(deviceID, msgGas, msgTemperature, msgHumidity);

  //indicação luminosa do envio
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);                       
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(500);                          
  Serial.println("Finished updating data in orion...");

  int gassensorAnalog = analogRead(Gas_analog);  //VARIÁVEL RECEBE O VALOR LIDO NO PINO ANALÓGICO
  
  if (medianGas > 300) {
     digitalWrite (Buzzer, HIGH) ; //send tone
     delay(10);
  }
  else {
    digitalWrite (Buzzer, LOW) ;  //no tone
  }
  delay(1000); //INTERVALO DE 100 MILISSEGUNDOS
}

//plug&play
void orionCreateEntitie(String entitieName) {

    String bodyRequest = "{\"id\": \"" + entitieName + "\", \"type\": \"sensor\", \"gas\": { \"value\": \"0\", \"type\": \"integer\"},\"temperature\": { \"value\": \"0\", \"type\": \"integer\"}, \"humidity\": { \"value\": \"0\", \"type\": \"integer\"}}";
    httpRequest("/entities", bodyRequest);
}

//update 
void orionUpdate(String entitieID, String gas, String temperature, String humidity){

    String bodyRequest = "{\"gas\": { \"value\": \""+ gas + "\", \"type\": \"float\"}, \"temperature\": { \"value\": \""+ temperature +"\", \"type\": \"float\"}, \"humidity\": { \"value\": \""+ humidity +"\", \"type\": \"float\"}}";
    String pathRequest = "/entities/" + entitieID + "/attrs?options=forcedUpdate";
    httpRequest(pathRequest, bodyRequest);
}

//request
void httpRequest(String path, String data)
{
  String payload = makeRequest(path, data);

  if (!payload) {
    return;
  }

  Serial.println("##[RESULT]## ==> " + payload);

}

//request
String makeRequest(String path, String bodyRequest)
{
  String fullAddress = "http://" + String(orionAddressPath) + path;
  http.begin(fullAddress);
  Serial.println("Orion URI request: " + fullAddress);

  http.addHeader("Content-Type", "application/json"); 
  http.addHeader("Accept", "application/json"); 
  http.addHeader("fiware-service", "helixiot"); 
  http.addHeader("fiware-servicepath", "/"); 

Serial.println(bodyRequest);
  int httpCode = http.POST(bodyRequest);

  String response =  http.getString();

  Serial.println("HTTP CODE");
  Serial.println(httpCode);
  
  if (httpCode < 0) {
    Serial.println("request error - " + httpCode);
    return "";
  }

  if (httpCode != HTTP_CODE_OK) {
    return "";
  }

  http.end();

  return response;
}
