/*
 4-28-2011
 Spark Fun Electronics 2011
 Nathan Seidle
 
 This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
 
 This uses Bill Porter's exellent MP3 Shield Library
 
 There are 30 phases to WAK. Phase 1 has 1 button at a time and by phase 20 there are multiple buttons going
 at same time.
 
 Based on reaction time, the dispenser runs for different amounts of time.
 */

/*#include <SPI.h>
 
 //Add the SdFat Libraries
 #include <SdFat.h>
 #include <SdFatUtil.h> 
 
 //and the MP3 Shield Library
 #include <SFEMP3Shield.h>
 
 //Create and name the library object
 SFEMP3Shield MP3player;
 */
//byte temp;
//byte result;

#define number_of_buttons  2 //The number of buttons available to hit
//#define number_of_buttons  6
//#define game_length 30 //The number of rounds the game will last
#define game_length 12 //The number of rounds the game will last

boolean levels[game_length][number_of_buttons]; //Game board array
int currentLevel = 0; //goes all the way to game_length
int hitErrors; //Keeps track of how many buttons you hit that *aren't* lit up
long totalGameTime; //Total millis the user has used up

long timeSinceLastGame; //Don't let folks start a game quicker than every 10 seconds
//long minTimeBetweenGames = 10000; //# of milliseconds we need to wait between games, 10,000 = 10 seconds
long minTimeBetweenGames = 100; //# of milliseconds we need to wait between games, 10,000 = 10 seconds
long maxTimeBetweenLevels = 2000; //# of milliseconds you have to get the kat before game advances

char tempString[100]; //Used for the sprintf based debug statements

//Hardware connections
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
int led0 = 3;
int led1 = 5;
int led2 = 6;
int led3 = 9;
int led4 = 10;
int led5 = 11;

int button0 = A0;
int button1 = A1;
int button2 = A2;
int button3 = A3;
int button4 = A4;
int button5 = A5;

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


void setup() {

  Serial.begin(115200);
  Serial.println("Whack-A-Kat v1.0");

  randomSeed(A5);

  pinMode(led0, OUTPUT);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);
  pinMode(led5, OUTPUT);

  pinMode(button0, INPUT);
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);
  pinMode(button3, INPUT);
  pinMode(button4, INPUT);
  pinMode(button5, INPUT);

  //Enable pull ups
  digitalWrite(button0, HIGH);
  digitalWrite(button1, HIGH);
  digitalWrite(button2, HIGH);
  digitalWrite(button3, HIGH);
  digitalWrite(button4, HIGH);
  digitalWrite(button5, HIGH);

  //boot up the MP3 Player Shield
  /*  result = MP3player.begin();
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
   */

}

void loop() {

  waitForStart(); //Wait for someone to hit a button to begin the game

  startGame(); //Play start squence of noise and LED blinking

  populateTheGame(); //load random #s into the game array

  playTheGame(); //Play the game until levels are complete

    gameOver(); //Game is now complete! Print score, dispense cat food, play music

  timeSinceLastGame = millis(); //Remember what the time is so that folks don't start a new game too quickly
}

