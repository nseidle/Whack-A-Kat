#include <Servo.h> 
 
Servo foodServo;
 
int pos = 0;    // variable to store the servo position 
 
void setup() { 
  foodServo.attach(9);  // attaches the servo on pin 9 to the servo object 
}
 
 
void loop() { 
  foodServo.writeMicroseconds(3000); //Turn servo counter clockwise
  delay(6700); //For two seconds
  foodServo.writeMicroseconds(1500); //Stop servo from turning
  delay(2000);
}
