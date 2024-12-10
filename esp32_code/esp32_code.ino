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

const int humMeasurements = 100, tempMeasurements = 100;

float hum, temp, actual_hum, actual_temp; 

#define ID_MQTT "cbc1a155-0db1-446a-9a10-8304c3968412"
const char* BROKER_MQTT = "test.mosquitto.org"; //URL do broker
const int BROKER_PORT = 1883;

WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient

#define SENSORDHT11 1
#define ACTUATORLED 2 

void initWiFi(void){
  delay(10);
  Serial.print("Conectando-se na rede: ");
  Serial.println(ssid);
  reconnectWiFi();
}

void initMQTT(void){
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //informa qual broker e porta deve ser conectado
  MQTT.setCallback(mqttCallback);            //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}

void reconnectMQTT(void){
  while (!MQTT.connected()){
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT)){
      Serial.println("Conectado com sucesso ao broker MQTT!");
      char[] sensorsTopic = "/esp32/" + WiFi.macAddress() + "/sensors/";
      char[] actuatorsTopic = "/esp32/" + WiFi.macAddress() + "/actuators/";
      MQTT.subscribe(sensorsTopic);
      MQTT.subscribe(actuatorsTopic);
    }
    else{
      Serial.println("Falha ao reconectar no broker.");
      Serial.println("Havera nova tentativa de conexao em 2s");
      delay(2000);
    }
  }  
}