//Light up LEDs on the buttons
//If you hit the button, clear that button from array and turn off LED
//Record total time
//If time expires, record time and advance to next phase
void playTheGame(void) {

  //Restart the game
  totalGameTime = 0;
  currentLevel = 0;
  hitErrors = 0;

  //Play the game until the levels are done
  while(currentLevel < game_length) {
    sprintf(tempString, "Begin Level: %d", currentLevel);
    Serial.print(tempString);

    //Determine the number of buttons this round requires
    int hitsNeeded = 0;
    for(int x = 0 ; x < number_of_buttons ; x++) 
      if(levels[currentLevel][x] == 1) hitsNeeded++;

    //Light up button(s) for a certain amount of time
    //Serial.println("Lighting buttons");
    for(int ledNum = 0 ; ledNum < number_of_buttons ; ledNum++) {
      if(levels[currentLevel][ledNum] == 1) //light it up!
        controlLED(ledNum, HIGH);
      else
        controlLED(ledNum, LOW);
    }

    long levelStartTime = millis(); //Start the response timer

    //Check the buttons against the game array
    //Clearing successful hits as we go
    //Serial.println("Monitoring buttons");
    while(hitsNeeded) {

      if(millis() - levelStartTime > maxTimeBetweenLevels){ //You are too slow!
        Serial.print(" Too slow! ");

        //Turn off LEDs
        for(int ledNum = 0 ; ledNum < number_of_buttons ; ledNum++) controlLED(ledNum, LOW);

        //Penalty for not completing level
        hitErrors += hitsNeeded; //You now have whatever hits are left added to your error count

        break; //Goto next level
      }

      for(int x = 0 ; x < number_of_buttons ; x++) {
        int buttonState = checkButton(x);

        if(buttonState == LOW) { //You've hit one of the buttons!
          if(levels[currentLevel][x] == 1){ //This button needs to be pressed
            sprintf(tempString, " Button %d hit! ", x);
            Serial.print(tempString);

            //play cat sound

            //Debounce the button
            while(checkButton(x) == LOW) ; //Wait for you to stop pressing the button

            //Turn off button LED
            controlLED(x, LOW);

            hitsNeeded--;
            levels[currentLevel][x] == 0; //Mark this button as pressed
          }
          else { //This button doesn't need to be pressed but you hit it!
            Serial.print(" Bad hit! ");

            //play horn sound

            //Debounce the button
            while(checkButton(x) == LOW) ; //Wait for you to stop pressing the button

            hitErrors++;
          }
        } //End check this button state
      }

    } //End hitsNeeded checking

    //Once we are here, we have completed this level
    currentLevel++; //Advance to next level
    Serial.println(); //We are done printing this level info

    totalGameTime += (millis() - levelStartTime); //Add this level's time onto the total time
  }

}
//Plays a 3 second count down with LED blinks
//Blinks LEDs while we go
void startGame(void) {
  Serial.println();
  Serial.println("Begin new game!");

  Serial.print("Get ready: ");

  for(int x = 0 ; x < 4 ; x++) {
    long startTime = millis();

    Serial.print(3 - x);
    Serial.print(",");

    //Turn on all LEDs
    for(int led = 0 ; led < number_of_buttons ; led++)
      controlLED(led, HIGH);

    //Play normal Beep!
    delay(100);

    //Turn off LEDs
    for(int led = 0 ; led < number_of_buttons ; led++)
      controlLED(led, LOW);

    while(millis() - startTime < 500) ; //Wait for a half second each time around
  }

  //Turn on all LEDs
  for(int led = 0 ; led < number_of_buttons ; led++)
    controlLED(led, HIGH);

  //Play long Beep!
  delay(1000);

  //Turn off LEDs
  for(int led = 0 ; led < number_of_buttons ; led++)
    controlLED(led, LOW);

  Serial.println();
  Serial.println("Go!");

  delay(1000); //Wait one more second before we begin
}

//Pulses the button LEDs on/off to try to entice someone to play with me!!!!
void waitForStart(void) {
  Serial.println();
  Serial.println("Waiting for you to hit a button...");

  while(1) {

    //Bring LEDs up in brightness
    for(int fadeValue = 0 ; fadeValue <= 255; fadeValue +=5) {

      //Assign all the LEDs the same value
      for(int ledNum = 0 ; ledNum < number_of_buttons ; ledNum++) {
        if(checkButton(ledNum) == LOW){ //Someone wants to play!
          if(millis() - timeSinceLastGame > minTimeBetweenGames) //Make sure enough time has passed
            return;
          else
            Serial.println("Too soon!");
        }

        controlLEDanalog(ledNum, fadeValue);
      }

      delay(30);
    } 

    //Bring LEDs down in brightness
    for(int fadeValue = 255 ; fadeValue >= 0; fadeValue -=5) {

      //Assign all the LEDs the same value
      for(int ledNum = 0 ; ledNum < number_of_buttons ; ledNum++){
        if(checkButton(ledNum) == LOW){ //Someone wants to play!
          if(millis() - timeSinceLastGame > minTimeBetweenGames) //Make sure enough time has passed
            return;
          else
            Serial.println("Too soon!");
        }

        controlLEDanalog(ledNum, fadeValue);
      }

      delay(30);
    } 
  }
}

