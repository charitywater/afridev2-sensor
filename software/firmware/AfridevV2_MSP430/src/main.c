/** 
 * @file main.c
 * \n Source File
 * \n AfridevV2 MSP430 Bootloader Firmware
 * 
 * \brief Main entry and init routines
 */

// Includes
#include "outpour.h"

/***************************
 * Module Data Definitions
 **************************/

/****************************
 * Module Data Declarations
 ***************************/

static uint8_t rebootReason;                               /* Read from the MSP430 to identify boot reason */

/**************************
 * Module Prototypes
 **************************/

/***************************
 * Module Public Functions 
 **************************/

/**
* \brief "C" entry point
* 
* @return int Ignored
*/
//#define LPM3_EXAMPLE 1

int main(void)
{
#ifdef LPM3_EXAMPLE
    // the following code is used to verify the lowest possible sleep current level with the hardware design
    // no other peripherals are in use to increase power use
    WDTCTL = WDT_ADLY_1000;                   // WDT 1s/4 interval timer
    IE1 |= WDTIE;                             // Enable WDT interrupt
    P1DIR = 0xFF;                             // All P1.x outputs
    P1OUT = 0;                                // All P1.x reset
    P2DIR = 0xFF;                             // All P2.x outputs
    P2OUT = 0;                                // All P2.x reset
    P2OUT |= I2C_DRV;
    P3DIR = 0xFF;                             // All P2.x outputs
    P3OUT = 0;                                // All P2.x reset
    P4DIR = 0xFF;                             // All P4.x outputs
    P4OUT = 0;                                // All P4.x reset

    LED_GREEN_DISABLE();
    while(1)
    {
      __bis_SR_register(LPM3_bits | GIE);     // Enter LPM3, enable interrupts
      LED_RED_ENABLE();
      __delay_cycles(7000);                   // Delay
      LED_RED_DISABLE();
    }
#else
    // (Re)start and tickle the watchdog.  The watchdog is initialized for a
    // one second timeout.
    WATCHDOG_TICKLE();

    // Store the status of the reboot reason (POR, Watchdog, Reset, etc).
    rebootReason = IFG1;

    // Perform Hardware Initialization
    hal_sysClockInit();
    hal_pinInit();
    hal_uartInit();

    // Call sys exec and never return.
    sysExec_exec();

    // System should never reach this point in execution.
    // But just in case.....
    disableGlobalInterrupt();
    // Force watchdog reset
    WDTCTL = 0xDEAD;
    while (1);
#endif
}

#ifdef LPM3_EXAMPLE
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = WDT_VECTOR
__interrupt void watchdog_timer(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(WDT_VECTOR))) watchdog_timer (void)
#else
#error Compiler not supported!
#endif
{
  __bic_SR_register_on_exit(LPM3_bits);     // Clear LPM3 bits from 0(SR)
}
#endif
/**
* \brief Utility function to return the value of the IFG1 
*        register read at app startup.
* 
* @return uint8_t Contents of IFG1 register read at start of 
*         program.
*/
uint8_t getLastRebootReason(void)
{
    return (rebootReason);
}

/**********************
 * Private Functions 
 **********************/

/**
* \brief sys_error
* \brief A catastrophic situation has occurred.  Restart system.
*/
void sysError(void)
{
#if 1
    // For development, loop infinitely.
    while (1);
#else
    // Reset system
    // Force watchdog reset
    WDTCTL = 0xDEAD;
    while (1);
#endif
}

#ifdef FOR_USE_WITH_BOOTLOADER

//
//  External ISR prototypes used by ProxyVectorTable
//
extern __interrupt void TIMERB0_ISR(void);
extern __interrupt void ISR_Timer0_A0(void);
extern __interrupt void USCI0TX_ISR(void);
extern __interrupt void USCI0RX_ISR(void);

/******************************************************************************
 *
 * @brief   Dummy Interrupt service routine
 *  This ISR will be executed if an undeclared interrupt is called
 *
 * @return  none
 *****************************************************************************/
__interrupt void Dummy_Isr(void)
{
    // Something catastrophic has happened.
    // Erase the application record so that the bootloader will go into SOS mode.
    appRecord_erase();

    // Force watchdog reset
    WDTCTL = 0xDEAD;
    while (1);
}

