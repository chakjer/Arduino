bool core0init = false;
#include <SPI.h>
//Konfiguracja watchdog
#include <esp_task_wdt.h>
#define WDT_TIMEOUT 60 //60 sekund
//wewnetrzne przerwania
#include <Ticker.h>
//Konfiguracja wifi
#include <WiFi.h>
#include <WiFiClientSecure.h>
WiFiClientSecure wifiKlientSzyfrowany;
Ticker wifiTckr; // licznki do statusu wifi co 60 sekund
Ticker ntpTckr; // licznki do statusu ntp co 3600 sekund
Ticker rtcTckr; // licznki do statusu rtc co 1 sekunde
Ticker mqttTckr; // licznki do statusu mqtt co 30 sekund


//Dane odczytane z pliku wifi.conf
struct wifiKonfigPlik {
  char ssid[64];
  char haslo[64];
  char nazwaHosta[64];
};
wifiKonfigPlik wifiKonfig;

//NTP
#include <time.h>
#include <Timezone.h>
// Central European Time
TimeChangeRule LATO = {"CEST", Last, Sun, Mar, 2, 60};     // Central European Summer Time
TimeChangeRule ZIMA = {"CET ", Last, Sun, Oct, 3, 0};       // Central European Standard Time
Timezone DOM(LATO, ZIMA);
TimeChangeRule *tcr;        // pointer to the time change rule, use to get the TZ abbrev

struct ntpKonfigPlik {
  char serwer[64];
  int8_t strefa;
  int8_t korektaCzasLetni = 0;

};
ntpKonfigPlik ntpKonfig;

uint32_t czasUTC;
uint32_t czasUruchomienia;

//RTC
/*
          RTC<------------->ESP32
          SQW-------------->G23
          SCL-------------->G22
          SDA-------------->G21
          Vcc-------------->3V3
          GND-------------->GND
*/
#include <Wire.h>
#include <RTC.h>
DS3231 zegarRTC;
struct zegarRTCKonfigPlik {
  bool stan = 1;
};
zegarRTCKonfigPlik zegarRTCKonfig;
uint32_t czasLokalny;
uint32_t czasRTC = 0;
bool czasRTCBlokada = 0;

#include <ESP32Time.h>
ESP32Time rtcWewnetrzny(0);
bool rtcTyp;

//FS
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>

void spiffsKatalog(File katalog, int numTabs = 1);

//MQTT
#include <PubSubClient.h>
struct mqttKonfigPlik {
  bool stan;
  char serwer[64];
  int  port = 8883;
  bool autoryzacja;
  char uzytkownik[64];
  char haslo[64];
  char nazwa[64];

};
mqttKonfigPlik mqttKonfig;
PubSubClient mqttKlient(mqttKonfig.serwer, mqttKonfig.port, nullptr, wifiKlientSzyfrowany);

char mqttWiadomosc[64];
char mqttTematStatus[64];
uint32_t mqttCzasStatus;



//Webserver
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "www/strona_glowna.h"
#include <WebSerialLite.h>
AsyncWebServer wwwSerwer(80);


//OTA
#include <AsyncElegantOTA.h>;


//TASKI
//Core 0
TaskHandle_t uchwytCore0loop;



//MQTT
String mqttDane;


void setup() {
  //odczyt podstawowej konfiguracji
  xTaskCreatePinnedToCore(
    core0loop, /* Function to implement the task */
    "loop dla core0", /* Name of the task */
    10000,  /* Stack size in words */
    NULL,  /* Task input parameter */
    10,  /* Priority of the task */
    &uchwytCore0loop,  /* Task handle. */
    0); /* Core where the task should run */




  //Właczenie watchdog  dla core1
  esp_task_wdt_init(WDT_TIMEOUT, true);
}

