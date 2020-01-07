/**************************************************************************/ /**
 * @file
 * @version  V3.00
 * $Revision: 2 $
 * $Date: 16/12/28 1:08p $
 * @brief    SAU configuration for secure/nonsecure region settings.
 *
 * @note
 * Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 *
 ******************************************************************************/

#ifndef PARTITION_M2351
#define PARTITION_M2351

/*
//-------- <<< Use Configuration Wizard in Context Menu >>> -----------------
*/

/*
    SRAMNSSET
*/
/*
// Bit 0..16
// <o.0..16> Secure SRAM Size              <0=> 0 KB
//                                         <0x2000=> 8KB
//                                         <0x4000=> 16KB
//                                         <0x6000=> 24KB
//                                         <0x8000=> 32KB
//                                         <0xa000=> 40KB
//                                         <0xc000=> 48KB
//                                         <0xe000=> 56KB
//                                         <0x10000=> 64KB
//                                         <0x12000=> 72KB
//                                         <0x14000=> 80KB
//                                         <0x16000=> 88KB
//                                         <0x18000=> 96KB
*/
#define SCU_SECURE_SRAM_SIZE 0x4000
#define NON_SECURE_SRAM_BASE (0x30000000 + SCU_SECURE_SRAM_SIZE)

/*--------------------------------------------------------------------------------------------------------*/

/*
    NSBA
*/
#define FMC_INIT_NSBA 1
/*
//     <o>Secure Flash ROM Size <0x800-0x7FFFF:0x800>
*/

#define FMC_SECURE_ROM_SIZE 0x10000

#define FMC_NON_SECURE_BASE (0x10000000 + FMC_SECURE_ROM_SIZE)

/*--------------------------------------------------------------------------------------------------------*/

/*
// <h> Peripheral Secure Attribution Configuration
*/

/*
    PNSSET0
*/
/*
// Module 0..31
//   <o.9>  USBH       <0=> Secure <1=> Non-Secure
//   <o.13>  SD0   <0=> Secure <1=> Non-Secure
//   <o.16>  EBI    <0=> Secure <1=> Non-Secure
//   <o.24>  PDMA1      <0=> Secure <1=> Non-Secure
*/
#define SCU_INIT_PNSSET0_VAL 0x1012200
/*
    PNSSET1
*/
/*
// Module 0..31
//   <o.17>  CRC       <0=> Secure <1=> Non-Secure
//   <o.18>  CRPT   <0=> Secure <1=> Non-Secure
*/
#define SCU_INIT_PNSSET1_VAL 0x60000
/*
    PNSSET2
*/
/*
// Module 0..31
//   <o.1>  RTC       <0=> Secure <1=> Non-Secure
//   <o.3>  EADC   <0=> Secure <1=> Non-Secure
//   <o.5>  ACMP01     <0=> Secure <1=> Non-Secure
//
//   <o.7>  DAC      <0=> Secure <1=> Non-Secure
//   <o.8>  I2S0      <0=> Secure <1=> Non-Secure
//   <o.13>  OTG      <0=> Secure <1=> Non-Secure
//   <o.17>  TMR23      <0=> Secure <1=> Non-Secure
//   <h> EPWM
//   <o.24>  EPWM0      <0=> Secure <1=> Non-Secure
//   <o.25>  EPWM1      <0=> Secure <1=> Non-Secure
//   <o.26>  BPWM0      <0=> Secure <1=> Non-Secure
//   <o.27>  BPWM1      <0=> Secure <1=> Non-Secure
//   </h>
*/
#define SCU_INIT_PNSSET2_VAL 0xf0221a8
/*
    PNSSET3
*/
/*
// Module 0..31
//   <h>  SPI
//   <o.0>  SPI0       <0=> Secure <1=> Non-Secure
//   <o.1>  SPI1   <0=> Secure <1=> Non-Secure
//   <o.2>  SPI2      <0=> Secure <1=> Non-Secure
//   <o.3>  SPI3    <0=> Secure <1=> Non-Secure
//   <o.4>  SPI4      <0=> Secure <1=> Non-Secure
//   <o.5>  SPI5      <0=> Secure <1=> Non-Secure
//   </h>
//   <h> UART
//   <o.16>  UART0      <0=> Secure <1=> Non-Secure
//   <o.17>  UART1      <0=> Secure <1=> Non-Secure
//   <o.18>  UART2      <0=> Secure <1=> Non-Secure
//   <o.19>  UART3      <0=> Secure <1=> Non-Secure
//   <o.20>  UART4      <0=> Secure <1=> Non-Secure
//   <o.21>  UART5      <0=> Secure <1=> Non-Secure
//   </h>
*/
#define SCU_INIT_PNSSET3_VAL 0x3f001f
/*
    PNSSET4
*/
/*
// Module 0..31
//   <h> I2C
//   <o.0>  I2C0       <0=> Secure <1=> Non-Secure
//   <o.1>  I2C1   <0=> Secure <1=> Non-Secure
//   <o.2>  I2C2      <0=> Secure <1=> Non-Secure
//   </h>
//   <h> Smart Card
//   <o.16>  SC0      <0=> Secure <1=> Non-Secure
//   <o.17>  SC1      <0=> Secure <1=> Non-Secure
//   <o.18>  SC2      <0=> Secure <1=> Non-Secure
//   </h>
*/
#define SCU_INIT_PNSSET4_VAL 0x70007
/*
    PNSSET5
*/
/*
// Module 0..31
//   <o.0>  CAN0       <0=> Secure <1=> Non-Secure
//   <h> QEI
//   <o.16>  QEI0   <0=> Secure <1=> Non-Secure
//   <o.17>  QEI1      <0=> Secure <1=> Non-Secure
//   </h>
//   <h> ECAP
//   <o.20>  ECAP0    <0=> Secure <1=> Non-Secure
//   <o.21>  ECAP1      <0=> Secure <1=> Non-Secure
//   </h>
//   <o.23>  DSRC    <0=> Secure <1=> Non-Secure
//
//   <o.25>  TRNG    <0=> Secure <1=> Non-Secure
*/
#define SCU_INIT_PNSSET5_VAL 0x2330001
/*
    PNSSET6
*/
/*
// Module 0..31
//   <o.0>  USBD       <0=> Secure <1=> Non-Secure
//   <h> USCI
//   <o.16>  USCI0   <0=> Secure <1=> Non-Secure
//   <o.17>  USCI1      <0=> Secure <1=> Non-Secure
//   </h>
*/
#define SCU_INIT_PNSSET6_VAL 0x30001
/*
// </h>
*/

