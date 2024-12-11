#include <ArduinoJson.h>
#include <Arduino.h>

// Para o DHT11 - Sensor de temperatura e umidade
#include <DHT.h>

// Para o protocolo MQTT
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

// Pino analógico do DHT11
const int dhtPin = 4; 

const int humMeasurements = 100, tempMeasurements = 100;

float hum, temp, actual_hum, actual_temp; 

#define ID_MQTT "cbc1a155-0db1-446a-9a10-8304c3968412"
const char* BROKER_MQTT = "test.mosquitto.org"; // URL do broker
const int BROKER_PORT = 1883;

WiFiClient espClient;
PubSubClient MQTT(espClient); 

#define SENSORDHT11 1
#define ACTUATORLED 2 
#define DHTTYPE DHT11

DHT dht(dhtPin, DHTTYPE);

// Forward declarations
class DevicesManager;  
extern DevicesManager* manager;


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

void getSensorsTopic(char* sensorsTopic, int size){
  String sensorsTopicStr = "/esp32/" + String(WiFi.macAddress()) + "/sensors/";
  sensorsTopicStr.toCharArray(sensorsTopic, size);
}

void getActuatorsTopic(char* actuatorsTopic, int size){
  String actuatorsTopicStr = "/esp32/" + String(WiFi.macAddress()) + "/actuators/";
  actuatorsTopicStr.toCharArray(actuatorsTopic, size);
}

void reconnectMQTT(void){
  while (!MQTT.connected()){
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT)){
      char sensorsTopic[256], actuatorsTopic[256];
      getSensorsTopic(sensorsTopic, sizeof(sensorsTopic));
      getActuatorsTopic(actuatorsTopic, sizeof(actuatorsTopic));
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
  Serial.println(ssid);
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
  protected:
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
    int getType(){
      return this->type;
    }
    String getId(){
      return id;
    }

    // método virtual, que deve ser implementado pelas especializações de Sensor
    virtual void operate() = 0;
};

class DHTSensor: public Sensor{
  protected:
    float temp, hum, curHum, curTemp;
    void setJson(StaticJsonDocument<256>& json, String id, char valueStr[]){
      json["sensorId"] = id;
      json["value"] = valueStr;
      json["timestamp"] = getFmtDate();
    }
  public:
    DHTSensor(int type, String id) : Sensor(type, id){}
    void operate(){
      // A temperatura e umidade são obtidas a partir de uma média de 100 medidas
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
        char tempStr[256], humStr[256], topicStr[256];
        String topic = "/esp32/" + WiFi.macAddress() + "/sensors_data/";
        topic.toCharArray(topicStr, sizeof(topicStr));
        StaticJsonDocument<256> tempJson, humJson;
        curHum = hum;
        curTemp = temp;
        Serial.println("Temperatura: " + String(temp) + "°C\tHumidade" + String(hum) + "%");

        sprintf(tempStr, "%.2f°C", temp);
        sprintf(humStr, "%.2f%%", hum);

        setJson(tempJson, this->id, tempStr);
        setJson(humJson, this->id, humStr);

        serializeJson(tempJson, tempStr);
        serializeJson(humJson, humStr);

        MQTT.publish(topicStr, tempStr);
        MQTT.publish(topicStr, humStr);
      }
  }
};

class Actuator{
  protected:
    String id;
    int type;
    bool active;
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
    String getId(){
      return id;
    }
    int getType(){
      return type;
    }
    int isActive(){
      return active;
    }
    void virtual turnOn() = 0;
    void virtual turnOff() = 0;
};

