/*
 * Copyright (C) 2019  Cascoda, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CA821X_API_INCLUDE_CA821X_ENDIAN_H_
#define CA821X_API_INCLUDE_CA821X_ENDIAN_H_

#include <stdint.h>

#define LS_BYTE(x) ((uint8_t)((x)&0xFF))
#define MS_BYTE(x) ((uint8_t)(((x) >> 8) & 0xFF))
#define LS0_BYTE(x) ((uint8_t)((x)&0xFF))
#define LS1_BYTE(x) ((uint8_t)(((x) >> 8) & 0xFF))
#define LS2_BYTE(x) ((uint8_t)(((x) >> 16) & 0xFF))
#define LS3_BYTE(x) ((uint8_t)(((x) >> 24) & 0xFF))

static inline uint16_t GETLE16(const uint8_t *in)
{
	return ((in[1] << 8) & 0xff00) | (in[0] & 0x00ff);
}

static inline uint32_t GETLE32(const uint8_t *in)
{
	return (((uint32_t)in[3] << 24) + ((uint32_t)in[2] << 16) + ((uint32_t)in[1] << 8) + (uint32_t)in[0]);
}

static inline void PUTLE16(uint16_t in, uint8_t *out)
{
	out[0] = in & 0xff;
	out[1] = (in >> 8) & 0xff;
}

static inline void PUTLE32(uint32_t in, uint8_t *out)
{
	out[0] = in & 0xff;
	out[1] = (in >> 8) & 0xff;
	out[2] = (in >> 16) & 0xff;
	out[3] = (in >> 24) & 0xff;
}

static inline uint16_t GETBE16(const uint8_t *in)
{
	return ((in[0] << 8) & 0xff00) | (in[1] & 0x00ff);
}

static inline void PUTBE16(uint16_t in, uint8_t *out)
{
	out[1] = in & 0xff;
	out[0] = (in >> 8) & 0xff;
}

static inline uint32_t GETBE32(const uint8_t *in)
{
	return (((uint32_t)in[0] << 24) + ((uint32_t)in[1] << 16) + ((uint32_t)in[2] << 8) + (uint32_t)in[3]);
}

static inline void PUTBE32(uint32_t in, uint8_t *out)
{
	out[3] = in & 0xff;
	out[2] = (in >> 8) & 0xff;
	out[1] = (in >> 16) & 0xff;
	out[0] = (in >> 24) & 0xff;
}

#endif /* CA821X_API_INCLUDE_CA821X_ENDIAN_H_ */
