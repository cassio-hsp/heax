/*HEAX 1- NodeMCU (ESP12e) + DHT11 + BMP180
Características:
1- Cria página em Webserver dentro do PA gerado em modo config.
2- Permite selecionar que dado dos sensores DHT e BMP serão enviados via MQTT.
3- Comutação e reconexão automática pós finalização da  configuração.
4- Verifica status de conexão WiFi. Se cair por mais de 4min --> modo config.
5- Verifica status de conexão com broker MQTT, tenta reconexão.
6- Dados de config obtidos via página web são salvos em memória E2PROM emulada.
7- Se faltar energia/bateria ---> quando realimentado o ESP12e busca na memória as credenciais WiFi, as leituras de sensores desejadas.
8- Permite implementar controle remoto via MQTT usando void callback().
9- Foram modificados os topic levels para cada sensor (ver)
10- Foram melhoradas as páginas Web de configuração e confirmação.*/

#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>

const char *ssid_AP = "NodeDevice";           //const char *password_PA = "ESPLink";
const char* mqtt_server = "192.168.15.5";

ESP8266WebServer server(80);

WiFiClient cliente;
PubSubClient client(cliente);

#define DHTPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN,DHTTYPE);

Adafruit_BMP085 bmp180;

long tmp2 = 0;
char msg[50];
char temp_dht_msg[50];
char umi_dht_msg[50];
char temp_bmp_msg[50];
char alt_bmp_msg[50];

int PA_OK=0;
int CTR_WEB=0;        //Variavel BOOL do botão [Salvar WiFi Config] na pag. web.
int timeout=0;

String ssid;
String pass;
String ssid_r;
String pass_r;

String temp_dht11;
String umi_dht11;
String temp_bmp180;
String alt_bmp180;

String broker;
String usuario;
String senha;
String porta;
String broker_r;
String usuario_r;
String senha_r;
String porta_r;
int porta_i=1;

