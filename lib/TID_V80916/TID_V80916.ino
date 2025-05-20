/*********************************************************************************************************************
Computador de bordo para veículos GM equipados com TID
Criado por Filipe Baptista - filbot@gmail.com
Forum: http://www.corsaclube.com.br/viewtopic.php?f=88&t=104366

2018.03.03.  
Led villogás kiszedve
calcula_intervalo alá bekerült  coisas_a_fazer_sempre(1); // a menü léptetés alatt is figyelje a bemeneteket
Kalibrációs menüben a kíirások rövidítve,kijelzés időzítések javítva

2018.04.01
Aut.Trip menü - 3 sec.-enként lépteti a 

2018.09.02
Reset után az autotrip TT időmérője : nullázás javítva

2018.09.16
a nem használt int-ek törlése


*********************************************************************************************************************/

#include "TID.h"
//Created by Daniel Crane
TID mydisplay(3,4,5); //SDA, SCL, MRQ

#include <EEPROM.h>


const float R1 = 300000.0; // Valor do resistor R1 (300K)
const float R2 = 100000.0; // Valor do resistor R2 (100K)
const float fatorDist = 1.1000;  //Altere este valor para regular o cálculo de distância
const float fatorVel = 1.15;    //Altere este valor para regular o cálculo do velocimetro
const float fatorCons = 12.5;    //Altere este valor para regular o cálculo de consumo
const byte CapacidadeTanque = 40;  //Altere este valor para definir a capacidade do tanque de combustível em litros

char texttemp[11], textfinal[11];
int ArrayNivelTanque[30] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
bool buttonState, flagLig, flagAut, flagTensao, flagUT, flagTemp, flagConsumo, flagDist,flagautrip, flagVel, flag_simbolo_colchete, InjState,
DistVelSignalState, flagInjActvStart, flagDistVelCount, RPMSignal, flagRPMSignal;
byte PTC,IndiceMenu, atividade, iteracao_txt, flagModoInjetor, flagModoVel, flagModoDist, flagModoTempoViagem, VelKmPorH, VelKmPorHBKG,
VelKmMax, VelKmMedia, VelKmMediaT, NivelTanque, NivelTanqueAVG, IndiceArrayNivTanque, contador_CarroDesl, contador_CarroLig, flagDadosSalvos,
NivelTanqueMax, NivelTanqueMin, RPMCount;
unsigned long milisec, milisec1, milisecC, milisec1C, milisecBotao, milisec1Botao, microsec, microsec1, microsec_UP, microsec1_UP,
vazaomlTotalEEPROM, DistTotalEEPROM, secsTotalEEPROM, secsT;
unsigned int secs, secsReset, secs1, secsC, secs1C, intervaloMiliSecs, intervaloMiliSecsC, intervaloMiliSecsBotao, intervaloSecs,
intervaloSecsB, intervaloSecsC, intervaloMicroSecs, RPM;
float Vout,Rout,averageP, averageT,beta,Rinf,TempK,TempC,wtmax,intervaloMicroSecsTotal, vmin, vmax, vin, vazaoml, vazaomlConsInst, vazaomlPorSeg, vazaomlPorSeg2, vazaomlTotalReset, vazaomlTotal,
vazaoLTotal, vazaoLPorHora, vazaoLPorHora2, ConsInstKmL, ConsMed, ConsMedTotal, Aut, DistVelCount, DistVelCountBKG, DistCount,
DistVelCountConsInst, DistVelConsInst, DistTotal, DistT, DistTKm, DistReset, DistVel, VelMPorSeg, fatorDistN, fatorVelN, fatorConsN;
int ATcount,ATtime,ATset,average,totalNivelTanque;



void setup()
{ 
 // Serial.begin(9600); 
  pinMode(2, OUTPUT);  //TID - AA/DIS
  pinMode(7, INPUT);  //Sinal do bico injetor
  pinMode(10, INPUT);  //botão 10 => botão 1 (ou botão Set da chave de limpador)
  pinMode(11, INPUT);  //botão 11 => botão 2 (ou botão Reset da chave de limpador)
  pinMode(13, OUTPUT);  //LED
  pinMode(18, INPUT);  //Sinal do sensor de velocidade - Pino A4

  vmin = 100.0;
  flagInjActvStart = 1;
  flagDistVelCount = 1;
  flagDadosSalvos = 1;
  NivelTanqueMin = 110;
  flagModoInjetor = 1; ///////////////////////////// fogyasztás menü belépéskor mivel induljon
  ATtime = 3;

  fatorConsN = fatorCons;
  fatorDistN = fatorDist;
  fatorVelN = fatorVel;
  
  carregar();
  liga_dados();
  texto_boas_vindas();
  mydisplay.clear_message();
  zera_intervalo();
}


void loop()
{
  buttonState = digitalRead(10);
  if (buttonState == LOW)
  {
    IndiceMenu++;
    liga_dados();
    botao_delay();
    mydisplay.clear_message();
    apaga_simbolo_colchete();
    zera_intervalo();
    iteracao_txt = 0;
   }

  switch (IndiceMenu)
  {
    case 0:
      desliga_dados();
      break;
    case 1:
      menu_autonomia();
      break;
    case 2:
      menu_tensao();
      flagAut = 0;
      break;
    case 3:
      menu_distancia();
      flagTensao = 0;
      break;
    case 4:
      menu_consumo();
      flagDist = 0;
      break;
    case 5:
      menu_uptime();
      flagConsumo = 0;
      break;
    case 6:
      menu_velocidade();
      flagUT = 0;
      break;
    case 7:
      menu_autrip();  /////////////////////////////////////// Auto Trip menu //////////////////////
      flagVel = 0;
      break;
    case 8:
      menu_opcoes();
      flagautrip = 0; ///////////////////////////////////////////////////////////////////////////
      break;
    case 9:
      IndiceMenu = 0;
      break;
    
    case 20:
      menu_opcoes_Reset();
      break;
    case 21:
      menu_opcoes_Gravar();
      break;
    case 22:
      menu_opcoes_Calibrar();
      break;
    case 23:
      menu_opcoes_Debug();
      break;
    case 24:
      menu_opcoes_Sair(8);
      break;
    case 25:
      IndiceMenu = 20;
      break;
      
    case 30:
      menu_opcoes_Calibr_Incr_fatorCons();
      break;
    case 31:
      menu_opcoes_Calibr_Decr_fatorCons();
      break;
    case 32:
      menu_opcoes_Calibr_Incr_fatorDist();
      break;
    case 33:
      menu_opcoes_Calibr_Decr_fatorDist();
      break;
    case 34:
      menu_opcoes_Calibr_Incr_fatorVel();
      break;
    case 35:
      menu_opcoes_Calibr_Decr_fatorVel();
      break;
    case 36:
      menu_opcoes_Sair(22);
      break;
    case 37:
      IndiceMenu = 30;
      break;      
      
    case 40:
      menu_opcoes_Debug_nivcombust();
      break;
    case 41:  
      menu_opcoes_Debug_RPM();
      break;
    case 42:
      menu_opcoes_Debug_flag();
      break;
    case 43:
      menu_version();
      break;
    case 44:
      menu_opcoes_Sair(23);
      break;
    case 45:
      IndiceMenu = 40;
      break;
      
    case 254:
      menu_ate_logo();
      break;
      
    default:
      IndiceMenu = 0;
      break;
  }


  coisas_a_fazer_sempre(1);

}


/////////////////////////////////////////////////
////////////// Funções auxiliares
/////////////////////////////////////////////////

void zera_intervalo() //Momento zero/inicial. Marca o instante que começa a contagem
{
  milisec = millis();
  secs = milisec/1000;
}

void calcula_intervalo() //Calcula o tempo que passou desde o momento 0
{
  milisec1 = millis();
  secs1 = milisec1/1000;
  intervaloMiliSecs = milisec1 - milisec;
  intervaloSecs = secs1 - secs;
  coisas_a_fazer_sempre(1); //////////////////////////////// a gombnyomások várakozása alatt meghívja a bemenetek figyelését
}

void zera_intervalo_C() //Segunda função de cálculo de tempo, para funções que exigem mais de uma contagem de tempo por vez
{
  milisecC = millis();
  secsC = milisecC/1000;
}

void calcula_intervalo_C()
{
  milisec1C = millis();
  secs1C = milisec1C/1000;
  intervaloMiliSecsC = milisec1C - milisecC;
  intervaloSecsC = secs1C - secsC;
}

