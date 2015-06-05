/*
Copyright (C) 2015 Alexander Sadakov <al.sadakov_dog_gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "main.h"


volatile uint8_t flags;
volatile uint8_t conv_num=16;
volatile uint16_t conv_var, conv_result=0;

volatile uint8_t res_buf[RES_BUF_LEN];
volatile uint8_t res_buf_pos;

volatile uint8_t halt_countdown;

uint8_t sig_check_tick_counter=SIG_CHECK_TICKS;

uint8_t sig_average;
uint16_t sig_sum;
uint32_t beacon_wait_counter;

uint8_t button_counter;
uint8_t mode=0;


extern uint8_t txbuf[UART_BUF_SIZE];	//for debug



void delay(uint32_t t)
{
  while(t--);
}

void switch_mode()
{
    uint8_t i;

    for (i=0;i<mode+1;i++) {	// indicate selected mode
	LED_STATE_ON;
	delay(15000);
	LED_STATE_OFF;
	delay(15000);
    }

//    if (mode==MODE_BEACON) {
//	TIM4->CR1 &= ~TIM4_CR1_CEN;	//disable tim4
//    } else {
//	TIM4->CR1 |= TIM4_CR1_CEN;	//enable tim4
//    }
}



void send_alrm(uint8_t len, uint8_t power)
{
    uint8_t i;

//    TST_LED_ON;
    CC1101_SPITransfer(CC1101_PATABLE,power);
    CC1101_SPITransfer(CC1101_STX,0x00);
    for (i=0;i<len;i++)
	delay(10000);

    CC1101_SPITransfer(CC1101_SIDLE,0x00);
//    TST_LED_OFF;
}


void calculate_sig(void)
{
    uint8_t i;

    sig_sum=0;
    for (i=0;i<RES_BUF_LEN;i++) {
	sig_sum += res_buf[i];
    }
    sig_average = sig_sum/RES_BUF_LEN;

}

void shutdown()
{
    CC1101_CS_ON;
    SPI->CR1 &= ~SPI_CR1_SPE;	// disable spi
    PWR_OFF;
    TIM4->CR1 &= ~TIM4_CR1_CEN;	//disable tim4
//    DISABLE_SWITCH_EXTI_RISE;
//    ENABLE_SWITCH_EXTI_FALL;
    LED_PWR_OFF;
//    set_flag(FLAG_DEVICE_HALTED);
    do {
	halt();
	delay(100000);
//    } while ( ( SWITCH_PORT->IDR & SWITCH_PIN ) == 0 ); //button pressed
    } while ( SWITCH_PORT->IDR & SWITCH_PIN ); //button released
//    wfi();

}

void wakeup()
{
//    ENABLE_SWITCH_EXTI_RISE;
    DISABLE_SWITCH_EXTI_FALL;
//    EXTI->CR1 &= (uint8_t)(~EXTI_CR1_PAIS);
    
    CC1101_Config();
    TIM4->CR1 |= TIM4_CR1_CEN;		//enable tim4

    PWR_ON;
    CC1101_CS_OFF;
    LED_PWR_ON;
    reset_flag(FLAG_DEVICE_HALTED);
    
    switch_mode();
}

/*
void measure_voltage()
{
    ADC1->CSR &= ~SIG_IN_ADC_CHANNEL;	//reset ADC input SIG channel
    ADC1->CSR |= VCC_IN_ADC_CHANNEL;	//set ADC input VCC channel
    ADC1->CSR &= ~ADC1_CSR_EOCIE;	//disable end-of-conversion interrupt

    ADC1->CR1 |= ADC1_CR1_ADON;	//start adc conversion

    while ((ADC1->CSR & ADC1_CSR_EOC) == RESET); //wait end-of-conversion
    ADC1->CSR &= ~ADC1_CSR_EOC;			//clear end-of-conversion flag

    conv_result = ADC1->DRH;
    conv_result = ADC1->DRL;

    ADC1->CSR &= ~VCC_IN_ADC_CHANNEL;	//reset ADC VCC input channel
    ADC1->CSR |= SIG_IN_ADC_CHANNEL;	//set ADC SIG input channel
    ADC1->CSR |= ADC1_CSR_EOCIE;	//enable end-of-conversion interrupt


//    sprintf(txbuf,"vcc:%d\r\n", conv_result);
//    UART_SendStr(txbuf);    //send result to uart

}
*/

