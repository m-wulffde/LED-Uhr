#include <Arduino.h>
#include <ESP8266WiFi.h>          // https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager
#include <NTPClient.h>            // arduino-libraries/NTPClient @ 3.1.0
#include <Time.h>
#include <TimeLib.h>              // paulstoffregen/Time @ 1.6
#include <Adafruit_NeoPixel.h>    // adafruit/Adafruit NeoPixel @ 1.8.2


//************* Datenstrukturen ******************************
// Struktur für LED-RGB-Informationen
struct RGB {
  byte r, g, b;
};

// Struktur für Zeitinformationen
struct TIME {
  byte Hour, Minute;
};

                                  // Uhr Hintergrund Farben
const RGB U_Z  = { 64,  0, 64 };  // 12                 128,0,128  Lila
const RGB U_V  = { 32,  0, 20 };  // 3 6 9              64, 0, 40  Lila
const RGB U_St = {  8,  0,  5 };  // 1,2,4,5,7,8,10,11  32, 0, 20  Lila
const RGB U_HG = {  0,  0,  1 };  // Hintergrungfarbe

                                  // Uhr Zeiger Farben
const RGB Z_St = {  0,128,  0 };  // Stunden
const RGB Z_M  = { 64, 32,  0 };  // Minuten
const RGB Z_S  = {  2,  2,  2 };  // Sekunden

                                  // Tage
const RGB T_HG = {  0,  0,  1 };  // Hintergrungfarbe
const RGB T_P  = {  4,  1,  2 };  // 05 10 15 30
const RGB TT_P = {  8,  1,  5};   // 10 20 30
const RGB T_Z =  {  0,  5,  2};   // 10 20 30
const RGB T_ZZ = {  0,  7,  0};   // 10 20 30

                                  // Monate
const RGB M_HG = { 0, 0, 1 };     // Hintergrungfarbe
const RGB M_P  = { 4, 1, 2 };     // 1 3 6 9
const RGB M_Z  = { 0, 5, 2 };     // Zeiger
                               
const int hours_Offset_From_GMT = 1;  // Der Stunden Aufschlag zu GMT

// WLAN Verbindungsparameter Passwort - sollte nicht mehr benötigt werden.
// const char *ssid      = "Mein-WLAN";
// const char *password  = "Geheim"; 

byte SetClock;
// Standardmäßig wird 'time.nist.gov' verwendet.
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Datenkanal für den NeoPixel 
#define PIN 14 // das ist der D5 Pin

//************* Declarationen der Funktionen ******************************
void Pixel_Reset(byte r, byte g, byte b);
void Pixel_Uhr(time_t t);
void Pixel_Datum(time_t t);
void SetClockFromNTP ();
bool IsDst();

//************* NeoPixel ******************************
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(104, PIN, NEO_GRB + NEO_KHZ800);  

//**************************************************
//************* Setup ******************************
void setup() {
  Serial.begin(115200);       // Fehlersuche ermöglichen
  pixels.begin();             // NeoPixel-Bibliothek initialisiert.
  Pixel_Reset(1,0,0);         // rot (Start)

  // ************************* WiFiManager Start
  // Lokale Initialisierung. Sobald das Geschäft erledigt ist, besteht keine Notwendigkeit, es in der Nähe zu behalten
  WiFiManager wifiManager;
  //reset saved settings
  //wifiManager.resetSettings();
  
  // Benutzerdefinierte IP für das Portal festlegen
  //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  // holt SSID und Password vom EEPROM und versucht eine Verbindung herzustellen
  // wenn es keine Verbindung herstellt, startet es einen Zugangspunkt mit dem angegebenen Namen
  // hier "LED-Uhr"
  // und geht in eine Sperrschleife, die auf die Konfiguration wartet
  wifiManager.autoConnect("LED-Uhr"); // AutoConnectAP
  // oder verwenden Sie dies für den automatisch generierten Namen ESP + ChipID
  //wifiManager.autoConnect();

  // Wenn du hier ankommst, hast du dich mit dem WLAN verbunden
  Serial.println("connected...yeey :)");
  // ************************* WiFiManager Ende
  // WiFi.begin(ssid, password); // mit WLAN verbinden
  Pixel_Reset(0,1,0);         // grün (Start)
  SetClockFromNTP();          // Zeit hohlen vom NTP-Server mit Zeitzonen-Korrektur
  Pixel_Reset(0,0,0);         // aus
  // +++++++++++++++++++++++++++++++++++++++++++++++++++
}