void reconnectWiFi(void){
  //se já está conectado à rede WI-FI, nada é feito.
  //Caso contrário, são efetuadas tentativas de conexão
  if (WiFi.status() == WL_CONNECTED)
    return;

  WiFi.begin(ssid, password); // Conecta na rede WI-FI

  while (WiFi.status() != WL_CONNECTED){
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

String getFmtDate(){
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  return timeClient.getFormattedTime();
}

class Sensor{
  private:
    String id;
    int type, active;
  public:
    Sensor(int type, String id){
      this->type = type;
      this->id = id;
      this->active = false;
    }
    void activate(){
      this->active = true;
    }
    void deactivate(){
      this->active = false;
    }
    bool isActive(){
      return this->active;
    }

    // método virtual, que deve ser implementado pelas especializações de Sensor
    virtual void operate() = 0;
};

class DHT11: public Sensor{
  private:
    float temp, hum, curHum, curTemp;
    void setJson(JsonDocument json, String id, char valueStr[]){
      json["sensorId"] = id;
      json["value"] = valueStr;
      json["timestamp"] = getFmtDate();
    }
  public:
    DHT11(int type, String id) : Sensor(type, id){}
    void operate(){
      // A temperatura e humidade são obtidas a partir de uma média de 100 medidas
      float sumHum = 0, sumTemp = 0;
      for(int i=0; i<humMeasurements; i++){
        sumHum += dht.readHumidity(); 
      }
      for(int i=0; i<tempMeasurements; i++){
        sumTemp += dht.readTemperature();
      }
      hum = sumHum / humMeasurements;
      temp = sumTemp / tempMeasurements;
      if (isnan(hum) || isnan(temp)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
      }
      else if((int)curHum != (int)hum || (int)curTemp != (int)temp){
        //Se a temperatura e umidade medidas recentemente forem diferentes das últimas medidas, os novos valores são publicados nos respectivos tópicos MQTT
        char tempStr[256], humStr[256];
        String topic = "/esp32/" + WiFi.macAddress() + "/sensors_data/";
        JsonDocument tempJson, humJson;
        curHum = hum;
        curTemp = temp;
        String toPrint = "Temperatura: " + temp + "°C\tHumidade" + hum + "%";
        Serial.println(toPrint);

        sprintf(tempStr, "%.2f°C", temp);
        sprintf(humStr, "%.2f%%", hum);

        setJson(tempJson, this->id, tempStr);
        setJson(humJson, this->id, humStr);

        serializeJson(tempJson, tempStr);
        serializeJson(humJson, humStr);

        MQTT.publish(topic, tempStr);
        MQTT.publish(topic, humStr);
      }
  }
};

class Actuator{
  private:
    String id;
    int type, active;
  public:
    Actuator(String id, int type){
      this->id = id;
      this->type = type;
      this->active = false;
    }
    void activate(){
      this->active = true;
    }
    void deactivate(){
      this->active = false;
    }
    int getId(){
      return id;
    }
    int getType(){
      return type;
    }
    void virtual turnOn() = 0;
    void virtual turnOff() = 0;
};

class Led : Actuator{
  private:
    int pin;
  public:
    Led(String id, int type, int pin) : Actuator(id, type){
      this->pin = pin;
      pinMode(pin, OUTPUT);
    }
    Led(String id, int type) : Actuator(id, type){
      this->pin = 9;
      pinMode(pin, OUTPUT);
    }
    void turnOn(){
      digitalWrite(pin, HIGH);
    }
    void turnOff(){
      digitalWrite(pin, LOW);
    }
    void blink(int times, int delayMs){
      for(int i=0; i<times; i++){
        turnOn();
        delay(delayMs/2);
        turnOff();
        delay(delayMs/2);
      }
  }
};

// instancia um novo sensor de acordo com seu tipo e o insere no array de sensores, fazendo com que ele seja operado no procedimento loop
void addSensor(String id, int type){
  switch (type){
    case 1:
      DHT11 *newDHT = new DHT11(type, id);
      newDHT->activate();
      sensors[lastSenIndex++] = *newDHT;
      break;
    
    default:
      break;
  }
}

void addActuator(String id, int type){
  switch (type){
    case 2:
      Led *newLed = new Led(id, type);
      newLed->activate();
      actuators[lastActIndex++] = *newLed;
      break;
    
    default:
      break;
  }
}

// Aciona o atuador da lista de atuadores de acordo com o id e o comando passado
void callActuator(String id, String command){
  for(int i=0; i<lastActIndex; i++){
    // Se for o atuador procurado e ele estiver ativo 
    if(actuators[i] != null && actuators[i]->getId == id && actuators[i]->isActive()){
      switch(actuators[i]->getType()){
        // Se o atuador for um led, ele será acionado de acordo com o comando recebido pelo MQTT
        case 2:
          switch(comando){
            case "turnOn":
              actuators[i]->turnOn();
              break;
            case "turnOff":
              actuators[i]->turnOff();
              break;
            case "blink":
              actuators[i]->blink(3, 1000);
              break;
            default:
              break;
          }
        default:
          break;
      }
    }
  }
}

// função callback usada para operar sensores - na função loop - ou acionar atuadores
void mqttCallback(char* top, byte* payload, unsigned int length){

  String topic = top;
  
  if(topic.contains("sensors")){
    // Nesse caso, se trata da entrada de um sensor pelo usuário
    JsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, String(payload));

    // Verifica se houve erro no parsing
    if (error) {
      Serial.print("Erro ao converter JSON: ");
      Serial.println(error.f_str());
      return;
    }

    // Os dados são decodificados e o sensor requisitado passa a ser operado no loop
    String id = doc["sensorId"];
    int type = doc["type"];
    addSensor(sensorId, type); 
  }
  else if(topic.contains("actuators")){
    // Nesse caso, se trata da entrada de um atuador pelo usuário
    JsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, String(payload));

    // Verifica se houve erro no parsing
    if (error) {
      Serial.print("Erro ao converter JSON: ");
      Serial.println(error.f_str());
      return;
    }

    // Os dados são decodificados e o atuador passa a poder executar comandos do usuário
    String id = doc["actuatorId"];
    int type = doc["type"];
    addActuator(sensorId, type);
  }
  else if(topic.contains("actuator_data")){
    // Nesse caso, se trata de um comando sobre o atuador pelo usuário
    JsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, String(payload));

    // Verifica se houve erro no parsing
    if (error) {
      Serial.print("Erro ao converter JSON: ");
      Serial.println(error.f_str());
      return;
    }

    String id = doc["actuatorId"];
    String cmd = doc["command"];
    callActuator(id, cmd);
  }
  else{
    Serial.print("Tópico desconhecido: ");
    Serial.println(topic);
  }
}

Sensor *sensors[10];
int lastSenIndex = 0;
Actuator *actuators[10];
int lastActIndex = 10;

void setup(){
  initWiFi();
  initMQTT();
  VerificaConexoesWiFIEMQTT();
  timeClient.begin();

  // GMT 1 = 3600, GMT -1 = -3600
  timeClient.setTimeOffset(-3*3600);  

  Serial.println("Esp32 Mac Adress: " + WiFi.macAddress());
}

void loop(){
  VerificaConexoesWiFIEMQTT();
  for(int i=0; i<lastSenIndex; i++){
    if(sensors[i] != NULL && sensors[i]->isActive()){
      //Se o sensor estiver ativo, ele opera medindo valores e, se for o caso, os publica
      sensors[i]->operate();
    }
  }
  MQTT.loop();
}