void botao_delay()
{
  //mydisplay.display_symbol(11);
  atraso(280,1);
  //mydisplay.clear_symbol(11);
}

void atraso(int tempo, bool extras)
{
  milisecBotao = millis();
  while(intervaloMiliSecsBotao <= tempo)
  {
    milisec1Botao = millis();
    intervaloMiliSecsBotao = milisec1Botao - milisecBotao;
    coisas_a_fazer_sempre(extras);
  }
  intervaloMiliSecsBotao = 0;
}

bool intervaloMiliSecsCheck (byte movimentos)
{
  if ( (intervaloMiliSecs > (700*(iteracao_txt+1)+300)) && (intervaloMiliSecs < (700*(iteracao_txt+1)+370)) )//400
  {
    iteracao_txt++;
    if (iteracao_txt >= movimentos)
      iteracao_txt = 0;
    return true;
  }
  else
    return false;
}

void LED_atividade()
{
  atividade = ((millis())/1000) % 2;
  if (atividade == 0)
    digitalWrite(13,HIGH);
  else
    digitalWrite(13,LOW);
}


/////////////////////////////////////////////////
////////////// Funções de controle
/////////////////////////////////////////////////

void desliga_dados() //Desliga o envio de dados para o TID, fazendo a data reaparecer
{
  if (flagLig == 1)
  {
    digitalWrite(2,LOW);
    flagLig = 0;
  }
}

void liga_dados() //Liga o envio de dados para o TID, apagando a data
{
  if (flagLig == 0) 
  {
    zera_intervalo();
    digitalWrite(2,HIGH);
    flagLig = 1;
    while(intervaloMiliSecs <= 300)
    {
      calcula_intervalo();
      coisas_a_fazer_sempre(0);
    }
  }
}

void exibe_simbolo_colchete()
{
  if (flag_simbolo_colchete == 0)
  {
    mydisplay.display_symbol(9);
    flag_simbolo_colchete = 1;
  }
}

void apaga_simbolo_colchete()
{
  if (flag_simbolo_colchete == 1)
  {
    mydisplay.clear_symbol(9);
    flag_simbolo_colchete = 0;
  }
}

void carregar()
{
  mydisplay.display_symbol(5);
  //Carregando o consumo total
  for (byte j=0; j<6; j++)
  {
    texttemp[j] = EEPROM.read(j);
  }
  vazaomlTotalEEPROM = atof(texttemp); //string to float
  vazaomlTotalEEPROM = vazaomlTotalEEPROM - 100000;
  
  memset(texttemp, 0, sizeof(texttemp)); //Limpa o array
  
  //Carregando a distância total
  for (byte j=6; j<13; j++)
  {
    texttemp[j-6] = EEPROM.read(j);
  }  
  DistTotalEEPROM = atof(texttemp); //string to float
  DistTotalEEPROM = DistTotalEEPROM - 1000000;
  
  IndiceMenu = EEPROM.read(13);
  
  memset(texttemp, 0, sizeof(texttemp)); //Limpa o array
  
  //Carregando o tempo total
  for (byte j=14; j<21; j++)
  {
    texttemp[j-14] = EEPROM.read(j);
  }
  secsTotalEEPROM = atof(texttemp);
  secsTotalEEPROM = secsTotalEEPROM - 1000000;

  unsigned int segundos = secsTotalEEPROM;
  unsigned int minutos = segundos/60; //convert seconds to minutes
  unsigned int horas = minutos/60; //convert minutes to hours
  segundos = segundos - (minutos * 60);
  minutos = minutos - (horas * 60);
  mydisplay.clear_symbol(5);
}

void gravar()
{
  vazaomlTotal = (intervaloMicroSecsTotal/1000000.0) * fatorCons;
  dtostrf((vazaomlTotalEEPROM + vazaomlTotal - vazaomlTotalReset + 100000),6,0,texttemp); //Transformando o float em string
  //EEPROM.put(0, texttemp); <---- não usar, EEPROM.put tem bug
  for (byte j=0; j<6; j++)
  {
    EEPROM.update(j, texttemp[j]);
  }
  
  DistT = (DistCount/16) * fatorDist;
  dtostrf((DistTotalEEPROM + DistT - DistReset + 1000000),7,0,texttemp);  //Transformando o float em string
  //EEPROM.put(6, texttemp); <---- não usar, EEPROM.put tem bug
  for (byte j=6; j<13; j++)
  {
    EEPROM.update(j, texttemp[j-6]);
  }  
  
  EEPROM.update(13, IndiceMenu);
  
  secs = (millis())/1000;
  secsT = secsTotalEEPROM + secs - secsReset;
  dtostrf((secsT + 1000000),7,0,texttemp);
  //EEPROM.put(14, texttemp); <---- não usar, EEPROM.put tem bug
  for (byte j=14; j<21; j++)
  {
    EEPROM.update(j, texttemp[j-14]);
  }
  
  if (flagLig == 1)
    mydisplay.display_symbol(5);
}

void reset_trip()
{
  //Reset da vazao - p/ consumo médio
  vazaomlTotal = (intervaloMicroSecsTotal/1000000.0) * fatorCons;
  vazaomlTotalReset = vazaomlTotal;
  vazaomlTotalEEPROM = 0;
  //EEPROM.put(0, "100000"); <---- não usar, EEPROM.put tem bug
  dtostrf((100000),6,0,texttemp);
  for (byte j=0; j<6; j++)
  {
    EEPROM.update(j, texttemp[j]);
  }
  
  //Reset da distancia
  DistT = (DistCount/16) * fatorDist;
  DistReset = DistT;
  DistTotalEEPROM = 0;
  //EEPROM.put(6, "1000000"); <---- não usar, EEPROM.put tem bug
  dtostrf((1000000),7,0,texttemp);  //Transformando o float em string
  for (byte j=6; j<13; j++)
  {
    EEPROM.update(j, texttemp[j-6]);
  }
  
  EEPROM.update(13, 0);
  
  //Reset do tempo - p/ velocidade média
  secs = (millis())/1000;
  secsReset = secs;
  secsTotalEEPROM = 0;
  //EEPROM.put(14, "1000000"); <---- não usar, EEPROM.put tem bug
  dtostrf((1000000),7,0,texttemp);
  for (byte j=14; j<21; j++)
  {
    EEPROM.update(j, texttemp[j-14]);
  }
  
  mydisplay.display_message(F("  Reset OK"),255);
  atraso(1500,1);
  zera_intervalo();
}

/////////////////////////////////////////////////
////////////// Outras funções
/////////////////////////////////////////////////

