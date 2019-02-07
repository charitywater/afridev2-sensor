/** 
 * @file main.c
 * \n Source File
 * \n AfridevV2 MSP430 Bootloader Firmware
 * 
 * \brief Main entry and init routines
 */

// Includes
#include "outpour.h"
#include "linkAddr.h"

/***************************
 * Module Data Definitions
 **************************/

/*!
 *  Jumps to application using its reset vector address
 */
#define TI_MSPBoot_APPMGR_JUMPTOAPP()     {((void (*)()) __App_Reset_Vector) ();}

/**
 * \def SOS_DELAY_TICKS
 * \brief Specify how long to wait until trying the next 
 *        firmware upgrade attempt.  This delay is used if the
 *        current upgrade fails, and no good code is detected in
 *        the application area.
 */
#define SOS_DELAY_TICKS ((sys_tick_t)2*60*60*12)

/**
 * \def MAX_ALLOWED_REBOOTS_WITH_NO_APPRECORD
 * \brief If the software app runs and reboots before writing an 
 *        appRecord, then the bootloader assumes there is a
 *        problem with the app firmware.  It will only allow a
 *        reboot to occur MAX_ALLOWED_REBOOTS times before going
 *        into SOS mode.
 */
#define MAX_ALLOWED_REBOOTS_WITH_NO_APPRECORD 64

/**
 * \typedef bootData_t 
 * \brief Contains module data.
 */
typedef struct __attribute__((__packed__))mainData_s {
    uint8_t rebootReason;                                  /**< Read from the MSP430 to identify boot reason */
    bool appRecordIsGood;                                  /**< Flag to indicate appRecord is valid */
    int16_t blrRecordCount;                                /**< Contains the bootRecord count field */
    int8_t newFwCopyResult;                                /**< Result of the FW copy attempt from backup image to main image*/
    bool newFwIsReady;                                     /**< Value of fwIsReady flag in the appRecord */
    uint16_t newFwCrc;                                     /**< Value of newFwCrc in the appRecord */
} bootData_t;

/**
 * \def DEBUG_LED_USE
 * \brief Defining this constant enables the use of the LEDs to signal
 *        various failures detected in the boot loader.   To disable this
 *        the following constant should be commented out
 */
#define DEBUG_LED_USE 1

/***************************
 * Module Data Declarations
 **************************/

/**
* \var sosDelayTicks
* \brief Counter for the 12 hour delay used to wait until trying
*        the next firmware upgrade attempt.
*/
volatile int32_t sosDelayTicks;

/**
 * \var bootData
 * \brief Declare the object that contains the module data.
 */
bootData_t bootData;

/**************************
 * Module Prototypes
 **************************/
static void sosMode(void);
static void low_power_12_hour_delay(void);
static void disableIndividualInterrupts(void);

static void signalPassLED(uint8_t blinkCount);
#ifdef DEBUG_LED_USE
static void signalErrorLED(uint8_t blinkCount);
#endif

/***************************
 * Module Public Functions 
 **************************/

/**
* \brief Copy the boot data structure.  Used to return info in 
*        case of a firmware update failure.  This data will be
*        included in the SOS packet.
* 
* @param bufP  Pointer to buffer to copy data to
* 
* @return uint8_t  Returns the number of bytes copied.
*/
uint8_t main_copyBootInfo(uint8_t *bufP)
{
    int length = 0;

    // Copy boot data structure to buffer
    memcpy(bufP, &bootData, sizeof(bootData_t));

    // increment buffer pointer
    bufP += sizeof(bootData_t);
    length += sizeof(bootData_t);

    // Copy bootRecord structure to buffer
    length += bootRecord_copy(bufP);

    // Return size of boot data structure
    return (length);
}

/**
* \brief Returns a pointer to the boot info structure and the 
*        corresponding length of the structure.  This data will
*        be included in the SOS packet.
* 
* @param bufPP Filled-in with the address of the bootData 
*              structure.
* @param lengthP Filled-in with the length of the bootData 
*                structure.
*/
void main_accessBootInfo(const uint8_t **bufPP, uint8_t *lengthP)
{
    *bufPP = (uint8_t *)&bootData;
    *lengthP = sizeof(bootData_t);
}

