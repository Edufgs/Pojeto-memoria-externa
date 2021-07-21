#include "main.h"
#include "usb_cdc.h" /*conexão USB tipo CDC: Communication Device Class*/
#include <2404.C>

//pinos LCD
#ifndef lcd_enable
   #define lcd_enable   pin_d3   // pino enable do LCD
   #define lcd_rs       pin_d2   // pino rs do LCD
   //#define lcd_rw     pin_e2   // pino rw do LCD
   #define lcd_d4       pin_d4   // pino de dados d4 do LCD
   #define lcd_d5       pin_d5   // pino de dados d5 do LCD
   #define lcd_d6       pin_d6   // pino de dados d6 do LCD
   #define lcd_d7       pin_d7   // pino de dados d7 do LCD
#endif

#include "mod_lcd.c"

void dadosUsb(); //Envia os dados do USB para o computador
void potenciometros(); //Pega a voltagem do poteciometro
void insereMemoria(unsigned int pot); //Insere na memoria os dados
void data(unsigned int dia,mes,ano,semana,horas,minutos,segundos);// grava na memoria a data e hora

//Variaveis
unsigned int contador = 0; //Conta quantas vezes entrou na interrupção interna (Timer0)
unsigned int pot1, pot2; //Valores em volts dos potenciometros
unsigned int16 j;
long int endereco =1; //O vetor de posição vai de 2 a 504;
float v1, v2;
char pc;

#int_RTCC
void RTCC_isr(void) 
{
   contador++;
}

void main()
{
   setup_adc_ports(AN0_TO_AN1|VSS_VDD);
   setup_adc(ADC_CLOCK_INTERNAL);
   
   setup_psp(PSP_DISABLED);
   setup_spi(SPI_SS_DISABLED);
   setup_wdt(WDT_OFF);
   
   setup_timer_0(RTCC_INTERNAL|RTCC_DIV_64); //349ms
   setup_timer_1(T1_DISABLED);
   setup_timer_2(T2_DISABLED,0,1);
   setup_timer_3(T3_DISABLED|T3_DIV_BY_1);
   
   setup_comparator(NC_NC_NC_NC);
   setup_vref(FALSE);
   
   init_ext_eeprom(); //inicia memoria
   lcd_ini();//Inicia o display
   
   //Guarda o Data (dia,mes,ano,semana,hora,minutos,segundos)
   data(20,7,21,3,1,35,20);
   

   //Inicia o USB
   usb_cdc_init();
   usb_init();
   while(!usb_cdc_connected());
   
   printf(lcd_escreve,"\fInicio!!!      "); //Limpa o display e imprime
   
   //Verifica se tem dados na memoria
   endereco = read_ext_eeprom(0);
   endereco = endereco + read_ext_eeprom(1);
   
   
   enable_interrupts(INT_RTCC);
   enable_interrupts(GLOBAL);

   while(true){
      if(contador >=3){
         potenciometros(); //Pega os valores no potenciometros
         v1 = pot1*0.019607843;
         v2 = pot2*0.019607843;
         printf(lcd_escreve,"\fP1:%f P2:%f\nEndereco:%ld      "v1,v2,endereco); //Limpa o display e imprime
         
         //insere na memoria
         insereMemoria(pot1);
         insereMemoria(pot2);
         contador = 0;
      }
      dadosUsb(); //verifica se tem dados no buffer
   }
}

// Conta de conversão (ADC -> Volts)
//    ADC   =   V
//    255   =   5
//   value  =   X
        
//       5 * value
//  x = ----------- -> x = 0.019607843 * value
//          255
//Manda os dados pela USB
void dadosUsb(){
   if(usb_cdc_kbhit()){ //Retorna TRUE se houver um ou mais caracteres recebidos e esperando no buffer de recebimento.
      pc = usb_cdc_getc(); //Obtém um caractere do buffer de recebimento
      if(pc == 'A'){
         printf(lcd_escreve,"\fEnviando dados!!!      "); //Limpa o display e imprime
         data(21,7,21,4,15,35,20);
         //manda a data e hora para o grafico
         printf(usb_cdc_putc,"%u/%u/%u %u %u:%u:%u \n",read_ext_eeprom(505),read_ext_eeprom(506),read_ext_eeprom(507),read_ext_eeprom(508),read_ext_eeprom(509),read_ext_eeprom(510),read_ext_eeprom(511)); 
         //Imprime no grafico os dados
         for(j=endereco+1;j<=503;j++){
            v1 = read_ext_eeprom(j)*0.019607843;
            j++;
            v2 = read_ext_eeprom(j)*0.019607843;
            printf(usb_cdc_putc,"%f %f\n",v1,v2);
         }
         for(j=2;j<=endereco;j++){
            v1 = read_ext_eeprom(j)*0.019607843;
            j++;
            v2 = read_ext_eeprom(j)*0.019607843;
            printf(usb_cdc_putc,"%f %f\n",v1,v2);
         }
      }  
   }
}

//Pega os dados do potenciometro
void potenciometros(){
   //Potenciometro 1
   set_adc_channel(0);//AN0
   pot1 =read_adc();
         
   //Potenciometro 2
   set_adc_channel(1);//AN1
   pot2 =read_adc();
}

//insere na memoria
void insereMemoria(unsigned int pot){
   if(endereco <503){
      endereco++;
      write_ext_eeprom(endereco,pot);
      if(endereco <= 255){
         write_ext_eeprom(0,endereco);
      }else{
         write_ext_eeprom(1,endereco-255);
      }
   }else{
      endereco = 2;
      write_ext_eeprom(0,endereco);
      write_ext_eeprom(1,0);
      write_ext_eeprom(endereco,pot);
   }
}

//Insere na memoria o dia, mes, ano, semana, hora, minutos, segundos na ultima posição da memoria
void data(unsigned int dia,mes,ano,semana,horas,minutos,segundos){
   write_ext_eeprom(511,segundos); //De 0 ate 59
   write_ext_eeprom(510,minutos); //De 0 ate 59
   write_ext_eeprom(509,horas); //De 0 ate 23
   write_ext_eeprom(508,semana); //de 0 ate 6, (Domindo, segunda, terça, quarta, quinta, sexta).
   write_ext_eeprom(507,ano); //0 até 99, (2000, 2001,...., 2099), ultimos 2 digitos.
   write_ext_eeprom(506,mes); //0 até 11, (janeiro, fevereiro,....,dezembro)
   write_ext_eeprom(505,dia); //0 a 31
}
