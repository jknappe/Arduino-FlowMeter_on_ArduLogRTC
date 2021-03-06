//===========================================================================================
// PREAMBLE
//-------------------------------------------------------------------------------------------

//Logger type: Arduino Uno on ArduLogRTC shield
 
//Program author: Jan Knappe
//Contact: jan.knappe@tcd.ie

//RTC programming functionality implemented from hoobytronics.co.uk

//===========================================================================================

//===========================================================================================
// PREAMBLE
//-------------------------------------------------------------------------------------------

// INCLUDE LIBRARIES
//---------------------------------------------------------  
#include <Wire.h>                                               //Connecting to RTC
#include <RTClib.h>                                             //Using RTC for timestamp
  RTC_DS1307 RTC;                                               //Define RTC object  
#define RTC_ADDRESS 0x68                                        //Define RTC address
#include <SD.h>                                                 //Communicate with SD card  
  File file;                                                    //Defining File object for SD card

// DEFINE PINS
//---------------------------------------------------------  
const byte LEDPin1 = 5;                                         //LED pin on board
const byte LEDPin2 = 2;                                         //LED pin on button
const byte SDPin = 10;                                          //SD pin (CS/SS)
const byte switchPin = 3;                                       //Reed switch pin 

// DEFINE VARIABLES
//---------------------------------------------------------  
boolean LEDState = false;                                       //Indicate LED on or off
int LEDTimeBlink = 500;                                         //Duration of LED blink
unsigned long LEDTimeOn = 0;                                    //Initialize counter for time LED on
boolean switchEvent = false;                                    //Define default switch status (= disconnected)
const int switchIncrement = 1;                                  //Increment for each switch closure
int switchSum = 0;                                              //Initialize sum of switch closures
boolean LogState = false;                                       //Define trigger to initate logging to data file
int LogInterval = 60;                                           //Define interval for logging to SD card (in seconds)
int LogDelay = 1000;                                            //Define delay to prevent multiple logging events per seconds
unsigned long LastLogTime = 0;                                  //Initialize time since last logging
char command_string[60];                                        //Define string to store RTC command
unsigned char command_index = 0;                                //Define command index
char rtc_datetime[14];                                          //Define string to store RTC time
char i = 0;                                                     //Define index  
//==========================================================================================


//==========================================================================================
// VOID SETUP
//------------------------------------------------------------------------------------------

void setup() {                                                  //START PROGRAM
  Serial.begin(9600);                                           //Set terminal baud rate to 9600  
  
  Serial.println ("==============================");             //Print program name             
  Serial.println ("ArduFlow Tipping Bucket Logger");
  Serial.println ("==============================");             
  
  pinMode(switchPin, INPUT);                                    //Switch pin as input
    digitalWrite(switchPin, HIGH);                              //Activate iternal pullup resistor

// INITIALIZE RTC
//---------------------------------------------------------  
  Wire.begin();                                                 //Start I2C communication

  Serial.print ("Program last compiled: ");                     //Print compilation date
  Serial.print (__DATE__);
  Serial.print (", ");
  Serial.println (__TIME__);
  
  Serial.print(F("Initializing RTC: "));
  
  if (!RTC.begin()) {                                           //IF RTC is not found
    Serial.println(F("RTC not found."));                        //Print error message
    while(1);                                                   //And halt program
  }                                                             //End IF    
  Serial.println(F("successful")); 
  
  if (!RTC.isrunning()) {                                       //IF RTC is not running
    RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));             //Initialize RTC with time of compilation
  }                                                             //End IF  
  
  Serial.print(F("RTC time is set to: "));
  printNowTime();                                               //Print current RTC time
  Serial.println();
  Serial.println(F("To change RTC time, type: date DD/MM/YY HH:MM"));
  
  command_string[0]=0x00;                                       //Initialize input command string    

// INITIALIZE SD CARD
//---------------------------------------------------------  
  pinMode(SDPin, OUTPUT);                                       //Set SDPin as output
    digitalWrite(SDPin, HIGH);                                  //Activate internal pullup resistor
  Serial.print(F("Initializing SD Card: "));
  delay(500);

  if (!SD.begin(SDPin)) {                                       //IF SD is not found
    error("Initializing unsuccessful.");                        //Call error function
  }                                                             //End IF    
  Serial.println("successful.");                                //Otherwise print success
  
