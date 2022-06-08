#include "FastLED.h"
#include <Arduino.h>
#include <Wire.h> 
#include "RTClib.h" 
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <TM1637Display.h>

///////////////////////// Bluetooth pinos
SoftwareSerial bluetooth(6, 5);

//////////////TM1637 display module pinos
#define CLK 8
#define DIO 7

const uint8_t SEG_DONE[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,          
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   
  SEG_C | SEG_E | SEG_G,                           
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            
};
TM1637Display display(CLK, DIO);   //Inicializa o display

///////////////////////// RTC MODULE DS1307
//RTC_DS1307 rtc; 
  RTC_DS3231 rtc;

DateTime now; //classe now significa o tempo atual para o qual o rtc foi configurado

///////////////////////// variáveis para o modo de luz()
#define FRAMES_PER_SECOND 60
bool gReverseDirection = false;
CRGBPalette16 gPal;
int COOLING = 8;
int SPARKING = 80; //40

///////////////////////// LED WS2811
// Definir LEDS
#define DATA_PIN    4  
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    10
#define BRIGHTNESS  255
CRGB leds[NUM_LEDS];

//////////////////////////////////////////////////////////// Botões
///////////////////////////////////// Botão das horas
const int safe_button_press = 200;

bool read_button_clock = true;

const byte button_clock = 12; // pino 12
byte button_clock_state = 0;
byte button_clock_last_state = 0;
unsigned long button_clock_startPressed = 0;    // momento  que foi pressionado
unsigned long button_clock_endPressed = 0;      // momento que foi solto
unsigned long button_clock_holdTime = 0;        // quanto tempo ficou pressionado
unsigned long button_clock_idleTime = 0;        // quanto tempo o botao teve inativo
//unsigned long time_before_play_alarm = 0;
unsigned long time_last_played;

int beep_delay = 2000;

int times_beeped = 0;
int times_beeped_absolute = 0;

bool potenciometro = true;

unsigned long currentMillis;

unsigned long allow_change_clock = 0;

///////////////////////////////////// Botao UP
const byte button_up = 11; // pino 11
byte button_up_state = 0;
byte button_up_last_state = 0;
unsigned long button_up_startPressed = 0;    // momento  que foi pressionado
unsigned long button_up_endPressed = 0;      // momento que foi solto
unsigned long button_up_holdTime = 0;        // quanto tempo ficou pressionado
unsigned long button_up_idleTime = 0;        // quanto tempo o botao teve inativo

///////////////////////////////////// Botao DOWN
const byte button_down = 10; // pino 10
byte button_down_state = 0;
byte button_down_last_state = 0;
unsigned long button_down_startPressed = 0;    // momento  que foi pressionado
unsigned long button_down_endPressed = 0;      // momento que foi solto
unsigned long button_down_holdTime = 0;        // quanto tempo ficou pressionado
unsigned long button_down_idleTime = 0;        // quanto tempo o botao teve inativo

///////////////////////////////////// Botao das luzes 
const byte button_luzes = 9; // pino9
byte button_luzes_state = 0;
byte button_luzes_last_state = 0;
unsigned long button_luzes_startPressed = 0;    // momento  que foi pressionado
unsigned long button_luzes_endPressed = 0;      // momento que foi solto
unsigned long button_luzes_holdTime = 0;        // quanto tempo ficou pressionado
unsigned long button_luzes_idleTime = 0;        // quanto tempo o botao teve inativo

///////////////////////// BUZZER pino
const int pinBuzzer = 2;

//////Potenciometro
int pot_val_old = 0;
int min_pot_val = 1;
int max_pot_val = 1023;

// Definir alarme:
int  alarm_time_hour;
int alarm_time_hour_address = 0;
int  alarm_time_minute;
int alarm_time_minute_address = 1;
//////// Definir novas horas e minutos:
int new_minute;
int new_hour;

unsigned long time_begin_sunshine = 0;
unsigned long time_end_sunshine = 0;
unsigned long time_end_alarm = 0;


unsigned long time_last_showed = 0;

bool set_hour_minute_flag = false;
bool clock_on = true;
bool alarm_on = true;
bool sleeping = true;
bool set_alarm = false;
bool allow_change_set_alarm = true;
bool allow_change_alarm_on = false;
bool allow_change_hour_minute = false;
bool luzes_on = true;
bool button_check;

int luzes_mode = 1;
int num_luzes_modes = 6;

int brilho = 3;
int old_brilho = 0;

//////////////////Bluetooth
String comando_bt;
String data_bt = "";
char command;
int dados;