/**
* \brief "C" entry point
* 
* @return int Ignored
*/
int main(void)
{
    bool jumpToApplication = false;

    // Zero the module data structure
    memset(&bootData, 0, sizeof(bootData_t));
    bootData.newFwCopyResult = FW_COPY_ERR_NO_BACKUP_IMAGE;

    // Store the status of the reboot reason (POR, Watchdog, Reset, etc).
    bootData.rebootReason = IFG1;
    // Must clear the IFG1 register, as it does not clear on its own.
    // We don't want previous reboot reasons clouding our decisions.
    IFG1 = 0;

    // (Re)start and tickle the watchdog.  The watchdog is initialized for a
    // one second timeout.
    WATCHDOG_TICKLE();

    // make a 25 msec delay (@16 MHz) before changing clock.  This is to be sure that the
	// DCO settings are not made before the 2 mSec window of Td(BOR) identified in Figure 12 of
	// TI's MIXED SIGNAL MICROCONTROLLER specification SLAS800 March 2013 p. 25
	// the port pins are not yet initialized, so the Green LED is not lit for this delay
    signalPassLED(1);

    // Perform Hardware initialization
    hal_sysClockInit();
    hal_pinInit();

    // Increment the bootloaderRecord Count.  The bootloader record
    // is located in the flash INFO section.  Inside the structure is a
    // counter tracking how many times we have booted since identifying
    // an Application Record.  The bootloader uses the existence of a
    // valid Application Record to verify that the application ran
    // successfully on the previous boot.
    //
    // The count is zero'd below if a valid Application Record is found
    // (the Application Record is located in one of the INFO sections).
    //
    // If no valid bootloader record is found, a fresh bootloader
    // structure is written.
    bootData.blrRecordCount = bootRecord_getBootloaderRecordCount();
    if (bootData.blrRecordCount < 0)
    {
        // No bootloader record was found.  This is probably because we are
        // booting for the first time after the flash was programmed and the
        // bootrecord is all FF's.
        bootRecord_initBootloaderRecord();
    }
    else
    {
        // Only increment the bootloader record if this is not a
        // power-on-reset (POR) or hard reset via the reset pin.

        if (!(bootData.rebootReason & (PORIFG | RSTIFG)))
        {
            bootRecord_incrementBootloaderRecordCount();
#ifdef DEBUG_LED_USE
            if (bootData.rebootReason & NMIIFG)
        	   signalErrorLED(2);
            else if (bootData.rebootReason & OFIFG)
               signalErrorLED(3);
            else if (bootData.rebootReason & WDTIFG)
               signalErrorLED(4);
#endif
        }
    }

    // Check if there is a new firmware image that should be used.
    // This is specified in the appRecord.
    bootData.appRecordIsGood = appRecord_checkForValidAppRecord();
    appRecord_getNewFirmwareInfo(&bootData.newFwIsReady, &bootData.newFwCrc);
    if (bootData.newFwIsReady)
    {
        // appRecord specifies new firmware is ready in the backup image
        // location.  Attempt to copy it to the main image location.
        bootData.newFwCopyResult = msp430Flash_moveAndVerifyBackupToApp();
        if (bootData.newFwCopyResult == FW_COPY_SUCCESS)
        {
            // The application firmware was updated successfully.
            // Refresh the bootloader record and set to the flash to jump
            // to the application.
            bootRecord_initBootloaderRecord();
            // Set flag to mark it's OK to jump to application
            jumpToApplication = true;
        }
#ifdef DEBUG_LED_USE
        else
        {
        	// signal FIRMWARE CRC failure, ignore the new firmware image
        	signalErrorLED(5);
        }
#endif
    }

    if (!jumpToApplication)
    {
        // No upgrade firmware was found or the upgrade failed.
        // We check if there is a valid application record in flash.  This
        // signals that the application code ran successfully on the previous
        // session.
        if (bootData.appRecordIsGood)
        {
            // If an appRecord was found, we assume the app ran OK previously,
            // and we are rebooting for valid reasons.  Refresh the bootloader
            // record so that the count will be zero.
            bootRecord_initBootloaderRecord();
            // Set flag to mark it's OK to jump to application
            jumpToApplication = true;
        }
        else
        {
            // There is no appRecord!
            // This could be the first time booting after the flash was
            // programmed externally.  If this is the case, then the
            // blrRecordCount will be zero.  But if the bootloader count
            // is greater than zero, then we assume something is wrong.
            // If the bootloader is greater than zero, than we assume that
            // we attempted to start the app but for some reason it did not
            // correctly write an application record.  Allow for up to four
            // reboots to see if the problem will fix itself. If greater than
            // four, go into SOS mode.
            bootData.blrRecordCount = bootRecord_getBootloaderRecordCount();
            if (bootData.blrRecordCount < MAX_ALLOWED_REBOOTS_WITH_NO_APPRECORD)
            {
                // Set flag to mark it's OK to jump to application
                jumpToApplication = true;
            }
#ifdef DEBUG_LED_USE
            else
            {
            	// signalAPP RECORD failure, go to SOS (reluctantly, but if it fails 64 times something is wrong)
            	signalErrorLED(6);
            }
#endif
        }
    }

    // If the jump to application flag is set, then make
    // final preparations and jump to app.
    if (jumpToApplication)
    {
        // Get the flash parameters from the linker file.
        uint16_t *app_reset_vector = (uint16_t *)getAppResetVector();
        uint16_t app_flash_start = getAppImageStartAddr();
        uint16_t app_vector_table = getAppVectorTableAddr();

        // Verify that the location stored at the application reset vector falls into a legal range
        if ((*app_reset_vector >= app_flash_start) && (*app_reset_vector < app_vector_table))
        {
        	// signal FIRMWARE CRC failure
        	signalPassLED(3);

            // Erase the application record.
            // The app will re-write it once it starts.  This is how we know the
            // app started correctly the next time we boot.
            appRecord_erase();
            // Start the watchdog
            WATCHDOG_TICKLE();
            // Disable Interrupts
            disableGlobalInterrupt();
            // Stop timer
            timerA1_0_halt();
            // Jump to app
            TI_MSPBoot_APPMGR_JUMPTOAPP();
        }
#ifdef DEBUG_LED_USE
        else
        {
        	// this is a BUILD defect, not an operational one, this should not happen at all in the field
        	signalErrorLED(7);
        }
#endif
    }

    // signal SOS (both lit)
    LED_GREEN_ENABLE();
    LED_RED_ENABLE();
    // If we made it here, then something is wrong.  Go into SOS mode.
    sosMode();

    return (0);
}

