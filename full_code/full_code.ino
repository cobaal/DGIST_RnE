#include <IRremote.h>             // IR Remote Lib
#include <Wire.h>                 // I2C Communication lib
#include <Adafruit_MLX90614.h>    // Contractless Infrared Temperature Sensor Lib
#include <LiquidCrystal_I2C.h>    // LCD 1602 I2C Lib
#include <DHT.h>                  // DHT Sensor Lib

/* for LED Display */
LiquidCrystal_I2C lcd(0x27,16,2); // addr : 0x3F or 0x27

/* for CIT Sensor */
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

/* for IR Receiver */
int irReceiverPin = 12;
unsigned long last = millis();
IRrecv irrecv(irReceiverPin);
decode_results decodedSignal;

unsigned int rawCodes_ON[RAWBUF];
int codeLen_ON;
unsigned int rawCodes_OFF[RAWBUF];
int codeLen_OFF;
unsigned int rawCodes_SETTEMP[13][RAWBUF];
int codeLen_SETTEMP[13];

/* for IR Transmitter */
IRsend irsend;
int setPoint;

/* for DHT Sensor */
DHT dht(2, DHT22);

/* for controller */
int controllerPin = 8;
int lastA0 = analogRead(A0);
int lastA1 = analogRead(A1);
int lastD8 = digitalRead(8);
int setTemp = 18;

/* for Heart Sensor */
int PulseSensorPurplePin = 2;
int HeartSignal;
int Threshold = 425;

int buffer[1000] = {0};
int idx = 0;
int BPM = 0;

/* for MENU */
int IRcase = -1;

void setup() {
  Serial.begin(9600);

  /* for IR Receiver */
  irrecv.enableIRIn();

  /* for IR Sender */
  //pinMode(9, OUTPUT);
  //digitalWrite(9, LOW);       // aduino uno: pin3, aduino mega: pin9

  /* for Controller */
  pinMode(controllerPin, INPUT_PULLUP);

  /* for LED Display */
  lcd.init();
  lcd.backlight();

  /* for CIT Sensor */
  mlx.begin();

  /* for DHT Sensor */
  dht.begin();
}