/*
// <h> GPIO Secure Attribution Configuration
*/

/*
    IONSSET
*/
/*
// Bit 0..31
//   <o.0>  PA       <0=> Secure <1=> Non-Secure
//   <o.1>  PB   <0=> Secure <1=> Non-Secure
//   <o.2>  PC      <0=> Secure <1=> Non-Secure
//   <o.3>  PD    <0=> Secure <1=> Non-Secure
//   <o.4>  PE      <0=> Secure <1=> Non-Secure
//   <o.5>  PF      <0=> Secure <1=> Non-Secure
//   <o.6>  PG      <0=> Secure <1=> Non-Secure
//   <o.7>  PH      <0=> Secure <1=> Non-Secure
*/
#define SCU_INIT_IONSSET_VAL 0xff
/*
// </h>
*/

/* ---------------------------------------------------------------------------------------------------- */

/*
// <e>Secure Attribute Unit (SAU) Control
*/
#define SAU_INIT_CTRL 1

/*
//   <q> Enable SAU
//   <i> To enable Secure Attribute Unit (SAU).
*/
#define SAU_INIT_CTRL_ENABLE 1

/*
//   <o> All Memory Attribute When SAU is disabled
//     <0=> All Memory is Secure
//     <1=> All Memory is Non-Secure
//   <i> To set the ALLNS bit in SAU CTRL.
//   <i> When all Memory is Non-Secure (ALLNS is 1), IDAU can override memory map configuration.
*/
#define SAU_INIT_CTRL_ALLNS 0

/*
// </e>
*/

/*
// <h>Enable and Set Secure/Non-Secure region
*/
#define SAU_REGIONS_MAX 8 /* Max. number of SAU regions */

/*
//   <e>SAU Region 0
//   <i> Setup SAU Region 0
*/
#define SAU_INIT_REGION0 1
/*
//     <o>Start Address <0-0xFFFFFFE0>
*/
#define SAU_INIT_START0 0xf800 /* start address of SAU region 0 */
/*
//     <o>End Address <0x1F-0xFFFFFFFF>
*/
#define SAU_INIT_END0 0xffff /* end address of SAU region 0 */
/*
//     <o>Region is
//         <0=>Non-Secure
//         <1=>Secure, Non-Secure Callable
*/
#define SAU_INIT_NSC0 1
/*
//   </e>
*/

