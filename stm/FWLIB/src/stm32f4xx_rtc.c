/* ----------------------------------------------------------------------
 * Copyright (C) 2025-2026 RAI K63. All rights reserved.
 *
 * $Date:        2026-05-19
 * $Revision:    1.0
 *
 * Project:      RAI STM
 * Title:        stm32f4xx_rtc.c
 *
 * Description:  stm32f4xx_rtc.c module
 *
 * Target Processor: Cortex-M7/Cortex-M4/Cortex-M3/Cortex-M0
 * -------------------------------------------------------------------- */

#include "stm32f4xx_rtc.h"


#define RTC_TR_RESERVED_MASK    ((uint32_t)0x007F7F7F)
#define RTC_DR_RESERVED_MASK    ((uint32_t)0x00FFFF3F)
#define RTC_INIT_MASK           ((uint32_t)0xFFFFFFFF)
#define RTC_RSF_MASK            ((uint32_t)0xFFFFFF5F)
#define RTC_FLAGS_MASK          ((uint32_t)(RTC_FLAG_TSOVF | RTC_FLAG_TSF | RTC_FLAG_WUTF | \
                                            RTC_FLAG_ALRBF | RTC_FLAG_ALRAF | RTC_FLAG_INITF | \
                                            RTC_FLAG_RSF | RTC_FLAG_INITS | RTC_FLAG_WUTWF | \
                                            RTC_FLAG_ALRBWF | RTC_FLAG_ALRAWF | RTC_FLAG_TAMP1F | \
                                            RTC_FLAG_RECALPF | RTC_FLAG_SHPF))

#define INITMODE_TIMEOUT         ((uint32_t) 0x00010000)
#define SYNCHRO_TIMEOUT          ((uint32_t) 0x00020000)
#define RECALPF_TIMEOUT          ((uint32_t) 0x00020000)
#define SHPF_TIMEOUT             ((uint32_t) 0x00001000)


static uint8_t RTC_ByteToBcd2(uint8_t Value);
static uint8_t RTC_Bcd2ToByte(uint8_t Value);


ErrorStatus RTC_DeInit(void)
{
  __IO uint32_t wutcounter = 0x00;
  uint32_t wutwfstatus = 0x00;
  ErrorStatus status = ERROR;


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;


  if (RTC_EnterInitMode() == ERROR)
  {
    status = ERROR;
  }
  else
  {

    RTC->TR = (uint32_t)0x00000000;
    RTC->DR = (uint32_t)0x00002101;

    RTC->CR &= (uint32_t)0x00000007;


    do
    {
      wutwfstatus = RTC->ISR & RTC_ISR_WUTWF;
      wutcounter++;
    } while((wutcounter != INITMODE_TIMEOUT) && (wutwfstatus == 0x00));

    if ((RTC->ISR & RTC_ISR_WUTWF) == RESET)
    {
      status = ERROR;
    }
    else
    {

      RTC->CR &= (uint32_t)0x00000000;
      RTC->WUTR = (uint32_t)0x0000FFFF;
      RTC->PRER = (uint32_t)0x007F00FF;
      RTC->CALIBR = (uint32_t)0x00000000;
      RTC->ALRMAR = (uint32_t)0x00000000;
      RTC->ALRMBR = (uint32_t)0x00000000;
      RTC->SHIFTR = (uint32_t)0x00000000;
      RTC->CALR = (uint32_t)0x00000000;
      RTC->ALRMASSR = (uint32_t)0x00000000;
      RTC->ALRMBSSR = (uint32_t)0x00000000;


      RTC->ISR = (uint32_t)0x00000000;


      RTC->TAFCR = 0x00000000;

      if(RTC_WaitForSynchro() == ERROR)
      {
        status = ERROR;
      }
      else
      {
        status = SUCCESS;
      }
    }
  }


  RTC->WPR = 0xFF;

  return status;
}


ErrorStatus RTC_Init(RTC_InitTypeDef* RTC_InitStruct)
{
  ErrorStatus status = ERROR;


  assert_param(IS_RTC_HOUR_FORMAT(RTC_InitStruct->RTC_HourFormat));
  assert_param(IS_RTC_ASYNCH_PREDIV(RTC_InitStruct->RTC_AsynchPrediv));
  assert_param(IS_RTC_SYNCH_PREDIV(RTC_InitStruct->RTC_SynchPrediv));


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;


  if (RTC_EnterInitMode() == ERROR)
  {
    status = ERROR;
  }
  else
  {

    RTC->CR &= ((uint32_t)~(RTC_CR_FMT));

    RTC->CR |=  ((uint32_t)(RTC_InitStruct->RTC_HourFormat));


    RTC->PRER = (uint32_t)(RTC_InitStruct->RTC_SynchPrediv);
    RTC->PRER |= (uint32_t)(RTC_InitStruct->RTC_AsynchPrediv << 16);


    RTC_ExitInitMode();

    status = SUCCESS;
  }

  RTC->WPR = 0xFF;

  return status;
}


void RTC_StructInit(RTC_InitTypeDef* RTC_InitStruct)
{

  RTC_InitStruct->RTC_HourFormat = RTC_HourFormat_24;


  RTC_InitStruct->RTC_AsynchPrediv = (uint32_t)0x7F;


  RTC_InitStruct->RTC_SynchPrediv = (uint32_t)0xFF;
}


void RTC_WriteProtectionCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    RTC->WPR = 0xFF;
  }
  else
  {

    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;
  }
}


ErrorStatus RTC_EnterInitMode(void)
{
  __IO uint32_t initcounter = 0x00;
  ErrorStatus status = ERROR;
  uint32_t initstatus = 0x00;


  if ((RTC->ISR & RTC_ISR_INITF) == (uint32_t)RESET)
  {

    RTC->ISR = (uint32_t)RTC_INIT_MASK;


    do
    {
      initstatus = RTC->ISR & RTC_ISR_INITF;
      initcounter++;
    } while((initcounter != INITMODE_TIMEOUT) && (initstatus == 0x00));

    if ((RTC->ISR & RTC_ISR_INITF) != RESET)
    {
      status = SUCCESS;
    }
    else
    {
      status = ERROR;
    }
  }
  else
  {
    status = SUCCESS;
  }

  return (status);
}


