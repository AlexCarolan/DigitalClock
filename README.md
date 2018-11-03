# DigitalClock
A 24-hour digital clock designed to run on a PIC18 40 pin Microcontroller, made with C and inline assembly.  
Allows the user to chnage and set the time using PORTB buttons.  

## User Guide

For demonstrative purposes the clock runs at 60 times normal speed i.e. 1 minute a second.  
The clock has two modes, 0 - normal operation and 1 - setting the time.

#### Buttons

RB0 - Changes the mode of operation  
RB1 - When mode 0 is active (No action)    
      When mode 1 is active increases selected value by 1     
RB2 - When mode 0 is active run at 300 X normal speed i.e. 5 minutes a second   
      When mode 1 is active move unit selector to next value