void setup() {
  ///aqui  irei configurar o modo dos pinos como entrada, saída ou entrada com pull up ativo
  ///iniciar a biblioteca fast led e rtc
  ///dar valores
  ///apenas 1 vez
  delay(2000); 
    
    Serial.begin(9600);
    bluetooth.begin(9600);

    //////////////////////////////////////////////////////////// LED WS2811
    // Configuração dos LEDS
    FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip).setDither(BRIGHTNESS < 255);
    FastLED.setBrightness(BRIGHTNESS);//controle do brilho

    // Potenciometro
    pinMode(A1, INPUT);//pino de entrada

    pinMode(button_clock, INPUT_PULLUP); //configurar o botao clock como pino de entrada com pull up ativo
    button_clock_last_state = digitalRead(button_clock);  //vai ler o ultimo clique do relogio
    button_clock_state = digitalRead(button_clock); //ler o tempo que foi pressionado o botão
    pinMode(button_up, INPUT_PULLUP);
    pinMode(button_down, INPUT_PULLUP);
    pinMode(button_luzes, INPUT_PULLUP);

    pinMode(pinBuzzer, OUTPUT); //pino do buzzer como saida(output)

  
  ////////////////////////////////////////////////////////////RTC
    rtc.begin();    //rtc inicia
    now = rtc.now();  //vai procurar o valor de now

    //////////////////////////////////////////////////////////// alarme

    alarm_time_hour = EEPROM.read(alarm_time_hour_address);      //EEPROM(chip incorporado não no rtc mas no arduino) irá ler o valor que iremos dar ao alarme para depois armazenar
    alarm_time_minute = EEPROM.read(alarm_time_minute_address);

}
/////////////////////////////////////////VOID LOOP
void loop() {
  ///Verificação dos botoes leds rtc ecrã tudo repetidamente
  ///funciona como um ciclo vai sempre se repetindo

  /////////////////////////////////////////////////////////RTC
  DateTime now = rtc.now(); //VAI BUSCAR O VALOR DAS HORAS ATUAIS

  ////////////////////////////////////////////////////////Bluetooth

  if (bluetooth.available()) {

        command = 'x';

        while (bluetooth.available()) {
            
            char char_bt = (bluetooth.read());

            comando_bt += char_bt;

            if (command == 'x') {
                command = char_bt;
            }
          
        }
        Serial.println(comando_bt);
        data_bt = comando_bt.substring(1); 
        comando_bt = "";       //limpa o buffer
    }

    while (command != 'x') {
          delay(10);
          
          //ligar led
         if (command == 'J') {
          luzes_on = !luzes_on;
          command = 'x';
          break;
          }

          //ligar TM1637
          if (command == 'X'){
            clock_on = !clock_on;
            if (clock_on) {
                    Serial.println("ecra on");
                }
                if (!clock_on) {
                    Serial.println("ecra off");
                }
            command = 'x';
            break;
            }

            if (command == 'U'){
            luzes_mode = 1;
            command = 'x';
            break;
            }

            if (command == 'N'){
            luzes_mode = 2;
            command = 'x';
            break;
            }
            if (command == 'U'){
            luzes_mode = 3;
            command = 'x';
            break;
            }
            if (command == 'P'){
            luzes_mode = 4;
            command = 'x';
            break;
            }
            if (command == 'L'){
            luzes_mode = 5;
            command = 'x';
            break;
            }
            if (command == 'T'){
            luzes_mode = 6;
            command = 'x';
            break;
            }

           if (command == 'M'){
            luzes_mode += 1;

            if (luzes_mode > num_luzes_modes) {
                luzes_mode = 1;
            }
            command = 'x';
            break;
           }
           

            if (command == 'E') {
            Serial.print("xxxx -> ");
            Serial.println(data_bt);
            String hora = data_bt.substring(0,2);
            String minuto = data_bt.substring(2,4);

            int int_minuto = minuto.toInt();
            int int_hora = hora.toInt();

            //int_minuto = EEPROM.read(alarm_time_minute_address);
            //int_hora = EEPROM.read(alarm_time_hour_address);

            new_minute = int_minuto;
            new_hour = int_hora;

            rtc.adjust(DateTime(now.year(), now.month(), now.day(), new_hour, new_minute, now.second()));

            Serial.println("ajustado relogio para");
            Serial.println(int_hora);
            Serial.println(int_minuto);
            //Serial.println(hora);
            get_current_time_once();

            command = 'x';

            /*static String hora = data_bt.substring(1, 2);
            static String minuto = data_bt.substring(3, 4);    

            if (set_hour_minute_flag) {
 
            new_minute = minuto.toInt();
            new_hour = hora.toInt();

            if(new_hour >=24) {
              new_hour = 0;
              }
            
            if (new_minute >= 60) {
                new_minute = 0;
            }
        }

            //EEPROM.write(alarm_time_minute_address, int_minuto);
            //EEPROM.write(alarm_time_hour_address, int_hora);

            Serial.println("horas ajustadas");*/

            command = 'x';
            break;
           }

          if (command == 'A') {
          alarm_on = !alarm_on;
          if (alarm_on) {
                    Serial.println("O Alarme está ativo");
                }
                if (!alarm_on) {
                    Serial.println("O alarme está desativado");
                }

          command = 'x';
          break;
          }

          if (command == 'B'){
            set_hour_minute_flag = !set_hour_minute_flag;
            command = 'x';
            break;
            }

            if (command == 'D'){
            set_alarm = !set_alarm;
            command = 'x';
            break;
            }

            if (command == 'R') {
            Serial.print("xxxx -> ");
            Serial.println(data_bt);
            String hora = data_bt.substring(0,2);
            String minuto = data_bt.substring(2,4);

            int int_minuto = minuto.toInt();
            int int_hora = hora.toInt();

            EEPROM.write(alarm_time_minute_address, int_minuto);
            EEPROM.write(alarm_time_hour_address, int_hora);

            alarm_time_hour = int_hora;
            alarm_time_minute = int_minuto;

            Serial.println("ajustado alarme para");
            Serial.println(int_hora);
            Serial.println(int_minuto);
            //Serial.println(hora);

            command = 'x';
            break;
            }

           if (command == 'I') {            

            String pot_val = data_bt;
            pot_val_old = map(pot_val.toInt(),0,100,min_pot_val,max_pot_val);
            Serial.print("--------------->+");
            Serial.println(pot_val);

            command = 'x';
            break;
            }
            

          command = 'x';
           
      }





  
/*  if (bluetooth.available() > 0){
      command = bluetooth.read();
      dados = command;
      Serial.println(dados);
    }
    delay (400);

  if (dados == 'M') {
    !luzes_on;
    }

  if (dados == 'J'){
    luzes_on = !luzes_on;
    }*/

    //////////////////////
/*
  if (dados == 'J') {
    luzes_mode = 5;
    }
*/
  /*  
  if (bluetooth.available() > 0) {

        command = 'x';

        while (bluetooth.available() > 0) {
            
            char char_bt = (()bluetooth.read());

            char_bt += char_bt;

            if (command == 'x') {
                command = char_bt;
            }

        }

        data_bt = comando_bt.substring(1);        
    }*/

/*
    if (bluetooth.available()) {

        command = 'x';

        while (bluetooth.available()) {
            
            char char_bt = (bluetooth.read());

            comando_bt += char_bt;

            if (command == 'x') {
                command = char_bt;
            }

        }

        data_bt = comando_bt.substring(1);        
    }

    while (command != 'x') {

        if (command == 'j') {

            luzes_mode += 1;

            if (luzes_mode > num_luzes_modes) {
                luzes_mode = 1;
            }

            command = 'x';
            break;
        }

        if (command == 'v') {

            luzes_mode -= 1;

            if (luzes_mode < 1) {
                luzes_mode = num_luzes_modes;
            }

            command = 'x';
            break;
        }

        if (command == 't') {

            clock_on = !clock_on;            
            command = 'x';
            break;
        }

        if (command == 'm') {
            luzes_on = !luzes_on;
            command = 'x';
            break;
        }

        if (command == 'e') {

            static String hora = data_bt.substring(1, 2);
            static String minuto = data_bt.substring(3, 4);

            int int_minuto = minuto.toInt();
            int int_hora = hora.toInt();

            EEPROM.write(alarm_time_minute_address, int_minuto);
            EEPROM.write(alarm_time_hour_address, int_hora);

            command = 'x';
      

       if (command == 'k') {            

            luzes_mode = 1;
            static String pot_val = data_bt;
            pot_val_old = map(pot_val.toInt(),0,100,min_pot_val,max_pot_val);

            display.showNumberDec(data_bt.toInt(), false);
            display.setBrightness(7);




            command = 'x';
        }

        command = 'x';

    }
    }*/

  //////////////////////////////////////////////////////Potenciometro
  //// Potenciometro valores
  if(potenciometro){
    int pot_val_new = analogRead(A1);

    if (abs(pot_val_new - pot_val_old) > 10) {
        pot_val_old = pot_val_new;
    }
  }
  /////////////////////////////////////////////////////////tm1637
  /*Serial.begin(9600);
  display.setBrightness(7);//brilho
  display.showNumberDec((now.hour()* 100 )+ now.minute(), true); //display vai buscar a hora e minuto atuais (função now)*/

  //verificar botoes
  button_check = check_if_buttons_were_pressed();

  //////////////////////////////////////////////// Mostrar efeitos

    if (luzes_on) {
    //se as luzes = true então irá de definir os modos nos leds de (0 a 6) no caso
        if (luzes_mode == 1) {

            brilho = map(pot_val_old, min_pot_val, max_pot_val, 20, BRIGHTNESS); //escala o valor do potenciometro

            if (brilho > 220) {
                brilho = BRIGHTNESS; //BRIGHTNESS=255
            }

            //Serial.println(brilho);

           /* if ((brilho - old_brilho) > 2 || (brilho - old_brilho) < -2) {
                old_brilho = brilho;
            }*/

            moon(brilho);
        }

        else if (luzes_mode == 2) {
            SPARKING = map(pot_val_old, min_pot_val, max_pot_val, 10, 200); 
            fireplace(SPARKING, COOLING);
            FastLED.show();
        }

        else if (luzes_mode == 3) {
            jujuba(pot_val_old);
        }

        else if (luzes_mode == 4) {

            rainbowCycle(map(pot_val_old, min_pot_val, max_pot_val, 1, 20));
            FastLED.show();
        }
        else if (luzes_mode == 5) {
            Respiro();
        }
        else if (luzes_mode == 6) {
            Serial.print("My color: ");
            Serial.println(pot_val_old);
            myColor(pot_val_old);
            
        }
        else if (luzes_mode == 100) {
            sunshine();
        }
        else {
            luzes_mode = 1; // Safety control
        }

    }

    if (!luzes_on) {
        turn_off_leds();
        //se for = false então os leds podem-se desligar
    }

    //////////////////////////////////////////////// Mostrar horas
    //true
    if (clock_on) {
        display_current_time(now, alarm_time_hour, alarm_time_minute);
    }
    //false
    if (!clock_on) {
        display.clear();
    }

    //////////////////////////////////////////////// Definir alarme 3sg
    if (set_alarm) {

        read_button_clock = true;
        clock_on = true;

        alarm_time_hour = EEPROM.read(alarm_time_hour_address);
        alarm_time_minute = EEPROM.read(alarm_time_minute_address);
        

    }

    if (!set_alarm && !set_hour_minute_flag) {
        time_last_showed = millis();
    }

    //////////////////////////////////////////////// Definir novas horas e minutos 8sg

    if (set_hour_minute_flag) {

        set_alarm = false;
        read_button_clock = true;
        clock_on = true;

    }

    ////////////////////////////////////////////////verificação de alarme

    if (alarm_on) {

        if (sunshine_check(now, alarm_time_hour, alarm_time_minute)) {
            luzes_on = true;
            luzes_mode = 100;
            clock_on = true;
        }
        else {

            time_begin_sunshine = millis();
            time_end_sunshine = time_begin_sunshine + (240000);
            time_end_alarm = time_begin_sunshine + (900000);           


        }

        // Verifica se o alarme coincide com as horas do rtc (now)
        if (wake_up_check(now, alarm_time_hour, alarm_time_minute)) {

            set_alarm = false;
            sleeping = false;
            play_alarm();

        }
    }

}

