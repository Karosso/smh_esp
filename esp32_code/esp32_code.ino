
#include <DHT.h>
#include <ArduinoJson.h>
// Para o protocolo MQTT
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Para o timestamp
#include <NTPClient.h>
#include <WiFiUdp.h>

const char* ssid = "Bruno";
const char* password = "brunominhavida1";

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

//PINO ANALÓGICO UTILIZADO PELO DHT11
const int DHT11_1 = 4; 

float hum, temp, actual_hum, actual_temp; 

#define ID_MQTT "cbc1a155-0db1-446a-9a10-8304c3968412"
const char* BROKER_MQTT = "test.mosquitto.org"; //URL do broker
const int BROKER_PORT = 1883;

WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient

Sensor sensors[10];
int lastSenIndex = 0;
Actuator actuators[10];
int lastActIndex = 10;

void initWiFi(void)
{
  delay(10);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID);
  Serial.println("Aguarde");

  reconnectWiFi();
}

void initMQTT(void)
{
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //informa qual broker e porta deve ser conectado
  MQTT.setCallback(mqtt_callback);            //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}

void reconnectMQTT(void)
{
  while (!MQTT.connected())
  {
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT))
    {
      Serial.println("Conectado com sucesso ao broker MQTT!");
      String sensorsTopic = "/esp32/" WiFi.macAddress() + "/sensors/";
      String actuatorsTopic = "/esp32/" WiFi.macAddress() + "/actuators/";
      MQTT.subscribe(sensorsTopic);
      MQTT.subscribe(actuatorsTopic);
    }
    else
    {
      Serial.println("Falha ao reconectar no broker.");
      Serial.println("Havera nova tentativa de conexao em 2s");
      delay(2000);
    }
  }  
}

void reconnectWiFi(void)
{
  //se já está conectado à rede WI-FI, nada é feito.
  //Caso contrário, são efetuadas tentativas de conexão
  if (WiFi.status() == WL_CONNECTED)
    return;

  WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Conectado com sucesso na rede ");
  Serial.println(SSID);
  Serial.print("IP obtido: ");
  Serial.println(WiFi.localIP());
}

void VerificaConexoesWiFIEMQTT(void)
{
  reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita
  reconnectWiFi(); //se não há conexão com o WiFI, a conexão é refeita
}




// função callback usada para acionar atuadores
void mqttCallback(char* top, byte* payload, unsigned int length){

  String topic = top;
  
  if(topic.contains("sensors")){
    StaticJsonDocument<256> doc;

    // Faz o parsing do JSON
    DeserializationError error = deserializeJson(doc, String(payload));

    // Verifica se houve erro no parsing
    if (error) {
      Serial.print("Erro ao parsear JSON: ");
      Serial.println(error.f_str());
      return;
    }

    // Extraímos os valores dos campos JSON
    int sensorId = doc["sensorId"];  // Atribui o valor do sensorId a variável sensorId
    int type = doc["type"];  // Atribui o valor da temperatura
    sensors[lastSenIndex++] = Sensor(type, id);
  }
  else if(topic.contais("actuators")){

  }
  else{
    Serial.print("Tópico desconhecido: ");
    Serial.println(topic);
  }
}

String getFmtDate(){
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  return timeClient.getFormattedDate();
}

void turnLedOn(){

}

void turnLedOff(){

}


class Sensor{
  int type;
  int id;
  int active;
  Sensor(int type, int id){
    this.type = type;
    this.id = id;
    this.active = false;
  }
  void activate(){
    this.active = true;
  }
  void deactivate(){
    this.active = false;
  }
  bool isActive(){
    return this.active;
  }

}

class DHT11: public Sensor{
  int temp, hum, curHum, curTemp;
  void operate(){
    hum = dht.readHumidity();
    temp = dht.readTemperature();
    if (isnan(hum) || isnan(temp)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
    else if((int)curHum != (int)hum || (int)curTemp != (int)temp){
      char temp_aux[10], hum_aux[10];
      curHum = hum;
      curTemp = temp;
      Serial.print("Temperatura: ");
      Serial.print(temp);
      Serial.println("°C");
      Serial.print("Umidade: ");
      Serial.print(hum);
      Serial.println("%");
      sprintf(temp_aux, "%.2f°C", temp);
      sprintf(hum_aux, "%.2f%%", hum);
      String topic = "/esp32/" + WiFi.macAddress() + "/sensors_data/";
      String tempMessage = 
      MQTT.publish(top, temp_aux);
      MQTT.publish(TOPICO_PUBLISH_UMIDADE, hum_aux);
    }
  }
}


void setup(){

  initWiFi();
  initMQTT();
  VerificaConexoesWiFIEMQTT();
  timeClient.begin();
  
  // GMT 1 = 3600, GMT -1 = -3600
  timeClient.setTimeOffset(-3*3600);
  
}

void loop(){

}



