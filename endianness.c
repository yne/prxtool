/***************************************************************
 * PRXTool : Utility for PSP executables.
 * (c) TyRaNiD 2k5
 *
 * types.h - Definition of basic cross platform types.
 ***************************************************************/

#ifndef __TYPES_H__
#define __TYPES_H__

#ifdef WORDS_BIGENDIAN
uint32_t lw_le(uint32_t data){
	uint8_t *ptr;
	uint32_t val;

	ptr = (uint8_t*) &data;
	val = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);

	return val;
}

uint16_t lh_le(uint16_t data){
	uint8_t *ptr;
	uint16_t val;

	ptr = (uint8_t*) &data;

	val = ptr[0] | (ptr[1] << 8);

	return val;
}

#define LW_LE(x) (lw_le((x)))
#define LW_BE(x) (x)
#define LH_LE(x) (lh_le((x)))
#define LH_BE(x) (x)

#else

uint32_t lw_be(uint32_t data){
	uint8_t *ptr;
	uint32_t val;

	ptr = (uint8_t*) &data;

	val = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];

	return val;
}

uint16_t lh_be(uint16_t data){
	uint8_t *ptr;
	uint16_t val;

	ptr = (uint8_t*) &data;

	val = (ptr[0] << 16) | ptr[1];

	return val;
}

#define LW_LE(x) (x)
#define LW_BE(x) (lw_be((x)))
#define LH_LE(x) (x)
#define LH_BE(x) (lh_be((x)))

#endif

#define LW(x) (LW_LE(x))
#define LH(x) (LH_LE(x))


#ifdef WORDS_BIGENDIAN
void sw_le(uint32_t *data, uint32_t val){
	uint8_t* ptr = (uint8_t*) data;

	ptr[0] = (uint8_t) (val & 0xFF);
	ptr[1] = (uint8_t) ((val >> 8) & 0xFF);
	ptr[2] = (uint8_t) ((val >> 16) & 0xFF);
	ptr[3] = (uint8_t) ((val >> 24) & 0xFF);
}

void sh_le(uint16_t *data, uint16_t val){
	uint8_t *ptr = (uint8_t*) data;

	ptr[0] = (uint8_t) (val & 0xFF);
	ptr[1] = (uint8_t) ((val >> 8) & 0xFF);
}

#define SW_LE(x, v) (sw_le((uint32_t*) &(x), (v)))
#define SW_BE(x, v) ((x) = (v))
#define SH_LE(x, v) (sh_le((uint16_t*) &(x), (v)))
#define SH_BE(x, v) ((x) = (v))

#else

void sw_be(uint32_t *data, uint32_t val){
	uint8_t *ptr = (uint8_t*) data;

	ptr[0] = (uint8_t) ((val >> 24) & 0xFF);
	ptr[1] = (uint8_t) ((val >> 16) & 0xFF);
	ptr[2] = (uint8_t) ((val >> 8) & 0xFF);
	ptr[3] = (uint8_t) (val & 0xFF);
}

void sh_be(uint16_t *data, uint16_t val){
	uint8_t* ptr = (uint8_t*) data;

	ptr[0] = (uint8_t) ((val >> 8) & 0xFF);
	ptr[1] = (uint8_t) (val & 0xFF);
}

#define SW_LE(x, v) ((x) = (v))
#define SW_BE(x, v) (sw_be((uint32_t*) &(x), (v)))
#define SH_LE(x, v) ((x) = (v))
#define SH_BE(x, v) (sh_be((uint16_t*) &(x), (v)))

#endif

#define SW(x, v) (SW_LE(x, v))
#define SH(x, v) (SH_LE(x, v))


/* Do a safe alloc which should work on vc6 or latest gcc etc */
/* If alloc fails will always return NULL */
#define SAFE_ALLOC(p, t) try { (p) = new t; } catch(...) { (p) = NULL; }


#endif