void coisas_a_fazer_sempre(bool extras)
{
  if (extras == 1)
  {
    /*
      Colocar dentro deste if a chamada de funções extras ao computador de bordo
      que vocês venham a desenvolver que precisem rodar em background e que em
      algum momento use a função "atraso".
      
      Dentro dessas funções extras que vocês criarem, use a função atraso com o 
      segundo parâmetro igual a 0, da seguinte forma "atraso(TempoEmMiliSegundos,0);"
      
      Esta ação é obrigatória para evitar um loop infinito.
      
      Caso a função extra não use a função "atraso" mas precise trabalhar em background
      (independente do menu selecionado no TID), faça a chamada para ela de fora
      deste if.
      
      Caso a função extra não precise rodar em background, pode chamá-la de dentro
      do "switch (IndiceMenu)" criando um menu específico para sua função.
    */

  }  
  /* Coloque logo abaixo a chamada de funções extras que não vão usar a função "atraso" 
     mas que precisam trabalhar em background */
  //LED_atividade();

  
  //Leitura do min e max da bateria
  int sensorValue = analogRead(A0);
  float voltage = sensorValue * (5.0 / 1023.0);
  vin = voltage / (R2/(R1+R2));
  if (vin < vmin)
  {
    vmin = vin;
  }
  if (vin > vmax)
  {
    vmax = vin;
  }
  //Leitura do tempo do injetor  
  InjState = digitalRead(7);
  if (InjState == LOW)
  {
    if (flagInjActvStart == 1)
    {
      microsec = micros(); //zera intervalo
      flagInjActvStart = 0;
    }
  }
  else
  {
    if (flagInjActvStart == 0)
    {
      microsec1 = micros(); //calcula intervalo
      intervaloMicroSecs = microsec1 - microsec; //calcula intervalo
      flagInjActvStart = 1;
      intervaloMicroSecsTotal = intervaloMicroSecsTotal + intervaloMicroSecs;
      vazaoml = (intervaloMicroSecs/1000000.0) * fatorCons; // us/u * ml/s
      vazaomlPorSeg = vazaomlPorSeg + vazaoml;
      vazaomlPorSeg2 = vazaomlPorSeg2 + vazaoml;
      vazaomlConsInst = vazaomlConsInst + vazaoml;
    }
  }

  //Leitura do sensor de velocidade/distancia
  DistVelSignalState = digitalRead(18);
  if (DistVelSignalState == HIGH)
  {
    if (flagDistVelCount == 1)
    {
      DistVelCount = DistVelCount + 1;
      DistVelCountBKG = DistVelCountBKG + 1;
      DistCount = DistCount + 1;
      DistVelCountConsInst = DistVelCountConsInst + 1;
      flagDistVelCount = 0;
    }
  }
  else
  {
    if (flagDistVelCount == 0)
    {
      flagDistVelCount = 1;
    }
  }

  //Calculo de Vel, vel max e Consumo em L/h
  calcula_intervalo_C();
  if (intervaloSecsC >= 1)
  {
    zera_intervalo_C();
    //Coloque abaixo a chamada de funções extras que precisam rodar a cada 1 segundo em background sem precisar exibir nada no TID
    
    
    //Calculo vel e vel max
    VelKmPorHBKG = (DistVelCountBKG/16) * fatorVel * 3.6;
    DistVelCountBKG = 0.0;
    if (VelKmPorHBKG > VelKmMax)
    {
      VelKmMax = VelKmPorHBKG;
    }

    //Calculo consumo L/h
    vazaoLPorHora2 = vazaomlPorSeg2/1000.0*3600.0;
    vazaomlPorSeg2 = 0.0;

    //Detecta quando o carro está desligado
	  if (VelKmPorHBKG < 2 && vazaoLPorHora2 < 0.2 && flagDadosSalvos == 0)
	  {
	    if (contador_CarroDesl <= 7)
	      contador_CarroDesl++;
      if (contador_CarroDesl == 3) //checa se o carro ficou desligado por mais de X segundos
      {
        gravar();
        contador_CarroLig = 0;
        IndiceMenu = 254;
        if (flagLig == 0)
          liga_dados();
        zera_intervalo();
      }
      else if (contador_CarroDesl == 5)
      {
        flagDadosSalvos = 1;
        if (flagLig == 1)
          mydisplay.clear_symbol(5);
      }
	  }
	  else if (vazaoLPorHora2 > 0.6)
	  {
	    if (contador_CarroLig <= 5)
	    {
	      contador_CarroLig++;
	    }
      else if (contador_CarroLig > 5) //checa se o carro ficou ligado por mais de 5 segundos
      {
        flagDadosSalvos = 0;
        contador_CarroDesl = 0;
        if (IndiceMenu == 254)
        {
          IndiceMenu = EEPROM.read(13);
          zerar_flags_menu();
        }
      }
	  }
  }
}

void zerar_flags_menu()
{
  flagAut = 0;
  flagTensao = 0;
  flagDist = 0;
  flagConsumo = 0;
  flagUT = 0;
  flagVel = 0;
  flagautrip = 0;
}


/////////////////////////////////////////////////
////////////// Funções de exibição
/////////////////////////////////////////////////

void texto_boas_vindas()
{
  mydisplay.clear_message();
  delay(100);
  for (byte i=0;i<11;i++) {
    mydisplay.display_message(F("          .Welcome!."),20);
  delay(100);
  }
  mydisplay.display_message(F(".Welcome!."),20);
  delay(2000);
  for (byte i=11;i>0;i--) {
  mydisplay.display_message(F(".Welcome!.          "),20);
  delay(100);
  }
}

/* Descomente este trecho caso queira usar o "boas-vindas" animado e comente a função acima
void texto_boas_vindas()
{
  byte i;
  zera_intervalo();
  for (i=0;i<11;i++)
  {
    mydisplay.display_message(F("           Bem-vindo!"),20);
    mydisplay.display_symbol(i);
    coisas_a_fazer_sempre();
  }
  mydisplay.clear_message();
  coisas_a_fazer_sempre();
  mydisplay.display_message(F("Bem-vindo!"),20);
  while(intervaloMiliSecs <= 3000)
  {
    calcula_intervalo();
    coisas_a_fazer_sempre();
  }
  for (i=11;i>0;i--)
  {
    mydisplay.display_message(F("Bem-vindo!           "),255);
    mydisplay.clear_symbol(i);
    coisas_a_fazer_sempre();
  }
}*/

void menu_tensao()
{
  char texttemp2[11];
  if (flagTensao == 0)
  {
    calcula_intervalo();
    if (intervaloMiliSecs < 100)
      mydisplay.display_message(F("Battery   "),255);
    if (intervaloSecs >= 3)
    {
      flagTensao = 1;
      exibe_simbolo_colchete();
    }
  }
  else if (flagTensao == 1)
  {
    buttonState = digitalRead(11);
    if (buttonState == LOW)
    {
      botao_delay();
      apaga_simbolo_colchete();
      dtostrf(vmin,3,1,texttemp);
      dtostrf(vmax,3,1,texttemp2);
      sprintf(textfinal, " %s/%s", texttemp,texttemp2);
      mydisplay.display_message(textfinal,255);
      atraso(2500,1);
      exibe_simbolo_colchete();
    }
    calcula_intervalo();
    if (intervaloSecs >= 2)
    {
      dtostrf(vin,4,2,texttemp);     //Converte o float em string - dtostrf(variável a ser convertida, minStringWidthIncDecimalPoint, casas decimais, variável de saída)
      if (vin < 10)
      {
        sprintf(textfinal, "Ba  %s V", texttemp);
      }
      else
      {
        sprintf(textfinal, "Ba %s V", texttemp); //Junta os textos em uma só variável
      }
      mydisplay.display_message(textfinal,255);
      zera_intervalo();
    }
  }
}


void menu_uptime()
{
  if (flagUT == 0)
  {
    calcula_intervalo();
    if (intervaloMiliSecs < 100) //500
      mydisplay.display_message(F("Traveltime"),255);
  //  else if (intervaloMiliSecsCheck(2) == true)
  //    mydisplay.display_message(F("Travel time "),2);

    if (intervaloSecs >= 4)
    {
      flagUT = 1;
      exibe_simbolo_colchete();
    }
  }
  else if (flagUT == 1)
  {
    
    buttonState = digitalRead(11);
    if (buttonState == LOW)
    {
      flagModoTempoViagem = flagModoTempoViagem + 1;
      if (flagModoTempoViagem == 2)
        flagModoTempoViagem = 0;
      botao_delay();
      secs = 0;
    }
    
    calcula_intervalo();
    if (intervaloMiliSecs >= 500)
    {
	    int hours=0;
      int mins=0;
      secs = ((millis())/1000) - secsReset; //convert milliseconds to seconds
      if (flagModoTempoViagem == 0)
      {
        mins=secs/60; //convert seconds to minutes
        hours=mins/60; //convert minutes to hours
        secs=secs-(mins*60); //subtract the coverted seconds to minutes in order to display 59 secs max
        mins=mins-(hours*60); //subtract the coverted minutes to hours in order to display 59 minutes max
        if (secs < 10 && mins < 10)
          sprintf(textfinal,"Tt %i:0%i:0%i",hours,mins,secs);
        else if (secs >= 10 && mins < 10)
          sprintf(textfinal,"Tt %i:0%i:%i",hours,mins,secs);
        else if (secs < 10 && mins >= 10)
          sprintf(textfinal,"Tt %i:%i:0%i",hours,mins,secs);
        else if (secs >= 10 && mins >= 10)
          sprintf(textfinal,"Tt %i:%i:%i",hours,mins,secs);
      }
      else if (flagModoTempoViagem == 1)
      {
        secsT = secsTotalEEPROM + secs;
        mins = secsT/60; //convert seconds to minutes
        hours = mins/60; //convert minutes to hours
        secsT = secsT - (long(mins) * 60);
        mins = mins - (hours * 60);
        if (secsT < 10 && mins < 10 && hours < 10)
          sprintf(textfinal,"TT %i:0%i:0%i",hours,mins,secsT);
        else if (secsT >= 10 && mins < 10 && hours < 10) 
          sprintf(textfinal,"TT %i:0%i:%i",hours,mins,secsT);
        else if (secsT < 10 && mins >= 10 && hours < 10)
          sprintf(textfinal,"TT %i:%i:0%i",hours,mins,secsT);
        else if (secsT >= 10 && mins >= 10 && hours < 10)
          sprintf(textfinal,"TT %i:%i:%i",hours,mins,secsT);
        else if (secsT < 10 && mins < 10 && hours >= 10)
          sprintf(textfinal,"TT%i:0%i:0%i",hours,mins,secsT);
        else if (secsT >= 10 && mins < 10 && hours >= 10) 
          sprintf(textfinal,"TT%i:0%i:%i",hours,mins,secsT);
        else if (secsT < 10 && mins >= 10 && hours >= 10)
          sprintf(textfinal,"TT%i:%i:0%i",hours,mins,secsT);
        else if (secsT >= 10 && mins >= 10 && hours >= 10)
          sprintf(textfinal,"TT%i:%i:%i",hours,mins,secsT);
      }
      mydisplay.display_message(textfinal,255);
	    zera_intervalo();
	  }
  }
}



