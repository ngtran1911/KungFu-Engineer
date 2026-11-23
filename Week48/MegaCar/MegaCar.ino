#include <Wire.h>
#include <LiquidCrystal.h>
#include <util/atomic.h>
#include <stdlib.h>

// Existing parallel LCD and motor driver wiring. Positive PWM means forward.
LiquidCrystal lcd(37,36,35,34,33,32);
const byte DIR_L=7, DIR_R=8, PWM_L=9, PWM_R=10;
const byte ENC_L=2, ENC_R=3, SWITCH_PIN=19;
const float LEFT_PPCM=9.5, RIGHT_PPCM=9.5;
const int X_SIGN=1, Y_SIGN=1; // Reverse either sign for a differently mounted stick.
const char* names[]={"North","East","South","West"};
const char* sectors[]={"N","NE","E","SE","S","SW","W","NW"};
volatile unsigned long leftTicks=0,rightTicks=0;
unsigned long leftNow,rightNow,leftStart,rightStart;
enum Motion { IDLE, DRIVE, TURN };
Motion motion=IDLE;
bool joystick=false, neutralRequired=true;
int x,y,centerX,centerY, speedPWM=128, distanceCm;
int zeroBearing=-1, bearing=-1, previous, remaining;
int queue[8], head=0, count=0;
bool seenPing=false; // Require a live browser before accepting movement.
unsigned long lastPing, started, lastSample, lastDisplay;
char latest[33]="Waiting", line[33];
byte used=0;
bool discard=false;
const char* fault="";

// Interrupt handlers only count rising edges. The main loop takes an atomic snapshot.
void leftEdge(){ ++leftTicks; }
void rightEdge(){ ++rightTicks; }

// Drive one motor from a signed PWM command and cap the magnitude at 255.
void wheel(byte dir,byte pwm,int command){
  digitalWrite(dir,command<0);
  analogWrite(pwm,min(abs(command),255));
}
void motors(int left,int right){
  wheel(DIR_L,PWM_L,left);
  wheel(DIR_R,PWM_R,right);
}

// Clear all pending movement whenever control changes or a command fails.
void stopAll(){
  motors(0,0);
  motion=IDLE;
  count=0;
  head=0;
}
void fail(const char* message){ // ERROR handling
  stopAll(); // ERROR handling
  fault=message; // ERROR handling: displayed on the second LCD row
}

// The compass has 256 steps per revolution. Return -1 on a bus failure.
int readCompass(){
  Wire.beginTransmission(0x60);
  Wire.write(1);
  if(Wire.endTransmission()!=0 || Wire.requestFrom(0x60,1)!=1) return -1; // ERROR handling
  return Wire.read();
}

// Convert a difference across north into a signed change within half a revolution.
int shortest(int counts){ return (counts+384)%256-128; }

// Reject partial numbers, out-of-range values and empty arguments.
bool number(const char* text,int limit,int &value){
  char* end;
  long parsed=strtol(text,&end,10);
  if(end==text || *end || parsed < -limit || parsed > limit) return false; // ERROR handling
  value=parsed;
  return true;
}

// Route complete UART lines. Heading commands wait their turn in an eight-slot queue.
void command(char* msg){
  if(!strcmp(msg,"ping")){seenPing=true;lastPing=millis();return;}
  if(joystick) return;
  if(!strncmp(msg,"IP ",3)) return; // Ignore the ESP startup address on the shared UART.
  strcpy(latest,msg);
  fault="";
  int value;
  if(!strncmp(msg,"lcd:",4)) return; // The LCD command page shows the supplied text.
  if(!strncmp(msg,"speed:",6) && number(msg+6,255,value) && value>=0){
    speedPWM=value;
    if(!value) stopAll();
    return;
  }
  if(!seenPing || millis()-lastPing>2500){fail("Heartbeat lost");return;} // ERROR handling
  if(!strncmp(msg,"drive:",6) && number(msg+6,50,value)){
    stopAll();
    if(value && speedPWM){
      distanceCm=value; leftStart=leftNow; rightStart=rightNow;
      started=millis(); motion=DRIVE;
    }
    return;
  }
  // Values 400..403 represent absolute headings and cannot collide with slider degrees.
  if(!strncmp(msg,"find:",5)){
    for(value=0;value<4;++value) if(!strcmp(msg+5,names[value])) break;
    if(value==4){fail("Bad direction");return;} // ERROR handling
    value+=400;
  }else if(strncmp(msg,"degree:",7) || !number(msg+7,360,value)){
    fail("Bad command");return; // ERROR handling
  }
  if(count==8){fail("Turn queue full");return;} // ERROR handling
  queue[(head+count)%8]=value; ++count;
}

// Read without waiting for a newline. Discard an entire oversized line.
void receive(){
  while(Serial.available()){
    char c=Serial.read();
    if(c=='\r') continue;
    if(c=='\n'){
      line[used]=0;
      if(!discard && used) command(line);
      used=0; discard=false;
    }else if(used<sizeof(line)-1) line[used++]=c;
    else if(!discard){discard=true;if(!joystick) fail("UART line long");} // ERROR handling
  }
}

// Debounce the active-low push switch. Require a centered stick after switching.
void modeButton(){
  static bool raw=HIGH, stable=HIGH;
  static unsigned long changed;
  bool sample=digitalRead(SWITCH_PIN);
  if(sample!=raw){raw=sample;changed=millis();}
  if(raw!=stable && millis()-changed>=35){
    stable=raw;
    if(!stable){stopAll(); joystick=!joystick; neutralRequired=true; fault="";}
  }
}