////////////////////////////////////////////////////FUNÇOES
///alarme
void show_alarm_status(bool status) {

    unsigned long time_to_stop = millis() + 2000;

    while (millis() < time_to_stop) {

        if (status) {

            for (int i = 0; i < NUM_LEDS; i++) {
                leds[i] = CRGB::ForestGreen;
            }
            FastLED.show();

        }

        if (!status) {

            for (int i = 0; i < NUM_LEDS; i++) {
                leds[i] = CRGB::Red;
            }
            FastLED.show();


        }

    }



}

void get_current_time_once() {

    DateTime now_to_change = rtc.now();

    new_minute = now_to_change.minute();
    new_hour = now_to_change.hour();

}

void play_alarm() {

    if (millis() > time_last_played) {
        tone(pinBuzzer, 600, 350);
        time_last_played = millis() + beep_delay;
        times_beeped += 1;
        times_beeped_absolute += 1;
        Serial.println("alarme a tocar");
    }

    if (times_beeped >= 15) {

        if (beep_delay >= 600) {
            beep_delay -= 500;
        }
        else {
            beep_delay = 400;
        }
        
        times_beeped = 0;
    }

    if (millis() >= time_end_alarm) {  //Desligar alarm
        alarm_on = false;
        times_beeped = 0;
        times_beeped_absolute = 0;
        beep_delay = 2000;
    }

    // Turn on the lights
    luzes_mode = 100;
    luzes_on = true;
    CRGB color1 = CHSV(255, 220, 0);

    // fade all existing pixels toward bgColor by "5" (out of 255)
    fadeTowardColor(leds, NUM_LEDS, color1, 1);
    FastLED.show();

}

