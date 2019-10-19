/** 
 * @file modemMsg.h
 * \n Source File
 * \n AfridevV2 MSP430 Firmware
 * 
 * \brief SIM900 modem data definitions.
 */
#ifndef MODEM_MSG_H_
#define MODEM_MSG_H_

/**
 * \typedef MessageType_t
 * \brief Identify the different AfridevV2 outgoing messages 
 *        identifiers.
 */
typedef enum messageType {
    MSG_TYPE_FINAL_ASSEMBLY = 0x00,
    MSG_TYPE_OTAREPLY = 0x03,
    MSG_TYPE_RETRYBYTE = 0x04,
    MSG_TYPE_CHECKIN = 0x05,
    MSG_TYPE_ACTIVATED = 0x07,
    MSG_TYPE_GPS_LOCATION = 0x08,
    MSG_TYPE_DAILY_LOG = 0x21,
    MSG_TYPE_SENSOR_DATA = 0x22,
    MSG_TYPE_SOS = 0x23,	
    MSG_TYPE_TIMESTAMP = 0x24,
    // this message type is an internal message (it will never be seen by the IOT server)
    MSG_TYPE_MODEM_SEND_TEST = 0x2F,

    // The following are debug messages for use with the PC
    // data logger.  These messages do not go to the Modem, and
    // are only sent when the Modem is powered off.
    MSG_TYPE_DEBUG_PAD_STATS = 0x10,
    MSG_TYPE_DEBUG_STORAGE_INFO = 0x11,
    MSG_TYPE_DEBUG_TIME_INFO = 0x12,

} MessageType_t;

/**
 * \typedef OtaOpcode_t 
 * \brief Identify each possible incoming AfridevV2 OTA opcode.
 */
typedef enum otaOpcodes_e {
    OTA_OPCODE_GMT_CLOCKSET = 0x01,
    OTA_OPCODE_LOCAL_OFFSET = 0x02,
    OTA_OPCODE_RESET_DATA = 0x03,
    OTA_OPCODE_RESET_RED_FLAG = 0x04,
    OTA_OPCODE_ACTIVATE_DEVICE = 0x05,
    OTA_OPCODE_SILENCE_DEVICE = 0x06,
    OTA_OPCODE_SET_TRANSMISSION_RATE = 0x07,
    OTA_OPCODE_RESET_DEVICE = 0x08,
    OTA_OPCODE_CLOCK_REQUEST = 0x0C,
    OTA_OPCODE_GPS_REQUEST = 0x0D,
    OTA_OPCODE_SET_GPS_MEAS_PARAMS = 0x0E,
    OTA_OPCODE_SENSOR_DATA = 0x0F,
    OTA_OPCODE_FIRMWARE_UPGRADE = 0x10,
    OTA_OPCODE_MEMORY_READ = 0x1F,
} OtaOpcode_t;

/**
 * \typedef outpour_modem_command_t
 * \brief These mirror the SIM900 BodyTrace Command Messsage 
 *        Type names, but are put in numerical sequential order
 *        to allow for table indexing of processing function
 *        based on command number (see modemCmd.c).
 */
typedef enum outpour_modem_command_e {
    OUTPOUR_M_COMMAND_PING = 0x00,                         /**< PING */
    OUTPOUR_M_COMMAND_MODEM_INFO = 0x1,                    /**< MODEM INFO */
    OUTPOUR_M_COMMAND_MODEM_STATUS = 0x2,                  /**< MODEM STATUS */
    OUTPOUR_M_COMMAND_MESSAGE_STATUS = 0x3,                /**< MESSAGE STATUS */
    OUTPOUR_M_COMMAND_SEND_TEST = 0x4,                     /**< SEND TEST */
    OUTPOUR_M_COMMAND_SEND_DATA = 0x5,                     /**< SEND DATA */
    OUTPOUR_M_COMMAND_GET_INCOMING_PARTIAL = 0x6,          /**< GET INCOMING PARTIAL */
    OUTPOUR_M_COMMAND_DELETE_INCOMING = 0x7,               /**< DELETE INCOMING */
    OUTPOUR_M_COMMAND_SEND_DEBUG_DATA = 0x8,               /**< SEND DEBUG DATA - FOR OUTPUT INTERNAL DEBUG ONLY */
    OUTPOUR_M_COMMAND_POWER_OFF = 0x9,                     /**< POWER OFF */
} outpour_modem_command_t;

/**
 * \typedef modem_command_t
 * \brief These are the SIM900 BodyTrace Command Messsage Types
 */
