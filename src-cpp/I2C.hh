#pragma once

#include "util.hh"
#include "queue.hh"

#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/twi.h>

// See http://www.chrisherring.net/all/tutorial-interrupt-driven-twi-interface-for-avr-part1/

enum TWIMode {
  TWIMaster,
  TWISlave
};

class TWIMaster {
public:
  static void setup(uint32_t clockFrequency) {
  	_state = kReady;
  	_errorCode = 0xFF;
  	_repStart = 0;
  	// Set prescaler (no prescaling) (TODO: set correct prescaler)
    // Note that no prescaler is necessary for 100kHz clock
  	TWSR = 0;
  	// Set bit rate
  	TWBR = ((F_CPU / clockFrequency) - 16) / 2;
  	// Enable TWI and interrupt
  	TWCR = (1 << TWIE) | (1 << TWEN);
  }
  
  static byte writeData(byte *const txData, byte bytesToWrite, bool repStart = false) {
    _txData = txData;
    _txDataLength = bytesToWrite;
  }
  
  static bool readData(byte TWIaddr, byte *rxData, byte bytesToRead, bool repStart = false) {
    _rxData = rxData;
    _rxDataLength = bytesToRead;
  }
  
  static bool isReady(void) {
  	return (_state == kReady) || (_state == kRepeatedStartSent);
  }
  
  static bool onTWIInterrupt() {
  	switch (TW_STATUS)
  	{
  		// ----\/ ---- MASTER TRANSMITTER OR WRITING ADDRESS ----\/ ----  //
  		case TW_MT_SLA_ACK: // SLA+W transmitted and ACK received
  		  // Set mode to Master Transmitter
  		  _state = kTransmit;
        // And fall through to next statement
        ;
  		case TW_START: // Start condition has been transmitted
  		case TW_MT_DATA_ACK: // Data byte has been transmitted, ACK received
  			if (_txDataLength > 0) // If there is more data to send
  			{
  				TWDR = *_txData++;
          _txDataLength--; // Load data to transmit buffer
  				_errorCode = TW_NO_INFO;
  				TWISendTransmit(); // Send the data
  			}
  			// This transmission is complete however do not release bus yet
  			else if (_repStart)
  			{
  				_errorCode = 0xFF;
  				TWISendStart();
  			}
  			// All transmissions are complete, exit
  			else
  			{
  				_state = kReady;
  				_errorCode = 0xFF;
  				TWISendStop();
  			}
  			break;
		
  		// ----\/ ---- MASTER RECEIVER ----\/ ----  //
		
  		case TW_MR_SLA_ACK: // SLA+R has been transmitted, ACK has been received
  			// Switch to Master Receiver mode
  			_state = kReceive;
  			// If there is more than one byte to be read, receive data byte and return an ACK
  			if (_rxDataLength > 1)
  			{
  				_errorCode = TW_NO_INFO;
  				TWISendACK();
  			}
  			// Otherwise when a data byte (the only data byte) is received, return NACK
  			else
  			{
  				_errorCode = TW_NO_INFO;
  				TWISendNACK();
  			}
  			break;
		
  		case TW_MR_DATA_ACK: // Data has been received, ACK has been transmitted.
		
  			/// -- HANDLE DATA BYTE --- ///
  			*_rxData++ = TWDR;
        _rxDataLength--;
  			// If there is more than one byte to be read, receive data byte and return an ACK
  			if (_rxDataLength > 1)
  			{
  				_errorCode = TW_NO_INFO;
  				TWISendACK();
  			}
  			// Otherwise when a data byte (the only data byte) is received, return NACK
  			else
  			{
  				_errorCode = TW_NO_INFO;
  				TWISendNACK();
  			}
  			break;
		
  		case TW_MR_DATA_NACK: // Data byte has been received, NACK has been transmitted. End of transmission.
		
  			/// -- HANDLE DATA BYTE --- ///
  			*_rxData+++ = TWDR;	
        _rxDataLength--;
  			// This transmission is complete however do not release bus yet
  			if (_repStart)
  			{
  				_errorCode = 0xFF;
  				TWISendStart();
  			}
  			// All transmissions are complete, exit
  			else
  			{
  				_state = kReady;
  				_errorCode = 0xFF;
  				TWISendStop();
  			}
  			break;
		
  		// ----\/ ---- MT and MR common ----\/ ---- //
		
  		case TW_MR_SLA_NACK: // SLA+R transmitted, NACK received
  		case TW_MT_SLA_NACK: // SLA+W transmitted, NACK received
  		case TW_MT_DATA_NACK: // Data byte has been transmitted, NACK received
  		case TW_MT_ARB_LOST: // Arbitration has been lost
  			// Return error and send stop and set mode to ready
  			if (_repStart)
  			{				
  				_errorCode = TW_STATUS;
  				TWISendStart();
  			}
  			// All transmissions are complete, exit
  			else
  			{
  				_state = kReady;
  				_errorCode = TW_STATUS;
  				TWISendStop();
  			}
  			break;
  		case TW_REP_START: // Repeated start has been transmitted
  			// Set the mode but DO NOT clear TWINT as the next data is not yet ready
  			_state = kRepeatedStartSent;
  			break;
		
  		// ----\/ ---- SLAVE RECEIVER ----\/ ----  //
		
  		// TODO  IMPLEMENT SLAVE RECEIVER FUNCTIONALITY
		
  		// ----\/ ---- SLAVE TRANSMITTER ----\/ ----  //
		
  		// TODO  IMPLEMENT SLAVE TRANSMITTER FUNCTIONALITY
		
  		// ----\/ ---- MISCELLANEOUS STATES ----\/ ----  //
  		case TW_NO_INFO: 
        // It is not really possible to get into this ISR on this condition
  			// Rather, it is there to be manually set between operations
  			break;
  		case TW_BUS_ERROR: // Illegal START/STOP, abort and return error
  			_errorCode = TW_STATUS;
  			_state = kReady;
  			TWISendStop();
  			break;
  	}    
  }
  
private:
  
  enum State {
  	kReady,
  	kInitializing,
  	kRepeatedStartSent,
  	kTransmit,
  	kReceive
  };
  
  static State   _state;
  static byte    _errorCode;
  static byte    _repStart;	
  
  static byte   * _txData;  
  static byte   * _rxData;   // Global receive buffer
  // Buffer lengths
  static byte   _txDataLength; // The total length of the transmit buffer
  static byte   _rxDataLength; // The total number of bytes to read (should be less than RXMAXBUFFLEN)
  
  
  /************************ Helper methods ************************/
  
  // Send the START signal, enable interrupts and TWI, clear TWINT flag to resume transfer.
  static void TWISendStart() { 
    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)|(1<<TWIE);
  }
  
  // Send the STOP signal, enable interrupts and TWI, clear TWINT flag.
  static void TWISendStop() { 
    TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN)|(1<<TWIE);
  } 

  // Used to resume a transfer, clear TWINT and ensure that TWI and interrupts are enabled.
  static void TWISendTransmit()	{
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE);
  } 
  
  // FOR MR mode. Resume a transfer, ensure that TWI and interrupts are enabled 
  // and respond with an ACK if the device is addressed as a slave or after it receives a byte.
  static void TWISendACK() {
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE)|(1<<TWEA);
  } 

  // FOR MR mode. Resume a transfer, ensure that TWI and interrupts are enabled 
  // but DO NOT respond with an ACK if the device is addressed as a slave or after 
  // it receives a byte.
  static void TWISendNACK() { 
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE);
  }
};