void menu_consumo()
{
 if (flagConsumo == 0)
  {
    calcula_intervalo();
    if (intervaloMiliSecs < 100)
      mydisplay.display_message(F("Cons.ption"),255);
    else if (intervaloSecs >= 3)
    {
      flagConsumo = 1;
      vazaomlPorSeg = 0;
      vazaomlConsInst = 0;
      exibe_simbolo_colchete();
    }
  }
  else if (flagConsumo == 1)
  {

    buttonState = digitalRead(11);
    if (buttonState == LOW)
    {
      flagModoInjetor++;
      if (flagModoInjetor == 6)
        flagModoInjetor = 0;
      botao_delay();
      vazaomlPorSeg = 0;
      secs = 0;
    }

    calcula_intervalo();
    if (intervaloSecs >= 1)
    {
      vazaoLPorHora = vazaomlPorSeg/1000.0*3600.0;
      vazaomlPorSeg = 0.0;

      //Calcular consumo instantâneo
      DistVelConsInst = (DistVelCountConsInst/16) * fatorDist;
      ConsInstKmL = (DistVelConsInst/1000) / (vazaomlConsInst/1000);
      DistVelCountConsInst = 0.0;
      vazaomlConsInst = 0.0;

      //Calcular consumo medio
      vazaomlTotal = (intervaloMicroSecsTotal/1000000.0) * fatorCons;
      DistT = (DistCount/16) * fatorDist;
      ConsMed = ((DistT - DistReset)/1000)/((vazaomlTotal - vazaomlTotalReset)/1000);
 
      //Calcular consumo da viagem
      vazaoLTotal = (vazaomlTotal - vazaomlTotalReset)/1000.0;

      if (flagModoInjetor == 0)  //Consumo instantaneo
      {
        if (vazaoLPorHora < 0.1)
        {
          if (VelKmPorHBKG != 0)
            mydisplay.display_message(F("Ci --- kmL"),255);
          else
            mydisplay.display_message(F("Ci --- ---"),255);
        }
        else if (ConsInstKmL < 1.4)
        {
          if (vazaoLPorHora < 10.00)
          {
            dtostrf((((byte)(vazaoLPorHora*10))/10.0),2,1,texttemp);
          }
          else
          {
            dtostrf(vazaoLPorHora,3,0,texttemp);
          }
          sprintf(textfinal,"Ci %s L/h",texttemp);
          mydisplay.display_message(textfinal,255);
        }
        else
        {
          if (ConsInstKmL < 10.00)
          {
            dtostrf((((byte)(ConsInstKmL*10))/10.0),2,1,texttemp);
          }
          else if (ConsInstKmL > 99.00)
          {
            ConsInstKmL = 99;
            dtostrf(ConsInstKmL,3,0,texttemp);
          }
          else
          {
            dtostrf(ConsInstKmL,3,0,texttemp);
          }
          sprintf(textfinal,"Ci %s kmL",texttemp);
          mydisplay.display_message(textfinal,255);
        }
      }
      else if (flagModoInjetor == 1) //Consumo medio
      { 
        averageP = 100 / ConsMed;
        if ((averageP >= 0.0) && (averageP < 100.0)) /////////////////////////
        { 
          if (averageP < 10.0)
          {
            dtostrf((((byte)(averageP*10))/10.0),2,1,texttemp);
          } 
          else
          {
            dtostrf(averageP,3,0,texttemp);
          }
         }
        else
         {
          averageP = 0.0;
          dtostrf(averageP,2,1,texttemp);
        }
         if (averageP > 12.0)
          {
           averageT = 100/(DistTotal/((vazaomlTotalEEPROM + (vazaomlTotal - vazaomlTotalReset))/1000.0));
          // averageP = averageT; 
           if (averageT < 10.00)
          {
            dtostrf((((byte)(averageT*10))/10.0),2,1,texttemp);
          }
          }
        
        sprintf(textfinal,"Ca %s L/1",texttemp);
        mydisplay.display_message(textfinal,255);
      }
      else if (flagModoInjetor == 2) //Consumo litros por hora
      {
        if (vazaoLPorHora < 10.00)
        {
          dtostrf((((byte)(vazaoLPorHora*10))/10.0),2,1,texttemp);
        }
        else
        {
          dtostrf(vazaoLPorHora,3,0,texttemp);
        }
        sprintf(textfinal,"Ch %s L/h",texttemp);
        mydisplay.display_message(textfinal,255);
      }
      else if (flagModoInjetor == 3) //Consumo em litros
      {
        dtostrf(vazaoLTotal,5,2,texttemp);
        sprintf(textfinal,"Cp %s L",texttemp);
        mydisplay.display_message(textfinal,255);
      }
      else if (flagModoInjetor == 4) //Consumo médio total
      {
        DistT = (DistCount/16) * fatorDist;
        DistTotal = (DistTotalEEPROM + (DistT - DistReset))/1000.0;
        ConsMedTotal = DistTotal/((vazaomlTotalEEPROM + (vazaomlTotal - vazaomlTotalReset))/1000.0);        
        averageT = 100/(DistTotal/((vazaomlTotalEEPROM + (vazaomlTotal - vazaomlTotalReset))/1000.0)); 
        if ((ConsMedTotal >= 0.0) && (ConsMedTotal < 100.0))
        {
          if (averageT < 10.0)
          {
            dtostrf((((byte)(averageT*10))/10.0),2,1,texttemp);
          }
          else
          {
            dtostrf(averageT,3,0,texttemp);
          }
        }
        else
        {
          ConsMedTotal = 0.0;
          dtostrf(averageT,2,1,texttemp);
        }
        sprintf(textfinal,"CA %s L/1",texttemp);
        mydisplay.display_message(textfinal,255);
      }
      else if (flagModoInjetor == 5) //Consumo total
      {
        dtostrf(((vazaomlTotalEEPROM + (vazaomlTotal - vazaomlTotalReset))/1000.0),5,2,texttemp);
        sprintf(textfinal,"CT %s L",texttemp);
        mydisplay.display_message(textfinal,255);
      }
      zera_intervalo();
    }
  } 
}


void menu_distancia()
{
  if (flagDist == 0)
  {
    calcula_intervalo();
    if (intervaloMiliSecs < 100)
      mydisplay.display_message(F("Distance  "),255);
    if (intervaloSecs >= 3)
    {
      flagDist = 1;
      exibe_simbolo_colchete();
    }
  }
  else if (flagDist == 1)
  {
    
    buttonState = digitalRead(11);
    if (buttonState == LOW)
    {
      flagModoDist = flagModoDist + 1;
      if (flagModoDist == 2)
        flagModoDist = 0;
      botao_delay();
      secs = 0;
    }

    calcula_intervalo();
    if (intervaloSecs >= 2)
    {
      zera_intervalo();
      if (flagModoDist == 0)
      {
        DistT = (DistCount/16) * fatorDist;
        DistTKm = (DistT - DistReset)/1000;
        if (DistTKm < 100.00)
        {
          dtostrf((((int)(DistTKm*10))/10.0),4,1,texttemp);
        }
        else
        {
          dtostrf(DistTKm,4,0,texttemp);
        }
        sprintf(textfinal,"Di %s km",texttemp);
      }
      else if (flagModoDist == 1)
      {
        DistT = (DistCount/16) * fatorDist;
        DistTotal = (DistTotalEEPROM + (DistT - DistReset))/1000.0;
        if (DistTotal < 100.00)
        {
          dtostrf((((int)(DistTotal*10))/10.0),4,1,texttemp);
        }
        else
        {
          dtostrf(DistTotal,4,0,texttemp);
        }
        sprintf(textfinal,"DT %s km",texttemp);
      }
      mydisplay.display_message(textfinal,255);
    }
  }
}


