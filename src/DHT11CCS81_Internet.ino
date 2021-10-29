#include "secrets.h"
#include "ESP8266WiFi.h"
#include "ThingSpeak.h"
#include "DHT.h"
#include "Adafruit_CCS811.h"

// Configuración conexión a ThingSpeak
unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;
const char * myReadAPIKey = SECRET_READ_APIKEY;

// Configuración conexión a WIFI
char ssid[] = SECRET_SSID;   
char pass[] = SECRET_PASS;

// Pin donde ler os datos do sensor de DHT11
uint8_t DHTPin = DHT_PIN;
uint8_t LEDPin = 2;

// Variables onde gardar os datos
float temperatura;
float humidade;
float eCO2;
float VOC;

// Liña base para o arrancar o sensor CCS811
uint16_t CCS811_baseline;


Adafruit_CCS811 ccs;
DHT dht(DHT_PIN, DHTTYPE);
WiFiClient  client;


/* Aquí configuramos todo o que necesitamos
 *    - WIFI
 *    - Conexión con Thingspeak
 *    - Sensor de temperatura e humidade: DHT11
 *    - Sensor de eCO2: CCS811
 */
void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(LEDPin, OUTPUT);
  digitalWrite(LEDPin, HIGH);


// Conectado coa WIFI
  WiFi.mode(WIFI_STA);
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Intentando conectar a rede WIFI: ");
    Serial.println(SECRET_SSID);
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, pass);
      Serial.print(".");
      delay(5000);
    }
    Serial.println("\nConectado a WIFI.");
  }

// Mostrar la dirección ESP IP en el Monitor Serie
  Serial.println(WiFi.localIP());

// Iniciamos a conexión coa páxina onde enviar os datos
  ThingSpeak.begin(client);
  int statusCode = 999;
  boolean existsBaseline = false ;
  Serial.print("Intentando recuperar baseline para CCS811:");
  for( int i=0; i<4; i++){
    CCS811_baseline = ThingSpeak.readLongField(myChannelNumber, 5, myReadAPIKey);
    statusCode = ThingSpeak.getLastReadStatus();
    delay(5000);
    Serial.print(i);
    if(statusCode == 200){
      existsBaseline = true;
      break;
    }
  }
  Serial.println(CCS811_baseline);
  
//Iniciar sensor DHT11
  Serial.println("Iniciando sensor temperatura");
  pinMode(DHT_PIN, INPUT);  
  dht.begin();

//Iniciar sensor CSS811
  Serial.println("Iniciando sensor CO2");
  if(!ccs.begin()){
    Serial.println("Error al iniciar el sensor de CO2.");
    while(1);
  }
  ccs.setDriveMode(CCS811_DRIVE_MODE_10SEC);
  if(existsBaseline){
    ccs.setBaseline(CCS811_baseline);
  }
  // Esperamos a que todo esté listo.
  delay(DELAY);
}


/*
 * Aquí leemos os valores dos sensores é o enviamos ao servidor.
 *  
 */
void loop() {
             
  // Leemos a temperatura e humidade do sensor DHT11
  temperatura = dht.readTemperature();
  humidade = dht.readHumidity(); 


  // Leemos o VOC e eCO2 do sensor CCS811
  if(ccs.available()){
    if(!ccs.readData()){
      eCO2 = ccs.geteCO2();
      VOC = ccs.getTVOC();
      CCS811_baseline = ccs.getBaseline ();
      ccs.setEnvironmentalData(temperatura, humidade);
    }
  }

  // Enviamos os datos ao servidor
  ThingSpeak.setField(1,temperatura);
  ThingSpeak.setField(2,humidade);
  ThingSpeak.setField(3,eCO2);
  ThingSpeak.setField(4,VOC);
  ThingSpeak.setField(5,CCS811_baseline);

  int httpCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  if (httpCode == 200) {
    Serial.println("Enviados los datos correctamente.");
     digitalWrite(LEDPin, LOW);
     delay(500);
     digitalWrite(LEDPin, HIGH);
  }
  else {
    Serial.println("Problema al enviar los datos. HTTP error code " + String(httpCode));
  }
 
  
  Serial.print("Temperatura: ");
  Serial.println(temperatura);
  Serial.print("Humidade: ");
  Serial.println(humidade);
  Serial.print("CO2: ");
  Serial.println(eCO2);
  Serial.print("CCS_Baseline: ");
  Serial.println(CCS811_baseline);

  // Esperamos para a seguinte lectura
  delay(DELAY);
}
