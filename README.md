# La Boîte à Ampoules

Welcome to **La Boîte à Ampoules**! This project is part of **DeepRed** by **Fondation INP** with the support of **Lynred**. This is a school project lead by Côme Campenet, Paul Fauvernier, Léo Schorlé, and Alexandre Veyret.

This project in large point of view is a test of an escape game using infrared technologies. The test is based on a box that contains a 3*5 light bulb matrix. When the box is open the lights are all off and when the box is closed the some of the light are turned on, drawing a code useful for the next test. The fact is that the light bulbs are heating when on so it's emmtiting infrared light even when they are turned off. This is the reason why we need to use an infrared camera to read the code when the box is open. The fact is that the code is changing between 2 clue codes, and once the players read it, they need to put the two codes together following the "formulas" given. When they have the solution code, they enter it in the buttons of the daughter box (digicode) and the box does something to handover to the next trial (display a code, open a chest, a door, ...). 


## Table of Contents

- [Features](#features)

- [Hardware](#hardware)

- [Software Requirements](#software-requirements)

- [Installation](#installation)

- [Usage](#usage)

## Features


- Setup the solution code
- Encode the clue codes
- Automatically communicate the codes between the boxes
- User-friendly web interface
- Playing the game with different modes
- Options to setup the code easier
- Monitor the buttons that players push

## Hardware


In the project , we used the following hardware:
- **ESP32** (Microcontroller)

### Mother Box (server, light bulb box)

- Electronics circuit (this schematic is dupilcated 15 times, one for each light bulb):

[circuit](./circuit.png)

### Daughter Box (client, digicode)

[circuit](./Schematic_daugther_box.pdf)

## Software Requirements


- [PlatformIO](https://platformio.org/) (for development)

- [Arduino IDE](https://www.arduino.cc/en/software) (optional, if you just take the [main](src/main.cpp) file)

- Libraries:

  - `WiFi.h`

  - `Arduino.h`

  - `ArduinoJson`

## Installation

1. **Clone the repository :**
```bash
git clone https://github.com/comecampenet/boite_ampoules.git

cd boite_ampoules
```
2. **Open the project in PlatformIO :**
- Lauch PlatformIO IDE

- Open the the cloned project folder

3. **Install dependencies :**
- PlatformIO wil automatically install the required librairies when you build the project

4. **Build and upload the code :**
- Connect your ESP32 board to your computer with a USB cable
- Use PlatformIO IDE to build and upload the codes to each ESP32 boards

## Usage
1. **Connect to the ESP32 Wifi :**
- Connect to the ESP32 access point

- Open your web browser and navigate to the IP address assigned to your ESP32, often `http://192.168.4.1`
2. **Control your bulbs :**
- Just click on the buttons corresponding to each bulbs to draw your code
3. **Explore the code :**
- Open the [main](src/main.cpp) file to see the code and feel free to modify it 

