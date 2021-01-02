/*



!!!!!!!!!! I M P O R T A N T !!!!!!!!!!!!!!!
**************************************************
		F  I  R  S  T     O  F     A  L  L
**************************************************
1) calibrate your pH sensor with the solution provided (4,7,10)




-------------------------------------------------------------*/


//WE THANK YOU FOR, CalibrateMeter2(), void readPH()


//--------------------------------//       ***This can be used to simply measure pH
//      pH Regulator/Meter        //       or measure and regulate the pH of a reaction
//                                //    
//  ~  DONALD (DJ) ALEXANDER  ~   //       ***THIS PROGRAM USES A SERIAL INPUT AND 
//                                //       WILL ONLY RUN IF A SERIAL VALUE IS INPUTTED... 
// Regulates high pH for dropping //       TO OPERATE THIS FEATURE, CLICK TOP RIGHT 
//    pH reactions... use base    //       BUTTON (SERIAL MONITOR), INPUT VALUE & PRESS SEND.
//--------------------------------//       use "Autoscroll", "No line ending", and "9600 baud" options

#include <Servo.h> 

//MOTORS
//motors connected to pin 2,3,4
int motorWater = 2, motorAcid=3; motorBase=4, timeOfInjection=0; //tempo che la pompa sarà accesa per iniettare base/acid

//WATER LEVEL MODULE
int waterLvlPin = 12; //Analog reader for water level
float level; //level of water will be store here

//PH SENSOR
int pHpin = 13; //Analog pin for pH sensor
float pHvalue=0, pHneedsToBe=7 /* water pH*/, deltapH=0;

//SOLAR PANEL
int photo1=9, photo2=10, photo3=11, photo4=14; //photoresistors MUST BE PUT ON ANALOG PORTS
const int photoresistors[4] = { photo1, photo2, photo3, photo4 };
float photoResistorValues[4]; //will contain photoresistors value for solar panel movement
Servo servo1, servo2;
const float CONST_photoToServo = 0.17595307917888564;

void setup(){ //RUNS ONCE
	//setup motors
  pinMode(motorWater, OUTPUT);
	pinMode(motorAcid, OUTPUT);
	pinMode(motorBase, OUTPUT);

  //setup solar panel stuff
  pinMode(photo1, INPUT);
  pinMode(photo2, INPUT);
  pinMode(photo3, INPUT);
  pinMode(photo4, INPUT);
  servo1.attach(5);
  servo2.attach(6);
  servo1.write(90);  // set servo to mid-point
  servo2.write(90);  // set servo to mid-point

	Serial.begin(9600);
	Serial.println("\n################# I M P O R T A N T ######################");
	Serial.println("\n\n[ setup ]  Calibrate your pH sensor if nevere done before! Check code for instructions!!!\n\n");
}

void loop(){ //THE LOOP WORKS EVERY 60 SECONDS

  /*
    MOVE SERVO BASED ON PHOTORESISTORS
  */
  moveSolarPanel();

	/*
		INSIDE THE LOOP WE:
		0) fill the circuit with water if you have the module
		1) read ph status
		2) adjust it if needed
		3) pump water flow

	*/

	//# 0 ##################################################################
	//FILL WITH WATER UNTIL LEVEL REACHES OK
	if ( analogRead(waterLvlPin) < minWaterLevel ){ fillWater(); }

	//# 1 ##################################################################
	//READ PH LEVEL
	readPH();
	Serial.println("\n [ loop ] Current pH level: " + pHvalue);

	//# 2 ##################################################################
	//CHECK PH LEVEL + ADJUST IF NEEDED
	checkpH();

	//# 3 ##################################################################
	//ACTIVATE WATER PUMP 1/3 OF THE TIME, 
	for(int i=0; i<30; i++){ //we check the system every 30 minutes
		//1000ms = 1s, 60000ms=60s, 1m=60000ms, 30m = (60000ms*30)

		//we activate the water flow pump 1/3 of the time, 60min/3 = 20min of action!
		if(i % 3 == 0){
			Serial.println("[ loop ]  STARTING water flow!");
			pin(motorWater, HIGH); //OPEN WATER PUMP
		}else{
			Serial.println("[ loop ]  STOPPING water flow!");
			pin(motorWater, LOW); //CLOSE WATER PUMP
		}

		delay(60000); //WAIT FOR 60 seconds at each iteration
	}
}