/*
//   <e>SAU Region 1
//   <i> Setup SAU Region 1
*/
#define SAU_INIT_REGION1 0
/*
//     <o>Start Address <0-0xFFFFFFE0>
*/
#define SAU_INIT_START1 0x10040000
/*
//     <o>End Address <0x1F-0xFFFFFFFF>
*/
#define SAU_INIT_END1 0x1007FFFF
/*
//     <o>Region is
//         <0=>Non-Secure
//         <1=>Secure, Non-Secure Callable
*/
#define SAU_INIT_NSC1 0
/*
//   </e>
*/

/*
//   <e>SAU Region 2
//   <i> Setup SAU Region 2
*/
#define SAU_INIT_REGION2 0
/*
//     <o>Start Address <0-0xFFFFFFE0>
*/
#define SAU_INIT_START2 0x2000F000
/*
//     <o>End Address <0x1F-0xFFFFFFFF>
*/
#define SAU_INIT_END2 0x2000FFFF
/*
//     <o>Region is
//         <0=>Non-Secure
//         <1=>Secure, Non-Secure Callable
*/
#define SAU_INIT_NSC2 1
/*
//   </e>
*/

/*
//   <e>SAU Region 3
//   <i> Setup SAU Region 3
*/
#define SAU_INIT_REGION3 0
/*
//     <o>Start Address <0-0xFFFFFFE0>
*/
#define SAU_INIT_START3 0xf800
/*
//     <o>End Address <0x1F-0xFFFFFFFF>
*/
#define SAU_INIT_END3 0xffff
/*
//     <o>Region is
//         <0=>Non-Secure
//         <1=>Secure, Non-Secure Callable
*/
#define SAU_INIT_NSC3 1
/*
//   </e>
*/

/*
   <e>SAU Region 4
   <i> Setup SAU Region 4
*/
#define SAU_INIT_REGION4 1
/*
     <o>Start Address <0-0xFFFFFFE0>
*/
#define SAU_INIT_START4 FMC_NON_SECURE_BASE /* start address of SAU region 4 */

/*
     <o>End Address <0x1F-0xFFFFFFFF>
*/
#define SAU_INIT_END4 0x1007FFFF /* end address of SAU region 4 */

/*
     <o>Region is
         <0=>Non-Secure
         <1=>Secure, Non-Secure Callable
*/
#define SAU_INIT_NSC4 0
/*
   </e>
*/

/*
   <e>SAU Region 5
   <i> Setup SAU Region 5
*/
#define SAU_INIT_REGION5 1

/*
     <o>Start Address <0-0xFFFFFFE0>
*/
#define SAU_INIT_START5 0x00807E00

/*
     <o>End Address <0x1F-0xFFFFFFFF>
*/
#define SAU_INIT_END5 0x00807FFF

/*
     <o>Region is
         <0=>Non-Secure
         <1=>Secure, Non-Secure Callable
*/
#define SAU_INIT_NSC5 1
/*
   </e>
*/

/*
   <e>SAU Region 6
   <i> Setup SAU Region 6
*/
#define SAU_INIT_REGION6 1

/*
     <o>Start Address <0-0xFFFFFFE0>
*/
#define SAU_INIT_START6 NON_SECURE_SRAM_BASE

/*
     <o>End Address <0x1F-0xFFFFFFFF>
*/
#define SAU_INIT_END6 0x30017FFF

/*
     <o>Region is
         <0=>Non-Secure
         <1=>Secure, Non-Secure Callable
*/
#define SAU_INIT_NSC6 0
/*
   </e>
*/

/*
   <e>SAU Region 7
   <i> Setup SAU Region 7
*/
#define SAU_INIT_REGION7 1

/*
     <o>Start Address <0-0xFFFFFFE0>
*/
#define SAU_INIT_START7 0x50000000

/*
     <o>End Address <0x1F-0xFFFFFFFF>
*/
#define SAU_INIT_END7 0x5FFFFFFF

/*
     <o>Region is
         <0=>Non-Secure
         <1=>Secure, Non-Secure Callable
*/
#define SAU_INIT_NSC7 0
/*
   </e>
*/