void core0loop( void * parameter) {
  String core = "[Core " + String(xPortGetCoreID()) + "]";
  if (SPIFFS.begin()) {
    Serial.begin(115200);
    Serial.setDebugOutput(false);
    Serial.println("[Core " + String(xPortGetCoreID()) + "]" + "START.....");
    Serial.println(core + "[SPIFFS] Zamonotowano FS");
    File katalog = SPIFFS.open("/");
    spiffsKatalog(katalog);
    katalog.close();

    spiffsWiFiOdczytKonfiguracji();
    inicjalizacjaWiFi();
    inicjalizacjaWWW();

    spiffsNTPOdczytKonfiguracji();
    if ( ntpKonfig.serwer != "" and WiFi.status() == WL_CONNECTED and WiFi.SSID() != "InitWifi") inicjalizacjaNTP();

    spiffsZegarOdczytKonfiguracji();
    inicjalizacjaZegar();

    rtcTckr.attach(0.5, rtcSprawdzCzas);

    spiffsMQTTOdczytKonfiguracji();
    if (mqttKonfig.stan and WiFi.status() == WL_CONNECTED and WiFi.SSID() != "InitWifi") inicjalizacjaMQTT();

    AsyncElegantOTA.begin(&wwwSerwer, "admin", "test1234");

    core0init = true;

  } else {
    String core = "[Core " + String(xPortGetCoreID()) + "]";
    Serial.println(core + "[SPIFFS] Problem z zamonotowaniem FS");
    //WebSerial.println(core + "[SPIFFS] Problem z zamonotowaniem FS");
  }

  if (ntpKonfig.korektaCzasLetni) {
    czasUruchomienia = DOM.toLocal(czasUTC + ntpKonfig.strefa * 3600, &tcr);
  } else {
    czasUruchomienia = czasUTC + ntpKonfig.strefa * 3600;
  }
  //glowna petla loop
  for (;;) {
    if (mqttKonfig.stan) mqttKlient.loop();
    vTaskDelay(1000);
  }

}
//sprzetowy lub wewnetrzny
void rtcSprawdzCzas() {
  czasRTCBlokada = 1;
  if (zegarRTC.isRunning()) {
    if ( czasRTC != zegarRTC.getEpoch()) {
      String core = "[Core " + String(xPortGetCoreID()) + "]";
      czasLokalny = DOM.toLocal(zegarRTC.getEpoch() + ntpKonfig.strefa * 3600, &tcr);
      rtcTyp = false;
      //Serial.println(core + "[RTC] aktualny czas: " + String(unixTimeToHumanReadable(czasLokalny)));
    }
    rtcWewnetrzny.setTime(zegarRTC.getEpoch());
  } else {
    czasLokalny = DOM.toLocal(rtcWewnetrzny.getEpoch() + ntpKonfig.strefa * 3600, &tcr);
    rtcTyp = true;

  }
  czasRTCBlokada = 0;
}

void loop() {

  delay(1000);
}

void wifiTimerCheck() {
  polaczWiFi();
}
void polaczWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    String core = "[Core " + String(xPortGetCoreID()) + "]";
    Serial.print(core + "[WiFi] Łączenie do sieci");
    WiFi.mode(WIFI_STA);
    WiFi.hostname(wifiKonfig.nazwaHosta);
    WiFi.begin(wifiKonfig.ssid, wifiKonfig.haslo);

    for (int i = 0; i < 10; i++) {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print(core + "[WiFi] Połączono z siecią ");
        Serial.print(WiFi.SSID());
        Serial.print(" , IP ");
        Serial.println(WiFi.localIP());
        wifiKlientSzyfrowany.setInsecure();
        wifiTckr.attach(60.000, wifiTimerCheck);
        return;
      } else {
        Serial.print(".");
        delay(1000);

      }
    }
  }
}

void inicjalizacjaWiFi() {
  polaczWiFi();

  if (WiFi.status() != WL_CONNECTED) {
    String core = "[Core " + String(xPortGetCoreID()) + "]";
    const char* wifiAPssid = "InitWifi";
    const char* wifiAPhaslo = "init1234";
    Serial.println(core + " [WiFi] Problem z połączeniem do sieci");
    Serial.println(core + " [WiFi] Tworzenie AP InitWiFi");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(wifiAPssid, wifiAPhaslo);
    Serial.print("[Wifi] IP address = ");
    Serial.println(WiFi.softAPIP());
  }
}