// CREATE DATA FILE
//---------------------------------------------------------  
  char filename[] = "LOGGER00.CSV";                             //Create dummy filename 
  for (uint8_t i = 0; i < 100; i++) {                           //FOR 1 to 100
    filename[6] = i/10 + '0';                                   //Create numbered filenames 
    filename[7] = i%10 + '0';                                   //And add NULL as escape character
    
    if (! SD.exists(filename)) {                                //IF filename does not exists  
      file = SD.open(filename, FILE_WRITE);                     //Create and open file 
      break;                                                    //And leave the loop
    }                                                           //End IF 
    
  }                                                             //End FOR

  if (! file) {                                                 //IF file cannot be created, call error function
    error("Could not create data file. Format SD card and try again.");
  }                                                             //End IF 
  
  Serial.print("Logging to: ");
  Serial.println(filename);
  Serial.println("");

// WRITE DATA FILE HEADERS
//---------------------------------------------------------  
  Serial.println("timestamp, tips");                            //write header to serial output
    file.println("timestamp, tips");                            //and data file
  
// BLINK AT START
//---------------------------------------------------------  
  pinMode(LEDPin1, OUTPUT);                                     //LED as output  
  pinMode(LEDPin2, OUTPUT);                                     //LED as output  
  
  for (int i = 1; i < 4; i ++) {                                //FOR 1 to 3
    digitalWrite(LEDPin1, HIGH);                                //Blink LED on/off
    digitalWrite(LEDPin2, HIGH);                                //Blink LED on/off
    delay(LEDTimeBlink);                                        //To indicate start of logging
    digitalWrite(LEDPin1, LOW);
    digitalWrite(LEDPin2, LOW);
    delay(LEDTimeBlink);                                 
  }                                                             //End FOR  
  
}                                                               //End VOID SETUP
//==========================================================================================


//==========================================================================================
// VOID LOOP
//------------------------------------------------------------------------------------------

void loop() {

  digitalWrite(LEDPin1, LEDState);                              //Set LED to LEDState (= on for tip in last 500 ms, off else)
  digitalWrite(LEDPin2, LEDState);                               
  unsigned long DeciSecSinceStart = millis()/100;               //Measure time since program start in deciseconds
  
  if ((unsigned long)(DeciSecSinceStart*100 - LEDTimeOn) >= LEDTimeBlink) {
    LEDState = false;                                           //IF LED blinked for long enough, switch it off
  }                                                             //End IF

  digitalRead(switchPin);                                       //Read switch status
  int switchReading = digitalRead(switchPin);                   //Write switch status to switch Reading variable
  delay(100);                                                   //Wait
  
   if ((switchEvent==false)&&(digitalRead(switchPin)==HIGH)) {  //IF status change from low to high occured
    switchEvent=true;                                           //Set switchEvent TRUE    
  }                                                             //End IF
  
   if ((switchEvent==true)&&(digitalRead(switchPin)==LOW)) {    //IF switch passing detected
      LEDState = true;                                          //Switch LED on
      LEDTimeOn = DeciSecSinceStart*100;                        //And set timer
      switchEvent=false;                                        //Reset switchEvent detector 
      switchSum+=switchIncrement;                               //Increase switch counter
    }                                                           //End IF

  if ((DeciSecSinceStart/10)%LogInterval == 0) {                //If LogInterval is reached
    LogState = true;                                            //Set log flag to TRUE
    
    if ((LogState == true)&&((unsigned long)(DeciSecSinceStart*100 - LastLogTime) <= LogDelay)) { 
      LogState = false;                                         //IF last logging not too long ago set log flag to FALSE
    }                                                           //End IF
    
    if (LogState == true) {                                     //IF log flag is TRUE   
      printNowTime();                                           //Call printNowTime function      
      Serial.print(", ");  
        file.print(", ");        
      Serial.println(switchSum);                                //Print number of switch closures withing logging interval    
        file.println(switchSum); 
      LastLogTime = DeciSecSinceStart*100;                      //Update time of most recent logging event
      LogState = false;                                         //Set log flag to FALSE
      switchSum = 0;                                            //Reset switchSum
    }                                                           //End IF
    
  }                                                             //End IF

  file.flush();                                                 //Close file on SD card

  if (Serial.available() > 0) {                                 //IF serial data (for setting RTC) is available
    char inByte = Serial.read();                                //Safe data in buffer  
    command_string[command_index] = inByte;                     //And write buffer string to receiving string position
      command_index++;                                          //Increase string position index
      
    if (inByte==13) {                                           //IF carriage return is entered (code 13) and command starts with "date" and has 19 characters
      if(command_string[0]=='d' && command_string[1]=='a' && command_string[2]=='t' && command_string[3]=='e' && command_index>18) {
        command_index=0;                                        //Reset string counter 
        for(i=0;i<17;i++) command_string[i]=command_string[i+5];//Remove "date" from string
        setDateTime(command_string);                            //And pass remaining command to setDateTime() function
      }                                                         //End IF
      else {                                                    //If command has not specified structure
        Serial.println("Unknown command.");                     //Print "Unknown command."
      }                                                         //End ELSE
    }                                                           //End IF
      
    if(command_index>=59) command_index=0;                      //IF command is too long, circle to beginning
   
  }                                                             //End IF  
  
}                                                               //End VOID LOOP
//==========================================================================================