//************* NTP-Server Zeit *******************
void SetClockFromNTP ()
{
  timeClient.update();                              // Uhrzeit vom NTP-Server hohlen
  setTime(timeClient.getEpochTime());               // Systemzeit von der Uhr einstellen
  if (IsDst())
    adjustTime((hours_Offset_From_GMT + 1) * 3600); // die Systemzeit mit der benutzerdefinierten Zeitzone einstellen (3600 Sekunden in einer Stunden)
  else
    adjustTime(hours_Offset_From_GMT * 3600);       // Die Systemzeit mit der benutzerdefinierten Zeitzone (3600 Sekunden in einer Stunden) ausgegleichen.
}

bool IsDst()                                        // Sommerzeit 
{
  if (month() < 3 || month() > 10)  return false; 
  if (month() > 3 && month() < 10)  return true; 
  int previousSunday = day() - weekday();
  if (month() == 3) return previousSunday >= 24;
  if (month() == 10) return previousSunday < 24;  
  return false;                                     // Mist, Fehler
}

//************* Pixel_Reset ******************************
// Alle Pixel bekommen die gleiche Farbe
void Pixel_Reset(byte r,byte g,byte b)
{
  if (r == 0 and g == 0 and b == 0) {
    for (int i = 0; i < 104; i++) {
      pixels.setPixelColor(i, i, i/5, i/10 );
      pixels.show();
      delay (25);
    }
     for (int i = 0; i < 104; i++) {
      pixels.setPixelColor(i, i/10, i/5, i );
      pixels.show();
      delay (25);
    }
    for (int i = 0; i < 104; i++) {
      pixels.setPixelColor(i, i/10, i, i/5 );
      pixels.show();
      delay (25);
    }
  }
  
  for (int i = 0; i < 104; i++)                     // Alle Pixel die übergebene Farbe
    pixels.setPixelColor(i, r, g, b);
  pixels.show();
}

