#ifndef SONOS_CONTROL_H
#define SONOS_CONTROL_H

#include <ESP8266WiFi.h> 
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>

WiFiUDP udp;
IPAddress sonosIP;
const int udpPort = 1900;

void discoverSONOSDevices() {
  const char *request = "M-SEARCH * HTTP/1.1\r\n"
                        "HOST: 239.255.255.250:1900\r\n"
                        "MAN: \"ssdp:discover\"\r\n"
                        "MX: 3\r\n"
                        "ST: urn:schemas-upnp-org:device:ZonePlayer:1\r\n\r\n";

  Serial.print("Looking for SONOS devices...");

  udp.beginMulticast(WiFi.localIP(), IPAddress(239, 255, 255, 250), udpPort);
  udp.beginPacket(IPAddress(239, 255, 255, 250), udpPort);
  udp.write(request);
  udp.endPacket();

  unsigned long timeout = millis() + 5000;
  while (millis() < timeout) {
    int packetSize = udp.parsePacket();
    if (packetSize) {
      char packetBuffer[512];
      udp.read(packetBuffer, 512);
      packetBuffer[packetSize] = 0;
      String response = String(packetBuffer);

      if (response.indexOf("Sonos") >= 0) {
        int locStart = response.indexOf("LOCATION: ") + 10;
        int locEnd = response.indexOf("\r\n", locStart);
        String location = response.substring(locStart, locEnd);
        int ipStart = location.indexOf("//") + 2;
        int ipEnd = location.indexOf(":", ipStart);
        String ipStr = location.substring(ipStart, ipEnd);
        IPAddress deviceIP;
        deviceIP.fromString(ipStr);

        // Fetch device description to check room name
        WiFiClient client;
        HTTPClient http;
        if (http.begin(client, location)) {
          int httpResponseCode = http.GET();
         
          if (httpResponseCode > 0) {
            String payload = http.getString();
            Serial.println("Received payload:");
            Serial.println(payload);  // Debugging
     
            String searchString = "<roomName>" + String(ROOM_NAME) + "</roomName>";
     
            if (payload.indexOf(searchString) >= 0) {
              sonosIP = deviceIP;
              String foundDevice = "Found SONOS device in '" + String(ROOM_NAME) + "' at IP:";
              Serial.print(foundDevice);
              Serial.println(sonosIP);
              break;
            } else {
              Serial.println("Room name not found in payload.");
            }
          } else {
            Serial.print("HTTP GET failed, error code: ");
            Serial.println(httpResponseCode);
          }
          http.end();
        }
      }
    }
  }
  udp.stop();
}


String getCurrentTransportState() {
    if (!sonosIP) {
        Serial.println("SONOS IP not found");
        return "";
    }

    WiFiClient client;
    HTTPClient http;
    String soapRequest = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                         "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\""
                         " s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
                         "<s:Body>"
                         "<u:GetTransportInfo xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\">"
                         "<InstanceID>0</InstanceID>"
                         "</u:GetTransportInfo>"
                         "</s:Body>"
                         "</s:Envelope>";

    if (http.begin(client, String("http://") + sonosIP.toString() + ":1400/MediaRenderer/AVTransport/Control")) {
        http.addHeader("Content-Type", "text/xml; charset=\"utf-8\"");
        http.addHeader("SOAPACTION", "\"urn:schemas-upnp-org:service:AVTransport:1#GetTransportInfo\"");

        int httpResponseCode = http.POST(soapRequest);
        if (httpResponseCode > 0) {
            String response = http.getString();
            http.end();

            int startIndex = response.indexOf("<CurrentTransportState>") + 23;
            int endIndex = response.indexOf("</CurrentTransportState>");
            String transportState = response.substring(startIndex, endIndex);
            return transportState;
        } else {
            Serial.print("HTTP Error: ");
            Serial.println(http.errorToString(httpResponseCode).c_str());
        }
        http.end();
    }
    return "";
}


void sendSOAPCommand(String soapRequest, String action, String urlPath) {
    if (!sonosIP) {
        Serial.println("SONOS IP not found");
        return;
    }

    WiFiClient client;
    HTTPClient http;

    if (http.begin(client, "http://" + sonosIP.toString() + ":1400/" + urlPath)) {
        http.addHeader("Content-Type", "text/xml; charset=\"utf-8\"");
        http.addHeader("SOAPACTION", "\"" + action + "\"");
        http.addHeader("Host", sonosIP.toString() + ":1400");

        Serial.println(soapRequest);
        int httpResponseCode = http.POST(soapRequest);
        if (httpResponseCode > 0) {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
        } else {
            Serial.print("HTTP Error: ");
            Serial.println(http.errorToString(httpResponseCode).c_str());
        }
        http.end();
    }
}



