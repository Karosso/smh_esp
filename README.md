# Gerenciador de Sensores e Atuadores IoT com ESP32 - Smart Home Control

Este projeto implementa um sistema IoT flexível usando um microcontrolador ESP32, capaz de gerenciar múltiplos sensores e atuadores através de comunicação MQTT. O sistema faz parte de um ecossistema maior de automação residencial, integrando-se com um aplicativo mobile em Flutter e o backend em C# que possui um serviço em MQTT para gerenciar as mensagens trocadas entre o app e o esp, e o CRUD para gerenciar o a base de dados dos usuários.

## 🌐 Ecossistema do Projeto

Este repositório é parte de um sistema maior de automação residencial que inclui:

- 📱 **Aplicativo Mobile** (Flutter): [smart_home_control](https://github.com/Karosso/smart_home_control)
- 🖥️ **Backend** (C#): [TISM_MQTT](https://github.com/Karosso/TISM_MQTT)
- 🔥 **Firebase**: Armazenamento de dados e autenticação
- 🔌 **ESP32**: Este repositório - Gerenciamento de sensores e atuadores

## 🚀 MVP - Versão Atual

A versão atual do MVP implementa os seguintes dispositivos:
- Sensor DHT11 (temperatura e umidade)
- Sensor NTC (temperatura)
- LED como atuador

## ✨ Funcionalidades

- Gerenciamento dinâmico de sensores e atuadores
- Publicação de dados em tempo real via MQTT
- Reconexão automática ao WiFi e broker MQTT
- Suporte a timestamp no formato ISO 8601
- Comunicação baseada em JSON
- Integração com Firebase para armazenamento de dados
- Controle via aplicativo mobile
- Interface com backend em C#

## 🛠️ Pré-requisitos

### Hardware
- Placa de desenvolvimento ESP32
- Sensor DHT11
- Termistor NTC
- LED
- Resistores e fiação apropriados

### Bibliotecas
```cpp
#include <ArduinoJson.h>
#include <DHT.h>
#include <Thermistor.h>
#include <NTC_Thermistor.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>
```

## 📌 Configuração dos Pinos

- Pino DHT11: GPIO 4
- Pino NTC: GPIO 34
- Pino LED: GPIO 19 (padrão)

## 📡 Tópicos MQTT

O sistema utiliza os seguintes tópicos MQTT (onde MAC é o endereço MAC do ESP32):

- `/esp32/MAC/sensors_added/` - Adicionar novos sensores
- `/esp32/MAC/sensors_rem/` - Remover sensores
- `/esp32/MAC/actuators_added/` - Adicionar novos atuadores
- `/esp32/MAC/actuators_rem/` - Remover atuadores
- `/esp32/MAC/actuators_data/` - Controlar atuadores
- `/esp32/MAC/sensors_data/` - Receber dados dos sensores

## 📊 Formatos das Mensagens JSON

### Adicionando um Sensor
```json
{
  "sensorId": "string",
  "type": number
}
```

### Adicionando um Atuador
```json
{
  "actuatorId": "string",
  "type": number
}
```

### Dados do Sensor
```json
{
  "SensorId": "string",
  "Value": "string",
  "Timestamp": "string"
}
```

### Comando para Atuador
```json
{
  "ActuatorId": "string",
  "Command": "string"
}
```

## 🔌 Sensores Suportados

| ID | Sensor | Descrição |
|----|--------|-----------|
| 1 | DHT11 | Temperatura e Umidade |
| 2 | DHT22 | Temperatura e Umidade (Alta Precisão) |
| 3 | AM2302 | Temperatura e Umidade |
| 4 | CCS811 | Qualidade do Ar |
| 5 | MH-Z19B | CO2 |
| 6 | MQ135 | Gás |
| 7 | MQ9 | Gás |
| 8 | HC-SR501 | Movimento PIR |
| 9 | RCWL0516 | Movimento Radar |
| 10 | VL53L0X | Distância a Laser |
| 11 | Ultrasonic | Distância Ultrassônica |
| 12 | Maxbotix | Distância Ultrassônica |
| 13 | TSL2561 | Luz Digital |
| 14 | APDS9960 | Cor, Luz e Proximidade |
| 15 | SW420 | Vibração |
| 16 | HX711 | Célula de Carga |
| 17 | YFS201 | Fluxo de Água |
| 18 | PH_SENSOR | pH |
| 19 | MAX30100 | Oximetria |
| 20 | NTC | Temperatura NTC |

## ⚡ Atuadores Suportados

| ID | Atuador | Descrição |
|----|---------|-----------|
| 1 | Relé 1 Canal | Controle de dispositivos simples |
| 2 | Lâmpada | Controle de iluminação |
| 3 | Relé 2 Canais | Controle duplo independente |
| 4 | Relé 4 Canais | Controle múltiplo |
| 5 | Servo Motor | Movimentos precisos |
| 6 | Motor DC | Controle de velocidade e direção |
| 7 | Motor Passo a Passo | Movimentos de alta precisão |
| 8 | Ventilador | Controle de ventilação |
| 9 | Fechadura Elétrica | Controle de acesso |
| 10 | Cortina Automática | Automação de cortinas |
| 11 | Bomba de Água | Sistemas de irrigação |
| 12 | Alarme de Segurança | Sistema de segurança |
| 13 | Luminária Inteligente | Iluminação avançada |
| 14 | Chuveiro Elétrico | Controle de temperatura |
| 15 | Compressor de Ar | Sistemas pneumáticos |
| 16 | Motor Hidráulico | Sistemas hidráulicos |
| 17 | Ponte H | Controle avançado de motores DC |

## 🔧 Instruções de Configuração

1. Configure suas credenciais WiFi:
```cpp
const char* ssid = "seu_wifi_ssid";
const char* password = "sua_senha_wifi";
```

2. Configure seu broker MQTT:
```cpp
const char* BROKER_MQTT = "broker.emqx.io";
const int BROKER_PORT = 1883;
```

3. Faça o upload do código para seu ESP32

4. Monitore a saída serial para verificar conexões e operações dos dispositivos

## ⚠️ Tratamento de Erros

O sistema inclui tratamento de erros para:
- Falhas de conexão WiFi
- Falhas de conexão MQTT
- Erros de leitura dos sensores
- Erros de análise JSON
- Falhas de comunicação com o backend
- Problemas de sincronização com o Firebase

## 📚 Estrutura das Classes

- `DevicesManager`: Classe principal para gerenciamento de sensores e atuadores
- `Sensor`: Classe base para todos os sensores
- `DHTSensor`: Implementação do sensor DHT11
- `NTCSensor`: Implementação do termistor NTC
- `Actuator`: Classe base para todos os atuadores
- `Led`: Implementação do atuador LED

## 🤝 Como Contribuir

1. Faça um Fork do projeto
2. Crie uma Branch para sua Feature (`git checkout -b feature/AmazingFeature`)
3. Faça o Commit de suas mudanças (`git commit -m 'Add some AmazingFeature'`)
4. Faça o Push para a Branch (`git push origin feature/AmazingFeature`)
5. Abra um Pull Request

## 📝 Licença

Este projeto está licenciado sob a Licença MIT - veja abaixo o texto completo:

```
MIT License

Copyright (c) 2024 Oscar Dias (https://github.com/Karosso)
Copyright (c) 2024 Bruno Reis (https://github.com/brunohreis)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## 📞 Suporte

- Em caso de dúvidas ou problemas, abra uma issue no repositório do projeto
- Para questões relacionadas ao aplicativo mobile, visite [smart_home_control](https://github.com/Karosso/smart_home_control)
- Para questões relacionadas ao backend, visite [TISM_MQTT](https://github.com/Karosso/TISM_MQTT)

## 📋 Observações Importantes

- O sistema foi projetado para ser expansível, permitindo a adição de novos tipos de sensores e atuadores
- A versão MVP atual suporta apenas LED, DHT11 e NTC, mas a estrutura está preparada para expansão
- Recomenda-se testar cada novo sensor/atuador em ambiente controlado antes do deployment
- A qualidade da conexão WiFi pode afetar a confiabilidade do sistema
- Mantenha o firmware do ESP32 atualizado para melhor compatibilidade e segurança
