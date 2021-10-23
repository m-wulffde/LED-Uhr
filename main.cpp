#include <Arduino.h>
#include <ESP8266WiFi.h>          // https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
// #include <ESP8266WebServer.h>
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
                                  const RGB U_HG = {  0,  0,  1};   // Hintergrungfarbe
const RGB U_Z  = { 64,  0, 64};   // 12                 128,0,128  Lila
const RGB U_V  = { 32,  0, 20};   // 3 6 9              64, 0, 40  Lila
const RGB U_St = {  8,  0,  5};   // 1,2,4,5,7,8,10,11  Stunde
const RGB U_St_D = {0,  0,  1};   // 1,2,4,5,7,8,10,11  Stunde im dunkeln

                                  // Uhr Zeiger Farben
const RGB Z_St = {128,  0,  0};   // Stunden
const RGB Z_M  = { 64, 15,  0};   // Minuten
const RGB Z_S  = {  2,  2,  2};   // Sekunden

                                  // Tage
const RGB T_HG = {  0,  0,  1};   // Hintergrungfarbe
const RGB T_P  = {  4,  1,  2};   // 4, 1, 2 | jeden  5 Tag
const RGB T_PP = { 12,  3,  6};   // 10 20 30| jeden 10 Tag
const RGB T_A =  {  3,  1,  0};   // 0, 5, 2 | Alle Tage
const RGB T_T =  {  9,  3,  0};   //         | Tag

                                  // Monate
const RGB M_HG = { 0, 0, 1 };     // 0, 0, 1 | Hintergrungfarbe
const RGB M_P  = { 4, 1, 2 };     // 4, 1, 2 | Kennzeichnung Monat 1 3 6 9 
const RGB M_A  = { 3, 1, 0 };     // 0, 5, 2 | Alle Monate
const RGB M_M  = { 9, 3, 0 };     // 0, 5, 2 | Monate

const boolean TMF = true;         // Alle Tage / Monate und füllen
const boolean Lichtspiel = true; // Lichtspiel ja/nein true/false

const int hours_Offset_From_GMT = 1; // Der Stunden Aufschlag zu GMT

byte SetClock;
// Standardmäßig wird 'time.nist.gov' verwendet.
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Datenkanal für den NeoPixel 
#define PIN 14 // das ist der D5 Pin

// Taster
byte Taster1 = 12; // GPIO12 | D6
byte Taster2 = 13; // GPIO13 | D7
byte Taster3 = 16; // GPIO16 | D0


// Lichtsensor
#define Lichtsensor A0
const short Licht_HD = 50;            // beginn des Dunkelwert
short HD_Wert = 50;                   // Lichtwert von den letzten 9 Messungen.
byte HD = 1;                          // Letzte Messung Hell/Dunkel

//************* Declarationen der Funktionen ******************************
void Pixel_Reset(byte r, byte g, byte b);
void Pixel_Uhr(time_t t);
void Pixel_Datum(time_t t);
void SetClockFromNTP ();
void Pixel_Helligkeit();              // void Pixel_Helligkeit(byte Lichtwert);
bool IsDst();                         // Sommerzeit 

//************* NeoPixel ******************************
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(104, PIN, NEO_GRB + NEO_KHZ800);  

//**************************************************
//*************** Setup ****************************
void setup() {
  pinMode(Taster1, INPUT);      // Taster IO festlegen
  pinMode(Taster2, INPUT);      // Taster IO festlegen
  pinMode(Taster3, INPUT);      // Taster IO festlegen
  pinMode(Lichtsensor, INPUT);  // Lichtsensor IO festlegen

  Serial.begin(9600);           // Fehlersuche ermöglichen 9600/ 115200
  Serial.println("Digitale-Uhr www.M-Wulff.de");
  pixels.begin();               // NeoPixel-Bibliothek initialisiert.
  // pixels.setBrightness(200);    80 = 1/3 Helligkeit verändert auch die direckten Werte.
  Pixel_Reset(1,0,0);           // rot (Start)

  // ************************* WiFiManager Start
  // Lokale Initialisierung. Sobald das Geschäft erledigt ist, besteht keine Notwendigkeit, es in der Nähe zu behalten
  WiFiManager wifiManager;
  //reset saved settings
  wifiManager.resetSettings();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(600);

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
  Serial.println("WLAN Verbunden...yeey :)");
  // ************************* WiFiManager Ende
  
  Pixel_Reset(0,1,0);         // grün (Start)
  SetClockFromNTP();          // Zeit hohlen vom NTP-Server mit Zeitzonen-Korrektur
  if (Lichtspiel)
    Pixel_Reset(0,0,0);       // Lichtspiel
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
    for (byte i = 0; i < 104; i++) {
      pixels.setPixelColor(i, i, i/5, i/10 );
      pixels.show();
      delay (25);
    }
     for (byte i = 0; i < 104; i++) {
      pixels.setPixelColor(i, i/10, i/5, i );
      pixels.show();
      delay (25);
    }
    for (byte i = 0; i < 104; i++) {
      pixels.setPixelColor(i, i/10, i, i/5 );
      pixels.show();
      delay (25);
    }
  }
  
  for (byte i = 0; i < 104; i++)                     // Alle Pixel die übergebene Farbe
    pixels.setPixelColor(i, r, g, b);
  pixels.show();
}