void RTC_ExitInitMode(void)
{

  RTC->ISR &= (uint32_t)~RTC_ISR_INIT;
}


ErrorStatus RTC_WaitForSynchro(void)
{
  __IO uint32_t synchrocounter = 0;
  ErrorStatus status = ERROR;
  uint32_t synchrostatus = 0x00;


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;


  RTC->ISR &= (uint32_t)RTC_RSF_MASK;


  do
  {
    synchrostatus = RTC->ISR & RTC_ISR_RSF;
    synchrocounter++;
  } while((synchrocounter != SYNCHRO_TIMEOUT) && (synchrostatus == 0x00));

  if ((RTC->ISR & RTC_ISR_RSF) != RESET)
  {
    status = SUCCESS;
  }
  else
  {
    status = ERROR;
  }


  RTC->WPR = 0xFF;

  return (status);
}


ErrorStatus RTC_RefClockCmd(FunctionalState NewState)
{
  ErrorStatus status = ERROR;


  assert_param(IS_FUNCTIONAL_STATE(NewState));


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;


  if (RTC_EnterInitMode() == ERROR)
  {
    status = ERROR;
  }
  else
  {
    if (NewState != DISABLE)
    {

      RTC->CR |= RTC_CR_REFCKON;
    }
    else
    {

      RTC->CR &= ~RTC_CR_REFCKON;
    }

    RTC_ExitInitMode();

    status = SUCCESS;
  }


  RTC->WPR = 0xFF;

  return status;
}


void RTC_BypassShadowCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;

  if (NewState != DISABLE)
  {

    RTC->CR |= (uint8_t)RTC_CR_BYPSHAD;
  }
  else
  {

    RTC->CR &= (uint8_t)~RTC_CR_BYPSHAD;
  }


  RTC->WPR = 0xFF;
}


ErrorStatus RTC_SetTime(uint32_t RTC_Format, RTC_TimeTypeDef* RTC_TimeStruct)
{
  uint32_t tmpreg = 0;
  ErrorStatus status = ERROR;


  assert_param(IS_RTC_FORMAT(RTC_Format));

  if (RTC_Format == RTC_Format_BIN)
  {
    if ((RTC->CR & RTC_CR_FMT) != (uint32_t)RESET)
    {
      assert_param(IS_RTC_HOUR12(RTC_TimeStruct->RTC_Hours));
      assert_param(IS_RTC_H12(RTC_TimeStruct->RTC_H12));
    }
    else
    {
      RTC_TimeStruct->RTC_H12 = 0x00;
      assert_param(IS_RTC_HOUR24(RTC_TimeStruct->RTC_Hours));
    }
    assert_param(IS_RTC_MINUTES(RTC_TimeStruct->RTC_Minutes));
    assert_param(IS_RTC_SECONDS(RTC_TimeStruct->RTC_Seconds));
  }
  else
  {
    if ((RTC->CR & RTC_CR_FMT) != (uint32_t)RESET)
    {
      tmpreg = RTC_Bcd2ToByte(RTC_TimeStruct->RTC_Hours);
      assert_param(IS_RTC_HOUR12(tmpreg));
      assert_param(IS_RTC_H12(RTC_TimeStruct->RTC_H12));
    }
    else
    {
      RTC_TimeStruct->RTC_H12 = 0x00;
      assert_param(IS_RTC_HOUR24(RTC_Bcd2ToByte(RTC_TimeStruct->RTC_Hours)));
    }
    assert_param(IS_RTC_MINUTES(RTC_Bcd2ToByte(RTC_TimeStruct->RTC_Minutes)));
    assert_param(IS_RTC_SECONDS(RTC_Bcd2ToByte(RTC_TimeStruct->RTC_Seconds)));
  }


  if (RTC_Format != RTC_Format_BIN)
  {
    tmpreg = (((uint32_t)(RTC_TimeStruct->RTC_Hours) << 16) | \
             ((uint32_t)(RTC_TimeStruct->RTC_Minutes) << 8) | \
             ((uint32_t)RTC_TimeStruct->RTC_Seconds) | \
             ((uint32_t)(RTC_TimeStruct->RTC_H12) << 16));
  }
  else
  {
    tmpreg = (uint32_t)(((uint32_t)RTC_ByteToBcd2(RTC_TimeStruct->RTC_Hours) << 16) | \
                   ((uint32_t)RTC_ByteToBcd2(RTC_TimeStruct->RTC_Minutes) << 8) | \
                   ((uint32_t)RTC_ByteToBcd2(RTC_TimeStruct->RTC_Seconds)) | \
                   (((uint32_t)RTC_TimeStruct->RTC_H12) << 16));
  }


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;


  if (RTC_EnterInitMode() == ERROR)
  {
    status = ERROR;
  }
  else
  {

    RTC->TR = (uint32_t)(tmpreg & RTC_TR_RESERVED_MASK);


    RTC_ExitInitMode();


    if ((RTC->CR & RTC_CR_BYPSHAD) == RESET)
    {
    if(RTC_WaitForSynchro() == ERROR)
    {
      status = ERROR;
    }
    else
    {
      status = SUCCESS;
    }
  }
    else
    {
      status = SUCCESS;
    }
  }

  RTC->WPR = 0xFF;

  return status;
}


void RTC_TimeStructInit(RTC_TimeTypeDef* RTC_TimeStruct)
{

  RTC_TimeStruct->RTC_H12 = RTC_H12_AM;
  RTC_TimeStruct->RTC_Hours = 0;
  RTC_TimeStruct->RTC_Minutes = 0;
  RTC_TimeStruct->RTC_Seconds = 0;
}


