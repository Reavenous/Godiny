#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LedControl.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

// ----- MAX7219 -----
LedControl lc = LedControl(D5, D7, D6, 1); // DIN, CLK, CS (GPIO14, GPIO13, GPIO12)

// ----- RTC DS1302 -----
#define PIN_IO   D3  // GPIO0
#define PIN_SCLK D4  // GPIO2
#define PIN_CE   D8  // GPIO15
ThreeWire myWire(PIN_IO, PIN_SCLK, PIN_CE);
RtcDS1302<ThreeWire> Rtc(myWire);

// ----- Wi-Fi AP -----
const char* ssid = "ESP_CLOCK";
const char* password = "12345678";
ESP8266WebServer server(80);

// ----- Segmentové mapování pro přepojené segmenty -----
const byte digitMap[10] = {
  B01111101, // 0
  B00110000, // 1
  B01101110, // 2
  B01111010, // 3
  B00110011, // 4
  B01011011, // 5
  B01011111, // 6
  B01110000, // 7
  B01111111, // 8
  B01111011  // 9
};

bool blink = false;

// ----- HTML FORM -----
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head><title>Nastavení hodin</title></head>
<body>
  <h1>Nastavení času (HH:MM)</h1>
  <form action="/set" method="GET">
    Hodiny: <input type="number" name="h" min="0" max="23"><br>
    Minuty: <input type="number" name="m" min="0" max="59"><br>
    <input type="submit" value="Nastavit">
  </form>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);

  // ---- MAX7219 ----
  lc.shutdown(0, false);
  lc.setIntensity(0, 8);
  lc.clearDisplay(0);

  // ---- RTC ----
  Rtc.Begin();
  if (Rtc.GetIsWriteProtected()) Rtc.SetIsWriteProtected(false);
  if (!Rtc.GetIsRunning()) Rtc.SetIsRunning(true);

  // ---- WiFi AP ----
  WiFi.softAP(ssid, password);
  Serial.print("AP adresa: ");
  Serial.println(WiFi.softAPIP());

  // ---- Web server routes ----
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", htmlPage);
  });

  server.on("/set", HTTP_GET, []() {
    if (server.hasArg("h") && server.hasArg("m")) {
      int h = server.arg("h").toInt();
      int m = server.arg("m").toInt();

      RtcDateTime now = Rtc.GetDateTime();
      RtcDateTime updated(
        now.Year(), now.Month(), now.Day(),
        h, m, 0
      );
      Rtc.SetDateTime(updated);
      server.send(200, "text/html", "<p>Čas nastaven. <a href='/'>Zpět</a></p>");
    } else {
      server.send(400, "text/plain", "Chybí parametr");
    }
  });

  server.begin();
  Serial.println("Web server spuštěn");
}

void loop() {
  server.handleClient();

  RtcDateTime now = Rtc.GetDateTime();
  int h1 = now.Hour() / 10;
  int h2 = now.Hour() % 10;
  int m1 = now.Minute() / 10;
  int m2 = now.Minute() % 10;

  // Vyčistit displej
  for (int i = 0; i < 4; i++) {
    lc.setRow(0, i, 0);
  }

  // Zobrazit čas s blikající dvojtečkou (segment g na pozici 2)
  lc.setRow(0, 3, digitMap[h1]);
  lc.setRow(0, 2, blink ? (digitMap[h2] | 0x40) : digitMap[h2]);
  lc.setRow(0, 1, digitMap[m1]);
  lc.setRow(0, 0, digitMap[m2]);

  blink = !blink;
  delay(1000);
}