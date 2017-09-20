/*
   Cider Drag Races

   A project to let two racers compete in drinking a glass of cider,
   complete with start button, weight activated drink coasters,
   a drag-strip "christmas tree", and buzzer.

   Along with a few easter eggs.

   Pin Assignments per Default Sketch:
     Digital:
       2 - Left Player Display
       3 - Right Player Display
       4 - unused (RX for left display)
       5 - unused (RX for right display)
       13 - Left Ready Button
       7 - Left Pressure Simulator
       8 - Right Ready Button
       9 - Buzzer (+)
       10 - Right Pressure Simularor
       11 - Left Player Light Tree (6 lights) (If WS2812B)
       12 - Right Player Light Tree (6 lights)(If WS2812B)
       30-39 - Colored LEDs

     Analog:
       0 - Right Player Pressure Sensor
       1 - Left Player Pressure Sensor
       7 - Unused; Seeds RNG.


   Fred Damstra
   fred.damstra@gmail.com

   2016-09-29 - Initial Draft

   Licensed under GPLv3: https://www.gnu.org/licenses/gpl.html
*/
#include <SoftwareSerial.h>
#include "Adafruit_NeoPixel.h"
#include "WS2812_Definitions.h"
#include <eRCaGuy_ButtonReader.h>

/**********
   CONFIGURATION SECTION
*/

/* Buzzer Setup:
    Wire ground to common ground, and positive to a PWM
    pin. In this example, pin 9.

    Lots of help for this example at
    https://learn.sparkfun.com/tutorials/sik-experiment-guide-for-arduino---v32/experiment-11-using-a-piezo-buzzer
*/
const int debug = 0;

const int buzzerPin = 9;
const unsigned long songFrequency = 90000; // How frequently the song may play in seconds
const float songProbability = 0.0; // Probability that the song will play during that frequency

/* randomPin should be an unused analog pin for seeding the RNG */
const int randomPin = 5;

/* WS2812 Christmas Tree Setup:
    Two strips of 6 LEDs connected to 5V and Ground, plus the signal pin
*/
const int ledPinRight = 12;
const int ledPinLeft  = 11;
const int brightness  = 255;

/* Digital (old school) Christmas Tree Setup:
    Two strips of 6 LEDs connected to 5V and Ground, plus the signal pin
*/
//const int ledPinsLeft[]  = {30, 32, 34, 36, 38};
const int ledPinsLeft[]  = {38, 36, 34, 32, 30};
//const int ledPinsRight[] = {31, 33, 35, 37, 39};
const int ledPinsRight[] = {39, 37, 35, 33, 31};

const unsigned long prettyLEDFrequency = 30; // How frequently something pretty happens on the LEDs
const float prettyLEDProbability = 0.75; // Probability that the song will play during that frequency

/* Christmas Tree */
const unsigned long startDelay = 1000; /* How long to wait between hitting the start ubtton and starting */
const unsigned long maxDelay = 5000;   /* In non dragrace mode, how long might we wait *after* a guaranteed startDelay? */
const unsigned long lightDelay = 500;  /* How long to wait between lights during the countdown */
const unsigned long topColor   = PURPLE; /* Top LED is constant to show power */

/* 7-Segment Display Setup
    Two 7 OpenSegment displays from SparkFun in serial mode.
*/
const int leftDisplayTX  = 2;
const int rightDisplayTX = 3;
const int leftDisplayRX  = 4; /* Unused */
const int rightDisplayRX = 5; /* Unused */

/* Force Sensitive Resistor Switches (Analog inputs) */
const int leftFSR  = A7;
const int rightFSR = A0;
const int leftFSRThreshold  = 10; /* > this number is closed */
const int rightFSRThreshold = 10;
const int leftFSRSimPin = 7; /* For when the FSRs aren't there */
const int rightFSRSimPin = 10; /* For when the FSRs aren't there */

/* Start Button */
const int leftReadyButtonPin = 13;
const int rightReadyButtonPin = 8;

/* Difficulty */
const int difficultyLeftPin = 46;
const int difficultyRightPin = 47;

/* Loop delays
    n.b. It occurs to me that unusual values for these variables /may/ upset
    the math on looping. Probably not, but if you run into problems, return
    these to original values of 100 and 25 respectively
*/
const int mainDelay = 100; /* How long to sleep during the main loop */
const int countdownDelay = 25; /* How long to sleep during the countdown loop */

/* Special Functions */
const int modePin = 51; /* Red */
const int buzzPin = 52; /* Green */
const int clearPin = 53; /* White */


/***********
   Application Constants
*/
const int LED_COUNT = 6;

const unsigned int buzzerFrequency = 70;
const int buzzerDuration = 1800;
const unsigned int beepFrequency = 3700;
const int beepDuration = 100;
const int beepMultiplier = 3; /* long beep is this x as long a short beep */

const unsigned long renegDelay = 500;
const unsigned long debounceDelay = 250;

