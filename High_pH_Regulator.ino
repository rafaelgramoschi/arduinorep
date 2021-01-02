//--------------------------------//       ***This can be used to simply measure pH
//      pH Regulator/Meter        //       or measure and regulate the pH of a reaction
//                                //    
//  ~  DONALD (DJ) ALEXANDER  ~   //       ***THIS PROGRAM USES A SERIAL INPUT AND 
//                                //       WILL ONLY RUN IF A SERIAL VALUE IS INPUTTED... 
// Regulates high pH for dropping //       TO OPERATE THIS FEATURE, CLICK TOP RIGHT 
//    pH reactions... use base    //       BUTTON (SERIAL MONITOR), INPUT VALUE & PRESS SEND.
//--------------------------------//       use "Autoscroll", "No line ending", and "9600 baud" options

/*----------------------------------------------------------------------------------------------------------------------------------*/
/*
   LIST OF HARDWARE:
      Arduino Uno
      Keyboard
      12 V Peristaltic Liquid Pump
      Analog pH Sensor / Meter Pro Kit For Arduino
      I2C 20x4 Arduino LCD Display Module
      IN4001 Diode
      PN2222 Transistor
      12V DC Power Adapter
      
   PIN LAYOUT FOR HARDWARE:                                  
      A4 -------------- to SDA of LCD                                 
      A5 -------------- to SCL of LCD                                 
      GND ------------- to GND of LCD
      5V -------------- to VCC of LCD

      A0 -------------- to middle prong (base) of transistor
      GND ------------- to **left prong (emitter) of transistor           **referred to the flat side of transistor
      (-) of pump ----- to **right prong (collector) of transistor    
      (+) of pump ----- to Vin (12 V)... Arduino connected to outlet by 12V adapter

      1N4001 Diode connected between (-) and (+) prongs of pump with the silver band of diode on (+) side... protects motor

      A3 -------------- to signal wire (blue) of pH meter
      5V -------------- to (+) wire (red) of pH meter
      GND ------------- to (-) wire (black) of pH meter
*/
/*-----( Import needed libraries )-------------------------------------------------------------------------------------------------*/
#include <Wire.h>  
#include <LiquidCrystal_I2C.h>      //include lcd library

/*-----( Declare Constants )-------------------------------------------------------------------------------------------------------*/
int motorPin = A0;            //pin that turns on pump motor
int pHpin = A3;               //pin that sends signals to pH meter
int blinkPin = 13;            //LED pin
float offset = 2.97;          //the offset to account for variability of pH meter
float offset2 = 0;            //offset after calibration
float slope = 0.59;           //slope of the calibration line
int fillTime = 10;            //time to fill pump tubes with acid/base after cleaning... pumps at 1.2 mL/sec
int delayTime = 10;           //time to delay between pumps of acid/base in seconds
int smallAdjust = 1;          //number of seconds to pump in acid/base to adjust pH when pH is off by 0.3-1 pH
int largeAdjust = 3;          //number of seconds to pump in acid/base to adjust pH when pH is off by > 1 pH
int negative = 0;             //indicator if the calibration algorithm should be + or - the offset (if offset is negative or not)