#ifdef DEBUG_LED_USE
/**
* \brief This is used to signal specific error conditions in the Boot Loader.
*
*/
static void signalErrorLED(uint8_t blinkCount)
{
	uint8_t i,j;

    WATCHDOG_TICKLE();
	for(j=0; j<blinkCount; j++)
	{
		LED_GREEN_DISABLE();
		LED_RED_ENABLE();

		for (i=0; i< 200; i++)
		   __delay_cycles(1000);

		LED_RED_DISABLE();
	    WATCHDOG_TICKLE();

		for (i=0; i< 200; i++)
		   __delay_cycles(1000);
	}
	for (i=0; i< 200; i++)
	   __delay_cycles(1000);
    WATCHDOG_TICKLE();
}
#endif
/**
* \brief This is used to signal specific good conditions in the Boot Loader. Such as successful Boot completion
*
*/
static void signalPassLED(uint8_t blinkCount)
{
	uint8_t i,j;

    WATCHDOG_TICKLE();
	for(j=0; j<blinkCount; j++)
	{
		LED_GREEN_ENABLE();
		LED_RED_DISABLE();

		for (i=0; i< 200; i++)
		   __delay_cycles(1000);

		LED_GREEN_DISABLE();
	    WATCHDOG_TICKLE();

		for (i=0; i< 200; i++)
		   __delay_cycles(1000);
	}
	for (i=0; i< 200; i++)
	   __delay_cycles(1000);
    WATCHDOG_TICKLE();
}