void RTC_GetTime(uint32_t RTC_Format, RTC_TimeTypeDef* RTC_TimeStruct)
{
  uint32_t tmpreg = 0;


  assert_param(IS_RTC_FORMAT(RTC_Format));


  tmpreg = (uint32_t)(RTC->TR & RTC_TR_RESERVED_MASK);


  RTC_TimeStruct->RTC_Hours = (uint8_t)((tmpreg & (RTC_TR_HT | RTC_TR_HU)) >> 16);
  RTC_TimeStruct->RTC_Minutes = (uint8_t)((tmpreg & (RTC_TR_MNT | RTC_TR_MNU)) >>8);
  RTC_TimeStruct->RTC_Seconds = (uint8_t)(tmpreg & (RTC_TR_ST | RTC_TR_SU));
  RTC_TimeStruct->RTC_H12 = (uint8_t)((tmpreg & (RTC_TR_PM)) >> 16);


  if (RTC_Format == RTC_Format_BIN)
  {

    RTC_TimeStruct->RTC_Hours = (uint8_t)RTC_Bcd2ToByte(RTC_TimeStruct->RTC_Hours);
    RTC_TimeStruct->RTC_Minutes = (uint8_t)RTC_Bcd2ToByte(RTC_TimeStruct->RTC_Minutes);
    RTC_TimeStruct->RTC_Seconds = (uint8_t)RTC_Bcd2ToByte(RTC_TimeStruct->RTC_Seconds);
  }
}


uint32_t RTC_GetSubSecond(void)
{
  uint32_t tmpreg = 0;


  tmpreg = (uint32_t)(RTC->SSR);


  (void) (RTC->DR);

  return (tmpreg);
}


ErrorStatus RTC_SetDate(uint32_t RTC_Format, RTC_DateTypeDef* RTC_DateStruct)
{
  uint32_t tmpreg = 0;
  ErrorStatus status = ERROR;


  assert_param(IS_RTC_FORMAT(RTC_Format));

  if ((RTC_Format == RTC_Format_BIN) && ((RTC_DateStruct->RTC_Month & 0x10) == 0x10))
  {
    RTC_DateStruct->RTC_Month = (RTC_DateStruct->RTC_Month & (uint32_t)~(0x10)) + 0x0A;
  }
  if (RTC_Format == RTC_Format_BIN)
  {
    assert_param(IS_RTC_YEAR(RTC_DateStruct->RTC_Year));
    assert_param(IS_RTC_MONTH(RTC_DateStruct->RTC_Month));
    assert_param(IS_RTC_DATE(RTC_DateStruct->RTC_Date));
  }
  else
  {
    assert_param(IS_RTC_YEAR(RTC_Bcd2ToByte(RTC_DateStruct->RTC_Year)));
    tmpreg = RTC_Bcd2ToByte(RTC_DateStruct->RTC_Month);
    assert_param(IS_RTC_MONTH(tmpreg));
    tmpreg = RTC_Bcd2ToByte(RTC_DateStruct->RTC_Date);
    assert_param(IS_RTC_DATE(tmpreg));
  }
  assert_param(IS_RTC_WEEKDAY(RTC_DateStruct->RTC_WeekDay));


  if (RTC_Format != RTC_Format_BIN)
  {
    tmpreg = ((((uint32_t)RTC_DateStruct->RTC_Year) << 16) | \
              (((uint32_t)RTC_DateStruct->RTC_Month) << 8) | \
              ((uint32_t)RTC_DateStruct->RTC_Date) | \
              (((uint32_t)RTC_DateStruct->RTC_WeekDay) << 13));
  }
  else
  {
    tmpreg = (((uint32_t)RTC_ByteToBcd2(RTC_DateStruct->RTC_Year) << 16) | \
              ((uint32_t)RTC_ByteToBcd2(RTC_DateStruct->RTC_Month) << 8) | \
              ((uint32_t)RTC_ByteToBcd2(RTC_DateStruct->RTC_Date)) | \
              ((uint32_t)RTC_DateStruct->RTC_WeekDay << 13));
  }


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;


  if (RTC_EnterInitMode() == ERROR)
  {
    status = ERROR;
  }
  else
  {

    RTC->DR = (uint32_t)(tmpreg & RTC_DR_RESERVED_MASK);


    RTC_ExitInitMode();


    if ((RTC->CR & RTC_CR_BYPSHAD) == RESET)
    {
    if(RTC_WaitForSynchro() == ERROR)
    {
      status = ERROR;
    }
    else
    {
      status = SUCCESS;
    }
  }
    else
    {
      status = SUCCESS;
    }
  }

  RTC->WPR = 0xFF;

  return status;
}


void RTC_DateStructInit(RTC_DateTypeDef* RTC_DateStruct)
{

  RTC_DateStruct->RTC_WeekDay = RTC_Weekday_Monday;
  RTC_DateStruct->RTC_Date = 1;
  RTC_DateStruct->RTC_Month = RTC_Month_January;
  RTC_DateStruct->RTC_Year = 0;
}


void RTC_GetDate(uint32_t RTC_Format, RTC_DateTypeDef* RTC_DateStruct)
{
  uint32_t tmpreg = 0;


  assert_param(IS_RTC_FORMAT(RTC_Format));


  tmpreg = (uint32_t)(RTC->DR & RTC_DR_RESERVED_MASK);


  RTC_DateStruct->RTC_Year = (uint8_t)((tmpreg & (RTC_DR_YT | RTC_DR_YU)) >> 16);
  RTC_DateStruct->RTC_Month = (uint8_t)((tmpreg & (RTC_DR_MT | RTC_DR_MU)) >> 8);
  RTC_DateStruct->RTC_Date = (uint8_t)(tmpreg & (RTC_DR_DT | RTC_DR_DU));
  RTC_DateStruct->RTC_WeekDay = (uint8_t)((tmpreg & (RTC_DR_WDU)) >> 13);


  if (RTC_Format == RTC_Format_BIN)
  {

    RTC_DateStruct->RTC_Year = (uint8_t)RTC_Bcd2ToByte(RTC_DateStruct->RTC_Year);
    RTC_DateStruct->RTC_Month = (uint8_t)RTC_Bcd2ToByte(RTC_DateStruct->RTC_Month);
    RTC_DateStruct->RTC_Date = (uint8_t)RTC_Bcd2ToByte(RTC_DateStruct->RTC_Date);
  }
}