/*
// </h>
*/

/*
// <e>Setup behavior of Sleep and Exception Handling
*/
#define SCB_CSR_AIRCR_INIT 1

/*
//   <o> Deep Sleep can be enabled by
//     <0=>Secure and Non-Secure state
//     <1=>Secure state only
//   <i> Value for SCB->CSR register bit DEEPSLEEPS
*/
#define SCB_CSR_DEEPSLEEPS_VAL 0

/*
//   <o>System reset request accessible from
//     <0=> Secure and Non-Secure state
//     <1=> Secure state only
//   <i> Value for SCB->AIRCR register bit SYSRESETREQS
*/
#define SCB_AIRCR_SYSRESETREQS_VAL 0

/*
//   <o>Priority of Non-Secure exceptions is
//     <0=> Not altered
//     <1=> Lowered to 0x80-0xFF
//   <i> Value for SCB->AIRCR register bit PRIS
*/
#define SCB_AIRCR_PRIS_VAL 0

/* Assign HardFault to be always secure for safe */
#define SCB_AIRCR_BFHFNMINS_VAL 0

/*
// </e>
*/

/*
// <h>Assign Interrupt to Secure or Non-secure Vector
*/

/*
    Initialize ITNS 0 (Interrupts 0..31)
*/
#define NVIC_INIT_ITNS0 1
/*
// BODOUT       Always secure
// IRC          Always secure
// PWRWU_       Always secure
// SRAM_PERR    Always secure
// CLKFAIL      Always secure

//   <o.6>  RTC                   <0=> Secure <1=> Non-Secure
//   <o.7>  TAMPER                <0=> Secure <1=> Non-Secure
// WDT  Always secure
// WWDT Always secure
//   <h> EINT
//   <o.10> EINT0                 <0=> Secure <1=> Non-Secure
//   <o.11> EINT1                 <0=> Secure <1=> Non-Secure
//   <o.12> EINT2                 <0=> Secure <1=> Non-Secure
//   <o.13> EINT3                 <0=> Secure <1=> Non-Secure
//   <o.14> EINT4                 <0=> Secure <1=> Non-Secure
//   <o.15> EINT5                 <0=> Secure <1=> Non-Secure
//   </h>
//   <h> GPIO
//   <o.16> GPA                   <0=> Secure <1=> Non-Secure
//   <o.17> GPB                   <0=> Secure <1=> Non-Secure
//   <o.18> GPC                   <0=> Secure <1=> Non-Secure
//   <o.19> GPD                   <0=> Secure <1=> Non-Secure
//   <o.20> GPE                   <0=> Secure <1=> Non-Secure
//   <o.21> GPF               <0=> Secure <1=> Non-Secure
//   </h>
//   <o.22> SPI0              <0=> Secure <1=> Non-Secure
//   <o.23> SPI1              <0=> Secure <1=> Non-Secure
//   <h> EPWM
//   <o.24> BRAKE0            <0=> Secure <1=> Non-Secure
//   <o.25> EPWM0_P0          <0=> Secure <1=> Non-Secure
//   <o.26> EPWM0_P1          <0=> Secure <1=> Non-Secure
//   <o.27> EPWM0_P2          <0=> Secure <1=> Non-Secure
//   <o.28> BRAKE1            <0=> Secure <1=> Non-Secure
//   <o.29> EPWM1_P0          <0=> Secure <1=> Non-Secure
//   <o.30> EPWM1_P1          <0=> Secure <1=> Non-Secure
//   <o.31> EPWM1_P2          <0=> Secure <1=> Non-Secure
//   </h>
//
*/
#define NVIC_INIT_ITNS0_VAL 0x11ff0000

