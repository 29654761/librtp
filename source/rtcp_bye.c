/**
 * @file rtcp_bye.c
 * @brief RTCP goodbye packet.
 * @author Wilkins White
 * @copyright 2022 Daxbot
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "rtp/rtcp_bye.h"
#include "util.h"

rtcp_bye *rtcp_bye_create()
{
    rtcp_bye *packet = (rtcp_bye*)malloc(sizeof(rtcp_bye));
    if(packet)
        memset(packet, 0, sizeof(rtcp_bye));

    return packet;
}

void rtcp_bye_free(rtcp_bye *packet)
{
    if(!packet)
        return;

    if(!packet->src_ids)
        free(packet->src_ids);

    if(!packet->message)
        free(packet->message);

    free(packet);
}

int rtcp_bye_init(rtcp_bye *packet)
{
    if(!packet)
        return -1;

    packet->header.common.version = 2;
    packet->header.common.pt = RTCP_BYE;

    return 0;
}

int rtcp_bye_size(const rtcp_bye *packet)
{
    if(!packet)
        return -1;

    int size = 4 + (packet->header.common.count * 4);
    if(packet->message) {
        size += strlen(packet->message);
        size += 4 - (size % 4);
    }

    return size;
}

int rtcp_bye_serialize(const rtcp_bye *packet, uint8_t *buffer, int size)
{
    if(!packet || !buffer)
        return -1;

    const int packet_size = rtcp_bye_size(packet);
    if(size < packet_size)
        return -1;

    if(rtcp_header_serialize(&packet->header, buffer, size) < 0)
        return -1;

    int offset = 4;
    for(uint8_t i = 0; i < packet->header.common.count; ++i) {
        write_u32(buffer + offset, packet->src_ids[i]);
        offset += 4;
    }

    if(packet->message) {
        const uint8_t size = strlen(packet->message);
        *(buffer + offset++) = size;

        memcpy(buffer + offset, packet->message, size);
        offset += size;
    }

    return packet_size;
}

int rtcp_bye_parse(
    rtcp_bye *packet, const uint8_t *buffer, int size)
{
    if(!packet || !buffer || size < 4)
        return -1;

    const int pt = rtcp_header_parse(&packet->header, buffer, size);
    if(pt != RTCP_BYE)
        return -1;

    int offset = 4;
    if(packet->header.common.count) {
        // Parse sources
        if(size < 4 + (4 * packet->header.common.count)) {
            free(packet);
            return -1;
        }

        packet->src_ids = (uint32_t*)calloc(
            packet->header.common.count, sizeof(uint32_t));

        for(uint8_t i = 0; i < packet->header.common.count; ++i) {
            packet->src_ids[i] = read_u32(buffer + offset);
            offset += 4;
        }
    }

    if(size > offset) {
        // Store message
        int length = buffer[offset++];
        if(length > size - offset)
            length = size - offset;

        packet->message = (char *)malloc(length + 1);
        memcpy(packet->message, buffer + offset, length);
        packet->message[length] = '\0';
    }

    return 0;
}

int rtcp_bye_find_source(const rtcp_bye *packet, uint32_t src_id)
{
    if(packet) {
        for(uint8_t i = 0; i < packet->header.common.count; ++i) {
            if(packet->src_ids[i] == src_id)
                return i;
        }
    }

    return -1;
}

int rtcp_bye_add_source(rtcp_bye *packet, uint32_t src_id)
{
    if(!packet)
        return -1;

    if(!packet->header.common.count) {
        packet->src_ids = (uint32_t*)malloc(sizeof(uint32_t));
        packet->header.common.count = 1;
    }
    else {
        if(packet->header.common.count == 0xff)
            return -1;

        if(rtcp_bye_find_source(packet, src_id) != -1)
            return -1;

        packet->header.common.count += 1;

        const size_t nmemb = packet->header.common.count * sizeof(uint32_t);
        packet->src_ids = (uint32_t*)realloc(packet->src_ids, nmemb);
    }

    packet->src_ids[packet->header.common.count - 1] = src_id;

    // Update header length
    packet->header.common.length = (rtcp_bye_size(packet) / 4) - 1;

    return 0;
}

int rtcp_bye_remove_source(rtcp_bye *packet, uint32_t src_id)
{
    if(!packet)
        return -1;

    const int index = rtcp_bye_find_source(packet, src_id);
    if(index < 0)
        return 0;

    const size_t size = (packet->header.common.count - index) * sizeof(uint32_t);
    if(size)
        memmove(&packet->src_ids[index], &packet->src_ids[index + 1], size);

    packet->header.common.count -= 1;
    if(packet->header.common.count > 0) {
        const size_t nmemb = packet->header.common.count * sizeof(uint32_t);
        packet->src_ids = (uint32_t*)realloc(packet->src_ids, nmemb);
    }
    else {
        free(packet->src_ids);
        packet->src_ids = NULL;
    }

    // Update header length
    packet->header.common.length = (rtcp_bye_size(packet) / 4) - 1;

    return 0;
}

int rtcp_bye_set_message(rtcp_bye *packet, const char *message)
{
    if(!packet || !message)
        return -1;

    if(packet->message)
        free(packet->message);

    const size_t size = strlen(message);
    packet->message = (char*)malloc(size + 1);
    sprintf(packet->message, "%s", message);

    // Update header length
    packet->header.common.length = (rtcp_bye_size(packet) / 4) - 1;

    return 0;
}

int rtcp_bye_clear_message(rtcp_bye *packet)
{
    if(!packet)
        return -1;

    free(packet->message);
    packet->message = NULL;

    // Update header length
    packet->header.common.length = (rtcp_bye_size(packet) / 4) - 1;

    return 0;
}