void inicjalizacjaZegar() {
  String core = "[Core " + String(xPortGetCoreID()) + "]";
  komunikatNL(core + "[Zegar] Strefa czasowa: " + String(ntpKonfig.strefa));
  if (zegarRTCKonfig.stan) {
    komunikatNL(core + "[Zegar] RTC włączone");

    if (zegarRTC.begin()) {
      if (zegarRTC.isRunning()) komunikatNL(core + "[RTC] Zegar uruchomiony");
      else
      {
        komunikatNL(core + "[RTC] Uruchamiam zegar. Ustawiam czas!");
        zegarRTC.startClock();
      }
      komunikat(core + "[RTC] Tryb godzinowy : ");
      if (zegarRTC.getHourMode() == CLOCK_H24) komunikatNL("24 godziny");
      else
      {
        zegarRTC.setHourMode(CLOCK_H24);
        komunikatNL("12 godzin, ustawiam 24 godziny");
      }
      zegarRTC.setOutPin(SQW001Hz);
      zegarRTC.enableSqwePin();
      if (zegarRTC.getEpoch() != getTimeEpoch() and getTimeEpoch() > 0) zegarRTC.setEpoch(getTimeEpoch());
      komunikatNL(core + "[RTC] Czas zewnętrzny: " + unixTimeToHumanReadable(zegarRTC.getEpoch()));
      //rtcTckr.attach(1, rtcSprawdzCzas);
    } else komunikatNL(core + "[Zegar] RTC błąd podłaczenia");

  } else komunikatNL(core + "[Zegar] RTC wyłączone");
  rtcWewnetrzny.setTime(getTimeEpoch());
  komunikatNL(core + "[RTC] Czas wewnętrzny: " + unixTimeToHumanReadable(rtcWewnetrzny.getEpoch()));


}



void ntpTimerCheck() {
  String core = "[Core " + String(xPortGetCoreID()) + "]";
  uint32_t tempTime = getTimeEpoch();

  if (zegarRTC.getEpoch() != tempTime and tempTime > 0) {
    zegarRTC.setEpoch(tempTime);
  }

  if (rtcWewnetrzny.getEpoch() != tempTime and tempTime > 0) {
    rtcWewnetrzny.setTime(tempTime);
  }
  komunikatNL(core + "[RTC] Czas zewnętrzny: " + unixTimeToHumanReadable(zegarRTC.getEpoch()));
  komunikatNL(core + "[RTC] Czas wewnetrzny: " + unixTimeToHumanReadable(rtcWewnetrzny.getEpoch()));

}

void inicjalizacjaNTP() {
  String core = "[Core " + String(xPortGetCoreID()) + "]";
  komunikatNL(core + "[NTP] Serwer NTP: " + String(ntpKonfig.serwer));
  komunikatNL(core + "[NTP] Strefa czasowa: " + String(ntpKonfig.strefa));
  komunikatNL(core + "[NTP] Korekta czasu letniego: " + String(ntpKonfig.korektaCzasLetni));
  const char* tempNTPSerwer = ntpKonfig.serwer;
  configTime(0, 0, tempNTPSerwer);
  czasUTC = getTimeEpoch();
  rtcWewnetrzny.setTime(getTimeEpoch());

  komunikatNL(core + "[NTP] Czas UTC: " + String(unixTimeToHumanReadable(czasUTC)));
  ntpTckr.attach(3600.000, ntpTimerCheck);


}

void mqttTimerCheck() {
  if (mqttKonfig.stan) mqttStatus();

}
void inicjalizacjaMQTT() {
  String core = "[Core " + String(xPortGetCoreID()) + "]";
  komunikatNL(core + "[MQTT] Serwer MQTT: " + String(mqttKonfig.serwer) + ":" + String(mqttKonfig.port));
  //wifiKlientMQTT.setInsecure();
  mqttKlient.setServer(mqttKonfig.serwer, mqttKonfig.port);
  mqttKlient.connect(mqttKonfig.nazwa, mqttKonfig.uzytkownik, mqttKonfig.haslo);
  if (mqttKlient.connected()) {
    komunikatNL(core + "[MQTT] Połączono z serwerem MQTT");
    String mqttTemp = "statusy/esp32/" + String(mqttKonfig.nazwa);
    mqttTemp.toCharArray(mqttTematStatus, 64);
    mqttTemp = "online,ip:" + WiFi.localIP().toString();
    mqttTemp.toCharArray(mqttWiadomosc, 64);
    mqttKlient.publish(mqttTematStatus, mqttWiadomosc);

    mqttKlient.setCallback(mqttOdczytTematow);
    mqttSubskrybujTematy();

    mqttTckr.attach(30.000, mqttTimerCheck);

  } else {
    komunikatNL(core + "[MQTT] Brak połączenia z serwerem MQTT");
    komunikatNL(core + "[MQTT] Błąd: " + mqttKlient.state());

  }
}

void mqttSubskrybujTematy() {
  //teamty z mqtt do nasluchu
  char text[50];
  String tempTemat;
  String core = "[Core " + String(xPortGetCoreID()) + "]";
  komunikatNL(core + "[MQTT] Subskrybcja tematów");
  tempTemat = "sterowanie/" + String(mqttKonfig.nazwa) + "/";
  tempTemat.toCharArray(text, 50);
  mqttKlient.subscribe(text);


}

