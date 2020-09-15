 
#include <MemoryFree.h>
#include <RHRouter.h>
#include <RHMesh.h>
#include <RH_RF95.h>
#include <RH_ABZ.h>
#define RH_HAVE_SERIAL
#define LED 9
#define N_NODES 4

uint8_t nodeId;
uint8_t routes[N_NODES]; // full routing table for mesh
int16_t rssi[N_NODES]; // signal strength info

// Singleton instance of the radio driver
RH_ABZ abz;

// Class to manage message delivery and receipt, using the driver declared above
RHMesh *manager;

// message buffer
char buf[RH_MESH_MAX_MESSAGE_LEN];

void setup() {
  randomSeed(analogRead(0));
  pinMode(LED, OUTPUT);
  Serial2.begin(115200);
  while (!Serial2) ; // Wait for serial port to be available
  String msg = Serial2.readString(); // await string from the parent controller
  delay(500);
  // nodeId = EEPROM.read(0);
  nodeId = 5;
  if (nodeId > 10) {
    Serial2.print(F("EEPROM nodeId invalid: "));
    Serial2.println(nodeId);
    nodeId = 1;
  }
  Serial2.print(F("initializing node "));

  manager = new RHMesh(abz, nodeId);

  if (!manager->init()) {
    Serial2.println(F("init failed"));
  } else {
    Serial2.println("done");
  }
  abz.setTxPower(23, false);
  abz.setFrequency(915.0);
  abz.setCADTimeout(500);

  // long range configuration requires for on-air time
  boolean longRange = false;
  if (longRange) {
    RH_ABZ::ModemConfig modem_config = {
      0x78, // Reg 0x1D: BW=125kHz, Coding=4/8, Header=explicit
      0xC4, // Reg 0x1E: Spread=4096chips/symbol, CRC=enable
      0x08  // Reg 0x26: LowDataRate=On, Agc=Off.  0x0C is LowDataRate=ON, ACG=ON
    };
    abz.setModemRegisters(&modem_config);
    if (!abz.setModemConfig(RH_RF95::Bw125Cr48Sf4096)) {
      Serial2.println(F("set config failed"));
    }
  }

  Serial2.println("abz ready");

  for(uint8_t n=1;n<=N_NODES;n++) {
    routes[n-1] = 0;
    rssi[n-1] = 0;
  }

  Serial2.print(F("mem = "));
  Serial2.println(freeMemory());
}

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
    RHRouter::RoutingTableEntry *route = manager->getRouteTo(n);
    if (n == nodeId) {
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

void loop() {
  while(true) { 
    for(uint8_t n=1;n<=N_NODES;n++) {
      
      if (n == nodeId) continue; // self
  
      updateRoutingTable();
      getRouteInfoString(buf, RH_MESH_MAX_MESSAGE_LEN);
  
      Serial2.print(F("->"));
      Serial2.print(n);
      Serial2.print(F(" :"));
      Serial2.print(buf);
  
      // send an acknowledged message to the target node
      uint8_t error = manager->sendtoWait((uint8_t *)buf, strlen(buf), n);
      if (error != RH_ROUTER_ERROR_NONE) {
        Serial2.println();
        Serial2.print(F(" ! "));
        Serial2.println(getErrorString(error));
      } else {
        Serial2.println(F(" OK"));
        // we received an acknowledgement from the next hop for the node we tried to send to.
        RHRouter::RoutingTableEntry *route = manager->getRouteTo(n);
        if (route->next_hop != 0) {
          rssi[route->next_hop-1] = abz.lastRssi();
        }
      }
     
      if (nodeId == 1) printNodeInfo(nodeId, buf); // debugging
      // listen for incoming messages. Wait a random amount of time before we transmit
      // again to the next node
      unsigned long nextTransmit = millis() + random(3000, 5000);
      while (nextTransmit > millis()) {
        int waitTime = nextTransmit - millis();
        uint8_t len = sizeof(buf);
        uint8_t from;
        if (manager->recvfromAckTimeout((uint8_t *)buf, &len, waitTime, &from)) {
          buf[len] = '\0'; // null terminate string
          Serial2.print(from);
          Serial2.print(F("->"));
          Serial2.print(F(" :"));
          Serial2.println(buf);
          if (nodeId == 1) printNodeInfo(from, buf); // debugging
          // we received data from node 'from', but it may have actually come from an intermediate node
          RHRouter::RoutingTableEntry *route = manager->getRouteTo(from);
          if (route->next_hop != 0) {
            rssi[route->next_hop-1] = abz.lastRssi();
          }
        }
      }
    }
  }
}
