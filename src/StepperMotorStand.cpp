#include <Arduino.h>
#include <util/atomic.h>



#define MOTOR_DDR DDRC
#define MOTOR_PORT PORTC
#define MOTOR_STOP PORTC&=(~((1<<PC2)|(1<<PC3)|(1<<PC4)|(1<<PC5)))
#define MOTOR_TIMER_START TIMSK2|=(1<<OCIE2A)
#define MOTOR_TIMER_STOP TIMSK2&=(~(1<<OCIE2A))
#define MOTOR_HIGH_LEVEL_ELEMENTARY_DELAY 64  // us (for motor_move() functions)
uint8_t motor_high_level_delay;
uint8_t EEMEM motor_high_level_delay_EEPROM = 32;

void motor_move(int16_t steps);
void motor_move_non_blocking(int16_t steps);
void motor_up(void);
void motor_down(void);
void motor_stop(void);
int8_t motor_up_flag = 0;
int8_t last_step = 0;
uint16_t steps_move = 0;

uint8_t EEMEM OCR2A_EEPROM_value = 31;
uint8_t full_step_flag;
uint8_t EEMEM full_step_flag_EEPROM = 0;


#define SIGNALS_DDR DDRB
#define SIGNALS_PORT PORTB
#define SIGNALS_PIN PINB
#define ENDSTOP_1 PB2
#define ENDSTOP_2 PB3
uint8_t signals_port_history = 0xFF;


#include <USART.h>
#define USART_BUFFER_SIZE 25
#define ATOI_STR_SIZE 10



int main(void) {

    ATOMIC_BLOCK(ATOMIC_FORCEON) {

        USART_init();
        USART_putstring("\n\nStepper motor control driver. Enter 'help' for instructions\n> ");

        MOTOR_DDR |= (1<<PC2)|(1<<PC3)|(1<<PC4)|(1<<PC5);

        // Interrupt setup for endstops
        PCICR |= (1<<PCIE0);
        PCMSK0 |= (1<<PCINT2)|(1<<PCINT3);

        // Timer2 for motor motion algorithm
        TCCR2A |= (1<<WGM21);  // CTC mode
        OCR2A = eeprom_read_byte(&OCR2A_EEPROM_value);
        TCCR2B = (1<<CS20)|(1<<CS21)|(1<<CS22);  // prescaler 1024

        motor_high_level_delay = eeprom_read_byte(&motor_high_level_delay_EEPROM);
        full_step_flag = eeprom_read_byte(&full_step_flag_EEPROM);

    }

    motor_up();

    while (1) {}
}



// Endstops interrupt
ISR (PCINT0_vect) {

    _delay_ms(50);

    uint8_t changed_bits;
    changed_bits = SIGNALS_PIN ^ signals_port_history;
    signals_port_history = SIGNALS_PIN;

    if ( changed_bits & (1<<ENDSTOP_1) ) {
        if ( !(SIGNALS_PIN & (1<<ENDSTOP_1)) ) {
            /* HIGH to LOW pin change */
            motor_down();
        }
    }

    else if ( (changed_bits & (1<<ENDSTOP_2)) ) {
        if ( !(SIGNALS_PIN & (1<<ENDSTOP_2)) ) {
            /* HIGH to LOW pin change */
            motor_up();
        }
    }

}



// ISR for timer for motor
ISR (TIMER2_COMPA_vect) {

    if (full_step_flag) {
        if (motor_up_flag) {
            if (++last_step == 3) {
                MOTOR_PORT = (1<<(3+2)) | (1<<(0+2));
                last_step = -1;
            }
            else {
                MOTOR_PORT = (1<<(last_step+2)) | (1<<(last_step+1+2));
            }
        }

        else {
            if (--last_step == -1) {
                MOTOR_PORT = (1<<(0+2)) | (1<<(3+2));
                last_step = 3;
            }
            else {
                MOTOR_PORT = (1<<(last_step+1+2)) | (1<<(last_step+2));
            }
        }
    }

    else {
        MOTOR_PORT = 1 << (last_step+2);

        if (motor_up_flag) {
            if (++last_step == 4) last_step = 0;
        }

        else {
            if (--last_step == -1) last_step = 3;
        }
    }

    if (steps_move > 0) {
        if (--steps_move <= 1) {
            steps_move = 0;
            motor_stop();
        }
    }

}



void motor_up(void) {
    motor_up_flag = true;
    MOTOR_TIMER_START;
}



void motor_down(void) {
    motor_up_flag = false;
    MOTOR_TIMER_START;
}



void motor_stop(void) {
    MOTOR_TIMER_STOP;
    MOTOR_STOP;
    motor_up_flag = -1;
}



