#include <Servo.h>

int photo1=A5, photo2=A4;
float phdx=0, phsx=0;
const float CONST_photoToServo = 0.17595307917888564;

int degree=0;

String str="";

void setup() {
  Serial.begin(9600);
  pinMode(photo1, INPUT);
  pinMode(photo2, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  str = "";
  phsx = analogRead(photo2);
  str += phsx; //left sensor
  str += " ";
  phdx = analogRead(photo1);
  str += phdx; //right sensor
  
  if( phdx <= phsx ){
    //muovi da 90 a 180°
    degree = (int)( phdx*CONST_photoToServo );
  }else{
    //muovi da 90 a 0
    degree = -(int)( phsx*CONST_photoToServo );
  }
  str += " ";
  str += degree;
  str += "°";
  Serial.println(str);
  
  delay(5000);
}
