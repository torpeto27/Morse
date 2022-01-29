/*
 * The Following Code was originally written to use a SS Micro as a simple code practice oscillator, but has been adapted
 * to work with the Teensy++ 2.0 board.
 * (Other Arduino platforms should work as well)
 * The input keying method can be either a straight Key, or a "Paddle" type Key
 * With the paddle key input the oscilator supports Iambic "A" mode.
 * Note the straight key uses a seperate input pin (D0) from the paddle inputs (D1 & D2).
 * For output the Micro can directly drive a 8 ohm speaker wired in serries with 200 ohms (pins D15 & 14)
 * Speaker volume wired this way is not loud, but ample for most code practice envirnoments
 * NOTE: Teensy++ 2.0 requires the Teensyduino add-on to the Arduino IDE.  VSCode CAN NOT be configured for this board at this time.
 * 
 * Code Practice Oscillator; Aka: The Three Arduinos, https://www.youtube.com/watch?v=U1LHWX86INQ
 * Teensyduino: https://www.pjrc.com/teensy/td_download.html
 */
int wpm =12;// theorecctical sending speed ; measured in Words per Minute 
bool DitLast = false;
bool Running = false;
bool DitActive = false;
bool DahActive = false;
bool SpaceActive = false;
bool padDit = false;
bool padDah = false;
bool spdSwNO = true; // change to 'true' if your push button speed switches are are mormally open, or you're not using the speed change function
int DitPeriod; //measured in MilliSeonds; actual value calculated in SetUp section using WPM value
int TonePeriod;//measured in MicroSeconds; the final value is set based on the toneFerq value
int toneFreq = 600; //measured in hertz
int spdChngDelay=0;
unsigned long End;
unsigned long ok2Chk;

void setup() {
  //start serial connection
  //Serial.begin(9600); // for testing only
  //Configure the Speaker Pins, Digital Pins 15 & 16, as Outputs
  //Use 200 ohms resistance between speaker outputs to limit current
  pinMode(15, OUTPUT);
  digitalWrite(15, LOW);
  pinMode(16, OUTPUT);
  digitalWrite(16, LOW);
   //configure Key and Paddle Digital pins 0, 1 & 2 as inputs, and enable the internal pull-up resistor
  pinMode(2, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(0, INPUT_PULLUP);
  //configure 3 as digital output [used in this case to drive a seperate CW decoder module]
  pinMode(3, OUTPUT);
  digitalWrite(3, HIGH);
  //Note switches I used are normally closed, so input goes HIGH when the user operates the switch
  pinMode(4, INPUT_PULLUP);
  //digitalWrite(A0, HIGH);
  pinMode(5, INPUT_PULLUP);
  //digitalWrite(A1, HIGH);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  DitPeriod = 1200/wpm;
  TonePeriod= (int)( 1000000.0/((float)toneFreq*2));
 
}

void loop() {
  //read input pins (Low means contact closure)  
  int StraightKeyVal = digitalRead(0);  //Straight Key input
  int PaddleDitVal = digitalRead(1);    //Paddle "Dit" input
  int PaddleDahVal = digitalRead(2);    //Paddle "Dah" input
  int SpeedIncVal = digitalRead(4);     //Speed Increment input
  int SpeedDecVal = digitalRead(5);     //Speed Decrement input
  int ToneIncVal = digitalRead(6);      //Tone Increment input
  int ToneDecVal = digitalRead(7);      //Tone Decrement input
  if(spdSwNO){
    SpeedIncVal = !SpeedIncVal; 
    SpeedDecVal = !SpeedDecVal;
    ToneIncVal = !ToneIncVal; 
    ToneDecVal = !ToneDecVal;
  }
  if (!StraightKeyVal){//if TRUE, Straight Key input is closed [to Ground])
   MakeTone();
   Running = false;
   DitActive = false;
   DahActive = false;
   SpaceActive = false;
  }
  if((SpaceActive & (millis()>ok2Chk))|| !Running ){ // 
    if(!PaddleDitVal) padDit = true;
    if(!PaddleDahVal) padDah = true;
  }
  if (!Running){// Check "paddle" status
    if (padDit & padDah){// if "TRUE" both input pins are closed, and we are in the "Iambic" mode
      if(DitLast) padDit = false;
      else padDah = false;
    }
    if (padDit & !DitActive ){
      DitActive = true;
      Running = true;
      padDit = false;
      End = (DitPeriod) + millis();
      //Serial.println(End);// testing only
    }
    else if (padDah & !DahActive ){
      DahActive = true;
      padDah = false;
      Running = true;
      End = (3*DitPeriod) +  millis();
    }
    //now check for speed change, But only we are actively sending stuff
    if (!PaddleDitVal || !PaddleDahVal){
      if(SpeedIncVal){ //user signaling to decrease speed
        spdChngDelay++;
        if(spdChngDelay==3){
          spdChngDelay=0;
          wpm--;
          if(wpm <8) wpm = 8;
          DitPeriod = 1200/wpm;
        }
      }
      if(SpeedDecVal){ //user signaling to increase speed
        spdChngDelay++;
        if(spdChngDelay==3){
          spdChngDelay=0;
          wpm++;
          if(wpm >30) wpm = 30;
          DitPeriod = 1200/wpm;
        }
      }
      if(ToneIncVal){ //user signaling to increase tone freq
        toneFreq += 50;
        TonePeriod = (int)( 1000000.0/((float)toneFreq*2));
      }
      if(ToneDecVal){ //user signaling to decrease tone freq
        toneFreq += 30;
        TonePeriod = (int)( 1000000.0/((float)toneFreq*2));
      }
    }
  }
  else{// We are "running" (electronic keyer / Paddle mode is active)
    if (!SpaceActive){
      if (End > millis()){
          MakeTone();
        }
      else{
        End = (DitPeriod) +  millis();
        ok2Chk = millis()+ (2*DitPeriod/3);//start looking ahead to see which symbol (dit/dah) the user wants to send next; in this case, the micro will check paddle status 2/3 the way thru the space interval 
        SpaceActive = true;
        if (DitActive){
          DitActive = false;
          DitLast = true;
        }
        else{
          DahActive = false;
          DitLast = false;
        }
      }
    }
    else{ // SpaceActive is true;  So we want to remain silent for the lenght of a "dit"
       if (End > millis()){
        digitalWrite(3, HIGH); //output to remote decoder
        digitalWrite(15, LOW);
        digitalWrite(16, LOW);
      }
      else{ //"dit" silence period met, so clear spaceActive flag 
        SpaceActive = false;
        Running = false;
      }
    }
    
 }// end of "Running" flag set 'true' (electronic keyer is active) code
 
  if(StraightKeyVal & !Running ){ //if the straight key input is "Open", and the Paddle mode isn't active, turn the speaker off
    digitalWrite(3, HIGH); //output to remote decoder 
    digitalWrite(15, LOW);
    digitalWrite(16, LOW);
  }
 
  delayMicroseconds(TonePeriod);// pause for 1/2 a tone cycle before repeating the Loop.
}//End Main Loop

void MakeTone(){//Cycle the speaker pins
  digitalWrite(3, LOW); //output to remote decoder
  digitalWrite(15, LOW);
  digitalWrite(16, HIGH);
  delayMicroseconds(TonePeriod);
  digitalWrite(15, HIGH);
  digitalWrite(16, LOW);
}