//** Funktion für Regenbogenfarben um RGB Farben zu erzeugen **
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

//********** Funktion für Regenbogenfarben *****************
void rainbowCycle() {
  uint16_t i, j;
  pixels.setBrightness(10);                                         // Ausgabe auf niedrige Helligkeit 
  for(j=0; j<600; j++) {                                            // Durchläufe
    for(i=92; i< 104; i++)                                          // Monats Ring
      pixels.setPixelColor(i, Wheel(((i * 256 / 12) + j) & 255));   
    for(i=60; i< 92; i++)                                           // Tages Ring
      pixels.setPixelColor(i, Wheel(((i * 256 / 32) + j) & 255));   
    for(i=0; i< 60; i++)                                            // Minuten Ring
      pixels.setPixelColor(i, Wheel(((i * 256 / 60) + j) & 255)); 
    pixels.show();
    delay(10);
  }
  pixels.setBrightness(255);                                        // Ausgabe auf volle Helligkeit
}

//************* Pixel Helligkeit setzen ********************
// Pixel für Dunkel oder Hell-Betrieb setzen
void Pixel_Helligkeit(){
  float H_Wert = analogRead(Lichtsensor);     // lesen der Lichtstärke
  HD_Wert = (HD_Wert * 9 + H_Wert) / 10;
  if (HD << 1)                                               
    if (Licht_HD <= HD_Wert * 0,8)            // Dunkel
      HD = 1;
  if (HD == 1)                                                
    if (Licht_HD > HD_Wert * 1.2)             // Hell
      HD = 12; 
                                              // Werte mal ausgeben
  Serial.print("Licht_HD "); Serial.print(Licht_HD); Serial.print(", H_Wert "); Serial.print(H_Wert);  Serial.print(", HD_Wert = "); Serial.print(HD_Wert); Serial.print(", Dunkel = "); Serial.println(HD); 
}

//********************* Uhr Ausgeben **********************
//********** den 60er 32er und 12 Ring ausgeben ***********

