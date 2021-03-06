/*
 HardwareSerial.cpp - esp8266 UART support

 Copyright (c) 2014 Ivan Grokhotkov. All rights reserved.
 This file is part of the esp8266 core for Arduino environment.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

 Modified 31 March 2015 by Markus Sattler (rewrite the code for UART0 + UART1 support in ESP8266)
 Modified 25 April 2015 by Thomas Flayols (add configuration different from 8N1 in ESP8266)
 Modified 3 May 2015 by Hristo Gochkov (change register access methods)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "Arduino.h"
#include "HardwareSerial.h"
#include "Esp.h"

HardwareSerial::HardwareSerial(int uart_nr)
    : _uart_nr(uart_nr), _rx_size(256)
{}

void HardwareSerial::begin(unsigned long baud, SerialConfig config, SerialMode mode, uint8_t tx_pin)
{
    end();
    _uart = uart_init(_uart_nr, baud, (int) config, (int) mode, tx_pin, _rx_size);
#if defined(DEBUG_ESP_PORT) && !defined(NDEBUG)
    if (this == &DEBUG_ESP_PORT)
    {
        setDebugOutput(true);
        println();
        println(ESP.getFullVersion());
    }
#endif
}

void HardwareSerial::end()
{
    if(uart_get_debug() == _uart_nr) {
        uart_set_debug(UART_NO);
    }

    if (_uart) {
        uart_uninit(_uart);
        _uart = NULL;
    }
}

size_t HardwareSerial::setRxBufferSize(size_t size){
    if(_uart) {
        _rx_size = uart_resize_rx_buffer(_uart, size);
    } else {
        _rx_size = size;
    }
    return _rx_size;
}

void HardwareSerial::swap(uint8_t tx_pin)
{
    if(!_uart) {
        return;
    }
    uart_swap(_uart, tx_pin);
}

void HardwareSerial::set_tx(uint8_t tx_pin)
{
    if(!_uart) {
        return;
    }
    uart_set_tx(_uart, tx_pin);
}

void HardwareSerial::pins(uint8_t tx, uint8_t rx)
{
    if(!_uart) {
        return;
    }
    uart_set_pins(_uart, tx, rx);
}

void HardwareSerial::setDebugOutput(bool en)
{
    if(!_uart) {
        return;
    }
    if(en) {
        if(uart_tx_enabled(_uart)) {
            uart_set_debug(_uart_nr);
        } else {
            uart_set_debug(UART_NO);
        }
    } else {
        // disable debug for this interface
        if(uart_get_debug() == _uart_nr) {
            uart_set_debug(UART_NO);
        }
    }
}

bool HardwareSerial::isTxEnabled(void)
{
    return _uart && uart_tx_enabled(_uart);
}

bool HardwareSerial::isRxEnabled(void)
{
    return _uart && uart_rx_enabled(_uart);
}

int HardwareSerial::available(void)
{
    int result = static_cast<int>(uart_rx_available(_uart));
    if (!result) {
        optimistic_yield(10000);
    }
    return result;
}

int HardwareSerial::peek(void)
{
    // this may return -1, but that's okay
    return uart_peek_char(_uart);
}

int HardwareSerial::read(void)
{
    // this may return -1, but that's okay
    return uart_read_char(_uart);
}

int HardwareSerial::availableForWrite(void)
{
    if(!_uart || !uart_tx_enabled(_uart)) {
        return 0;
    }

    return static_cast<int>(uart_tx_free(_uart));
}

void HardwareSerial::flush()
{
    if(!_uart || !uart_tx_enabled(_uart)) {
        return;
    }

    uart_wait_tx_empty(_uart);
    //Workaround for a bug in serial not actually being finished yet
    //Wait for 8 data bits, 1 parity and 2 stop bits, just in case
    delayMicroseconds(11000000 / uart_get_baudrate(_uart) + 1);
}

size_t HardwareSerial::write(uint8_t c)
{
    if(!_uart || !uart_tx_enabled(_uart)) {
        return 0;
    }

    uart_write_char(_uart, c);
    return 1;
}

int HardwareSerial::baudRate(void)
{
    // Null pointer on _uart is checked by SDK
    return uart_get_baudrate(_uart);
}


HardwareSerial::operator bool() const
{
    return _uart != 0;
}


#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_SERIAL)
HardwareSerial Serial(UART0);
#endif
#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_SERIAL1)
HardwareSerial Serial1(UART1);
#endif