//************* Uhr Ausgeben ******************************
// den gesamte 60er Ring ausgeben
void Pixel_Uhr(time_t t){                   
  for (int i = 0; i < 60; i++)
    pixels.setPixelColor(i, pixels.Color(U_HG.r, U_HG.g, U_HG.b));        // Hintergrund Uhr
  for (int i = 0; i < 60; i = i + 5)
    pixels.setPixelColor(i, pixels.Color(U_St.r, U_St.g, U_St.b));        // Stunden Punkt
  for (int i = 0; i < 60; i = i + 15)
    pixels.setPixelColor(i, pixels.Color(U_V.r, U_V.g, U_V.b));           // Viertelstunde Punkt
  pixels.setPixelColor(0, pixels.Color(U_Z.r, U_Z.g, U_Z.b));             // die 12 Punkt
                                                                          // Zeiger Minuten 
  if (minute(t) > 0)
    pixels.setPixelColor(minute(t)-1, pixels.Color(Z_M.r/10, Z_M.g/10, Z_M.b/10));
  else
    pixels.setPixelColor(minute(t)+59, pixels.Color(Z_M.r/10, Z_M.g/10, Z_M.b/10));                                                                                 
  if (minute(t) < 60)
    pixels.setPixelColor(minute(t)+1, pixels.Color(Z_M.r/10, Z_M.g/10, Z_M.b/10));                                                                                  
                                                                          // Zeiger Stunden
  if ((((hour(t) % 12) * 5) + minute(t) / 12) > 0)
    pixels.setPixelColor(((hour(t) % 12) * 5) + minute(t) / 12 -1, pixels.Color(Z_St.r/10, Z_St.g/10, Z_St.b/10));
  pixels.setPixelColor(((hour(t) % 12) * 5) + minute(t) / 12, pixels.Color(Z_St.r, Z_St.g, Z_St.b));  // Volle Leistung Zeiger Stunden
  if ((((hour(t) % 12) * 5) + minute(t) / 12) < 60)
    pixels.setPixelColor(((hour(t) % 12) * 5) + minute(t) / 12 +1, pixels.Color(Z_St.r/10, Z_St.g/10, Z_St.b/10)); 

  pixels.setPixelColor(minute(t), pixels.Color(Z_M.r, Z_M.g, Z_M.b));     // Volle Leistung Zeiger Minuten 

  pixels.setPixelColor(second(t), pixels.Color(Z_S.r, Z_S.g, Z_S.b));     // Zeiger Sekunden
  
  pixels.show(); // Alle Pixel anzeigen
}
//************* Datum Ausgeben ******************************
void Pixel_Datum(time_t t){                                               // ### Tag
  for (int i = 60; i < 92; i++)
    pixels.setPixelColor(i, pixels.Color(T_HG.r, T_HG.g, T_HG.b));        // Hintergrund Tage
  for (int i = 60; i < 92; i = i + 5)
    pixels.setPixelColor(i, pixels.Color(T_P.r, T_P.g, T_P.b));           // alle 5 Tage Punkt
  for (int i = 60; i < 92; i = i + 10)
    pixels.setPixelColor(i, pixels.Color(TT_P.r, TT_P.g, TT_P.b));        // alle 10 Tage Punkt
                                                                          // Zeiger
  for (int i = 60; i < 92; i++)
    if (i <= day() + 60)
      pixels.setPixelColor(i, pixels.Color(T_Z.r, T_Z.g, T_Z.b));
  for (int i = 60; i < 92; i = i + 5)
    if (i <= day() + 60)
      pixels.setPixelColor(i, pixels.Color(T_ZZ.r, T_ZZ.g, T_ZZ.b));      // alle 5 Tage Zeiger Punkt    

  pixels.setPixelColor(60, pixels.Color(TT_P.r, TT_P.g, TT_P.b));
  
                                                                          // ### Monat
  for (int i = 92; i < 104; i++)
    pixels.setPixelColor(i, pixels.Color(M_HG.r, M_HG.g, M_HG.b));        // Hintergrund Monate
  for (int i = 92; i < 104; i = i + 3)
    pixels.setPixelColor(i, pixels.Color(M_P.r, M_P.g, M_P.b));           // alle 3 Monate
                                                                          // Zeiger
  for (int i = 92; i < 104; i++)
    if (i <= month() + 92)
      pixels.setPixelColor(i, pixels.Color(M_Z.r, M_Z.g, M_Z.b));
  if ( month() == 12 )
    pixels.setPixelColor(92, pixels.Color(M_Z.r, M_Z.g, M_Z.b));
  else
    pixels.setPixelColor(92, pixels.Color(M_P.r, M_P.g, M_P.b)); 
}


//***************************************************
//************* Hauptprogramm Schleife  *************
void loop() {
  time_t t = now();       // Holt sich die aktuelle Uhrzeit
  Pixel_Datum(t);         // eventuell nicht ganz passend
  Pixel_Uhr(t);
  Serial.println(month()); // Debug Ausgaben
  delay(400);             // 0,8 Sekunden warten
  if (minute(t) == 0) {   // jeder Stunden die Uhrzeit vom Zeitserver hohlen und aktualisieren
    SetClockFromNTP();    // Holt sich die Zeit vom NTP-Server mit Zeitzonen-Korrektur
    delay(400);           // 0,8 Sekunden warten
    if (second(t) <= 1)   // Nur einmal das Lichtspiel
      Pixel_Reset(0,0,0); // Lichtspiel, aus
  }
}