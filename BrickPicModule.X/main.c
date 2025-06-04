/**
 * BrickLab I²C Slave - Stepper Motor Implementation
 * Uses brick_i2c_api.h for protocol definitions
 */

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "brick_i2c_api.h"  // Include the API header

// Configuration bits (same as MCC)
#pragma config FEXTOSC = ECH
#pragma config RSTOSC = HFINTOSC_64MHZ
#pragma config CLKOUTEN = OFF
#pragma config FCMEN = ON
#pragma config CSWEN = ON
#pragma config MCLRE = EXTMCLR
#pragma config BOREN = SBORDIS
#pragma config PWRTE = OFF
#pragma config LPBOREN = OFF
#pragma config XINST = OFF
#pragma config ZCD = OFF
#pragma config STVREN = ON
#pragma config BORV = VBOR_190
#pragma config PPS1WAY = ON
#pragma config WDTCPS = WDTCPS_31
#pragma config WDTE = OFF
#pragma config WDTCWS = WDTCWS_7
#pragma config WDTCCS = SC
#pragma config WRT0 = OFF
#pragma config WRT1 = OFF
#pragma config WRT2 = OFF
#pragma config WRT3 = OFF
#pragma config WRT4 = OFF
#pragma config WRT5 = OFF
#pragma config WRT6 = OFF
#pragma config WRT7 = OFF
#pragma config WRTB = OFF
#pragma config WRTC = OFF
#pragma config WRTD = OFF
#pragma config LVP = ON
#pragma config SCANE = ON
#pragma config CPD = OFF
#pragma config CP = OFF
#pragma config EBTR0 = OFF
#pragma config EBTR1 = OFF
#pragma config EBTR2 = OFF
#pragma config EBTR3 = OFF
#pragma config EBTR4 = OFF
#pragma config EBTR5 = OFF
#pragma config EBTR6 = OFF
#pragma config EBTR7 = OFF
#pragma config EBTRB = OFF

// I²C Client events (replicating i2c_client_types.h)
typedef enum {
    I2C_CLIENT_TRANSFER_EVENT_NONE = 0,
    I2C_CLIENT_TRANSFER_EVENT_ADDR_MATCH,
    I2C_CLIENT_TRANSFER_EVENT_RX_READY,
    I2C_CLIENT_TRANSFER_EVENT_TX_READY,
    I2C_CLIENT_TRANSFER_EVENT_STOP_BIT_RECEIVED,
    I2C_CLIENT_TRANSFER_EVENT_ERROR,
} i2c_client_transfer_event_t;

// Device UUID - STEPPER MOTOR
const uint8_t BRICK_DEVICE_UUID[16] = {
    0x42, 0x4c, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x93, 0x74, 0x67, 0x5a, 0x47, 0x12, 0xa0, 0x23
};

#define I2C_DEVICE_ADDRESS (BRICK_DEVICE_UUID[8])  // 0x93

// Stepper motor control functions
// Pins: S1=RC4, S2=RD3, S3=RD2, STEP=RC6, DIR=RC5
static inline void stepper_set_s1(uint8_t val)   { LATCbits.LATC4 = (val != 0); }
static inline void stepper_set_s2(uint8_t val)   { LATDbits.LATD3 = (val != 0); }
static inline void stepper_set_s3(uint8_t val)   { LATDbits.LATD2 = (val != 0); }
static inline void stepper_set_step(uint8_t val) { LATCbits.LATC6 = (val != 0); }
static inline void stepper_set_dir(uint8_t val)  { LATCbits.LATC5 = (val != 0); }

// LED control functions (for debugging/status)
static inline void digital_write_red(uint8_t val)   { LATDbits.LATD1 = (val != 0); }   // Status LED
static inline void digital_write_green(uint8_t val) { LATDbits.LATD0 = (val != 0); } // Status LED
static inline void digital_write_blue(uint8_t val)  { LATCbits.LATC7 = (val != 0); }  // Status LED