void menu_velocidade()
{
  if (flagVel == 0)
  {
    calcula_intervalo();
    if (intervaloMiliSecs < 100)
      mydisplay.display_message(F("Speed     "),255);
    if (intervaloSecs >= 3)
    {
      flagVel = 1;
      DistVelCount = 0.0;
      exibe_simbolo_colchete();
    }
  }
  else if (flagVel == 1)
  {
  
    buttonState = digitalRead(11);
    if (buttonState == LOW)
    {
      flagModoVel = flagModoVel + 1;
      if (flagModoVel == 4)
        flagModoVel = 0;
      botao_delay();
      DistVelCount = 0.0;
      secs = 0;
    }

    calcula_intervalo();
    if (intervaloSecs >= 1)
    {
      //unsigned int intervV = intervaloSecs;
      zera_intervalo();
      if (flagModoVel == 0)
      {
        DistVel = (DistVelCount/16) * fatorVel;
        VelKmPorH = DistVel * 3.6;                   // m/s para km/h
        
        dtostrf(VelKmPorH,3,0,texttemp);
        sprintf(textfinal,"Sp %s kmh",texttemp);
        mydisplay.display_message(textfinal,255);
      }
      else if (flagModoVel == 1)
      {
        DistT = (DistCount/16) * fatorDist;
        secs = (millis())/1000;
        VelKmMedia = ((DistT - DistReset)/(secs - secsReset)) * 3.6;
        if ((VelKmMedia < 0) || (VelKmMedia > 200))
          VelKmMedia = 0;
        dtostrf(VelKmMedia,3,0,texttemp);
        sprintf(textfinal,"Sa %s kmh",texttemp);
        mydisplay.display_message(textfinal,255);
      }
      else if (flagModoVel == 2)
      {
        //VM = DT/TT
        DistT = (DistCount/16) * fatorDist;
        DistTotal = (DistTotalEEPROM + (DistT - DistReset))/1000.0;
        
        secs = (millis())/1000; //convert milliseconds to seconds
        secsT = secsTotalEEPROM + secs - secsReset;
        
        //VelKmMediaT = (DistTotal/(secsT/3600));
        VelKmMediaT = (DistTotal/((float)secsT/3600.0));
        
        if ( !((VelKmMediaT >= 0) && (VelKmMediaT < 200)) )
          VelKmMediaT = 0;
        
        dtostrf(VelKmMediaT,3,0,texttemp);
        sprintf(textfinal,"VM %s kmh",texttemp);
        mydisplay.display_message(textfinal,255);
      }
      else if (flagModoVel == 3)
      {
        dtostrf(VelKmMax,3,0,texttemp);
        sprintf(textfinal,"Mx %s kmh",texttemp);
        mydisplay.display_message(textfinal,255);
      } 
      DistVelCount = 0.0;
    }
  }
}


void menu_autonomia()
{
  if (flagAut == 0)
  {
    calcula_intervalo();
    if (intervaloMiliSecs < 100)
      mydisplay.display_message(F("Autonomia "),255);
    if (intervaloSecs >= 3)
    {
      flagAut = 1;
      zera_intervalo();
    }
  }
  else
  {
    calcula_intervalo();
    if (intervaloSecs >= 1)
    {
      zera_intervalo();

      if (flagDadosSalvos == 0)
      {
        //Obtendo a quantidade de combustível no tanque
        int sensorValue = analogRead(A7);
        if (sensorValue < 210)
          NivelTanque = 100;
        else if (sensorValue >= 210 && sensorValue < 315)
          NivelTanque = (-0.0024 * sensorValue + 1.505) * 100;
        else if (sensorValue >= 315 && sensorValue < 465)
          NivelTanque = (-0.00159 * sensorValue + 1.2455) * 100;
        else if (sensorValue >= 465 && sensorValue < 630)
          NivelTanque = (-0.0015 * sensorValue + 1.2) * 100;
        else if (sensorValue >= 630 && sensorValue < 765)
          NivelTanque = (-0.0019 * sensorValue + 1.4535) * 100;
        else if (sensorValue >= 765)
          NivelTanque = 0;
      
        // subtrai a leitura do ciclo anterior
        totalNivelTanque = totalNivelTanque - ArrayNivelTanque[IndiceArrayNivTanque];
        // lê o nivel atual
        ArrayNivelTanque[IndiceArrayNivTanque] = NivelTanque;
        //
        if (NivelTanqueMax < NivelTanque)
          NivelTanqueMax = NivelTanque;
        if (NivelTanqueMin > NivelTanque)
          NivelTanqueMin = NivelTanque;
        
        // adiciona a leitura no total
        totalNivelTanque = totalNivelTanque + ArrayNivelTanque[IndiceArrayNivTanque];
        //avança posição na array
        IndiceArrayNivTanque++;
      
        if (IndiceArrayNivTanque >= 30)
        {
          IndiceArrayNivTanque = 0;
          //calcula a média
          NivelTanqueAVG = (totalNivelTanque - (NivelTanqueMax + NivelTanqueMin)) / (30-2);
          
          NivelTanqueMax = 0;
          NivelTanqueMin = 110;
          
          //Obtendo o consumo médio total
          DistT = (DistCount/16) * fatorDist;
          DistTotal = (DistTotalEEPROM + (DistT - DistReset))/1000.0;
          ConsMedTotal = DistTotal/((vazaomlTotalEEPROM + (vazaomlTotal - vazaomlTotalReset))/1000.0);
          if ( !((ConsMedTotal >= 0.0) && (ConsMedTotal < 100.0)) )
          {
            ConsMedTotal = 0.0;
          }
      
          //Autonomia = CombustívelNoTanque * ConsMedTotal
          Aut = (CapacidadeTanque * NivelTanqueAVG / 100) * ConsMedTotal;
        }
      }

      //SE autonomia for MENOR que 10.0 OU motor estiver desligado
      if ( (Aut < 10.0) || (flagDadosSalvos == 1) )
      {
        sprintf(textfinal,"Au  --- km");
      }
      else
      {
        dtostrf((((int)(Aut*10))/10.0),4,0,texttemp);
        sprintf(textfinal,"Au %s km",texttemp);
      }
      mydisplay.display_message(textfinal,255);
    }
  }
}


void menu_ate_logo()
{
  calcula_intervalo();
  if (intervaloMiliSecs < 100)
    mydisplay.display_message(F(">GoodBye!<"),255);
  if (intervaloSecs >= 3)
    zera_intervalo();
}


void menu_opcoes()
{
  byte i;
  calcula_intervalo();
  if (intervaloMiliSecs < 100)
    mydisplay.display_message(F("Options ->"),255);
  if (intervaloSecs >= 3)
    zera_intervalo();

  buttonState = digitalRead(11);
  if (buttonState == LOW)
  {
    IndiceMenu = 20;
    botao_delay();
    /* Descomente este trecho para habilitar a animação da palavra "Opções" percorrendo a tela - Somente para TID mais novo com tela inclinada
    for (i=0;i<10;i++)
    {
      atraso(60,1);
      mydisplay.display_message(F("Opcoes   >         "),20);
    }
    mydisplay.display_message(F(">         "),255);
    */
    zera_intervalo();
  }  
}


void menu_opcoes_Reset()
{
  calcula_intervalo();
  if (intervaloMiliSecs < 100)
    mydisplay.display_message(F(">Reset    "),255);
  else if (intervaloSecs >= 3)
    zera_intervalo();

  buttonState = digitalRead(11);
  if (buttonState == LOW)
  {
    botao_delay();
    mydisplay.display_message(F("     Back?"),255);
    atraso(700,1);
    milisecBotao = millis();
    while(intervaloMiliSecsBotao <= 3000)
    {
      milisec1Botao = millis();
      intervaloMiliSecsBotao = milisec1Botao - milisecBotao;
      coisas_a_fazer_sempre(1);
      buttonState = digitalRead(11);
      if (buttonState == LOW)
      {
        reset_trip();
        intervaloMiliSecsBotao = 3100;
      }
    }
    intervaloMiliSecsBotao = 0;
    zera_intervalo();
  }
}