void mqttOdczytTematow(char* topic, uint8_t * payload, uint16_t length) {

  char text[50];
  payload[length] = '\0';



  komunikat("[Core " + String(xPortGetCoreID()) + "]" + "[MQTT] Temat: ");
  komunikat(topic);
  komunikat("\t\t");
  komunikatNL((char *)payload);
}

void mqttStatus() {
  String core = "[Core " + String(xPortGetCoreID()) + "]";
  if (mqttKlient.connected()) {
    komunikatNL(core + "[MQTT] Aktualizacja statusu");

    String mqttTemp = "statusy/esp32/" + String(mqttKonfig.nazwa);
    mqttTemp.toCharArray(mqttTematStatus, 64);
    mqttTemp = "online,ip:" + WiFi.localIP().toString();
    mqttTemp.toCharArray(mqttWiadomosc, 64);
    mqttKlient.publish(mqttTematStatus, mqttWiadomosc);

  } else {
    komunikatNL(core + "[MQTT] Brak połączenia z serwerem MQTT");
    if (WiFi.status() == WL_CONNECTED) inicjalizacjaMQTT();

  }
}

void inicjalizacjaWWW() {
  String core = "[Core " + String(xPortGetCoreID()) + "]";
  wwwSerwer.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", PAGE_MAIN, statusESP);
  });

  wwwSerwer.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/css", PAGE_CSS);
  });

  wwwSerwer.on("/reboot", HTTP_GET, [] (AsyncWebServerRequest * request) {
    komunikatNL("Restarting in 2 seconds");
    request->send_P(200, "text/html", PAGE_MAIN, statusESP);
    delay(2000);
    ESP.restart();
  });

  wwwSerwer.on("/ustawienia.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", PAGE_USTAWIENIA, ustawieniaESP);
  });

  wwwSerwer.on("/zapiszUstawieniaSieci", HTTP_GET, [](AsyncWebServerRequest * request) {

    uint8_t liczbaParametrow = request->params();

    for (uint8_t i = 0; i < liczbaParametrow; i++)
    {
      AsyncWebParameter* parametr = request->getParam(i);
      if ( parametr->name() == "nazwaSieci") parametr->value().toCharArray(wifiKonfig.ssid, 64);
      if ( parametr->name() == "hasloSieci") parametr->value().toCharArray(wifiKonfig.haslo, 64);
      if ( parametr->name() == "nazwaHosta") parametr->value().toCharArray(wifiKonfig.nazwaHosta, 64);
      if ( parametr->name() == "serwerNTP") parametr->value().toCharArray(ntpKonfig.serwer, 64);
      if ( parametr->name() == "serwerNTPStrefa") ntpKonfig.strefa = parametr->value().toInt();
      if ( parametr->name() == "serwerNTPCzasLetni") ntpKonfig.korektaCzasLetni = parametr->value().toInt();
      if ( parametr->name() == "rtcStan") zegarRTCKonfig.stan = parametr->value().toInt();
      if ( parametr->name() == "mqttStan") mqttKonfig.stan = parametr->value().toInt();
      if ( parametr->name() == "mqttSerwer") parametr->value().toCharArray(mqttKonfig.serwer, 64);
      if ( parametr->name() == "mqttPort") mqttKonfig.port = parametr->value().toInt();
      if ( parametr->name() == "mqttAutoryzacja") mqttKonfig.autoryzacja = parametr->value().toInt();
      if ( parametr->name() == "mqttUzytkownik") parametr->value().toCharArray(mqttKonfig.uzytkownik, 64);
      if ( parametr->name() == "mqttHaslo") parametr->value().toCharArray(mqttKonfig.haslo, 64);
      if ( parametr->name() == "mqttNazwa") parametr->value().toCharArray(mqttKonfig.nazwa, 64);

      komunikatNL(parametr->name() + ": " + parametr->value());
    }
    spiffsWiFiZapisKonfiguracji();
    spiffsNTPZapisKonfiguracji();
    spiffsMQTTZapisKonfiguracji();
    spiffsZegarZapisKonfiguracji();
    request->send_P(200, "text/html", PAGE_USTAWIENIA, ustawieniaESP);
  });

  wwwSerwer.onNotFound([](AsyncWebServerRequest * request) {
    request->send_P(404, "text/html", PAGE_404);
  });
  WebSerial.begin(&wwwSerwer);
  WebSerial.onMessage(konsolaKomenda);
  wwwSerwer.begin();
  komunikatNL(core + "[HTTP] Serwer HTTP uruchomiony");

}