/**
* \brief SOS mode processing.  No valid application firmware is 
*        detected.  Send an SOS message, check for a firmware
*        update message from the modem, and then go into low
*        power mode for 12 hours.  Repeat the sequence forever
*        until a new firmware image is received.
*/
static void sosMode(void)
{

    // The fwUpdateResult is the status set by the firmware upgrade state machine
    // which is located in the msgOta.c module.  The status is used
    // to make decisions on how to proceed.
    fwUpdateResult_t fwUpdateResult = RESULT_NO_FWUPGRADE_PERFORMED;
    bool sosFlag = false;
    bool jumpToApplication = false;

    WATCHDOG_TICKLE();

    // Perform Hardware initialization
    hal_uartInit();

    // Perform module initialization
    timerA1_0_init_for_sys_tick();
    modemCmd_init();
    modemPower_init();
    modemMgr_init();
    otaMsgMgr_init();

    WATCHDOG_TICKLE();

    // Enter the boootloader infinite loop.
    while (1)
    {

        // Check if a firmware upgrade message is available from the modem.
        // If the sosFlag is set to true,  a special SOS message is transmitted
        // from the modem to the cloud in addition to checking for a firmware
        // upgrade message.
        otaMsgMgr_getAndProcessOtaMsgs(sosFlag);

        // Run in a loop until the modem check is complete.
        while (!otaMsgMgr_isOtaProcessingDone())
        {
            // A "spinning" delay is used on each iteration.  The delay uses
            // the hardware timer to determine when the delay interval is complete.
            while (timerA1_0_check_for_sys_tick() == false)
            {
                // Polling is used for UART communication instead of interrupts.
                // So we need to run the polling function during the delay.
                modemCmd_pollUart();
                WATCHDOG_TICKLE();
            }
            // Run each exec routine that is used as part of the modem communication
            // and OTA processing.
            modemCmd_exec();
            modemPower_exec();
            modemMgr_exec();
            otaMsgMgr_exec();
        }

        // For debug
        // SendReasonAndCountToUart();

        // Get the result of the firmware upgrade check from the modem,
        // and act on it accordingly.
        fwUpdateResult = otaMsgMgr_getFwUpdateResult();
        if (fwUpdateResult == RESULT_DONE_SUCCESS)
        {
            // The application firmware was updated successfully.
            // Refresh the bootloader record and set to the flash to jump
            // to the application.
            bootRecord_initBootloaderRecord();
            // Set flag to mark it's OK to jump to application
            jumpToApplication = true;
        }

        if (jumpToApplication)
        {
            // Get the flash parameters from the linker file.
            uint16_t app_reset_vector = getAppResetVector();
            uint16_t app_flast_start = getAppImageStartAddr();
            uint16_t app_vector_table = getAppVectorTableAddr();

            // Verify that the application reset vector falls into a legal range
            if ((app_reset_vector >= app_flast_start) && (app_reset_vector < app_vector_table))
            {
                // Erase the application record.
                // The app will re-write it once it starts.  This is how we know the
                // app started correctly the next time we boot.
                appRecord_erase();
                // Start the watchdog
                WATCHDOG_TICKLE();
                // Disable Interrupts
                disableGlobalInterrupt();
                // Stop timer
                timerA1_0_halt();
                // Jump to app
                TI_MSPBoot_APPMGR_JUMPTOAPP();
            }
        }

        // If we just completed the loop sequence with the SOS flag set, then
        // fall into a 12 hour delay.
        if (sosFlag)
        {
            // don't kill the battery waiting to resolve the SOS, it's more than likely sealed up in the housing anyways
            // turn off the LEDs
    		LED_GREEN_DISABLE();
    		LED_RED_DISABLE();

            low_power_12_hour_delay();

            // turn on the SOS LED indication when trying to send SOS message over modem, in case the Modem is locked up
    		LED_GREEN_ENABLE();
    		LED_RED_ENABLE();
        }
        else
        {
            // We made it here without jumping to the application.  So something
            // is wrong. For various potential reasons, the bootloader failed to
            // upgrade successfully or identify a valid application program in flash.
            // Set the SOS flag which will result in a SOS message to be transmitted
            // to the cloud on the next loop iteration when checking for an upgrade
            // message from the modem.
            sosFlag = true;
        }
    }
}

/**********************
 * Private Functions 
 **********************/

/**
* \brief Disable unused interrupts.
*/
static void disableIndividualInterrupts(void)
{
    TA0CTL &= ~TAIE;
    TA1CTL &= ~TAIE;
    TB0CTL &= ~TBIE;
    P1IE = 0;
    P2IE = 0;
    UC0IE = 0;
    UCA0CTL1 = 0;
    UCB0CTL1 = 0;
}

