#include <Arduino.h>

// ##### pins ####
// input
const uint8_t IN_A = 2;
const uint8_t IN_B = 3;
const uint8_t IN_C = 4;
const uint8_t IN_D = 5;
// output

// #### declarations ####
// functions
void display_number(short nb);
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
  nb_choice = get_nb_choice();
  display_number(nb_choice);
}

void loop() {
  u_int8_t new_nb_choice = get_nb_choice();
  if (new_nb_choice != nb_choice)
  {
    nb_choice = new_nb_choice;
    display_number(nb_choice);
  }
}

// functions
void display_number(short nb)
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
  else   if (a && !b && !c && d)
    return 9;
}