/**
 * \var ProxyVectorTable
 *
 * \brief This is a "proxy" interrupt table which is used to
 *         "redirect" MSP430 interrupt vectors to the
 *         appropriate interrupt routines within the Application
 *         code. This technique is based on a TI bootloader
 *         design. Please see document SLAA600A for a
 *         description of this scheme. Note that if an interrupt
 *         vector is not used by the application, the Dummy_Isr
 *         function pointer is used. Currently the only
 *         interrupts used by the application are the A0 timer,
 *         B0 timer and UART rx/tx.
 *
 *  \brief The MSP430 uses dedicated hardware interrupt vectors.
 *         The CPU grabs the address at the vector location and
 *         jumps to it. Because these locations are fixed
 *         according to the CPU design, a proxy method must be
 *         used so that the Application can use interrupts based
 *         on its own needs. Each vector in the standard MSP430
 *         interrupt vector table is setup so that it points to
 *         the locations specified in this "proxy" table. The
 *         entries in this table contains a branch instruction
 *         to the specific application interrupt handler.
 *
 * \brief Some details about the proxy table;
 * \li It always resides in the same location as specified in
 *     the linker command file.  The location must be known by
 *     the bootloader.
 * \li It contains a BRA instruction (0x4030) followed by the
 *     address of each ISR processing routine.  The bootloader
 *     jumps to the application ISR routine by jumping to the
 *     location associated with the hardware interrupt in the
 *     table.
 * \li Note that the number and location of each vector should
 *     match the declaration in Boot's Vector_Table
 */

#pragma DATA_SECTION(ProxyVectorTable, ".APP_PROXY_VECTORS")
#pragma RETAIN(ProxyVectorTable)
const uint16_t ProxyVectorTable[] =
{
    0x4030, (uint16_t)Dummy_Isr,                           // 0xFFE0 APP_PROXY_VECTOR(0)  TA1_1
    0x4030, (uint16_t)Dummy_Isr,                           // 0xFFE2 APP_PROXY_VECTOR(1)  TA1_0
    0x4030, (uint16_t)Dummy_Isr,                           // 0xFFE4 APP_PROXY_VECTOR(2)  P1
    0x4030, (uint16_t)Dummy_Isr,                           // 0xFFE6 APP_PROXY_VECTOR(3)  P2
    0x4030, (uint16_t)Dummy_Isr,                           // 0xFFEA APP_PROXY_VECTOR(4)  ADC10
    0x4030, (uint16_t)USCI0TX_ISR,                         // 0xFFEC APP_PROXY_VECTOR(5)  USCI I2C TX/RX
    0x4030, (uint16_t)USCI0RX_ISR,                         // 0xFFEE APP_PROXY_VECTOR(6)  USCI I2C STAT
    0x4030, (uint16_t)Dummy_Isr,                           // 0xFFF0 APP_PROXY_VECTOR(7)  TA0_1
    0x4030, (uint16_t)ISR_Timer0_A0,                       // 0xFFF2 APP_PROXY_VECTOR(8)  TA0_0
#ifdef LPM3_EXAMPLE
    0x4030, (uint16_t)watchdog_timer,                      // 0xFFF4 APP_PROXY_VECTOR(9)  WDT
#else	
    0x4030, (uint16_t)Dummy_Isr,                           // 0xFFF4 APP_PROXY_VECTOR(9)  WDT
#endif
    0x4030, (uint16_t)Dummy_Isr,                           // 0xFFF6 APP_PROXY_VECTOR(10) COMP_A
    0x4030, (uint16_t)Dummy_Isr,                           // 0xFFF8 APP_PROXY_VECTOR(11) TB0_1
    0x4030, (uint16_t)TIMERB0_ISR,                         // 0xFFFA APP_PROXY_VECTOR(12) TB0_0
    0x4030, (uint16_t)Dummy_Isr,                           // 0xFFFC APP_PROXY_VECTOR(13) NMI
};
#endif


/******************************
 * Doxygen Support Defines 
 ******************************/

/**
* \defgroup PUBLIC_API Public API
* \brief Public API's are functions called by other C modules to
*        initiate some type of work or set/get data in the
*        module.
*/

/**
* \defgroup SYSTEM_INIT_API System and Module Initialization API
* \brief These routines are called once at system start to 
*        initialize data and/or hardware.  Each main processing
*        module in the system has its own initialization
*        routine.
*/

/**
* \defgroup EXEC_ROUTINE Top Level Processing Routines.
* \brief Exec routines are called on each iteration of the main
*        processing loop for the software.  They are responsible
*        for "orchestrating" the specific functionality of a
*        specific module in the system.  Because the system does
*        not have multiple threads, all work must be done in a
*        piece meal fashion using state machine processing by
*        each Exec as needed.
*/

/**
* \defgroup ISR Interrupt Handler API
* \brief Interrupt handler for the system.  Currently there are 
*        three types used:  UART, A0 Timer and B0 Timer.
*        \li The Uart has an individual ISR for transmit and
*        receive.
*        \li The A0 Timer interrupt is a one second tick timer
*        used to bring the system out of low power mode and
*        initiate the main loop processing.
*        \li The B0 Timer interrupt is used as part of the
*        Capacitive touch processing.
*/