// Global variables
static volatile uint8_t rx_buf[8];   // cmd + payload (increased for stepper data)
static volatile uint8_t rx_idx = 0;  // number of bytes written so far
static volatile uint8_t tx_idx = 0;  // UUID byte index for reads

// Device state
static brick_device_impl_t device_state = {0};

// Callback function pointer
static bool (*i2c_callback_function)(i2c_client_transfer_event_t event) = NULL;

// Hand-written I²C functions (replicating MCC interface)
void I2C1_WriteByte(uint8_t wrByte) {
    SSP1BUF = wrByte;
    SSP1CON1bits.WCOL = 0;  // Clear write collision
}

uint8_t I2C1_ReadByte(void) {
    return SSP1BUF;
}

void I2C1_CallbackRegister(bool (*callback)(i2c_client_transfer_event_t clientEvent)) {
    i2c_callback_function = callback;
}

// System initialization functions
void CLOCK_Initialize(void) {
    OSCCON1 = (0 << _OSCCON1_NDIV_POSN) | (6 << _OSCCON1_NOSC_POSN);
    OSCCON3 = (0 << _OSCCON3_SOSCPWR_POSN) | (0 << _OSCCON3_CSWHOLD_POSN);
    OSCEN = 0x00;
    OSCFRQ = (8 << _OSCFRQ_HFFRQ_POSN);  // 64_MHz
    OSCTUNE = (0 << _OSCTUNE_TUN_POSN);
    while(!OSCSTATbits.HFOR);  // Wait for oscillator
}

void PIN_MANAGER_Initialize(void) {
    // LAT registers
    LATA = 0x0; LATB = 0x0; LATC = 0x3; LATD = 0x0; LATE = 0x0;
    
    // Open-drain for I²C
    ODCONA = 0x0; ODCONB = 0x0; ODCONC = 0x3; ODCOND = 0x0; ODCONE = 0x0;
    
    // TRIS registers
    TRISA = 0xFF; TRISB = 0xFF; TRISC = 0xFF; TRISD = 0xFF; TRISE = 0x7;
    
    // ANSEL registers (RC0, RC1 digital for I²C)
    ANSELA = 0xFF; ANSELB = 0xFF; ANSELC = 0xFC; ANSELD = 0xFF; ANSELE = 0x7;
    
    // Configure stepper motor pins as digital outputs
    // S1=RC4, S2=RD3, S3=RD2, STEP=RC6, DIR=RC5
    ANSELCbits.ANSELC4 = 0;  TRISCbits.TRISC4 = 0;  // S1
    ANSELDbits.ANSELD3 = 0;  TRISDbits.TRISD3 = 0;  // S2
    ANSELDbits.ANSELD2 = 0;  TRISDbits.TRISD2 = 0;  // S3
    ANSELCbits.ANSELC6 = 0;  TRISCbits.TRISC6 = 0;  // STEP
    ANSELCbits.ANSELC5 = 0;  TRISCbits.TRISC5 = 0;  // DIR
    
    // Configure status LED pins as digital outputs
    ANSELDbits.ANSELD0 = 0;  TRISDbits.TRISD0 = 0;  // Green status LED
    ANSELDbits.ANSELD1 = 0;  TRISDbits.TRISD1 = 0;  // Red status LED
    ANSELCbits.ANSELC7 = 0;  TRISCbits.TRISC7 = 0;  // Blue status LED
    
    // PPS configuration (same as MCC: RC0=SDA, RC1=SCL)
    PPSLOCK = 0x55; PPSLOCK = 0xAA; PPSLOCKbits.PPSLOCKED = 0;
    SSP1CLKPPS = 0x11;  RC1PPS = 0x0F;  // RC1->SCL
    SSP1DATPPS = 0x10;  RC0PPS = 0x10;  // RC0->SDA
    PPSLOCK = 0x55; PPSLOCK = 0xAA; PPSLOCKbits.PPSLOCKED = 1;
}

