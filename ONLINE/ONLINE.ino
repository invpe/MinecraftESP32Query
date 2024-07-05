#include <WiFi.h>
#include <WiFiUdp.h>

const char *ssid = "xxxxxxxxxxxxxxxxxx";
const char *password = "yyyyyyyyyyyyyyyyyy";
IPAddress minecraftServer(192, 168, 1, 44);   

WiFiUDP udpClient;
unsigned int minecraftPort = 25565;
char buffer[2048] = { 0x00 };
uint32_t challengeToken;
uint64_t uiLastCheck = millis();
void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize UDP client
  udpClient.begin(1911);

  // Challenge request
  memset(buffer, 0x00, sizeof(buffer));
  byte bFirstPacket[] = { 0xFE, 0xFD, 0x09, 0x00, 0x00, 0x00, 0x01 };
  udpClient.beginPacket(minecraftServer, minecraftPort);
  udpClient.write(bFirstPacket, sizeof(bFirstPacket));
  udpClient.endPacket();
}
void loop() {
  // Request interval
  if (millis() - uiLastCheck >= 5000) {
    // Basic stat request
    byte bBasicStatRequest[] = {
      0xFE, 0xFD,                     // Magic
      0x00,                           // Type
      0x00, 0x00, 0x00, 0x01,         // Session ID
      (challengeToken >> 24) & 0xFF,  // Challenge token (big-endian)
      (challengeToken >> 16) & 0xFF,
      (challengeToken >> 8) & 0xFF,
      challengeToken & 0xFF
    };
    memset(buffer, 0x00, sizeof(buffer));
    udpClient.beginPacket(minecraftServer, minecraftPort);
    udpClient.write(bBasicStatRequest, sizeof(bBasicStatRequest));
    udpClient.endPacket();
    uiLastCheck = millis();
  }


  // Read
  int packetSize = udpClient.parsePacket();
  int bytesRead = udpClient.read(buffer, sizeof(buffer));

  
  if (bytesRead > 0) {
    for (int x = 0; x < bytesRead; x++) {
      Serial.printf("%x", buffer[x]);
    }
    Serial.println("");

    // Basic stat
    if (buffer[0] == 0x00 && buffer[1] == 0x00 && buffer[2] == 0x00 && buffer[3] == 0x00 && buffer[4] == 0x01) {
      Serial.println("GOT Basic Stats");
      int iType = buffer[0];
      uint32_t uiSession = (buffer[1] << 24) | (buffer[2] << 16) | (buffer[3] << 8) | (buffer[4]);
      std::string strMOTD(reinterpret_cast<char *>(buffer + 5));
      std::string strGameType(reinterpret_cast<char *>(buffer + 5 + strMOTD.size() + 1));
      std::string strMap(reinterpret_cast<char *>(buffer + 5 + strGameType.size() + 1 + strMOTD.size() + 1));
      std::string strPlayers(reinterpret_cast<char *>(buffer + 5 + strMap.size() + 1 + strGameType.size() + 1 + strMOTD.size() + 1));
      Serial.printf("MOTD: %s GameType: %s Map: %s Players: %s\n", strMOTD.data(),
                    strGameType.data(),
                    strMap.data(),
                    strPlayers.data());
    }
    // Challenge
    else if (buffer[0] == 0x09 && buffer[1] == 0x00) {
      Serial.println("GOT Challenge");
      int iType = buffer[0];
      uint32_t uiSession = (buffer[1] << 24) | (buffer[2] << 16) | (buffer[3] << 8) | (buffer[4]);
      std::string strChallenge(reinterpret_cast<char *>(buffer + 5));
      challengeToken = atoi(strChallenge.c_str());
    }
  }
}