/*
    Initialize ITNS 1 (Interrupts 0..31)
*/
#define NVIC_INIT_ITNS1 1
/*
//   <h> TIMER
// TMR0 Always secure
// TMR1 Always secure
//   <o.2>  TMR2              <0=> Secure <1=> Non-Secure
//   <o.3>  TMR3              <0=> Secure <1=> Non-Secure
//   </h>
//   <o.4>  UART0             <0=> Secure <1=> Non-Secure
//   <o.5>  UART1             <0=> Secure <1=> Non-Secure
//   <o.6>  I2C0              <0=> Secure <1=> Non-Secure
//   <o.7>  I2C1              <0=> Secure <1=> Non-Secure
// PDMA0 is secure only
//   <o.9>  DAC               <0=> Secure <1=> Non-Secure
//   <o.10> EADC0             <0=> Secure <1=> Non-Secure
//   <o.11> EADC1             <0=> Secure <1=> Non-Secure
//   <o.12> ACMP01            <0=> Secure <1=> Non-Secure

//   <o.14> EADC2             <0=> Secure <1=> Non-Secure
//   <o.15> EADC3             <0=> Secure <1=> Non-Secure
//   <o.16> UART2             <0=> Secure <1=> Non-Secure
//   <o.17> UART3             <0=> Secure <1=> Non-Secure

//   <o.19> SPI2              <0=> Secure <1=> Non-Secure
//   <o.20> SPI3              <0=> Secure <1=> Non-Secure
//   <o.21> USBD              <0=> Secure <1=> Non-Secure
//   <o.22> USBH              <0=> Secure <1=> Non-Secure
//   <o.23> USBOTG            <0=> Secure <1=> Non-Secure
//   <o.24> CAN0              <0=> Secure <1=> Non-Secure

//   <h> Smart Card
//   <o.26> SC0               <0=> Secure <1=> Non-Secure
//   <o.27> SC1               <0=> Secure <1=> Non-Secure
//   <o.28> SC2               <0=> Secure <1=> Non-Secure
//   </h>

//   <o.30> SPI4              <0=> Secure <1=> Non-Secure

//
*/
#define NVIC_INIT_ITNS1_VAL 0x5dfb16f4

/*
    Initialize ITNS 2 (Interrupts 0..31)
*/
#define NVIC_INIT_ITNS2 1
/*
//   <o.0>  SDH0              <0=> Secure <1=> Non-Secure



//   <o.4>  I2S0              <0=> Secure <1=> Non-Secure

//
//   <o.7>  CRYPTO                <0=> Secure <1=> Non-Secure
//   <o.8>  GPG               <0=> Secure <1=> Non-Secure

//   <o.10> UART4             <0=> Secure <1=> Non-Secure
//   <o.11> UART5             <0=> Secure <1=> Non-Secure
//   <o.12> USCI0             <0=> Secure <1=> Non-Secure
//   <o.13> USCI1             <0=> Secure <1=> Non-Secure
//   <o.14> BPWM0             <0=> Secure <1=> Non-Secure
//   <o.15> BPWM1             <0=> Secure <1=> Non-Secure


//   <o.18> I2C2              <0=> Secure <1=> Non-Secure

//   <o.20> QEI0              <0=> Secure <1=> Non-Secure
//   <o.21> QEI1              <0=> Secure <1=> Non-Secure
//   <o.22> ECAP0             <0=> Secure <1=> Non-Secure
//   <o.23> ECAP1             <0=> Secure <1=> Non-Secure
//   <o.24> GPH               <0=> Secure <1=> Non-Secure
//   <o.25> EINT7             <0=> Secure <1=> Non-Secure


//   <o.28> USBH              <0=> Secure <1=> Non-Secure



//
*/
#define NVIC_INIT_ITNS2_VAL 0x1f0fd99

/*
    Initialize ITNS 3 (Interrupts 0..31)
*/
#define NVIC_INIT_ITNS3 1
/*
//   <o.0>  SPI5              <0=> Secure <1=> Non-Secure
//   <o.1>  DSRC              <0=> Secure <1=> Non-Secure
//   <o.2>  PDMA1             <0=> Secure <1=> Non-Secure
// SCU  Always secure
//
//   <o.5>  TRNG              <0=> Secure <1=> Non-Secure
*/
#define NVIC_INIT_ITNS3_VAL 0x24

/*
// </h>
*/

/*
    max 128 SAU regions.
    SAU regions are defined in partition.h
 */

#define SAU_INIT_REGION(n)                                \
	SAU->RNR  = (n & SAU_RNR_REGION_Msk);                 \
	SAU->RBAR = (SAU_INIT_START##n & SAU_RBAR_BADDR_Msk); \
	SAU->RLAR = (SAU_INIT_END##n & SAU_RLAR_LADDR_Msk) | ((SAU_INIT_NSC##n << SAU_RLAR_NSC_Pos) & SAU_RLAR_NSC_Msk) | 1U
#endif /* PARTITION_M2351 */
