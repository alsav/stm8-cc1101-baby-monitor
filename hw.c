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


void GPIO_Config() {


    // test-led init (PP)
    TST_LED_PORT->DDR |= TST_LED_PIN;	//out
    TST_LED_PORT->CR1 |= TST_LED_PIN;	//pull-up/push-pull


    //switch pin config
    SWITCH_PORT->DDR &= ~SWITCH_PIN;		//in
    SWITCH_PORT->CR2 |= SWITCH_PIN;	//with_interrupt

    //exti config (switch)
    EXTI->CR1 &= (uint8_t)(~EXTI_CR1_PAIS);		//clear EXTI PORTA-sens interrupts
//    EXTI->CR1 |= (uint8_t)(SWITCH_EXTI_FALL);	//set EXTI PORT-sense
//    EXTI->CR1 |= (uint8_t)(SWITCH_EXTI_RISE_FALL);	//set EXTI PORT-sense
    ENABLE_SWITCH_EXTI_FALL;
//    ENABLE_SWITCH_EXTI_RISE;

    //power-control config (PP-OD)
    PWR_CTL_PORT->DDR |= PWR_CTL_PIN;	// out open-drain
    PWR_CTL_PORT->CR1 |= PWR_CTL_PIN;	//pull-up/push-pull

    //leds config 
    LED_PORT->DDR |= LED_PWR_PIN | LED_STATE_PIN;	//data direction put
    LED_PORT->CR1 |= LED_PWR_PIN | LED_STATE_PIN;	//pull-up/push-pull

    //CC1101 SCK,MOSI,CS config
    CC1101_SPI_PORT->DDR |= CC1101_CS_PIN;	//out
    CC1101_SPI_PORT->CR1 |= CC1101_CS_PIN;	//pp




}


void TIM4_Config() {
//    TIM4->PSCR |= TIM4_PRESCALER_8;	//set timer prescaler
//    TIM4->PSCR |= TIM4_PRESCALER_128;	//set timer prescaler
    TIM4->PSCR |= TIM4_PRESCALER_64;	//set timer prescaler
//    TIM4->ARR = 0xb0;			//set timer reload value (19ms for all adc conversions)
    TIM4->ARR = 0xff;			//set timer reload value (60hz conv period)
    TIM4->SR1 &= ~TIM4_SR1_UIF;		//clear update interrupt flag
    TIM4->IER |= TIM4_IT_UPDATE;		//enable interrupt
}


void ADC_Config()
{
    ADC1->CSR = ADC1_CSR_RESET_VALUE;	//reset CSR register
//    ADC1->CSR |= TEMP_SENS_ADC_CHANNEL;	//set ADC input channel
    ADC1->CSR |= SIG_IN_ADC_CHANNEL;	//set ADC input channel

    ADC1->CR1 |= ADC1_PRESSEL_FCPU_D18;	//set prescaler to fcpu/18

    ADC1->CR2 |= ADC1_CR2_ALIGN;	//set right data alignment (data in ADC_DRL register)

//    ADC1->CR1 |= ADC1_CR1_ADON;		//enable adc

    ADC1->CSR |= ADC1_CSR_EOCIE;	//enable end-of-conversion interrupt

}



uint8_t config_cc1101_gfsk[]={
    CC1101_IOCFG0,      0x06,
    CC1101_FIFOTHR,     0x47,
    CC1101_PKTLEN,      0x20,	//packet length
    CC1101_PKTCTRL0,    0x04,
    CC1101_FSCTRL1,     0x06,
    CC1101_FSCTRL0,     0x00,
    CC1101_FREQ2,       0x10,
    CC1101_FREQ1,       0xAA,
    CC1101_FREQ0,       (0x56+FREQ_CORR), //433.300 - lpd/10
    CC1101_MDMCFG4,     0xf6,
    CC1101_MDMCFG3,     0xE4,
    CC1101_MDMCFG2,     0x13,
    CC1101_MDMCFG1,     0x20,
    CC1101_MDMCFG0,     0x00,
    CC1101_DEVIATN,     0x10,
    CC1101_MCSM0,       0x18,
    CC1101_FOCCFG,      0x16,
    CC1101_WORCTRL,     0xFB,
    CC1101_FSCAL3,      0xE9,
    CC1101_FSCAL2,      0x2A,
    CC1101_FSCAL1,      0x00,
    CC1101_FSCAL0,      0x1F,
    CC1101_TEST2,       0x81,
    CC1101_TEST1,       0x35,
    CC1101_TEST0,       0x09,
};



void CC1101_Config()	//for cc1101
{
    uint8_t i;

    CC1101_CS_OFF;

    //spi config
    SPI->CR1 |= SPI_BAUDRATEPRESCALER_32 | SPI_MODE_MASTER;
    SPI->CR2 |= SPI_CR2_SSM | SPI_CR2_SSI;

    delay(10000);
    SPI->CR1 |= SPI_CR1_SPE;	// enable spi

    delay(10000);

    CC1101_SPITransfer(CC1101_SRES,0x00);	//reset


    for(i=0; i<sizeof(config_cc1101_gfsk); i+=2)
	CC1101_SPITransfer(config_cc1101_gfsk[i],config_cc1101_gfsk[i+1]);

}




uint8_t CC1101_SPITransfer(uint8_t addr, uint8_t data)
{

    CC1101_CS_ON;

    while ((SPI->SR & SPI_SR_TXE) == RESET);	//wait until tx buf not empty
    SPI->DR = addr;				//send addr

    while ((SPI->SR & SPI_SR_TXE) == RESET);	//wait until tx buf not empty
    SPI->DR = data;				//send data

    while ((SPI->SR & SPI_SR_RXNE) == RESET);	//wait until rx buf empty
    data = SPI->DR;				//receive data

    while ((SPI->SR & SPI_SR_RXNE) == RESET);	//wait until rx buf empty
    data = SPI->DR;				//receive data

    while ((SPI->SR & SPI_SR_BSY) == SET);	//wait until spi busy

    CC1101_CS_OFF;
    
    return data;
}




void HW_Init()
{

//    disableInterrupts();

    GPIO_Config();
    ADC_Config();
    TIM4_Config();

//    enableInterrupts();



}











