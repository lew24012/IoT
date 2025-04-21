#include <ESP8266WiFi.h>

// --- Configuration ---
const char *ssid = "MySimpleAP";       // Use the same SSID as before
const char *password = "password123"; // Use the same password as before

// --- Event Handlers ---
WiFiEventHandler stationConnectedHandler;
WiFiEventHandler stationDisconnectedHandler;

// Function called when a station connects to the AP
void onStationConnected(const WiFiEventSoftAPModeStationConnected& evt) {
  Serial.print("Client Connected: MAC=");
  // Print MAC address
  for (int i = 0; i < 6; ++i) {
    Serial.printf("%02X", evt.mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
  // Note: The IP address might not be assigned *immediately* at this event.
  // Check the client's own serial output for its assigned IP.
}

// Function called when a station disconnects from the AP
void onStationDisconnected(const WiFiEventSoftAPModeStationDisconnected& evt) {
  Serial.print("Client Disconnected: MAC=");
   for (int i = 0; i < 6; ++i) {
    Serial.printf("%02X", evt.mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring Access Point...");

  // Register event handlers BEFORE starting the AP
  stationConnectedHandler = WiFi.onSoftAPModeStationConnected(&onStationConnected);
  stationDisconnectedHandler = WiFi.onSoftAPModeStationDisconnected(&onStationDisconnected);

  // Start the Access Point
  Serial.printf("Setting AP SSID: %s\n", ssid);
  bool apStarted = WiFi.softAP(ssid, password);

  if (apStarted) {
    Serial.println("AP Started Successfully!");
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Failed to start AP!");
    while(1) { delay(1000); }
  }

  Serial.println("AP Setup Complete. Waiting for connections...");
}

// --- Loop ---
void loop() {
  // Optional: Print the number of connected stations periodically
  static unsigned long lastCheck = 0;
  unsigned long currentMillis = millis();

  if (currentMillis - lastCheck >= 10000) { // Every 10 seconds
    lastCheck = currentMillis;
    int numStations = WiFi.softAPgetStationNum();
    Serial.printf("Stations connected: %d\n", numStations);
  }

  delay(100);
}