void RTC_SetAlarm(uint32_t RTC_Format, uint32_t RTC_Alarm, RTC_AlarmTypeDef* RTC_AlarmStruct)
{
  uint32_t tmpreg = 0;


  assert_param(IS_RTC_FORMAT(RTC_Format));
  assert_param(IS_RTC_ALARM(RTC_Alarm));
  assert_param(IS_ALARM_MASK(RTC_AlarmStruct->RTC_AlarmMask));
  assert_param(IS_RTC_ALARM_DATE_WEEKDAY_SEL(RTC_AlarmStruct->RTC_AlarmDateWeekDaySel));

  if (RTC_Format == RTC_Format_BIN)
  {
    if ((RTC->CR & RTC_CR_FMT) != (uint32_t)RESET)
    {
      assert_param(IS_RTC_HOUR12(RTC_AlarmStruct->RTC_AlarmTime.RTC_Hours));
      assert_param(IS_RTC_H12(RTC_AlarmStruct->RTC_AlarmTime.RTC_H12));
    }
    else
    {
      RTC_AlarmStruct->RTC_AlarmTime.RTC_H12 = 0x00;
      assert_param(IS_RTC_HOUR24(RTC_AlarmStruct->RTC_AlarmTime.RTC_Hours));
    }
    assert_param(IS_RTC_MINUTES(RTC_AlarmStruct->RTC_AlarmTime.RTC_Minutes));
    assert_param(IS_RTC_SECONDS(RTC_AlarmStruct->RTC_AlarmTime.RTC_Seconds));

    if(RTC_AlarmStruct->RTC_AlarmDateWeekDaySel == RTC_AlarmDateWeekDaySel_Date)
    {
      assert_param(IS_RTC_ALARM_DATE_WEEKDAY_DATE(RTC_AlarmStruct->RTC_AlarmDateWeekDay));
    }
    else
    {
      assert_param(IS_RTC_ALARM_DATE_WEEKDAY_WEEKDAY(RTC_AlarmStruct->RTC_AlarmDateWeekDay));
    }
  }
  else
  {
    if ((RTC->CR & RTC_CR_FMT) != (uint32_t)RESET)
    {
      tmpreg = RTC_Bcd2ToByte(RTC_AlarmStruct->RTC_AlarmTime.RTC_Hours);
      assert_param(IS_RTC_HOUR12(tmpreg));
      assert_param(IS_RTC_H12(RTC_AlarmStruct->RTC_AlarmTime.RTC_H12));
    }
    else
    {
      RTC_AlarmStruct->RTC_AlarmTime.RTC_H12 = 0x00;
      assert_param(IS_RTC_HOUR24(RTC_Bcd2ToByte(RTC_AlarmStruct->RTC_AlarmTime.RTC_Hours)));
    }

    assert_param(IS_RTC_MINUTES(RTC_Bcd2ToByte(RTC_AlarmStruct->RTC_AlarmTime.RTC_Minutes)));
    assert_param(IS_RTC_SECONDS(RTC_Bcd2ToByte(RTC_AlarmStruct->RTC_AlarmTime.RTC_Seconds)));

    if(RTC_AlarmStruct->RTC_AlarmDateWeekDaySel == RTC_AlarmDateWeekDaySel_Date)
    {
      tmpreg = RTC_Bcd2ToByte(RTC_AlarmStruct->RTC_AlarmDateWeekDay);
      assert_param(IS_RTC_ALARM_DATE_WEEKDAY_DATE(tmpreg));
    }
    else
    {
      tmpreg = RTC_Bcd2ToByte(RTC_AlarmStruct->RTC_AlarmDateWeekDay);
      assert_param(IS_RTC_ALARM_DATE_WEEKDAY_WEEKDAY(tmpreg));
    }
  }


  if (RTC_Format != RTC_Format_BIN)
  {
    tmpreg = (((uint32_t)(RTC_AlarmStruct->RTC_AlarmTime.RTC_Hours) << 16) | \
              ((uint32_t)(RTC_AlarmStruct->RTC_AlarmTime.RTC_Minutes) << 8) | \
              ((uint32_t)RTC_AlarmStruct->RTC_AlarmTime.RTC_Seconds) | \
              ((uint32_t)(RTC_AlarmStruct->RTC_AlarmTime.RTC_H12) << 16) | \
              ((uint32_t)(RTC_AlarmStruct->RTC_AlarmDateWeekDay) << 24) | \
              ((uint32_t)RTC_AlarmStruct->RTC_AlarmDateWeekDaySel) | \
              ((uint32_t)RTC_AlarmStruct->RTC_AlarmMask));
  }
  else
  {
    tmpreg = (((uint32_t)RTC_ByteToBcd2(RTC_AlarmStruct->RTC_AlarmTime.RTC_Hours) << 16) | \
              ((uint32_t)RTC_ByteToBcd2(RTC_AlarmStruct->RTC_AlarmTime.RTC_Minutes) << 8) | \
              ((uint32_t)RTC_ByteToBcd2(RTC_AlarmStruct->RTC_AlarmTime.RTC_Seconds)) | \
              ((uint32_t)(RTC_AlarmStruct->RTC_AlarmTime.RTC_H12) << 16) | \
              ((uint32_t)RTC_ByteToBcd2(RTC_AlarmStruct->RTC_AlarmDateWeekDay) << 24) | \
              ((uint32_t)RTC_AlarmStruct->RTC_AlarmDateWeekDaySel) | \
              ((uint32_t)RTC_AlarmStruct->RTC_AlarmMask));
  }


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;


  if (RTC_Alarm == RTC_Alarm_A)
  {
    RTC->ALRMAR = (uint32_t)tmpreg;
  }
  else
  {
    RTC->ALRMBR = (uint32_t)tmpreg;
  }


  RTC->WPR = 0xFF;
}


void RTC_AlarmStructInit(RTC_AlarmTypeDef* RTC_AlarmStruct)
{

  RTC_AlarmStruct->RTC_AlarmTime.RTC_H12 = RTC_H12_AM;
  RTC_AlarmStruct->RTC_AlarmTime.RTC_Hours = 0;
  RTC_AlarmStruct->RTC_AlarmTime.RTC_Minutes = 0;
  RTC_AlarmStruct->RTC_AlarmTime.RTC_Seconds = 0;


  RTC_AlarmStruct->RTC_AlarmDateWeekDaySel = RTC_AlarmDateWeekDaySel_Date;
  RTC_AlarmStruct->RTC_AlarmDateWeekDay = 1;


  RTC_AlarmStruct->RTC_AlarmMask = RTC_AlarmMask_None;
}