void menu_opcoes_Gravar()
{
  calcula_intervalo();
  if (intervaloMiliSecs < 100)
    mydisplay.display_message(F(">Save     "),255);
  else if (intervaloSecs >= 3)
    zera_intervalo();  

  buttonState = digitalRead(11);
  if (buttonState == LOW)
  {
    gravar();
    mydisplay.display_message(F("       OK!"),255);
    atraso(1500,1);
    mydisplay.clear_symbol(5);
    zera_intervalo();
  }
}


void menu_opcoes_Calibrar()
{
  calcula_intervalo();
  if (intervaloMiliSecs < 100)
    mydisplay.display_message(F(">Calibrate"),255);
  else if (intervaloSecs >= 3)
    zera_intervalo();  

  buttonState = digitalRead(11);
  if (buttonState == LOW)
  {
    IndiceMenu = 30;
    botao_delay();
    mydisplay.clear_message();
    zera_intervalo();
  }
}


void menu_opcoes_Debug()
{
  calcula_intervalo();
  if (intervaloMiliSecs < 100)
    mydisplay.display_message(F(">Debug    "),255);
  if (intervaloSecs >= 3)
    zera_intervalo();

  buttonState = digitalRead(11);
  if (buttonState == LOW)
  {
    IndiceMenu = 40;
    botao_delay();
  }
}


void menu_opcoes_Debug_nivcombust()
{
  calcula_intervalo();
  if (intervaloMiliSecs < 100)
  {
    int sensorValue = analogRead(A7);
    if (sensorValue < 210)
      NivelTanque = 100;
    else if (sensorValue >= 210 && sensorValue < 315)
      NivelTanque = (-0.0024 * sensorValue + 1.505) * 100;
    else if (sensorValue >= 315 && sensorValue < 465)
      NivelTanque = (-0.00159 * sensorValue + 1.2455) * 100;
    else if (sensorValue >= 465 && sensorValue < 630)
      NivelTanque = (-0.0015 * sensorValue + 1.2) * 100;
    else if (sensorValue >= 630 && sensorValue < 765)
      NivelTanque = (-0.0019 * sensorValue + 1.4535) * 100;
    else if (sensorValue >= 765)
      NivelTanque = 0;
    
    dtostrf(NivelTanque,3,0,texttemp);
    sprintf(textfinal,"NiTnq %s%%",texttemp);
    mydisplay.display_message(textfinal,255);
  }
  if (intervaloSecs >= 3)
    zera_intervalo();
}


void menu_opcoes_Debug_flag()
{
  calcula_intervalo();
  if (intervaloMiliSecs < 50)
  {
    //dtostrf(flagDadosSalvos,1,0,texttemp);
    sprintf(textfinal,"FlagDS   %i",flagDadosSalvos);
    mydisplay.display_message(textfinal,255);
  }
  if (intervaloSecs >= 1)
    zera_intervalo();
}



void menu_opcoes_Debug_RPM() //Estimativa de RPM com base no pulso dos bicos injetores - não é leitura real de RPM!
{
  calcula_intervalo();
  if (intervaloMiliSecs < 50)
    mydisplay.display_sub_message(F("RPMe"),255,0,3);

  RPMSignal = digitalRead(7);
  if (RPMSignal == LOW)
  {
    if (flagRPMSignal == 1)
    {
      RPMCount++;
      flagRPMSignal = 0;
    }
  }
  else
  {
    if (flagRPMSignal == 0)
      flagRPMSignal = 1;
  }

  calcula_intervalo();
  if (intervaloMiliSecs >= 1200) //1000
  {
    RPM = (RPMCount - 1) * 50 * 2.3; // * 60
    if (RPMCount == 0)
      RPM = 0;

    dtostrf(RPM,4,0,texttemp);
    sprintf(textfinal,"RPMe  %s",texttemp);
    mydisplay.display_message(textfinal,255);

    RPMCount = 0;
    zera_intervalo();
  }
}


void menu_opcoes_Calibr_Incr_fatorCons()
{
  float totalvz0;
  float interv;
  float totalvz1;
  
  calcula_intervalo();
  if (intervaloMiliSecs < 100) 
  mydisplay.display_message(F("Fuel up.05"),255);
 else if (intervaloSecs >= 3 && intervaloSecs <= 6 )
  {
    dtostrf(fatorConsN,5,2,texttemp);
    sprintf(textfinal,"fCon %s",texttemp);
    mydisplay.display_message(textfinal,255);
  }
  else if (intervaloSecs >= 6)
  {
    mydisplay.clear_message();
    zera_intervalo();
  }

  buttonState = digitalRead(11);
  if (buttonState == LOW)
  {
    totalvz0 = (vazaomlTotalEEPROM + (vazaomlTotal - vazaomlTotalReset));
    interv = totalvz0/fatorCons;
    fatorConsN = fatorConsN + 0.05;
    totalvz1 = interv * fatorConsN;
    dtostrf(fatorConsN,5,2,texttemp);
    sprintf(textfinal,"fCon %s",texttemp);
    mydisplay.display_message(textfinal,255);
    atraso(1500,0);
    dtostrf((totalvz1/1000.0),5,2,texttemp);
    sprintf(textfinal,"CT %s L",texttemp);
    mydisplay.display_message(textfinal,255);
    atraso(2000,0);
    mydisplay.clear_message();
    iteracao_txt = 0;
    zera_intervalo();
  }
}


void menu_opcoes_Calibr_Decr_fatorCons()
{
  float totalvz0;
  float interv;
  float totalvz1;
  
  calcula_intervalo();
  if (intervaloMiliSecs < 100)
    mydisplay.display_message(F("Fuel dw.05"),10);
  else if (intervaloSecs >= 3 && intervaloSecs <= 6 )
  {
    dtostrf(fatorConsN,5,2,texttemp);
    sprintf(textfinal,"fCon %s",texttemp);
    mydisplay.display_message(textfinal,255);
  }
  else if (intervaloSecs >= 6)
  {
    mydisplay.clear_message();
    zera_intervalo();
  }

  buttonState = digitalRead(11);
  if (buttonState == LOW)
  {
    totalvz0 = (vazaomlTotalEEPROM + (vazaomlTotal - vazaomlTotalReset));
    interv = totalvz0/fatorCons;
    fatorConsN = fatorConsN - 0.05;
    totalvz1 = interv * fatorConsN;
    dtostrf(fatorConsN,5,2,texttemp);
    sprintf(textfinal,"fCon %s",texttemp);
    mydisplay.display_message(textfinal,255);
    atraso(1500,0);
    dtostrf((totalvz1/1000.0),5,2,texttemp);
    sprintf(textfinal,"CT %s L",texttemp);
    mydisplay.display_message(textfinal,255);
    atraso(2000,0);
    mydisplay.clear_message();
    iteracao_txt = 0;
    zera_intervalo();
  }
}


void menu_opcoes_Calibr_Incr_fatorDist()
{
  float DistTotalTemp0;
  float intermd;
  float DistTotalTemp1;
  
  calcula_intervalo();
  if (intervaloMiliSecs < 100)
    mydisplay.display_message(F("Dst.up0.05"),10);
 else if (intervaloSecs >= 3 && intervaloSecs <= 6 )
  {
    dtostrf(fatorDistN,5,2,texttemp);
    sprintf(textfinal,"fDis %s",texttemp);
    mydisplay.display_message(textfinal,255);
  }
  else if (intervaloSecs >= 6 )
  {
    mydisplay.clear_message();
    zera_intervalo();
  }

  buttonState = digitalRead(11);
  if (buttonState == LOW)
  {
    //pegar a dist original
    DistT = (DistCount/16) * fatorDist;
    DistTotalTemp0 = (DistTotalEEPROM + (DistT - DistReset))/1000.0;
    //dividir pelo fatorDist original
    intermd = DistTotalTemp0/fatorDist;
    fatorDistN = fatorDistN + 0.05;
    //Multiplicar pelo novo fatordist
    DistTotalTemp1 = intermd * fatorDistN;
    dtostrf(fatorDistN,5,2,texttemp);
    sprintf(textfinal,"fDis %s",texttemp);
    mydisplay.display_message(textfinal,255);
    atraso(1500,0);
    dtostrf((DistTotalTemp1),5,2,texttemp);
    sprintf(textfinal,"DT %skm",texttemp);
    mydisplay.display_message(textfinal,255);
    atraso(2000,0);
    mydisplay.clear_message();
    iteracao_txt = 0;
    zera_intervalo();
  }
}


