/* 
 * Copyright (c) 2023 Jan Novotný.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

#define LED                    13    // Relay pin

byte    PIN_LEN              = 4;    // Length of PIN code
                                     // Not defined as constant so it can be changed

const long WAIT_MS           = 5000; // time before locking again
enum METHODS{WAIT, CONFIRM};
enum METHODS METHOD          = WAIT; // handle re-locking after an unlock, either by a delay or by user confirm

const byte rows              = 4;
const byte cols              = 4;
const byte colPins[rows]     = {5, 4, 3, 2};
const byte rowPins[cols]     = {9, 8, 7, 6};
const byte lcd_addr          = 0x27;
const byte lcd_cols          = 16;
const byte lcd_rows          = 2;

const char keys[rows][cols] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

Keypad keypad = Keypad(
  makeKeymap(keys),
  rowPins,
  colPins,
  rows,
  cols
);

LiquidCrystal_I2C lcd = LiquidCrystal_I2C(lcd_addr, lcd_cols, lcd_rows);

void setup() {
  //Serial.begin(9600);
  lcd.begin(lcd_addr, lcd_cols, lcd_rows);  
  lcd.setBacklight(100);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
}

// Reset password string to "----", returns new string if 'str' isn't specified
char* strunset(char* str = NULL, byte strlen = PIN_LEN) {
  if (str == NULL)
    str = malloc(sizeof(char) * (strlen+1));
  for (int i = 0; i < strlen; i++) {
    str[i] = '-';
  }
  str[strlen] = '\0';
  return str;
}

// Center text in row
void lcd_center(char* str, int row) {
  lcd.setCursor(lcd_cols / 2 - strlen(str) / 2, row);
  lcd.print(str);
}


// Clear the entered pasword
void reset_pass(char *current, byte& index, bool wrong=false) {
  index = 0;
  current = strunset(current);
  lcd.clear();
  if (wrong) {
    lcd_center("Wrong password!", lcd_rows / 2 - 1);
    delay(2000);
  }
  lcd.clear();
}

void unlock() {
  digitalWrite(LED, HIGH);
  lcd.clear();
  lcd_center("unlocked", lcd_rows / 2);
}

void lock() {
  digitalWrite(LED, LOW);
}

int int_from_pinstr(char* str) {
  int r = 0;
  const int len = strlen(str);
  for (int i = 0; str[i] != '-' && i < len; i++) {
    r = r * 10 + (str[i] - 48);
  }
  if (strlen(str))
    return r;
  else 
    return 5;
}

void get_pin_from_keypad(char* str, char* pass, byte len=PIN_LEN) {
  char key;
  char* current = strunset(current, len);
  byte index = 0;
  lcd.clear();
  while (true) {
    key = keypad.getKey();
    lcd_center(str, lcd_rows / 2 - 1);
    lcd_center(current, lcd_rows / 2);
    if (isdigit(key)) {
      if (index<len) {
        current[index] = key;
        index++;
      }
    }
    else if (key == 'C') {
      current = strunset(current, len);
      index = 0;
    }
    else if (key == 'D') {
      pass[len] = '\0';
      strncpy(pass, current, len);
      current = strunset(current, len);
      index = 0;
      lcd.clear();
      free(current);
      return;
    }
  }
}

void menu(char* pass, long& wait) {
  static const char* items[] = {"set PIN", "timeout mode", "set timeout", "set PIN len"};
  const byte items_len = sizeof(items) / sizeof(char*);
  byte index = 0;
  char* w_s;
  byte n_i, c_pos;
  bool redraw = true;
  byte last = 0;
  char key;
  lcd.clear();
  while (true) {
    key = keypad.getKey();
    if (key == 'A') { // move up
      if (index > 0) index--;
    }
    else if (key == 'B') { // move down
      if (index < items_len-1) index++;
    }
    else if (key == 'D') { 
      if (index == 0) { // set new pin
        get_pin_from_keypad("Set PIN:", pass);
        redraw = true;
      }
      else if (index == 1) { // switch unlock mode
        if (METHOD == WAIT) {
          METHOD = CONFIRM;
          lcd.clear();
          lcd_center("timeout: off", lcd_rows / 2 - 1);
          delay(1500);
          redraw = true;
        }
        else if (METHOD == CONFIRM) {
          METHOD = WAIT;
          lcd.clear();
          lcd_center("timeout: on", lcd_rows / 2 - 1);
          delay(1500);
          redraw = true;
        }
      }
      else if (index == 2) { // set new delay
        w_s = strunset();
        get_pin_from_keypad("Set timeout (s):", w_s, 4);
        wait = int_from_pinstr(w_s) * 1000L;
        free(w_s);
        redraw = true;
      }
      else if (index == 3) { // set length of PIN
        do {
          w_s = strunset();
          get_pin_from_keypad("Set PIN length:", w_s, 1);
          PIN_LEN = int_from_pinstr(w_s);
          free(w_s);
        } while (!(PIN_LEN > 0 && PIN_LEN <= 9));

        get_pin_from_keypad("Set PIN:", pass);

        redraw = true;
      }
    }
    else if (key == '*') {
      lcd.clear();
      return;
    }

    if (last != index || redraw) {
      lcd.clear();
      n_i = index;
      if (index >= items_len - lcd_rows) n_i = items_len - lcd_rows;
      for (int i = 0; i < lcd_rows; i++) {
        lcd.setCursor(3, i);
        lcd.print(items[n_i+i]);
      }

      c_pos = index-n_i;
      lcd.setCursor(0, c_pos);
      lcd.write('-');
      lcd.write('>');
    }
    if (redraw) redraw = false;
    last = index;
  }
}

void loop() {
  static char* current    = strunset(); // we never need to free these strings
  static char* pass       = strunset();
  static long  wait       = 5000;
  static bool  pass_set   = false;
  static byte  index      = 0;
  static char  key;
  static int   failed     = 0;
  static int   multiplier = 15;

  key = keypad.getKey();

  /* --- Set the initial password --- */
  if (!pass_set) {
    get_pin_from_keypad("Set PIN:", pass);
    pass_set = true;
  }
  /* -------------------------------- */

  if (key == 'D' && !strcmp(current, pass)) { // unlock
    failed = 0;
    multiplier = 15;
    unlock();
    if (METHOD == WAIT) {
      char t[5];
      lcd.clear();
      lcd_center("* lock  # menu", lcd_rows / 2 - 1);
      long m = millis();
      long ms;
      while (millis() < m+wait) {
        key = keypad.getKey();
        if (key == '#') {
          menu(pass, wait);
          break;
        }
        else if (key == '*') break;
        ms = m+wait-millis();
        byte center = lcd_cols/2 - 3 / 2;
        lcd.setCursor(center, 1);
        itoa(ms/1000, t, 10);
        lcd_center(t,lcd_rows / 2);
      }
    }
    else if (METHOD == CONFIRM) {
      lcd_center("* lock  # menu", lcd_rows / 2 - 1);
      key = '0';
      while (true) { // wait for lock confirm
        key = keypad.getKey();
        if (key == '*') {
          break;
        }
        else if (key == '#') {
          menu(pass, wait);
          break;
        }
      }
    }

    lock();
    reset_pass(current, index);
  }
  else if (isdigit(key)) {
    if (index<PIN_LEN) {
      current[index] = key;
      index++;
    }
  }


  else if (key == 'C') { // clear password
    reset_pass(current, index);
  }


  else if (key == 'D') { // failed unlock
    reset_pass(current, index, true);
    failed++;
    if (failed > 2) { // anti-bruteforce wait after 3 failed attempts
      lcd.clear();
      lcd.println("No bruteforcing!");
      long d = 1000 * multiplier;
      multiplier*=2;
      long m = millis();
      while (millis() < m+d) {
        int ms = m+d-millis();
        lcd.setCursor(lcd_cols/2 - 3 / 2, 1);
        lcd.println(ms/1000);
        m++;
      }
      failed = 0;
    }
    reset_pass(current, index, false);
  }

  lcd_center(current, lcd_rows / 2 - 1);
  lcd_center("locked", lcd_rows / 2);
}
