/*
 Name:		Simon.ino
 Created:	4/27/2021 7:11:15 PM
 Author:	Scott Marley

 Info:		Code used to demonstrate some of the capabilities of the Grove Beginner Kit from Seeed Studios.
			The game is set up in rounds, which consist of actions. An action could be a button press, covering
			the light sensor, tilting the board etc. The first round has one action, the second round has two,
			up to a max of maxRounds. Reaching maxRounds allows you to win the game. This could easily be expanded
			upon to have levels of difficulty, a high score stored in the EEPROM or additional sensors.
*/

#include <U8g2lib.h>
#include <LIS3DHTR.h>
#include <Wire.h>

#define btnPin		6
#define buzzerPin	5
#define lightPin	A6
#define soundPin	A2
#define potPin		A0
#define maxRounds	5

#include "media.h"

U8G2_SSD1306_128X64_NONAME_1_HW_I2C OLED(U8G2_R2, U8X8_PIN_NONE);
LIS3DHTR<TwoWire> ACCEL;

enum gameStates { START, DISPLAY_ACTIONS, LISTEN_ACTIONS, WON, LOST };
const char* actionLabels[] = { "Button", "Light", "Sound", "Twist", "Tilt left", "Tilt right", "Tilt forward", "Tilt back" };
int soundAverage = 0;
int lightThreshold = 0;

uint8_t actionList[maxRounds];
uint8_t roundNum = 0;
uint8_t actionCounter = 0;
uint8_t gameState = START;

void setup() {
	Serial.begin(9600);
	
	pinMode(btnPin, INPUT);
	pinMode(lightPin, INPUT);
	pinMode(buzzerPin, OUTPUT);
	pinMode(soundPin, INPUT);
	pinMode(potPin, INPUT);

	// Initialise OLED
	OLED.begin();
	OLED.setFont(u8g2_font_t0_16b_mr);

	// Initialise accelerometer
	ACCEL.begin(Wire, 0x19);
	delay(100);
	ACCEL.setOutputDataRate(LIS3DHTR_DATARATE_50HZ);

	// Calibrate the light sensor
	lightThreshold = analogRead(lightPin);
}


void loop() {
	switch (gameState) {

	case START:

		// Populate actions list with random numbers
		randomSeed(analogRead(A1));
		for (int i = 0; i < maxRounds; i++) {
			actionList[i] = random(0, 8);
		}

		// Draw start screen
		OLED.firstPage();
		do {
			OLED.setFont(u8g2_font_t0_16b_mr);
			OLED.setCursor(20, 20);
			OLED.print(F("SensorSimon"));
			OLED.setFont(u8g2_font_t0_14_mr);
			OLED.setCursor(22, 40);
			OLED.print(F("Press Start!"));
		} while (OLED.nextPage());

		// Wait for start to be pressed
		while(digitalRead(btnPin) == LOW) { /* wait */}

		OLED.clear();

		gameState = DISPLAY_ACTIONS;
		break;

	case DISPLAY_ACTIONS:
		
		// Display actions on screen
		for (int i = 0; i <= roundNum; i++) {
			playSound(0);
			OLED.firstPage();
			do {
				OLED.setFont(u8g2_font_t0_16b_mr);
				OLED.drawXBMP(49, 8, 30, 30, imageList[actionList[i]]);
				int cursorX = (128 - OLED.getStrWidth(actionLabels[actionList[i]])) / 2;
				OLED.setCursor(cursorX, 60);
				OLED.print(actionLabels[actionList[i]]);
			} while (OLED.nextPage());
			delay(700);
		}
		OLED.clear();

		gameState = LISTEN_ACTIONS;
;		break;

	case LISTEN_ACTIONS:

		// Create fake rolling average of sound levels
		soundAverage = ((soundAverage * 15) + analogRead(soundPin)) / 16;
		
		// Listen for actions
		if (digitalRead(btnPin) == HIGH) {
			Serial.println("Button");
			checkAction(0);
		}
		
		if (analogRead(lightPin) < lightThreshold - 50) {
			Serial.println("Light");
			while (analogRead(lightPin) < lightThreshold - 50) {};
			checkAction(1);
		}
		if (soundAverage > 600) {
			Serial.println("Sound");
			while (soundAverage > 400) {
				soundAverage = ((soundAverage * 15) + analogRead(soundPin)) / 16;
			};
			checkAction(2);
		}
		if (analogRead(potPin) < 1020) {
			Serial.println("Twist");
			while (analogRead(potPin) < 1020) {};
			checkAction(3);
		}
		if (ACCEL.getAccelerationY() > 0.4) {
			Serial.println("Tilt left");
			while (ACCEL.getAccelerationY() > 0.1) {};
			checkAction(4);
		}
		if (ACCEL.getAccelerationY() < -0.4) {
			Serial.println("Tilt right");
			while (ACCEL.getAccelerationY() < -0.1) {};
			checkAction(5);
		}
		if (ACCEL.getAccelerationX() > 0.4) {
			Serial.println("Tilt forward");
			while (ACCEL.getAccelerationX() > 0.1) {};
			checkAction(6);
		}
		if (ACCEL.getAccelerationX() < -0.4) {
			Serial.println("Tilt back");
			while (ACCEL.getAccelerationX() < -0.1) {};
			checkAction(7);
		}
		break;

	case LOST:

		// Lost the game
		OLED.firstPage();
		do {
			OLED.setCursor(20, 20);
			OLED.print(F("You reached"));
			OLED.setCursor(35, 40);
			OLED.print(F("level "));
			OLED.print(roundNum);
		} while (OLED.nextPage());

		playSound(2);
		delay(1000);

		actionCounter = 0;
		roundNum = 0;
		gameState = START;
		break;
	
	case WON:

		// Won the game!
		do {
			OLED.setFont(u8g2_font_t0_16b_mr);
			OLED.setCursor(4, 20);
			OLED.print(F("Congratulations"));
			OLED.setFont(u8g2_font_t0_14_mr);
			OLED.setCursor(32, 40);
			OLED.print(F("You Win!"));
		} while (OLED.nextPage());

		playSound(3);
		delay(1000);

		actionCounter = 0;
		roundNum = 0;
		gameState = START;
		break;
	}
}

void checkAction(uint8_t action) {
	if (action == actionList[actionCounter]) {
		playSound(1);
		if (actionCounter == roundNum) {
			if (roundNum == maxRounds - 1) {	
				// Won the game
				gameState = WON;
			}
			else {
				// Move onto the next round
				roundNum++;
				actionCounter = 0;
				gameState = DISPLAY_ACTIONS;
			}
		}
		else {
			// Move onto the next action
			actionCounter++;
		}
	}
	else {
		// Lost the game
		gameState = LOST;
	}
	delay(200);
}