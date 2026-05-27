#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>

// =====================================================================
//  BetaFly — ESP-12S Transparent Bridge
//  Bridges WebSocket (Wi-Fi) <──> Serial UART (STM32F405)
//  No data processing — pure, zero-delay forwarding.
// =====================================================================

// --- Access Point credentials ---
static const char* AP_SSID = "BetaFly_Config";
static const char* AP_PASS = "12345678";

// --- WebSocket server on port 81 ---
WebSocketsServer webSocket(81);

// --- Serial read buffer (accumulates bytes until a full line arrives) ---
static String serialBuffer;

// =====================================================================
//  WebSocket event handler
//  Runs in the context of webSocket.loop() — never blocks.
// =====================================================================
void webSocketEvent(uint8_t clientNum, WStype_t type,
                    uint8_t* payload, size_t length)
{
    switch (type) {

        case WStype_TEXT:
            // Web → STM32: forward the raw text plus a newline terminator
            Serial.write(payload, length);
            Serial.write('\n');
            break;

        case WStype_BIN:
            // Web → STM32: forward binary frames as-is
            Serial.write(payload, length);
            break;

        // WStype_CONNECTED / WStype_DISCONNECTED: no bridge action needed
        default:
            break;
    }
}

// =====================================================================
//  setup()
// =====================================================================
void setup()
{
    // 1. UART at 115200 baud — must match STM32 configuration
    Serial.begin(115200);
    serialBuffer.reserve(256);   // pre-allocate to avoid heap fragmentation

    // 2. Start Wi-Fi Access Point
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);

    // 3. Start WebSocket server and register the event callback
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
}

// =====================================================================
//  loop()  — NON-BLOCKING, no delay() calls anywhere
// =====================================================================
void loop()
{
    // 1. Drive the WebSocket engine (handles ping/pong, framing, events)
    webSocket.loop();

    // 2. STM32 → Web: drain the UART RX buffer one byte at a time.
    //    Accumulate until a newline, then broadcast the complete line.
    //    Using Serial.available() keeps this fully non-blocking.
    while (Serial.available()) {
        char c = static_cast<char>(Serial.read());

        if (c == '\n') {
            if (serialBuffer.length() > 0) {
                webSocket.broadcastTXT(serialBuffer);
                serialBuffer = "";
            }
        } else if (c != '\r') {   // strip carriage-returns silently
            serialBuffer += c;
        }
    }
}