void RTC_GetAlarm(uint32_t RTC_Format, uint32_t RTC_Alarm, RTC_AlarmTypeDef* RTC_AlarmStruct)
{
  uint32_t tmpreg = 0;


  assert_param(IS_RTC_FORMAT(RTC_Format));
  assert_param(IS_RTC_ALARM(RTC_Alarm));


  if (RTC_Alarm == RTC_Alarm_A)
  {
    tmpreg = (uint32_t)(RTC->ALRMAR);
  }
  else
  {
    tmpreg = (uint32_t)(RTC->ALRMBR);
  }


  RTC_AlarmStruct->RTC_AlarmTime.RTC_Hours = (uint32_t)((tmpreg & (RTC_ALRMAR_HT | \
                                                     RTC_ALRMAR_HU)) >> 16);
  RTC_AlarmStruct->RTC_AlarmTime.RTC_Minutes = (uint32_t)((tmpreg & (RTC_ALRMAR_MNT | \
                                                     RTC_ALRMAR_MNU)) >> 8);
  RTC_AlarmStruct->RTC_AlarmTime.RTC_Seconds = (uint32_t)(tmpreg & (RTC_ALRMAR_ST | \
                                                     RTC_ALRMAR_SU));
  RTC_AlarmStruct->RTC_AlarmTime.RTC_H12 = (uint32_t)((tmpreg & RTC_ALRMAR_PM) >> 16);
  RTC_AlarmStruct->RTC_AlarmDateWeekDay = (uint32_t)((tmpreg & (RTC_ALRMAR_DT | RTC_ALRMAR_DU)) >> 24);
  RTC_AlarmStruct->RTC_AlarmDateWeekDaySel = (uint32_t)(tmpreg & RTC_ALRMAR_WDSEL);
  RTC_AlarmStruct->RTC_AlarmMask = (uint32_t)(tmpreg & RTC_AlarmMask_All);

  if (RTC_Format == RTC_Format_BIN)
  {
    RTC_AlarmStruct->RTC_AlarmTime.RTC_Hours = RTC_Bcd2ToByte(RTC_AlarmStruct-> \
                                                        RTC_AlarmTime.RTC_Hours);
    RTC_AlarmStruct->RTC_AlarmTime.RTC_Minutes = RTC_Bcd2ToByte(RTC_AlarmStruct-> \
                                                        RTC_AlarmTime.RTC_Minutes);
    RTC_AlarmStruct->RTC_AlarmTime.RTC_Seconds = RTC_Bcd2ToByte(RTC_AlarmStruct-> \
                                                        RTC_AlarmTime.RTC_Seconds);
    RTC_AlarmStruct->RTC_AlarmDateWeekDay = RTC_Bcd2ToByte(RTC_AlarmStruct->RTC_AlarmDateWeekDay);
  }
}


ErrorStatus RTC_AlarmCmd(uint32_t RTC_Alarm, FunctionalState NewState)
{
  __IO uint32_t alarmcounter = 0x00;
  uint32_t alarmstatus = 0x00;
  ErrorStatus status = ERROR;


  assert_param(IS_RTC_CMD_ALARM(RTC_Alarm));
  assert_param(IS_FUNCTIONAL_STATE(NewState));


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;


  if (NewState != DISABLE)
  {
    RTC->CR |= (uint32_t)RTC_Alarm;

    status = SUCCESS;
  }
  else
  {

    RTC->CR &= (uint32_t)~RTC_Alarm;


    do
    {
      alarmstatus = RTC->ISR & (RTC_Alarm >> 8);
      alarmcounter++;
    } while((alarmcounter != INITMODE_TIMEOUT) && (alarmstatus == 0x00));

    if ((RTC->ISR & (RTC_Alarm >> 8)) == RESET)
    {
      status = ERROR;
    }
    else
    {
      status = SUCCESS;
    }
  }


  RTC->WPR = 0xFF;

  return status;
}


void RTC_AlarmSubSecondConfig(uint32_t RTC_Alarm, uint32_t RTC_AlarmSubSecondValue, uint32_t RTC_AlarmSubSecondMask)
{
  uint32_t tmpreg = 0;


  assert_param(IS_RTC_ALARM(RTC_Alarm));
  assert_param(IS_RTC_ALARM_SUB_SECOND_VALUE(RTC_AlarmSubSecondValue));
  assert_param(IS_RTC_ALARM_SUB_SECOND_MASK(RTC_AlarmSubSecondMask));


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;


  tmpreg = (uint32_t) (uint32_t)(RTC_AlarmSubSecondValue) | (uint32_t)(RTC_AlarmSubSecondMask);

  if (RTC_Alarm == RTC_Alarm_A)
  {

    RTC->ALRMASSR = tmpreg;
  }
  else
  {

    RTC->ALRMBSSR = tmpreg;
  }


  RTC->WPR = 0xFF;

}


uint32_t RTC_GetAlarmSubSecond(uint32_t RTC_Alarm)
{
  uint32_t tmpreg = 0;


  if (RTC_Alarm == RTC_Alarm_A)
  {
    tmpreg = (uint32_t)((RTC->ALRMASSR) & RTC_ALRMASSR_SS);
  }
  else
  {
    tmpreg = (uint32_t)((RTC->ALRMBSSR) & RTC_ALRMBSSR_SS);
  }

  return (tmpreg);
}


void RTC_WakeUpClockConfig(uint32_t RTC_WakeUpClock)
{

  assert_param(IS_RTC_WAKEUP_CLOCK(RTC_WakeUpClock));


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;


  RTC->CR &= (uint32_t)~RTC_CR_WUCKSEL;


  RTC->CR |= (uint32_t)RTC_WakeUpClock;


  RTC->WPR = 0xFF;
}


void RTC_SetWakeUpCounter(uint32_t RTC_WakeUpCounter)
{

  assert_param(IS_RTC_WAKEUP_COUNTER(RTC_WakeUpCounter));


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;


  RTC->WUTR = (uint32_t)RTC_WakeUpCounter;


  RTC->WPR = 0xFF;
}


uint32_t RTC_GetWakeUpCounter(void)
{

  return ((uint32_t)(RTC->WUTR & RTC_WUTR_WUT));
}