//Play win music to indicate game over
//Blink LEDs
//Print your time
//Dispense tasty food
void gameOver(void) {
  //Play win music!

  //Blink LEDs fast
  for(int x = 0 ; x < 10 ; x++) {
    //Turn on all LEDs
    for(int led = 0 ; led < number_of_buttons ; led++)
      controlLED(led, HIGH);

    delay(100);

    //Turn off LEDs
    for(int led = 0 ; led < number_of_buttons ; led++)
      controlLED(led, LOW);

    delay(100);
  }

  sprintf(tempString, "Game over! It took you %d seconds.", totalGameTime/1000);
  Serial.println(tempString);

  sprintf(tempString, "Hit errors: %d", hitErrors);
  Serial.println(tempString);

  sprintf(tempString, "Your score: %u", totalGameTime + (hitErrors*1000) );
  Serial.println(tempString);

  //Activate the food dispenser for some reward

}

//Given a button number, this will turn on or off an LED
void controlLED(int buttonNumber, int state) {
  if(buttonNumber == 0) digitalWrite(led0, state);
  if(buttonNumber == 1) digitalWrite(led1, state);
  if(buttonNumber == 2) digitalWrite(led2, state);
  if(buttonNumber == 3) digitalWrite(led3, state);
  if(buttonNumber == 4) digitalWrite(led4, state);
  if(buttonNumber == 5) digitalWrite(led5, state);
}

//Controls the button LEDs but with analog values rather than just on/off
void controlLEDanalog(int buttonNumber, int value) {
  if(buttonNumber == 0) analogWrite(led0, value);
  if(buttonNumber == 1) analogWrite(led1, value);
  if(buttonNumber == 2) analogWrite(led2, value);
  if(buttonNumber == 3) analogWrite(led3, value);
  if(buttonNumber == 4) analogWrite(led4, value);
  if(buttonNumber == 5) analogWrite(led5, value);
}

//Given a button number, this returns the state of that button
int checkButton(int buttonNumber) {

  if(buttonNumber == 0) return(digitalRead(button0));
  if(buttonNumber == 1) return(digitalRead(button1));
  if(buttonNumber == 2) return(digitalRead(button2));
  if(buttonNumber == 3) return(digitalRead(button3));
  if(buttonNumber == 4) return(digitalRead(button4));
  if(buttonNumber == 5) return(digitalRead(button5));

  Serial.print("Error: You shouldn't be here");
  return(0);  
}

/*
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
 */

void populateTheGame() {

  for(int x = 0 ; x < game_length ; x++)
    for(int j = 0 ; j < number_of_buttons ; j++)
      levels[x][j] = 0;

  for(int roundNumber = 0 ; roundNumber < game_length ; roundNumber++) {

    int spotsToAssign;
    if(roundNumber < 5) spotsToAssign = 1;
    else if(roundNumber < 20) spotsToAssign = 2;
    else if(roundNumber < 25) spotsToAssign = 3;
    else if(roundNumber < 30) spotsToAssign = 4;

    if(spotsToAssign > number_of_buttons) spotsToAssign = number_of_buttons; //Correct for too few buttons

      do{
      for(int button = 0 ; button < number_of_buttons && spotsToAssign > 0 ; button++){
        if(random(number_of_buttons) == button){ //if our random number equals the button number then turn on this in the array
          levels[roundNumber][button] = 1;
          spotsToAssign--;
        }
      }
    } 
    while(spotsToAssign > 0);

  }

  //Print the array
  Serial.println("Game:");
  for(int x = 0 ; x < game_length ; x++){
    sprintf(tempString, "Round %02d: ", x);
    Serial.print(tempString);

    for(int j = 0 ; j < number_of_buttons ; j++){
      if(levels[x][j] == 0)
        Serial.print("0 ");
      else
        Serial.print("1 ");
    }

    Serial.println();
  }

}







