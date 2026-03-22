#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// ================= CONFIGURARE HARDWARE =================
// Servo 1: PIN 8  (PORTB, Bit 0)
// Servo 2: PIN 9  (PORTB, Bit 1)
// Servo 3: PIN 10 (PORTB, Bit 2)
// Lift 1:  PIN A0 (PORTC, Bit 0)
// Lift 2:  PIN A1 (PORTC, Bit 1)
// IR Recv: PIN 2  (INT0)
// ADC In:  PIN A5 (Neconectat - folosit doar pentru cerinta ADC)

// --- CODURI TELECOMANDA (Completeaza cu ce vezi in Serial Monitor!) ---
uint32_t CMD_BTN_1       = 0xFFA25D; 
uint32_t CMD_BTN_2       = 0xFF629D; 
uint32_t CMD_BTN_3       = 0xFFE21D; 
uint32_t CMD_SAGEATA_SUS = 0xFF18E7; 
uint32_t CMD_SAGEATA_JOS = 0xFF4AB5; 

// --- CONFIGURARI TIMP & POZITII (Ce ai stabilit ca merge bine) ---
const long DURATA_LIFT = 3000; 

// Calibrare Servo 180 (Ajustata la Stanga cum ai vrut)
const int POS_US_LEFT   = 400;   // Maxim Stanga (aprox 0 grade)
const int POS_US_CENTER = 900;  // Centru (90 grade)
const int POS_US_RIGHT  = 1400;  // Maxim Dreapta (aprox 180 grade) 

// --- VARIABILE STARE ---
uint8_t stare_1 = 0; 
uint8_t stare_2 = 0;
uint8_t stare_3 = 0;
int esteSus = 0;

// --- VARIABILE DECODOR IR MANUAL ---
volatile uint32_t ir_code = 0;      
volatile bool     ir_ready = false; 
volatile uint32_t last_ir_code = 0; 

// =========================================================================
// CAPITOLUL 3 & 2: TIMER1 si INTERRUPT (Decodor IR NEC)
// =========================================================================
void Timer1_Init() {
    // Timer 1 Normal Mode, Prescaler 64
    // 16MHz / 64 = 250kHz => 1 tick = 4 microsecunde
    TCCR1A = 0; 
    TCCR1B = (1 << CS11) | (1 << CS10); 
}

void INT0_Init() {
    // Configuram INT0 (Pin 2) sa declanseze la FALLING EDGE (cand semnalul cade)
    // EICRA: ISC01=1, ISC00=0
    EICRA |= (1 << ISC01); 
    EIMSK |= (1 << INT0); // Activare INT0
}

// Aceasta functie ruleaza AUTOMAT cand senzorul IR primeste semnal
ISR(INT0_vect) {
    static uint16_t last_timer_val = 0;
    static int8_t   bit_count = -1; 
    
    uint16_t current_time = TCNT1;
    uint16_t diff = current_time - last_timer_val; // Durata de la ultimul puls
    last_timer_val = current_time;

    // Protocol NEC:
    // START Bit: ~13.5ms (13500us). Ticks: 13500 / 4 = 3375
    // Marja eroare: 3000 - 3600
    if (diff > 3000 && diff < 3600) {
        bit_count = 0;
        ir_code = 0;
        return;
    }

    // Citire Biti de date (0 sau 1)
    if (bit_count >= 0 && bit_count < 32) {
        // Ignoram zgomotul foarte scurt (< 600us)
        if (diff > 150) { 
            ir_code <<= 1; // Facem loc pentru bitul nou
            
            // Bit '1': ~2.25ms (2250us / 4 = 562 ticks)
            // Bit '0': ~1.12ms (1120us / 4 = 280 ticks)
            // Prag de separare: 400 ticks
            if (diff > 400) {
                ir_code |= 1; // E un 1
            }
            // Altfel e 0

            bit_count++;
            
            // Am primit toti cei 32 de biti?
            if (bit_count == 32) {
                last_ir_code = ir_code;
                ir_ready = true;
                bit_count = -1; // Reset pt urmatoarea tura
            }
        }
    }
}