ErrorStatus RTC_WakeUpCmd(FunctionalState NewState)
{
  __IO uint32_t wutcounter = 0x00;
  uint32_t wutwfstatus = 0x00;
  ErrorStatus status = ERROR;


  assert_param(IS_FUNCTIONAL_STATE(NewState));


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;

  if (NewState != DISABLE)
  {

    RTC->CR |= (uint32_t)RTC_CR_WUTE;
    status = SUCCESS;
  }
  else
  {

    RTC->CR &= (uint32_t)~RTC_CR_WUTE;

    do
    {
      wutwfstatus = RTC->ISR & RTC_ISR_WUTWF;
      wutcounter++;
    } while((wutcounter != INITMODE_TIMEOUT) && (wutwfstatus == 0x00));

    if ((RTC->ISR & RTC_ISR_WUTWF) == RESET)
    {
      status = ERROR;
    }
    else
    {
      status = SUCCESS;
    }
  }


  RTC->WPR = 0xFF;

  return status;
}


void RTC_DayLightSavingConfig(uint32_t RTC_DayLightSaving, uint32_t RTC_StoreOperation)
{

  assert_param(IS_RTC_DAYLIGHT_SAVING(RTC_DayLightSaving));
  assert_param(IS_RTC_STORE_OPERATION(RTC_StoreOperation));


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;


  RTC->CR &= (uint32_t)~(RTC_CR_BCK);


  RTC->CR |= (uint32_t)(RTC_DayLightSaving | RTC_StoreOperation);


  RTC->WPR = 0xFF;
}


uint32_t RTC_GetStoreOperation(void)
{
  return (RTC->CR & RTC_CR_BCK);
}


void RTC_OutputConfig(uint32_t RTC_Output, uint32_t RTC_OutputPolarity)
{

  assert_param(IS_RTC_OUTPUT(RTC_Output));
  assert_param(IS_RTC_OUTPUT_POL(RTC_OutputPolarity));


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;


  RTC->CR &= (uint32_t)~(RTC_CR_OSEL | RTC_CR_POL);


  RTC->CR |= (uint32_t)(RTC_Output | RTC_OutputPolarity);


  RTC->WPR = 0xFF;
}


ErrorStatus RTC_CoarseCalibConfig(uint32_t RTC_CalibSign, uint32_t Value)
{
  ErrorStatus status = ERROR;


  assert_param(IS_RTC_CALIB_SIGN(RTC_CalibSign));
  assert_param(IS_RTC_CALIB_VALUE(Value));


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;


  if (RTC_EnterInitMode() == ERROR)
  {
    status = ERROR;
  }
  else
  {

    RTC->CALIBR = (uint32_t)(RTC_CalibSign | Value);

    RTC_ExitInitMode();

    status = SUCCESS;
  }


  RTC->WPR = 0xFF;

  return status;
}


ErrorStatus RTC_CoarseCalibCmd(FunctionalState NewState)
{
  ErrorStatus status = ERROR;


  assert_param(IS_FUNCTIONAL_STATE(NewState));


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;


  if (RTC_EnterInitMode() == ERROR)
  {
    status =  ERROR;
  }
  else
  {
    if (NewState != DISABLE)
    {

      RTC->CR |= (uint32_t)RTC_CR_DCE;
    }
    else
    {

      RTC->CR &= (uint32_t)~RTC_CR_DCE;
    }

    RTC_ExitInitMode();

    status = SUCCESS;
  }


  RTC->WPR = 0xFF;

  return status;
}


void RTC_CalibOutputCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;

  if (NewState != DISABLE)
  {

    RTC->CR |= (uint32_t)RTC_CR_COE;
  }
  else
  {

    RTC->CR &= (uint32_t)~RTC_CR_COE;
  }


  RTC->WPR = 0xFF;
}


void RTC_CalibOutputConfig(uint32_t RTC_CalibOutput)
{

  assert_param(IS_RTC_CALIB_OUTPUT(RTC_CalibOutput));


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;


  RTC->CR &= (uint32_t)~(RTC_CR_COSEL);


  RTC->CR |= (uint32_t)RTC_CalibOutput;


  RTC->WPR = 0xFF;
}


ErrorStatus RTC_SmoothCalibConfig(uint32_t RTC_SmoothCalibPeriod,
                                  uint32_t RTC_SmoothCalibPlusPulses,
                                  uint32_t RTC_SmouthCalibMinusPulsesValue)
{
  ErrorStatus status = ERROR;
  uint32_t recalpfcount = 0;


  assert_param(IS_RTC_SMOOTH_CALIB_PERIOD(RTC_SmoothCalibPeriod));
  assert_param(IS_RTC_SMOOTH_CALIB_PLUS(RTC_SmoothCalibPlusPulses));
  assert_param(IS_RTC_SMOOTH_CALIB_MINUS(RTC_SmouthCalibMinusPulsesValue));


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;


  if ((RTC->ISR & RTC_ISR_RECALPF) != RESET)
  {

    while (((RTC->ISR & RTC_ISR_RECALPF) != RESET) && (recalpfcount != RECALPF_TIMEOUT))
    {
      recalpfcount++;
    }
  }


  if ((RTC->ISR & RTC_ISR_RECALPF) == RESET)
  {

    RTC->CALR = (uint32_t)((uint32_t)RTC_SmoothCalibPeriod | (uint32_t)RTC_SmoothCalibPlusPulses | (uint32_t)RTC_SmouthCalibMinusPulsesValue);

    status = SUCCESS;
  }
  else
  {
    status = ERROR;
  }


  RTC->WPR = 0xFF;

  return (ErrorStatus)(status);
}


void RTC_TimeStampCmd(uint32_t RTC_TimeStampEdge, FunctionalState NewState)
{
  uint32_t tmpreg = 0;


  assert_param(IS_RTC_TIMESTAMP_EDGE(RTC_TimeStampEdge));
  assert_param(IS_FUNCTIONAL_STATE(NewState));


  tmpreg = (uint32_t)(RTC->CR & (uint32_t)~(RTC_CR_TSEDGE | RTC_CR_TSE));


  if (NewState != DISABLE)
  {
    tmpreg |= (uint32_t)(RTC_TimeStampEdge | RTC_CR_TSE);
  }
  else
  {
    tmpreg |= (uint32_t)(RTC_TimeStampEdge);
  }


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;


  RTC->CR = (uint32_t)tmpreg;


  RTC->WPR = 0xFF;
}


