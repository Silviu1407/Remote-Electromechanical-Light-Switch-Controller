#include <IRremote.hpp>
#include <avr/io.h>
#include <util/delay.h>

// ================= MAPARE HARDWARE =================
// PIN 8, 9, 10 -> PORTB bitii 0, 1, 2 (Servo Mici)
// PIN A0, A1   -> PORTC bitii 0, 1    (Lift)
// PIN A5       -> ADC Canal 5         (Potentiometru)

const int IR_PIN = 2; 

// CODURI TELECOMANDA
const uint16_t CMD_BTN_1       = 0x45; 
const uint16_t CMD_BTN_2       = 0x46; 
const uint16_t CMD_BTN_3       = 0x47; 
const uint16_t CMD_SAGEATA_SUS = 0x18; 
const uint16_t CMD_SAGEATA_JOS = 0x52; 

uint8_t stare_1 = 0;
uint8_t stare_2 = 0;
uint8_t stare_3 = 0;
bool esteSus = false;

// --- RECALIBRARE UNGHIURI (Solutia pt Problema 1) ---
// 1500us este Centru. +/- 250us inseamna aprox 45 grade.
// Inainte era +/- 500us (care insemna 90 grade).
const int POS_US_LEFT   = 1250; 
const int POS_US_CENTER = 1500; 
const int POS_US_RIGHT  = 1750; 

// --- Configurare ADC ---
void ADC_Init() {
    ADMUX = 0b01000000; // Ref 5V
    ADCSRA = 0b10000111; // Enable + Prescaler 128
}

uint16_t ADC_Read(uint8_t channel) {
    ADMUX = (ADMUX & 0b11110000) | (channel & 0b00001111);
    ADCSRA |= 0b01000000; // Start
    while (ADCSRA & 0b01000000); // Wait
    return ADCW;
}

// --- PWM MANUAL SERVO MICI ---
void misca_servo_manual(uint8_t servo_id, int pulse_width_us, int durata_ms) {
    long cycles = durata_ms / 20; 
    
    for (int i = 0; i < cycles; i++) {
        // 1. HIGH
        if (servo_id == 1)      PORTB |= 0b00000001; 
        else if (servo_id == 2) PORTB |= 0b00000010; 
        else if (servo_id == 3) PORTB |= 0b00000100; 

        // 2. Pulse Width
        for(int k=0; k<pulse_width_us; k+=10) _delay_us(10); 

        // 3. LOW
        if (servo_id == 1)      PORTB &= 0b11111110; 
        else if (servo_id == 2) PORTB &= 0b11111101; 
        else if (servo_id == 3) PORTB &= 0b11111011; 

        // 4. Wait rest of cycle
        for(int k=0; k<(20000 - pulse_width_us); k+=100) _delay_us(100);
    }
}

// --- PWM LIFT - METODA SECVENTIALA (Solutia pt Problema 2) ---
// Aceasta metoda garanteaza ca ambele primesc semnal corect in fiecare ciclu
void misca_lift_simultan(int pulse1, int pulse2, int durata_ms) {
    long cycles = durata_ms / 20;
    
    for (int i = 0; i < cycles; i++) {
        // --- MOTOR 1 (A0) ---
        PORTC |= 0b00000001; // HIGH A0
        for(int k=0; k<pulse1; k+=10) _delay_us(10);
        PORTC &= 0b11111110; // LOW A0

        // --- MOTOR 2 (A1) ---
        // Pornim imediat ce s-a terminat primul puls
        PORTC |= 0b00000010; // HIGH A1
        for(int k=0; k<pulse2; k+=10) _delay_us(10);
        PORTC &= 0b11111101; // LOW A1

        // --- PAUZA ---
        // Scadem ambele pulsuri din cei 20000us (20ms)
        // Daca puls1=2400 si puls2=544, suma e aprox 3000. Raman 17000.
        int timp_consumat = pulse1 + pulse2;
        for(int k=0; k<(20000 - timp_consumat); k+=100) _delay_us(100);
    }
}

void setup() {
    Serial.begin(9600);

    // Configurare DDR (1=Output)
    DDRB |= 0b00000111; // 8, 9, 10 Output
    DDRC |= 0b00000011; // A0, A1 Output
    
    ADC_Init();

    IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);
    
    // Initializare pozitii
    misca_servo_manual(1, POS_US_CENTER, 100);
    misca_servo_manual(2, POS_US_CENTER, 100);
    misca_servo_manual(3, POS_US_CENTER, 100);
    
    // Stop Lift (1500us = Stop la servo 360)
    misca_lift_simultan(1500, 1500, 100);

    Serial.println("Sistem Calibrat: Unghiuri mici & Lift sincronizat.");
}

void loop() {
    uint16_t val_pot = ADC_Read(5); 
    long durata_lift = 2000 + ((long)val_pot * 4000 / 1023);

    if (IrReceiver.decode()) {
        uint16_t cmd = IrReceiver.decodedIRData.command;
        
        // --- LIFT ---
        if (cmd == CMD_SAGEATA_SUS && !esteSus) {
            Serial.println(">> Urcam Lift...");
            // Servo 360: 
            // 2000 = Viteza Max Sens Orar
            // 1000 = Viteza Max Sens Anti-Orar
            // Ajusteaza valorile daca se invart invers!
            misca_lift_simultan(2000, 1000, durata_lift);
            
            // Stop
            misca_lift_simultan(1500, 1500, 200); 
            esteSus = true;
        }
        else if (cmd == CMD_SAGEATA_JOS && esteSus) {
            Serial.println(">> Coboram Lift...");
            // Inversam sensurile
            misca_lift_simultan(1000, 2000, durata_lift);
            
            // Stop
            misca_lift_simultan(1500, 1500, 200);
            esteSus = false;
        }

        // --- SERVO MICI (UNGHIURI REGLATE) ---
        else if (cmd == CMD_BTN_1) {
            int target;
            if (stare_1 == 0) { target = POS_US_LEFT; } 
            else { target = POS_US_RIGHT; }

            misca_servo_manual(1, target, 1000); 
            if(stare_1==0) stare_1=1; else stare_1=0;
            misca_servo_manual(1, POS_US_CENTER, 300);
        }
        else if (cmd == CMD_BTN_2) {
            int target;
            if (stare_2 == 0) { target = POS_US_RIGHT; } 
            else { target = POS_US_LEFT; }

            misca_servo_manual(2, target, 1000);
            if(stare_2==0) stare_2=1; else stare_2=0;
            misca_servo_manual(2, POS_US_CENTER, 300);
        }
        else if (cmd == CMD_BTN_3) {
            int target;
            if (stare_3 == 0) { target = POS_US_LEFT; } 
            else { target = POS_US_RIGHT; }

            misca_servo_manual(3, target, 1000);
            if(stare_3==0) stare_3=1; else stare_3=0;
            misca_servo_manual(3, POS_US_CENTER, 300);
        }

        IrReceiver.resume();
    }
}