bool sunshine_check(DateTime now, int alarm_time_hour, int alarm_time_minute) {

    // Compara horas e minutos da hora definida para o alarme com a hora atual

    String compare_alarm = "";    
    
    if (alarm_time_hour == 0) {
        compare_alarm += "00";
    }
    else {
        compare_alarm += alarm_time_hour;
    }

    if (alarm_time_minute <= 9) {
        compare_alarm += "0";    
        if (alarm_time_minute == 0) {
            compare_alarm += "0";
        }
        else {
            compare_alarm += alarm_time_minute;
        }
    }    else {
        compare_alarm += alarm_time_minute;
    }

    String compare_now = "" ;

    if (now.hour() == 0) {
        compare_now += "00";
    }
    else {
        compare_now += now.hour();
    }

    if (now.minute() <= 9) {
        compare_now += "0";
        if (now.minute() == 0) {
            compare_now += "0";
        }
        else {
            compare_now += now.minute();
        }
    }    else {
        compare_now += now.minute();
    }
    
    int compare_alarm_int = compare_alarm.toInt();
    int compare_now_int = compare_now.toInt();

    if ((compare_alarm_int - compare_now_int)<=5 && (compare_alarm_int - compare_now_int) >=0 ){ 

            return true;        
    }
    else {
        return false;
    }

}

bool wake_up_check(DateTime now, int alarm_time_hour, int alarm_time_minute) {

    // Compara horas e minutos da hora definida para o alarme com a hora atual

    if (alarm_time_hour == now.hour() && alarm_time_minute == now.minute()) {
        return true;
    }
    else {
        return false;
    }

}

 ////////////////////////////////////////////////////////////////////////////////////////////////// Relógio horas

void display_current_time(DateTime now, int alarm_hour, int alarm_minute) {

    /////////////////////////// Horas Atuais:
    if (!set_alarm && !set_hour_minute_flag) {
        if (!set_alarm) {
        display.setBrightness(7); 
       uint8_t position[] = { 0, 0, 0, 0 };
       position[0] = display.encodeDigit(now.hour() / 10); // divide hora por 10 e coloca a parte inteira do resultado no primeiro display
       position[1] = display.encodeDigit(now.hour() % 10); // divide hora por 10 e coloca o resto da divisao no segundo display
       position[2] = display.encodeDigit((now.minute() % 60) / 10); // divide minuto por 10 e coloca a parte inteira do resultado no terceiro display
       position[3] = display.encodeDigit((now.minute() % 60) % 10); // divide minuto por 10 e coloca o resto da divisao no quarto display
        position[1] = position[1] + 128;  //// Coloca o separador ":"
        display.setSegments(position); //organiza/atualiza as informaçoes no display
    }

  }

    /////////////////////////// Mostrar alarme

    if (set_alarm) {

        if (millis() > time_last_showed && millis() < time_last_showed + 500) {


            // brilho
            display.setBrightness(7);

            // Zera e cria o array position[] para armazenar os valores do alarme
            uint8_t position[] = { 0, 0, 0, 0 };
            // Calculo individual para posição no display
            position[0] = display.encodeDigit(alarm_hour / 10); // divide hora por 10 e coloca a parte inteira do resultado no primeiro display
            position[1] = display.encodeDigit(alarm_hour % 10); // divide hora por 10 e coloca o resto da divisao no segundo display
            position[2] = display.encodeDigit((alarm_minute % 60) / 10); // divide minuto por 10 e coloca a parte inteira do resultado no terceiro display 
            position[3] = display.encodeDigit((alarm_minute % 60) % 10); // divide minuto por 10 e coloca o resto da divisao no quarto display
            position[1] = position[1] + 128; // Coloca o separador ":"
            display.setSegments(position);  //organiza/atualiza as informaçoes no display
        }

        if (millis() >= time_last_showed + 500) {
            display.clear();
            time_last_showed = millis() + 250;
        }
    }

    /////////////////////////// Novas horas e minutos
    if (set_hour_minute_flag) {

        if (millis() > time_last_showed && millis() < time_last_showed + 500) {


            // Set brightness
            display.setBrightness(7);

            // Zera e cria o array position[] para armazenar os novos valores das horas
            uint8_t position[] = { 0, 0, 0, 0 };
            // Calculo individual para posição no display
            position[0] = display.encodeDigit(new_hour / 10); // divide hora por 10 e coloca a parte inteira do resultado no primeiro display
            position[1] = display.encodeDigit(new_hour % 10); // divide hora por 10 e coloca o resto da divisao no segundo display
            position[2] = display.encodeDigit((new_minute % 60) / 10); // divide minuto por 10 e coloca a parte inteira do resultado no terceiro display 
            position[3] = display.encodeDigit((new_minute % 60) % 10); // divide minuto por 10 e coloca o resto da divisao no quarto display
            // Coloca o separador ":"
            position[1] = position[1] + 128;
            display.setSegments(position); //organiza/atualiza as informaçoes no display
        }

        if (millis() >= time_last_showed + 500) {
            display.clear();
            time_last_showed = millis() + 250;
        }

    }

}