void RTC_GetTimeStamp(uint32_t RTC_Format, RTC_TimeTypeDef* RTC_StampTimeStruct,
                                      RTC_DateTypeDef* RTC_StampDateStruct)
{
  uint32_t tmptime = 0, tmpdate = 0;


  assert_param(IS_RTC_FORMAT(RTC_Format));


  tmptime = (uint32_t)(RTC->TSTR & RTC_TR_RESERVED_MASK);
  tmpdate = (uint32_t)(RTC->TSDR & RTC_DR_RESERVED_MASK);


  RTC_StampTimeStruct->RTC_Hours = (uint8_t)((tmptime & (RTC_TR_HT | RTC_TR_HU)) >> 16);
  RTC_StampTimeStruct->RTC_Minutes = (uint8_t)((tmptime & (RTC_TR_MNT | RTC_TR_MNU)) >> 8);
  RTC_StampTimeStruct->RTC_Seconds = (uint8_t)(tmptime & (RTC_TR_ST | RTC_TR_SU));
  RTC_StampTimeStruct->RTC_H12 = (uint8_t)((tmptime & (RTC_TR_PM)) >> 16);


  RTC_StampDateStruct->RTC_Year = 0;
  RTC_StampDateStruct->RTC_Month = (uint8_t)((tmpdate & (RTC_DR_MT | RTC_DR_MU)) >> 8);
  RTC_StampDateStruct->RTC_Date = (uint8_t)(tmpdate & (RTC_DR_DT | RTC_DR_DU));
  RTC_StampDateStruct->RTC_WeekDay = (uint8_t)((tmpdate & (RTC_DR_WDU)) >> 13);


  if (RTC_Format == RTC_Format_BIN)
  {

    RTC_StampTimeStruct->RTC_Hours = (uint8_t)RTC_Bcd2ToByte(RTC_StampTimeStruct->RTC_Hours);
    RTC_StampTimeStruct->RTC_Minutes = (uint8_t)RTC_Bcd2ToByte(RTC_StampTimeStruct->RTC_Minutes);
    RTC_StampTimeStruct->RTC_Seconds = (uint8_t)RTC_Bcd2ToByte(RTC_StampTimeStruct->RTC_Seconds);


    RTC_StampDateStruct->RTC_Month = (uint8_t)RTC_Bcd2ToByte(RTC_StampDateStruct->RTC_Month);
    RTC_StampDateStruct->RTC_Date = (uint8_t)RTC_Bcd2ToByte(RTC_StampDateStruct->RTC_Date);
    RTC_StampDateStruct->RTC_WeekDay = (uint8_t)RTC_Bcd2ToByte(RTC_StampDateStruct->RTC_WeekDay);
  }
}


uint32_t RTC_GetTimeStampSubSecond(void)
{

  return (uint32_t)(RTC->TSSSR);
}


void RTC_TamperTriggerConfig(uint32_t RTC_Tamper, uint32_t RTC_TamperTrigger)
{

  assert_param(IS_RTC_TAMPER(RTC_Tamper));
  assert_param(IS_RTC_TAMPER_TRIGGER(RTC_TamperTrigger));

  if (RTC_TamperTrigger == RTC_TamperTrigger_RisingEdge)
  {

    RTC->TAFCR &= (uint32_t)((uint32_t)~(RTC_Tamper << 1));
  }
  else
  {

    RTC->TAFCR |= (uint32_t)(RTC_Tamper << 1);
  }
}


void RTC_TamperCmd(uint32_t RTC_Tamper, FunctionalState NewState)
{

  assert_param(IS_RTC_TAMPER(RTC_Tamper));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    RTC->TAFCR |= (uint32_t)RTC_Tamper;
  }
  else
  {

    RTC->TAFCR &= (uint32_t)~RTC_Tamper;
  }
}


void RTC_TamperFilterConfig(uint32_t RTC_TamperFilter)
{

  assert_param(IS_RTC_TAMPER_FILTER(RTC_TamperFilter));


  RTC->TAFCR &= (uint32_t)~(RTC_TAFCR_TAMPFLT);


  RTC->TAFCR |= (uint32_t)RTC_TamperFilter;
}


void RTC_TamperSamplingFreqConfig(uint32_t RTC_TamperSamplingFreq)
{

  assert_param(IS_RTC_TAMPER_SAMPLING_FREQ(RTC_TamperSamplingFreq));


  RTC->TAFCR &= (uint32_t)~(RTC_TAFCR_TAMPFREQ);


  RTC->TAFCR |= (uint32_t)RTC_TamperSamplingFreq;
}


void RTC_TamperPinsPrechargeDuration(uint32_t RTC_TamperPrechargeDuration)
{

  assert_param(IS_RTC_TAMPER_PRECHARGE_DURATION(RTC_TamperPrechargeDuration));


  RTC->TAFCR &= (uint32_t)~(RTC_TAFCR_TAMPPRCH);


  RTC->TAFCR |= (uint32_t)RTC_TamperPrechargeDuration;
}


void RTC_TimeStampOnTamperDetectionCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {

    RTC->TAFCR |= (uint32_t)RTC_TAFCR_TAMPTS;
  }
  else
  {

    RTC->TAFCR &= (uint32_t)~RTC_TAFCR_TAMPTS;
  }
}


void RTC_TamperPullUpCmd(FunctionalState NewState)
{

  assert_param(IS_FUNCTIONAL_STATE(NewState));

 if (NewState != DISABLE)
  {

    RTC->TAFCR &= (uint32_t)~RTC_TAFCR_TAMPPUDIS;
  }
  else
  {

    RTC->TAFCR |= (uint32_t)RTC_TAFCR_TAMPPUDIS;
  }
}


void RTC_WriteBackupRegister(uint32_t RTC_BKP_DR, uint32_t Data)
{
  __IO uint32_t tmp = 0;


  assert_param(IS_RTC_BKP(RTC_BKP_DR));

  tmp = RTC_BASE + 0x50;
  tmp += (RTC_BKP_DR * 4);


  *(__IO uint32_t *)tmp = (uint32_t)Data;
}