typedef enum modem_command_e {
    M_COMMAND_PING = 0x00,                                 /**< PING */
    M_COMMAND_MODEM_INFO = 0x1,                            /**< MODEM INFO */
    M_COMMAND_MODEM_STATUS = 0x2,                          /**< MODEM STATUS */
    M_COMMAND_MESSAGE_STATUS = 0x3,                        /**< MESSAGE STATUS */
    M_COMMAND_SEND_TEST = 0x20,                            /**< SEND TEST */
    M_COMMAND_SEND_DATA = 0x40,                            /**< SEND DATA */
    M_COMMAND_GET_INCOMING_PARTIAL = 0x42,                 /**< GET INCOMING PARTIAL */
    M_COMMAND_DELETE_INCOMING = 0x43,                      /**< DELETE INCOMING */
    M_COMMAND_SEND_DEBUG_DATA = 0x50,                      /**< SEND DEBUG DATA - FOR OUTPUT INTERNAL DEBUG ONLY */
    M_COMMAND_POWER_OFF = 0xe0,                            /**< POWER OFF */
} modem_command_t;

/**
 * \typedef modem_command_t
 * \brief These are the SIM900 BodyTrace Command error types
 */
typedef enum modem_error_e {
    MODEM_ERROR_SUCCESS = 0,                               /**< Success */
    MODEM_ERROR_WRITE,                                     /**< Write error */
    MODEM_ERROR_TIMEOUT,                                   /**< Operation timed out */
    MODEM_ERROR_INVALID,                                   /**< Invalid response (e.g. CRC mismatch) */
    MODEM_ERROR_SIZE                                       /**< Payload too big */
} modem_error_t;

/**
 * \typedef modem_state_t
 * \brief These are the SIM900 BodyTrace state definitions
 */
typedef enum modem_state_e {
    MODEM_STATE_INITIALIZING = 0x00,                       /**< System initializing */
    MODEM_STATE_IDLE,                                      /**< Idle, not connected */
    MODEM_STATE_REGISTERING,                               /**< Registering to the cellular network */
    MODEM_STATE_CONNECTING,                                /**< Connecting to data service */
    MODEM_STATE_CONNECTED,                                 /**< Connected to data service, no transmission in progress */
    MODEM_STATE_XFER,                                      /**< Transferring messages */
    MODEM_STATE_DISCONNECTING,                             /**< Disconnecting from data service */
    MODEM_STATE_DEREGISTERING,                             /**< Deregistering from network */

    MODEM_STATE_PROVISIONING = 0x20,                       /**< Modem is being provisioned */

    MODEM_STATE_ERROR_INTERNAL = 0x80,                     /**< An internal error occurred */
    MODEM_STATE_ERROR_BATTERY,                             /**< Supply voltage (battery charge) is critically low */
    MODEM_STATE_ERROR_SIM,                                 /**< Failed to initialize SIM */
    MODEM_STATE_ERROR_REGISTER,                            /**< Failed to register to network */
    MODEM_STATE_ERROR_CONNECT,                             /**< Failed to connect to data service */
    MODEM_STATE_ERROR_XFER,                                /**< Failed to transfer messages */


    MODEM_STATE_ERROR_PROV_KEY = 0xa0,                     /**< Failed to register device public key */
    MODEM_STATE_ERROR_PROV_XFER,                           /**< Failed to transfer data from provisioning service */
    MODEM_STATE_ERROR_PROV_INVALID,                        /**< Invalid provisioning response received */
    MODEM_STATE_ERROR_PROV_UNPROVISIONED,                  /**< Device not provisioned */

    MODEM_STATE_ERROR_TEST_VOLTAGE = 0xc0,                 /**< Supply voltage not accepted by remote service */
    MODEM_STATE_ERROR_TEST_ADC,                            /**< ADC value not accepted by remote service */
    MODEM_STATE_ERROR_TEST_RSSI,                           /**< Signal RSSI not accepted by remote service */
    MODEM_STATE_ERROR_TEST_DATA                            /**< Test data not accepted by remote service */
} modem_state_t;

/** Modem information */
typedef struct {
    uint8_t major;                                         /**< Software version major number */
    uint8_t minor;                                         /**< Software version minor number */
    uint64_t imei;                                         /**< IMEI (left pad with zeros for full IMEI) */
} modem_info_t;

/** Modem status */
typedef struct modem_status_s {
    modem_state_t state;                                   /**< Network state */
    uint16_t voltage;                                      /**< Power supply voltage (in mV) */
    uint16_t adc;                                          /**< ADC voltage (if connected) (in mV) */
    uint8_t rssi;                                          /**< RSSI (in -dBm) */
    uint8_t signalStrength;                                /**< Radio signal stength (in percents) */
    uint8_t provisioned;                                   /**< Provisioned flag (<code>0</code> if not provisioned; <code>1</code> if provisioned) */
    int8_t temperature;                                    /**< Temperature (in C) */
} modem_status_t;

/** Message status element */
typedef struct {
    uint16_t count;                                        /**< Number of messages pending */
    uint32_t size;                                         /**< Total length of message payloads pending (in bytes) */
} modem_message_status_el_t;

/** Message status element */
typedef struct {
    modem_message_status_el_t incoming;                    /**< Status of INCOMING messages */
    modem_message_status_el_t test;                        /**< Status of TEST messages */
    modem_message_status_el_t data;                        /**< Status of DATA messages */
} modem_message_status_t;

#endif
