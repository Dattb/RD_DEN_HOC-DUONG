/**
  ******************************************************************************
   Dat_UTC  6/10/2020  9:00 PM
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm8s.h"


#define CCR4_Val  ((uint16_t)127)
unsigned int cnt=0;

static void TIM1_Config(void);
void Delay(uint16_t nCount);
void FLASH_read_data(); // read data from EEPROM
void FLASH_save_data(); // write data to EEPROM
static void ADC_Config(); // config ADC1
uint16_t ADC_val = 0; //ADC value 
uint16_t ADC_val_save = 0;  // new ADC value is save after calib
uint16_t ADC_val_save1=0;  // old ADC value to check calib 
uint8_t ADC_H,ADC_L,PWM_H,PWM_L; // ADC and PWM register to read and write to EEPROM
uint8_t FLASH_val = 0x00;    // value you want to save to EEPROM
uint32_t FLASH_addr = 0x00;  // EEPROM address you want to save data
uint16_t PWM_pulse=0;       // PWM value
uint16_t PWM_pulse_save=0; // PWM value is save after calib
uint16_t main_cycle=0;     // main cycle to clear calib_button when it is  noised
uint8_t button_cnt=0;   // cnt button pulse to setup cases and operate

//unsigned int adc_read (unsigned int channel);


/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */

void main(void)
{ //ErrorStatus status = FALSE;
  CLK_DeInit(); // seret clock
  FLASH_DeInit(); // reset flash
    /* Configure the Fcpu to DIV1*/
  CLK_SYSCLKConfig(CLK_PRESCALER_CPUDIV1); // clock  divide  1
    
    /* Configure the HSI prescaler to the optimal value */
  CLK_SYSCLKConfig(CLK_PRESCALER_HSIDIV1); // init high speed internal clock 16MHz
    
   //status = CLK_ClockSwitchConfig(CLK_SWITCHMODE_AUTO, CLK_SOURCE_HSE, DISABLE, CLK_CURRENTCLOCKSTATE_DISABLE);
  /* TIM1 configuration -----------------------------------------*/
  TIM1_Config(); // set up timer 1PWM channel 4 
  ADC_Config();  // set up ADC 1
  GPIO_DeInit(GPIOC);//reset GPIO port C
  GPIO_ExternalPullUpConfig(GPIOB,GPIO_PIN_5,ENABLE); //GPIO PB5 is external pull up 
  GPIO_Init(GPIOB,GPIO_PIN_5,GPIO_MODE_IN_FL_NO_IT);  // GPIO PB5 is input with no external interrupt
   //////////////////////////////////////////////// configure eeprom
  
  FLASH_read_data();  // read data from EEPROM
  TIM1_SetCompare4(0); // set timer 1 channel 4 PWM value is 0
  Delay(5);
  if(PWM_pulse_save>1618) // when PWM value >20% (1618/8096)
  {
    while(PWM_pulse<PWM_pulse_save)
    {
    TIM1_SetCompare4(PWM_pulse++);
    Delay(1);
    }

  }
  else                    // when PWM value >20% (1618/8096)
  {
    while(PWM_pulse<5633)
    {
    TIM1_SetCompare4(PWM_pulse++);
    Delay(1);
    }
    
  }
while (1)
{ 
    
    
  if(!GPIO_ReadInputPin(GPIOB,GPIO_PIN_5))  // if calib button is put
  {
    button_cnt++; 
    PWM_pulse=1618;
    while(!GPIO_ReadInputPin(GPIOB,GPIO_PIN_5)) // when button is putting 
    { 
      Delay(6);
      PWM_pulse++;
      if(PWM_pulse>1680)button_cnt=0; // reset button cnt
      if(PWM_pulse>8096) PWM_pulse=8096;   // fix PWM value when it > 100% (8096)
      TIM1_SetCompare4(PWM_pulse);  // set PWM 
      PWM_pulse_save=PWM_pulse;     // save PWM value
      ADC_val_save=ADC1_GetConversionValue(); // read ADC value
      
    }
    if(button_cnt>=2)  // if  button is put 2 time  set PWM max 100%
    {
      while(PWM_pulse<8096) 
      {
        PWM_pulse++;
        uint16_t i=500;
        while(i--);
        TIM1_SetCompare4(PWM_pulse);
      }
      PWM_pulse_save=PWM_pulse;
      ADC_val_save=ADC1_GetConversionValue();
      button_cnt=0;
    }

   
    main_cycle=0; // clear main cycle
  }
  if(main_cycle==10)  // clear main cycle and button_cnt
  {
    button_cnt=0;
    main_cycle=0;
  }
  if(ADC_val_save1!=ADC_val_save)  // if calib ,save data to EEPROM
  { 
    ADC_val_save1=ADC_val_save;
    FLASH_save_data();
  }
  TIM1_SetCompare4(PWM_pulse);
  ADC_val=ADC1_GetConversionValue();
  if(ADC_val>ADC_val_save) // if ADC value > ADC value is saved, reduce PWM pulse
  {
    if(PWM_pulse>1618)PWM_pulse--;
    TIM1_SetCompare4(PWM_pulse);
    if(ADC_val-ADC_val_save<=15)
    {
      Delay(40);
    }
    else if(ADC_val-ADC_val_save<=30)
    {
      Delay(30);
    }
    else if(ADC_val-ADC_val_save<=45)
    {
      Delay(20);
    }
    else Delay(10);
  }
  if(ADC_val_save==ADC_val)  // if ADC value = ADC value is saved, delay serveral time
  {
    Delay(50);
  }
  if(ADC_val_save>ADC_val) // if ADC value < ADC value is saved, increase PWM pulse
  { 
    if(PWM_pulse<8096) PWM_pulse++;
    TIM1_SetCompare4(PWM_pulse);
    if(ADC_val_save-ADC_val<=15)
    {
      Delay(40);
    }
    else if(ADC_val_save-ADC_val<=30)
    {
      Delay(30);
    }
    else if(ADC_val_save-ADC_val<=45)
    {
      Delay(20);
    }
    else Delay(10);
  }
main_cycle++;
 } 
}

