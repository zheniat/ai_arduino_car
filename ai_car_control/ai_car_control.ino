int encoder_pin_right = 2 ; // pulse output from the module
int encoder_pin_left = 3 ; // pulse output from the module

unsigned int rpm_left; // rpm reading
unsigned int rpm_right; // rpm reading

volatile byte pulses_left; // number of pulses
volatile byte pulses_right; // number of pulses

unsigned long timeold;

unsigned int motorspeed = 90; //from 0 to 255
unsigned int interrupt_left = 0;
unsigned int interrupt_right = 1;

// number of pulses per revolution
// based on your encoder disc
unsigned int pulsesperturn = 20;
unsigned int time_gap = 1000;

//identifies whether the wheel is rotating forward (True) or backward (False)
boolean left_forward = false;
boolean right_forward = false;

//Ultrasonic sensors
const int front_right_trig = A0;
const int front_right_echo = A1;

const int back_right_trig = A2;
const int back_right_echo = A3;

const int back_left_trig = A4;
const int back_left_echo = A5;

const int front_left_trig = 12;
const int front_left_echo = 13;

// L9110 connections
#define L9110_1A 10 // Pin D10 --> Motor B Input A
#define L9110_1B 11 // Pin D11 --> Motor B Input B
#define L9110_2A 6 // Pin D6 --> Motor B Input A
#define L9110_2B 7 // Pin D7 --> Motor B Input B


#define MOTOR_LEFT_PWM L9110_2A // Motor PWM Speed
#define MOTOR_LEFT_DIR L9110_2B // Motor Direction

#define MOTOR_RIGHT_PWM L9110_1A // Motor PWM Speed
#define MOTOR_RIGHT_DIR L9110_1B // Motor Direction



void counter_left()  // counts from the speed sensor
{
  pulses_left++;  // increase +1 the counter value
} 

void counter_right()  // counts from the speed sensor
{
  pulses_right++;  // increase +1 the counter value
} 

void setup() 
{
  Serial.begin(38400);
  
  pinMode(encoder_pin_left, INPUT);
  pinMode(encoder_pin_right, INPUT);
  
  pinMode(MOTOR_LEFT_PWM, OUTPUT); 
  pinMode(MOTOR_LEFT_DIR, OUTPUT); 

  pinMode(MOTOR_RIGHT_PWM, OUTPUT); 
  pinMode(MOTOR_RIGHT_DIR, OUTPUT); 

  pinMode(front_right_trig, OUTPUT);
  pinMode(front_left_trig, OUTPUT);
  pinMode(back_right_trig, OUTPUT);
  pinMode(back_left_trig, OUTPUT);

  pinMode(front_right_echo, INPUT);
  pinMode(front_left_echo, INPUT);
  pinMode(back_right_echo, INPUT);
  pinMode(back_left_echo, INPUT);
  
  motor_reset();

   // Initialize
   pulses_left = 0;
   pulses_right = 0;
   
   rpm_left = 0;
   rpm_right = 0;
   
   timeold = 0;
   
  //Interrupt 0 is digital pin 2
   //Triggers on Falling Edge (change from HIGH to LOW)
   attachInterrupt(0, counter_right, FALLING);
   attachInterrupt(1, counter_left, FALLING);

    motor_reset();
    //l_forward(200);
    //r_back(100);

}

void loop()
{
  get_sensor_data();
  
  if(Serial.available()){ // Checks whether data is comming from the serial port
      char state = (char)Serial.read(); // Reads the data from the serial port
      Serial.println("BT state: ");
      Serial.print(state);
  } 
}


void get_sensor_data(){

  if (millis() - timeold >= time_gap) {
      //Don't process interrupts during calculations
      detachInterrupt(0);
      detachInterrupt(1);

      rpm_left = (60 * time_gap / pulsesperturn )/ (millis() - timeold)* pulses_left;
      rpm_right = (60 * time_gap / pulsesperturn )/ (millis() - timeold)* pulses_right;
      timeold = millis();
      pulses_left = 0;
      pulses_right = 0;
      Serial.println("RPM left = ");
      Serial.println(rpm_left,DEC);
      Serial.println("Direction left forward = ");
      Serial.println(left_forward,DEC);
      Serial.print("RPM right = ");
      Serial.println(rpm_right,DEC);
      Serial.println("Direction right forward = ");
      Serial.println(right_forward,DEC);
      
      //Restart the interrupt processing
      attachInterrupt(0, counter_left, FALLING);
      attachInterrupt(1, counter_right, FALLING);
      
      long dist_fr = get_distance(front_right_trig, front_right_echo);
      Serial.print("Distance FR: ");
      Serial.println(dist_fr);

      long dist_fl = get_distance(front_left_trig, front_left_echo);
      Serial.print("Distance FL: ");
      Serial.println(dist_fl);

      long dist_bl = get_distance(back_left_trig, back_left_echo);
      Serial.print("Distance BL: ");
      Serial.println(dist_bl);

      long dist_br = get_distance(back_right_trig, back_right_echo);
      Serial.print("Distance BR: ");
      Serial.println(dist_br);

   }  
}


long get_distance(int trig, int echo){
  // Clears the trigPin
  digitalWrite(trig, LOW);
  
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  // Calculating the distance
  int duration = pulseIn(echo, HIGH);
  return duration *0.034/2;
}


void motor_reset(){
  digitalWrite( MOTOR_LEFT_DIR, LOW );
  digitalWrite( MOTOR_LEFT_PWM, LOW );
  digitalWrite( MOTOR_RIGHT_DIR, LOW );
  digitalWrite( MOTOR_RIGHT_PWM, LOW );
}

//checks the speed and makes sure that it is within 0 and 255
int checkSpeed(int speed){
    if(speed < 0) {
      speed = 0; 
    } 

    if(speed > 255){
      speed = 255; 
    }

    return speed;
}



void l_forward(int speed){
    left_forward = true;
    digitalWrite( MOTOR_LEFT_DIR, LOW ); 
    analogWrite( MOTOR_LEFT_PWM, checkSpeed(speed)); 
}

void l_back(int speed){ 
    left_forward = false;
    digitalWrite( MOTOR_LEFT_DIR, HIGH ); 
    analogWrite( MOTOR_LEFT_PWM, checkSpeed(speed)); 
}


void r_forward(int speed){ 
  right_forward = true;
  digitalWrite( MOTOR_RIGHT_DIR, HIGH ); 
  analogWrite( MOTOR_RIGHT_PWM, checkSpeed(speed)); 
}


void r_back(int speed){ 
  right_forward = false;
  digitalWrite( MOTOR_RIGHT_DIR, LOW ); 
  analogWrite( MOTOR_RIGHT_PWM, checkSpeed(speed)); 
}