void menu_opcoes_Calibr_Decr_fatorDist()
{
  float DistTotalTemp0;
  float intermd;
  float DistTotalTemp1;
  
  calcula_intervalo();
  if (intervaloMiliSecs < 100)
    mydisplay.display_message(F("Dst.dw0.05"),10);
 else if (intervaloSecs >= 3 && intervaloSecs <= 6 )
  {
    dtostrf(fatorDistN,5,2,texttemp);
    sprintf(textfinal,"fDis %s",texttemp);
    mydisplay.display_message(textfinal,255);
  }
  else if (intervaloSecs >= 6 )
  {
    mydisplay.clear_message();
    zera_intervalo();
  }

  buttonState = digitalRead(11);
  if (buttonState == LOW)
  {
    //pegar a dist original
    DistT = (DistCount/16) * fatorDist;
    DistTotalTemp0 = (DistTotalEEPROM + (DistT - DistReset))/1000.0;
    //dividir pelo fatorDist original
    intermd = DistTotalTemp0/fatorDist;
    fatorDistN = fatorDistN - 0.05;
    //Multiplicar pelo novo fatordist
    DistTotalTemp1 = intermd * fatorDistN;
    dtostrf(fatorDistN,5,2,texttemp);
    sprintf(textfinal,"fDis %s",texttemp);
    mydisplay.display_message(textfinal,255);
    atraso(1500,0);
    dtostrf((DistTotalTemp1),5,2,texttemp);
    sprintf(textfinal,"DT %skm",texttemp);
    mydisplay.display_message(textfinal,255);
    atraso(2000,0);
    mydisplay.clear_message();
    iteracao_txt = 0;
    zera_intervalo();
  }
}


void menu_opcoes_Calibr_Incr_fatorVel()
{
  float VelTemp0;
  float VelTemp1;

  calcula_intervalo();
  if (intervaloMiliSecs < 100)
    mydisplay.display_message(F("Spd up0.05"),10);
 else if (intervaloSecs >= 3 && intervaloSecs <= 6 )
  {
    dtostrf(fatorVelN,5,2,texttemp);
    sprintf(textfinal,"fVel %s",texttemp);
    mydisplay.display_message(textfinal,255);
  }
   else if (intervaloSecs >= 6 )
  {
    mydisplay.clear_message();
    zera_intervalo();
  }

  buttonState = digitalRead(11);
  if (buttonState == LOW)
  {
    VelTemp0 = (float(VelKmMax))/fatorVel;
    fatorVelN = fatorVelN + 0.05;
    //Multiplicar pelo novo fatorvel
    VelTemp1 = VelTemp0 * fatorVelN;
    dtostrf(fatorVelN,5,2,texttemp);
    sprintf(textfinal,"fVel %s",texttemp);
    mydisplay.display_message(textfinal,255);
    atraso(1500,0);
    dtostrf(VelTemp1,3,0,texttemp);
    sprintf(textfinal,"Mx %s kmh",texttemp);
    mydisplay.display_message(textfinal,255);
    atraso(2000,0);
    mydisplay.clear_message();
    iteracao_txt = 0;
    zera_intervalo();
  }
}


void menu_opcoes_Calibr_Decr_fatorVel()
{
  float VelTemp0;
  float VelTemp1;

  calcula_intervalo();
  if (intervaloMiliSecs < 100)
    mydisplay.display_message(F("Spd dw0.05"),10);
  else if (intervaloSecs >= 3 && intervaloSecs <= 6 )
  {
    dtostrf(fatorVelN,5,2,texttemp);
    sprintf(textfinal,"fVel %s",texttemp);
    mydisplay.display_message(textfinal,255);
  }
  else if (intervaloSecs >= 6 )
  {
    mydisplay.clear_message();
    zera_intervalo();
  }

  buttonState = digitalRead(11);
  if (buttonState == LOW)
  {
    VelTemp0 = (float(VelKmMax))/fatorVel;
    fatorVelN = fatorVelN - 0.05;
    //Multiplicar pelo novo fatorvel
    VelTemp1 = VelTemp0 * fatorVelN;
    dtostrf(fatorVelN,5,2,texttemp);
    sprintf(textfinal,"fVel %s",texttemp);
    mydisplay.display_message(textfinal,255);
    atraso(1500,0);
    dtostrf(VelTemp1,3,0,texttemp);
    sprintf(textfinal,"Mx %s kmh",texttemp);
    mydisplay.display_message(textfinal,255);
    atraso(2000,0);
    mydisplay.clear_message();
    iteracao_txt = 0;
    zera_intervalo();
  }
}


void menu_opcoes_Sair(byte Indice_Retorno)
{
  calcula_intervalo();
  if (intervaloMiliSecs < 100)
    mydisplay.display_message(F("<-    Back"),255);
  if (intervaloSecs >= 3)
    zera_intervalo();

  buttonState = digitalRead(11);
  if (buttonState == LOW)
  {
    IndiceMenu = Indice_Retorno;
    botao_delay();
    zera_intervalo();
  }
}

