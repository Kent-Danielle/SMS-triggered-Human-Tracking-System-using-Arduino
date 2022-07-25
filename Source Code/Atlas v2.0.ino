
#include "Adafruit_FONA.h"
#define FONA_TX 2
#define FONA_RX 3
#define FONA_RST 4

#include <SoftwareSerial.h>

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);
float latitude, longitude;       // for coordinates
char replybuffer[255];           // idk rin
char fonaNotificationBuffer[64]; // for notifications from the FONA
char smsBuffer[250];             // idk men
char callerIDbuffer[32];         // sender's number
int button = 10;                 // button

void setup()
{
  while (!Serial)
    ;

  Serial.begin(115200);
  Serial.println(F("FONA SMS caller ID test"));
  Serial.println(F("Initializing....(May take 3 seconds)"));

  // make it slow so its easy to read!
  fonaSerial->begin(4800);
  if (!fona.begin(*fonaSerial))
  {
    Serial.println(F("Couldn't find FONA"));
    while (1)
      ;
  }
  Serial.println(F("FONA is OK"));

  // Print SIM card IMEI number.
  char imei[16] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = fona.getIMEI(imei);
  if (imeiLen > 0)
  {
    Serial.print("SIM card IMEI: ");
    Serial.println(imei);
  }

  fonaSerial->print("AT+CNMI=2,1\r\n"); // set up the FONA to send a +CMTI notification when an SMS is received
}

void loop()
{
  char *bufPtr = fonaNotificationBuffer; // handy buffer pointer
  int state = digitalRead(button);

  if (fona.available()) // SMS Function
  {
    int slot = 0; // this will be the slot number of the SMS
    int charCount = 0;

    // Read the notification into fonaInBuffer
    do
    {
      *bufPtr = fona.read();
      Serial.write(*bufPtr);
      delay(1);
    } while ((*bufPtr++ != '\n') && (fona.available()) && (++charCount < (sizeof(fonaNotificationBuffer) - 1)));

    *bufPtr = 0;

    // Scan the notification string for an SMS received notification.
    //   If it's an SMS message, we'll get the slot number in 'slot'

    Serial.println("Ready for SMS input\n");
    if (1 == sscanf(fonaNotificationBuffer, "+CMTI: " FONA_PREF_SMS_STORAGE ",%d", &slot))
    {
      Serial.println("Entering SMS function\n");
      Serial.print("slot: ");
      Serial.println(slot);

      // Retrieve SMS sender address/phone number.

      if (!fona.getSMSSender(slot, callerIDbuffer, 31))
      {
        Serial.println("Didn't find SMS message in slot!");
      }
      Serial.print(F("FROM: "));
      Serial.println(callerIDbuffer);

      // Retrieve SMS value.

      uint16_t smslen;
      if (fona.readSMS(slot, smsBuffer, 250, &smslen)) // pass in buffer and max len!
      {
        Serial.println(smsBuffer);
      }

      // Send back an automatic response

      SendLocation();

      // delete the original msg after it is processed

      if (fona.deleteSMS(slot))
      {
        Serial.println(F("OK!"));
      }
      else
      {
        Serial.print(F("Couldn't delete SMS in slot "));
        Serial.println(slot);
        fona.print(F("AT+CMGD=?\r\n"));
      }
    }
  }
}

void SendLocation()
{
  boolean gsmloc_success = fona.getGSMLoc(&latitude, &longitude);

  if (!gsmloc_success)
  {
    Serial.println("GSM location failed...");
    Serial.println(F("Disabling GPRS"));
    fona.enableGPRS(false);
    Serial.println(F("Enabling GPRS"));
    delay(300);
    fonaSS.print("AT+CMGF=1\r");
    delay(300);
    fonaSS.print("AT+CMGS=\"+639163271235\"\r");
    delay(300);
    fonaSS.print("I'm missing! please text this number to know my location");
    delay(300);
    fonaSS.println((char)26);
    fonaSS.println();
    delay(200);
    if (!fona.enableGPRS(true))
    {
      Serial.println(F("Failed to turn GPRS on"));
    }
  }

  else
  {
    Serial.println("success");
    Serial.print("GSMLoc lat:");
    Serial.println(latitude, 6);
    Serial.print("GSMLoc long:");
    Serial.println(longitude, 6);
    delay(700);
    fonaSS.print("AT+CMGF=1\r");
    delay(700);
    fonaSS.print("AT+CMGS=\"+639163271235\"\r");
    delay(700);
    fonaSS.print("Hello! I'm here:");
    fonaSS.print("https://www.google.com.ph/maps/place/");
    fonaSS.print(latitude, 6);
    fonaSS.print(",");
    fonaSS.println(longitude, 6);
    delay(700);
    fonaSS.println((char)26);
    fonaSS.println();
    delay(700);
  }
}
