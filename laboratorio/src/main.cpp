#include <Arduino.h>
#include <M5Cardputer.h>
#include <Preferences.h>
#include <Wire.h>
#include <M5UnitENV.h>

static constexpr int REDPIN=2, BLUEPIN=1;
static constexpr uint16_t BG=0x0849, PANEL=0x10EB, WHITE=0xFFFF, MUTED=0xAD75, CYAN=0x4E7F, YELLOW=0xFEE0, RED=0xF9A8, BLUE=0x449F, GREEN=0x5F0C;
M5Canvas cv(&M5Cardputer.Display);
Preferences prefs;
SHT3X sht;
QMP6988 qmp;
int mode=0, page=0;
bool envOK=false, haveEnv=false, locked=false, lastR=false, lastB=false;
uint32_t redCount=0, blueCount=0, lastRead=0, lastDraw=0, pulseAt=0;
float temp=0, hum=0, press=0;
int irRaw=0, irDigital=0;

void textAt(int x,int y,const char* text,uint16_t color=WHITE,int size=1){cv.setTextSize(size);cv.setTextColor(color);cv.setCursor(x,y);cv.print(text);}
void base(const char *title,const char *tag){
 cv.fillSprite(BG); cv.fillRoundRect(0,0,240,23,0,PANEL); cv.fillRoundRect(5,7,4,9,2,CYAN);
 textAt(14,5,title,WHITE,2); textAt(184,8,tag,CYAN,1);
 cv.drawFastHLine(0,119,240,0x294C);
}
void foot(const char *t){ textAt(7,124,t,MUTED); }
void safePins(){ if(mode==2) Wire.end(); pinMode(REDPIN,INPUT);pinMode(BLUEPIN,INPUT); }
void menu(){
 base("SENSOR LAB","ADV");
 textAt(9,30,"ELIGE UNA EXPERIENCIA",CYAN);
 textAt(11,46,"1",YELLOW,2);textAt(32,48,"DUAL BUTTON",WHITE,2);
 textAt(11,70,"2",YELLOW,2);textAt(32,72,"ENV III",WHITE,2);
 textAt(11,94,"3",YELLOW,2);textAt(32,96,"REFLECTIVE IR",WHITE,1);
 foot("1/2/3: elegir    I: seguridad");
}
void safety(){base("CONEXION","GUÍA");
 textAt(8,31,"1 APAGA el Cardputer",YELLOW,2);
 textAt(8,52,"2 RETIRA el USB",WHITE,2);
 textAt(8,73,"3 CAMBIA el modulo",WHITE,2);
 textAt(8,98,"No conectar encendido",CYAN,1);
 foot("M: menu   I: instrucciones");}
void prepare(){base("CAMBIO SEGURO","STOP");
 textAt(8,29,"SELECCION GUARDADA",GREEN,1);
 textAt(8,43,"APAGA EL EQUIPO",YELLOW,2);
 textAt(8,63,"RETIRA EL USB",WHITE,2);
 textAt(8,83,"CONECTA EL SENSOR",WHITE,2);
 textAt(8,105,"Enciende y abre esta app",CYAN);
 foot("Sin apagar: NO cambiar cable");}
void dual(bool info){
 base("DUAL BUTTON","GPIO");
 if(info){textAt(8,30,"Dos interruptores",CYAN,2);textAt(8,52,"Cada boton cambia su",WHITE,1);textAt(8,65,"senal entre 0 y 1.",WHITE,1);textAt(8,84,"Pulsar = cerrar circuito",WHITE,1);textAt(8,101,"Conteo desde inicio",MUTED,1);}
 else {cv.fillRoundRect(8,31,107,72,6,lastR?RED:PANEL);cv.fillRoundRect(124,31,108,72,6,lastB?BLUE:PANEL);
 textAt(16,37,"ROJO",WHITE,2);textAt(132,37,"AZUL",WHITE,2);
 textAt(16,61,lastR?"PULSADO":"LIBRE",WHITE,1);textAt(132,61,lastB?"PULSADO":"LIBRE",WHITE,1);
 cv.setTextSize(2);cv.setTextColor(WHITE);cv.setCursor(16,78);cv.printf("%lu",(unsigned long)redCount);cv.setCursor(132,78);cv.printf("%lu",(unsigned long)blueCount); }
 foot("I: explica   M: cambiar modulo");
}
void env(bool info){base("ENV III","I2C");
 if(info){textAt(8,30,"Temperatura: calor aire",WHITE,1);textAt(8,47,"Humedad: vapor de agua",WHITE,1);textAt(8,64,"Presion: peso del aire",WHITE,1);textAt(8,83,"Acerca tu mano y mira",CYAN,1);textAt(8,99,"como cambian los datos.",CYAN,1);}
 else if(!envOK || !haveEnv){textAt(8,32,"SIN LECTURA",YELLOW,2);textAt(8,56,"Apaga, retira USB y",WHITE);textAt(8,71,"verifica el ENV III.",WHITE);textAt(8,90,"No cambies con energia.",CYAN);}
 else {textAt(8,31,"TEMP",MUTED);textAt(129,31,"HUMEDAD",MUTED);cv.setTextSize(2);cv.setTextColor(WHITE);cv.setCursor(8,47);cv.printf("%.1f C",temp);cv.setCursor(129,47);cv.printf("%.0f %%",hum);textAt(8,75,"PRESION",MUTED);cv.setCursor(8,91);cv.printf("%.1f hPa",press);}
 foot("I: explica   M: cambiar modulo");}