uint32_t RTC_ReadBackupRegister(uint32_t RTC_BKP_DR)
{
  __IO uint32_t tmp = 0;


  assert_param(IS_RTC_BKP(RTC_BKP_DR));

  tmp = RTC_BASE + 0x50;
  tmp += (RTC_BKP_DR * 4);


  return (*(__IO uint32_t *)tmp);
}


void RTC_TamperPinSelection(uint32_t RTC_TamperPin)
{

  assert_param(IS_RTC_TAMPER_PIN(RTC_TamperPin));

  RTC->TAFCR &= (uint32_t)~(RTC_TAFCR_TAMPINSEL);
  RTC->TAFCR |= (uint32_t)(RTC_TamperPin);
}


void RTC_TimeStampPinSelection(uint32_t RTC_TimeStampPin)
{

  assert_param(IS_RTC_TIMESTAMP_PIN(RTC_TimeStampPin));

  RTC->TAFCR &= (uint32_t)~(RTC_TAFCR_TSINSEL);
  RTC->TAFCR |= (uint32_t)(RTC_TimeStampPin);
}


void RTC_OutputTypeConfig(uint32_t RTC_OutputType)
{

  assert_param(IS_RTC_OUTPUT_TYPE(RTC_OutputType));

  RTC->TAFCR &= (uint32_t)~(RTC_TAFCR_ALARMOUTTYPE);
  RTC->TAFCR |= (uint32_t)(RTC_OutputType);
}


ErrorStatus RTC_SynchroShiftConfig(uint32_t RTC_ShiftAdd1S, uint32_t RTC_ShiftSubFS)
{
  ErrorStatus status = ERROR;
  uint32_t shpfcount = 0;


  assert_param(IS_RTC_SHIFT_ADD1S(RTC_ShiftAdd1S));
  assert_param(IS_RTC_SHIFT_SUBFS(RTC_ShiftSubFS));


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;


  if ((RTC->ISR & RTC_ISR_SHPF) != RESET)
  {

    while (((RTC->ISR & RTC_ISR_SHPF) != RESET) && (shpfcount != SHPF_TIMEOUT))
    {
      shpfcount++;
    }
  }


  if ((RTC->ISR & RTC_ISR_SHPF) == RESET)
  {

    if((RTC->CR & RTC_CR_REFCKON) == RESET)
    {

      RTC->SHIFTR = (uint32_t)(uint32_t)(RTC_ShiftSubFS) | (uint32_t)(RTC_ShiftAdd1S);

      if(RTC_WaitForSynchro() == ERROR)
      {
        status = ERROR;
      }
      else
      {
        status = SUCCESS;
      }
    }
    else
    {
      status = ERROR;
    }
  }
  else
  {
    status = ERROR;
  }


  RTC->WPR = 0xFF;

  return (ErrorStatus)(status);
}


void RTC_ITConfig(uint32_t RTC_IT, FunctionalState NewState)
{

  assert_param(IS_RTC_CONFIG_IT(RTC_IT));
  assert_param(IS_FUNCTIONAL_STATE(NewState));


  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;

  if (NewState != DISABLE)
  {

    RTC->CR |= (uint32_t)(RTC_IT & ~RTC_TAFCR_TAMPIE);

    RTC->TAFCR |= (uint32_t)(RTC_IT & RTC_TAFCR_TAMPIE);
  }
  else
  {

    RTC->CR &= (uint32_t)~(RTC_IT & (uint32_t)~RTC_TAFCR_TAMPIE);

    RTC->TAFCR &= (uint32_t)~(RTC_IT & RTC_TAFCR_TAMPIE);
  }

  RTC->WPR = 0xFF;
}


FlagStatus RTC_GetFlagStatus(uint32_t RTC_FLAG)
{
  FlagStatus bitstatus = RESET;
  uint32_t tmpreg = 0;


  assert_param(IS_RTC_GET_FLAG(RTC_FLAG));


  tmpreg = (uint32_t)(RTC->ISR & RTC_FLAGS_MASK);


  if ((tmpreg & RTC_FLAG) != (uint32_t)RESET)
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }
  return bitstatus;
}


void RTC_ClearFlag(uint32_t RTC_FLAG)
{

  assert_param(IS_RTC_CLEAR_FLAG(RTC_FLAG));


  RTC->ISR = (uint32_t)((uint32_t)(~((RTC_FLAG | RTC_ISR_INIT)& 0x0000FFFF) | (uint32_t)(RTC->ISR & RTC_ISR_INIT)));
}


ITStatus RTC_GetITStatus(uint32_t RTC_IT)
{
  ITStatus bitstatus = RESET;
  uint32_t tmpreg = 0, enablestatus = 0;


  assert_param(IS_RTC_GET_IT(RTC_IT));


  tmpreg = (uint32_t)(RTC->TAFCR & (RTC_TAFCR_TAMPIE));


  enablestatus = (uint32_t)((RTC->CR & RTC_IT) | (tmpreg & (RTC_IT >> 15)));


  tmpreg = (uint32_t)((RTC->ISR & (uint32_t)(RTC_IT >> 4)));


  if ((enablestatus != (uint32_t)RESET) && ((tmpreg & 0x0000FFFF) != (uint32_t)RESET))
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }
  return bitstatus;
}


void RTC_ClearITPendingBit(uint32_t RTC_IT)
{
  uint32_t tmpreg = 0;


  assert_param(IS_RTC_CLEAR_IT(RTC_IT));


  tmpreg = (uint32_t)(RTC_IT >> 4);


  RTC->ISR = (uint32_t)((uint32_t)(~((tmpreg | RTC_ISR_INIT)& 0x0000FFFF) | (uint32_t)(RTC->ISR & RTC_ISR_INIT)));
}


static uint8_t RTC_ByteToBcd2(uint8_t Value)
{
  uint8_t bcdhigh = 0;

  while (Value >= 10)
  {
    bcdhigh++;
    Value -= 10;
  }

  return  ((uint8_t)(bcdhigh << 4) | Value);
}


static uint8_t RTC_Bcd2ToByte(uint8_t Value)
{
  uint8_t tmp = 0;
  tmp = ((uint8_t)(Value & (uint8_t)0xF0) >> (uint8_t)0x4) * 10;
  return (tmp + (Value & (uint8_t)0x0F));
}