class Led: public Actuator{
  protected:
    int pin;
  public:
    Led(String id, int type, int pin) : Actuator(id, type){
      this->pin = pin;
      pinMode(pin, OUTPUT);
    }
    Led(String id, int type) : Actuator(id, type){
      this->pin = 19;
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

class DevicesManager{
  protected:
    Sensor* sensors[256];
    Actuator* actuators[256];
    int lastActIndex, lastSenIndex;
    
  public:

    DevicesManager(){
      lastActIndex = lastSenIndex = 0;
    }

    // instancia um novo sensor de acordo com seu tipo e o insere no array de sensores, fazendo com que ele seja operado no procedimento loop
    void addSensor(String id, int type){
      switch (type){
        case 1:{
          if(lastSenIndex < 256){
            //Se já existir um sensor do mesmo tipo, o procedimento é interrompido sem adicionar o sensor
            for(int i=0; i<lastSenIndex; i++){
              if(sensors[i]->getType() == type){
                return;
              }
            }
            switch(type){
              case 1:{
                Sensor* dht = new DHTSensor(type, id);
                dht->activate();
                sensors[lastSenIndex++] = dht;
                break;
              }
              default:
                break; 
            } 
          }
          break;
        }
        default:
          break;
      }
    }

    void deleteSensor(String id){
      for(int i=0; i<lastSenIndex; i++){
        if(sensors[i] != NULL && sensors[i]->getId().equals(id)){
          // Se o sensor a ser deletado for encontrado, ele é removido de forma lógica
          lastSenIndex--;
        }
      }
    }

    void addActuator(String id, int type){
      switch (type){
        case 2: {
          Actuator* newLed = new Led(id, type);
          newLed->activate();
          actuators[lastActIndex++] = newLed;
          break;
        }  
        default:
          break;
      }
    }

    // Deleta o atuador da lista de atuadores passada por parâmetro
    void deleteActuator(String id){
      for(int i=0; i<lastActIndex; i++){
        if(actuators[i] != NULL && actuators[i]->getId().equals(id)){
          // Se o atuador a ser deletado for encontrado, ele é removido de forma lógica
          lastActIndex--;
        }
      }
    }

    // Aciona o atuador da lista de atuadores de acordo com o id e o comando passado
    void callActuator(String id, String command){
      for(int i=0; i<lastActIndex; i++){
        // Se for o atuador procurado e ele estiver ativo 
        if(actuators[i] != NULL && actuators[i]->getId().equals(id) && actuators[i]->isActive()){
          switch(actuators[i]->getType()){
            // Se o atuador for um led, ele será acionado de acordo com o comando recebido pelo MQTT
            case 2:
              if(command.equals("turnOn")){
                actuators[i]->turnOn();
              }
              else if(command.equals("turnOff")){
                actuators[i]->turnOff();
              }
              else if(command.equals("blink")){
                Led* led = static_cast<Led*>(actuators[i]); // Faz o cast para Led
                led->blink(3, 1000);
              }
            default:
              break;
          }
        }
      }
    }

    void operateAllSensors(){
      for(int i=0; i<lastSenIndex; i++){
        if(sensors[i] != NULL && sensors[i]->isActive()){
          //Se o sensor estiver ativo, ele opera medindo valores e, se for o caso, os publica
          sensors[i]->operate();
        }
      }
    }
};

// função callback usada para operar sensores - na função loop - ou acionar atuadores
void mqttCallback(char* top, byte* payload, unsigned int length){

  String topic = String(top);
  
  if(topic.indexOf("sensorsAdded") != -1){
    // Nesse caso, se trata da entrada de um sensor pelo usuário
    StaticJsonDocument<256> doc;
    // O conteúdo da mensagem é convertido de array de bytes para uma string
    char payStr[256];
    memcpy(payStr, payload, sizeof(payload));

    DeserializationError error = deserializeJson(doc, payStr);

    // Verifica se houve erro no parsing
    if (error) {
      Serial.print("Erro ao converter JSON: ");
      Serial.println(error.f_str());
      return;
    }

    // Os dados são decodificados e o sensor requisitado passa a ser operado no loop
    String id = doc["sensorId"];
    int type = doc["type"];
    manager->addSensor(id, type); 
  }
  else if(topic.indexOf("actuatorsAdded") != -1){
    // Nesse caso, se trata da entrada de um atuador pelo usuário
    StaticJsonDocument<256> doc;

    char payStr[256];
    memcpy(payStr, payload, sizeof(payload));

    DeserializationError error = deserializeJson(doc, payStr);

    // Verifica se houve erro no parsing
    if (error) {
      Serial.print("Erro ao converter JSON: ");
      Serial.println(error.f_str());
      return;
    }

    // Os dados são decodificados e o atuador passa a poder executar comandos do usuário
    String id = doc["actuatorId"];
    int type = doc["type"];
    manager->addActuator(id, type);
  }
  else if(topic.indexOf("sensors_deleted")){

    StaticJsonDocument<256> doc;
    // O conteúdo da mensagem é convertido de array de bytes para uma string
    char payStr[256];
    memcpy(payStr, payload, sizeof(payload));

    DeserializationError error = deserializeJson(doc, payStr);

    String id = doc["sensorId"];
    manager->deleteSensor(id);
  }
  else if(topic.indexOf("actuators_deleted")){
    StaticJsonDocument<256> doc;
    // O conteúdo da mensagem é convertido de array de bytes para uma string
    char payStr[256];
    memcpy(payStr, payload, sizeof(payload));

    DeserializationError error = deserializeJson(doc, payStr);

    String id = doc["actuatorId"];
    manager->deleteActuator(id);
  }
  else if(topic.indexOf("actuator_data") != -1){
    // Nesse caso, se trata de um comando sobre o atuador pelo usuário
    StaticJsonDocument<256> doc;

    char payStr[256];
    memcpy(payStr, payload, sizeof(payload));

    DeserializationError error = deserializeJson(doc, payStr);

    // Verifica se houve erro no parsing
    if (error) {
      Serial.print("Erro ao converter JSON: ");
      Serial.println(error.f_str());
      return;
    }

    String id = doc["actuatorId"];
    String cmd = doc["command"];
    manager->callActuator(id, cmd);
  }
  else{
    Serial.print("Tópico desconhecido: ");
    Serial.println(topic);
  }
}

DevicesManager* manager;

void setup(){
  Serial.begin(115200);
  //As conexões WiFi e MQTT são estabelecidas
  initWiFi();
  initMQTT();
  VerificaConexoesWiFIEMQTT();
  // O cliente NTP para o timestamp é iniciado
  timeClient.begin();
  timeClient.setTimeOffset(-3*3600); // GMT 1 = 3600, GMT -1 = -3600

  Serial.println("Esp32 Mac Adress: " + WiFi.macAddress());

  // O gerenciador de dispositivos é instanciado
  manager = new DevicesManager();
}

void loop(){
  // Caso a conexão for perdida, ela é reestabelecida
  VerificaConexoesWiFIEMQTT();

  // A cada ciclo do loop, os sensores medem valores e os publicam - se for o caso
  manager->operateAllSensors();

  // A rotina MQTT é executada
  MQTT.loop();
}