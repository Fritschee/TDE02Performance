#include <WiFi.h>          // Biblioteca para conexão Wi-Fi
#include <WebServer.h>     // Biblioteca para criação do servidor HTTP
#include <DHT.h>           // Biblioteca para leitura do sensor DHT

// Configurações do sensor DHT22
#define DHTPIN 4           // Pino digital do ESP32 conectado ao pino de dados do DHT22
#define DHTTYPE DHT22      // Define o modelo do sensor
DHT dht(DHTPIN, DHTTYPE);

// Configurações do sensor LDR
#define LDRPIN 34          // Pino analógico do ESP32 conectado ao LDR (Exemplo: GPIO34)

// Credenciais de acesso à rede Wi-Fi
const char* ssid     = "Wokwi-GUEST";   // Gateway Publico do Wokwi para testes
const char* password = "";

// Limites para alertas de temperatura e umidade
const float TEMPERATURA_MAX = 30.0; // Exemplo: Temperatura máxima permitida em °C
const float UMIDADE_MIN     = 40.0; // Exemplo: Umidade mínima permitida em %

// Cria o objeto do servidor HTTP na porta 80
WebServer server(80);

// Variáveis globais para armazenar as últimas leituras
float lastTemperature = 0.0;
float lastHumidity = 0.0;
int lastLuminosity = 0;

// Variável para controlar o tempo da próxima leitura do sensor
unsigned long lastSensorReadMillis = 0;
const long sensorReadInterval = 5000; // Intervalo de leitura do sensor em milissegundos (5 segundos)

// Função para ler o sensor de luminosidade (LDR)
int readLDR() {
  // O pino do LDR é analógico. A leitura varia de 0 (escuro) a 4095 (claro)
  return analogRead(LDRPIN);
}

// Função para verificar os limites e gerar alertas
String checkAlerts() {
  String alerts = "";
  if (lastTemperature > TEMPERATURA_MAX) {
    alerts += "<p style='color:red;'><strong>ALERTA: Temperatura Alta! (" + String(lastTemperature) + "&deg;C)</strong></p>";
    Serial.println("ALERTA: Temperatura Alta!");
  }
  if (lastHumidity < UMIDADE_MIN) {
    alerts += "<p style='color:blue;'><strong>ALERTA: Umidade Baixa! (" + String(lastHumidity) + "%)</strong></p>";
    Serial.println("ALERTA: Umidade Baixa!");
  }
  return alerts;
}

// Função que trata as requisições na rota "/"
void handleRoot() {
  // Gera a string de alertas (usando as últimas leituras globais)
  String currentAlerts = checkAlerts();

  // Cria uma página HTML simples que se atualiza a cada 5 segundos
  String html = "<html><head><meta http-equiv='refresh' content='5'/><title>Monitoramento ESP32</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; background-color: #f0f0f0; color: #333; margin: 20px;}";
  html += "h1 { color: #0056b3; }";
  html += "p { font-size: 1.1em; margin-bottom: 5px;}";
  html += ".sensor-value { font-weight: bold; color: #007bff; }";
  html += ".alert { color: red; font-weight: bold; }";
  html += "</style>";
  html += "</head><body>";
  html += "<h1>Monitoramento de Ambiente com ESP32</h1>";
  html += "<p><strong>Última Atualização:</strong> " + String(millis() / 1000) + " segundos</p>"; // Adiciona tempo de uptime
  html += "<p><strong>Temperatura:</strong> <span class='sensor-value'>" + String(lastTemperature, 1) + " &deg;C</span></p>";
  html += "<p><strong>Umidade:</strong> <span class='sensor-value'>" + String(lastHumidity, 1) + " %</span></p>";
  html += "<p><strong>Luminosidade (LDR):</strong> <span class='sensor-value'>" + String(lastLuminosity) + "</span> (0 = Escuro, 4095 = Claro)</p>";
  html += currentAlerts; // Adiciona os alertas à página
  html += "</body></html>";

  // Envia o código 200 (OK) e a página HTML para o cliente
  server.send(200, "text/html", html);
}

void setup() {
  // Inicializa o Monitor Serial para visualização dos dados
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nIniciando o sistema...");

  // Inicializa o sensor DHT22
  dht.begin();

  // Conexão com a rede Wi-Fi
  Serial.println("Conectando-se ao Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado! Endereço IP: ");
  Serial.println(WiFi.localIP());

  // Configura a rota da interface web
  server.on("/", handleRoot);
  // Inicia o servidor HTTP
  server.begin();
  Serial.println("Servidor HTTP iniciado.");

  // Força uma primeira leitura dos sensores para inicializar as variáveis globais
  // e ter dados no Serial Monitor logo de início.
  lastTemperature = dht.readTemperature();
  lastHumidity = dht.readHumidity();
  lastLuminosity = readLDR();

  // Garante que as primeiras leituras inválidas do DHT não fiquem como 0.0
  if (isnan(lastTemperature)) lastTemperature = 0.0; // Ou um valor padrão como 25.0
  if (isnan(lastHumidity)) lastHumidity = 0.0;       // Ou um valor padrão como 50.0
}

void loop() {
  // Processa as requisições HTTP
  server.handleClient();

  // Verifica se é hora de fazer uma nova leitura dos sensores
  if (millis() - lastSensorReadMillis >= sensorReadInterval) {
    lastSensorReadMillis = millis(); // Atualiza o tempo da última leitura

    // Realiza a leitura dos dados do sensor DHT
    float newHumidity = dht.readHumidity();
    float newTemperature = dht.readTemperature();
    int newLuminosity = readLDR(); // Realiza a leitura do LDR

    // Atualiza as variáveis globais COM AS ÚLTIMAS LEITURAS VÁLIDAS
    // Somente atualiza se a leitura do DHT for um número válido
    if (!isnan(newHumidity)) {
      lastHumidity = newHumidity;
    } else {
      Serial.println("Erro na leitura da umidade do DHT22!");
    }
    if (!isnan(newTemperature)) {
      lastTemperature = newTemperature;
    } else {
      Serial.println("Erro na leitura da temperatura do DHT22!");
    }
    lastLuminosity = newLuminosity; // LDR geralmente não retorna NaN

    // Exibe os dados no Monitor Serial
    Serial.print("Temperatura: ");
    Serial.print(lastTemperature, 1);
    Serial.print(" °C\t");
    Serial.print("Umidade: ");
    Serial.print(lastHumidity, 1);
    Serial.print(" %\t");
    Serial.print("Luminosidade: ");
    Serial.print(lastLuminosity);
    Serial.println();

    // Verifica e exibe alertas no Serial Monitor
    checkAlerts();
  }

  // Não usamos mais delay(5000) no final do loop,
  // pois a leitura é controlada por millis() e sensorReadInterval
}