void I2C1_Initialize(void) {
    // Disable MSSP1 first
    SSP1CON1bits.SSPEN = 0;
    
    // Configure I²C slave mode
    SSP1CON1bits.SSPM = 0b0110;  // I²C Slave mode, 7-bit address
    SSP1ADD = I2C_DEVICE_ADDRESS;  // Set slave address
    
    // Configure control bits
    SSP1CON2bits.SEN = 1;     // Clock stretching enabled
    SSP1CON3bits.BOEN = 1;    // Buffer overwrite enable
    SSP1CON3bits.AHEN = 0;    // Address hold disabled
    SSP1CON3bits.DHEN = 0;    // Data hold disabled
    
    // Clear flags
    PIR3bits.SSP1IF = 0;
    PIR3bits.BCL1IF = 0;
    SSP1CON1bits.WCOL = 0;
    SSP1CON1bits.SSPOV = 0;
    
    // Enable MSSP1
    SSP1CON1bits.SSPEN = 1;
    SSP1CON1bits.CKP = 1;  // Release clock
}

void INTERRUPT_Initialize(void) {
    INTCONbits.IPEN = 0;   // Disable priority interrupts
    INTCONbits.PEIE = 1;   // Enable peripheral interrupts
    PIE3bits.SSP1IE = 1;   // Enable MSSP interrupt
    PIE3bits.BCL1IE = 1;   // Enable bus collision interrupt
    INTCONbits.GIE = 1;    // Enable global interrupts
}

void SYSTEM_Initialize(void) {
    CLOCK_Initialize();
    PIN_MANAGER_Initialize();
    I2C1_Initialize();
    INTERRUPT_Initialize();
}

void INTERRUPT_GlobalInterruptEnable(void) {
    INTCONbits.GIE = 1;
}

void INTERRUPT_PeripheralInterruptEnable(void) {
    INTCONbits.PEIE = 1;
}

// I²C ISR functions (replicating MCC generated ISR)
void I2C1_ISR(void) {
    PIR3bits.SSP1IF = 0;  // Clear interrupt flag
    
    if (i2c_callback_function) {
        if (!SSP1STATbits.D_nA) {
            // Address byte received
            uint8_t dummy = SSP1BUF;  // Read to clear
            (void)dummy;
            
            // Call address match callback
            i2c_callback_function(I2C_CLIENT_TRANSFER_EVENT_ADDR_MATCH);
            
            if (SSP1STATbits.R_nW) {
                // Master wants to read - call TX_READY
                i2c_callback_function(I2C_CLIENT_TRANSFER_EVENT_TX_READY);
            }
        } else {
            // Data byte
            if (!SSP1STATbits.R_nW) {
                // Master writing - call RX_READY
                i2c_callback_function(I2C_CLIENT_TRANSFER_EVENT_RX_READY);
            } else {
                // Master reading - call TX_READY
                i2c_callback_function(I2C_CLIENT_TRANSFER_EVENT_TX_READY);
            }
        }
    }
    
    // Release clock
    SSP1CON1bits.CKP = 1;
    
    // Check for STOP condition
    if (!SSP1STATbits.S) {
        if (i2c_callback_function) {
            i2c_callback_function(I2C_CLIENT_TRANSFER_EVENT_STOP_BIT_RECEIVED);
        }
    }
}

void I2C1_ERROR_ISR(void) {
    PIR3bits.BCL1IF = 0;  // Clear bus collision flag
    uint8_t dummy = SSP1BUF;  // Read to clear
    (void)dummy;
    SSP1CON1bits.WCOL = 0;
    SSP1CON1bits.SSPOV = 0;
    SSP1CON1bits.CKP = 1;
    
    if (i2c_callback_function) {
        i2c_callback_function(I2C_CLIENT_TRANSFER_EVENT_ERROR);
    }
}