void menu_version()
{
  calcula_intervalo();
  if (intervaloMiliSecs < 100)
    mydisplay.display_message(F("Sw.v:80916"),255);
  if (intervaloSecs >= 4)
    zera_intervalo();

  buttonState = digitalRead(11);
  if (buttonState == LOW)
  {
    botao_delay();
    zera_intervalo();
  }
}
void menu_autrip() //////////////////////////////////////////////////////////////////////////////////////
{ 
  if (flagautrip == 0)
  { 
    calcula_intervalo();
    if (intervaloMiliSecs < 100)
      mydisplay.display_message(F("Auto Trip."),255);
    if (intervaloSecs >= 3)
    {
      flagautrip = 1;
    //  zera_intervalo();
    }
  }
  else if (flagautrip == 1)
  {
    buttonState = digitalRead(11);
    if (buttonState == LOW)
    {
     ATtime = ATtime + 3;
     if (ATtime > 10 ) ATtime = 3;    
     dtostrf(ATtime,1,0,texttemp);
     sprintf(textfinal,"T.time: %ss",texttemp);
     mydisplay.display_message(textfinal,255);
     delay(800);
    }
   int  ATcount = ((millis())/ (ATtime*1000)) % 7;
   //////////////////////////////////////////////////////////////////////////////////////
     if (ATcount == 0)  
      {
        calcula_intervalo();
      if (intervaloSecs >= 1)
      {
        zera_intervalo();
        
        vazaoLPorHora = vazaomlPorSeg/1000.0*3600.0;
        vazaomlPorSeg = 0.0;

        DistVelConsInst = (DistVelCountConsInst/16) * fatorDist;
        ConsInstKmL = (DistVelConsInst/1000) / (vazaomlConsInst/1000);
        DistVelCountConsInst = 0.0;
        vazaomlConsInst = 0.0;

        vazaomlTotal = (intervaloMicroSecsTotal/1000000.0) * fatorCons;
        DistT = (DistCount/16) * fatorDist;
        ConsMed = ((DistT - DistReset)/1000)/((vazaomlTotal - vazaomlTotalReset)/1000);
 
        vazaoLTotal = (vazaomlTotal - vazaomlTotalReset)/1000.0;
        averageP = 100 / ConsMed;
        if ((averageP >= 0.0) && (averageP < 100.0)) /////////////////////////
        {
        if (averageP < 1 || averageP > 12.0) {
        mydisplay.display_message(F("Cf   ---  "),255);
        }
        else 
        {
         if (averageP > 10  )  mydisplay.display_message(F("Cf  ----  "),255);
         if (averageP < 6.0 ) mydisplay.display_message(F("Cf >*****<"),255);
         if (averageP < 6.5 && averageP > 6.0 ) mydisplay.display_message(F("Cf >****-<"),255);
         if (averageP < 7.0 && averageP > 6.5 ) mydisplay.display_message(F("Cf >***--<"),255);
         if (averageP < 7.5 && averageP > 7.0 ) mydisplay.display_message(F("Cf >**---<"),255);
         if (averageP < 8.0 && averageP > 7.5 ) mydisplay.display_message(F("Cf >*----<"),255);
         if (averageP < 8.5 && averageP > 8.0 ) mydisplay.display_message(F("Cf >-----<"),255);
         if (averageP < 9.0 && averageP > 8.5 ) mydisplay.display_message(F("Cf  <---> "),255);
         if (averageP < 9.5 && averageP > 9.0 ) mydisplay.display_message(F("Cf <- - ->"),255);
         if (averageP < 10  && averageP > 9.5 ) mydisplay.display_message(F("Cf  ----- "),255);
        }
      }}}
   //////////////////////////////////////////////////////////////////////////////////////
   if (ATcount == 1)  
      {
      calcula_intervalo();
      if (intervaloSecs >= 1)
      {
        zera_intervalo();
      DistTotal = (DistTotalEEPROM + (DistT - DistReset))/1000.0;
      averageT = 100/(DistTotal/((vazaomlTotalEEPROM + (vazaomlTotal - vazaomlTotalReset))/1000.0));
       if ((averageT >= 0.0) && (averageT < 15.0)) /////////////////////////
        {
        if (averageT < 10.0)
          {
            dtostrf((((byte)(averageT*10))/10.0),2,1,texttemp);
          }
          else
          {
            dtostrf(averageT,3,0,texttemp);
          }
         }
         else
         {
         averageT = 0.0;
         dtostrf(averageT,2,1,texttemp);
         } 
        if (averageT < 1 || averageT > 20.0) {
        mydisplay.display_message(F("CA --- L/1"),255);
        }
        else 
        {
        sprintf(textfinal,"CA %s L/1",texttemp);
        mydisplay.display_message(textfinal,255);
        }
      }}
    ///////////////////////////////////////////////////////////////////////////////////
    
     if (ATcount == 2)  
      {
        calcula_intervalo();
      if (intervaloSecs >= 1)
      {
        zera_intervalo();
        DistT = (DistCount/16) * fatorDist;
        DistTotal = (DistTotalEEPROM + (DistT - DistReset))/1000.0;
        if (DistTotal < 100.00)
        {
          dtostrf((((int)(DistTotal*10))/10.0),4,1,texttemp);
        }
        else
        {
          dtostrf(DistTotal,4,0,texttemp);
        }
        sprintf(textfinal,"DT %s km",texttemp);
        mydisplay.display_message(textfinal,255);
        }
        }
  //////////////////////////////////////////////////////////////////////////////////////
   if (ATcount == 3)  
      {
      calcula_intervalo();
      if (intervaloSecs >= 1)
      {
        zera_intervalo();
        dtostrf(vin,4,2,texttemp);     //Converte o float em string - dtostrf(variável a ser convertida, minStringWidthIncDecimalPoint, casas decimais, variável de saída)
      if (vin < 10)
      {
        sprintf(textfinal, "Ba  %s V", texttemp);
      }
      else
      {
        sprintf(textfinal, "Ba %s V", texttemp); //Junta os textos em uma só variável
      }
      mydisplay.display_message(textfinal,255);
      zera_intervalo();
      }}
  /////////////////////////////////////////////////////////////////////////////////////
   if (ATcount == 4)  
      {
      calcula_intervalo();
      if (intervaloSecs >= 1)
      {
        zera_intervalo();
        DistT = (DistCount/16) * fatorDist;
        secs = (millis())/1000;
        VelKmMedia = ((DistT - DistReset)/(secs - secsReset)) * 3.6;
        if ((VelKmMedia < 0) || (VelKmMedia > 200))
          VelKmMedia = 0;
        dtostrf(VelKmMedia,3,0,texttemp);
        sprintf(textfinal,"Sa %s kmh",texttemp);
        mydisplay.display_message(textfinal,255);
      }}
  /////////////////////////////////////////////////////////////////////////////////////
  if (ATcount == 5)  
      {
      calcula_intervalo();
      if (intervaloSecs >= 1)
      {
      int hours=0;
      int mins=0;
      secs = ((millis())/1000) - secsReset; //convert milliseconds to seconds
       secsT = secsTotalEEPROM + secs;
        mins = secsT/60; //convert seconds to minutes
        hours = mins/60; //convert minutes to hours
        secsT = secsT - (long(mins) * 60);
        mins = mins - (hours * 60);
        if (secsT < 10 && mins < 10 && hours < 10)
          sprintf(textfinal,"TT %i:0%i:0%i",hours,mins,secsT);
        else if (secsT >= 10 && mins < 10 && hours < 10) 
          sprintf(textfinal,"TT %i:0%i:%i",hours,mins,secsT);
        else if (secsT < 10 && mins >= 10 && hours < 10)
          sprintf(textfinal,"TT %i:%i:0%i",hours,mins,secsT);
        else if (secsT >= 10 && mins >= 10 && hours < 10)
          sprintf(textfinal,"TT %i:%i:%i",hours,mins,secsT);
        else if (secsT < 10 && mins < 10 && hours >= 10)
          sprintf(textfinal,"TT%i:0%i:0%i",hours,mins,secsT);
        else if (secsT >= 10 && mins < 10 && hours >= 10) 
          sprintf(textfinal,"TT%i:0%i:%i",hours,mins,secsT);
        else if (secsT < 10 && mins >= 10 && hours >= 10)
          sprintf(textfinal,"TT%i:%i:0%i",hours,mins,secsT);
        else if (secsT >= 10 && mins >= 10 && hours >= 10)
          sprintf(textfinal,"TT%i:%i:%i",hours,mins,secsT);
         mydisplay.display_message(textfinal,255);
         zera_intervalo();
  }}
  /////////////////////////////////////////////////////////////////////////////////////  
  if (ATcount == 6)  
      {    
   calcula_intervalo();
    if (intervaloSecs >= 1)
    {
      zera_intervalo();

      if (flagDadosSalvos == 0)
      {
        //Obtendo a quantidade de combustível no tanque
        int sensorValue = analogRead(A7);
        if (sensorValue < 210)
          NivelTanque = 100;
        else if (sensorValue >= 210 && sensorValue < 315)
          NivelTanque = (-0.0024 * sensorValue + 1.505) * 100;
        else if (sensorValue >= 315 && sensorValue < 465)
          NivelTanque = (-0.00159 * sensorValue + 1.2455) * 100;
        else if (sensorValue >= 465 && sensorValue < 630)
          NivelTanque = (-0.0015 * sensorValue + 1.2) * 100;
        else if (sensorValue >= 630 && sensorValue < 765)
          NivelTanque = (-0.0019 * sensorValue + 1.4535) * 100;
        else if (sensorValue >= 765)
          NivelTanque = 0;
      
        // subtrai a leitura do ciclo anterior
        totalNivelTanque = totalNivelTanque - ArrayNivelTanque[IndiceArrayNivTanque];
        // lê o nivel atual
        ArrayNivelTanque[IndiceArrayNivTanque] = NivelTanque;
        //
        if (NivelTanqueMax < NivelTanque)
          NivelTanqueMax = NivelTanque;
        if (NivelTanqueMin > NivelTanque)
          NivelTanqueMin = NivelTanque;
        
        // adiciona a leitura no total
        totalNivelTanque = totalNivelTanque + ArrayNivelTanque[IndiceArrayNivTanque];
        //avança posição na array
        IndiceArrayNivTanque++;
      
        if (IndiceArrayNivTanque >= 30)
        {
          IndiceArrayNivTanque = 0;
          //calcula a média
          NivelTanqueAVG = (totalNivelTanque - (NivelTanqueMax + NivelTanqueMin)) / (30-2);
          
          NivelTanqueMax = 0;
          NivelTanqueMin = 110;
          
          //Obtendo o consumo médio total
          DistT = (DistCount/16) * fatorDist;
          DistTotal = (DistTotalEEPROM + (DistT - DistReset))/1000.0;
          ConsMedTotal = DistTotal/((vazaomlTotalEEPROM + (vazaomlTotal - vazaomlTotalReset))/1000.0);
          if ( !((ConsMedTotal >= 0.0) && (ConsMedTotal < 100.0)) )
          {
            ConsMedTotal = 0.0;
          }
      
          //Autonomia = CombustívelNoTanque * ConsMedTotal
          Aut = (CapacidadeTanque * NivelTanqueAVG / 100) * ConsMedTotal;
        }
      }

      //SE autonomia for MENOR que 10.0 OU motor estiver desligado
      if ( (Aut < 10.0) || (flagDadosSalvos == 1) )
      {
        sprintf(textfinal,"Au  --- km");
      }
      else
      {
        dtostrf((((int)(Aut*10))/10.0),4,0,texttemp);
        sprintf(textfinal,"Au %s km",texttemp);
      }
      mydisplay.display_message(textfinal,255);
    }}
     /////////////////////////////////////////////////////////////////////////////////////
  }}

