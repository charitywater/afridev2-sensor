/** 
 * @file modemMsg.h
 * \n Source File
 * \n AfridevV2 MSP430 Bootloader Firmware
 * 
 * \brief SIM900 modem data definitions.
 *
 * \par   Copyright Notice
 *        Copyright 2021 charity: water
 *
 *        Licensed under the Apache License, Version 2.0 (the "License");
 *        you may not use this file except in compliance with the License.
 *        You may obtain a copy of the License at
 *
 *            http://www.apache.org/licenses/LICENSE-2.0
 *
 *        Unless required by applicable law or agreed to in writing, software
 *        distributed under the License is distributed on an "AS IS" BASIS,
 *        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *        See the License for the specific language governing permissions and
 *        limitations under the License.
 *
 */
#ifndef MODEM_MSG_H_
#define MODEM_MSG_H_

/**
 * \typedef MessageType_t
 * \brief Identify the different AfridevV2 outgoing messages 
 *        identifiers.
 */
typedef enum messageType {
    MSG_TYPE_OTAREPLY = 0x03,
    MSG_TYPE_SOS = 0x23,
} MessageType_t;

/**
 * \typedef OtaOpcode_t 
 * \brief Identify each possible incoming AfridevV2 OTA opcode.
 */
typedef enum otaOpcodes_e {
    OTA_OPCODE_FIRMWARE_UPGRADE = 0x10
} OtaOpcode_t;

/**
 * \typedef modem_command_t
 * \brief These are the SIM900 BodyTrace Command Message Types
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