void ir(bool info){base("REFLECTIVE IR","IR");
 if(info){textAt(8,31,"Emite luz infrarroja y",WHITE);textAt(8,46,"detecta su reflejo.",WHITE);textAt(8,63,"Color y distancia cambian",CYAN);textAt(8,78,"la senal: no mide cm.",CYAN);textAt(8,96,"Prueba blanco vs negro.",MUTED);}
 else {textAt(8,30,"REFLEJO  (ADC 0-4095)",MUTED);cv.setTextSize(3);cv.setTextColor(CYAN);cv.setCursor(8,47);cv.printf("%d",irRaw);cv.fillRoundRect(8,80,224,9,4,PANEL);cv.fillRoundRect(8,80,constrain(irRaw*224/4095,0,224),9,4,CYAN);textAt(8,98,irDigital?"SALIDA DIGITAL: 1":"SALIDA DIGITAL: 0",WHITE);}
 foot("I: explica   M: cambiar modulo");}
void draw(){if(locked)prepare();else if(mode==0){if(page)safety();else menu();}else if(mode==1)dual(page);else if(mode==2)env(page);else ir(page);cv.pushSprite(0,0);}
void activate(int m){
 mode=m;page=0;pinMode(REDPIN,INPUT);pinMode(BLUEPIN,INPUT);
 if(m==1){pinMode(REDPIN,INPUT);pinMode(BLUEPIN,INPUT); lastR=digitalRead(REDPIN)==LOW;lastB=digitalRead(BLUEPIN)==LOW;}
 if(m==3){pinMode(REDPIN,INPUT);analogReadResolution(12);analogSetPinAttenuation(BLUEPIN,ADC_11db);}
 if(m==2){Wire.setTimeOut(50); bool q=qmp.begin(&Wire,QMP6988_SLAVE_ADDRESS_L,2,1,100000U);bool s=sht.begin(&Wire,SHT3X_I2C_ADDR,2,1,100000U);envOK=q&&s;}
}
void setup(){
 auto cfg=M5.config();M5Cardputer.begin(cfg,true);M5Cardputer.Display.setRotation(1);M5Cardputer.Display.setBrightness(190);
 cv.createSprite(240,135);cv.setTextWrap(false);
 pinMode(REDPIN,INPUT);pinMode(BLUEPIN,INPUT);
 prefs.begin("sensorlab",false);int next=prefs.getInt("next",0);prefs.putInt("next",0);
 if(next>=1 && next<=3)activate(next);else mode=0;
 draw();
}
void loop(){
 M5Cardputer.update();
 if(M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()){
  auto keys=M5Cardputer.Keyboard.keysState();
  for(char c:keys.word){
   if(locked)continue;
   if((c=='i'||c=='I')){page=!page;draw();}
   if((c=='m'||c=='M') && mode){safePins();mode=0;page=0;draw();}
   if(mode==0 && (c=='1'||c=='2'||c=='3')){safePins();prefs.putInt("next",c-'0');locked=true;draw();}
  }
 }
 if(!locked && mode==1){bool r=digitalRead(REDPIN)==LOW,b=digitalRead(BLUEPIN)==LOW;if(millis()-pulseAt>35 && (r!=lastR||b!=lastB)){if(r&&!lastR)redCount++;if(b&&!lastB)blueCount++;lastR=r;lastB=b;pulseAt=millis();draw();}}
 if(!locked && mode==3 && millis()-lastRead>=80){lastRead=millis();irRaw=analogRead(BLUEPIN);irDigital=digitalRead(REDPIN);}
 if(!locked && mode==2 && envOK && millis()-lastRead>=1200){lastRead=millis();bool s=sht.update(),q=qmp.update();haveEnv=s&&q;if(haveEnv){temp=sht.cTemp;hum=sht.humidity;press=qmp.pressure/100.0f;}}
 if(!locked && mode && millis()-lastDraw>350){lastDraw=millis();draw();}
 delay(8);
}
