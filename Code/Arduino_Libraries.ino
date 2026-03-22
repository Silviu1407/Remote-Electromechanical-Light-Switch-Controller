#include <Servo.h>
#include <IRremote.hpp>

// --- PINI EXISTENTI (Servo 180) ---
const int SERVO_PIN_1 = 8;
const int SERVO_PIN_2 = 9;
const int SERVO_PIN_3 = 10;

// --- PINI NOI (LIFT - PE ANALOG!) ---
// Folosim A0 si A1 ca sa evitam conflictele cu Timer 0 si Timer 2
const int SERVO_LIFT_1_PIN = A0; 
const int SERVO_LIFT_2_PIN = A1; 

const int IR_PIN = 2;

// --- SETĂRI LIFT ---
const int DURATA_MISCARE = 3000; 
bool esteSus = false;

// --- POZIȚII ---
const int POS_LEFT   = 45;
const int POS_CENTER = 90;
const int POS_RIGHT  = 135;

const uint16_t CMD_BTN_1  = 0x45; 
const uint16_t CMD_BTN_2  = 0x46; 
const uint16_t CMD_BTN_3  = 0x47; 

uint8_t STARE_ANTERIOARA_1 = 0;
uint8_t STARE_ANTERIOARA_2 = 0;
uint8_t STARE_ANTERIOARA_3 = 0;

const uint16_t CMD_SAGEATA_SUS = 0x18; // Pune codul tau!
const uint16_t CMD_SAGEATA_JOS = 0x52; // Pune codul tau!

Servo myServo1, myServo2, myServo3;
Servo liftServo1, liftServo2;

const int DURATA_REVENIRE  = 750;

void setup() {
  Serial.begin(9600);
  
  // Atasare Servo 180
  myServo1.attach(SERVO_PIN_1);
  myServo2.attach(SERVO_PIN_2);
  myServo3.attach(SERVO_PIN_3);
  
  myServo1.write(POS_CENTER);
  myServo2.write(POS_CENTER);
  myServo3.write(POS_CENTER);

  // Atasare Lift (PE A0 si A1)
  liftServo1.attach(SERVO_LIFT_1_PIN);
  liftServo2.attach(SERVO_LIFT_2_PIN);

  // STOP INITIAL
  liftServo1.write(90);
  liftServo2.write(90);

  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);
  Serial.println("Sistem gata! Lift pe A0 si A1.");
}

void loop() {
  if (IrReceiver.decode()) {
    uint16_t cmd = IrReceiver.decodedIRData.command;
    if (cmd != 0) { Serial.print("Cod: 0x"); Serial.println(cmd, HEX); }

    // --- LIFT ---
    if (cmd == CMD_SAGEATA_SUS && !esteSus) {
        Serial.println(">> Urcam...");
        liftServo1.write(180);
        delay(100);          // Pauza antishoc
        liftServo2.write(0); // Sens opus
        delay(DURATA_MISCARE);
        liftServo1.write(90); liftServo2.write(90); // Stop
        esteSus = true;
    }
    else if (cmd == CMD_SAGEATA_JOS && esteSus) {
        Serial.println(">> Coboram...");
        liftServo1.write(0);
        delay(100);          // Pauza antishoc
        liftServo2.write(180);
        delay(DURATA_MISCARE);
        liftServo1.write(90); liftServo2.write(90); // Stop
        esteSus = false;
    }

    // --- SELECTIE & CONTROL 180 ---
    else if (cmd == CMD_BTN_1 && STARE_ANTERIOARA_1 == 0) 
    {
        myServo1.write(POS_LEFT);
        STARE_ANTERIOARA_1 = 1;
        delay(DURATA_REVENIRE);
        myServo1.write(POS_CENTER);
    }
    else if (cmd == CMD_BTN_1 && STARE_ANTERIOARA_1 == 1) 
    {
        myServo1.write(POS_RIGHT);
        STARE_ANTERIOARA_1 = 0;
        delay(DURATA_REVENIRE);
        myServo1.write(POS_CENTER);
    }

    else if (cmd == CMD_BTN_2 && STARE_ANTERIOARA_2 == 0)
    {
        myServo2.write(POS_RIGHT);
        STARE_ANTERIOARA_2 = 1;
        delay(DURATA_REVENIRE);
        myServo2.write(POS_CENTER);
    }
    else if (cmd == CMD_BTN_2 && STARE_ANTERIOARA_2 == 1)
    {
      myServo2.write(POS_LEFT);
      STARE_ANTERIOARA_2 = 0;
      delay(DURATA_REVENIRE);
      myServo2.write(POS_CENTER);
    }

    else if (cmd == CMD_BTN_3 && STARE_ANTERIOARA_3 == 0)
    {
        myServo3.write(POS_LEFT);
        STARE_ANTERIOARA_3 = 1;
        delay(DURATA_REVENIRE);
        myServo3.write(POS_CENTER);
    }
    else if (cmd == CMD_BTN_3 && STARE_ANTERIOARA_3 == 1)
    {
      myServo3.write(POS_RIGHT);
      STARE_ANTERIOARA_3 = 0;
      delay(DURATA_REVENIRE);
      myServo3.write(POS_CENTER);
    }

    IrReceiver.resume();
  }
}