//alterar hora e minutos do rtc
void  set_hour_minute() {

    rtc.adjust(DateTime(now.year(), now.month(), now.day(), new_hour, new_minute, now.second())); //(ano), (mes), (dia), (hora), (minutos), (segundos)
 
}
void  update_my_time_now() {
    DateTime now = rtc.now();
}

/////////////////////// Modos de luz efeitos
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

byte cor_sunshine_R = 0xff;
byte cor_sunshine_G = 0xbf;
byte cor_sunshine_B = 0x00;

void sunshine() {

    float r, g, b;

    unsigned long fator = map(millis(), time_begin_sunshine, time_end_sunshine, 0, 1000);

    if (fator > 1000) {
        fator = 1000;
    }


    r = cor_sunshine_R * fator / 1000;
    g = cor_sunshine_G * fator / 1000;
    b = cor_sunshine_B * fator / 1000;
    setAll(r, g, b);
    showStrip(); 


    /*for (int i = 0; i < NUM_LEDS; i++) {


        leds[i] = CRGB::Gold;
        FastLED.setBrightness(brilho3);
        FastLED.show();

        if (!check_if_buttons_were_pressed) {
            break;
        }

    }*/

}
//modo personalizado
void myColor(int pot_val) {


    CRGB color1 = CHSV(map(pot_val, min_pot_val, max_pot_val, 0, 255), 255, 255);

    // fade all existing pixels toward bgColor by "5" (out of 255)
    fadeTowardColor(leds, NUM_LEDS, color1, 20);
    FastLED.show();
    // Serial.println(map(pot_val,0,1023,0,255));

}


bool comecar = true;
bool respirar = false;
bool segurar = false;
bool expirar = false;
bool segurar2 = false;

unsigned long time_begin;
unsigned long time_to_hold;
unsigned long time_to_let_go;
unsigned long time_to_let_go2;
unsigned long time_to_begin_again;

byte cor_respiro_R = 0xff;
byte cor_respiro_G = 0x55;
//byte cor_respiro_B = 0x16;
byte cor_respiro_B = 0x00;


//byte cor_respiro_R = 0xff;
//byte cor_respiro_G = 0xd1;
//byte cor_respiro_B = 0x19;

//modo relaxante
void Respiro() {
  //assim como quando nós inspiramos e expiramos esta sequencia de efeitos parece que acompanha-nos nesse ritmo parecendo que é relaxante
  //uma sequencia mais relaxante para um mood mais calmo

    float r, g, b;

    if (comecar) {
        time_begin = millis();
        time_to_hold = millis() + 4000;
        time_to_let_go = time_to_hold + 3000;
        time_to_let_go2 = time_to_let_go + 300;
        comecar = false;
        respirar = true;
    }

    if (respirar) {

        unsigned long fator = map(millis(), time_begin, time_to_hold, 0, 1000);

        r = cor_respiro_R * fator / 1000;
        g = cor_respiro_G * fator / 1000;
        b = cor_respiro_B * fator / 1000;
        setAll(r, g, b);
        showStrip();

        if (millis() >= time_to_hold) {

            respirar = false;

        }
    }

    if (millis() >= time_to_hold) {
        //time_to_let_go = millis() + 80000;

        CRGB bgColor1(0, 100, 255);
        fadeTowardColor(leds, NUM_LEDS, bgColor1, 20);
        FastLED.show();
    }

    if (millis() >= time_to_let_go) {
        time_to_hold = millis() + 80000;

        CRGB bgColor2(255, 85, 0);
        fadeTowardColor(leds, NUM_LEDS, bgColor2, 20);
        FastLED.show();

    }

    if (millis() >= time_to_let_go2) {
        time_to_let_go = millis() + 80000;
        time_to_let_go2 = millis() + 80000;
        time_begin = millis();
        time_to_hold = millis() + 2500;
        expirar = true;

    }

    if (expirar) {
        // Expira

        unsigned long fator = map(millis(), time_begin, time_to_hold, 1000, 0);

        r = (cor_respiro_R * fator / 1200);
        g = (cor_respiro_G * fator / 1200);
        b = (cor_respiro_B * fator / 1200);
        setAll(r, g, b);
        showStrip();

        if (millis() >= time_to_hold) {
            /*for (int i = 0; i < NUM_LEDS; i++) {
                leds[i] = CRGB::Black;
            }
            FastLED.show();*/
            time_to_begin_again = millis() + 3300;
            time_to_hold = millis() + 80000;
            expirar = false;
            segurar2 = true;
        }
    }

    if (segurar2) {
        for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = CRGB::Black;
        }
        FastLED.show();

        if (millis() >= time_to_begin_again) {
            comecar = true;
            segurar2 = false;
        }
    }
}