//==========================================================================================
// FUNCTION PRINTNOWTIME
//------------------------------------------------------------------------------------------

void printNowTime() {               //To print current RTC time in DD/MM/YYYY HH:MM:SS format
  
  DateTime now = RTC.now();         //Fetch time from RTC

  if (now.day() < 10){              //IF days < 10
    Serial.print(0);                //Print leading zero to serial
      file.print(0);                //and data file
  }                                 //End IF
  Serial.print(now.day(), DEC);     //Print day to serial        
    file.print(now.day(), DEC);     //and data file        
  Serial.print("/");                //Print separator to serial  
    file.print("/");                //and data file 
  if (now.month() < 10){            //IF month < 10
    Serial.print(0);                //Print leading zero to serial
      file.print(0);                //and data file
  }                                 //End IF
  Serial.print(now.month(), DEC);   //Print month to serial   
    file.print(now.month(), DEC);   //and data file  
  Serial.print(F("/"));             //Print separator to serial   
    file.print(F("/"));             //and data file   
  Serial.print(now.year(), DEC);    //Print year to serial    
    file.print(now.year(), DEC);    //and data file  
  Serial.print(F(" "));             //Print separator to serial
    file.print(F(" "));             //and data file
  if (now.hour() < 10){             //IF hour < 10
    Serial.print(0);                //Print leading zero to serial
      file.print(0);                //and data file
  }                                 //End IF
  Serial.print(now.hour(), DEC);    //Print hour to serial
    file.print(now.hour(), DEC);    //and data file
  Serial.print(F(":"));             //Print separator to serial
    file.print(F(":"));             //and data file
  if (now.minute() < 10){           //IF minute < 10
    Serial.print(0);                //Print leading zero to serial
      file.print(0);                //and data file
  }                                 //End IF
  Serial.print(now.minute(), DEC);  //Print minute
    file.print(now.minute(), DEC);  //Print minute
  Serial.print(F(":00"));           //Print separator and seconds
    file.print(F(":00"));           //Print separator and seconds
  
}                                   //End VOID PRINTNOWTIME
//==========================================================================================


//==========================================================================================
// VOID ERROR
//------------------------------------------------------------------------------------------
void error(char const *str) {               //Error function
  Serial.print("error: ");                  //Print to terminal: "error:"
  Serial.println(str);                      //Followed by specific error string and
  digitalWrite(LEDPin1, HIGH);              //Blink LED on
  digitalWrite(LEDPin2, HIGH);                                
  while(1);                                 //Pause the program until reset
}                                           //End VOID ERROR
//==========================================================================================


//==========================================================================================
// VOID SETDATETIME
//------------------------------------------------------------------------------------------
void setDateTime(char datetime[]){          //Setting date and time with serial command
  byte second;                              //Takes input string DD/MM/YY HH:MM:SS
  byte minute;                              //To set RTC
  byte hour;
  byte weekDay;
  byte monthDay;
  byte month;
  byte year;

  monthDay = datetime[0]-48;                //Convert input strings to numbers
  monthDay = monthDay*10 + datetime[1]-48;        
  month = datetime[3]-48;
  month = month*10 + datetime[4]-48;      
  year = datetime[6]-48;
  year = year*10 + datetime[7]-48;           
  hour = datetime[9]-48;
  hour = hour*10 + datetime[10]-48;       
  minute = datetime[12]-48;
  minute = minute*10 + datetime[13]-48;     
  second = 0;
  second = 0;     
  
  Wire.beginTransmission(RTC_ADDRESS);      //Communicate with RTC
    Wire.write(0x00);                       //Stop RTC oscillator
      Wire.write(decToBcd(second));         //Write date and time to RTC using conversion function
      Wire.write(decToBcd(minute));
      Wire.write(decToBcd(hour));
      Wire.write(decToBcd(weekDay));
      Wire.write(decToBcd(monthDay));
      Wire.write(decToBcd(month));
      Wire.write(decToBcd(year));
    Wire.write(0x00);                       //Start RTC oscillator
  Wire.endTransmission();  

  Serial.print(F("RTC time is set to: ")); //Print current RTC time
  printNowTime();                          
  Serial.println();
  
}                                          //End VOID SETDATETIME
//==========================================================================================


//==========================================================================================
// VOID DECTOBCD
//------------------------------------------------------------------------------------------
byte decToBcd(byte val){                   //Convert decimal number to binary coded decimal
  return ( (val/10*16) + (val%10) );
}                                          //End VOID DECTOBCD
//==========================================================================================
