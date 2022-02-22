/**
 * @file rtcp_report.c
 * @brief RTCP SR/RR report block.
 * @author Wilkins White
 * @copyright 2022 Daxbot
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "rtp/rtcp_report.h"
#include "util.h"

int rtcp_report_serialize(
    const rtcp_report *report, uint8_t *buffer, int size)
{
    assert(report != NULL);
    assert(buffer != NULL);

    if(size < 24)
        return -1;

    write_u32(buffer, report->ssrc);

    buffer[4] = report->fraction;
    write_s24(buffer + 5, report->lost);

    write_u32(buffer + 8, report->last_seq);
    write_u32(buffer + 12, report->jitter);
    write_u32(buffer + 16, report->lsr);
    write_u32(buffer + 20, report->dlsr);

    return 24;
}

int rtcp_report_parse(rtcp_report *report, const uint8_t *buffer, int size)
{
    assert(report != NULL);
    assert(buffer != NULL);

    if(size < 24)
        return -1;

    report->ssrc = read_u32(buffer);

    report->fraction = buffer[4];
    report->lost = read_s24(buffer + 5);
    report->last_seq = read_u32(buffer + 8);
    report->jitter = read_u32(buffer + 12);
    report->lsr = read_u32(buffer + 16);
    report->dlsr = read_u32(buffer + 20);

    return 0;
}

void rtcp_report_set_fraction(rtcp_report *report, float percent_lost)
{
    assert(report != NULL);

    report->fraction = percent_lost * 255.0;
}

float rtcp_report_get_fraction(rtcp_report *report)
{
    assert(report != NULL);

    return (float)(report->fraction) / 255.0;
}