void loop() {    
  /* LCD Print */
  if (IRcase == -1) {
    lcd.setCursor(0,0);
    lcd.print("NEED A SETTING..!");
    
  } else if (IRcase == 0) {
    lcd.setCursor(0,0);
    lcd.print("SET TEMP : ");
    lcd.setCursor(11,0);
    lcd.print(setTemp); 
    lcd.setCursor(13,0);
    lcd.print("'C  ");    
    
  } else if (IRcase == 1) {
    lcd.setCursor(0,0);
    lcd.print("WAIT: POWER ON   ");

  } else if (IRcase >= 2 && IRcase <= 14) {
    lcd.setCursor(0,0);
    lcd.print("WAIT: TEMP ");
    lcd.setCursor(11,0);
    lcd.print(IRcase + 16);
    lcd.setCursor(13,0);
    lcd.print("'C  "); 
        
  } else if (IRcase == 15) {
    lcd.setCursor(0,0);
    lcd.print("WAIT: POWER OFF  ");
  } 
  
  /* IR Receive */
  if (irrecv.decode(&decodedSignal) == true && (IRcase == -1 || IRcase > 15)) {
    // Nothing
    irrecv.resume();
    
  } else if (irrecv.decode(&decodedSignal) == true && IRcase == 1) {
    IRcase++;
    if (millis() - last > 250) {
      /* store */
      codeLen_ON = decodedSignal.rawlen - 1;
      for (int i = 1; i <= codeLen_ON; i++) {
        if (i % 2) {
          // Mark
          rawCodes_ON[i - 1] = decodedSignal.rawbuf[i]*USECPERTICK - MARK_EXCESS;
        } else {
          // Space
          rawCodes_ON[i - 1] = decodedSignal.rawbuf[i]*USECPERTICK + MARK_EXCESS;
        }        
      }
    }
    last = millis();
    irrecv.resume();
    
  } else if (irrecv.decode(&decodedSignal) == true && IRcase == 15) {
    IRcase = 0;
    if (millis() - last > 250) {
      /* store */
      codeLen_OFF = decodedSignal.rawlen - 1;
      for (int i = 1; i <= codeLen_ON; i++) {
        if (i % 2) {
          // Mark
          rawCodes_OFF[i - 1] = decodedSignal.rawbuf[i]*USECPERTICK - MARK_EXCESS;
        } else {
          // Space
          rawCodes_OFF[i - 1] = decodedSignal.rawbuf[i]*USECPERTICK + MARK_EXCESS;
        }        
      }
    }
    last = millis();
    irrecv.resume();

  } else if (irrecv.decode(&decodedSignal) == true && (IRcase >= 2 && IRcase <= 14)) {
    if (millis() - last > 250) {
      /* store */
      codeLen_SETTEMP[IRcase - 2] = decodedSignal.rawlen - 1;
      for (int i = 1; i <= codeLen_ON; i++) {
        if (i % 2) {
          // Mark
          rawCodes_SETTEMP[IRcase - 2][i - 1] = decodedSignal.rawbuf[i]*USECPERTICK - MARK_EXCESS;
        } else {
          // Space
          rawCodes_SETTEMP[IRcase - 2][i - 1] = decodedSignal.rawbuf[i]*USECPERTICK + MARK_EXCESS;
        }        
      }
    }
    IRcase++;
    last = millis();
    irrecv.resume();
  }

  /* Controller and IR Send*/
  int nowA0 = analogRead(A0);
  int nowA1 = analogRead(A1);
  int nowD8 = digitalRead(8);

  if (lastD8 == 1 && nowD8 == 0) {
    IRcase = 1;
      
  } else if (lastA0 < 600 && nowA0 > 1000) {
    setTemp = 18;
    irsend.sendRaw(rawCodes_ON, codeLen_ON, 38);
    delay(50); // Wait a bit between retransmissions
    irrecv.enableIRIn();

  } else if (lastA0 > 500 && nowA0 < 50) {
    irsend.sendRaw(rawCodes_OFF, codeLen_OFF, 38);
    delay(50); // Wait a bit between retransmissions
    irrecv.enableIRIn();
      
  } else if (lastA1 > 500 && nowA1 < 50) {
    setTemp = setTemp == 30 ? setTemp : setTemp + 1;
    irsend.sendRaw(rawCodes_SETTEMP[setTemp - 18], codeLen_SETTEMP[setTemp - 18], 38);
    delay(50); // Wait a bit between retransmissions
    irrecv.enableIRIn();
      
  } else if (lastA1 < 600 && nowA1 > 1000) {
    setTemp = setTemp == 18 ? setTemp : setTemp - 1;
    irsend.sendRaw(rawCodes_SETTEMP[setTemp - 18], codeLen_SETTEMP[setTemp - 18], 38);
    delay(50); // Wait a bit between retransmissions
    irrecv.enableIRIn();
  }
  
  lastA0 = nowA0;
  lastA1 = nowA1;
  lastD8 = nowD8;
    
  /* CIT Read */
  String strTemp = String("");
  strTemp += (double)(mlx.readObjectTempC()); // read Temperature
  lcd.setCursor(0,1);
  lcd.print("        ");
  lcd.setCursor(0,1);
  lcd.print(strTemp);

  /* DHT Read */
  int dht_h = dht.readHumidity();
  int dht_t = dht.readTemperature();

  lcd.setCursor(7,1);
  lcd.print(dht_h);
  lcd.setCursor(10,1);
  lcd.print(dht_t); 

  /* for HeartSensor */
  HeartSignal = analogRead(PulseSensorPurplePin);
  if (buffer[idx] == 0 && buffer[((idx+1)%1000)] == 1) BPM--;
  if (HeartSignal > Threshold) {
    buffer[idx] = 1;
  } else {
    buffer[idx] = 0;
  }
  if (buffer[((idx-1)%1000)] == 0 && buffer[idx] == 1) BPM++;
  idx = (idx + 1) % 1000;

  ////////////////////////////////////////////////////////////////////////////////////////////////
  // Serial.println(HeartSignal);
  ////////////////////////////////////////////////////////////////////////////////////////////////////

  lcd.setCursor(13,1);
  lcd.print(BPM); 

  /* Print Data */
  if (IRcase == 0) {
    Serial.print(strTemp);
    Serial.print(", ");
    Serial.print(dht_h);
    Serial.print(", ");
    Serial.print(dht_t);
    Serial.print(", ");
    Serial.print(BPM);
    Serial.print(", ");
    Serial.println(setTemp);
  }

  if (Serial.available()) {
    setPoint = Serial.read();
    if (setPoint >= 18 && setPoint <= 30) {
      setTemp = setPoint;
      irsend.sendRaw(rawCodes_SETTEMP[setTemp - 18], codeLen_SETTEMP[setTemp - 18], 38);
      delay(50); // Wait a bit between retransmissions
      irrecv.enableIRIn();
    }    
  }
  
  delay(10);
}
