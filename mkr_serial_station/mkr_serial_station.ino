#include <MKRWAN.h>
#include <RH_Serial.h>
#include <RHMesh.h>

#define GATEWAY_ADDRESS 0
#define SERVER_ADDRESS 1
#define N_NODES 4

LoRaModem modem;

uint8_t routes[N_NODES]; // full routing table for mesh
int16_t rssi[N_NODES]; // signal strength info

RH_Serial driver(Serial2);
RHMesh manager(driver, SERVER_ADDRESS);

const __FlashStringHelper* getErrorString(uint8_t error) {
  switch(error) {
    case 1: return F("invalid length");
    break;
    case 2: return F("no route");
    break;
    case 3: return F("timeout");
    break;
    case 4: return F("no reply");
    break;
    case 5: return F("unable to deliver");
    break;
  }
  return F("unknown");
}

void updateRoutingTable() {
  for(uint8_t n=1;n<=N_NODES;n++) {
    RHRouter::RoutingTableEntry *route = manager.getRouteTo(n);
    if (n == SERVER_ADDRESS) {
      routes[n-1] = 255; // self
    } else {
      routes[n-1] = route->next_hop;
      if (routes[n-1] == 0) {
        // if we have no route to the node, reset the received signal strength
        rssi[n-1] = 0;
      }
    }
  }
}

// Create a JSON string with the routing info to each node
void getRouteInfoString(char *p, size_t len) {
  p[0] = '\0';
  strcat(p, "[");
  for(uint8_t n=1;n<=N_NODES;n++) {
    strcat(p, "{\"n\":");
    sprintf(p+strlen(p), "%d", routes[n-1]);
    strcat(p, ",");
    strcat(p, "\"r\":");
    sprintf(p+strlen(p), "%d", rssi[n-1]);
    strcat(p, "}");
    if (n<N_NODES) {
      strcat(p, ",");
    }
  }
  strcat(p, "]");
}

void printNodeInfo(uint8_t node, char *s) {
  Serial2.print(F("node: "));
  Serial2.print(F("{"));
  Serial2.print(F("\""));
  Serial2.print(node);
  Serial2.print(F("\""));
  Serial2.print(F(": "));
  Serial2.print(s);
  Serial2.println(F("}"));
}

void printModemStatus() { // TODO: this is sketchy but it avoids using dicts, try to make less hacky
  int8_t stat = modem.getStatus();
  stat += 20;
  String stat_strs[] = {
    "Packet Exceeds Max Length!", // code -20
    "", // -19
    "", // -18
    "", // -17
    "", // -16
    "", // -15
    "", // -14
    "", // -13
    "", // -12
    "", // -11
    "", // -10
    "", // -9
    "LORA_ERROR_UNKNOWN", // -8
    "LORA_ERROR_RX", // -7
    "LORA_ERROR_NO_NETWORK", // -6
    "LORA_ERROR_OVERFLOW", // -5
    "LORA_ERROR_BUSY", // -4
    "LORA_ERROR_PARAM", // -3
    "LORA_ERROR", // -2
    "TIMEOUT" // -1
  };
  if (stat - 20 < 0) { 
    Serial.println(stat_strs[stat]);
  } else { 
    Serial.print("GOOD (");
    Serial.print(stat-20);
    Serial.println(")");
  }
}