void FadeInOut(byte red, byte green, byte blue, int speed) {
    float r, g, b;

    for (int k = 0; k < (256 * speed); k = k + 1) {
        r = (k / (256.0 * speed)) * red;
        g = (k / (256.0 * speed)) * green;
        b = (k / (256.0 * speed)) * blue;
        setAll(r, g, b);
        showStrip();
        if (check_if_buttons_were_pressed()) {
            break;
        }
    }

    for (int k = (255 * speed); k >= 0; k = k - 2) {
        r = (k / (256.0 * speed)) * red;
        g = (k / (256.0 * speed)) * green;
        b = (k / (256.0 * speed)) * blue;
        setAll(r, g, b);
        showStrip();
        if (check_if_buttons_were_pressed()) {
            break;
        }
    }
}

void rainbowCycle(int SpeedDelay) {
    byte* c;
    uint16_t i, j;



    for (j = 0; j < 256; j++) { // 5 ciclos de todas as cores na fita
        for (i = 0; i < NUM_LEDS; i++) {
            c = Wheel(((i * 256 / NUM_LEDS) + j) & 255);
            setPixel(i, *c, *(c + 1), *(c + 2));
            if (check_if_buttons_were_pressed()) {
                break;
            }

        }
        showStrip();

        if (check_if_buttons_were_pressed()) {
            break;
        }

        delay(SpeedDelay);
    }
}

byte* Wheel(byte WheelPos) {
    static byte c[3];

    if (WheelPos < 85) {
        c[0] = WheelPos * 3;
        c[1] = 255 - WheelPos * 3;
        c[2] = 0;
    }
    else if (WheelPos < 170) {
        WheelPos -= 85;
        c[0] = 255 - WheelPos * 3;
        c[1] = 0;
        c[2] = WheelPos * 3;
    }
    else {
        WheelPos -= 170;
        c[0] = 0;
        c[1] = WheelPos * 3;
        c[2] = 255 - WheelPos * 3;
    }    

    return c;
}

void colorWipe(byte red, byte green, byte blue, int SpeedDelay) {
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        setPixel(i, red, green, blue);
        showStrip();
        delay(SpeedDelay);
    }
}

void CylonBounce(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {

    for (int i = 0; i < NUM_LEDS - EyeSize - 2; i++) {
        setAll(0, 0, 0);
        setPixel(i, red / 10, green / 10, blue / 10);
        for (int j = 1; j <= EyeSize; j++) {
            setPixel(i + j, red, green, blue);
        }
        setPixel(i + EyeSize + 1, red / 10, green / 10, blue / 10);
        showStrip();
        delay(SpeedDelay);
    }

    delay(ReturnDelay);

    for (int i = NUM_LEDS - EyeSize - 2; i > 0; i--) {
        setAll(0, 0, 0);
        setPixel(i, red / 10, green / 10, blue / 10);
        for (int j = 1; j <= EyeSize; j++) {
            setPixel(i + j, red, green, blue);
        }
        setPixel(i + EyeSize + 1, red / 10, green / 10, blue / 10);
        showStrip();
        delay(SpeedDelay);
    }

    delay(ReturnDelay);
}
//modo noite
void moon(int brilho1) {

    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Beige;
        FastLED.setBrightness(brilho1);
        FastLED.show();

        if (!check_if_buttons_were_pressed) {
            break;
        }

    }
}
//modo chama
void fireplace(int SPARKING, int COOLING) {

    gPal = CRGBPalette16(CRGB::Black, CRGB::Red, CRGB::OrangeRed, CRGB::Yellow);

    //  crgb:black vai emitindo cores mais escuras pretas
    // ciclo vai do preto ate ao amarelo parecendo um efeito de fogueira
    //gPal = HeatColors_p;

    // These are other ways to set up the color palette for the 'fire'.
    // First, a gradient from black to red to yellow to white -- similar to HeatColors_p
    //   gPal = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::Yellow, CRGB::White);

    // Second, this palette is like the heat colors, but blue/aqua instead of red/yellow
    //   gPal = CRGBPalette16( CRGB::Black, CRGB::Blue, CRGB::Aqua,  CRGB::White);

    // Third, here's a simpler, three-step gradient, from black to red to white
    //   gPal = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::White);

    // Add entropy to random number generator; we use a lot of it.
    random16_add_entropy(random());

    // Fourth, the most sophisticated: this one sets up a new palette every
    // time through the loop, based on a hue that changes every time.
    // The palette is a gradient from black, to a dark color based on the hue,
    // to a light color based on the hue, to white.
    //
    //   static uint8_t hue = 0;
    //   hue++;
    //   CRGB darkcolor  = CHSV(hue,255,192); // pure hue, three-quarters brightness
    //   CRGB lightcolor = CHSV(hue,128,255); // half 'whitened', full brightness
    //   gPal = CRGBPalette16( CRGB::Black, darkcolor, lightcolor, CRGB::White);

    // Array of temperature readings at each simulation cell
    static byte heat[NUM_LEDS];

    //reduzir luz
    for (int i = 0; i < NUM_LEDS; i++) {
        heat[i] = qsub8(heat[i], random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }

    // aumenta e se difunde 
    for (int k = NUM_LEDS - 1; k >= 2; k--) {
        heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
    }

    // acende novas luzes aleatorias no fundo
    if (random8() < SPARKING) {
        int y = random8(7);
        heat[y] = qadd8(heat[y], random8(160, 255));
    }

    // Step 4.  Map from heat cells to LED colors
    for (int j = 0; j < NUM_LEDS; j++) {
        // Scale the heat value from 0-255 down to 0-240
        // for best results with color palettes.
        byte colorindex = scale8(heat[j], 240);
        CRGB color = ColorFromPalette(gPal, colorindex);
        int pixelnumber;
        if (gReverseDirection) {
            pixelnumber = (NUM_LEDS - 1) - j;
        }
        else {
            pixelnumber = j;
        }
        leds[pixelnumber] = color;
    }

    FastLED.show(); // display this frame
    FastLED.delay(1000 / FRAMES_PER_SECOND);

}
//desligar leds
void turn_off_leds() {

    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Black;
    }

    FastLED.show();

}