String statusESP(const String & var) {
  if (var == "statusIpAdres") {
    String ipAdres;
    if (WiFi.localIP() > 0 ) {
      ipAdres = WiFi.localIP().toString();
    } else {
      ipAdres = WiFi.softAPIP().toString();
    }
    return ipAdres;
  }
  if (var == "statusNazwaSieci") {
    String nazwaSieci;
    if ( WiFi.SSID() != "" ) {
      nazwaSieci = WiFi.SSID();
    } else {
      nazwaSieci = "InitWiFi";
    }
    return nazwaSieci;
  }
  if (var == "statusGodzina") {
    String godzina = unixTimeToHumanReadable(czasUruchomienia);
    return godzina;
  }
  if (var == "statusNazwaHosta") {
    String nazwaHosta = String(wifiKonfig.nazwaHosta);
    return nazwaHosta;
  }

  return String();
}

String ustawieniaESP(const String& var) {

  if (var == "ustawieniaNazwaSieci") {
    return String(wifiKonfig.ssid);
  }
  if (var == "ustawieniaHasloSieci") {
    return  String(wifiKonfig.haslo);
  }
  if (var == "ustawieniaNazwaHosta") {
    return String(wifiKonfig.nazwaHosta);
  }
  if (var == "ustawieniaNTPSerwer") {
    return String(ntpKonfig.serwer);
  }
  if (var == "ustawieniaNTPStrefa") {
    return String(ntpKonfig.strefa);
  }
  if (var == "ustawieniaNTPCzasLetni") {
    return String(ntpKonfig.korektaCzasLetni);
  }
  if (var == "ustawieniaRTCStan") {
    return String(zegarRTCKonfig.stan);
  }
  if (var == "ustawieniaMQTTStan") {
    return String(mqttKonfig.stan);
  }
  if (var == "ustawieniaMQTTSerwer") {
    return String(mqttKonfig.serwer);
  }
  if (var == "ustawieniaMQTTPort") {
    return String(mqttKonfig.port);
  }
  if (var == "ustawieniaMQTTAutoryzacja") {
    return String(mqttKonfig.autoryzacja);
  }
  if (var == "ustawieniaMQTTUzytkownik") {
    return String(mqttKonfig.uzytkownik);
  }
  if (var == "ustawieniaMQTTHaslo") {
    return String(mqttKonfig.haslo);
  }
  if (var == "ustawieniaMQTTNazwa") {
    return String(mqttKonfig.nazwa);
  }

  return String();
}

