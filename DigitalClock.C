#define CurrentMode   0x00

#define Wait_LV1      0x01
#define Wait_LV2      0x02
#define Wait_LV3      0x03
#define Wait_LV4      0x0E

#define HoursUpper    0x04
#define HoursLower    0x05
#define MinUpper      0x06
#define MinLower      0x07

#define HoursUpperEncoded     0x08
#define HoursLowerEncoded     0x09
#define MinUpperEncoded       0x0A
#define MinLowerEncoded       0x0B

#define EncodingValue         0x0C
#define ChangePos             0x0D

void interrupt()
{
     //Check if RB0 has been pressed
     if (INTCON.INT0IF == 1){
        asm{
        ;Flip first bit of CurrentMode register
        BTG           CurrentMode, 0
        }
     }

     //Check if RB1 has been pressed
     if (INTCON3.INT1IF == 1){
        asm{
        ;Check that mode is 1
        MOVLW           0x01
        XORWF           CurrentMode, 0
        BNZ             END

        ;Check which value to increment
        CHECK_HIGH_HOUR:
        MOVLW           0x00
        XORWF           ChangePos, 0
        BNZ             CHECK_LOW_HOUR
        INCF            HoursUpper

        CHECK_LOW_HOUR:
        MOVLW           0x01
        XORWF           ChangePos, 0
        BNZ             CHECK_HIGH_MIN
        INCF            HoursLower

        CHECK_HIGH_MIN:
        MOVLW           0x02
        XORWF           ChangePos, 0
        BNZ             CHECK_LOW_MIN
        INCF            MinUpper

        CHECK_LOW_MIN:
        MOVLW           0x03
        XORWF           ChangePos, 0
        BNZ             END
        INCF            MinLower
        
        END:
        ;Ensure that any values that exceed their range wrap back to 0
        MOVLW           0x03
        CPFSLT          HoursUpper
        CLRF            HoursUpper

        MOVLW           0x02         ;Check if upper hour is 2
        CPFSLT          HoursUpper
        GOTO            CLEAR_IF_TWO
        
        MOVLW           0x0A
        CPFSLT          HoursLower
        CLRF            HoursLower
        GOTO            CLEAR_CHECK_NEXT
        
        CLEAR_IF_TWO:
        MOVLW           0x04
        CPFSLT          HoursLower
        CLRF            HoursLower
        
        CLEAR_CHECK_NEXT:
        MOVLW           0x06
        CPFSLT          MinUpper
        CLRF            MinUpper
        
        MOVLW           0x0A
        CPFSLT          MinLower
        CLRF            MinLower
        }
     }

     //Check if RB2 has been pressed
     if (INTCON3.INT2IF == 1){
        asm{
            ;Go to next position for increase
            INCF            ChangePos

            MOVLW           0x04         ;Check postion has not exceeded 4
            CPFSLT          ChangePos
            CLRF            ChangePos
        }
     }

     asm{
     ;Run a short delay to counter button debounce
     MOVLW            0x09
     MOVWF            Wait_LV4
     CALL             WAITLOOP

     ;Reset interrupt registers and retun to main program
     CLRF             INTCON
     CLRF             INTCON2
     CLRF             INTCON3
     MOVLW            0x90
     MOVWF            INTCON
     MOVLW            0xF0
     MOVWF            INTCON2
     MOVLW            0x18
     MOVWF            INTCON3

     RETFIE

     ;Delay code - run cycles to consume time
     DELAY:
            DECFSZ         Wait_LV3
            GOTO           DELAY
     RETURN


     WAITLOOP:
           CALL            DELAY
           DECFSZ          Wait_LV4
           GOTO            WAITLOOP
     RETURN


   }
}