// Map each side of center separately to account for an off-center resting value.
int axis(int raw,int center){
  int delta=raw-center;
  if(abs(delta)<=50) return 0;
  int span=(delta>0 ? 1023-center : center)-50;
  return (delta>0 ? 1 : -1)*(long)(abs(delta)-50)*255/span;
}
void manualDrive(){
  int steer=X_SIGN*axis(x,centerX), throttle=Y_SIGN*axis(y,centerY);
  if(neutralRequired){
    neutralRequired=steer || throttle;
    return;
  }
  // Differential mixing preserves the wheel-speed ratio even at a diagonal.
  int l=throttle+steer,r=throttle-steer;
  int scale=max(255,max(abs(l),abs(r)));
  motors((long)l*255/scale,(long)r*255/scale);
}

// Compare the two measured distances and slow the leading wheel. Stop each at its goal.
void driveStep(){
  float l=(leftNow-leftStart)/LEFT_PPCM,r=(rightNow-rightStart)/RIGHT_PPCM;
  int goal=abs(distanceCm), sign=distanceCm>0 ? 1 : -1;
  if(l>=goal && r>=goal){motors(0,0); motion=IDLE;return;}
  int trim=constrain((int)((l-r)*14),-speedPWM/2,speedPWM/2);
  motors(l>=goal ? 0 : sign*(speedPWM-max(trim,0)),
         r>=goal ? 0 : sign*(speedPWM+min(trim,0)));
}

// Start a queued turn using the calibrated bearing measured at execution time.
void beginTurn(){
  int request=queue[head];
  head=(head+1)%8; --count;
  remaining=request>=400 ? shortest((request-400)*64-bearing) : round(request*256.0/360);
  previous=bearing; started=millis(); motion=TURN;
}

// Integrate signed movement across north. Slow down near target and correct overshoot.
void turnStep(){
  static byte settled;
  remaining-=shortest(bearing-previous); previous=bearing;
  if(abs(remaining)<=2){
    motors(0,0);
    if(++settled>=3){motion=IDLE;settled=0;}
  }else{
    settled=0;
    int power=abs(remaining)>12 ? 100 : 75;
    int rightward=remaining>0 ? power : -power;
    motors(rightward,-rightward);
  }
}

// Write a padded 16-character row to clear stale characters without LCD flicker.
void row(byte n,const char* s){
  lcd.setCursor(0,n);
  for(byte i=0;i<16;++i) lcd.write(*s ? *s++ : ' ');
}
void display(){
  char buf[40];
  if(bearing<0) snprintf(buf,sizeof(buf),"%s compass --",joystick?"Joystick":"ESP");
  else snprintf(buf,sizeof(buf),"%s %d %s",joystick?"Joystick":"ESP",(bearing*360L+128)/256 > 359 ? 0 : (int)((bearing*360L+128)/256),sectors[((bearing+16)/32)%8]);
  row(0,buf);
  // Cycle the lower row through both odometers and the active input source.
  byte page=(millis()/1700)%5;
  if(*fault) row(1,fault);
  else{
    if(page==0 || page==2) snprintf(buf,sizeof(buf),"%c %lu pulses",page==0?'L':'R',page==0?leftNow:rightNow);
    else if(page==1 || page==3) snprintf(buf,sizeof(buf),"%c %lu cm",page==1?'L':'R',(unsigned long)(page==1?leftNow/LEFT_PPCM:rightNow/RIGHT_PPCM));
    else if(joystick) snprintf(buf,sizeof(buf),"X:%d Y:%d",x,y);
    else snprintf(buf,sizeof(buf),"%.16s",!strncmp(latest,"lcd:",4)?latest+4:latest);
    row(1,buf);
  }
}

void setup(){
  // Keep UART0 for the ESP. D19/RX1 is exclusively the joystick button.
  Serial.begin(9600); pinMode(SWITCH_PIN,INPUT_PULLUP);
  pinMode(DIR_L,OUTPUT); pinMode(DIR_R,OUTPUT);
  pinMode(PWM_L,OUTPUT); pinMode(PWM_R,OUTPUT); stopAll();
  pinMode(ENC_L,INPUT); pinMode(ENC_R,INPUT);
  attachInterrupt(digitalPinToInterrupt(ENC_L),leftEdge,RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_R),rightEdge,RISING);
  // At reset, leave joystick centered and point chassis at true north.
  centerX=analogRead(A5); centerY=analogRead(A6);
  lcd.begin(16,2); Wire.begin(); Wire.setWireTimeout(25000,true); // ERROR handling: bounded I2C wait
  for(byte attempt=0;attempt<10 && zeroBearing<0;++attempt){delay(30);zeroBearing=readCompass();}
  if(zeroBearing<0){joystick=true;fail("Compass: reset");} // ERROR handling
}

void loop(){
  // Every pass services input and the encoders even while the car is moving.
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){leftNow=leftTicks;rightNow=rightTicks;}
  x=analogRead(A5); y=analogRead(A6);
  modeButton(); receive();
  bool fresh=millis()-lastSample>=30;
  if(fresh){
    lastSample=millis();
    int raw=readCompass();
    bearing=raw<0 || zeroBearing<0 ? -1 : (raw-zeroBearing+256)%256;
  }
  // ERROR handling: a missing browser heartbeat cancels web motion and every pending turn.
  if(!joystick && (motion!=IDLE || count) && millis()-lastPing>2500) fail("Heartbeat lost"); // ERROR handling
  if(motion!=IDLE && millis()-started>15000) fail("Motion timeout"); // ERROR handling
  if(joystick) manualDrive();
  else if(motion==DRIVE) driveStep();
  else if(fresh && (motion==TURN || count)){
    if(bearing<0){fail("Compass error");} // ERROR handling
    else{
      if(motion==IDLE) beginTurn();
      turnStep();
    }
  }
  if(millis()-lastDisplay>=150){lastDisplay=millis();display();}
}
