# 🎵 MIDI Library

## Transport

Interface to send and receive MIDI packets

-   **USBDevice** – USB MIDI class device provided by the core

-   **SerialDevice** –  UART-based classic serial MIDI connection

-   **V2Link::Port** – RS422-based bidirectional serial connection, using 5-byte packets

-   **V2Link::Packet** – Simple conversion from and to **V2MIDI::Packet**

## Port

Transport-independent MIDI interface

Supports message parsing/dispatching, system exclusive buffering/streaming,
packet statistics. USB devices support up to 16 ports which show up as individual
MIDI ports to applications. In simple setups one **Port** is connected to
one **USBDevice**. Multiple transports can share one **Port**, like **V2Link**
and **USBDevice**.

## Packet

MIDI packet

4 byte USB class device version 1 format

## HighResolution

High-resolution **Continuous Controller** support

Combine MSB + LSB to a 14 bit value for sending and receiving.

## Clock

MIDI Beat Clock

Support for **Song Position Pointer** counting, **Start**, **Stop**, **Continue**.
