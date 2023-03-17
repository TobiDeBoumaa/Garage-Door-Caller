// Please select the corresponding model

// #define SIM800L_IP5306_VERSION_20190610
// #define SIM800L_AXP192_VERSION_20200327
// #define SIM800C_AXP192_VERSION_20200609
#define SIM800L_IP5306_VERSION_20200811

// Define the serial console for debug prints, if needed
//#define DUMP_AT_COMMANDS
#define TINY_GSM_DEBUG SerialMon

#include "utilities.h"

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial
// Set serial for AT commands (to the module)
#define SerialAT Serial1

// Configure TinyGSM library
#define TINY_GSM_MODEM_SIM800   // Modem is SIM800
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb

#include <TinyGsmClient.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

// Set phone numbers, if you want to test SMS and Calls
#define SMS_TARGET "+380xxxxxxxxx"
#define CALL_TARGET "+380xxxxxxxxx"
#include "sim_pin.h"         // just the defines for the apn and pin number
const char apn[] = YOUR_APN; // Your APN
const char simPIN[] = YOUR_SIMPIN; // SIM card PIN code, if any

static volatile int num = 0;

String readCurrentCall(String serialRaw) {

  String outPutBuffer = "";
  serialRaw.trim();
  if (serialRaw.indexOf(F("+CLCC:")) != -1) {
    String CLCCvalue = serialRaw.substring(serialRaw.indexOf(
        F("+CLCC:"))); // on answer call, it give +COLP: xxxx OK and +CLCC: xxx.
                       // So we must split it.
    String currentCallStatus =
        CLCCvalue.substring(CLCCvalue.indexOf(F("+CLCC:")) + 11,
                            CLCCvalue.indexOf(F("+CLCC:")) + 12);
    String telefonNummer = CLCCvalue.substring(CLCCvalue.indexOf(F(",\"")) + 2,
                                               CLCCvalue.indexOf(F("\",")));

    if (currentCallStatus == "0") {
      currentCallStatus = "STATUS:ACTIVE"; // Görüşme var
    } else if (currentCallStatus == "1") {
      currentCallStatus = "STATUS:HELD";
    } else if (currentCallStatus == "2") {
      currentCallStatus = "STATUS:DIALING"; // Çevriliyor
    } else if (currentCallStatus == "3") {
      currentCallStatus = "STATUS:RINGING"; // Çalıyor
    } else if (currentCallStatus == "4") {
      currentCallStatus = "STATUS:INCOMING"; // Gelen arama
    } else if (currentCallStatus == "5") {
      currentCallStatus = "STATUS:WAITING"; // gelen arama bekliyor
    } else if (currentCallStatus == "6") {
      if (serialRaw.indexOf(F("BUSY")) != -1) {
        currentCallStatus = "STATUS:BUSY"; // kullanıcı meşgul bitti
      } else {
        currentCallStatus = "STATUS:CALLEND"; // görüşme bitti
      }
    }

    outPutBuffer = currentCallStatus + "|NUMBER:" + telefonNummer;
  }

  return outPutBuffer;
}

void setup() {
  // Set console baud rate
  SerialMon.begin(115200);

  delay(10);

  // Start power management
  if (setupPMU() == false) {
    Serial.println("Setting power error");
  }

  // Some start operations
  setupModem();

  // Set GSM module baud rate and UART pins
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

  delay(6000);
}

void loop() {
  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Initializing modem...");
  modem.restart();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem: ");
  SerialMon.println(modemInfo);

  // Unlock your SIM card with a PIN if needed
  if (strlen(simPIN) && modem.getSimStatus() != 3) {
    SerialMon.print("Unlocking SIM Card...");
    SerialMon.println(modem.simUnlock(simPIN) ? "PIN OK" : "PIN failed");
  }

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork(240000L)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  } else
    SerialMon.println(" OK");

  // Swap the audio channels
  SerialAT.print("AT+CHFA=1\r\n");
  delay(2);

  // Set ringer sound level
  SerialAT.print("AT+CRSL=100\r\n");
  delay(2);

  // Set loud speaker volume level
  SerialAT.print("AT+CLVL=100\r\n");
  delay(2);

  // Calling line identification presentation
  SerialAT.print("AT+CLIP=1\r\n");
  delay(10000);

  // Do nothing forevermore
  while (true) {
    if (SerialAT.available()) {
      String buffer = "";
      buffer = SerialAT.readString();
      num = num + 1;
      SerialMon.print(num);
      SerialMon.print(". ");

      // This example for how you catch incoming calls.
      if (buffer.indexOf("+CLCC:") != -1) {
        SerialMon.println(readCurrentCall(buffer));
      } else {
        SerialMon.println(buffer);
      }
    }
    modem.maintain();
  }
}
