/**
 * @copyright Copyright © 2013-2014, Rafał Słota, Konrad Zemek
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file ip.h This file contains functions and definitions connected to the
 * Internet Protocol layer.
 */

#ifndef TCPIPStack_ip_h
#define TCPIPStack_ip_h


#include "common.h"

#include <stdint.h>
#include <stddef.h>

#define IP_HEADER_LEN     40
#define IP_PACKET_MAX_LEN 1280
#define IP_DATA_MAX_LEN   (IP_PACKET_MAX_LEN - IP_HEADER_LEN)
#define IP_PROTOCOL_ICMP  0x3A
#define IP_PROTOCOL_TCP   0x06
#define IP_PROTOCOL_UDP   0x11

/**
 * Sends data through the ip layer, to the ethernet layer.
 * @param session The session object created by net_init().
 * @param dst_ip The IP address of datagram's recipient.
 * @param protocol The upper layer protocol number.
 * @param data The data to be sent in the IP datagram.
 * @param data_len The length of data.
 * @returns Number of bytes of data that were sent on success, 0 on error.
 */
size_t ip_send(session_t *session, const uint8_t dst_ip[], uint8_t protocol,
               const uint8_t data[], size_t data_len);

/**
 * Receives data from the ethernet layer, through the ip layer.
 * @param session The session object created by net_init().
 * @param buffer The buffer to which the data will be written.
 * @param buffer_len The length of the buffer.
 * @returns Number of bytes written into the buffer, 0 on error.
 */
size_t ip_recv(session_t *session, uint8_t buffer[], const size_t buffer_len);

/**
 * Calculates the checksum used by some transport layer protocols.
 * @param session The session object created by net_init().
 * @param dst_ip The destination IP address of the IP datagram.
 * @param protocol The protocol number corresponding to the used transport
 * layer protocol.
 * @param data The whole payload of the resulting IP packet, except any
 * extension headers; the checksum field must be 0. The data needs to be in the
 * network byte order.
 * @param data_len The length of data.
 * @returns The calculated checksum to place in transport layer's header.
 * @note The calculated checksum is already in the network byte order.
 */
uint16_t ip_chksum(session_t *session, const uint8_t dst_ip[], uint8_t protocol,
                   const uint8_t data[], size_t data_len);


#endif
