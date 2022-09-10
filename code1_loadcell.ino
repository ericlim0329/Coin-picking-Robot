#include "HX711.h"
#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 7, 4, 3, 2);

const int LOADCELL_DOUT_PIN = 6;
const int LOADCELL_SCK_PIN = 5;
HX711 scale;
float calibration_factor = 40; // this calibration factor must be adjusted according to your load cell
float units;
int i = 0;
int boolean1 = 1;
int boolean2 = 1;
float prev = 0.0;
float neww = 0.0;
float array_units[10] = {0,0,0,0,0,0,0,0,0,-1.0};

float count = 0;

//array_units[9] = -1.0;
void setup() {
Serial.begin(9600);
scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
Serial.println("HX711 calibration sketch");
Serial.println("Remove all weight from scale");
Serial.println("After readings begin, place known weight on scale");
Serial.println("Press + or a to increase calibration factor");
Serial.println("Press - or z to decrease calibration factor");
scale.set_scale(calibration_factor); //Adjust to this calibration factor
scale.tare(); //Reset the scale to 0
long zero_factor = scale.read_average(); //Get a baseline reading
Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
Serial.println(zero_factor);

lcd.begin(16,1);
lcd.begin(16,2);

}
void loop() {
scale.set_scale(calibration_factor);
Serial.print("Reading ");
units = scale.get_units(), 5;



if (units < 0)
{
units = 0.00;
}

//Serial.print(" boolean1: ");
//Serial.print(boolean1);
//Serial.print(" grams");
//Serial.print(" calibration_factor: ");
//Serial.print(calibration_factor);

  if(boolean1 == 1){
    array_units[i] = units;

    if(array_units[9] != -1.0){
      prev = neww;
      neww = array_units[9];
      i = 0;
      boolean1 = 0;
      boolean2 = 0;
      array_units[9] = -1.0;
    }
    if(boolean2 == 1){
      i++;
    }
  }

  if(units > (neww + 100)){
    boolean1 = 1;
    boolean2 = 1;
  }

 //lcd.begin(16,1);
 //lcd.print(units);

  if((neww - prev - 200) < 660 && (neww - prev + 200) > 660){ //uncertainty increased
    lcd.setCursor(0,0);
    lcd.print("10 cents coin");
    //lcd.begin(16,2);
    //count = count + 0.10;
    //lcd.print(count);
  }
  else if((neww - prev - 100) < 1440 && (neww - prev + 100) > 1440){
      lcd.setCursor(0,0);
      lcd.print("5 cents coin");
    //lcd.setCursor(0,1);
    //count = count + 0.05;
    //lcd.print(count);
  }
  else if((neww - prev - 100) < 1680 && (neww - prev + 100) > 1680){
    lcd.setCursor(0,0);
    lcd.print("25 cents coin");
    //lcd.setCursor(0,1);
    //count = count + 0.25;
    //lcd.print(count);
  }
  else if((neww - prev - 100) < 2440 && (neww - prev + 100) > 2440){
      lcd.setCursor(0,0);
      lcd.print("1 dollar coin");
    //lcd.setCursor(0,1);
    //count = count + 1.0;
    //lcd.print(count);
  }
  else if((neww - prev - 200) < 2680 && (neww - prev + 200) > 2680){ //uncertainty increased
      lcd.setCursor(0,0);
      lcd.print("2 dollar coin");
    //lcd.setCursor(0,1);
    //count = count + 2.0;
    //lcd.print(count);
  }
  else{
      lcd.setCursor(0,0);
      lcd.print("             ");
    //lcd.setCursor(0,1);
    //lcd.print(count);
  }

Serial.print("units: ");
Serial.print(units);
Serial.print(" neww: ");
Serial.print(neww);
Serial.print(" prev: ");
Serial.print(prev);
Serial.print(" i: ");
Serial.print(i);
Serial.print(" array[i]: ");
Serial.print(array_units[i]);

Serial.println();
if (Serial.available())
{
char temp = Serial.read();
if (temp == '+' || temp == 'a')
calibration_factor += 1;
else if (temp == '-' || temp == 'z')
calibration_factor -= 1;
}
if (Serial.available())
{
char temp = Serial.read();
if (temp == 't' || temp == 'T')
scale.tare(); //Reset the scale to zero
}

}