void setup() {
  Serial.begin(115200);
  while(!Serial);
  Serial2.begin(115200);
  while(!Serial2);
  pinMode(LED_BUILTIN, OUTPUT);    
  modem.begin(US915_HYBRID);
  modem.init();
  //-------------------------------------LoRaWAN------------------------------------------------------------
  // AT Commands for joining Multitech network
  Serial.println("Joining Lora Network");
  delay(200);
  String res; 
  //Test
  // Serial2.println("AT");
  //OTAA network ID
  // Serial2.println("AT+NI=1,Ky-Newton"); // Match your network name
  Serial.print("Network Naming: ");
  modem.sendATMsg("+NI=", "1,Ky-Newton");
  printModemStatus();
  delay(100);
  //OTAA network key
  // Serial2.println("AT+NK=1,Ky-Newton"); // Match your password
  Serial.print("Network Password: ");
  modem.sendATMsg("+NK=", "1,Ky-Newton");
  printModemStatus();
  delay(100);
  //frequency sub band 3
  // Serial2.println("AT+FSB=3");
  Serial.print("Frequency Sub-band: ");
  modem.sendATMsg("+FSB=", "3");
  printModemStatus();
  delay(100);
  //join delay 5 sec
  Serial.print("Join Delay: ");
  modem.sendATMsg("+JD=", "5");
  printModemStatus();
  // Serial2.println("AT+JD=5");
  delay(100);
  //byte rate 53 bytes and distance abt .5-3 mile
  Serial.print("TX Mode: ");
  modem.sendATMsg("+TXDR=", "1");
  printModemStatus();
  delay(100);
  Serial.print("Join Mode: ");
  modem.sendATMsg("+NJM=", "1");
  printModemStatus();
  delay(100);
  Serial.print("TX Mode: ");
  modem.sendATMsg("+TXDR=", "1");
  printModemStatus();
  delay(100);
  //store in memory
  Serial.print("Store: ");
  modem.sendATMsg("&W");
  printModemStatus();
  delay(100);
  //reset
  Serial.print("Soft Reset: ");
  modem.sendATMsg("Z");
  printModemStatus();
  delay(5000);
  //Join network
  Serial.print("Join Network: ");
  modem.sendATMsg("+JOIN");
  printModemStatus();
  delay(1000);
  //-------------------------------------End LoRaWAN------------------------------------------------------------
  delay(5000); // setup delay
  // Serial2 as Mesh Manager
  Serial2.end();
  driver.serial().begin(115200);
  if(!manager.init()) {
    Serial.println("init failed");
    for(;;){}
  }
}


char buf[RH_MESH_MAX_MESSAGE_LEN];

void loop() {
  for(uint8_t n=0; n<N_NODES; n++) {
    if (n == SERVER_ADDRESS) continue; // self
    // --- print current client info ---
    Serial.print(F("->"));
    Serial.print(n);
    Serial.print(F(" :"));
    // Serial.print(buf);

    // --- try to contact client through the mesh and update the table accordingly ---
    updateRoutingTable();
    getRouteInfoString(buf, RH_MESH_MAX_MESSAGE_LEN);
    uint8_t error = manager.sendtoWait((uint8_t *)buf, strlen(buf), n);
    if (error != RH_ROUTER_ERROR_NONE) {
      Serial.println();
      Serial.print(F(" ! "));
      Serial.println(getErrorString(error));
    } else {
      Serial.println(F(" OK"));
      // we received an acknowledgement from the next hop for the node we tried to send to.
      RHRouter::RoutingTableEntry *route = manager.getRouteTo(n);
      /*
      if (route->next_hop != 0) {
        rssi[route->next_hop-1] = abz.lastRssi();
      }
      */
    }

    // --- listen for incoming messages and transmit them back to the gateway ---
    unsigned long nextTransmit = millis() + random(3000, 5000);
    while (nextTransmit > millis()) {
      int waitTime = nextTransmit - millis();
      uint8_t len = sizeof(buf);
      uint8_t from;
      if (manager.recvfromAckTimeout((uint8_t *)buf, &len, waitTime, &from)) {
        buf[len] = '\0'; // null terminate string
        Serial.print(from);
        Serial.print(F("->"));
        Serial.print(F(" :"));
        Serial.print(buf);
        if (SERVER_ADDRESS == 1) printNodeInfo(from, buf); // debugging
        // we received data from node 'from', but it may have actually come from an intermediate node
        RHRouter::RoutingTableEntry *route = manager.getRouteTo(from);

        // Send information back to the gateway through the mesh
        if(from == GATEWAY_ADDRESS) continue; // message was from gateway
        if(!manager.sendtoWait((uint8_t *)buf, sizeof(buf), GATEWAY_ADDRESS)) {
          // find route to gateway
          Serial.println("ERROR: Sending to gateway failed! Trying to find path..."); 
        }
        /*
        if (route->next_hop != 0) {
          rssi[route->next_hop-1] = abz.lastRssi();
        }
        */
      }
    }
  }
}