// Main interrupt manager (replicating MCC interrupt.c)
void __interrupt() INTERRUPT_InterruptManager(void) {
    if (INTCONbits.PEIE == 1) {
        if (PIE3bits.BCL1IE == 1 && PIR3bits.BCL1IF == 1) {
            I2C1_ERROR_ISR();
        } else if (PIE3bits.SSP1IE == 1 && PIR3bits.SSP1IF == 1) {
            I2C1_ISR();
        }
    }
}

// Apply stepper motor command
static void apply_stepper_payload(void) {
    // rx_buf[1] = stepper instruction byte
    // Bit 0 = STEP, Bit 1 = S1, Bit 2 = S2, Bit 3 = S3
    uint8_t instr = rx_buf[1];
    
    // Store instruction in device state
    device_state.stepper_motor.instr = instr;
    
    // Set stepper control pins
    stepper_set_step(instr & 0x01);  // Bit 0 = STEP
    stepper_set_s1((instr >> 1) & 0x01);   // Bit 1 = S1
    stepper_set_s2((instr >> 2) & 0x01);   // Bit 2 = S2
    stepper_set_s3((instr >> 3) & 0x01);   // Bit 3 = S3
    
    // Optional: Set direction based on higher bits or separate command
    // For now, direction can be controlled via separate logic
    
    // Status indication
    digital_write_blue(instr != 0);  // Blue LED shows stepper activity
}

// Apply RGB payload (for debugging/status)
static void apply_rgb_payload(void) {
    // rx_buf[1] = R, rx_buf[2] = G, rx_buf[3] = B
    digital_write_red(rx_buf[1]);
    digital_write_green(rx_buf[2]);
    digital_write_blue(rx_buf[3]);
}

// I²C Callback function (modified for stepper motor)
bool I2C_Callback(i2c_client_transfer_event_t event) {
    switch (event) {
        case I2C_CLIENT_TRANSFER_EVENT_ADDR_MATCH:
            rx_idx = 0;
            tx_idx = 0;
            digital_write_red(1);  // Red LED on during I²C transaction
            return true;

        case I2C_CLIENT_TRANSFER_EVENT_TX_READY:
            if (tx_idx < sizeof(BRICK_DEVICE_UUID)) {
                I2C1_WriteByte(BRICK_DEVICE_UUID[tx_idx++]);
                return true;
            }
            return false;

        case I2C_CLIENT_TRANSFER_EVENT_RX_READY:
            if (rx_idx < sizeof(rx_buf)) {
                rx_buf[rx_idx++] = I2C1_ReadByte();
                return true;
            }
            (void)I2C1_ReadByte(); // overflow protection
            return true;

        case I2C_CLIENT_TRANSFER_EVENT_STOP_BIT_RECEIVED:
            digital_write_red(0);  // Red LED off
            
            if (rx_idx == 0)  // No command
                return true;

            switch ((brick_command_type_t)rx_buf[0]) {  // rx_buf[0] = command ID
                case CMD_STEPPER_MOVE:
                    if (rx_idx >= 2) {  // cmd + 1 instruction byte minimum
                        apply_stepper_payload();
                    }
                    break;

                case CMD_LED_RGB:
                    if (rx_idx == 4)  // cmd + 3 payload bytes (for status LEDs)
                        apply_rgb_payload();
                    break;

                case CMD_SERVO_SET_ANGLE:
                    // Add servo control here if needed
                    break;

                default:
                    break;
            }
            rx_idx = 0;  // ready for next packet
            return true;

        default:
            return false;
    }
}

// Main function
int main(void) {
    SYSTEM_Initialize();

    // Initialize stepper motor pins (all off initially)
    stepper_set_s1(0);
    stepper_set_s2(0);
    stepper_set_s3(0);
    stepper_set_step(0);
    stepper_set_dir(0);
    
    // Initialize status LEDs (all off initially)
    digital_write_red(0);
    digital_write_green(0);
    digital_write_blue(0);

    // Register I²C callback
    I2C1_CallbackRegister(I2C_Callback);

    // Enable interrupts
    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();

  
    while (1) {
        // Keep watchdog happy if enabled
        CLRWDT();
    }
    
    return 0;
}