/**
* \brief Wait for 12 hours.  The function loops for 12 hours, 
*        waking from the timer every .5 seconds to tickle the
*        watchdog.  Once the 12 hours has completed, the
*        function reboots the MSP430.
*/
static void low_power_12_hour_delay(void)
{
    sosDelayTicks = SOS_DELAY_TICKS;
    // Modem should already be off, but just for safety, turn it off
    modemPower_powerDownModem();
    // This is not really needed, but because we had some issues in the past
    // with potential un-handled interrupts, make sure other interrupts are
    // disabled.
    disableIndividualInterrupts();
    // Enable TimerA1 to interrupt every .5 seconds
    timerA1_0_init_for_sleep_tick();
    // Enable the global interrupt
    enableGlobalInterrupt();
    while (sosDelayTicks > 0)
    {
        WATCHDOG_TICKLE();
        // sleep, wake on Timer1A interrupt
        __bis_SR_register(LPM3_bits);
        sosDelayTicks--;
    }
    bootRecord_addDebugInfo();
    disableGlobalInterrupt();
    // Force watchdog reset
    WDTCTL = 0xDEAD;
    while (1);
}

/** 
 * \var Vector_Table 
 * \brief The MSPBoot Vector Table is fixed and it can't be 
 *        erased or modified. Most vectors point to a proxy
 *        vector entry in Application area.  The bootloader only
 *        uses one interrupt - the timer interrupt.
 *  
 *  The MSP430 uses dedicated hardware interrupt vectors.  The
 *  CPU grabs the address at the vector location and jumps it.
 *  Because these locations are fixed according to the CPU
 *  design, a proxy method must be used so that the Application
 *  can use interrupts based on its own needs. Each vector in
 *  this table is setup so that it points to a special location
 *  in the Application code.  The special location in the
 *  application code contains a branch instruction to the
 *  specific application interrupt handler.
 */

//
//  Macros and definitions used in the interrupt vector table
//
/* Value written to unused vectors */
#define UNUSED                  (0x3FFF)
/*!
 *  Macro used to calculate address of vector in Application Proxy Table
 */
#define APP_PROXY_VECTOR(x)     ((uint16_t)&__App_Proxy_Vector_Start[x*2])

#ifdef __IAR_SYSTEMS_ICC__
#pragma location="BOOT_VECTOR_TABLE"
__root const uint16_t Vector_Table[] =
#elif defined (__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(Vector_Table, ".BOOT_VECTOR_TABLE")
#pragma RETAIN(Vector_Table)
const uint16_t Vector_Table[] =
#endif
{
    APP_PROXY_VECTOR(0),                                   // FFE0 = TA1_1
    (uint16_t)ISR_Timer1_A0,                               // FFE2 = TA1_0 (Used by bootloader)
    APP_PROXY_VECTOR(2),                                   // FFE4 = P1
    APP_PROXY_VECTOR(3),                                   // FFE6 = P2
    UNUSED,                                                // FFE8 = unused
    APP_PROXY_VECTOR(4),                                   // FFEA = ADC10
    APP_PROXY_VECTOR(5),                                   // FFEC = USCI I2C TX/RX
    APP_PROXY_VECTOR(6),                                   // FFEE = USCI I2C STAT
    APP_PROXY_VECTOR(7),                                   // FFF0 = TA0_1
    APP_PROXY_VECTOR(8),                                   // FFF2 = TA0_0
    APP_PROXY_VECTOR(9),                                   // FFF4 = WDT
    APP_PROXY_VECTOR(10),                                  // FFF6 = COMP_A
    APP_PROXY_VECTOR(11),                                  // FFF8 = TB0_1
    APP_PROXY_VECTOR(12),                                  // FFFA = TB0_0
    APP_PROXY_VECTOR(13),                                  // FFFC = NMI
};

#if 0
// DEBUG FUNCTION
volatile uint8_t rebootReason;
volatile int16_t blrRecordCount;
#pragma SET_CODE_SECTION(".infoD")
void SendReasonAndCountToUart(void)
{
    volatile int i;
    for (i = 0; i < 20; i++)
    {
        _delay_cycles(10000);
        UCA0TXBUF = rebootReason;
        _delay_cycles(10000);
        UCA0TXBUF = blrRecordCount;
    }
}
#pragma SET_CODE_SECTION()
#endif