/***********
   Application Variables
*/
unsigned long lastSongCheck = 0;
Adafruit_NeoPixel rightTree = Adafruit_NeoPixel(LED_COUNT, ledPinRight, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel leftTree  = Adafruit_NeoPixel(LED_COUNT, ledPinLeft,  NEO_GRB + NEO_KHZ800);

SoftwareSerial leftDisplay(leftDisplayRX, leftDisplayTX);
SoftwareSerial rightDisplay(rightDisplayRX, rightDisplayTX);

eRCaGuy_ButtonReader leftReadyButton = eRCaGuy_ButtonReader(leftReadyButtonPin);
eRCaGuy_ButtonReader rightReadyButton = eRCaGuy_ButtonReader(rightReadyButtonPin);
eRCaGuy_ButtonReader leftFSRButton = eRCaGuy_ButtonReader(leftFSRSimPin);
eRCaGuy_ButtonReader rightFSRButton = eRCaGuy_ButtonReader(rightFSRSimPin);
eRCaGuy_ButtonReader clearToggle = eRCaGuy_ButtonReader(clearPin);


/* Buzzer: Some constants for playing a song: */
const int songLength = 18;
char notes[] = "cdfda ag cdfdg gf ";
int beats[] = {1, 1, 1, 1, 1, 1, 4, 4, 2, 1, 1, 1, 1, 1, 1, 4, 4, 2};
const int tempo = 118;

int songLength2 = 26;
char notes2[] = "eeeeeeegcde fffffeeeeddedg";
int beats2[] = { 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2};
const int tempo2 = 130;

/* LEDs */
unsigned long lastPrettyLEDCheck = 0;

/* 7 Segment */
long rightClock = 0;
long leftClock = 0;

#define APOSTROPHE  5
#define COLON       4
#define DECIMAL4    3
#define DECIMAL3    2
#define DECIMAL2    1
#define DECIMAL1    0



/***********
   Code
*/
void setup() {
  Serial.begin(9600); // Serial used for logging

  /* Buzzer Setup */
  pinMode(buzzerPin, OUTPUT);
  randomSeed(analogRead(randomPin));


  /* WS2812 LED Setup */
  rightTree.begin();
  leftTree.begin();
  /* Digital LED Setup */
  for(int i = 0; i < 5; i++) {
    pinMode(ledPinsLeft[i], OUTPUT);
    digitalWrite(ledPinsLeft[i], LOW);
    pinMode(ledPinsRight[i], OUTPUT);
    digitalWrite(ledPinsRight[i], LOW);
  }
  /* clearLEDs(); */
  clearLEDs();

  /* 7 Segment Displays */
  leftDisplay.listen();
  leftDisplay.begin(9600);
  leftDisplay.write('v');
  leftDisplay.write(0x7A);
  leftDisplay.write((byte) brightness);
  rightDisplay.listen();
  rightDisplay.begin(9600);
  rightDisplay.write('v');
  rightDisplay.write(0x7A);
  rightDisplay.write((byte) brightness);
  longbeep();
  printClocks(1223,1996);

  /* Initialize our buttons */
  pinMode(leftReadyButtonPin, INPUT_PULLUP);
  pinMode(rightReadyButtonPin, INPUT_PULLUP);
  pinMode(leftFSRSimPin, INPUT_PULLUP);
  pinMode(rightFSRSimPin, INPUT_PULLUP);
  pinMode(leftFSR, INPUT);
  pinMode(rightFSR, INPUT);

  /* And our switches */
  pinMode(modePin, INPUT_PULLUP); /* Red */
  pinMode(clearPin, INPUT_PULLUP); /* Green */
  pinMode(buzzPin, INPUT_PULLUP); /* White */

  /* And our sliders */
  pinMode(difficultyLeftPin, INPUT_PULLUP); /* HIGH is normal difficulty */
  pinMode(difficultyRightPin, INPUT_PULLUP);  

  /* Demo the Christmas Tree */
  setLEDs(BLACK, BLACK, BLACK, BLACK, YELLOW);
  longbeep();  delay(400);
  setLEDs(BLACK, BLACK, BLACK, YELLOW, BLACK);
  longbeep();  delay(400);
  setLEDs(BLACK, BLACK, YELLOW, BLACK, BLACK);
  longbeep();  delay(400);
  setLEDs(BLACK, GREEN, BLACK, BLACK, BLACK);
  longbeep(); delay(400);
  setLEDs(RED, BLACK, BLACK, BLACK, BLACK);
  longbeep();
  
  /* Arbitrary delay before updating the 7-segments again */
  delay(1000);
  
  /* 7 Segment Display Continued */
  if(random(0, 1000) < 100) {
    leftClock=531;
    rightClock=8008;
    printClocks(leftClock, rightClock);
  } else {
    leftClock=1223;
    rightClock=2016;
    printClocks(leftClock,rightClock);
  }
  longbeep();

  /* Switches */
  pinMode(leftFSR, INPUT);
  pinMode(rightFSR, INPUT);

  /* start button */
  //pinMode(startButtonPin, INPUT_PULLUP);
  Serial.println("Setup complete.");
}

/* MAIN LOOP */
void loop() {
  int8_t clearButtonAction;
  boolean clearButtonState;
  // put your main code here, to run repeatedly:
  easterEggSongCheck(); // Sometimes we play a song
  prettyLEDCheck();

  clearToggle.readButton(&clearButtonAction, &clearButtonState);

  if(clearButtonAction == 1) {
    Serial.println("clear action called");
    if(random(0, 1000) < 100) {
      leftClock=531;
      rightClock=8008;
      printClocks(leftClock, rightClock);
    } else {
      leftClock=0;
      rightClock=0;
      printClocks(leftClock,rightClock);
    }
  }
  
  printClocks(leftClock, rightClock);

  shouldwebuzz();
  
  /* If either coaster is open (no glass), continue on */
  if (leftSwitchOpen() || rightSwitchOpen()) {
    clearLEDs();
    if(!leftSwitchOpen()) {
      digitalWrite(ledPinsLeft[0], HIGH);
    } else if(!rightSwitchOpen()) {
      digitalWrite(ledPinsRight[0], HIGH);
    }
    delay(mainDelay);
    return; /* Can't continue in main loop */
  } else {
    /* Both are closed, race can begin */
    ledsToRed();
    if(millis() % 5000 == 0) {
      Serial.println("Waiting for start button to be pressed.");
    }
    if (startButtonPressed()) {
      countdown();
      /* We're back from a race */
      /* Reset easter eggs */
      lastSongCheck = lastPrettyLEDCheck = millis();
      startButtonPressed(); /* Clear out "stuck" state input */
      //return; /* no continue in main loop */
    }
  }

  /* Rest before looping again */
  delay(mainDelay);
}

void countdown(void) {
  unsigned long randomAdditionalDelay;
  int mode; /* 1 is drag race, 0 is randomness */
  int cstep = -1; /* Which step of the countdown are we on? */
  rightClock = leftClock = 0;

  if(digitalRead(modePin) == HIGH) {
    /* Drag race mode */
    mode = 1;
  } else {
    /* Random delay mode */
    setLEDs(RED, BLACK, BLACK, BLACK, YELLOW);
    randomAdditionalDelay = random(maxDelay);
    mode = 0;
  }

  if(mode) {
    Serial.print("Start button pressed. Beginning xmas tree in ");
    Serial.print(startDelay);
    Serial.println("ms");
  } else {
    Serial.print("Start button pressed. Skipping xmas tree. Beginning in ");
    Serial.print(startDelay + randomAdditionalDelay);
    Serial.println("ms.");    
  }
  printClocks(leftClock, rightClock);
  startsong();
  delay(startDelay);

  unsigned long start = millis();
  while (1) {

    shouldwebuzz();
    
    if (leftSwitchOpen_debounce(1) || rightSwitchOpen_debounce(1)) {
      /* FALSE START! */
      if (leftSwitchOpen_debounce(1)) {
        Serial.println("FALSE START: Player Left");
        buzz_nonblock();
        leftClock = 9999;
        rightClock = 0;
        printClocks(leftClock, rightClock);
        for(int i = 0; i < 3; i++) {
          leftDisplay.listen();
          leftDisplay.write(0x7A);
          leftDisplay.write((byte) 0);
          delay(150);
          leftDisplay.listen();
          leftDisplay.write(0x7A);
          leftDisplay.write((byte) brightness);
          delay(150); 
        }
        delay(buzzerDuration - 900); /* Rest for the remaining buzz */
        leftSwitchOpen_debounce(1);
        rightSwitchOpen_debounce(1); /* Get another reading just to clear things out */
        return;
      } else if (rightSwitchOpen_debounce(1)) {
        Serial.println("FALSE START: Player Right");
        buzz_nonblock();
        rightClock = 9999;
        leftClock = 0;
        printClocks(leftClock, rightClock);
        for(int i = 0; i < 3; i++) {
          rightDisplay.listen();
          rightDisplay.write(0x7A);
          rightDisplay.write((byte) 0);
          delay(150);
          rightDisplay.listen();
          rightDisplay.write(0x7A);
          rightDisplay.write((byte) 255);
          delay(150);
        }
        delay(buzzerDuration - 900); /* Rest for the remaining buzz */
        leftSwitchOpen_debounce(1);
        rightSwitchOpen_debounce(1); /* Get another reading just to clear things out */
        return;
      } else {
        /* WTF? Timing issue */
        Serial.println("WARNING: False start detected, but corrected?!");
      }
    }
    /* No false start */

    if(mode) {
      /* drag race! */
      switch ( (millis() - start) / lightDelay ) {
        case 0:
          if (cstep != 0) {
            Serial.println("Countdown ... 3");
            setLEDs(RED, BLACK, BLACK, BLACK, YELLOW);
            beep();
            cstep = 0;
          }
          continue;
        case 1:
          if (cstep != 1) {
            Serial.println("Countdown ... 2");
            setLEDs(RED, BLACK, BLACK, YELLOW, BLACK);
            beep();
            cstep = 1;
          }
          continue;
        case 2:
          if (cstep != 2) {
            Serial.println("Countdown ... 1");
            setLEDs(RED, BLACK, YELLOW, BLACK, BLACK);
            beep();
            cstep = 2;
          }
          continue;
        case 3:
          Serial.println("Countdown ... GO!!!!!!!!");
          setLEDs(BLACK, GREEN, BLACK, BLACK, BLACK);
          longbeep();
          race_loop();
          return;
      } 
    } else {
      /* Silent start */
      if(millis() > (start + startDelay + randomAdditionalDelay)) {
          Serial.println("Random delay met... GO!!!!!!!!");
          setLEDs(BLACK, GREEN, BLACK, BLACK, BLACK);
          longbeep();
          race_loop();
          return;
      }
    }
    delay(countdownDelay); /* Make the loop less aggressive */
  }
  Serial.println("Should never get here...");
}

void race_loop(void) {
  /* Race is on! */
  unsigned long start = millis();
  Serial.print("Race is ON at ");
  Serial.println(start);

  boolean handicap_left = false;
  boolean handicap_right = false;

  unsigned long timerLeft, timerRight;
  timerLeft = timerRight = 0;

  /* determine handicap settings */
  if(digitalRead(difficultyLeftPin) == 0) {
    handicap_left = true;
  }
  if(digitalRead(difficultyRightPin) == 0) {
    handicap_right = true;
  }

  /* Record whether the racer picked up his/her cup */
  int leftStarted  = 0;
  int rightStarted = 0;
  unsigned long loopstart = millis();
  unsigned long lastloop = millis();
  unsigned long leftRenegBounce = 0;
  unsigned long rightRenegBounce = 0;

  while (1) {
    loopstart = millis();
    //Serial.print("Loop time with serial: ");
    //Serial.println(loopstart - lastloop);
    /* Check for winners */
    boolean leftOpen, rightOpen;
    leftOpen = rightOpen = false;

    /* Choose which one to read first randomly to offset any potential bias. I
     * find that the loop is generally less than 1ms, so this is probably unnecessary,
     * but it doesn't hurt.
     */
    if(random(0, 2)) {
//      Serial.print("Left first. Time = ");
//      Serial.println(millis());
      leftOpen = leftSwitchOpen();
      rightOpen = rightSwitchOpen();
    } else {
//      Serial.print("Right first. Time = ");
//      Serial.println(millis());
      rightOpen = rightSwitchOpen();
      leftOpen = leftSwitchOpen();
    }

    /* Check for finishers */
    if (!leftOpen && leftStarted && !timerLeft) {
      /* Finished */
      Serial.print("Left player finished at ");
      Serial.println(millis() - start);
      timerLeft = (millis() - start) / 10;
      if(handicap_left) {
        /* Handicapped player */
        timerLeft = timerLeft - (timerLeft >> 2);      
      }
      beep_nonblock();
    } else if (leftOpen && !leftStarted) {
      Serial.print("Left cup lifted at ");
      Serial.println(millis() - start);
      leftStarted = 1;
    }
    if (!rightOpen && rightStarted && !timerRight) {
      /* Finished */
      Serial.print("Right player finished at ");
      Serial.println(millis() - start);
      timerRight = (millis() - start) / 10;
      if(handicap_right) {
        /* Handicapped player */
        timerRight = timerRight - (timerRight >> 2);
      }
      beep_nonblock();
    } else if (rightOpen && !rightStarted) {
      Serial.print("Right cup lifted at ");
      Serial.println(millis() - start);
      rightStarted = 1;
    }

    /* Check for renegers 
     *  Note to cheaters: too hard a hit can make the opponent's cup bounce.
     *  However, we can't turn this off, because things bounce on removal.
     */
    if (leftSwitchOpen() && timerLeft) {
      /* Bullshit! */
      if(leftRenegBounce == 0) {
        leftRenegBounce = millis();
      } else if((millis() - leftRenegBounce) > renegDelay) {
        Serial.println("Left player RENEGS! This is bullshit!");
        timerLeft = 0;
        leftRenegBounce = 0;
      } else {
        Serial.print("Left reneg on delay: ");
        Serial.println(millis() - leftRenegBounce);
      }
    }
    if (rightSwitchOpen() && timerRight) {
      /* Bullshit! */
      if(rightRenegBounce == 0) {
        rightRenegBounce = millis();
      } else if((millis() - rightRenegBounce) > renegDelay) {
        Serial.println("Right player RENEGS! This is bullshit!");
        timerRight = 0;
        rightRenegBounce = 0;
      } else {
        Serial.print("Right reneg on delay: ");
        Serial.println(millis() - rightRenegBounce);
      }
    }

    /* If still a winner and not a reneger, I think we have a winner */
    if (timerLeft) {
      leftClock = timerLeft;
    } else {
      leftClock = (millis() - start) / 10;
      if(handicap_left) {
        /* Handicapped player */
        leftClock = leftClock - (leftClock >> 2);
      }
    }
    if (timerRight) {
      rightClock = timerRight;
    } else {
      rightClock = (millis() - start) / 10;
      if(handicap_right) {
        /* Handicapped player */
        rightClock = rightClock - (rightClock >> 2);
      }
    }
    /* Print the clocks */
    printClocks(leftClock, rightClock);
    if (timerRight && timerLeft) {
      /* Race is finished */
      ledsToRed();
      if(timerRight < timerLeft) {
        Serial.println("Right player WINS!");
        for(int i = 0; i < 3; i++) {
          rightDisplay.listen();
          rightDisplay.write(0x7A);
          rightDisplay.write((byte) 0);
          delay(150);
          beep();
          rightDisplay.listen();
          rightDisplay.write(0x7A);
          rightDisplay.write((byte) 255);
          delay(150);
        }
      } else if(timerLeft < timerRight) {
        for(int i = 0; i < 3; i++) {
          leftDisplay.listen();
          leftDisplay.write(0x7A);
          leftDisplay.write((byte) 0);
          delay(150);
          beep();
          leftDisplay.listen();
          leftDisplay.write(0x7A);
          leftDisplay.write((byte) 255);
          delay(150);
        }
        Serial.println("Left player WINS!");        
      } else {
        Serial.println("TIE GAME!!!");
      }
      playWinner();
      /* Wait until start buttons are no longer pressed */
      while(startButtonPressed()) {
        delay(100);
      }
      return;
    }
    lastloop = loopstart;
  }
  Serial.println("ERROR: Woah. Just Woah. How the fuck? 0x73184fcgukl");
}

/* Beep and buzz if those toggles are pressed */
void shouldwebuzz() {
  if(digitalRead(buzzPin) == LOW) {
    buzz_nonblock();
  }
}

/* Start button switch */
int startButtonPressed() {
  int8_t leftButtonAction, rightButtonAction;
  boolean leftButtonState, rightButtonState;

  leftReadyButton.readButton(&leftButtonAction, &leftButtonState);
  rightReadyButton.readButton(&rightButtonAction, &rightButtonState);

  if(leftButtonState == false && rightButtonState == false) {
    Serial.println("Both ready buttons pressed.");
    return 1; 
  } else {
/*    Serial.print("Left state: ");
    Serial.println(leftButtonState);
    Serial.print("Right state: ");
    Serial.println(rightButtonState);
*/
    return 0;
  }
}

/* FSR Functions */
int leftSwitchOpen() {
  return leftSwitchOpen_debounce(0);
}

int leftSwitchOpen_debounce(int silent) {
  /* Not a standard debounce method. We're going to keep the last 16 values
   * and majority rules.
   */
  static int results[16] = {0,0,0,0,0,
                            0,0,0,0,0,
                            0,0,0,0,0,0};
  static int pointer = 0;
  results[pointer] = leftSwitchOpen(silent);
  pointer = (pointer + 1) % 16;

  int sum = 0;
//  Serial.print("Left array: [");
  for(int i = 0; i < 16; i++) {
    sum+=results[i];
//    Serial.print(results[i]);
//    Serial.print(",");
  }
  if(sum > 8) {
//    Serial.println("]; Returning 1");
    return 1;
  } else {
//    Serial.println("]; Returning 0");
    return 0;
  }
}

int leftSwitchOpen(int silent) {
  int8_t leftButtonAction;
  boolean leftButtonState;
  static int lastvalue = 0;

/* Sim Mode: 
  leftFSRButton.readButton(&leftButtonAction, &leftButtonState);
  if(leftButtonState == false) { */
    /* Button is low, meaning it's pressed */
/*    return 0;
  }
  return 1;
*/
  int value = analogRead(leftFSR);
  if( value > leftFSRThreshold) {
    /* Pressed! */
    if(lastvalue < leftFSRThreshold) {
      Serial.print("Left pressed, pressure: ");
      Serial.println(value);
    }
    lastvalue = value;
    return 0;
  } 
  if(lastvalue > leftFSRThreshold) {
    Serial.print("Left released, pressure: ");
    Serial.println(value);
    lastvalue = value;
  }
  return 1;
}

int rightSwitchOpen() {
  return rightSwitchOpen_debounce(0);
}

int rightSwitchOpen_debounce(int silent) {
  /* Not a standard debounce method. We're going to keep the last 16 values
   * and majority rules.
   */
  static int results[16] = {0,0,0,0,0,
                            0,0,0,0,0,
                            0,0,0,0,0,0};
  static int pointer = 0;
  results[pointer] = rightSwitchOpen(silent);
  pointer = (pointer + 1) % 16;

  int sum = 0;
//  Serial.print("Right array: [");
  for(int i = 0; i < 16; i++) {
    sum+=results[i];
//    Serial.print(results[i]);
//    Serial.print(",");
  }
  if(sum > 8) {
//    Serial.println("]; Returning 1");
    return 1;
  } else {
//    Serial.println("]; Returning 0");
    return 0;
  }
}

int rightSwitchOpen(int silent) {
  int8_t rightButtonAction;
  boolean rightButtonState;
  static int lastvalue = 0;

/*
  rightFSRButton.readButton(&rightButtonAction, &rightButtonState);
  if(rightButtonState == false) { */
    /*Button is low, meaning it's pressed */
/*    return 0;
  }
  return 1;
*/
  int value = analogRead(rightFSR);
  if( value > rightFSRThreshold) {
    /* Pressed! */
    if(lastvalue < rightFSRThreshold) {
      Serial.print("Right pressed, pressure: ");
      Serial.println(value);
    }
    lastvalue = value;
    return 0;
  } 
  if(lastvalue > rightFSRThreshold) {
    Serial.print("Right released, pressure: ");
    Serial.println(value);
    lastvalue = value;
  }
  return 1;
}

int switchOpen(int pin, int threshold, int silent) {
  int fsrADC = analogRead(pin);
#ifdef SIMULATION
  fsrADC = random(0, 1000);
#endif

  if (fsrADC < threshold) {
    if (!silent) {
      Serial.print(fsrADC);
      Serial.println(" -- OPEN");
    }
    return 1;
  }
  if (!silent) {
    Serial.print(fsrADC);
    Serial.println(" -- CLOSED");
  }
  return 0;
}

/* 7-Segment Functions */
void printClocks(unsigned long leftValue, unsigned long rightValue) {
  if (leftValue > 9999) {
    leftValue = 9999;
  }
  if (rightValue > 9999) {
    rightValue = 9999;
  }
//  Serial.print("Clocks: ");
  char tempstring[10];
  snprintf(tempstring, sizeof(tempstring), "%04lu", leftValue);
  leftDisplay.listen();
  leftDisplay.write(tempstring);
//  Serial.print(tempstring);
  snprintf(tempstring, sizeof(tempstring), "%04lu", rightValue);
  rightDisplay.listen();
  rightDisplay.write(tempstring);
//  Serial.print(" ");
//  Serial.println(tempstring);

  /* TUrn on decimal point */
  leftDisplay.listen();
  leftDisplay.write(0x77); /* Decimal, colon, and apostrophe command */
  leftDisplay.write(0b000010); /* Decimal 2 is second bit) */
  rightDisplay.listen();
  rightDisplay.write(0x77); /* Decimal, colon, and apostrophe command */
  rightDisplay.write(0b000010); /* Decimal 2 is second bit) */
}

/* LED Functions */
void ledsToRed() {
  ws2812_ledsToRed();
  digitalWrite(ledPinsLeft[0], HIGH);
  digitalWrite(ledPinsRight[0], HIGH);
  for(int i=1; i<5; i++) {
    digitalWrite(ledPinsLeft[i], LOW);
    digitalWrite(ledPinsRight[i], LOW);
  }
}

// Sets red light on.
void ws2812_ledsToRed() {
  clearLEDs();
  rightTree.setPixelColor(0, RED);
  leftTree.setPixelColor(0, RED);
  rightTree.show();
  leftTree.show();
}

/* Clears all LEDs (for WS2812, except the top one) */
void clearLEDs() {
  ws2812_clearLEDs();
  for(int i = 0; i < 5; i++) {
    digitalWrite(ledPinsLeft[i], LOW);
    digitalWrite(ledPinsRight[i], LOW);
  }
}

void ws2812_clearLEDs()
{
  for (int i = 0; i < (LED_COUNT - 1); i++)
  {
    rightTree.setPixelColor(i, 0);
    leftTree.setPixelColor(i, 0);
  }
  rightTree.setPixelColor(LED_COUNT - 1, topColor);
  leftTree.setPixelColor(LED_COUNT - 1, topColor);
  rightTree.show();
  leftTree.show();
}

void setLEDs(unsigned long redlight,
             unsigned long greenlight,
             unsigned long yellow3,
             unsigned long yellow2,
             unsigned long yellow1) {
  ws2812_setLEDs(redlight, greenlight, yellow3, yellow2, yellow1);

  /* If black, turn it off, otherwise turn it on */
  if(redlight == BLACK) {
    digitalWrite(ledPinsLeft[0], LOW);
    digitalWrite(ledPinsRight[0], LOW);
  } else {
    digitalWrite(ledPinsLeft[0], HIGH);
    digitalWrite(ledPinsRight[0], HIGH);
  }
                
  /* If black, turn it off, otherwise turn it on */
  if(greenlight == BLACK) {
    digitalWrite(ledPinsLeft[1], LOW);
    digitalWrite(ledPinsRight[1], LOW);
  } else {
    digitalWrite(ledPinsLeft[1], HIGH);
    digitalWrite(ledPinsRight[1], HIGH);
  }
                
  /* If black, turn it off, otherwise turn it on */
  if(yellow3 == BLACK) {
    digitalWrite(ledPinsLeft[2], LOW);
    digitalWrite(ledPinsRight[2], LOW);
  } else {
    digitalWrite(ledPinsLeft[2], HIGH);
    digitalWrite(ledPinsRight[2], HIGH);
  }
                
  /* If black, turn it off, otherwise turn it on */
  if(yellow2 == BLACK) {
    digitalWrite(ledPinsLeft[3], LOW);
    digitalWrite(ledPinsRight[3], LOW);
  } else {
    digitalWrite(ledPinsLeft[3], HIGH);
    digitalWrite(ledPinsRight[3], HIGH);
  }
                
  /* If black, turn it off, otherwise turn it on */
  if(yellow1 == BLACK) {
    digitalWrite(ledPinsLeft[4], LOW);
    digitalWrite(ledPinsRight[4], LOW);
  } else {
    digitalWrite(ledPinsLeft[4], HIGH);
    digitalWrite(ledPinsRight[4], HIGH);
  }
}

void ws2812_setLEDs(unsigned long redlight,
             unsigned long greenlight,
             unsigned long yellow3,
             unsigned long yellow2,
             unsigned long yellow1) {
  rightTree.setPixelColor(0, redlight);
  rightTree.setPixelColor(1, greenlight);
  rightTree.setPixelColor(2, yellow3);
  rightTree.setPixelColor(3, yellow2);
  rightTree.setPixelColor(4, yellow1);
  rightTree.setPixelColor(LED_COUNT - 1, topColor);
  leftTree.setPixelColor(0, redlight);
  leftTree.setPixelColor(1, greenlight);
  leftTree.setPixelColor(2, yellow3);
  leftTree.setPixelColor(3, yellow2);
  leftTree.setPixelColor(4, yellow1);
  leftTree.setPixelColor(LED_COUNT - 1, topColor);
  rightTree.show();
  leftTree.show();
}

void prettyLEDCheck() {
  /* Lets see if we should play a song */
  if ( (millis() - lastPrettyLEDCheck) > (prettyLEDFrequency * 1000) ) {
    lastPrettyLEDCheck = millis();
    if ( (random(0, 1000) < long(prettyLEDProbability * 1000.0)) ) {
      Serial.println("Pretty LED Approved");
      prettyLED();
    } else {
      Serial.println("Pretty lED probability check declined.");
    }
  }
}

void prettyLED() {
  /* if using the ws2812's, you may want to turn this back on 
  ws2812_prettyLED();
  */
  if(random(0, 1000) < 500) {
    prettyCircleLED();
    prettyCircleLED();
//    prettyCircleLED();
  } else {
    prettyBounceLED();
    prettyBounceLED();
    prettyBounceLED();
  }
}

void prettyCircleLED() {  
  clearLEDs();
  for(int i = 4; i >= 0; i--) {
    shouldwebuzz();
    Serial.print("Pin: ");
    Serial.println(i);
    digitalWrite(ledPinsLeft[i], HIGH);
    delay(100);
    digitalWrite(ledPinsLeft[i], LOW);
  }
  for(int i = 0; i < 5; i++) {
    shouldwebuzz();
    Serial.print("Pin: ");
    Serial.println(i);
    digitalWrite(ledPinsRight[i], HIGH);
    delay(100);
    digitalWrite(ledPinsRight[i], LOW);
  }
}

void prettyBounceLED() {
  clearLEDs();
  for(int i = 4; i >= 0; i--) {
    shouldwebuzz();
    digitalWrite(ledPinsLeft[i], HIGH);
    digitalWrite(ledPinsRight[i], HIGH);
    delay(100);
    digitalWrite(ledPinsLeft[i], LOW);
    digitalWrite(ledPinsRight[i], LOW);
  }
  for(int i = 1; i < 5; i++) {
    shouldwebuzz();
    digitalWrite(ledPinsLeft[i], HIGH);
    digitalWrite(ledPinsRight[i], HIGH);
    delay(100);
    digitalWrite(ledPinsLeft[i], LOW);
    digitalWrite(ledPinsRight[i], LOW);
  }
}


void ws2812_prettyLED() {
  /* Do something interesting */
/*  cylon(YELLOW, 500); */
}

// Implements a little larson "cylon" sanner.
// This'll run one full cycle, down one way and back the other
void ws2812_cylon(unsigned long color, unsigned long wait)
{
  Serial.println("Cylons are attacking.");
  // weight determines how much lighter the outer "eye" colors are
  const byte weight = 4;
  // It'll be easier to decrement each of these colors individually
  // so we'll split them out of the 24-bit color value
  byte red = (color & 0xFF0000) >> 16;
  byte green = (color & 0x00FF00) >> 8;
  byte blue = (color & 0x0000FF);

  // Start at closest LED, and move to the outside
  for (int i = 0; i <= LED_COUNT - 1; i++)
  {
    clearLEDs();
    rightTree.setPixelColor(i, red, green, blue);  // Set the bright middle eye
    leftTree.setPixelColor(i, red, green, blue);  // Set the bright middle eye
    // Now set two eyes to each side to get progressively dimmer
    for (int j = 1; j < 3; j++)
    {
      if (i - j >= 0)
        leftTree.setPixelColor(i - j, red / (weight * j), green / (weight * j), blue / (weight * j));
      rightTree.setPixelColor(i - j, red / (weight * j), green / (weight * j), blue / (weight * j));
      if (i - j <= LED_COUNT)
        leftTree.setPixelColor(i + j, red / (weight * j), green / (weight * j), blue / (weight * j));
      rightTree.setPixelColor(i + j, red / (weight * j), green / (weight * j), blue / (weight * j));
    }
    leftTree.show();  // Turn the LEDs on
    rightTree.show();  // Turn the LEDs on
    delay(wait);  // Delay for visibility
  }

  // Now we go back to where we came. Do the same thing.
  for (int i = LED_COUNT - 2; i >= 1; i--)
  {
    clearLEDs();
    leftTree.setPixelColor(i, red, green, blue);
    rightTree.setPixelColor(i, red, green, blue);
    for (int j = 1; j < 3; j++)
    {
      if (i - j >= 0)
        leftTree.setPixelColor(i - j, red / (weight * j), green / (weight * j), blue / (weight * j));
      rightTree.setPixelColor(i - j, red / (weight * j), green / (weight * j), blue / (weight * j));
      if (i - j <= LED_COUNT)
        leftTree.setPixelColor(i + j, red / (weight * j), green / (weight * j), blue / (weight * j));
      rightTree.setPixelColor(i + j, red / (weight * j), green / (weight * j), blue / (weight * j));
    }

    leftTree.show();
    rightTree.show();
    delay(wait);
  }
}

/* Song Easter Egg */
void easterEggSongCheck() {
  /* Lets see if we should play a song */
  if ( (millis() - lastSongCheck) > (songFrequency * 1000) ) {
    lastSongCheck = millis();
    if ( (random(0, 1000) < long(songProbability * 1000.0)) ) {
      Serial.println("Song probability check approved. Never give up!");
      playsong();
    } else {
      Serial.println("Song probability check declined.");
    }
  }
}

int frequency(char note)
{
  // This function takes a note character (a-g), and returns the
  // corresponding frequency in Hz for the tone() function.

  int i;
  const int numNotes = 8;  // number of notes we're storing

  // The following arrays hold the note characters and their
  // corresponding frequencies. The last "C" note is uppercase
  // to separate it from the first lowercase "c". If you want to
  // add more notes, you'll need to use unique characters.

  // For the "char" (character) type, we put single characters
  // in single quotes.

  char names[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C' };
  int frequencies[] = {262, 294, 330, 349, 392, 440, 494, 523};

  // Now we'll search through the letters in the array, and if
  // we find it, we'll return the frequency for that note.

  for (i = 0; i < numNotes; i++)  // Step through the notes
  {
    if (names[i] == note)         // Is this the one?
    {
      return (frequencies[i]);    // Yes! Return the frequency
    }
  }
  return (0); // We looked through everything and didn't find it,
  // but we still need to return a value, so return 0.
}

void playWinner() {
  /* TODO: Ideally, this is "Enjoy MonkeyBOX", but anything will do */
  /* Also, ideally won't block */
  if(random(0, 10) < 9) {
    playsong();
  } else {
    playsong2();
  }
  Serial.println("Insert winner music here.");
}

void startsong() {
  /* TODO: Make this something fancier */
  beep();
  delay(200);
  beep();
  delay(200);
  beep();
  delay(200);
}

void buzz() {
  /* play a loud, annoying buzz */
  Serial.println("*BUZZ*");
  tone(buzzerPin, buzzerFrequency, buzzerDuration);
  delay(buzzerDuration);
}

void buzz_nonblock() {
  /* play a loud, annoying buzz */
  Serial.println("*BUZZ*");
  tone(buzzerPin, buzzerFrequency, buzzerDuration);
}


void beep() {
  /* play a simple, pleasant beep */
  Serial.println("*beep*");
  tone(buzzerPin, beepFrequency, beepDuration);
  delay(beepDuration);
}

void beep_nonblock() {
  /* play a simple, pleasant beep, but don't wait for it to finish */
  Serial.println("*beep* (but don't wait for it)");
  tone(buzzerPin, beepFrequency, beepDuration);
}

void longbeep() {
  /* play a simple, pleasant beep */
  Serial.println("*beeeeeeeeeeeeeeeeeeeeeeeeeep*");
  tone(buzzerPin, beepFrequency, beepDuration * beepMultiplier);
  /* longbeep doesn't delay. Maybe that's inconsistent */
  /* delay(beepDuration * beepMultiplier); */
}

void playsong() {
  /* Plays a song.
      Cut directly from the SparkFun tutorial at
      https://learn.sparkfun.com/tutorials/sik-experiment-guide-for-arduino---v32/experiment-11-using-a-piezo-buzzer
  */
  int i, duration;

  for (i = 0; i < songLength; i++) // step through the song arrays
  {
    duration = beats[i] * tempo;  // length of note/rest in ms

    if (notes[i] == ' ')          // is this a rest?
    {
      delay(duration);            // then pause for a moment
    }
    else                          // otherwise, play the note
    {
      tone(buzzerPin, frequency(notes[i]), duration);
      delay(duration);            // wait for tone to finish
    }
    delay(tempo / 10);            // brief pause between notes
  }
}

void playsong2() {
  /* Plays a song.
      Cut directly from the SparkFun tutorial at
      https://learn.sparkfun.com/tutorials/sik-experiment-guide-for-arduino---v32/experiment-11-using-a-piezo-buzzer
  */
  int i, duration;

  for (i = 0; i < songLength2; i++) // step through the song arrays
  {
    duration = beats2[i] * tempo2;  // length of note/rest in ms

    if (notes2[i] == ' ')          // is this a rest?
    {
      delay(duration);            // then pause for a moment
    }
    else                          // otherwise, play the note
    {
      tone(buzzerPin, frequency(notes2[i]), duration);
      delay(duration);            // wait for tone to finish
    }
    delay(tempo2 / 10);            // brief pause between notes
  }
}

