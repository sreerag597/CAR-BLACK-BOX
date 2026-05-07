/*
 * File:   matrix_keypad.c
 * Author: sreerag
 *
 * Created on December 30, 2025, 10:23 AM
 */


#include <xc.h>
#include "matrix_keypad.h"

void init_matrix_keypad(void)
{
    RBPU = 0; //portb pull up
    
    //RB1-RB4 AS I/P 
    TRISB = TRISB | 0X1E;  // XXXX XXXX | 0001 1110 = XXX1 111X
    
    //RB5-RB7 AS O/P
    TRISB = TRISB & 0X1F; // XXX1 111X & 0001 1111 = 0001 111X
    
}
unsigned char read_matrix_keypad(void)
{
    
    RB5=0;
    RB6=1;
    RB7=1;
    
    if(RB1==0)
    {
        return 1; //switch1 pressed
    }
    else if(RB2==0)
    {
        return 4; //switch4 pressed
    }
    else if(RB3==0)
    {
        return 7; //switch7 pressed
    }
    else if(RB4==0)
    {
        return 10; //switch10 pressed
    }
    
    RB5=1;
    RB6=0;
    RB7=1;
    
    if(RB1==0)
    {
        return 2; //switch2 pressed
    }
    else if(RB2==0)
    {
        return 5; //switch5 pressed
    }
    else if(RB3==0)
    {
        return 8; //switch8 pressed
    }
    else if(RB4==0)
    {
        return 11; //switch11 pressed
    }
    
    RB5=1;
    RB6=1;
    RB7=0;
    
    if(RB1==0)
    {
        return 3; //switch3 pressed
    }
    else if(RB2==0)
    {
        return 6; //switch6 pressed
    }
    else if(RB3==0)
    {
        return 9; //switch9 pressed
    }
    else if(RB4==0)
    {
        return 12; //switch12 pressed
    } 
    return 0XFF; //NO KEY PRESSED
}