void moveSolarPanel(){

  //populate array with values from photoresistors
  for(int i=0; i<4; i++){
    photoResistorValues[i] = analogRead(photoresistors[i]);
  }

  /*
  
    photoResistorValues from 0 to 1023
    512 = metà buio/luce
    the darkest, the highest
    
    [ 0, 0, 1000, 1000 ]
      N  S   E      W
    servo needs to move to east and west by 1000
      
    servo.write(from 0 to 180)

    SOOO minVal pr = 0, max = 1023
          minVal sv = 0, max = 180
    180/1023 = 0.17595307917888564
    photoResistorValue * 0.17595307917888564 = conversion to angle of servo 
  */
  //how to write servos properly based on 4 photoResistorValues?
  //servo1 moved by [photo1, photo2]
  //servo2 moved by [photo3, photo4]
  if photoResistorValues[0] > photoResistorValues[1] ? servo1.write( (int) (photoResistorValues[0]*CONST_photoToServo) ) : servo1.write( (int) (photoResistorValues[1]*CONST_photoToServo) );
  if photoResistorValues[2] > photoResistorValues[3] ? servo2.write( (int) (photoResistorValues[2]*CONST_photoToServo) ) : servo2.write( (int) (photoResistorValues[3]*CONST_photoToServo) );

}

void fillWater(){
	//fill the PVC circuit with water

	pin(motorWater, HIGH);

	checklevel:
	if(level > minWaterLevel){
		pin(motorWater, LOW);
		Serial.println("\n [ fill Water ]  Finished filling circuit with water.\n\n");

	}else{
		Serial.println("\n [ fill Water ]  Filling circuit with water ... ");
		goto checklevel;
	}
}

void checkpH(){
	//if pH is more than what we want to be, then activate motors to correct the pH
	if( pHvalue > pHneedsToBe || pHvalue < pHneedsToBe ){
		//if deltapH is -, there is more acid, so we need to add base
		//if deltapH is +, there is more base, so we need to add acid
		deltapH = pHvalue - pHneedsToBe;
		
		if(deltapH > 2 || deltapH < -2){ //if there is too much change (not a subtle change)
			//  ACID   NEUTRO    BASE 
			//0---------- 7 ------------14
			//deltapH<0|     |deltapH>0
			//          .pH.

			//if deltapH > 0 we need to add more acid (so the pHvalue value increases)
			Serial.println("[ check pH ]  There are some heavy changes in system's pH. Delta pH: " + deltapH);
			if deltapH > 0 ? addSolution(motorAcid, deltapH) : addSolution(motorBase, deltapH);
		}else{
			Serial.println("[ check pH ]  There are no heavy changes in system's pH.");
		}
	}
}

// in loop(), there is an if statement that brings us here
// the loop is runned every 60 seconds, so basically we check the phLevel every 60 seconds
void addSolution(motorPin, deltapH){
	
	/*

	1) quanta soluzione serve per ripristinare lo stato del pH alla normalità?
	2) accendo il motore per iniettare quella quantità
	3) spengo il motore

	*/

	//- 1 --------------------------------------------------------------
	//5v pump injects 1,2 - 1,6 L/min
	
	//1200mL - 1600 mL / min
	//per trovare quanti ml al secondo facciamo
	//1200/60 = 20mL/s || 1600/60 +-= 26,66mL/s
	
	/*
		formule chimica per calcolare quanta soluzione si deve iniettare in base al pH
	
		[H^+] = the concentration of the hydrogen ion
		pH = -log[10^-pH]
		[H^+] = 10^-pH

		molar mass of NaHCO3 --- (bicarbonato di sodio): 84,007 g/mol MEANS 1 mol in 84.007 g
		50 g/L of water, 25 °C, pH < 8.6

		molar mass of C36H32Cl4N6O4 --- (lemon juice ): 754.5 g/mol
		average pH = 2.4
		
		volume solution = 1.0L

		1) convert the molar mass into moles ( [grams of substance you put]*[value mol/g] = mol of that substance )

		2) find Molarity of that substance inside 1L ( [mol of that substance]/1L = Molarity of that substance )

		3) find [OH^-] of substance, if the substance is highly base, [OH^-] will be equal to Molarity of that substance

		4) find [H^+] = ([Kw]/[OH^-]) 

	*/

	Serial.println("[ add Solution ]  Trying to adjust system's pH.");
	//quindi calcolo per quanto tempo dovrà essere la pompa accesa (timeOfInjection)
	timeOfInjection = 500; //in 1s we inject 20ml-26.96 ml of solution

	//- 2 ---------------------------------------------------------------
	pin(motorPin, HIGH);
	delay(timeOfInjection);
	//- 3 ---------------------------------------------------------------
	pin(motorPin, LOW);
	//after the injection, we turn off the solution's motor and the code will go back to loop(), which will checkpH() again,
	//and will get back here if needed
}

