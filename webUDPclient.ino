#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <vector> // To store received messages

// --- Configuration ---
const char *ssid = "MySimpleAP";       // Must match AP SSID
const char *password = "password123"; // Must match AP password

const int webServerPort = 80;       // Standard HTTP port
const int messageUdpPort = 4211;    // Port for Client-to-Client UDP messages
const int maxMessages = 15;         // Max number of received messages to store/display

// --- Global Objects ---
ESP8266WebServer server(webServerPort);
WiFiUDP udpListener;
std::vector<String> receivedMessages; // Dynamic array to store messages

// --- Forward Declarations ---
void handleRoot();
void handleSendMessage();
void handleNotFound();
void checkForIncomingMessages();

// --- Setup ---
void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println("ESP8266 Client Web Server & UDP Messenger");

  // --- Connect to WiFi ---
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // Start clean
  delay(100);
  Serial.printf("Connecting to SSID: %s\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("My IP Address: ");
  Serial.println(WiFi.localIP());

  // --- Start UDP Listener ---
  if (udpListener.begin(messageUdpPort)) {
    Serial.printf("UDP Listener started on port %d\n", messageUdpPort);
  } else {
    Serial.println("Failed to start UDP Listener!");
    // Handle error? For now, just log it.
  }

  // --- Configure Web Server Handlers ---
  server.on("/", HTTP_GET, handleRoot);         // Main page
  server.on("/send", HTTP_POST, handleSendMessage); // Form submission endpoint
  server.onNotFound(handleNotFound);           // 404 handler

  // --- Start Web Server ---
  server.begin();
  Serial.printf("HTTP Server started on port %d\n", webServerPort);
}

// --- Loop ---
void loop() {
  server.handleClient(); // Handle incoming web requests
  checkForIncomingMessages(); // Check for UDP messages

  delay(10); // Small delay is good practice
}

// --- Web Server Handlers ---

// Serves the main HTML page
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>ESP8266 Chat</title>";
  // Auto-refresh the page every 15 seconds to show new messages
  //html += "<meta http-equiv='refresh' content='30'>";
  html += "<style>";
  html += "body { font-family: sans-serif; max-width: 600px; margin: auto; padding: 15px; }";
  html += "h1, h2 { text-align: center; }";
  html += "form { margin-bottom: 20px; padding: 15px; border: 1px solid #ccc; border-radius: 5px; background-color: #f9f9f9; }";
  html += "label, input, textarea { display: block; margin-bottom: 10px; width: 95%; }";
  html += "input[type='text'], textarea { padding: 8px; }";
  html += "input[type='submit'] { padding: 10px 15px; background-color: #007bff; color: white; border: none; border-radius: 3px; cursor: pointer; }";
  html += "input[type='submit']:hover { background-color: #0056b3; }";
  html += "#messages { list-style: none; padding: 0; border: 1px solid #eee; max-height: 300px; overflow-y: scroll; }";
  html += "#messages li { padding: 8px; border-bottom: 1px solid #eee; }";
  html += "#messages li:nth-child(odd) { background-color: #f9f9f9; }";
  html += "</style></head><body>";

  html += "<h1>ESP8266 UDP Messenger</h1>";
  html += "<h2>My IP: " + WiFi.localIP().toString() + "</h2>";

  // --- Send Message Form ---
  html += "<form action='/send' method='POST'>"; // Use POST
  html += "<h3>Send Message</h3>";
  html += "<label for='target_ip'>Target IP Address:</label>";
  html += "<input type='text' id='target_ip' name='target_ip' required placeholder='e.g., 192.168.4.3'>";
  html += "<label for='message'>Message:</label>";
  // Use textarea for potentially longer messages
  html += "<textarea id='message' name='message' rows='3' required></textarea>";
  html += "<input type='submit' value='Send Message'>";
  html += "</form>";

  // --- Display Received Messages ---
  html += "<h3>Received Messages</h3>";
  html += "<ul id='messages'>";
  if (receivedMessages.empty()) {
    html += "<li>No messages received yet.</li>";
  } else {
    // Display messages, newest first (optional)
    for (int i = receivedMessages.size() - 1; i >= 0; i--) {
       // Basic HTML escaping for safety (replace < and >)
       String msg = receivedMessages[i];
       msg.replace("<", "&lt;");
       msg.replace(">", "&gt;");
       html += "<li>" + msg + "</li>";
    }
  }
  html += "</ul>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

// Handles the form submission to send a message
void handleSendMessage() {
  String targetIpStr;
  String message;
  bool error = false;

  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  if (server.hasArg("target_ip") && server.hasArg("message")) {
    targetIpStr = server.arg("target_ip");
    message = server.arg("message");
  } else {
    error = true;
  }

  IPAddress targetIp;
  if (!error && !targetIp.fromString(targetIpStr)) {
    error = true;
  }

  if (error || message.length() == 0) {
    // Send a simple error page or redirect with an error message (more complex)
    String errorHtml = "<!DOCTYPE html><html><head><title>Error</title></head><body>";
    errorHtml += "<h1>Error Sending Message</h1>";
    errorHtml += "<p>Invalid target IP or empty message.</p>";
    errorHtml += "<p><a href='/'>Go Back</a></p>";
    errorHtml += "</body></html>";
    server.send(400, "text/html", errorHtml);
    return;
  }

  // --- Send UDP Packet ---
  Serial.printf("Sending UDP message to %s:%d\n", targetIpStr.c_str(), messageUdpPort);
  Serial.printf("Message: %s\n", message.c_str());

  // Format: "[Sender IP]: message"
  String fullMessage = "[" + WiFi.localIP().toString() + "]: " + message;

  udpListener.beginPacket(targetIp, messageUdpPort);
  udpListener.print(fullMessage); // Send the message
  bool success = udpListener.endPacket();

  if (!success) {
      Serial.println("UDP Send failed!");
      // Optionally notify the user on the web page? For now, just log.
  }

  // --- Redirect back to the main page ---
  // Use 303 See Other for POST redirect
  server.sendHeader("Location", "/", true);
  server.send(303, "text/plain", "");
}

// Handles 404 Not Found errors
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  Serial.print(message);
}

// --- Helper Functions ---

// Checks for incoming UDP packets and stores them
void checkForIncomingMessages() {
  int packetSize = udpListener.parsePacket();
  if (packetSize > 0) {
    // Read packet data
    char incomingPacket[512]; // Buffer for incoming message
    int len = udpListener.read(incomingPacket, sizeof(incomingPacket) - 1);
    if (len > 0) {
      incomingPacket[len] = 0; // Null-terminate
    }

    String msg = String(incomingPacket);
    IPAddress remoteIp = udpListener.remoteIP(); // Get sender's IP

    Serial.printf("Received UDP: From %s:%d, Size %d, Message: %s\n",
                  remoteIp.toString().c_str(), udpListener.remotePort(), packetSize, msg.c_str());

    // Add message to the vector
    if (receivedMessages.size() >= maxMessages) {
      receivedMessages.erase(receivedMessages.begin()); // Remove oldest message if full
    }
    receivedMessages.push_back(msg); // Add new message
  }
}