void showStrip() {

    // FastLED
    FastLED.show();

}

void setPixel(int Pixel, byte red, byte green, byte blue) {

    // FastLED
    leds[Pixel].r = red;
    leds[Pixel].g = green;
    leds[Pixel].b = blue;

}

void setAll(byte red, byte green, byte blue) {
    for (int i = 0; i < NUM_LEDS; i++) {
        setPixel(i, red, green, blue);
    }
    showStrip();
}

void nblendU8TowardU8(uint8_t& cur, const uint8_t target, uint8_t amount)
{
    if (cur == target) return;

    if (cur < target) {
        uint8_t delta = target - cur;
        delta = scale8_video(delta, amount);
        cur += delta;
    }
    else {
        uint8_t delta = cur - target;
        delta = scale8_video(delta, amount);
        cur -= delta;
    }
}

// Blend one CRGB color toward another CRGB color by a given amount.
// Blending is linear, and done in the RGB color space.
// This function modifies 'cur' in place.
CRGB fadeTowardColor(CRGB& cur, const CRGB& target, uint8_t amount)
{
    nblendU8TowardU8(cur.red, target.red, amount);
    nblendU8TowardU8(cur.green, target.green, amount);
    nblendU8TowardU8(cur.blue, target.blue, amount);
    return cur;
}

// Fade an entire array of CRGBs toward a given background color by a given amount
// This function modifies the pixel array in place.
void fadeTowardColor(CRGB* L, uint16_t N, const CRGB& bgColor, uint8_t fadeAmount)
{
    for (uint16_t i = 0; i < N; i++) {
        fadeTowardColor(L[i], bgColor, fadeAmount);
    }
}

CRGB color = CHSV(random8(), 255, 255);

unsigned long next_color = 0;
///modo fade
void jujuba(int pot_val) {

    if (millis() >= next_color) {
        color = CHSV(random8(), 255, 255);
        next_color = millis() + map(pot_val, min_pot_val, max_pot_val, 1000, 15000);
    }

    // todos os pixels existentes em direção a bgColor em "5" (de 255)
    fadeTowardColor(leds, NUM_LEDS, color, 1);
    FastLED.show();
}

/////////////////////////////////////BOTOES

void button_luzes_update_state() {

    if (digitalRead(button_luzes) == LOW) {  // If lights button is pressed: 

        button_luzes_startPressed = millis();
        button_luzes_idleTime = button_luzes_startPressed - button_luzes_endPressed;

        // ligar e desligar leds
        luzes_on = !luzes_on;

    }

    else {
        button_luzes_endPressed = millis();
        button_luzes_holdTime = button_luzes_endPressed - button_luzes_startPressed;
    }



}

void button_luzes_updateCounter() {

    // the button is still pressed
    if (button_luzes_state == LOW) {
        button_luzes_holdTime = millis() - button_luzes_startPressed;
    }
    else {
        button_luzes_idleTime = millis() - button_luzes_endPressed;
    }
}

void button_down_update_state() {

    if (digitalRead(button_down) == LOW) {  // se o botao down for pressionado 

        button_down_startPressed = millis();
        button_down_idleTime = button_down_startPressed - button_down_endPressed;

        // mudar valores do alarme
        if (set_alarm) {
            alarm_time_minute = alarm_time_minute + 5;

            EEPROM.write(alarm_time_minute_address, alarm_time_minute);

            if (alarm_time_minute >= 60) {
                alarm_time_minute = 0;
                EEPROM.write(alarm_time_minute_address, alarm_time_minute);
            }

        }
        else {

            luzes_mode -= 1;

            if (luzes_mode <= 0) {
                luzes_mode = num_luzes_modes;
            }


            button_down_endPressed = millis();
            button_down_holdTime = button_down_endPressed - button_down_startPressed;



        }

        // Mudar o valor(relógio) dos minutos até 60
        if (set_hour_minute_flag) {

            new_minute = new_minute + 1;

            if (new_minute >= 60) {
                new_minute = 0;
            }
        }

    }

    /* else {

         light_mode -= 1;

         if (light_mode <= 0) {
             light_mode = num_light_modes;
         }


         button_down_endPressed = millis();
         button_down_holdTime = button_down_endPressed - button_down_startPressed;
     }*/



}

void button_down_updateCounter() {

    // the button is still pressed
    if (button_down_state == LOW) {
        button_down_holdTime = millis() - button_down_startPressed;
    }
    else {
        button_down_idleTime = millis() - button_down_endPressed;
    }
}

void button_up_update_state() {

    if (digitalRead(button_up) == LOW) {  // Se o botão up for pressionado

        button_up_startPressed = millis();
        button_up_idleTime = button_up_startPressed - button_up_endPressed;

        //Mudar o valor das horas no alarme até 24 
        if (set_alarm) {

            alarm_time_hour = alarm_time_hour + 1;

            EEPROM.write(alarm_time_hour_address, alarm_time_hour);

            if (alarm_time_hour >= 24) {
                alarm_time_hour = 0;
                EEPROM.write(alarm_time_hour_address, alarm_time_hour);
            }

        }
        else {
            // Alterar os efeitos irá andar o nº a frente

            luzes_mode += 1;
            
            if (luzes_mode > num_luzes_modes) {
                luzes_mode = 1;
            }

            button_up_endPressed = millis();
            button_up_holdTime = button_up_endPressed - button_up_startPressed;

        }

        // Alterar o valor das horas no relogio
        if (set_hour_minute_flag) {

            new_hour = new_hour + 1;

            if (new_hour >= 24) {
                new_hour = 0;
            }
        }

    }
}

