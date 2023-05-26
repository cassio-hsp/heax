# heax
Heax module firmware

# HEAX 1- NodeMCU (ESP12e) + DHT11 + BMP180
##Características:
1. Cria página em Webserver dentro do PA gerado em modo config.
2. Permite selecionar que dado dos sensores DHT e BMP serão enviados via MQTT.
3. Comutação e reconexão automática pós finalização da  configuração.
4. Verifica status de conexão WiFi. Se cair por mais de 4min --> modo config.
5. Verifica status de conexão com broker MQTT, tenta reconexão.
6. Dados de config obtidos via página web são salvos em memória E2PROM emulada.
7. Se faltar energia/bateria ---> quando realimentado o ESP12e busca na memória as credenciais WiFi, as leituras de sensores desejadas.
8. Permite implementar controle remoto via MQTT usando void callback().
9. Foram modificados os topic levels para cada sensor (ver)
10. Foram melhoradas as páginas Web de configuração e confirmação.
