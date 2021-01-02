#include <Servo.h>

String str = ""; //variabile d'appoggio

//int greenled = 9;
//int blueled = 10;
//const float CONST_photoToLed = 0.24926686217008798; //1023(photoresistor) max value TO 255(analogWrite) max value

int photo1=A5, photo2=A4; //photoresistors MUST BE PUT ON ANALOG PORTS
float photoResistorValues[2]; //will contain photoresistors value for solar panel movement
int photoresistors[2] = {photo1, photo2};
float phdx=0, phsx=0;
const float CONST_photoToServo = 0.17595307917888564;

Servo servo1;
int degree=0;

void setup() {
  Serial.begin(9600);
  //pinMode(pompWater, OUTPUT);
  servo1.attach(5);
  servo1.write(90);
  pinMode(photo1, INPUT);
  pinMode(photo2, INPUT);
}

void loop(){
  moveSolarPanel();
  delay(1500);
}

void moveSolarPanel(){

  //populate array with values from photoresistors
  for(int i=0; i<2; i++){
    photoResistorValues[i] = analogRead(photoresistors[i]);
  }
  str = "Right resistor: ";
  str += photoResistorValues[0];
  Serial.println( str );
  str = "Left  resistor: ";
  str += photoResistorValues[1];
  Serial.println( str );
  
  //analogWrite(greenled, (int) (  (1023-photoResistorValues[0])*CONST_photoToLed )  );
  //analogWrite(blueled, (int) (  (1023-photoResistorValues[1])*CONST_photoToLed )  );

    //photoResistorValues[0] (destra)
    // 0 * CONST_photoToServo = 0
    // 511.5 * CONST_photoToServo = 90
    //questo gira da 0 a 90°
    
    // gira da 90° a 180°
    // 517 * CONST_photoToServo +-= 91°
    // 1023 * CONST_photoToServo = 180
    // esempio:
    // sensore con valore 511, gira di 90° ma io voglio che giri di -90° 

  degree = 0;
  
  phdx = photoResistorValues[0]; //destra
  phsx = photoResistorValues[1]; //sinistra
  //più il valore è basso e più c'è luce

    //se c'è più luce a destra (significa da 0 a 516)
    //muovi da 90 a 180
    if( phdx <= phsx ){
      //muovi da 90 a 180°
      degree = (int)( phdx*CONST_photoToServo );
    }
    
    //se c'è più luce a sinistra (significa da 0 a 516)
    //muovi da 90 a 0
    if( phsx < phdx ){
      //muovi da 90 a 0
      degree = -(int)( phsx*CONST_photoToServo );
    }
    str = "Degree ";
    str += degree;
    Serial.println(str);
    servo1.write(degree);
    
  Serial.println("----------------\n");
}