String content;
//----------------------------FUNÇÕES_SENSORIAMENTO-----------------------------
void temp_dht()
{
  snprintf (temp_dht_msg, 75, "%f", dht.readTemperature());
  client.publish("RAIZ/DHT11/TEMPERATURA", temp_dht_msg);
  Serial.print("TEMP DHT: "); Serial.println(temp_dht_msg);
  delay(1000);
}
void umi_dht()
{
  snprintf (umi_dht_msg, 75, "%f", dht.readHumidity());
  client.publish("RAIZ/DHT11/UMIDADE", umi_dht_msg);
  Serial.print("UMI DHT: ");Serial.println(umi_dht_msg);
  delay(2000);
}
void temp_bmp()
{
  snprintf (temp_bmp_msg, 75, "%f", bmp180.readTemperature());
  client.publish("RAIZ/BMP180/TEMPERATURA", temp_bmp_msg);
  Serial.print("TEMP BMP: "); Serial.println(temp_bmp_msg);
  delay(1000);
}
void alt_bmp()
{
  snprintf (alt_bmp_msg, 75, "%f", bmp180.readAltitude());
  client.publish("RAIZ/BMP180/ALTITUDE", alt_bmp_msg);
  Serial.print("ALT BMP: "); Serial.println(alt_bmp_msg);
  delay(1000);
}
//--------------------------MONITOR DE CONEXÃO WIFI-----------------------------
//Monitora o status da conexão WiFi. Se cair por mais de 4 minutos volta ao modo PA/Config.
void wifi_status()
{
  while (WiFi.status() != WL_CONNECTED)
  {
    timeout+=1;
    delay(1000);
    if(timeout==240)                        //240 unidades = 4minutos
    {
      EEPROM.write(381, 1);                //Faz eeprom(120) igual a 1 para apos o reset entrar no modo config. Ver em SETUP().
      EEPROM.commit();
      ESP.reset();
    }
  }
}
//--------------------------WEBPAGE PARA MODO ACESS POINT-----------------------
//Gera a página web/html, lê os dados digitados no formulário, salva na memória não volátil.
void webpage()
{
    content += "<!DOCTYPE HTML><html>";
    content += "<html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
    content += "<title>SenseNode - Setup</title>";
    content += "<style> body{background-color:#a6a6a4; font-family:verdana;}</style>";
    content += "<style> h3{text-align:center; background-color:#e6e6e6; font-family:verdana;}</style>";
    content += "<style> h4{text-align:center; background-color:#e6e6e6; font-family:verdana;}</style>";
    content += "<head><center><h3>SenseNode v1.3</h3>";
    content += "<p><b>SETUP DO WIFI</b></p>";
    content += "<p><form method='get' action='setting'>SSID: <br><input name='ssid' size='30' length=32></br></p>";
    content += "<p>PASSWORD: <br><input name='pass' size='30' length=64></br></p></head>";
    content += "<body><center><b>SETUP DO SERVIDOR MQTT</b>";
    content += "<p>Server MQTT (IP ou URL):<br><input name='broker' size='30' length='64'></br></p>";
    content += "<p>Usuario: <br><input name='usuario' size='30' length='64'></p>";
    content += "<p>Senha: <br><input name='senha' size='30' length=64></br></p>";
    content += "<p>Porta: <br><input name='porta' size='30' length=8></br></p></center>";

    content += "<p><b><center>DADOS A COLETAR:</center></b>";
    content += "<p><center>DHT11_Temperatura: <input type='checkbox' name='temp_dht11' value='1'></center></p>";
    content += "<p><center>DHT11_Umidade:<input type='checkbox' name='umi_dht11' value='1'></center></p>";
    content += "<p><center>BMP180_Temperatura: <input type='checkbox' name='temp_bmp180' value='1'></center></p>";
    content += "<p><center>BMP180_Altitude:<input type='checkbox' name='alt_bmp180' value='1'></center></p>";
    content += "<p><center><input type='submit' value='Salvar'></form></html></center></p>";
    content += "<h4><center>Por @heaxstudio</center></h4><body></html>";

    server.send(200, "text/html", content);


    server.on("/setting", []() {
      ssid = server.arg("ssid");
      pass = server.arg("pass");
      temp_dht11 = server.arg("temp_dht11");      //1byte. Pode ter valor 0 ou 1, para simbolizar uso ou não daquele sensor.
      umi_dht11 = server.arg("umi_dht11");        //1byte. Pode ter valor 0 ou 1, para simbolizar uso ou não daquele sensor.
      temp_bmp180 = server.arg("temp_bmp180");    //1byte. Pode ter valor 0 ou 1, para simbolizar uso ou não daquele sensor.
      alt_bmp180 = server.arg("alt_bmp180");      //1byte. Pode ter valor 0 ou 1, para simbolizar uso ou não daquele sensor.
      broker = server.arg("broker");
      usuario = server.arg("usuario");
      senha = server.arg("senha");
      porta = server.arg("porta");
      //------------------Limpeza_Memoria----------------------
      for (int i=0; i<300; ++i)
      {EEPROM.write(i, 0);}
      EEPROM.commit();
      //------------------Comprimentos-------------------------
      EEPROM.write(0, ssid.length());
      EEPROM.write(33, pass.length());
      EEPROM.write(99, broker.length());
      EEPROM.write(165, usuario.length());
      EEPROM.write(231, senha.length());
      EEPROM.write(297, porta.length());
      //------------------Gravacao_SSID_Memoria-----------------
      for (int i=0; i<ssid.length(); ++i)
      {EEPROM.write(i+1, ssid[i]);}
      //------------------Gravacao_PASS_Memoria-----------------
      for (int i=0; i<pass.length(); ++i)
      {EEPROM.write(i+34, pass[i]);}
      //------------------Gravacao_Broker_Memoria---------------
      for (int i=0; i<broker.length(); ++i)
      {EEPROM.write(i+100, broker[i]);}
      //------------------Gravacao_Usuario_Memoria--------------
      for (int i=0; i<usuario.length(); ++i)
      {EEPROM.write(i+166, usuario[i]);}
      //------------------Gravacao_Senha_Memoria--------------
      for (int i=0; i<senha.length(); ++i)
      {EEPROM.write(i+232, senha[i]);}
      //------------------Gravacao_Porta_Memoria--------------
      for (int i=0; i<porta.length(); ++i)
      {EEPROM.write(i+298, porta[i]);}
      //------------------Gravação_Sensores_Selecionados---------
      EEPROM.write(370, temp_dht11[0]);       //grava se foi checked temp_dht11
      EEPROM.write(371, umi_dht11[0]);        //grava se foi checked umi_dht11
      EEPROM.write(372, temp_bmp180[0]);      //grava se foi checked temp_bmp180
      EEPROM.write(373, alt_bmp180[0]);       //grava se foi checked press_bmp180
      EEPROM.commit();
      content = "<!DOCTYPE HTML>\r\n<html>";
      content += "<html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>\r\n";
      content += "<title>SenseNode - Setup</title>";
      content += "<style>.c{text-align: center;} div,input{padding:5px;font-size:1em;} input{width:80%;} body{background-color:#a6a6a6; text-align: center;font-family:verdana;} .q{float: right;width: 64px;text-align: right;}</style>";
      content += "</head>";
      content += "<h3>Pronto! Dispositivo configurado. Pode fechar a aba. :)</h3>";
      server.send(200, "text/html", content);
      CTR_WEB=1;
      delay(2000);
      ESP.reset();});
}

