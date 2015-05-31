/**************************************************************************/
/*!
    @file     Adafruit_BLE.c
    @author   hathach

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2014, Adafruit Industries (adafruit.com)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**************************************************************************/
#include "Adafruit_BLE.h"

/******************************************************************************/
/*!
    @brief  Performs a system reset using AT command
*/
/******************************************************************************/
bool Adafruit_BLE::reset(void)
{
  bool isOK;
  // println();
  for (uint8_t t=0; t < 5; t++) {
    isOK = sendCommandCheckOK(F("ATZ"));

    if (isOK) break;
  }
  if (! isOK) {
    // ok we're going to get desperate
    delay(50);
    println("+++");
    delay(50);
    
    for (uint8_t t=0; t < 5; t++) {
      isOK = sendCommandCheckOK(F("ATZ"));
      
      if (isOK) break;
    }

    if (!isOK)
      return false;
  }

  // Bluefruit need 1 second to reboot
  delay(1000);

  // flush all left over
  flush();

  return isOK;
}

/******************************************************************************/
/*!
    @brief  Performs a factory reset
*/
/******************************************************************************/
bool Adafruit_BLE::factoryReset(void)
{
  bool isOK = false;
  while (! isOK) {
    println("AT+FACTORYRESET");
    isOK = waitForOK();
    // Bluefruit need 1 second to reboot
    delay(1000);
  }

  // flush all left over
  flush();

  return isOK;
}

/******************************************************************************/
/*!
    @brief  Enable or disable AT Command echo from Bluefruit

    @parma[in] enable
               true to enable (default), false to disable
*/
/******************************************************************************/
bool Adafruit_BLE::echo(bool enable)
{
  print("ATE=");
  println((int)enable);

  return waitForOK();
}

/******************************************************************************/
/*!
    @brief  Check connection state, returns true is connected!
*/
/******************************************************************************/
bool Adafruit_BLE::isConnected(void)
{
  int32_t connected = 0;
  sendCommandWithIntReply(F("AT+GAPGETCONN"), &connected);
  return connected;
}


/******************************************************************************/
/*!
    @brief  Print Bluefruit's information retrieved by ATI command
*/
/******************************************************************************/
void Adafruit_BLE::info(void)
{
  bool v = _verbose;
  _verbose = false;

  Serial.println(F("----------------"));

  println("ATI");

  while (readline(_timeout)) {
    Serial.println(buffer);
    if (strcmp(buffer, "OK") == 0) break;
  }

  Serial.println(F("----------------"));

  _verbose = v;
}


/******************************************************************************/
/*!
    @brief  Send a command from a flash string, and parse an int reply
*/
/******************************************************************************/
bool Adafruit_BLE::sendCommandWithIntReply(const __FlashStringHelper *cmd, int32_t *reply) {
  println(cmd); // the easy part
  
  if (_verbose) {
    Serial.print("\n<- ");
  }
  (*reply) = readln_parseInt();
  return waitForOK();
}


/******************************************************************************/
/*!
    @brief  Send a command from a flash string, and parse an int reply
*/
/******************************************************************************/
bool Adafruit_BLE::sendCommandCheckOK(const __FlashStringHelper *cmd) {
  println(cmd); // the easy part
  return waitForOK();
}

/******************************************************************************/
/*!
    @brief  Read one line of response data from internal FIFO to the buffer,
            perform low level data transaction if needed.

    @param[in] buffer
               Buffer to store read data
    @param[in] length
               Buffer's length

    @return Number of bytes are read. If it is equal to the \ref length, the whole
            line (or line feed) is not fully retrieved.
    
    @note   Timeout for each read() character is _timeout (see \ref Stream
            implementation). _timeout can be changed by setTimeout()        
*/
/******************************************************************************/
size_t Adafruit_BLE::readln( char *buffer, size_t length)
{
  size_t len = readBytesUntil('\r', buffer, length);

  if ( (len > 0) && (len < length) )
  {
    // Add null terminator
    buffer[len] = 0;

    // skip \n
//    if (peek() == '\n' ) read();
    if ( timedPeek() == '\n' ) read();
  }

  return len;
}

/******************************************************************************/
/*!
    @brief  Read one line of response data from internal FIFO to the interna buffer
    @note   Timeout for each read() character is _timeout (see \ref Stream
            implementation). _timeout can be changed by setTimeout()
*/
/******************************************************************************/
size_t Adafruit_BLE::readln(void)
{
  size_t len = readln(buffer, BLE_BUFSIZE);
  buffer[len] = 0;

  return len;
}


/******************************************************************************/
/*!
    @brief  Read the whole response and check if it ended up with OK.
    @return true if response is ended with "OK". Otherwise it could be "ERROR"
*/
/******************************************************************************/
bool Adafruit_BLE::waitForOK(void)
{
  if (_verbose) {
    Serial.print("\n<- ");
  }

  while (readline(_timeout)) {
    //Serial.println(buffer);
    if ( strcmp(buffer, "OK") == 0 ) return true;
    if ( strcmp(buffer, "ERROR") == 0 ) return false;
  }
  return false;
}

/******************************************************************************/
/*!
    @brief  Get a line of response data (see \ref readln) and try to interpret
            it to an integer number. If the number is prefix with '0x', it will
            be interpreted as hex number. This function also drop the rest of
            data to the end of the line.
*/
/******************************************************************************/
int32_t Adafruit_BLE::readln_parseInt(void)
{
  size_t len = readln();
  if (len == 0) return 0;

  // also parsed hex number e.g 0xADAF
  int32_t val = strtol(buffer, NULL, 0);

  // discard the rest of the line
  while( BLE_BUFSIZE == len )
  {
    len = readln(buffer, BLE_BUFSIZE);
  }

  return val;
}



/******************************************************************************/
/*!
    @brief  Get (multiple) lines of response data (see \ref readln) to internal
            buffer.

    @param[in] timeout
               Timeout for each read() operation
    @param[in] multiline
               Read multiple lines
*/
/******************************************************************************/
uint16_t Adafruit_BLE::readline(uint16_t timeout, boolean multiline) {

  uint16_t replyidx;
  uint16_t prev_timeout = _timeout; // save default timeout
  _timeout = timeout;

  replyidx = readln(buffer, BLE_BUFSIZE);

  // Run out of time
  if (replyidx == 0) return 0;

  // Read until buffer is full or no more data
  if ( multiline ) {
    uint16_t len;
    do {
      len = readln(buffer+replyidx, BLE_BUFSIZE-replyidx);
      replyidx += len;
    }while( (len > 0) && (replyidx < BLE_BUFSIZE) );
  }

  _timeout = prev_timeout; // restore default timeout

  return replyidx;

#if 0

  /*
  if (_verbose) {
    Serial.print("\n<- ");
  }
  */

  while (timeout--) {
    while(available()) {
      char c =  read();
      //Serial.println(c);
      if (c == '\r') continue;
      if (c == 0xA) {
        if (replyidx == 0)   // the first 0x0A is ignored
          continue;
        
        if (!multiline) {
          timeout = 0;         // the second 0x0A is the end of the line
          break;
        }
      }
      buffer[replyidx] = c;
      replyidx++;

      if (replyidx >= (BLE_BUFSIZE-1)) {
	//if (_verbose) { Serial.println("*overflow*"); }  // for my debuggin' only!
	timeout = 0;
	break;
      }
    }
    
    if (timeout == 0) break;
    delay(1);
  }
  buffer[replyidx] = 0;  // null term

  return replyidx;
#endif
}