int main( void )
{
    uint8_t i=0;


    disableInterrupts();

    HW_Init();
    UART_Config();

    enableInterrupts();

    halt();
//    shutdown();
    wakeup();





    while(1) {

	if ( get_flag(FLAG_DEVICE_HALTED) ) {
	    shutdown();
	    wakeup();
	}

	if (mode==MODE_BEACON) {
	    if (beacon_wait_counter==0) {
		send_alrm(4,CC1101_POWER_10DBM);
		delay(100000);
		send_alrm(4,CC1101_POWER_5DBM);
		delay(100000);
		send_alrm(4,CC1101_POWER_0DBM);
		delay(100000);
		send_alrm(4,CC1101_POWER__10DBM);
		delay(100000);
		send_alrm(4,CC1101_POWER__20DBM);
//		delay(800000);
		beacon_wait_counter=500000;
	    } else {
		beacon_wait_counter--;
	    }

	}



	if ( get_flag(FLAG_END_ADC_CONV) ){
	    reset_flag(FLAG_END_ADC_CONV)

//button check
	    if ( ( SWITCH_PORT->IDR & SWITCH_PIN ) == 0 ) { //button pressed
		button_counter++;
		if (button_counter>=BTN_HALT_TICKS){
		    set_flag(FLAG_DEVICE_HALTED);
		    button_counter=0;
		}

	    } else if ( SWITCH_PORT->IDR & SWITCH_PIN ) { //button released
		if (button_counter>3) {
		//switch mode
		    mode = (mode+1) % MODES_NUM;
		    switch_mode();
		}
		button_counter = 0;
	    }


	    if (sig_check_tick_counter==0) {
		sig_check_tick_counter = SIG_CHECK_TICKS;
		calculate_sig();

//		measure_voltage();
	    ADC1->CSR &= ~SIG_IN_ADC_CHANNEL;	//reset ADC input SIG channel
	    ADC1->CSR |= VCC_IN_ADC_CHANNEL;	//set ADC input VCC channel
	    set_flag(FLAG_VCC_MEASURE);
	    if (conv_result>475) {
		sprintf(txbuf,"VCC:%d, HALT!\r\n",conv_result);
		send_alrm(6,CC1101_POWER_10DBM);
		UART_SendStr(txbuf);    //send result to uart
		conv_result=0;
		set_flag(FLAG_DEVICE_HALTED);
	    }

//		sprintf(txbuf,"Sum:%d, Avg:%d\r\n",sig_sum, sig_average);
		sprintf(txbuf,"S:%d A:%d V:%d\r\n",sig_sum, sig_average,conv_result);
		UART_SendStr(txbuf);    //send result to uart

		if (sig_sum>=SIG_SUM_TRESHOLD_LOW && mode!=MODE_BEACON) {
		    LED_STATE_ON;
		    send_alrm(ALARM_LEN,CC1101_POWER_10DBM);
		    LED_STATE_OFF;
		}
	    }
	    sig_check_tick_counter--;



	}


    }



}




INTERRUPT_HANDLER(EXTI_PORTA_IRQHandler, 3)
{
}

//INTERRUPT_HANDLER(EXTI_PORTB_IRQHandler, 4)
//{
//}

//INTERRUPT_HANDLER(EXTI_PORTC_IRQHandler, 5)
//{
//}


//end adc conversion
INTERRUPT_HANDLER(ADC1_IRQHandler, 22)
{
    ADC1->CSR &= ~ADC1_CSR_EOC;			//clear end-of-conversion flag

    if ( get_flag(FLAG_VCC_MEASURE) ) {
	reset_flag(FLAG_VCC_MEASURE);
	conv_result = ADC1->DRL;
	conv_result += (ADC1->DRH << 8);
	ADC1->CSR &= ~VCC_IN_ADC_CHANNEL;	//reset ADC VCC input channel
	ADC1->CSR |= SIG_IN_ADC_CHANNEL;	//set ADC SIG input channel

    } else {
	conv_var += ADC1->DRL;
	conv_var += (ADC1->DRH << 8);		//add conversion to result

	if (conv_num) {				//if not all adc conversions: wait tim4 and start adc again
	    conv_num--;

	} else {					//all conversions finished

	    res_buf[res_buf_pos]=conv_var>>4;
	    res_buf_pos = (res_buf_pos+1) % RES_BUF_LEN;
	    conv_var=0;
	    conv_num=16;

	    set_flag(FLAG_END_ADC_CONV);

	}
    }
}


//timer interrupt
INTERRUPT_HANDLER(TIM4_UPD_OVF_IRQHandler, 23)
{
    TIM4->SR1 &= ~TIM4_SR1_UIF;	//clear update interrupt flag
    ADC1->CR1 |= ADC1_CR1_ADON;	//start adc conversion
    set_flag(FLAG_TIMER_TICK);
}

