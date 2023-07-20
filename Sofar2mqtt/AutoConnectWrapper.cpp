#include "AutoConnectWrapper.h"

#if defined(ARDUINO_ARCH_ESP8266)
  FS& FlashFS = LittleFS;
#elif defined(ARDUINO_ARCH_ESP32)
  fs::SPIFFSFS& FlashFS = SPIFFS;
#endif

AutoConnectWrapper::AutoConnectWrapper() :
    portal(server),
    loopCount(0),
    auxConfig(CONF_URI, "Configuration"),
    confHeader("confHeader", "<h2>Configuration</h2>"),
    confHeaderMisc("confHeaderMisc", "<h3>Miscellaneous</h3>"),
    confHeaderMqtt("confHeaderMqtt", "<h3>MQTT</h3>"),
    confNewline("confNewline", "<hr>")
{
  confIdentifier = AutoConnectInput("confIdentifier", configuration.identifier.c_str(), "Identifier", "^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\\-]*[A-Za-z0-9])$", "Identifier");
  confMqttServer = AutoConnectInput("confMqttServer", configuration.mqttServer.c_str(), "Server", "^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\\-]*[A-Za-z0-9])$", "192.168.0.2");
  confMqttPort = AutoConnectInput("confMqttPort", String(configuration.mqttPort).c_str(), "Port", "", "1883", AC_Tag_BR, AC_Input_Number);
  confMqttUser = AutoConnectInput("confMqttUser", configuration.mqttUser.c_str(), "User", "", "mqtt");
  confMqttPassword = AutoConnectInput("confMqttPassword", configuration.mqttPassword.c_str(), "Password", "", "password");
  confSave = AutoConnectSubmit("confSave", "Save", CONF_URI);

  auxConfig.add({
    confHeader,
    confHeaderMisc,
    confIdentifier,
    confHeaderMqtt,
    confMqttServer,
    confMqttPort,
    confMqttUser,
    confMqttPassword,
    confNewline,
    confSave
  });
}

void AutoConnectWrapper::setup() {
  logln("Setup");

  FlashFS.begin(FORMAT_ON_FAIL);

  loadConfiguration();

  server.on("/", std::bind(&AutoConnectWrapper::displayRoot, this));

  config.ota = AC_OTA_BUILTIN;
  config.autoReconnect = true;

  if (configuration.isValid()) {
    config.apid = configuration.identifier;
    config.hostName = configuration.identifier;
  }

  portal.config(config);
  portal.join(auxConfig);
  portal.on(CONF_URI, std::bind(&AutoConnectWrapper::onDisplayConfiguration, this, std::placeholders::_1, std::placeholders::_2));
  portal.onConnect(std::bind(&AutoConnectWrapper::onConnect, this, std::placeholders::_1));
  portal.onOTAStart(std::bind(&AutoConnectWrapper::OTAStart, this));
  portal.begin();
}

void AutoConnectWrapper::loop() {
  portal.handleClient();
}

void AutoConnectWrapper::loadConfiguration() {
  File confJson = FlashFS.open(CONF_FILE, "r");
  if (confJson) {
    if (auxConfig.loadElement(confJson)) {
      parseConfiguration();
      logln("Configuration loaded");
    } else {
      logln("Configuration failed to load");
    }
    confJson.close();
  } else {
    logln("Configuration not found");
  }
}

void AutoConnectWrapper::saveConfiguration() {
  File confJson = FlashFS.open(CONF_FILE, "w");
  auxConfig.saveElement(confJson, {"confIdentifier", "confMqttServer", "confMqttPort", "confMqttUser", "confMqttPassword"});
  confJson.close();
  logln("Configuration saved");
}

void AutoConnectWrapper::parseConfiguration() {
  configuration.identifier = auxConfig.getElement<AutoConnectInput>("confIdentifier").value;
  configuration.identifier.trim();

  configuration.mqttServer = auxConfig.getElement<AutoConnectInput>("confMqttServer").value;
  configuration.mqttServer.trim();

  configuration.mqttPort = auxConfig.getElement<AutoConnectInput>("confMqttPort").value.toInt();

  configuration.mqttUser = auxConfig.getElement<AutoConnectInput>("confMqttUser").value;
  configuration.mqttUser.trim();

  configuration.mqttPassword = auxConfig.getElement<AutoConnectInput>("confMqttPassword").value;
  configuration.mqttPassword.trim();
}

Configuration AutoConnectWrapper::getConfiguration() {
  return configuration;
}

boolean AutoConnectWrapper::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

boolean AutoConnectWrapper::isUpdating() {
  return onOTA;
}

boolean AutoConnectWrapper::isReady() {
  return isConnected() && !isUpdating() && configuration.isValid();
}

String AutoConnectWrapper::getIpAddress() {
  return ipAddress;
}

void AutoConnectWrapper::displayRoot() {
  String content = R"(
    <!DOCTYPE html>
    <html>
    <head>
      <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
    </head>
    <body>
    __CONTENT__
    </body>
    </html>
  )";
    
  String confState;
  if (configuration.isValid() && isConnected()) {
    confState = "Configured & connected";
  } else {
    confState = "Please configure and connect";
  }

  confState += "&ensp;" + String(AUTOCONNECT_LINK(COG_16)) + "<br /><br />";
  confState += "Identifier : " + configuration.identifier + "<br />";
  confState += "MQTT Server : " + configuration.mqttServer + ":" + String(configuration.mqttPort) +"<br />";
  confState += "Connected to AP : " + String(isConnected()) + "<br /><br />";
  confState += "<h1>Console</h1>";
  confState += "<code style=\"white-space: pre-line\">" + console + "</code>";
  
  content.replace("__CONTENT__", confState);
  
  server.send(200, "text/html", content);
}

void AutoConnectWrapper::onConnect(IPAddress& ipaddr) {
  String ipAddress = ipaddr.toString();
  log("WiFi connected on ");
  log(WiFi.SSID());
  log(", IP: ");
  logln(ipAddress);
}

void AutoConnectWrapper::OTAStart() {
  logln("Start OTA updating");
  onOTA = true;
}

String AutoConnectWrapper::onDisplayConfiguration(AutoConnectAux& aux, PageArgument& args) {
  if (args.hasArg("confMqttServer")) {
    saveConfiguration();
    parseConfiguration();
  }
  return String("");
}

void AutoConnectWrapper::logln(String str) {
  log(str + "\n");
}

void AutoConnectWrapper::log(String str) {
  Serial.print(str);

  console = console + str;

  if (console.length() > 1024) {
    console = "[...] " + console.substring(console.length() - 1024, console.length());
  }
}
