#include <Arduino.h>

// ##### pins ####

// input
const uint8_t IN_A = 2;
const uint8_t IN_B = 3;
const uint8_t IN_C = 4;
const uint8_t IN_D = 5;
const uint8_t IN_CLOSED = 6;

// output


// #### declarations ####

// functions
void display_number(uint8_t nb);
void clear_display();
uint8_t get_nb_choice();

// variables
uint8_t nb_choice = 0;

// #### main code ####

// main 
void setup() {

  pinMode(IN_A,INPUT);
  pinMode(IN_B,INPUT);
  pinMode(IN_C,INPUT);
  pinMode(IN_D,INPUT);
  pinMode(IN_CLOSED,INPUT);
  
  nb_choice = get_nb_choice();
  
  display_number(nb_choice);
}

void loop() {
  
  uint8_t nb_choice = get_nb_choice();
  
  if (digitalRead(IN_CLOSED))
    display_number(nb_choice);
  
  else
    clear_display();

}

// functions
void display_number(uint8_t nb)
{

}

void clear_display()
{
  
}

uint8_t get_nb_choice()
{

  bool a = digitalRead(IN_A);
  bool b = digitalRead(IN_B);
  bool c = digitalRead(IN_C);
  bool d = digitalRead(IN_D);

  if (!a && !b && !c && !d) 
    return 0;

  else if (!a && !b && !c && d) 
    return 1;

  else if (!a && !b && c && !d)
    return 2;

  else if (!a && !b && c && d)
    return 3;

  else if (!a && b && !c && !d)
    return 4;

  else if (!a && b && !c && d)
    return 5;

  else if (!a &&  b && c && !d)
    return 6;

  else  if (!a && b && c && d)
    return 7;

  else  if (a && !b && !c && !d)
    return 8;

  else  if (a && !b && !c && d)
    return 9;

  return 0;
}
