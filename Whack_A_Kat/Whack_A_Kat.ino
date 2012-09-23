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

#include <SPI.h>

//Add the SdFat Libraries
#include <SdFat.h>
#include <SdFatUtil.h> 

#include <Servo.h> 

//and the MP3 Shield Library
#include <SFEMP3Shield.h>

//Create and name the library object
SFEMP3Shield MP3player;

//Create servo object
Servo foodServo;

//byte temp;
byte MP3result;

#define number_of_buttons  6 //The number of buttons available to hit
#define number_of_rounds 8 //The number of rounds the game will last

//We have to turn down the volume when all the cats are on otherwise the system will reset
//We can turn up the volume when individual cats are activated
#define ONECAT_VOLUME  30 //40 is ok, 20 is pretty stinking loud
#define ALLCAT_VOLUME  60 //Less then 60 and system will reset

//#define FOOD_AMOUNT 6700 //Three servings
#define FOOD_AMOUNT 6700/2 //Two servings
//#define FOOD_AMOUNT 6700/3 //One serving

int levels[number_of_rounds][number_of_buttons]; //Game board array
int level_times[number_of_rounds]; //Keeps track of each round time

int currentLevel = 0; //goes all the way to game_length
int hitErrors; //Keeps track of how many buttons you hit that *aren't* lit up
long totalGameTime; //Total millis the user has used up

long timeSinceLastGame; //Don't let folks start a game quicker than every 10 seconds
//long minTimeBetweenGames = 10000; //# of milliseconds we need to wait between games, 10,000 = 10 seconds
long minTimeBetweenGames = 100; //# of milliseconds we need to wait between games, 10,000 = 10 seconds

char tempString[100]; //Used for the sprintf based debug statements

long overallScoreTotal; //This is the total of all the scores played since power up. Used for average game score
long gamesPlayed; //Number of games played since power on. Used for average game score


//Hardware connections
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
int led0 = 8;
int led1 = 9;
int led2 = 10;
int led3 = 11;
int led4 = 12;
int led5 = 13;

int cat0 = 22; //Control to the first pipe amp (aka Cat #0)
int cat1 = 24;
int cat2 = 26;
int cat3 = 28;
int cat4 = 30;
int cat5 = 32;

int button0 = 19;
int button1 = 18;
int button2 = 17;
int button3 = 16;
int button4 = 15;
int button5 = 14;

//int foodControl = 25;
int gameControl = A7;
int audioControl = A13;
int statLED = A15;

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

  pinMode(cat0, OUTPUT);
  pinMode(cat1, OUTPUT);
  pinMode(cat2, OUTPUT);
  pinMode(cat3, OUTPUT);
  pinMode(cat4, OUTPUT);
  pinMode(cat5, OUTPUT);

  pinMode(statLED, OUTPUT);
  //  pinMode(foodControl, OUTPUT);

  pinMode(button0, INPUT);
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);
  pinMode(button3, INPUT);
  pinMode(button4, INPUT);
  pinMode(button5, INPUT);

  pinMode(audioControl, INPUT);
  pinMode(gameControl, INPUT);

  //Enable pull ups
  digitalWrite(button0, HIGH);
  digitalWrite(button1, HIGH);
  digitalWrite(button2, HIGH);
  digitalWrite(button3, HIGH);
  digitalWrite(button4, HIGH);
  digitalWrite(button5, HIGH);
  digitalWrite(audioControl, HIGH);
  digitalWrite(gameControl, HIGH);

  //Set outputs
  catsOff(); //Turn off all cats

  //boot up the MP3 Player Shield
  MP3result = MP3player.begin();
  //check result, see readme for error codes.
  if(MP3result != 0) {
    Serial.print("Error code: ");
    Serial.print(MP3result);
    Serial.println(" when trying to start MP3 player");
  }

  MP3player.SetVolume(ALLCAT_VOLUME, ALLCAT_VOLUME); //System runs with all 6 on

  overallScoreTotal = 0; //Reset total scores
  gamesPlayed = 0; //Reset games played

  //Test routine - not for production code
  //testRoutine();

  //dispenseFood();
}