//------------------------CALLBACK DO MQTT--------------------------------------
//Callback para recepção dos dados do tópico desejado (Subscribe).
void callback(char* topic, byte* payload, unsigned int length)
{
  if ((char)payload[0] == '1')
  {
    digitalWrite(16, LOW);  //Exemplo de aplicação por subscribe.
  }
  else
  {
    digitalWrite(16, HIGH);
  }
}
//------------------------------RECONEXÃO MQTT----------------------------------
void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Tentando conexão...");
    if (client.connect("CHS_1"))          //client.connect("CHS", "eoosilwh", "xkXUfA_Fqbwt")
    {
      client.publish("RAIZ/STATUS", "Conectado."); //Publica tópico quando conectado.
      client.subscribe("INPUT/CANAL1");
    }
    else
    {
      delay(2000);      //neste local se pode colocar aviso de não conexão via Serial.
    }
  }
}
//-------------------------RESGATA CREDENCIAIS WIFI----------------------------
//Resgata da memória E2PROM emulada o SSID e PASSWORD salvos pelo usuário.
void wifi_memo()
{
  int ssid_comp = EEPROM.read(0);
  int pass_comp = EEPROM.read(33);

  for (int i = 1; i <=ssid_comp; ++i)
  {ssid_r += char(EEPROM.read(i));}
  for (int i = 34; i <(pass_comp+34); ++i)
  {pass_r += char(EEPROM.read(i));}
}
//-------------------------RESGATA CREDENCIAIS MQTT----------------------------
//Resgata da memória E2PROM emulada o BROKER e LOGIN salvos pelo usuário.
void mqtt_memo()
{
  int broker_comp = EEPROM.read(99);
  int usuario_comp = EEPROM.read(165);
  int senha_comp = EEPROM.read(231);
  int porta_comp = EEPROM.read(297);

  for (int i = 100; i <(broker_comp+100); ++i)
  {broker_r += char(EEPROM.read(i));}

  for (int i = 166; i <(usuario_comp+166); ++i)
  {usuario_r += char(EEPROM.read(i));}

  for (int i = 232; i <(senha_comp+232); ++i)
  {senha_r += char(EEPROM.read(i));}

  for (int i = 298; i <(porta_comp+298); ++i)
  {
    //!!Falta desenvolver a conversão texto-int para usar em setServer().
  }
}
//-------------------------FUNÇÃO MODO PA---------------------------------------
//Coloca o módulo em modo ponto de acesso (PA).
void rodaPA()
{
  WiFi.softAP(ssid_AP);
  IPAddress myIP = WiFi.softAPIP();
  delay(2000);
  server.on("/", webpage);
  server.begin();
  PA_OK=1;
}
//-----------------------FUNÇÃO MODO STATION------------------------------------
void rodaST()
{
  if(PA_OK==1)
  {WiFi.mode(WIFI_STA);}
  delay(1000);
  WiFi.begin(ssid_r.c_str(), pass_r.c_str());
  delay(1000);
  wifi_status();
  PA_OK=0;
}
//----------------------------------SETUP---------------------------------------
//Observar o IF dentro de SETUP. Responsável por colocar o módulo em config por botão.
void setup()
{
  Serial.begin(115200);
  EEPROM.begin(512);
  Wire.begin(4,5);
  pinMode(14, INPUT);
  pinMode(16, OUTPUT);

  if(digitalRead(14)==HIGH || EEPROM.read(380)==1)
  {
    EEPROM.write(380, 0);Serial.println("Inside Config!");
    EEPROM.commit();
    rodaPA();
    delay(1000);
    while(PA_OK==1 && CTR_WEB==0)
    {
      server.handleClient();
      delay(100);
    }
  }
  wifi_memo();
  client.setServer(broker_r.c_str(), 1883);                //client.setServer(const char, uint16_t);
  Serial.println(broker_r.c_str());
  client.setCallback(callback);
  delay(1000);
  rodaST();

  if(char(EEPROM.read(370))=='1' || char(EEPROM.read(371))=='1')     //endereços de checkbox para usar DHT.
  {
    if (!bmp180.begin())
    {
      dht.begin();
    }
  }

  if(char(EEPROM.read(372))=='1' || char(EEPROM.read(373))=='1')     //endereços de checkbox para usar BMP.
  {
    if (!bmp180.begin())
    {
      Serial.println("Sensor nao encontrado!!");
      while (1) {}
    }
  }
}

//---------------------------------LOOP-----------------------------------------
//Observar a função WiFiStatus(). O sistema está sempre monitorando o status da conexão.
//
void loop()
{
  wifi_status();

  if (!client.connected())      //verificação de conexão/reconexão do MQTT
  {
    reconnect();
  }
  client.loop();

  long tmp1 = millis();
  if (tmp1 - tmp2 > 2000)
  {
    tmp2 = tmp1;
    if(char(EEPROM.read(370))=='1')
    {temp_dht();}

    if(char(EEPROM.read(371))=='1')
    {umi_dht();}

    if(char(EEPROM.read(372))=='1')
    {temp_bmp();}

    if(char(EEPROM.read(373))=='1')
    {alt_bmp();}
  }
}
