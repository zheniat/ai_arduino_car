/* MOTOR PINOUT*/
int encoder_pin_right = 2 ; // pulse output from the module
int encoder_pin_left = 3 ; // pulse output from the module

unsigned int motorspeed = 90; //from 0 to 255

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
#define L9110_2A 4 // Pin D6 --> Motor B Input A
#define L9110_2B 5 // Pin D7 --> Motor B Input B


#define MOTOR_LEFT_PWM L9110_2A // Motor PWM Speed
#define MOTOR_LEFT_DIR L9110_2B // Motor Direction

#define MOTOR_RIGHT_PWM L9110_1A // Motor PWM Speed
#define MOTOR_RIGHT_DIR L9110_1B // Motor Direction

/* END MOTOR PINOUT */

/* WHEEL ROTATION CONTROL*/

unsigned long timeold;

volatile byte pulses_left; // number of pulses
volatile byte pulses_right; // number of pulses

// number of pulses per revolution
// based on your encoder disc
unsigned int pulsesperturn = 20;
unsigned int time_gap = 1000;

void counter_left()  // counts from the speed sensor
{
  pulses_left++;  // increase +1 the counter value
} 

void counter_right()  // counts from the speed sensor
{
  pulses_right++;  // increase +1 the counter value
} 


/* END WHEEL ROTATION CONTROL*/

/* CONTROL DATA */
//Data controlling motor rotation, received from the server
//Data format: <100,-1, +1>  - duration ms, left wheel direction (+1 - forward, -1 backward), right wheel direction

const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use when parsing

 // variables to hold the parsed data
char receivedMessage[numChars] = {0};
int duration = 0;
int leftWheel = 0;
int rightWheel = 0;

boolean newData = false;

/*END CONTROL DATA*/

/*DRIVE DURATION*/
unsigned long time_start_drive = 0; 
unsigned long car_drive_time = 0;

/*END DRIVE DURATION*/

void setup() 
{
  Serial.begin(9600);
  
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

  // Initialize
   pulses_left = 0;
   pulses_right = 0;
   
   timeold = 0;
   
  //Interrupt 0 is digital pin 2
   //Triggers on Falling Edge (change from HIGH to LOW)
   attachInterrupt(0, counter_right, FALLING);
   attachInterrupt(1, counter_left, FALLING);

   time_start_drive = 0;
   car_drive_time = -1;

  
  motor_reset();
}

void loop()
{ 
  //Check for new commands from the serial port
  recvDataWithStartEndMarkers();
  if (newData == true) {
      strcpy(tempChars, receivedChars);
          // this temporary copy is necessary to protect the original data
          //   because strtok() used in parseData() replaces the commas with \0
      parseDataAndDrive();      
      newData = false;
   }
   stopCarAndReport();
}

void stopCarAndReport(){
  if (car_drive_time > 0 && (millis() - time_start_drive >= car_drive_time)) {
    car_drive_time = -1;
    motor_reset();
    String result = get_sensor_data();
    Serial.print(result);
  }
}

void recvDataWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;

    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

void parseDataAndDrive() {      // split the data into its parts

    char * strtokIndx; // this is used by strtok() as an index

    strtokIndx = strtok(tempChars,",");// get the first part - duration
    duration = atoi(strtokIndx);     // convert this part to an integer

    if(duration<=0 || duration>30000){
      duration = 1;
    }
    
    strtokIndx = strtok(NULL, ","); //get the second part - left wheel
    leftWheel = atoi(strtokIndx);     // convert this part to an integer
    
    strtokIndx = strtok(NULL, ","); //get the second part - right wheel
    rightWheel = atoi(strtokIndx);  // convert to an integer

    motor_reset();
    
    if(leftWheel>0){
      l_forward();
    } else if(leftWheel<0) {
      l_back();
    }

    if(rightWheel>0){
      r_forward();
    } else if(rightWheel<0){
      r_back();
    }
    //start dring car for the duration
    time_start_drive = millis();
    car_drive_time = duration;
}

//Returns sensor data
//Format: <direction_left, direction_right, rpm_left, rpm_right, dist_front_right, dist_front_left, dist_back_right, dist_back_left>
//Example: <1, -1, 140, 150, 10, 11, 85, 90> - left rpm 140 forward, right rpm 150 backward, distances: 10, 11, 85, and 90
String get_sensor_data(){
  String result = "<";
  if (millis() - timeold >= time_gap) {

    //Don't process interrupts during calculations
    detachInterrupt(0);
    detachInterrupt(1);

    int dir_l = left_forward ? 1 : -1;
    result += dir_l;
    result += ',';
    int dir_r = right_forward ? 1 : -1;
    result += dir_r;
    result += ',';
    float time_diff = millis() - timeold;
    
    float rpm_l = (60 * time_gap / pulsesperturn )/ time_diff * pulses_left;
    float rpm_r = (60 * time_gap / pulsesperturn )/ time_diff * pulses_right;
    
    result +=  roundf(10 * rpm_l) / 10;
    result += ',';
    result += roundf(10 * rpm_r) / 10;
    result += ',';
    
    timeold = millis();
    pulses_left = 0;
    pulses_right = 0;
      
    //Restart the interrupt processing
    attachInterrupt(0, counter_left, FALLING);
    attachInterrupt(1, counter_right, FALLING);

    long dist_fr = get_distance(front_right_trig, front_right_echo);
    long dist_fl = get_distance(front_left_trig, front_left_echo);
    long dist_bl = get_distance(back_left_trig, back_left_echo);
    long dist_br = get_distance(back_right_trig, back_right_echo);
    
    result += dist_fr;
    result += ',';
    result += dist_fl;
    result += ',';
    result += dist_br;
    result += ',';
    result += dist_bl;
  }
  result += +">";
  return result;
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


void l_forward(){
    left_forward = true;
    digitalWrite( MOTOR_LEFT_DIR, LOW ); 
    digitalWrite( MOTOR_LEFT_PWM, HIGH ); 
}

void l_back(){ 
    left_forward = false;
    digitalWrite( MOTOR_LEFT_DIR, HIGH ); 
    digitalWrite( MOTOR_LEFT_PWM, LOW ); 
}


void r_forward(){ 
  right_forward = true;
  digitalWrite( MOTOR_RIGHT_DIR, HIGH); 
  digitalWrite( MOTOR_RIGHT_PWM, LOW); 
}


void r_back(){ 
  right_forward = false;
  digitalWrite( MOTOR_RIGHT_DIR, LOW ); 
  digitalWrite( MOTOR_RIGHT_PWM, HIGH); 
}