void CalibrateMeter2()   /*--(Subroutine, calibrates the pH meter using system of equations)----------------------------------------*/
{
                            //clear lcd screen
  Serial.println("Place pH meter in");       //print out instructions for calibration of pH meter
  //lcd.setCursor(0,1);
  Serial.println("pH 4 solution...");
  //lcd.setCursor(0,3);
  Serial.println("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input
                            //flash "WAIT" for 3 secs... subroutine

  delay(50);                            //quick delay
                            //clear lcd screen
  //lcd.setCursor(0,0);                   //print out instructions for calibration of pH meter
  Serial.println("Wait for calibration");    
      
  for (double i=100; i>0; i--)          //read values for 10 seconds
    {
      readPH();                         //read current pH value
      pH4val = pHvalue;                 //set equal to variable for this pH
      //lcd.setCursor(0,2);               //set to 3rd row
      Serial.println("Time remaining: ");    //display time remaining
      Serial.println(i/10, 1);               //calculates the time left for calibration
      //lcd.setCursor(19,2);              //set cursor to second decimal place of time
      Serial.println(" ");                   //erase second decimal place to only display tenths place
      //lcd.setCursor(2,3);               //set to bottom line
      Serial.println("pH Reading: ");        //display current reading
      Serial.println(pHvalue);               //current reading
    }

  delay(50);                            //quick delay
                            //clear lcd screen
  Serial.println("Wash pH meter off");       //print out instructions for cleaning of pH meter
  //lcd.setCursor(0,1);
  Serial.println("with DI water.");
  //lcd.setCursor(0,3);
  Serial.println("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input

  delay(50);                            //quick delay
                            //clear lcd screen
  Serial.println("Place pH meter in");       //print out instructions for calibration of pH meter
  //lcd.setCursor(0,1);
  Serial.println("pH 7 solution...");
  //lcd.setCursor(0,3);
  Serial.println("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input
                            //flash "WAIT" for 3 secs... subroutine

  delay(50);                            //quick delay
                            //clear lcd screen
  //lcd.setCursor(0,0);                   //print out instructions for calibration of pH meter
  Serial.println("Wait for calibration");    
      
  for (double i=100; i>0; i--)          //read values for 10 seconds
    {
      readPH();                         //read current pH value
      pH7val = pHvalue;                 //set equal to variable for this pH
      //lcd.setCursor(0,2);               //set to 3rd row
      Serial.println("Time remaining: ");    //display time remaining
      Serial.println(i/10, 1);               //calculates the time left for calibration
      //lcd.setCursor(19,2);              //set cursor to second decimal place of time
      Serial.println(" ");                   //erase second decimal place to only display tenths place
      //lcd.setCursor(2,3);               //set to bottom line
      Serial.println("pH Reading: ");        //display current reading
      Serial.println(pHvalue);               //current reading
    }

  delay(50);                            //quick delay
                            //clear lcd screen
  Serial.println("Wash pH meter off");       //print out instructions for cleaning of pH meter
  //lcd.setCursor(0,1);
  Serial.println("with DI water.");
  //lcd.setCursor(0,3);
  Serial.println("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input

  delay(50);                            //quick delay
                            //clear lcd screen
  Serial.println("Place pH meter in");       //print out instructions for calibration of pH meter
  //lcd.setCursor(0,1);
  Serial.println("pH 10 solution...");
  //lcd.setCursor(0,3);
  Serial.println("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input
                            //flash "WAIT" for 3 secs... subroutine
  
  delay(50);                            //quick delay
                            //clear lcd screen
  //lcd.setCursor(0,0);                   //print out instructions for calibration of pH meter
  Serial.println("Wait for calibration");    
      
  for (double i=100; i>0; i--)          //read values for 10 seconds
    {
      readPH();                         //read current pH value
      pH10val = pHvalue;                //set equal to variable for this pH
      //lcd.setCursor(0,2);               //set to 3rd row
      Serial.println("Time remaining: ");    //display time remaining
      Serial.println(i/10, 1);               //calculates the time left for calibration
      //lcd.setCursor(19,2);              //set cursor to second decimal place of time
      Serial.println(" ");                   //erase second decimal place to only display tenths place
      //lcd.setCursor(2,3);               //set to bottom line
      Serial.println("pH Reading: ");        //display current reading
      Serial.println(pHvalue);               //current reading
    }

  delay(50);                            //quick delay
                            //clear lcd screen
  Serial.println("Wash pH meter off");       //print out instructions for cleaning of pH meter
  //lcd.setCursor(0,1);
  Serial.println("with DI water.");
  //lcd.setCursor(0,3);
  Serial.println("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input

  delay(50);                            //quick delay
                            //clear lcd screen
  Serial.println("Place pH meter in");       //print out instructions for placing pH meter in solution
  //lcd.setCursor(0,1);
  Serial.println("wanted solution.");
  //lcd.setCursor(0,3);
  Serial.println("When ready input '1'");
      
  while (Serial.available() <= 0)       //while nothing pressed..we don't really need the user to press '1', just any key
    {
      //wait until key is pressed
    }
  wasteByte = Serial.read();            //clear serial input
                            //flash "WAIT" for 3 secs... subroutine

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

