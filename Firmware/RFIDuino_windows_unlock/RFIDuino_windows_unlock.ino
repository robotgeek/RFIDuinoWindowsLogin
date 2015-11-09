/***************************************************************************
 * RFIDuino Windows Unlock
 *
 * This demo uses the RFIDuino sheild, a Geekduino (or other Arduino Board)
 * to send an RFID to a Windows Vista/7/8/10 PC. See the link below for 
 * windows software and instructions.
 *
 * Links
 *    RFIDUino Windows Login Instructions
 *      http://learn.robotgeek.com/getting-started/41-rfiduino/181-rfiduino-windows-login.html
 *    RFIDUino Getting Started Guide
 *      http://learn.robotgeek.com/getting-started/41-rfiduino/142-rfiduino-getting-started-guide.html
 *    RFIDUino Library Documentation
 *      http://learn.robotgeek.com/41-rfiduino/144-rfiduino-library-documentation.html
 *    RFIDUino Shield Product Page
 *      http://www.robotgeek.com/rfiduino
 *  
 * Notes
 * The RFIDuino library is compatible with both versions of the RFIDuino shield
 * (1.1 and 1.2), but the user must initialize the library correctly. See 
 * 'HARDWARE VERSION' instructions near the beginning of the code.
 *
 * The RFIDuino Shield is designed to be used with 125khz EM4100 family tags. 
 * This includes any of the EM4102 tags sold by Trossen Robotics/ RobotGeek. 
 * The RFIDuino shield may not work with tags outside the EM4100 family
 *
 * User Output pins
 *    myRFIDuino.led1 - Red LED
 *    myRFIDuino.led2 - Green LED
 *    myRFIDuino.buzzer - Buzzer
 *
 ***************************************************************************/

#include <RFIDuino.h> //include the RFIDuino Library

#define SERIAL_PORT Serial      //Serial port definition for Geekduino, Arduino Uno, and most Arduino Boards
//#define SERIAL_PORT Serial1   //Serial port defintion for Arduino Leonardo and other ATmega32u4 based boards

/***************
* HARDWARE VERSION - MAKE SURE YOU HAVE SET THE CORRECT HARDWARE VERSION BELOW
* Version 1.1 has a 4-pin antenna port and no version marking
* Version 1.2 has a 2-pin antenna port and is marked 'Rev 1.2'
*****************/
//RFIDuino myRFIDuino(1.1);     //initialize an RFIDuino object for hardware version 1.1
RFIDuino myRFIDuino(1.2);   //initialize an RFIDuino object for hardware version 1.2

byte tagData[5]; //Holds the ID numbers from the tag  

int melody[] = {
  666, 1444, 666}; //holds notes to be played on tone
int noteDurations[] = {
  4, 8, 8 }; //holds duration of notes: 4 = quarter note, 8 = eighth note, etc.

void setup()
{
  //begin serial communicatons at 9600 baud and print a startup message
  SERIAL_PORT.begin(9600);
  SERIAL_PORT.print(">");
  
  //Comment out to turn off Debug Mode Buzzer START
  for (int thisNote = 0; thisNote < 3; thisNote++)
  {
    int noteDuration = 1000/noteDurations[thisNote];
    tone(myRFIDuino.buzzer, melody[thisNote],noteDuration);
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    noTone(myRFIDuino.buzzer);
  }
  //Comment out to turn off Debug Mode Buzzer END
  
  //The RFIDUINO hardware pins and user outputs(Buzzer / LEDS) are all initialized via pinMode() in the library initialization, so you don not need to to that manually
}

void loop()
{   
  //scan for a tag - if a tag is sucesfully scanned, return a 'true' and proceed
  if(myRFIDuino.scanForTag(tagData) == true)
  {
    digitalWrite(myRFIDuino.led2,HIGH);     //turn green LED on
    
    SERIAL_PORT.print("ID:"); //print a header to the Serial port.
    //loop through the byte array
    for(int n=0;n<5;n++)
    {
      SERIAL_PORT.print(tagData[n],DEC);  //print the byte in Decimal format
      if(n<4)//only print the comma on the first 4 nunbers
      {
        SERIAL_PORT.print(" ");
      }
    }
    SERIAL_PORT.print("\r\n>");//return character for next line and the prompt characer '>' for the next line
             
    delay(500);                             //wait for .5 second
    digitalWrite(myRFIDuino.led2,LOW);      //turn the green LED off


  }

 }// end loop()
 
 
  
  