void motor_move(int16_t steps) {

    int16_t steps_cnt;
    uint8_t motor_high_level_delay_cnt;

    if (steps > 0) {
        for (steps_cnt=steps; steps_cnt>0; steps_cnt--) {

            if (full_step_flag) {
                if (++last_step == 3) {
                    MOTOR_PORT = (1<<(3+2)) | (1<<(0+2));
                    last_step = -1;
                }
                else {
                    MOTOR_PORT = (1<<(last_step+2)) | (1<<(last_step+1+2));
                }
            }

            else {
                MOTOR_PORT = 1<<(last_step+2);
                if (++last_step==4) last_step=0;
            }

            for (motor_high_level_delay_cnt=0; motor_high_level_delay_cnt<motor_high_level_delay; motor_high_level_delay_cnt++) {
                _delay_us(MOTOR_HIGH_LEVEL_ELEMENTARY_DELAY);
            }

        }
    }

    else if (steps < 0) {
        for (steps_cnt=abs(steps); steps_cnt>0; steps_cnt--) {

            if (full_step_flag) {
                    if (--last_step == -1) {
                        MOTOR_PORT = (1<<(0+2)) | (1<<(3+2));
                        last_step = 3;
                    }
                    else {
                        MOTOR_PORT = (1<<(last_step+1+2)) | (1<<(last_step+2));
                    }
            }

            else {
                MOTOR_PORT = 1<<(last_step+2);
                if (--last_step==-1) last_step=3;
            }

            for (motor_high_level_delay_cnt=0; motor_high_level_delay_cnt<motor_high_level_delay; motor_high_level_delay_cnt++) {
                _delay_us(MOTOR_HIGH_LEVEL_ELEMENTARY_DELAY);
            }

        }
    }

    motor_stop();

}



void motor_move_non_blocking(int16_t steps) {
    if (steps > 0)
        motor_up_flag = 1;
    else if (steps < 0)
        motor_up_flag = 0;
    else
        return;

    steps_move = abs(steps) + 1;
    MOTOR_TIMER_START;
}



ISR (USART_RX_vect) {

    static uint8_t usart_char, usart_buffer_cnt=0;
    static char usart_buffer[USART_BUFFER_SIZE];
    static bool can_handle_command = false;

    usart_char = USART_receive();
    usart_buffer[usart_buffer_cnt] = usart_char;
    USART_send(usart_char);
    if (usart_char == '\n') {
        can_handle_command = true;
    }
    else if (++usart_buffer_cnt == USART_BUFFER_SIZE) {
        USART_send('\n');
        can_handle_command = true;
    }


    if (can_handle_command) {
        can_handle_command = false;
        usart_buffer_cnt = 0;

        int16_t temp_number;
        char atoi_str[ATOI_STR_SIZE];

        if ( strncmp("pulse ", usart_buffer, 6) == 0 ) {
            // new value located in string, starts from 6th symbol and has length in 5 symbols:
            strncpy(atoi_str, (char *)&(usart_buffer[6]), 5);
            temp_number = atoi(atoi_str);

            if ( (temp_number>=64) && (temp_number<=16384) ) {
                OCR2A = map(temp_number, 64, 16384, 0, 255);
                motor_high_level_delay = temp_number/MOTOR_HIGH_LEVEL_ELEMENTARY_DELAY;
            }
            else
                USART_putstring("High-level pulse duration must be 64us <= pulse <= 16384us\n");
        }

        else if ( strncmp("fullstep", usart_buffer, 8) == 0 ) {
            full_step_flag = 1;
        }

        else if ( strncmp("wave", usart_buffer, 4) == 0 ) {
            full_step_flag = 0;
        }

        else if ( strncmp("stop", usart_buffer, 4) == 0 ) {
            motor_stop();
        }

        else if ( strncmp("move ", usart_buffer, 5) == 0 ) {
            strncpy(atoi_str, (char *)&(usart_buffer[5]), 5+1);  // 1 for sign
            temp_number = atoi(atoi_str);

            motor_move(temp_number);
        }

        else if ( strncmp("movenb ", usart_buffer, 7) == 0 ) {
            strncpy(atoi_str, (char *)&(usart_buffer[7]), 5+1);  // 1 for sign
            temp_number = atoi(atoi_str);

            motor_move_non_blocking(temp_number);
        }

        else if ( strncmp("help", usart_buffer, 4) == 0 ) {
            USART_putstring(
                "Commands:\n\n\tpulse [value]\n\t\tSet the duration of the high-level pulse in microseconds (from 64us to 16384us). "
                "Values like '12345678' will be interpreted as 12345\n\n"
                "\tfullstep\n\t\tSet full-step drive mode of stepper motor\n\n"
                "\twave\n\t\tSet wave drive mode of stepper motor\n\n"
                "\tmove/movenb [±steps]\n\t\tMake [±steps] steps in blocking/non-blocking mode and stop. Sign defines the direction\n\n"
                "\tstop\n\t\tStop the motor in the current position\n\n"
                "\tinfo\n\t\tGet an information about a content of timer's registers (compare value and prescaler), "
                "current mode (full-step or wave) and movement direction\n\n"
                "\tsave\n\t\tSave current configuration in EEPROM\n"
            );
        }

        else if ( strncmp("save", usart_buffer, 4) == 0 ) {
            eeprom_update_byte(&OCR2A_EEPROM_value, OCR2A);
            eeprom_update_byte(&full_step_flag_EEPROM, full_step_flag);
            eeprom_update_byte(&motor_high_level_delay_EEPROM, motor_high_level_delay);

            USART_putstring("Saved in EEPROM\n");
        }

        else if ( strncmp("info", usart_buffer, 4) == 0 ) {
            sprintf(usart_buffer, "OCR2A: %u\n", OCR2A);
            USART_putstring(usart_buffer);

            sprintf(usart_buffer, "TCCR2B: %u\n", TCCR2B);
            USART_putstring(usart_buffer);

            sprintf(usart_buffer, "Fullstep mode: %u\n", full_step_flag);
            USART_putstring(usart_buffer);

            if (motor_up_flag == 1)
                USART_putstring("Move up\n");
            else if (motor_up_flag == 0)
                USART_putstring("Move down\n");
            else
                USART_putstring("Motor stopped\n");
        }

        else {
            USART_putstring("Unknown command\n");
        }

        USART_putstring("\n> ");

    }

}
