#include "stm32f3xx.h"
#include "pinAccess.h"

void wait()
{
    volatile int i=0;
    for(i=0;i<20000;i++);
}

void setup()
{
    pinMode(GPIOB,3,OUTPUT);

    //pin mode Motor
    pinMode(GPIOA,7,OUTPUT);
    pinMode(GPIOA,8,OUTPUT);    
    pinMode(GPIOA,6,OUTPUT);
    pinMode(GPIOA,5,OUTPUT);
}
void stepCCW(int step){
    static const unsigned char seq[] = {8, 0xc, 4, 6, 2, 3, 1, 9};

    GPIOA->ODR &= ~(0xF << 5);   
    GPIOA->ODR |=  (seq[step] << 5);   
}





void setupTimer(){
    
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; //enable clock for TIM2
    TIM2->PSC = 64000; //prescaler
    TIM2->ARR = 100; //auto reload value
    TIM2->CR1 |= TIM_CR1_CEN; //enable timer
    TIM2->CNT = 0; //reset counter
}
/* main function */
int main(void)
{
    setup();
    /* Infinite loop */
    while (1)
    {
        //digitalToggle(GPIOB,3);
        for (int i=0;i<8;i++){
            stepCCW(i);
            wait();
        }
        //wait();
    }
}