int getCurrentVolume() {
    // Build the SOAP request to get the current volume
    String soapRequest = 
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
        "<s:Body>"
        "<u:GetVolume xmlns:u=\"urn:schemas-upnp-org:service:RenderingControl:1\">"
        "<InstanceID>0</InstanceID>"
        "<Channel>Master</Channel>"
        "</u:GetVolume>"
        "</s:Body>"
        "</s:Envelope>";

    // Send the SOAP request using sendSOAPCommand
    WiFiClient client;
    HTTPClient http;

    String action = "urn:schemas-upnp-org:service:RenderingControl:1#GetVolume";
    String response = "";

    String url = "http://" + sonosIP.toString() + ":1400/MediaRenderer/RenderingControl/Control";

    if (http.begin(client, url)) {
        http.addHeader("Content-Type", "text/xml; charset=\"utf-8\"");
        http.addHeader("SOAPACTION", "\"" + action + "\"");
        http.addHeader("Host", sonosIP.toString() + ":1400");

        int httpResponseCode = http.POST(soapRequest);
        if (httpResponseCode > 0) {
            response = http.getString();  // Get the response payload
        } else {
            Serial.print("HTTP Error: ");
            Serial.println(http.errorToString(httpResponseCode).c_str());
        }
        http.end();
    }

    // Parse the response to extract the current volume
    int volumeStart = response.indexOf("<CurrentVolume>");
    int volumeEnd = response.indexOf("</CurrentVolume>");
    if (volumeStart != -1 && volumeEnd != -1) {
        String volumeStr = response.substring(volumeStart + 15, volumeEnd);
        return volumeStr.toInt();
    }
    
    // Return -1 in case of an error
    return -1;
}




String buildVolumeSOAPRequest(int volume) {
  String soapRequest = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                       "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\""
                       " s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
                       "<s:Body>"
                       "<u:SetVolume xmlns:u=\"urn:schemas-upnp-org:service:RenderingControl:1\">"
                       "<InstanceID>0</InstanceID>"
                       "<Channel>Master</Channel>"
                       "<DesiredVolume>" + String(volume) + "</DesiredVolume>"
                       "</u:SetVolume>"
                       "</s:Body>"
                       "</s:Envelope>";
  return soapRequest;
}

String buildPlaySOAPRequest() {
    return "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
           "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\""
           " s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
           "<s:Body>"
           "<u:Play xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\">"
           "<InstanceID>0</InstanceID>"
           "<Speed>1</Speed>"
           "</u:Play>"
           "</s:Body>"
           "</s:Envelope>";
}

String buildPauseSOAPRequest() {
    return "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
           "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\""
           " s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
           "<s:Body>"
           "<u:Pause xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\">"
           "<InstanceID>0</InstanceID>"
           "</u:Pause>"
           "</s:Body>"
           "</s:Envelope>";
}



void togglePlayPause() {
    String currentState = getCurrentTransportState();

    if (currentState == "PLAYING") {
        Serial.println("Pausing playback");
        String soapRequest = buildPauseSOAPRequest();
        sendSOAPCommand(soapRequest, "urn:schemas-upnp-org:service:AVTransport:1#Pause", "MediaRenderer/AVTransport/Control");
    } else if (currentState == "PAUSED_PLAYBACK" || currentState == "STOPPED") {
        Serial.println("Starting playback");
        String soapRequest = buildPlaySOAPRequest();
        sendSOAPCommand(soapRequest, "urn:schemas-upnp-org:service:AVTransport:1#Play", "MediaRenderer/AVTransport/Control");
    } else {
        Serial.println("Unknown transport state or unable to toggle");
    }
}

void adjustVolume(int volumeDelta) {
    // Get the current volume
    int currentVolume = getCurrentVolume();
    Serial.println("Current volume is " + String(currentVolume));
    Serial.println("Adding delta " + String(volumeDelta));
    
    if (currentVolume >= 0) {  // Ensure volume retrieval was successful
        int newVolume = currentVolume + volumeDelta;

        // Clamp the volume within the range of 0 to 100
        newVolume = max(0, min(30, newVolume));

        Serial.println("Adjusting volume to " + String(newVolume));

        // Build the SOAP request to set the new volume
        String soapRequest = buildVolumeSOAPRequest(newVolume);

        // Use sendSoapCommand to send the SOAP request
        sendSOAPCommand(soapRequest, "urn:schemas-upnp-org:service:RenderingControl:1#SetVolume", "MediaRenderer/RenderingControl/Control");
    }
}
#endif  //SONOS_CONTROL_H
