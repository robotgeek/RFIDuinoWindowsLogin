/***************************************************************************
 * RFIDuino Windows Unlock
 *
 * This demo uses the RFIDuino sheild, a Geekduino (or other Arduino Board)
 * to send an RFID to a Windows Vista/7/8/10 PC. See the link below for 
 * windows software and instructions.
 *
 * Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno           Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 */

#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         9          // Configurable, see typical pin layout above
#define SS_PIN          10         // Configurable, see typical pin layout above
#define READ_LED         2         // LED used to display card read

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

void setup()
{
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  //begin serial communicatons at 9600 baud and print a startup message
  Serial.begin(9600);
  Serial.print(">");
  pinMode(READ_LED, OUTPUT);
}

void loop()
{   
  delay(200);
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }

  Serial.print("ID:"); //print a header to the Serial port.
   for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
     
  }
   Serial.print("\r\n>");//return character for next line
   digitalWrite(READ_LED, HIGH); 
   delay(1000);
   digitalWrite(READ_LED, LOW); 

}
 
  
  

