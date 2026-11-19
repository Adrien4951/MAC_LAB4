#include "stm32f3xx.h"
#include "pinAccess.h"
#include <stdbool.h>
#include "adc.h"

void wait()
{
    volatile int i=0;
    for(i=0;i<20000;i++);
}

void setup()
{
    pinMode(GPIOB,3,OUTPUT);
    ADCInit(); 
    pinAnalog(GPIOA,3);
    
    //pin mode Motor
    pinMode(GPIOA,7,OUTPUT);
    pinMode(GPIOA,8,OUTPUT);    
    pinMode(GPIOA,6,OUTPUT);
    pinMode(GPIOA,5,OUTPUT);
    pinMode(GPIOB,1,INPUT_PULLUP);
}
void stepCCW(int step){
    static const unsigned char seq[] = {8, 0xc, 4, 6, 2, 3, 1, 9};

    GPIOA->ODR &= ~(0xF << 5);   
    GPIOA->ODR |=  (seq[step] << 5);   
}
void stepCW(int step){
    static const unsigned char seq[] = {8, 0xc, 4, 6, 2, 3, 1, 9};

    GPIOA->BSRR = 0xF << (5 + 16);      
    GPIOA->BSRR = seq[step] << 5;      
}




void setupTimer(){
    
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; //enable clock for TIM2
    TIM2->PSC = 64000; //prescaler
    TIM2->ARR = 10; //auto reload value
    TIM2->CR1 |= TIM_CR1_CEN; //enable timer
    TIM2->CNT = 0; //reset counter
    TIM2->CR1 |= TIM_CR1_ARPE;   // ARR mis à jour seulement au débordement
    
}
/* main function */
int main(void)
{
    setup();
    setupTimer();
    bool bp = false;
    bool turn = false;
    int i = 0;
    /* Infinite loop */
    while (1)
    {
        uint16_t val = ADCRead(); 
        if (val > 2047){
             val -= 2047;
             turn = true;


        }
        else{
            val= 2047 - val;
            turn = false;
        }
        TIM2->ARR = 100 - (val * 99) / 2047;   // val=0 → 100, 
       if(((GPIOB->IDR & (1U << 1)) == 0)&& bp == false){
            bp = true;
               TIM2->CR1 ^= TIM_CR1_CEN;   // inverse timer enable/disable
       }
       else if (((GPIOB->IDR & (1U << 1)) != 0) && bp == true)
       { 
            bp = false;
       }
        if ((TIM2->SR & TIM_SR_UIF))
        {
                TIM2->SR &= ~TIM_SR_UIF; //clear update event flag
                stepCW(i);
              if (turn) {
                  i++;
              }
              else {
                    i--;
              }
              if(i<0){
                    i=7;
                }

                if(i>7){
                    i=0;
                }
            }
            
        //wait();
    }
}