/**
  * @brief  Configure TIM1 to generate 7 PWM signals with 4 different duty cycles
  * @param  None
  * @retval None
  */
static void TIM1_Config(void) 
{
   TIM1_DeInit();
  /* Time Base configuration */
  /*
  TIM1_Period = 8096
  TIM1_Prescaler = 0
  TIM1_CounterMode = TIM1_COUNTERMODE_UP
  TIM1_RepetitionCounter = 0
  */
  TIM1_TimeBaseInit(0, TIM1_COUNTERMODE_UP,8096, 0); 
  /* Channel 1 Configuration in PWM mode */
  /*TIM1_Pulse = CCR4_Val*/
  TIM1_OC4Init(TIM1_OCMODE_PWM2, TIM1_OUTPUTSTATE_ENABLE, CCR4_Val, TIM1_OCPOLARITY_LOW,TIM1_OCIDLESTATE_SET);
  /* TIM1 counter enable */
  TIM1_Cmd(ENABLE);
  /* TIM1 Main Output Enable */
  TIM1_CtrlPWMOutputs(ENABLE);
}


static void ADC_Config()
{
  /*  Init GPIO for ADC2 */
  GPIO_Init(GPIOD, GPIO_PIN_3, GPIO_MODE_IN_FL_NO_IT);
  /* De-Init ADC peripheral*/
  ADC1_DeInit();
  /* Init ADC2 peripheral */
  ADC1_Init(ADC1_CONVERSIONMODE_CONTINUOUS, ADC1_CHANNEL_4, ADC1_PRESSEL_FCPU_D2,
  ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_RIGHT, ADC1_SCHMITTTRIG_CHANNEL4,DISABLE);
  /* Enable EOC interrupt */
  ADC1_ITConfig(ADC1_IT_AWDIE,ENABLE);
  /* Enable general interrupts */  
  enableInterrupts();
  /*Start Conversion */
  ADC1_StartConversion();
}




void Delay(uint16_t nCount) //delay serveral time (dont need to use timer)
{
  /* Decrement nCount value */
  
  for(int i=0;i<nCount;i++)
  { 
    uint16_t cnt=2000;
    while(cnt--);
  }
}

void FLASH_read_data()
{
  FLASH_SetProgrammingTime(FLASH_PROGRAMTIME_STANDARD);
  //Unlock Data memory 
  FLASH_Unlock(FLASH_MEMTYPE_DATA);
  /////////////////////////////////////////////////////// recover  ADC value is  calib
  ADC_H=0x0000;
  ADC_L=0x0000;
  ADC_L  =FLASH_ReadByte(0x4000);
  ADC_H  =FLASH_ReadByte(0x4001);
  ADC_val_save = (ADC_H<<8|ADC_L);
  ADC_val_save1=ADC_val_save;
  /////////////////////////////////////////////////////// recover PWM is calib
  PWM_H=0x0000;
  PWM_L=0x0000;
  PWM_L=FLASH_ReadByte(0x403F);
  PWM_H=FLASH_ReadByte(0x4040);
  PWM_pulse_save=(PWM_H<<8|PWM_L);
}

void FLASH_save_data()
{
  FLASH_SetProgrammingTime(FLASH_PROGRAMTIME_STANDARD);
  //Unlock Data memory 
  FLASH_Unlock(FLASH_MEMTYPE_DATA);
  /////////////////////// STM8S003's EEPROM  from 4000 to 407f
  /////////////////////////////////////////////////////// Write ADC value save to EEPROM
  FLASH_addr=0x4000; 
  FLASH_EraseByte(FLASH_addr);
  FLASH_ProgramByte(FLASH_addr, ADC_val_save1);
  FLASH_addr=0x4001; 
  FLASH_EraseByte(FLASH_addr);
  FLASH_ProgramByte(FLASH_addr, (ADC_val_save1>>8));
   /////////////////////////////////////////////////////// Write PWM pulse save to EEPROM 
  FLASH_addr=0x403F;
  FLASH_EraseByte(FLASH_addr);
  FLASH_ProgramByte(FLASH_addr, PWM_pulse_save);
  PWM_L=PWM_pulse_save;
  PWM_H=(PWM_pulse_save>>8);
  FLASH_addr=0x4040;
  FLASH_EraseByte(FLASH_addr);
  FLASH_ProgramByte(FLASH_addr, (PWM_pulse_save>>8));
}

#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