void Pixel_Uhr(time_t t){
  for (int i = 0; i < 60; i++)                                            //###### Uhr
    pixels.setPixelColor(i, pixels.Color(U_HG.r/HD, U_HG.g/HD, U_HG.b/HD));     // Hintergrund Uhr
  for (int i = 0; i < 60; i = i + 5)
    if (HD == 1)
      pixels.setPixelColor(i, pixels.Color(U_St.r, U_St.g, U_St.b));            // Stunden (hell)
    else
      pixels.setPixelColor(i, pixels.Color(U_St_D.r, U_St_D.g, U_St_D.b));      // Stunde (dunkel)
  for (int i = 0; i < 60; i = i + 15)
    if (HD == 1)
      pixels.setPixelColor(i, pixels.Color(U_V.r/HD, U_V.g/HD, U_V.b/HD));      // Viertelstunde Punkt
 
                                                                          //###### Zeiger Minuten 
  if (minute(t) > 0)
    pixels.setPixelColor(minute(t)-1, pixels.Color(Z_M.r/10/HD, Z_M.g/10/HD, Z_M.b/10/HD));
  else
    pixels.setPixelColor(minute(t)+59, pixels.Color(Z_M.r/10/HD, Z_M.g/10/HD, Z_M.b/10/HD));                                                                                 
  if (minute(t) < 60)
    pixels.setPixelColor(minute(t)+1, pixels.Color(Z_M.r/10/HD, Z_M.g/10/HD, Z_M.b/10/HD));

                                                                          //###### Zeiger Stunden
  if ((((hour(t) % 12) * 5) + minute(t) / 12) > 0)
    pixels.setPixelColor(((hour(t) % 12) * 5) + minute(t) / 12 -1, pixels.Color(Z_St.r/10/HD, Z_St.g/10/HD, Z_St.b/10/HD));
                                                                                // Volle Leistung Zeiger Stunden
  pixels.setPixelColor(((hour(t) % 12) * 5) + minute(t) / 12, pixels.Color(Z_St.r/HD, Z_St.g/HD, Z_St.b/HD));
  if ((((hour(t) % 12) * 5) + minute(t) / 12) < 60)
    pixels.setPixelColor(((hour(t) % 12) * 5) + minute(t) / 12 +1, pixels.Color(Z_St.r/10/HD, Z_St.g/10/HD, Z_St.b/10/HD)); 

  pixels.setPixelColor(minute(t), pixels.Color(Z_M.r/HD, Z_M.g/HD, Z_M.b/HD));  // Volle Leistung Zeiger Minuten 

  if (HD == 1)
    pixels.setPixelColor(second(t), pixels.Color(Z_S.r, Z_S.g, Z_S.b));         // Zeiger Sekunden

                                                                        //######## Tag
  if (HD == 1){
    for (int i = 60; i < 92; i++)
      pixels.setPixelColor(i, pixels.Color(T_HG.r, T_HG.g, T_HG.b));            // Hintergrund Tage
    if (TMF)
      for (int i = 60; i < 92; i++)                                   
        if (i <= day() + 60)
          pixels.setPixelColor(i, pixels.Color(T_A.r, T_A.g, T_A.b));           // alle abgelaufenen Tage
    for (int i = 60; i < 92; i = i + 5)
      pixels.setPixelColor(i, pixels.Color(T_P.r, T_P.g, T_P.b));               // jeden 5  Tage kennzeichnen
    for (int i = 60; i < 92; i = i + 10)
      pixels.setPixelColor(i, pixels.Color(T_PP.r, T_PP.g, T_PP.b));            // jeden 10 Tage kennzeichnen
   
    pixels.setPixelColor(day() + 60, pixels.Color(T_T.r, T_T.g, T_T.b));        // Tag jetzt

                                                                        //######## Monat
    for (int i = 92; i < 104; i++)
      pixels.setPixelColor(i, pixels.Color(M_HG.r, M_HG.g, M_HG.b));            // Hintergrund Monate
    if (TMF) {                                                                    // Alle Tage und Monate füllen
      for (int i = 93; i < 104; i++)                                            // alle abgelaufenen Monate
        if (i < month() + 93) {
          pixels.setPixelColor(i, pixels.Color(M_A.r, M_A.g, M_A.b));
        }
      for (int i = 92; i < 104; i = i + 3)                                      // jeden 3 Monate kennzeichnen
        pixels.setPixelColor(i, pixels.Color(M_P.r, M_P.g, M_P.b));
    }
    pixels.setPixelColor(month() + 92, pixels.Color(M_M.r, M_M.g, M_M.b));      // Monat jetzt
  } else {
    for (int i = 60; i < 104; i++)
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));                           // Monate und Tage ausschalten
  }
  pixels.show();                                                                // Alle Pixel anzeigen
}

//***************************************************
//************* Hauptprogramm Schleife  *************
//***************************************************
void loop() {
  time_t t = now();         // Holt sich die aktuelle Uhrzeit
  Pixel_Uhr(t);
                            
  if (digitalRead(Taster1)==LOW)
    rainbowCycle();
  if (digitalRead(Taster2)==LOW)
    Pixel_Reset(0,1,0);     // grün
  if (digitalRead(Taster3)==LOW)
    Pixel_Reset(0,0,1);     // blau

  Pixel_Helligkeit();       // Die Ausgabe der Pixel in Abhängigkeit der Helligkeit steuern
  delay(400);               // 0,4 Sekunden warten
  if (minute(t) == 0) {     // jeder Stunden die Uhrzeit vom Zeitserver hohlen und aktualisieren
    SetClockFromNTP();      // Holt sich die Zeit vom NTP-Server mit Zeitzonen-Korrektur
    delay(400);             // 0,4 Sekunden warten
    if ((Lichtspiel) and (HD == 1))
      if (second(t) <= 1)   // Nur einmal das Lichtspiel
        // Pixel_Reset(0,0,0); // Lichtspiel
        rainbowCycle();
  }
}