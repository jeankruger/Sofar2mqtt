#ifndef AUTO_CONNECT_WRAPPER_H
#define AUTO_CONNECT_WRAPPER_H

#if defined(ARDUINO_ARCH_ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <LittleFS.h>
  typedef ESP8266WebServer  WiFiWebServer;
  #define FORMAT_ON_FAIL
#elif defined(ARDUINO_ARCH_ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  #include <FS.h>
  #include <SPIFFS.h>
  typedef WebServer WiFiWebServer;
  #define FORMAT_ON_FAIL  true
#endif

#include <AutoConnect.h>

class Configuration {
public:
  String identifier = "Sofar2MQTT";
  String mqttServer;
  int mqttPort = 1883;
  String mqttUser;
  String mqttPassword;

  bool isValid() {
    return identifier.length() > 0 && mqttServer.length() > 0 && mqttPort > 0;
  }
};

class AutoConnectWrapper {
private:
    const char* CONF_URI = "/configuration";
    const char* CONF_FILE = "/configuration.json";

    WiFiWebServer server;
    AutoConnect portal;
    AutoConnectConfig config;
    
    boolean onOTA;
    Configuration configuration;
    String ipAddress;
    String console;
    int loopCount;

    AutoConnectAux auxConfig;
    AutoConnectText confHeader;
    AutoConnectText confHeaderMisc;
    AutoConnectInput confIdentifier;
    AutoConnectText confHeaderMqtt;
    AutoConnectInput confMqttServer;
    AutoConnectInput confMqttPort;
    AutoConnectInput confMqttUser;
    AutoConnectInput confMqttPassword;
    AutoConnectElement confNewline;
    AutoConnectSubmit confSave;

    void displayRoot();
    void loadConfiguration();
    void saveConfiguration();
    void parseConfiguration();
    void onConnect(IPAddress& ipaddr);
    void OTAStart();
    String onDisplayConfiguration(AutoConnectAux& aux, PageArgument& args);

public:
    AutoConnectWrapper();

    Configuration getConfiguration();
    boolean isConnected();
    boolean isUpdating();
    boolean isReady();
    String getIpAddress();

    void setup();
    void loop();
    void logln(String str);
    void log(String str);
};

#endif //AUTO_CONNECT_WRAPPER_H
