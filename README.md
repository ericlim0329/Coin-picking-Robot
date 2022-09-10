# Coin picking Robot
 Robot that is autonomous or controlled to pick up coins.
This robot was made as a final project for ELEC291 (electrical engineering design studio) with 6 students total

Peter van den Doel

Oliver Kis

Eric Lim

Amrit Sahota

Brandon Seo

Raul Vazquez Guerrero

VIDEO DEMO LINK https://www.youtube.com/watch?v=IEJqTksT0a4

0:00 Picking up 20 coins

6:44 controlling with joystick controller

8:10 explaining how we classify different coins

8:50 demoing classification of coins

The robot has 2 modes based on different versions of the code

The first version will drive around and if it detects a perimeter(wire with 16khz square wave) it will back up and turn right for a randomized amount of time. If it encounters a coin, it will back up for 0.4 seconds, pick it up with a magnetic arm, and drop it in a bucket. The robot also contains a load cell which it uses to classify coins based on mass and an LCD screen to display the coin type.

The main firmware for the robot runs on a PIC32 microcontroller

The code for classifying coins with a load cell runs on an ATMEL ATMEGA microcontroller with firmware loaded onto it using an Arduino UNO.

SOURCE CODE

source code is contained in

Robot_Base.c,Robot_Base_with_joystick.c,code1_loadcell.ino,and Robot_Base.mk

Robot_Base.c is for automatically finding coins and picking them up

Robot_Base_with_joystick.c is for giving the user manual control of the robot's movement

Robot_Base.mk is a makefile to be used for compiling the robot's code and flash loading it onto the PIC32

code1_loadcell.ino is arduino code to be loaded onto an atmega to classify coins with a load cell and display on an LCD

usart.c and usart.h provide some functions for serial console I/O which served useful for debugging

HX711.cpp and HX711.h are for the arduino library used to interface with the load cell through the HX711 ADC/amplifier

DETECTION

Coins are detected using a metal detector made from a Colpitts oscillator made with a CMOS inverter with an inductance that varies in response to magnetic metals. The circuit's frequency response changes and the PIC32 checks that it passes a threshold.

The perimeter (wire with 16khz square wave) was created by passing an output signal from a 555timer astable oscillator through a very long wire

The perimeter is detected using a tank circuit with a resonant frequency of 16khz. The output is passed through a non inverting amplifier, then a diode to remove negative voltages, and finally fed into the PIC32's onboard ADC to check if it passes a certain threshold.

ELECTROMECHANICAL CONSIDERATIONS

The wheels are controlled using 2 H-Bridges hooked up to DC motors and implements stick and slip steering.

The robot's arm is moved by controlling 2 servo motors using PWM(pulse width modulation) controls. One servo pivots the base of the arm while another servo raises and lowers the arm

The arm picks up coins with an electromagnet on the end that is turned on and off by using an output signal to open the channel on an N-type mosfet allowing current to flow through the magnet.

POWER SUPPLY

the robot is powered using four AA batteries to create a 6V source and one 9V battery to create a 9V source.

The 6V batteries power the electromagnet, the H bridges, and the servo motors.

The 9V battery is stepped down to 5v and then 3.3v through voltage regulators

5V is used to power the Colpitts oscillator for the metal detector and the OP-amp for the perimeter detectors

3.3V is used to power the PIC32 and joysticks.

Optocouplers allow for interface between the 3.3V outputs of the PIC32 with the Servos, H bridges, and magnet control MOSFET that all need 6V.

MANUAL CONTROL

Manual control is achieved through a wired controller with joysticks to control the robot's movement and a switch to toggle the magnet on and off

Two joysticks, each with an X and Y direction output are used to give the user manual control of the robot. Four ADC inputs are needed, so the manual control configuration requires unplugging the perimeter detectors from ADC pins 4 and 5 of the PIC32. The magnet is also hooked up to a switch on the controller

The joysticks output an analog signal between 3.3V and 0v depending on the direction it is moved in, resting at about 1.6V with no movement. Threshold outputs for the ADC pins were used for the PIC32 to decide when to move the robot.

The two outputs of one joystick move the robot forwards/backwards for one and turn left/right for the other. The two outputs of another joystick pivot the base of the robot's servo arm left/right for one and move the arm up/down for the other.

The magnet is controlled by connecting it to 6V through a toggle switch on the controller.

CLASSIFYING COINS

The hardware to classify coins functions independent of the hardware for the rest of the robot except for a 5V power supply from the same voltage regulator.

The coins are classified using a load cell that notices when a coin is placed in the bucket and uses the difference in weight to classify the type of coin.

The load cell's values are amplified and converted to digital signals through an HX711 ADC/amplifier. The data from the HX711 is read using 24 bit SPI with an ATMEL ATMEGA microcontroller. The code to interface with the HX711 comes from an arduino library avaliable at https://github.com/bogde/HX711. The arduino code is loaded onto the ATMEGA using an arduino as a flash loader and then the ATMEGA is attached to a breadboard on the robot to save space.

The type of coin is displayed on an LCD screen connected to the ATMEGA.