/*-----( Declare objects )---------------------------------------------------------------------------------------------------------*/
LiquidCrystal_I2C lcd (0x20, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  //set the LCD address to 0x20 for a 20 chars and 4 line display

/*-----( Declare Variables )-------------------------------------------------------------------------------------------------------*/
float pHvalue;                //reads the voltage of the pH probe
float pHregulate;             //holds value that the user wants the pH to stay at throughout reaction
float deltaPH;                //holds the value of the difference between pHregulate and pHvalue
float pH4val;                 //value for calibration at pH4
float pH7val;                 //value for calibration at pH7
float pH10val;                //value for calibration at pH10
byte incomingByte;            //reads the user's selection during the menu state
byte wasteByte;               //used to clear Serial input when the user's selection doesn't matter
int statusValue;              //indicates if the user wants a regulator or just a pH reading
int pHavg[10];                //array to find an average pH of 10 meter readings
int temp;                     //temporary place holder used to sort array from small to large
unsigned long int avgValue;   //stores the average value of the 6 middle pH array readings

/*----------------------------------------------------------------------------------------------------------------------------------*/
void setup()   /*----( SETUP: RUNS ONCE )-------------------------------------------------------------------------------------------*/
{
  pinMode(motorPin, OUTPUT);          //set A0 as an output so transistor for pump can be turned on
  pinMode(blinkPin, OUTPUT);          //set 13 to an output so that the LED can be turned on
  lcd.begin(20, 4);                   //initialize lcd screen 20 x 4
  Serial.begin(9600);                 //initialize serial communication

  lcd.setCursor(0,0);                 //set cursor to first row, first column
  lcd.print("1) pH Regulator");       //display "1) pH Regulator" on lcd
  lcd.setCursor(0,1);                 //set cursor to second row down, first column
  lcd.print("2) pH Meter");           //display "2) pH Meter" on lcd
  lcd.setCursor(0,3);                 //set cursor to second row down, first column
  lcd.print("(Use Serial Monitor)");  //tell user to use serial monitor to input choice
  
  while (Serial.available() <= 0)     //if <= 0, there is no input
    {
      //wait until there is incoming serial data
    }

  incomingByte = Serial.read();       //read the value sent from keyboard

  if (incomingByte == '1')            //if '1' is pressed (pH Regulator)
    {
      statusValue = 1;                //user wants to use the regulator, status 1.. variable stored for later in the code
      delay(50);                      //quick delay
      CleanPump();                    //clean out pump with DI water... subroutine
      SetUpPump();                    //prepares pump for use by filling with solution... subroutine
    }
  else if (incomingByte == '2')       //if '2' is pressed (pH Meter)
    {
      statusValue = 2;                //user wants to use just the meter, status 2.. variable stored for later in the code
    }
  else                                //if something other than '1' or '2' is pressed
    {
      Serial.println("Invalid Input: Press '1' or '2'");      //output warning directions for invalid entry
      setup();                                                //restart the setup code to allow another input
    }

  delay(50);                          //quick delay
  
again:
  lcd.clear();                        //clear lcd screen
  lcd.setCursor(0,0);                 //set cursor to first row, first column
  lcd.print("Does meter need");       //display "Does meter need" on lcd
  lcd.setCursor(0,1);                 //set cursor to second row down, first column
  lcd.print(" calibration?");         //display "calibration?" on lcd
  lcd.setCursor(3,2);                 //set cursor to third row down, third column
  lcd.print("Yes? Enter '1'");        //calibration choice
  lcd.setCursor(3,3);                 //set cursor to fourth row down, third column
  lcd.print("No?  Enter '2'");        //calibration choice
  
  while (Serial.available() <= 0)     //if <= 0, there is no input
    {
      //wait until there is incoming serial data
    }

  incomingByte = Serial.read();       //read the value sent from keyboard

  if (incomingByte == '1')            //if '1' is pressed (calibration needed)
    {
      //CalibrateMeter();               //calibrate the pH meter by potentiometer adjustment... subroutine
      CalibrateMeter2();              //calibrates pH meter by software algorithm... subroutine
      delay(500);                     //delay half second
    }
  else if (incomingByte == '2')       //if '2' is pressed (no calibration needed)
    {
      delay(50);                            //quick delay
      lcd.clear();                          //clear lcd screen
      lcd.print("Wash pH meter off");       //print out instructions for cleaning of pH meter
      lcd.setCursor(0,1);
      lcd.print("with DI water.");
      lcd.setCursor(0,3);
      lcd.print("When ready input '1'");
      
      while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
       {
         //wait until key is pressed
       }
      wasteByte = Serial.read();            //clear serial input

      delay(50);                            //quick delay
      lcd.clear();                          //clear lcd screen
      lcd.print("Place pH meter in");       //print out instructions for placing pH meter in solution
      lcd.setCursor(0,1);
      lcd.print("wanted solution.");
      lcd.setCursor(0,3);
      lcd.print("When ready input '1'");
      
      while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
        {
          //wait until key is pressed
        }
      wasteByte = Serial.read();            //clear serial input
      FlashWait();                          //flash "WAIT" for 3 secs... subroutine
    }
  else                                      //if something other than '1' or '2' is pressed
    {
      Serial.println("Invalid Input: Press '1' or '2'");      //output warning directions for invalid entry
      goto again;                                             //restart calibration question
    }

  lcd.clear();                              //clear screen

inputPH:
  if (statusValue == 1)                     //if '1' was pressed (pH Regulator)
   {
      lcd.setCursor(0,0);                   //set cursor to first row, first column
      lcd.print("Enter pH Value you");      //display "Enter pH Value you" on lcd
      lcd.setCursor(0,1);                   //set cursor to second row down, first column
      lcd.print("would like reaction");     //display "would like reaction" on lcd
      lcd.setCursor(0,2);                   //set cursor to third row down, first column
      lcd.print("to stay at...");           //display "to stay at." on lcd
      
      while (Serial.available() <= 0)       //if <= 0, there is no input
        {
          //wait until there is incoming serial data
          readPH();                                   //read the current pH value
          lcd.setCursor(0,3);                         //set cursor to third row down, first column
          lcd.print("  Current pH: ");                //display "Current pH: " on lcd
          lcd.print(pHvalue);                         //display current pH   
          lcd.print(" ");                             //clears extra digit if number before is > 10                          
          delay(100);                                 //delay for lcd timing
        }

      pHregulate = Serial.parseFloat();       //converts the ASCII char from serial input into a float pH value
      delay(50);                              //short delay
      lcd.clear();                            //clear screen
      lcd.setCursor(0,0);                     //set cursor to first row down, first column
      lcd.print("You have entered:");         //display "You have entered:" on lcd
      lcd.setCursor(6,1);                     //set cursor to second row down, seventh column
      lcd.print("pH ");                       //display "pH " on lcd
      lcd.print(pHregulate);                  //display value inputted on lcd
      lcd.setCursor(0,2);                     //set cursor to third row down, first column
      lcd.print("Correct? Enter '1'");        //display "Correct? Enter '1'" on lcd
      lcd.setCursor(0,3);                     //set cursor to fourth row down, first column
      lcd.print("New pH?  Enter '2'");        //display "New pH? Enter '2'" on lcd

  redo:
      while (Serial.available() <= 0)     //if <= 0, there is no input
        {
          //wait until there is incoming serial data
        }

      incomingByte = Serial.read();       //read the value sent from keyboard

      if (incomingByte == '1')            //if '1' is pressed (correct pH)
        {
          lcd.clear();                    //clear screen
        }
      else if (incomingByte == '2')       //if '2' is pressed (pH Meter)
        {
          lcd.clear();                    //clear lcd
          goto inputPH;                   //go back to new input
        }
      else                                //if something other than '1' or '2' is pressed
        {
          Serial.println("Invalid Input: Press '1' or '2'");      //output warning directions for invalid entry
          goto redo;                                              //reinput choice
        }
   }
}/* --(end setup)-- */
 
void loop()   /*----( LOOP: RUNS CONSTANTLY )---------------------------------------------------------------------------------------*/
{
  //this code will operate for both regulator and meter states... code within "if" statement only operates under regulator state
  delay(100);                       //small delay
  readPH();                         //read current value of pH                      
  lcd.setCursor(2,1);               //set cursor to second row down, third column
  lcd.print("pH Level = ");         //display "pH Level = " on lcd
  lcd.print(pHvalue);               //display value of pH
  lcd.print(" ");                   //clears extra digit if number before is > 10
  
  if (statusValue == 1)                   //if user wants regulator
    {
      deltaPH = (pHregulate - pHvalue);   //difference between expected pH and actual pH
//  recheck:
      if (deltaPH > 0.3)                  //if difference is greater than 0.3 pH away on low side
        {
          digitalWrite(motorPin, HIGH);   //turn on the pump motor
          digitalWrite(blinkPin, HIGH);   //turn on the LED
          
          if (deltaPH > 1)                //if difference in pH is extreme (more than 1 under expected)
            {
              for(int i=0; i < largeAdjust; i++)    //loop for larger pH adjust
                {
                  readPH();                         //check pH
                  lcd.setCursor(2,1);               //set cursor to second row down, third column
                  lcd.print("pH Level = ");         //display "pH Level = " on lcd
                  lcd.print(pHvalue);               //display value of pH
                  lcd.print(" ");                   //clears extra digit if number before is > 10
//                  deltaPH = (pHregulate - pHvalue); //difference between expected pH and actual pH
//                  if (deltaPH < 1)
//                    {
//                      digitalWrite(motorPin, LOW);  //turn off motor
//                      digitalWrite(blinkPin, LOW);  //turn off LED
//                      goto recheck;                 //recheck how much to pump
//                    }
                  lcd.setCursor(2,2);               //center of screen
                  lcd.print("Regulator Active");    //print "Regulator Active"
                  delay(500);                       //half second flash
                  lcd.setCursor(2,2);               //center of screen
                  lcd.print("                ");    //clear "Regulator Active"
                  delay(500);                       //half second flash
                }
            }
          else                                      //if difference in pH is between 0.3 and 1
            {
              for(int i=0; i < smallAdjust; i++)    //loop for smaller pH adjust time
                {
                  readPH();                         //check pH
                  lcd.setCursor(2,1);               //set cursor to second row down, third column
                  lcd.print("pH Level = ");         //display "pH Level = " on lcd
                  lcd.print(pHvalue);               //display value of pH
                  lcd.print(" ");                   //clears extra digit if number before is > 10
//                  deltaPH = (pHregulate - pHvalue); //difference between expected pH and actual pH
//                  if (deltaPH < 0.3)
//                    {
//                      digitalWrite(motorPin, LOW);  //turn off motor
//                      digitalWrite(blinkPin, LOW);  //turn off LED
//                      goto recheck;                 //recheck how much to pump
//                    }
                  lcd.setCursor(2,2);               //center of screen
                  lcd.print("Regulator Active");    //print "Regulator Active"
                  delay(250);                       //quarter second flash
                  lcd.setCursor(2,2);               //center of screen
                  lcd.print("Regulator Active");    //print "Regulator Active"
                  delay(250);                       //quarter second flash
                  lcd.setCursor(2,2);               //center of screen
                  lcd.print("Regulator Active");    //print "Regulator Active"
                  delay(250);                       //quarter second flash
                  lcd.setCursor(2,2);               //center of screen
                  lcd.print("Regulator Active");    //print "Regulator Active"
                  delay(250);                       //quarter second flash
                }
            }

          digitalWrite(motorPin, LOW);              //turn off motor
          digitalWrite(blinkPin, LOW);              //turn off LED       
          for(int i=0; i < delayTime*4; i++)        //delay "delayTime" seconds to allow new solution to mix
                {
                  readPH();                         //check pH
                  lcd.setCursor(2,1);               //set cursor to second row down, third column
                  lcd.print("pH Level = ");         //display "pH Level = " on lcd
                  lcd.print(pHvalue);               //display value of pH
                  lcd.print(" ");                   //clears extra digit if number before is > 10
                  lcd.setCursor(2,2);               //center of screen
                  lcd.print("Delay for mixing");    //print "Delay for mixing"
                  delay(100);                       //small delay
                }
          lcd.clear();                              //clear lcd
        }
    }
  
}/* --(end main loop )-- */

/*----------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------(SUBROUTINES)---------------------------------------------------------------------*/

void readPH()     /*--(Subroutine, reads current value of pH Meter)-----------------------------------------------------------------*/
{
  for(int i=0; i<10; i++)                   //get 10 sample values from the sensor to smooth the value
    { 
      pHavg[i] = analogRead(pHpin);         //get reading from pH sensor and put in array
      delay(10);                            //short delay between readings
    }
    
  for(int i=0; i<9; i++)                    //sort the analog values from small to large
    {
      for(int j=i+1; j<10; j++)             
        {
          if(pHavg[i] > pHavg[j])           //if "i" value of array is bigger than "j" value
            {
              temp = pHavg[i];              //assign "i" to temporary variable
              pHavg[i] = pHavg[j];          //switch "j" to "i" location
              pHavg[j] = temp;              //switch "i" to "j" location
            }
        }
    }
  avgValue = 0;
  for(int i=2; i<8; i++)                    //take the value total of 6 center array values
    {
      avgValue += pHavg[i];                 //get total
    }
  pHvalue = (float)avgValue*5.0/1024/6;     //map the analog (0-1023) into millivolt (0-5).. division by 6 for average

  if (negative == 0)                        //if the offset is positive... see calibration subroutine.. negative is initialized as 0
    {
      pHvalue = (slope*3.5*pHvalue + offset + offset2); //convert the millivolt into pH value, with positive offset and slope from calibration
    }
  else
    {
      pHvalue = (slope*3.5*pHvalue - offset + offset2); //convert the millivolt into pH value, with negative offset and slope from calibration
    }
}

void CleanPump()    /*--(Subroutine, cleans pump w/ DI water)-----------------------------------------------------------------------*/
{
  lcd.clear();                          //clear lcd screen
  lcd.print("Place pump tube in");      //print out instructions for cleaning pump before use
  lcd.setCursor(0,1);
  lcd.print("DI water & dispenser");
  lcd.setCursor(0,2);
  lcd.print("tube in waste glass.");
  lcd.setCursor(0,3);
  lcd.print("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input
  delay(500);                           //delay half second
  lcd.clear();                          //clear screen
  lcd.setCursor(8,1);                   //center of screen
  lcd.print("WAIT");                    //print wait
  digitalWrite(motorPin, HIGH);         //turn on the pump motor
  digitalWrite(blinkPin, HIGH);         //turn on the LED
  for(int i=0; i<15; i++)               //loop for 15 secs
    {
      lcd.setCursor(5,2);               //center of screen
      lcd.print("Cleaning...");         //print cleaning warning
      delay(500);                       //half second flash
      lcd.setCursor(5,2);               //center of screen
      lcd.print("           ");         //clear "cleaning..."
      delay(500);                       //half second flash
    }
  digitalWrite(motorPin, LOW);          //turn off the pump motor
  digitalWrite(blinkPin, LOW);          //turn off the LED
      
  lcd.clear();                          //clear screen
  lcd.print("Take pump tube out");      //instructions to clear DI water out of tubes
  lcd.setCursor(0,1);
  lcd.print("of DI water and keep");
  lcd.setCursor(0,2);
  lcd.print("dispenser in glass.");
  lcd.setCursor(0,3);
  lcd.print("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input
  delay(500);                           //delay half second
  lcd.clear();                          //clear screen
  lcd.setCursor(8,1);                   //center of screen
  lcd.print("WAIT");                    //print wait
  digitalWrite(motorPin, HIGH);         //turn on the pump motor
  digitalWrite(blinkPin, HIGH);         //turn on the LED
  for(int i=0; i<10; i++)               //loop for 10 secs
    {
      lcd.setCursor(5,2);               //center of screen
      lcd.print("Cleaning...");         //print cleaning warning
      delay(500);                       //half second flash
      lcd.setCursor(5,2);               //center of screen
      lcd.print("           ");         //clear "cleaning..."
      delay(500);                       //half second flash
    }
  digitalWrite(motorPin, LOW);          //turn off the pump motor
  digitalWrite(blinkPin, LOW);          //turn off the LED
}

void SetUpPump()      /*--(Subroutine, fills pump with solution)--------------------------------------------------------------------*/
{
  lcd.clear();                          //clear lcd screen
  lcd.print("Place pump tube in");      //print out instructions for setting up pump before use
  lcd.setCursor(0,1);
  lcd.print("acid/base & dispense");
  lcd.setCursor(0,2);
  lcd.print("tube in reaction.");
  lcd.setCursor(0,3);
  lcd.print("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input
  delay(500);                           //delay half second

  lcd.clear();                          //clear screen
  lcd.setCursor(8,1);                   //center of screen
  lcd.print("WAIT");                    //print wait
  digitalWrite(motorPin, HIGH);         //turn on the pump motor
  digitalWrite(blinkPin, HIGH);         //turn on the LED
  for(int i=0; i < fillTime; i++)       //loop for enough time to fill tubes with acid/base
    {
      lcd.setCursor(3,2);               //center of screen
      lcd.print("Filling Pump...");     //print fill warning
      delay(500);                       //half second flash
      lcd.setCursor(3,2);               //center of screen
      lcd.print("               ");     //clear "cleaning..."
      delay(500);                       //half second flash
    }
  digitalWrite(motorPin, LOW);          //turn off the pump motor
  digitalWrite(blinkPin, LOW);          //turn off the LED
      
  lcd.clear();                          //clear screen
}

void CalibrateMeter()   /*--(Subroutine, calibrates the pH meter w/ potentiometer)--------------------------------------------------*/
{
  lcd.clear();                          //clear lcd screen
  lcd.print("Place pH meter in");       //print out instructions for calibration of pH meter
  lcd.setCursor(0,1);
  lcd.print("pH 4 solution...");
  lcd.setCursor(0,3);
  lcd.print("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input
  FlashWait();                          //flash "WAIT" for 3 secs... subroutine

  delay(50);                            //quick delay
  lcd.clear();                          //clear lcd screen
  lcd.print("Turn ADJ on pH chip");     //print out instructions for calibration of pH meter
  lcd.setCursor(0,1);
  lcd.print("until reading is pH4");
  lcd.setCursor(0,3);
  lcd.print("When done input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      readPH();                         //read current pH value
      lcd.setCursor(2,2);
      lcd.print("pH Value = ");
      lcd.print(pHvalue);
      lcd.print(" ");                   //clears extra digit if number before is > 10
      delay(100);
    }
  wasteByte = Serial.read();            //clear serial input

  delay(50);                            //quick delay
  lcd.clear();                          //clear lcd screen
  lcd.print("Wash pH meter off");       //print out instructions for cleaning of pH meter
  lcd.setCursor(0,1);
  lcd.print("with DI water.");
  lcd.setCursor(0,3);
  lcd.print("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input

  delay(50);                            //quick delay
  lcd.clear();                          //clear lcd screen
  lcd.print("Place pH meter in");       //print out instructions for calibration of pH meter
  lcd.setCursor(0,1);
  lcd.print("pH 7 solution...");
  lcd.setCursor(0,3);
  lcd.print("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input
  FlashWait();                          //flash "WAIT" for 3 secs... subroutine
  
  delay(50);                            //quick delay
  lcd.clear();                          //clear lcd screen
  lcd.print("Turn ADJ on pH chip");     //print out instructions for calibration of pH meter
  lcd.setCursor(0,1);
  lcd.print("until reading is pH7");
  lcd.setCursor(0,3);
  lcd.print("When done input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      readPH();                         //read current pH value 
      lcd.setCursor(2,2);
      lcd.print("pH Value = ");
      lcd.print(pHvalue);
      lcd.print(" ");                   //clears extra digit if number before is > 10
      delay(100);
    }
  wasteByte = Serial.read();            //clear serial input  

  delay(50);                            //quick delay
  lcd.clear();                          //clear lcd screen
  lcd.print("Wash pH meter off");       //print out instructions for cleaning of pH meter
  lcd.setCursor(0,1);
  lcd.print("with DI water.");
  lcd.setCursor(0,3);
  lcd.print("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input

  delay(50);                            //quick delay
  lcd.clear();                          //clear lcd screen
  lcd.print("Place pH meter in");       //print out instructions for placing pH meter in solution
  lcd.setCursor(0,1);
  lcd.print("wanted solution.");
  lcd.setCursor(0,3);
  lcd.print("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input
  FlashWait();                          //flash "WAIT" for 3 secs... subroutine
}

void FlashWait()      /*--(Subroutine, flashes "WAIT" for a delay)------------------------------------------------------------------*/
{
  for(int i=0; i<3; i++)                //loop for 3 secs
    {
      lcd.setCursor(8,2);               //center of screen
      lcd.print("WAIT");                //print wait
      delay(500);                       //half second flash
      lcd.setCursor(8,2);               //center of screen
      lcd.print("           ");         //clear "WAIT"
      delay(500);                       //half second flash
    }
}

void CalibrateMeter2()   /*--(Subroutine, calibrates the pH meter using system of equations)----------------------------------------*/
{
  lcd.clear();                          //clear lcd screen
  lcd.print("Place pH meter in");       //print out instructions for calibration of pH meter
  lcd.setCursor(0,1);
  lcd.print("pH 4 solution...");
  lcd.setCursor(0,3);
  lcd.print("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input
  FlashWait();                          //flash "WAIT" for 3 secs... subroutine

  delay(50);                            //quick delay
  lcd.clear();                          //clear lcd screen
  lcd.setCursor(0,0);                   //print out instructions for calibration of pH meter
  lcd.print("Wait for calibration");    
      
  for (double i=100; i>0; i--)          //read values for 10 seconds
    {
      readPH();                         //read current pH value
      pH4val = pHvalue;                 //set equal to variable for this pH
      lcd.setCursor(0,2);               //set to 3rd row
      lcd.print("Time remaining: ");    //display time remaining
      lcd.print(i/10, 1);               //calculates the time left for calibration
      lcd.setCursor(19,2);              //set cursor to second decimal place of time
      lcd.print(" ");                   //erase second decimal place to only display tenths place
      lcd.setCursor(2,3);               //set to bottom line
      lcd.print("pH Reading: ");        //display current reading
      lcd.print(pHvalue);               //current reading
    }

  delay(50);                            //quick delay
  lcd.clear();                          //clear lcd screen
  lcd.print("Wash pH meter off");       //print out instructions for cleaning of pH meter
  lcd.setCursor(0,1);
  lcd.print("with DI water.");
  lcd.setCursor(0,3);
  lcd.print("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input

  delay(50);                            //quick delay
  lcd.clear();                          //clear lcd screen
  lcd.print("Place pH meter in");       //print out instructions for calibration of pH meter
  lcd.setCursor(0,1);
  lcd.print("pH 7 solution...");
  lcd.setCursor(0,3);
  lcd.print("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input
  FlashWait();                          //flash "WAIT" for 3 secs... subroutine

  delay(50);                            //quick delay
  lcd.clear();                          //clear lcd screen
  lcd.setCursor(0,0);                   //print out instructions for calibration of pH meter
  lcd.print("Wait for calibration");    
      
  for (double i=100; i>0; i--)          //read values for 10 seconds
    {
      readPH();                         //read current pH value
      pH7val = pHvalue;                 //set equal to variable for this pH
      lcd.setCursor(0,2);               //set to 3rd row
      lcd.print("Time remaining: ");    //display time remaining
      lcd.print(i/10, 1);               //calculates the time left for calibration
      lcd.setCursor(19,2);              //set cursor to second decimal place of time
      lcd.print(" ");                   //erase second decimal place to only display tenths place
      lcd.setCursor(2,3);               //set to bottom line
      lcd.print("pH Reading: ");        //display current reading
      lcd.print(pHvalue);               //current reading
    }

  delay(50);                            //quick delay
  lcd.clear();                          //clear lcd screen
  lcd.print("Wash pH meter off");       //print out instructions for cleaning of pH meter
  lcd.setCursor(0,1);
  lcd.print("with DI water.");
  lcd.setCursor(0,3);
  lcd.print("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input

  delay(50);                            //quick delay
  lcd.clear();                          //clear lcd screen
  lcd.print("Place pH meter in");       //print out instructions for calibration of pH meter
  lcd.setCursor(0,1);
  lcd.print("pH 10 solution...");
  lcd.setCursor(0,3);
  lcd.print("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input
  FlashWait();                          //flash "WAIT" for 3 secs... subroutine
  
  delay(50);                            //quick delay
  lcd.clear();                          //clear lcd screen
  lcd.setCursor(0,0);                   //print out instructions for calibration of pH meter
  lcd.print("Wait for calibration");    
      
  for (double i=100; i>0; i--)          //read values for 10 seconds
    {
      readPH();                         //read current pH value
      pH10val = pHvalue;                //set equal to variable for this pH
      lcd.setCursor(0,2);               //set to 3rd row
      lcd.print("Time remaining: ");    //display time remaining
      lcd.print(i/10, 1);               //calculates the time left for calibration
      lcd.setCursor(19,2);              //set cursor to second decimal place of time
      lcd.print(" ");                   //erase second decimal place to only display tenths place
      lcd.setCursor(2,3);               //set to bottom line
      lcd.print("pH Reading: ");        //display current reading
      lcd.print(pHvalue);               //current reading
    }

  delay(50);                            //quick delay
  lcd.clear();                          //clear lcd screen
  lcd.print("Wash pH meter off");       //print out instructions for cleaning of pH meter
  lcd.setCursor(0,1);
  lcd.print("with DI water.");
  lcd.setCursor(0,3);
  lcd.print("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input

  delay(50);                            //quick delay
  lcd.clear();                          //clear lcd screen
  lcd.print("Place pH meter in");       //print out instructions for placing pH meter in solution
  lcd.setCursor(0,1);
  lcd.print("wanted solution.");
  lcd.setCursor(0,3);
  lcd.print("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input
  FlashWait();                          //flash "WAIT" for 3 secs... subroutine

  slope = 6 /(pH10val - pH4val);                  //System of equations to make each reading equal pH 4 & 10 respectively shown below:
                                                  //       10=(pH10val*slope)+offset    -    4=(pH4val*slope)+offset
                                                  //This system of equations creates a straight line trend for all pH readings
  
  offset = (abs(11 - ((pH4val + pH7val)*slope)))/2; //S.O.E. using point at pH 7 and pH4/10 slope to ensure a best fit, below:
                                                    //         4=(pH4val*slope)+offset    +    7=(pH7val*slope)+offset
                                                    //slope and offset solved to create best fit line approximation

  offset2 = slope*2.97;                             //multiply by old offset value, new slope times old offset
  slope = 0.59*slope;                               //new slope * old slope... "offset2" and new "slope" are used for the following:
                                                    //calibrated pH = (old slope*3.5*pHvalue + old offset)*new slope + new offset
                                                    //              = (old pH reading)*slope + offset
                                                    //see 'readPH()' for application of this equation
  
  if ((pH4val + pH7val) > 11)                       //if total of pH4 and pH7 reading is greater than 11
    {     
      negative = 1;                                 //set negative to hold value of 1 to change algorithm of pHvalue..offset is < 0
    }
}
/* ( THE END ) */


