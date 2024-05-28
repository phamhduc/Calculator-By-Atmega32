#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#define D4 eS_PORTC4
#define D5 eS_PORTC5
#define D6 eS_PORTC6
#define D7 eS_PORTC7
#define RS eS_PORTC0
#define EN eS_PORTC2

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include "lcd.h"

// declaration//
//------------------------------------------------------//

//Global variables
static double a = 0,b = 0;
static double result = 0;			//Output number
char operate = '\0'; 
static char c = 0; // current cursor
char Dot_Flag = 0;
char Op_Flag = 0;
char D_Flag = 0;
char A_Flag = 0;
float Fl_Sign = 0; // float sign for float number 
char Lcd_Result[20] = {0};
unsigned char keypad[4][4] = {{'1','2','3','/'},
					   		  {'4','5','6','x'},
					          {'7','8','9','-'},
					          {'.','0','=','+'}};

//-----------------------------------------------------//
// init function//
void ports_init(void);
void external_interrupt_init(void);
void lcd_display();
char find_key();
double Input_Num(double num, char key); 
void enter_number(char key);

void floatToString(float number, char* buffer, int precision) {
    int intPart = (int)number;
    float decPart = number - intPart;

    // Calculate the multiplier based on desired precision
    int multiplier = 1;
    for (int i = 0; i < precision; i++) {
        multiplier *= 10;
    }

    // Round the decimal part to the desired precision
    int roundedDecPart = (int)(decPart * multiplier + 0.5);

    // Convert integer part to string
    itoa(intPart, buffer, 10);

    // Find the length of the integer part
    int len = 0;
    while (buffer[len] != '\0') {
        len++;
    }

    // Add decimal point
    buffer[len] = '.';
    len++;

    // Convert rounded decimal part to string
    itoa(roundedDecPart, buffer + len, 10);
}
// function declare//
void init_port()
{
	DDRC = 0xFF; /// COPNFIG LCD
	DDRD = 0b11110000;
	DDRB = 0b00000000;
	PORTD = 0b00000000;
	PORTB = 0x00;
}
//-----------------------------//	

int main(void)
{
	// INIT//
	init_port();
	Lcd4_Init();
	// set up interupt
	MCUCR = 0x00;    // make INT0 low level triggered
  	GICR  = (1<<INT0);
	Lcd4_Set_Cursor(2, 1);
	Lcd4_Write_String("INVALID");
	sei(); 
	while(1)
	{
		char key = find_key();

        if (key != '\0') {
            enter_number(key);
		
			_delay_ms(2200);
        }
	}
}
char find_key() {
    for (int i = 0; i < 4; i++) {
        // Set the i-th bit of PORTD to 1 (columns)
        PORTD = (1 << (i + 4));
        // Check each row on PortB
        for (int j = 0; j < 4; j++) {
            if (bit_is_set(PINB, j)) {
                // Key pressed, return corresponding character
                return keypad[i][j];
            }
        }

        // Delay for a short period for debounce
        _delay_ms(20);
    }

    return '\0'; // No key pressed
}
//
void enter_number(char key)
{
	
	if((key == '.'||(key >= '0' && key <= '9')))
	{
		if(Op_Flag == 0) // enter num for A
		{
			a = Input_Num(a,key);
		}
		else
		{
			Op_Flag = 1; // enter num for B
			b = Input_Num(b,key);
		}
	}
	else if( key == '=')
	{
		if(Op_Flag  == 0)
		{
			Lcd4_Set_Cursor(2, 1);
			result = a;	

	    // Check if the result is an integer (no decimal part)
	    	if ((result - (int)result) == 0)
	    	{
	        	sprintf(Lcd_Result, "%d", (int)result);
	    	}
	    	else
	    	{
				floatToString(result,Lcd_Result,2);
	    	}

	    	Lcd4_Write_String(Lcd_Result);
			A_Flag = 1;
			b = 0;
		}
		else
		{
			Op_Flag = 0;
			switch(operate)
			{
				case '+':
					result = a + b;
					a = result;
					break;
				case '-':
					result = a - b;
					a = result;
					break;
				case 'x':
					result = a * b;
					a = result;
					break;
				case '/':
					if(b == 0)
					{
						Lcd4_Set_Cursor(2,1);
						Lcd4_Write_String("INVALID");
						break;			
					}
					result = a / b;
					a = result;
					break;
				default:
					break;
			}
			Lcd4_Set_Cursor(2, 1);

	    // Check if the result is an integer (no decimal part)
	    	if ((result - (int)result) == 0)
	    	{
	        	sprintf(Lcd_Result, "%d", (int)result);
	    	}
	    	else
	    	{
				floatToString(result,Lcd_Result,2);
	    	}

	    	Lcd4_Write_String(Lcd_Result);
			A_Flag = 1;
			b = 0;
		}
	}
	else
	{
		Fl_Sign = 0;
		if(A_Flag == 1)
		{
			Lcd4_Clear();
			_delay_ms(20);// delay for cd
			Lcd4_Set_Cursor(1,0);
			Lcd4_Write_String("ANS");
			c = 3;
			a = result;
			Op_Flag = 0;
			A_Flag = 0;

		}
		if(Op_Flag == 1)
		{
			Op_Flag = 1; // if operation was called keep previous status
		}
		else
		{
			Lcd4_Set_Cursor(1,c);
			Lcd4_Write_Char(key);
			c++;
			operate = key;
			Op_Flag = 1;
			Dot_Flag = 0;
		}
	}

}

double Input_Num(double num, char key)
{
    if (key == '.')
    {
        if (Dot_Flag == 0)
        {
			Lcd4_Set_Cursor(1, c);
            Lcd4_Write_Char(key);
            c++;
            Dot_Flag = 1;
        }
    }
    else
    {
        if (Dot_Flag == 1)
        {
            num = num + (key - '0') * 0.1;
            Lcd4_Set_Cursor(1, c);
            Lcd4_Write_Char(key);
            c++;
            Dot_Flag = 2;
			Fl_Sign = 10;
            return num;
        }
		else if (Dot_Flag == 2)
		{
			num = num + (key - '0') * (0.1 /Fl_Sign);
			Fl_Sign = Fl_Sign*10;
			Lcd4_Set_Cursor(1, c);
            Lcd4_Write_Char(key);
			c++;
			return num;
		}
        else
        {
            num = num * 10 + (key - '0');
            Lcd4_Set_Cursor(1, c);
            Lcd4_Write_Char(key);
            c++;
            return num;
        }
    }

    return num;
}

ISR (INT0_vect){ 		// ISR for external interrupt 0
	Lcd4_Clear();
	_delay_ms(20);
	result = 0;
	a = 0;
	c = 0;
	b = 0;
	Dot_Flag = 0;
	Op_Flag = 0;
	D_Flag = 0;
	A_Flag = 0;

}


