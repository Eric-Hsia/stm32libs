#include "modbus_rtu.h"
#include <string.h>



/*
 * Вычисление CRC взято из мануала http://www.modbus.org/docs/PI_MBUS_300.pdf
 */

/* Table of CRC values for high–order byte     */
static const uint8_t table_crc_hi[] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
    0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
    0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
    0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
    0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
    0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
    0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
    0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
    0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
    0x40
};

/* Table of CRC values for low–order byte      */
static const uint8_t table_crc_lo[] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
    0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
    0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
    0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
    0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
    0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
    0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
    0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
    0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
    0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
    0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
    0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
    0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
    0x40
};

static uint16_t modbus_rtu_calc_crc(const void* data, size_t size)
{
    uint8_t crc_hi = 0xff;
    uint8_t crc_lo = 0xff;
    
    size_t index = 0;
    
    for(; size != 0; size --){
        index = crc_hi ^ *(uint8_t*)data ++;
        crc_hi = crc_lo ^ table_crc_hi[index];
        crc_lo = table_crc_lo[index];
    }
    return ((uint16_t)crc_hi << 8) | crc_lo;
}



ALWAYS_INLINE static void modbus_rtu_message_recv_init(modbus_rtu_message_t* msg, modbus_rtu_address_t address)
{
    msg->data_size = 0;
    msg->adu.address = address;
}

ALWAYS_INLINE static err_t modbus_rtu_message_recv(modbus_rtu_message_t* msg, usart_bus_t* usart)
{
    return usart_bus_recv(usart, &msg->adu.func, MODBUS_RTU_PACKET_SIZE_MAX - 1);
}

ALWAYS_INLINE static void modbus_rtu_message_recv_done(modbus_rtu_message_t* msg, usart_bus_t* usart)
{
    size_t bytes_received = usart_bus_bytes_received(usart) + 1;
    if(bytes_received <= MODBUS_RTU_FIELDS_CRC_SIZE){
        msg->data_size = 0;
    }else{
        msg->data_size = bytes_received - MODBUS_RTU_FIELDS_CRC_SIZE;
    }
}



err_t modbus_rtu_init(modbus_rtu_t* modbus, modbus_rtu_init_t* modbus_is)
{
    if(modbus_is->usart == NULL) return E_NULL_POINTER;
    
    modbus->usart = modbus_is->usart;
    
    modbus->mode = modbus_is->mode;
    modbus->address = modbus_is->address;
    
    modbus->recv_callback = NULL;
    modbus->sent_callback = NULL;
    
    //memset(&modbus->message, 0x0, sizeof(modbus_rtu_message_t));
    modbus_rtu_message_reset(&modbus->message);
    
    return E_NO_ERROR;
}

err_t modbus_rtu_send_message(modbus_rtu_t* modbus)
{
    return usart_bus_send(modbus->usart, &modbus->message.adu, modbus->message.data_size + MODBUS_RTU_FIELDS_CRC_SIZE);
}

bool modbus_rtu_usart_rx_byte_callback(modbus_rtu_t* modbus, uint8_t byte)
{
    if(modbus->mode == MODBUS_RTU_MODE_SLAVE && modbus->address != byte){
        usart_bus_sleep(modbus->usart);
        return true;
    }
    
    modbus_rtu_message_recv_init(&modbus->message, byte);
    
    if(modbus_rtu_message_recv(&modbus->message, modbus->usart) != E_NO_ERROR){
        modbus_rtu_message_reset(&modbus->message);
        usart_bus_sleep(modbus->usart);
        return true;
    }
    
    return true;
}

bool modbus_rtu_usart_rx_callback(modbus_rtu_t* modbus)
{
    if(usart_bus_rx_error(modbus->usart) != E_NO_ERROR) return true;
    
    modbus_rtu_message_recv_done(&modbus->message, modbus->usart);
    
    uint16_t calc_crc = modbus_rtu_calc_crc(&modbus->message.adu, modbus->message.data_size + MODBUS_RTU_FIELDS_SIZE);
    uint16_t crc = modbus_rtu_message_crc(&modbus->message);
    
    if(crc != calc_crc) return true;
    
    if(modbus->recv_callback) modbus->recv_callback();
    
    return true;
}

bool modbus_rtu_usart_tx_callback(modbus_rtu_t* modbus)
{
    return true;
}

modbus_rtu_msg_recv_callback_t modbus_rtu_msg_recv_callback(modbus_rtu_t* modbus)
{
    return modbus->recv_callback;
}

void modbus_rtu_set_msg_recv_callback(modbus_rtu_t* modbus, modbus_rtu_msg_recv_callback_t callback)
{
    modbus->recv_callback = callback;
}

modbus_rtu_msg_sent_callback_t modbus_rtu_msg_sent_callback(modbus_rtu_t* modbus)
{
    return modbus->sent_callback;
}

void modbus_rtu_set_msg_sent_callback(modbus_rtu_t* modbus, modbus_rtu_msg_sent_callback_t callback)
{
    modbus->sent_callback = callback;
}

modbus_rtu_message_t* modbus_rtu_message(modbus_rtu_t* modbus)
{
    return &modbus->message;
}

void modbus_rtu_message_reset(modbus_rtu_message_t* message)
{
    message->adu.address = 0;
    message->adu.func = 0;
    message->data_size = 0;
}

modbus_rtu_address_t modbus_rtu_message_address(modbus_rtu_message_t* message)
{
    return message->adu.address;
}

void modbus_rtu_message_set_address(modbus_rtu_message_t* message, modbus_rtu_address_t address)
{
    message->adu.address = address;
}

modbus_rtu_func_t modbus_rtu_message_func(modbus_rtu_message_t* message)
{
    return message->adu.func;
}

void modbus_rtu_message_set_func(modbus_rtu_message_t* message, modbus_rtu_func_t func)
{
    message->adu.func = func;
}

size_t modbus_rtu_message_data_size(modbus_rtu_message_t* message)
{
    return message->data_size;
}

err_t modbus_rtu_message_set_data_size(modbus_rtu_message_t* message, size_t size)
{
    if(size > MODBUS_RTU_DATA_SIZE_MAX) return E_INVALID_VALUE;
    
    message->data_size = size;
    
    return E_NO_ERROR;
}

void* modbus_rtu_message_data_ptr(modbus_rtu_message_t* message)
{
    return message->adu.data_and_crc;
}

err_t modbus_rtu_message_copy_data(modbus_rtu_message_t* message, const void* data, size_t size)
{
    if(size > MODBUS_RTU_DATA_SIZE_MAX) return E_INVALID_VALUE;
    if(size == 0) return E_INVALID_VALUE;
    if(data == NULL) return E_NULL_POINTER;
    
    memcpy(message->adu.data_and_crc, data, size);
    
    message->data_size = size;
    
    return E_NO_ERROR;
}

uint16_t modbus_rtu_message_crc(modbus_rtu_message_t* message)
{
    uint8_t crc_lo = message->adu.data_and_crc[message->data_size];
    uint8_t crc_hi = message->adu.data_and_crc[message->data_size + 1];
    
    return ((uint16_t)crc_hi << 8) | crc_lo;
}

uint16_t modbus_rtu_message_calc_crc(modbus_rtu_message_t* message)
{
    uint16_t crc = modbus_rtu_calc_crc(&message->adu, message->data_size + MODBUS_RTU_FIELDS_SIZE);
    
    message->adu.data_and_crc[message->data_size] = crc & 0xff;
    message->adu.data_and_crc[message->data_size + 1] = crc >> 8;
    
    return crc;
}