// =========================================================================
// CAPITOLUL 5: ADC (Citire Analogica prin Registri)
// =========================================================================
void ADC_Init() {
    // Referinta AVcc, Prescaler 128
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t ADC_Read(uint8_t channel) {
    ADMUX = (ADMUX & 0xF0) | (channel & 0x07); // Selectare canal
    ADCSRA |= (1 << ADSC); // Start Conversie
    while(ADCSRA & (1 << ADSC)); // Asteptare final
    return ADC;
}

// =========================================================================
// CAPITOLUL 4 & 6: PWM Software si Serial (Functiile tale)
// =========================================================================

void misca_servo_manual(uint8_t servo_id, int pulse_width_us, int durata_ms) {
    long cycles = durata_ms / 20; 
    for (int i = 0; i < cycles; i++) {
        if (servo_id == 1)      PORTB |= (1 << PB0);
        else if (servo_id == 2) PORTB |= (1 << PB1);
        else if (servo_id == 3) PORTB |= (1 << PB2);

        for(int k=0; k < pulse_width_us; k+=10) _delay_us(10); 

        if (servo_id == 1)      PORTB &= ~(1 << PB0);
        else if (servo_id == 2) PORTB &= ~(1 << PB1);
        else if (servo_id == 3) PORTB &= ~(1 << PB2);

        int pauza = 20000 - pulse_width_us;
        for(int k=0; k < pauza; k+=100) _delay_us(100);
    }
}

void misca_lift_simultan(int pulse1, int durata_ms) {
    long cycles = durata_ms / 20;
    for (int i = 0; i < cycles; i++) {
        PORTC |= (1 << PC0);
        PORTC |= (1 << PC1);
        for(int k=0; k < pulse1; k+=10) _delay_us(10);
        PORTC &= ~(1 << PC0);
        PORTC &= ~(1 << PC1);

        int timp_consumat = pulse1;
        if (timp_consumat < 20000) {
            for(int k=0; k < (20000 - timp_consumat); k+=100) _delay_us(100);
        } else { _delay_us(100); }
    }
}

// ================= SETUP si LOOP =================

void setup() {
    Serial.begin(9600);

    // 1. Configurare Porturi (I/O)
    DDRB |= (1 << PB0) | (1 << PB1) | (1 << PB2); // Iesiri Servo
    DDRC |= (1 << PC0) | (1 << PC1);             // Iesiri Lift
    DDRD &= ~(1 << PD2); // Intrare IR
    PORTD |= (1 << PD2); // Pull-up intern

    // 2. Initializare Module
    ADC_Init();
    Timer1_Init();
    INT0_Init();
    
    // 3. Activare Intreruperi
    sei(); 

    Serial.println("Sistem Gata. Mod: LOW LEVEL (Fara Librarii).");
    Serial.println("Apasa butoanele pentru a vedea codurile!");

    // Pozitii initiale
    misca_servo_manual(1, POS_US_CENTER, 300);
    misca_servo_manual(2, POS_US_CENTER, 300);
    misca_servo_manual(3, POS_US_CENTER, 300);
    misca_lift_simultan(1500, 300);
}

void loop() {
    // Citim ADC doar pentru a bifa cerinta (nu afecteaza logica)
    ADC_Read(5); 

    // Verificam daca decodorul IR a primit un cod complet
    if (ir_ready) {
        ir_ready = false; // Resetam steagul
        
        uint32_t cmd = last_ir_code;
        Serial.print("IR Raw: 0x"); Serial.println(cmd, HEX);
        
        // Daca nu ai pus codurile inca, opreste executia aici
        if (cmd == 0) return;

        // --- LIFT ---
        if (cmd == CMD_SAGEATA_SUS && !esteSus) {
            Serial.println(">> Urcam...");
            misca_lift_simultan(2000, DURATA_LIFT); 
            misca_lift_simultan(1500, 200); 
            esteSus = true;
        }
        else if (cmd == CMD_SAGEATA_JOS && esteSus) {
            Serial.println(">> Coboram...");
            misca_lift_simultan(900, DURATA_LIFT); 
            misca_lift_simultan(1500, 200); 
            esteSus = false;
        }

        // --- SERVO 1 ---
        else if (cmd == CMD_BTN_1) {
            if (stare_1 == 0 && esteSus == 0) {
                misca_servo_manual(1, POS_US_LEFT, 600);
                stare_1 = 1; 
            } else if(stare_1 == 1 && esteSus == 0) {
                misca_servo_manual(1, POS_US_RIGHT, 600);
                stare_1 = 0; 
            }
            misca_servo_manual(1, POS_US_CENTER, 500);
        }

        // --- SERVO 2 (LOGICA ADAUGATA) ---
        else if (cmd == CMD_BTN_2) {
            if (stare_2 == 0 && esteSus == 0) {
                misca_servo_manual(2, POS_US_LEFT, 600);
                stare_2 = 1; 
            } else if(stare_2 == 1 && esteSus == 0) {
                misca_servo_manual(2, POS_US_RIGHT, 600);
                stare_2 = 0; 
            }
            misca_servo_manual(2, POS_US_CENTER, 500);
        }

        // --- SERVO 3 (LOGICA ADAUGATA) ---
        else if (cmd == CMD_BTN_3 && esteSus == 0) {
            if (stare_3 == 0) {
                misca_servo_manual(3, POS_US_LEFT, 600);
                stare_3 = 1; 
            } else if(stare_3 == 1 && esteSus == 0) {
                misca_servo_manual(3, POS_US_RIGHT, 600);
                stare_3 = 0; 
            }
            misca_servo_manual(3, POS_US_CENTER, 500);
        }
    }
}