void button_up_updateCounter() {

    // the button is still pressed
    if (button_up_state == LOW) {
        button_up_holdTime = millis() - button_up_startPressed;
    }
    else {
        button_up_idleTime = millis() - button_up_endPressed;
    }
}

void button_clock_update_state() {

    allow_change_set_alarm = true;  // Enable permission to change the set_alarm flag

    if (digitalRead(button_clock) == LOW) {  // If clock button is pressed: 

        button_clock_startPressed = millis();
        button_clock_idleTime = button_clock_startPressed - button_clock_endPressed;

        clock_on = !clock_on; // If its true, it changes it's value to false, and vice versa   


    }
    else {
        button_clock_endPressed = millis();
        button_clock_holdTime = button_clock_endPressed - button_clock_startPressed;


    }


    // DEsligar alarme

    if (!sleeping) {
        alarm_on = false;
        sleeping = true;
        times_beeped = 0;
        times_beeped_absolute = 0;
    }

    if (button_clock_last_state == LOW) { // If it was LOW (pressed), it means it's just been released
       /* Serial.print("Button 'clock' was hold for ");
        Serial.println(button_clock_holdTime);*/
    }
    if (button_clock_last_state == HIGH) {
        /*  Serial.print("Button 'clock' was idle for ");
          Serial.println(button_clock_idleTime);*/
    }

}

void button_clock_updateCounter() {

    // the button is still pressed
    if (button_clock_state == LOW) {
        button_clock_holdTime = millis() - button_clock_startPressed;

        if (button_clock_holdTime > 1500 && button_clock_holdTime<3000 ) { // If it was pressed for more than 1.5 second:
            Serial.println("ok1.5");
            if (allow_change_set_alarm) {

                set_alarm = !set_alarm;
                Serial.println("SET ALARM FLAG CHANGED");

                allow_change_set_alarm = false;

                allow_change_alarm_on = true;
            }

        }

        if (button_clock_holdTime > 3000 && button_clock_holdTime<8000) { // If it was pressed for more than 3 second:
          Serial.println("ok3");
            if (allow_change_alarm_on) {

                alarm_on = !alarm_on;
                set_alarm = !set_alarm;
                allow_change_hour_minute = true;

                show_alarm_status(alarm_on);

                if (alarm_on) {
                    Serial.println("O Alarme está ativo");
                }
                if (!alarm_on) {
                    Serial.println("O alarme está desativado");
                }

                allow_change_alarm_on = false;


            }

        }

        if (button_clock_holdTime > 8000 && button_clock_holdTime<10000) { // If it was pressed for more than 8 seconds:
          Serial.println("ok8");
            if (allow_change_hour_minute) {

                Serial.println("As suas horas foram definidas");

                if (set_hour_minute_flag) {
                    set_hour_minute();
                }

                set_hour_minute_flag = !set_hour_minute_flag;
                set_alarm = false;
                clock_on = true;

                get_current_time_once();

                alarm_on = !alarm_on;
                allow_change_hour_minute = false;
            }
        }
    }
    else {
        button_clock_idleTime = millis() - button_clock_endPressed;
    }
}

////////////////////////////////////////////////////////////////verificação e leitura
bool check_if_buttons_were_pressed() {

    //////////////////////////////////////////////// BUTTON CLOCK

   // Check if button clock is pressed
    if (read_button_clock) {
        button_clock_state = digitalRead(button_clock);  // leitura dos botoes
    }

    if (button_clock_state != button_clock_last_state) { // button state muda ou seja o botao irá fazer outra funçao alternativa    

        button_clock_update_state();                     // atualiza o seu estado
        button_clock_last_state = button_clock_state;        // guarda o seu estado atual e segue o seu loop
        return true;

    }
    else {
        button_clock_updateCounter(); // button state not changed. It runs in a loop.
    }


    button_clock_last_state = button_clock_state;        // save state for next loop


     //////////////////////////////////////////////// BUTTON UP

    button_up_state = digitalRead(button_up); // leitura dos botoes

    if (button_up_state != button_up_last_state) { // button state muda ou seja o botao irá fazer outra funçao alternativa      

        button_up_update_state();
        button_up_last_state = button_up_state;        // save state for next loop
        return true;

    }
    else {
        button_up_updateCounter(); // button state not changed. It runs in a loop.
    }


    button_up_last_state = button_up_state;        // save state for next loop

    //////////////////////////////////////////////// BUTTON DOWN

    button_down_state = digitalRead(button_down); // leitura dos botoes

    if (button_down_state != button_down_last_state) { // button state muda ou seja o botao irá fazer outra funçao alternativa      

        button_down_update_state();
        button_down_last_state = button_down_state;        // save state for next loop
        return true;

    }
    else {
        button_down_updateCounter(); // button state not changed. It runs in a loop.
    }


    button_down_last_state = button_down_state;        // save state for next loop

    //////////////////////////////////////////////// BUTTON LIGHTS

    button_luzes_state = digitalRead(button_luzes); //leitura dos botoes

    if (button_luzes_state != button_luzes_last_state) { // button state muda ou seja o botao irá fazer outra funçao alternativa      

        button_luzes_update_state();
        button_luzes_last_state = button_luzes_state;        // save state for next loop
        return true;

    }
    else {
        button_luzes_updateCounter(); // button state not changed. It runs in a loop.
    }


    button_luzes_last_state = button_luzes_state;        // save state for next loop


    return false;


}