void main() {
     asm {

     ;==================================================
     ;               INITIALIZING PORTS
     ;==================================================

     ;Set TRISTATE for  PORT A & D
     CLRF             TRISD
     CLRF             TRISA
     CLRF             TRISC
     CLRF             PORTC
     CLRF             PORTB

     CLRF             ChangePos
     CLRF             CurrentMode

     ;Set PORTB as input
     MOVLW            0x07           ; select bits 0-2
     MOVWF            TRISB          ; configure port B as input
     MOVF             BSR, W         ; store BSR into W
     MOVLB            15             ; switch to bank 15
     CLRF             ANSELB         ; change port B to digital input
     MOVWF            BSR            ; restore BSR

     ;Configure PORTB interrupt registers
     MOVLW            0x90
     MOVWF            INTCON
     MOVLW            0xF0
     MOVWF            INTCON2
     MOVLW            0x18
     MOVWF            INTCON3

     ;Set time to start at 12:00
     MOVLW            0x01
     MOVWF            HoursUpper
     MOVLW            0x02
     MOVWF            HoursLower
     CLRF             MinUpper
     CLRF             MinLower

     ;==================================================
     ;               START OF MAIN PROGRAM
     ;==================================================

     MAIN:
          ;Convert and store values to seven seg format
          MOVFF           HoursUpper, EncodingValue      ;Load stored time into encoding variable
          CALL            ENCODE                         ;Encode value to seven seg format
          MOVWF           HoursUpperEncoded              ;Move encoded value to PORTD

          MOVFF           HoursLower, EncodingValue
          CALL            ENCODE
          IORLW           0x80                           ;Add decimal point to display
          MOVWF           HoursLowerEncoded

          MOVFF           MinUpper, EncodingValue
          CALL            ENCODE
          MOVWF           MinUpperEncoded

          MOVFF           MinLower, EncodingValue
          CALL            ENCODE
          MOVWF           MinLowerEncoded

          ;Check current mode
          MOVLW           0x01
          XORWF           CurrentMode, 0          ;Check state of CurrentMode
          BZ              MODE_ONE                ;Run mode 1 code if mode = 1

          ;==================================================
          ;           MODE 0 - Normal Operation
          ;==================================================

          ;Display time and wait for a second
          CALL            WAITLOOP

          MOVLW           0x01         ;Check for mode change
          CPFSLT          CurrentMode
          GOTO            MODE_ONE

          CALL            WAITLOOP

          MOVLW           0x01         ;Check for mode change
          CPFSLT          CurrentMode
          GOTO            MODE_ONE

          MOVLW           0x80
          MOVWF           Wait_LV2
          CALL            WAITLOOP

          ;Check RB2 and increment the time
          MOVLW           0x04
          XORWF           PORTB, 0          ;Check if RB2 pressed
          BNZ             INC_ONE           ;If not pressed inc by 1
          GOTO            INC_FIVE          ;If pressed inc by 5

          ;Provide the correct incrementaion
          INC_ONE:
          INCF            MinLower
          GOTO            INC_END

          INC_FIVE:
          MOVLW          0x05
          ADDWF          MinLower, 1

          INC_END:

          ;Check any overflow and carry bits
          MOVLW           0x0A              ;Check lower min for excess 10
          CPFSLT          MinLower
          CALL            CARRYLOWERMIN

          MOVLW           0x06              ;Check upper min for excess 6
          CPFSLT          MinUpper
          CALL            CARRYUPPERMIN

          MOVLW           0x0A              ;Check lower hour for excess 10
          CPFSLT          HoursLower
          CALL            CARRYLOWERHOUR

          MOVLW           0x02              ;Check upper hour for excess 2
          CPFSLT          HoursUpper
          CALL            CHECKLOWERHOUR    ;If above 2 check that lower hours is below 4

          CLRF            ChangePos         ;Reset start positon

          GOTO            MAIN

          ;==================================================
          ;           MODE 1 - Setting the Time
          ;==================================================
          MODE_ONE:

          CALL             WAITLOOP     ;Display full time

          ;Clear value selected before display
          CHECK_HIGH_HOUR:
          MOVLW           0x00
          XORWF           ChangePos, 0
          BNZ             CHECK_LOW_HOUR
          CLRF            HoursUpperEncoded

          CHECK_LOW_HOUR:
          MOVLW           0x01
          XORWF           ChangePos, 0
          BNZ             CHECK_HIGH_MIN
          CLRF            HoursLowerEncoded

          CHECK_HIGH_MIN:
          MOVLW           0x02
          XORWF           ChangePos, 0
          BNZ             CHECK_LOW_MIN
          CLRF            MinUpperEncoded

          CHECK_LOW_MIN:
          MOVLW           0x03
          XORWF           ChangePos, 0
          BNZ             CHECK_END
          CLRF            MinLowerEncoded

          CHECK_END:
          CALL             WAITLOOP     ;Display time minus selected value for flash effect

          GOTO             MAIN

     ;==================================================
     ;               FUNCTION CALLS
     ;==================================================

     ;Carry over lower min value
     CARRYLOWERMIN:
          CLRF             MinLower
          INCF             MinUpper
     RETURN

     ;Carry over upper min value
     CARRYUPPERMIN:
          CLRF             MinUpper
          INCF             HoursLower
     RETURN

     ;Carry over lower hour value
     CARRYLOWERHOUR:
          CLRF             HoursLower
          INCF             HoursUpper
     RETURN

     ;Verify that lower hour is not 4 or more when upper is 2
     CHECKLOWERHOUR:
          MOVLW           0x04              ;Check lower hour for excess 4
          CPFSLT          HoursLower
          CALL            RESETCLOCK
     RETURN

     ;Returns all time values to 0 called when midnight occurs
     RESETCLOCK:
          CLRF             MinLower
          CLRF             MinUpper
          CLRF             HoursLower
          CLRF             HoursUpper
     RETURN

     ;Provide and return encoded values for seven seg display
     ZERO:
          MOVLW    0x3F
     RETURN

     ONE:
          MOVLW    0x06
     RETURN

     TWO:
          MOVLW    0x5B
     RETURN

     THREE:
          MOVLW    0x4F
     RETURN

     FOUR:
          MOVLW    0x66
     RETURN

     FIVE:
          MOVLW    0x6D
     RETURN

     SIX:
           MOVLW   0x7D
     RETURN

     SEVEN:
           MOVLW   0x07
     RETURN

     EIGHT:
           MOVLW   0x7F
     RETURN

     NINE:
           MOVLW   0x6F
     RETURN

     ;Converts binary number to seven-seg display format
     ENCODE:
     MOVLW    0x00
     XORWF    EncodingValue, 0
     BZ       ZERO

     MOVLW    0x01
     XORWF    EncodingValue, 0
     BZ       ONE

     MOVLW    0x02
     XORWF    EncodingValue, 0
     BZ       TWO

     MOVLW    0x03
     XORWF    EncodingValue, 0
     BZ       THREE

     MOVLW    0x04
     XORWF    EncodingValue, 0
     BZ       FOUR

     MOVLW    0x05
     XORWF    EncodingValue, 0
     BZ       FIVE

     MOVLW    0x06
     XORWF    EncodingValue, 0
     BZ       SIX

     MOVLW    0x07
     XORWF    EncodingValue, 0
     BZ       SEVEN

     MOVLW    0x08
     XORWF    EncodingValue, 0
     BZ       EIGHT

     MOVLW    0x09
     XORWF    EncodingValue, 0
     BZ       NINE

     MOVLW    0xFF  ;If no match is found then return 0xFF insted
     RETURN

     ;Run idle for a full 0xFF loop
     DELAY:
            DECFSZ         Wait_LV1
            GOTO           DELAY
     RETURN

     ;Loop through delay code to create minor delay
     WAITLOOP:
           CALL            LOADTIME
           DECFSZ          Wait_LV2
           GOTO            WAITLOOP
     RETURN


     ;Load and display current time on seven seg display
     LOADTIME:

           ;Show upper hours
           MOVLW           0x08                        ;Select first seven seg display
           MOVWF           PORTA                       ;Load selection to PORTA
           MOVFF           HoursUpperEncoded, PORTD    ;Move encoded time to PORTD
           CALL            DELAYCLEAR                  ;Create delay and reset port

           ;Show lower hours
           MOVLW           0x04
           MOVWF           PORTA
           MOVFF           HoursLowerEncoded, PORTD
           CALL            DELAYCLEAR


           ;Show upper minutes
           MOVLW           0x02
           MOVWF           PORTA
           MOVFF           MinUpperEncoded, PORTD
           CALL            DELAYCLEAR


           ;Show lower minutes
           MOVLW           0x01
           MOVWF           PORTA
           MOVFF           MinLowerEncoded, PORTD
           CALL            DELAYCLEAR

     RETURN

     ;Call delay for multipexing and reset PORTA and PORTD
     DELAYCLEAR:
           CALL            DELAY
           CLRF            PORTA
           CLRF            PORTD
     RETURN



     }


}