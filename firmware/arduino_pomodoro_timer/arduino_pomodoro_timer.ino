#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define LEFTBUTTON 4
#define MIDBUTTON 3
#define RIGHTBUTTON 2
#define BUZZER 5
#define RED 7
#define YELLOW 11
#define LDR A0

LiquidCrystal_I2C lcd(0x27, 16, 2);

//variables for the pomodoro
int pomoMinutes = 0;
int pomoSeconds = 0;
bool setPomo = false;
bool isPomoDone = false;
bool  settingSeconds = true; //true pag seconds, false pag minutes ang inugset
int counter = 0;
int startFlag = 0;
  
void setup() {
  Serial.begin(115200);
  pinMode(RED, OUTPUT); 
  pinMode(LDR, INPUT);
  pinMode(LEFTBUTTON, INPUT_PULLUP);
  pinMode(MIDBUTTON, INPUT_PULLUP);
  pinMode(RIGHTBUTTON, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  pinMode(RED, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(LDR, INPUT);

  lcd.begin(16, 2);
  lcd.backlight();

  //welcome message;
  introduction();

}

void loop() {
  int ldrStatus = analogRead(LDR);
  static int currentMenu = 0;
  static unsigned long elapsedTime = 0;
  
  // Serial.print("LDR Value: ");
  // Serial.println(ldrStatus);
  // Serial.print("Start Flag: ");
  // Serial.println(startFlag);

  //for verification [LDR]
  
  if(isPhoneInDock() == 0) {
    if(pomoMinutes != 0 || pomoSeconds != 0) {
      counter++;
      delay(1000);
        Serial.print("Counter: ");
        Serial.println(counter);
    }
    lcd.setCursor(0,0);
    lcd.print(" PLACE PHONE ON");
    lcd.setCursor(0, 1);
    lcd.print("THE DOCK 2 START");
    delay(500);
    digitalWrite(YELLOW, HIGH);
    digitalWrite(RED, LOW);
    return; // Prevent further execution if the phone is not in the dock
  }

  //setting the pomodoro timer
  if(currentMenu == 0) {
    lcd.setCursor(0, 0);
    lcd.print("SET POMODORO:");
      if(settingSeconds) {
        lcd.setCursor(0,1);
        lcd.print(">> Seconds: ");
        if(pomoSeconds < 10)
          lcd.print(" ");
        lcd.print(pomoSeconds);
        lcd.print("  ");
      } else {
        lcd.setCursor(0,1);
        lcd.print(">> Minutes: ");
        if(pomoMinutes < 10)
          lcd.print(" ");
        lcd.print(pomoMinutes);
          lcd.print("   ");
      }

      if(buttonPressed(LEFTBUTTON)) {
        if(settingSeconds) {
          pomoSeconds++;
          if(pomoSeconds > 59)
            pomoSeconds = 0;
        } else {
          pomoMinutes++;
          if(pomoMinutes > 59)
            pomoMinutes = 0;
        }
      }
      //change para instead na seconds ang iset kay minutes na
      if(buttonPressed(RIGHTBUTTON)) {
        settingSeconds = !settingSeconds;
      }
      //confirm the timer fo the pomodoro
      if(buttonPressed(MIDBUTTON)) {
        setPomo = true;
        currentMenu = 1;
      }
  } // end of the first if condi ----------------------------------------

  //for countdown logic
  if(currentMenu == 1) {
    if(setPomo == 1) {
      unsigned long currentTime = millis();

      if(currentTime - elapsedTime >= 1000) {
        elapsedTime = currentTime;

        if(pomoSeconds == 0) {
          if(pomoMinutes == 0) { //if wa nay secs and mins
            isPomoDone = true;
            setPomo = false;
          } else { //if napay minutes bilin
            pomoMinutes--;
            pomoSeconds = 59;
          }
        } else { //if naa pa unod ang secs
          pomoSeconds--;
        }

        sendPomodoroData("RUNNING");
      }

      lcd.setCursor(0, 0);
      lcd.print("<< TIME LEFT: >>");
      lcd.setCursor(0, 1);
      lcd.print(" ");
      if(pomoMinutes < 10)
          lcd.print(" ");
      lcd.print(pomoMinutes);
      lcd.print("min : ");
      if(pomoSeconds < 10)
          lcd.print(" ");
      lcd.print(pomoSeconds);
      lcd.print("sec  ");
    }
  }//end for if currentMenu is 1

  //to trigger alarm
  if(isPomoDone) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("   TIME'S UP!   ");
    lcd.setCursor(0, 1);
    lcd.print(" POMODORO DONE  ");
    sendPomodoroData("COMPLETED.");

    for (int i = 0; i < 10; i++) {
      lcd.backlight();
      tone(BUZZER, HIGH); 
      delay(20); 
         noTone(BUZZER); 
         delay(100);
      tone(BUZZER, HIGH); 
      delay(20); 
         noTone(BUZZER); 
         delay(100); 
      tone(BUZZER, HIGH); 
      delay(20);
         noTone(BUZZER);
         delay(100); 
      tone(BUZZER, HIGH); 
      delay(300); 
      lcd.noBacklight();
         noTone(BUZZER);
         delay(300); 
    }

    lcd.backlight();
    isPomoDone = false;
    currentMenu = 0;
    counter = 0;
    lcd.clear();
  }
}
//other functions:------------------------------------------------------
bool isPhoneInDock() {
  int ldrStatus = analogRead(LDR);
  if (ldrStatus <= 30) {
    digitalWrite(YELLOW, HIGH);               
    digitalWrite(RED, LOW);
    // Serial.println("LDR is DARK, LED is ON");
    return true;
   }
  else {
    digitalWrite(YELLOW, LOW);         
    digitalWrite(RED, HIGH);
    Serial.println("---------------");
    return false;
  }
}

void introduction() {
  lcd.setCursor(3, 0);
  lcd.print("WELCOME TO");
  lcd.setCursor(1, 1);
  lcd.print("POMODORO TIMER");
  delay(6000);
  lcd.clear();
}


bool buttonPressed(int pin) {
  if (digitalRead(pin) == LOW) {
    delay(200);
    return true;
  }
  return false;
}

void sendPomodoroData(String state) {
  // Construct dynamic JSON payload
 String payload = "{\"minutes\":" + String(pomoMinutes) + 
                 ",\"seconds\":" + String(pomoSeconds) + 
                 ",\"counter\":" + String(counter) + "}";

  
  // Send JSON to ESP32 via Serial --- PAPRINT PADUNG ESP32
  Serial.println(payload);
}