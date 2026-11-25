/* Variable-length MAC SDU subheaders, non-eLCID R/F/LCID/L format.
 * TS 38.321 V19.4.0 6.1.2 and 6.2.1: L < 256 uses F=0 / 8-bit L.
 * No scheduling, logical-channel priority, or NR timing policy is selected here.
 */
#ifndef NR_MAC_SDU_H
#define NR_MAC_SDU_H

#include <assert.h>
#include <stdint.h>
#include <string.h>

static inline uint8_t nr_mac_sdu_header_length(uint32_t length)
{
  return length < 256 ? 2 : 3;
}

/* Largest positive L satisfying L + header_length(L) <= available.
 * At 258 bytes the best choice remains L=255 (one byte cannot be used),
 * whereas 259 bytes admits L=256. Zero means no positive MAC SDU fits.
 */
static inline uint32_t nr_mac_sdu_payload_capacity(int32_t available)
{
  if (available < 3)
    return 0;
  if (available < 259)
    return available - 2 < 255 ? available - 2 : 255;
  return available - 3 < UINT16_MAX ? available - 3 : UINT16_MAX;
}

/* Payload was produced after a safe two/three-byte reservation. Compact it
 * before writing the actual subheader if RLC returned a short PDU.
 */
static inline uint8_t nr_mac_sdu_write(uint8_t *header, uint8_t lcid, uint16_t length, uint8_t reserved)
{
  const uint8_t size = nr_mac_sdu_header_length(length);
  assert(length > 0 && (reserved == 2 || reserved == 3) && size <= reserved);
  if (size != reserved)
    memmove(header + size, header + reserved, length);
  header[0] = (lcid & 0x3f) | (size == 3 ? 0x40 : 0);
  if (size == 2) {
    header[1] = length;
  } else {
    header[1] = length >> 8;
    header[2] = length & 0xff;
  }
  return size;
}
#endif