void spiffsKatalog(File dir, int numTabs) {
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // nic w katalogu
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      spiffsKatalog(entry, numTabs + 1);
    } else {
      // nic w katalogu
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void spiffsWiFiOdczytKonfiguracji() {
  String core = "[Core " + String(xPortGetCoreID()) + "]";
  Serial.println(core + "[SPIFFS] Odczyt konfiguracji wifi");
  File plik = SPIFFS.open("/wifi.conf", "r");
  StaticJsonDocument<256> jsonDok;
  DeserializationError error = deserializeJson(jsonDok, plik);
  if (error)
    Serial.println(core + "[SPIFFS] Błąd odczytu pliku konfiguracyjnego Wifi");
  if (jsonDok.containsKey("ssid")) strlcpy(wifiKonfig.ssid, jsonDok["ssid"], sizeof(wifiKonfig.ssid));
  if (jsonDok.containsKey("haslo")) strlcpy(wifiKonfig.haslo, jsonDok["haslo"], sizeof(wifiKonfig.haslo));
  if (jsonDok.containsKey("nazwaHosta")) strlcpy(wifiKonfig.nazwaHosta, jsonDok["nazwaHosta"], sizeof(wifiKonfig.nazwaHosta));
  plik.close();

}
void spiffsWiFiZapisKonfiguracji() {
  String core = "[Core " + String(xPortGetCoreID()) + "]";
  komunikatNL(core + "[SPIFFS] Zapis konfiguracji wifi");
  File plik = SPIFFS.open("/wifi.conf", "w");
  if (!plik) {
    komunikatNL(core + "[SPIFFS] Błąd kasowania pliku konfiguracyjnego wifi");
    return;
  }
  StaticJsonDocument<256> jsonDok;
  jsonDok["ssid"] = wifiKonfig.ssid;
  jsonDok["haslo"] = wifiKonfig.haslo;
  jsonDok["nazwaHosta"] = wifiKonfig.nazwaHosta;
  if (serializeJson(jsonDok, plik) == 0) {
    komunikatNL((core + "[SPIFFS] Błąd zapisu pliku konfiguracyjnego wifi"));
  }
  plik.close();
  komunikatNL(core + "[Ustawienia] Sieć wifi: " + String(wifiKonfig.ssid));
  komunikatNL(core + "[Ustawienia] Hasło wifi: " + String(wifiKonfig.haslo));
  komunikatNL(core + "[Ustawienia] Nazwa hosta: " + String(wifiKonfig.nazwaHosta));
}

void spiffsNTPOdczytKonfiguracji() {
  String core = "[Core " + String(xPortGetCoreID()) + "]";
  komunikatNL(core + "[SPIFFS] Odczyt konfiguracji ntp");
  File plik = SPIFFS.open("/ntp.conf", "r");
  StaticJsonDocument<256> jsonDok;
  DeserializationError error = deserializeJson(jsonDok, plik);
  if (error) {
    komunikatNL(core + "[SPIFFS] Błąd odczytu pliku konfiguracyjnego ntp");
  }
  if (jsonDok.containsKey("serwer")) strlcpy(ntpKonfig.serwer, jsonDok["serwer"], sizeof(ntpKonfig.serwer));
  if (jsonDok.containsKey("strefa")) ntpKonfig.strefa = jsonDok["strefa"];
  if (jsonDok.containsKey("korektaCzasLetni")) ntpKonfig.korektaCzasLetni = jsonDok["korektaCzasLetni"];
  plik.close();
}
void spiffsNTPZapisKonfiguracji() {
  String core = "[Core " + String(xPortGetCoreID()) + "]";
  komunikatNL(core + "[SPIFFS] Zapis konfiguracji ntp");
  File plik = SPIFFS.open("/ntp.conf", "w");
  if (!plik) {
    komunikatNL(core + "[SPIFFS] Błąd kasowania pliku konfiguracyjnego ntp");
    return;
  }
  StaticJsonDocument<256> jsonDok;
  jsonDok["serwer"] = ntpKonfig.serwer;
  jsonDok["strefa"] = ntpKonfig.strefa;
  jsonDok["korektaCzasLetni"] = ntpKonfig.korektaCzasLetni;
  if (serializeJson(jsonDok, plik) == 0) {
    komunikatNL(core + "[SPIFFS] Błąd zapisu pliku konfiguracyjnego ntp");
  }
  plik.close();
  komunikatNL(core + "[Ustawienia] Serwer NTP: " + String(ntpKonfig.serwer));
  komunikatNL(core + "[Ustawienia] Serwer NTP strefa: " + String(ntpKonfig.strefa));
  komunikatNL(core + "[Ustawienia] Serwer NTP korekta czasu letniego: " + String(ntpKonfig.korektaCzasLetni));

}

void spiffsMQTTOdczytKonfiguracji() {
  String core = "[Core " + String(xPortGetCoreID()) + "]";
  if (SPIFFS.exists("/mqtt.conf")) {
    komunikatNL(core + "[SPIFFS] Odczyt konfiguracji mqtt");
    File plik = SPIFFS.open("/mqtt.conf", "r");
    StaticJsonDocument<256> jsonDok;
    DeserializationError error = deserializeJson(jsonDok, plik);
    if (error) {
      komunikatNL(core + "[SPIFFS] Błąd odczytu pliku konfiguracyjnego mqtt");
    }
    if (jsonDok.containsKey("stan")) mqttKonfig.stan = jsonDok["stan"];
    if (jsonDok.containsKey("serwer")) strlcpy(mqttKonfig.serwer, jsonDok["serwer"], sizeof(mqttKonfig.serwer));
    if (jsonDok.containsKey("port")) mqttKonfig.port = jsonDok["port"];
    if (jsonDok.containsKey("autoryzacja")) mqttKonfig.autoryzacja = jsonDok["autoryzacja"];
    if (jsonDok.containsKey("uzytkownik")) strlcpy(mqttKonfig.uzytkownik, jsonDok["uzytkownik"], sizeof(mqttKonfig.uzytkownik));
    if (jsonDok.containsKey("haslo")) strlcpy(mqttKonfig.haslo, jsonDok["haslo"], sizeof(mqttKonfig.haslo));
    if (jsonDok.containsKey("nazwa")) strlcpy(mqttKonfig.nazwa, jsonDok["nazwa"], sizeof(mqttKonfig.nazwa));

    plik.close();
  } else {
    komunikatNL(core + "[SPIFFS] Brak pliku mqtt.conf");
  }
}

void spiffsMQTTZapisKonfiguracji() {
  String core = "[Core " + String(xPortGetCoreID()) + "]";
  komunikatNL(core + "[SPIFFS] Zapis konfiguracji mqtt");
  File plik = SPIFFS.open("/mqtt.conf", "w");
  if (!plik) {
    komunikatNL(core + "[SPIFFS] Błąd kasowania pliku konfiguracyjnego mqtt");
    return;
  }
  StaticJsonDocument<256> jsonDok;
  jsonDok["stan"] = mqttKonfig.stan;
  jsonDok["serwer"] = mqttKonfig.serwer;
  jsonDok["port"] = mqttKonfig.port;
  jsonDok["autoryzacja"] = mqttKonfig.autoryzacja;
  jsonDok["uzytkownik"] = mqttKonfig.uzytkownik;
  jsonDok["haslo"] = mqttKonfig.haslo;
  jsonDok["nazwa"] = mqttKonfig.nazwa;

  if (serializeJson(jsonDok, plik) == 0) {
    komunikatNL(core + "[SPIFFS] Błąd zapisu pliku konfiguracyjnego mqtt");
  }
  plik.close();
  komunikatNL(core + "[Ustawienia] Serwer MQTT stan: " + String(mqttKonfig.stan));
  komunikatNL(core + "[Ustawienia] Serwer MQTT serwer: " + String(mqttKonfig.serwer));
  komunikatNL(core + "[Ustawienia] Serwer MQTT port: " + String(mqttKonfig.port));
  komunikatNL(core + "[Ustawienia] Serwer MQTT autoryzacja: " + String(mqttKonfig.autoryzacja));
  komunikatNL(core + "[Ustawienia] Serwer MQTT uzytkownik: " + String(mqttKonfig.uzytkownik));
  komunikatNL(core + "[Ustawienia] Serwer MQTT haslo: " + String(mqttKonfig.haslo));
  komunikatNL(core + "[Ustawienia] Serwer MQTT nazwa: " + String(mqttKonfig.nazwa));

}

void spiffsZegarOdczytKonfiguracji() {
  String core = "[Core " + String(xPortGetCoreID()) + "]";
  if (SPIFFS.exists("/zegar.conf")) {
    komunikatNL(core + "[SPIFFS] Odczyt konfiguracji zegara");
    File plik = SPIFFS.open("/zegar.conf", "r");
    StaticJsonDocument<256> jsonDok;
    DeserializationError error = deserializeJson(jsonDok, plik);
    if (error) {
      komunikatNL(core + "[SPIFFS] Błąd odczytu pliku konfiguracyjnego zegara");
    }
    if (jsonDok.containsKey("stan")) zegarRTCKonfig.stan = jsonDok["stan"];


    plik.close();

  } else {
    komunikatNL(core + "[SPIFFS] Brak pliku zegar.conf");
  }
}

void spiffsZegarZapisKonfiguracji() {
  String core = "[Core " + String(xPortGetCoreID()) + "]";
  komunikatNL(core + "[SPIFFS] Zapis konfiguracji zegara");
  File plik = SPIFFS.open("/zegar.conf", "w");
  if (!plik) {
    komunikatNL(core + "[SPIFFS] Błąd kasowania pliku konfiguracyjnego zegara");
    return;
  }
  StaticJsonDocument<256> jsonDok;
  jsonDok["stan"] = zegarRTCKonfig.stan;
  if (serializeJson(jsonDok, plik) == 0) {
    komunikatNL(core + "[SPIFFS] Błąd zapisu pliku konfiguracyjnego zegara");
  }
  plik.close();
  komunikatNL(core + "[Ustawienia] Zegar włączone RTC: " + String(zegarRTCKonfig.stan));
}

void konsolaKomenda(uint8_t *data, size_t len) {
  String instrukcja = "";
  for (int i = 0; i < len; i++) {
    instrukcja += char(data[i]);
  }
  WebSerial.println(instrukcja);
  if ( instrukcja == "reboot") {
    WebSerial.println("Restart za 1 sekundę");
    delay(5000);
    ESP.restart();
  }
  if ( instrukcja == "status") {
    WebSerial.println("Nazwa hosta:\t" + String(wifiKonfig.nazwaHosta));
    String nazwaSieci, ipAdres;
    if ( WiFi.SSID() != "" ) {
      nazwaSieci = WiFi.SSID();
    } else {
      nazwaSieci = "InitWiFi";
    }
    if (WiFi.localIP() > 0 ) {
      ipAdres = WiFi.localIP().toString();
    } else {
      ipAdres = WiFi.softAPIP().toString();
    }
    WebSerial.print("Nazwa sieci:\t");
    WebSerial.println(nazwaSieci);
    WebSerial.print("Adres IP:\t");
    WebSerial.println(ipAdres);
   if (rtcTyp) {
      WebSerial.println("Typ rtc:\twewnętrzny");
    } else {
      WebSerial.println("Typ rtc:\tzewnętrzny");
    }
    WebSerial.println("Aktualny czas:\t" + unixTimeToHumanReadable(czasLokalny));
    WebSerial.println("Uruchomino:\t" + unixTimeToHumanReadable(czasUruchomienia));
    if (mqttKlient.connected()) {
      WebSerial.println("MQTT:\t\tpołączony z serwerem");
    } else {
      WebSerial.println("MQTT:\t\tnie połączony z serwerem");
    }


  }
  if ( instrukcja == "epoch") {
    WebSerial.println("Zewnętrzny RTC: " + String(zegarRTC.getEpoch()));
    WebSerial.println("Wewnetrzny RTC: " + String(rtcWewnetrzny.getEpoch()));
  }
}
void komunikat(String x) {
  Serial.print(x);
  WebSerial.print(x);

}
void komunikatNL(String x) {
  Serial.println(x);
  WebSerial.println(x);
}
//--------testowe

//https://www.geeksforgeeks.org/convert-unix-timestamp-to-dd-mm-yyyy-hhmmss-format/
String unixTimeToHumanReadable(int32_t seconds)
{

  // Save the time in Human
  // readable format
  String ans = "";

  // Number of days in month
  // in normal year
  int16_t daysOfMonth[] = { 31, 28, 31, 30, 31, 30,
                            31, 31, 30, 31, 30, 31
                          };

  int32_t currYear, daysTillNow, extraTime,
          extraDays, index, date, month, hours,
          minutes, secondss, flag = 0;

  // Calculate total days unix time T
  daysTillNow = seconds / (24 * 60 * 60);
  extraTime = seconds % (24 * 60 * 60);
  currYear = 1970;

  // Calculating current year
  while (daysTillNow >= 365) {
    if (currYear % 400 == 0
        || (currYear % 4 == 0
            && currYear % 100 != 0)) {
      daysTillNow -= 366;
    }
    else {
      daysTillNow -= 365;
    }
    currYear += 1;
  }

  // Updating extradays because it
  // will give days till previous day
  // and we have include current day
  extraDays = daysTillNow + 1;

  if (currYear % 400 == 0
      || (currYear % 4 == 0
          && currYear % 100 != 0))
    flag = 1;

  // Calculating MONTH and DATE
  month = 0, index = 0;
  if (flag == 1) {
    while (true) {

      if (index == 1) {
        if (extraDays - 29 < 0)
          break;
        month += 1;
        extraDays -= 29;
      }
      else {
        if (extraDays
            - daysOfMonth[index]
            < 0) {
          break;
        }
        month += 1;
        extraDays -= daysOfMonth[index];
      }
      index += 1;
    }
  }
  else {
    while (true) {

      if (extraDays
          - daysOfMonth[index]
          < 0) {
        break;
      }
      month += 1;
      extraDays -= daysOfMonth[index];
      index += 1;
    }
  }

  // Current Month
  if (extraDays > 0) {
    month += 1;
    date = extraDays;
  }
  else {
    if (month == 2 && flag == 1)
      date = 29;
    else {
      date = daysOfMonth[month - 1];
    }
  }

  // Calculating HH:MM:YYYY
  hours = extraTime / 3600;
  minutes = (extraTime % 3600) / 60;
  secondss = (extraTime % 3600) % 60;

  ans += String(currYear);
  ans += "/";
  if (month < 10) {
    ans += String(0);
  }
  ans += String(month);
  ans += "/";
  if (date < 10) {
    ans += String(0);
  }
  ans += String(date);
  ans += " ";
  if (hours < 10) {
    ans += String(0);
  }
  ans += String(hours);
  ans += ":";
  if (minutes < 10) {
    ans += String(0);
  }
  ans += String(minutes);
  ans += ":";
  if (secondss < 10) {
    ans += String(0);
  }
  ans += String(secondss);

  // Return the time
  return ans;
}

//https://randomnerdtutorials.com/epoch-unix-time-esp32-arduino/
uint32_t getTimeEpoch() {
  String core = "[Core " + String(xPortGetCoreID()) + "]";

  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    komunikatNL(core + "[NTP] Błąd synchronizacji czasu z serwerem");
    return (0);
  }
  time(&now);
  return now;
}
