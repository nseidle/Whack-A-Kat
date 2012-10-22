/**************************************
*
*  Example for Sparkfun MP3 Shield Library
*      By: Bill Porter
*      www.billporter.info
*
*   Function:
*      This sketch listens for commands from a serial terminal (like the Serial Monitor in 
*      the Arduino IDE). If it sees 1-9 it will try to play an MP3 file named track00x.mp3
*      where x is a number from 1 to 9. For eaxmple, pressing 2 will play 'track002.mp3'.
*      A lowe case 's' will stop playing the mp3.
*      'f' will play an MP3 by calling it by it's filename as opposed to a track number. 
*
*      Sketch assumes you have MP3 files with filenames like 
*      "track001.mp3", "track002.mp3", etc on an SD card loaded into the shield. 
*
***************************************/

#include <SPI.h>

//Add the SdFat Libraries
#include <SdFat.h>
#include <SdFatUtil.h> 

//and the MP3 Shield Library
#include <SFEMP3Shield.h>

//create and name the library object
SFEMP3Shield MP3player;

byte temp;
byte result;

void setup() {

  Serial.begin(115200);
  Serial.println("Whack-A-Kat v1.0");
  
  //boot up the MP3 Player Shield
  result = MP3player.begin();
  //check result, see readme for error codes.
  if(result != 0) {
    Serial.print("Error code: ");
    Serial.print(result);
    Serial.println(" when trying to start MP3 player");
    }

  MP3player.SetVolume(2, 2);
  //Mp3SetVolume(20, 20); //Set initial volume (20 = -10dB) LOUD
  //Mp3SetVolume(40, 40); //Set initial volume (20 = -10dB) Manageable
  //Mp3SetVolume(80, 80); //Set initial volume (20 = -10dB) More quiet

  Serial.println("Send a number 1-9 to play a track or s to stop playing");
  
}

void loop() {
  
  if(Serial.available()){
    temp = Serial.read();
    
    Serial.print("Received command: ");
    Serial.write(temp);
    Serial.println(" ");
    
    //if s, stop the current track
    if (temp == 's') {
      MP3player.stopTrack();
    }
    else if (temp == 'c') {
      MP3player.playMP3("Cougar.mp3");

      //check result, see readme for error codes.
      if(result != 0) {
        Serial.print("Error code: ");
        Serial.print(result);
        Serial.println(" when trying to play track");
        }
      else
        Serial.println("Playing Cougar");
    }
  }
  
  delay(100);
  
}