void loop() {

  if(digitalRead(gameControl) == LOW) {
    Serial.println("Game is now turned off");
    while(digitalRead(gameControl) == LOW){ //Don't play if the switch is flipped
      digitalWrite(statLED, HIGH);
      delay(1000);
      digitalWrite(statLED, LOW);
      delay(1000);
    }
    Serial.println("Game on!");
  }

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

  //Now set volume for individual cat sounds
  //System dies when we set the volume too loud and turn on all speakers
  //This can be louder because we're only going to turn one on at a time
  MP3player.SetVolume(ONECAT_VOLUME, ONECAT_VOLUME);

  //Play the game until the levels are done
  while(currentLevel < number_of_rounds) {
    sprintf(tempString, "Begin Level: %d", currentLevel);
    Serial.print(tempString);

    if(MP3player.isPlaying() == false) catsOff(); //Turn off all the cats if no MP3 is playing

    //We need some off time of the LEDs so that people understand the buttons are on/off
    for(int ledNum = 0 ; ledNum < number_of_buttons ; ledNum++) controlLED(ledNum, LOW);
    delay(500);

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

    //Look up the max time for this round
    int maxRoundTime = level_times[currentLevel]; 

    long levelStartTime = millis(); //Start the response timer

    boolean catPlaying = false; //No cat sounds should be playing

    //Check the buttons against the game array
    //Clearing successful hits as we go
    //Serial.println("Monitoring buttons");
    while(hitsNeeded) {
      if(MP3player.isPlaying() == false) catsOff(); //Turn off all the cats if no MP3 is playing

      if(millis() - levelStartTime > maxRoundTime){ //You are too slow!
        Serial.print(" Too slow! ");

        //Turn off LEDs
        for(int ledNum = 0 ; ledNum < number_of_buttons ; ledNum++) controlLED(ledNum, LOW);

        //Penalty for not completing level
        hitErrors += hitsNeeded; //You now have whatever hits are left added to your error count

        break; //Goto next level
      }

      int buttonState[number_of_buttons]; //Records what buttons are pressed

      //Check the state of buttons
      for(int x = 0 ; x < number_of_buttons ; x++)
        buttonState[x] = checkButton(x);

      for(int x = 0 ; x < number_of_buttons ; x++) {

        if(buttonState[x] == LOW) { //You've hit one of the buttons!
          if(levels[currentLevel][x] == 1){ //This button needs to be pressed
            sprintf(tempString, " Button %d hit! ", x);
            Serial.print(tempString);

            //play cat sound
            if(catPlaying == false) {
              playCat(x);
              catPlaying = true;
            }

            //Debounce the button
            //while(checkButton(x) == LOW) ; //Wait for you to stop pressing the button

            //Turn off button LED
            controlLED(x, LOW);

            hitsNeeded--;
            levels[currentLevel][x] = 2; //Mark this button as pressed
          }
          else if (levels[currentLevel][x] == 2) {
            //Do nothing, this is just you holding the button for too long

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

//Given a number, plays a cat track
//Stops any previous track that may be playing
void playCat(int catNumber) {
  if(MP3player.isPlaying()) MP3player.stopTrack(); //Stop any previous track

  if(digitalRead(audioControl) == LOW) { //Check to see if we should turn off the sounds
    if(catNumber == 0) {
      selectCat(0);
      MP3player.playMP3("cat1.mp3");
    }
    if(catNumber == 1){
      selectCat(1);
      MP3player.playMP3("cat2.mp3");
    }
    if(catNumber == 2){
      selectCat(2);
      MP3player.playMP3("cat3.mp3");
    }
    if(catNumber == 3) {
      selectCat(3);
      MP3player.playMP3("cat4.mp3");
    }
    if(catNumber == 4){
      selectCat(4);
      MP3player.playMP3("cat5.mp3");
    }
    if(catNumber == 5){
      selectCat(5);
      MP3player.playMP3("cat6.mp3");
    }
    //delay(1000);
  } 
  else
    Serial.print("Sound off!");
}

//Plays a 3 second count down with LED blinks
//Blinks LEDs while we go
void startGame(void) {
  Serial.println();
  Serial.println("Begin new game!");

  Serial.print("Get ready: ");

  //Initially turn off LEDs
  for(int led = 0 ; led < number_of_buttons ; led++)
    controlLED(led, LOW);
  delay(250);

  for(int x = 0 ; x < 3 ; x++) {
    long startTime = millis();

    Serial.print(3 - x);
    Serial.print(",");

    //Turn on all LEDs
    for(int led = 0 ; led < number_of_buttons ; led++)
      controlLED(led, HIGH);

    //Play normal Beep!
    if(digitalRead(audioControl) == LOW) { //Check to see if we should turn off the sounds
      catsOn(); //Turn on all the cats
      MP3player.playMP3("beep.mp3");
    }

    delay(250);

    //Turn off LEDs
    for(int led = 0 ; led < number_of_buttons ; led++)
      controlLED(led, LOW);

    while(millis() - startTime < 500) ; //Wait for a half second each time around
  }

  //Turn on all LEDs
  for(int led = 0 ; led < number_of_buttons ; led++)
    controlLED(led, HIGH);

  //Play long Beep!
/*  if(digitalRead(audioControl) == LOW) //Check to see if we should turn off the sounds
    MP3player.playMP3("Cougar.mp3");
  delay(1000);
  MP3player.stopTrack();*/

  //while(MP3player.isPlaying()) ; //Wait for MP3 to finish playing before turning off all cats
  catsOff(); //Turn off all the cats

  //Turn off LEDs
  for(int led = 0 ; led < number_of_buttons ; led++)
    controlLED(led, LOW);

  Serial.println();
  Serial.println("Go!");

  delay(1000); //Wait one more second before we begin
}

//Pulses the button LEDs on/off to try to entice someone to play with me!!!!
void waitForStart(void) {
  catsOff(); //Turn off the cats while we wait!

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

  sprintf(tempString, "Game over! It took you %d seconds.", totalGameTime/1000);
  Serial.println(tempString);

  sprintf(tempString, "Hit errors: %d", hitErrors);
  Serial.println(tempString);

  long totalScore = totalGameTime + (hitErrors * 1000);
  sprintf(tempString, "Your score: %u", totalScore);
  Serial.println(tempString);

  overallScoreTotal += (long)totalScore; //Add this game to the total mix
  gamesPlayed++; //Increase the game count

  long avgScore = overallScoreTotal / gamesPlayed;

  Serial.print("overallScore: ");
  Serial.print(overallScoreTotal);
  Serial.print(" gamesPlayed: ");
  Serial.print(gamesPlayed);
  Serial.print(" avgScore: ");
  Serial.println(avgScore);

  //Play game music
  if (totalScore < avgScore) {

    //Set the MP3 volume softer so that we don't reset the system
    MP3player.SetVolume(ALLCAT_VOLUME, ALLCAT_VOLUME);

    if(MP3player.isPlaying()) MP3player.stopTrack(); //Stop any previous track

    //Play win music!
    if(digitalRead(audioControl) == LOW) { //Check to see if we should turn off the sounds
      catsOn(); //Turn on all cats
      MP3player.playMP3("win2.mp3");
    }
  }

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

  //Activate the food dispenser for some reward
  if (totalScore < avgScore) dispenseFood();

  catsOff();

}

//Turn on a continuous servo for a brief amount of time
void dispenseFood(void) {
  Serial.println("You win Food!");

  foodServo.attach(25);
  foodServo.writeMicroseconds(3000); //Turn servo counter clockwise
  delay(FOOD_AMOUNT); //The amount of time to adjust the dispenser one spot of food
  foodServo.writeMicroseconds(1500); //Stop servo from turning
  
  delay(1000); //Wait for servo to brake against momentum before detach

  foodServo.detach(); //Unplug servo from system
}

//Turn off all cats
void catsOff(void) {
  digitalWrite(cat0, LOW);
  digitalWrite(cat1, LOW);
  digitalWrite(cat2, LOW);
  digitalWrite(cat3, LOW);
  digitalWrite(cat4, LOW);
  digitalWrite(cat5, LOW);
}

//Turn all the cats (pipes) on at the same time
//Good for broadcasting a message or sound
void catsOn(void) {
  //We have to turn down the MP3 volume so the system doesn't reset
  //Anything less than 60 will cause system reset
  MP3player.SetVolume(ALLCAT_VOLUME, ALLCAT_VOLUME); //System runs with all 6 on

  digitalWrite(cat0, HIGH);
  digitalWrite(cat1, HIGH);
  digitalWrite(cat2, HIGH);
  digitalWrite(cat3, HIGH);
  digitalWrite(cat4, HIGH);
  digitalWrite(cat5, HIGH);
}

//Given the cat #, turn that cat (pipe) and only that cat on
void selectCat(int catNumber) {
  catsOff(); //Turn off all the other cats
  if(catNumber == 0) digitalWrite(cat0, HIGH);
  if(catNumber == 1) digitalWrite(cat1, HIGH);
  if(catNumber == 2) digitalWrite(cat2, HIGH);
  if(catNumber == 3) digitalWrite(cat3, HIGH);
  if(catNumber == 4) digitalWrite(cat4, HIGH);
  if(catNumber == 5) digitalWrite(cat5, HIGH);
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

  //Zero out the board
  for(int x = 0 ; x < number_of_rounds ; x++)
    for(int j = 0 ; j < number_of_buttons ; j++)
      levels[x][j] = 0;

  //Zero out the level times
  for(int x = 0 ; x < number_of_rounds ; x++)
    level_times[x] = 0;

  for(int roundNumber = 0 ; roundNumber < number_of_rounds ; roundNumber++) {

    //For the later rounds, assign more simultaneous buttons
    int spotsToAssign;
    if(roundNumber < 5) spotsToAssign = 1;
    else if(roundNumber < 20) spotsToAssign = 2;
    else if(roundNumber < 25) spotsToAssign = 3;
    else if(roundNumber < 30) spotsToAssign = 4;

    //Fill the game board with buttons
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

    //Assign different round lengths to each round
    level_times[roundNumber] = random(700, 2200); //Set the level time between 1s and 2s

  }

  //Print the array
  Serial.println("Game:");
  for(int x = 0 ; x < number_of_rounds ; x++){
    sprintf(tempString, "Round %02d: ", x);
    Serial.print(tempString);

    for(int j = 0 ; j < number_of_buttons ; j++){
      if(levels[x][j] == 0)
        Serial.print("0 ");
      else
        Serial.print("1 ");
    }

    Serial.print("Round Time: ");
    Serial.print(level_times[x]);

    Serial.println();
  }

}

//Turns on each speaker and LED individually
void testRoutine(void) {
  
  while(1){
    for(int x = 0 ; x < number_of_buttons ; x++) {
      playCat(x);
      controlLED(x, HIGH);
      delay(1500);
      controlLED(x, LOW);
    }
    
  }
  
}







