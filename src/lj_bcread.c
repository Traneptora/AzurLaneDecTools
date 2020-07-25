/*
** Bytecode reader.
** Copyright (C) 2005-2017 Mike Pall. See Copyright Notice in luajit.h
*/

#define lj_bcread_c
#define LUA_CORE

#include "lj_obj.h"
#include "lj_gc.h"
#include "lj_err.h"
#include "lj_buf.h"
#include "lj_str.h"
#include "lj_tab.h"
#include "lj_bc.h"
#if LJ_HASFFI
#include "lj_ctype.h"
#include "lj_cdata.h"
#include "lualib.h"
#endif
#include "lj_lex.h"
#include "lj_bcdump.h"
#include "lj_state.h"
#include "lj_strfmt.h"

#include <emmintrin.h>
#include <tmmintrin.h>
#include "defs.h"

typedef union vec4 {
    float m128_f32[4];
    uint64_t m128_u64[2];
    int8_t m128_i8[16];
    int16_t m128_i16[8];
    int32_t m128_i32[4];
    int64_t m128_i64[2];
    uint8_t m128_u8[16];
    uint16_t m128_u16[8];
    uint32_t m128_u32[4];
    __m128 m128;
} vec4;
typedef union vec4i {
    uint64_t m128i_u64[2];
    int8_t m128i_i8[16];
    int16_t m128i_i16[8];
    int32_t m128i_i32[4];
    int64_t m128i_i64[2];
    uint8_t m128i_u8[16];
    uint16_t m128i_u16[8];
    uint32_t m128i_u32[4];
    __m128i m128i;
} vec4i;

typedef union vec4d {
    double m128d_f64[2];
    __m128d m128d;
} vec4d;

/* Reuse some lexer fields for our own purposes. */
#define bcread_flags(ls)	ls->level
#define bcread_swap(ls) \
  ((bcread_flags(ls) & BCDUMP_F_BE) != LJ_BE*BCDUMP_F_BE)
#define bcread_oldtop(L, ls)	restorestack(L, ls->lastline)
#define bcread_savetop(L, ls, top) \
  ls->lastline = (BCLine)savestack(L, (top))

/* -- Input buffer handling ----------------------------------------------- */

/* Throw reader error. */
static LJ_NOINLINE void bcread_error(LexState *ls, ErrMsg em)
{
	lua_State *L = ls->L;
	const char *name = ls->chunkarg;
	if (*name == BCDUMP_HEAD1) name = "(binary)";
	else if (*name == '@' || *name == '=') name++;
	lj_strfmt_pushf(L, "%s: %s", name, err2msg(em));
	lj_err_throw(L, LUA_ERRSYNTAX);
}

void  bcread_error_mod(int a1, const char *a2, int a3)
{
	int L; // esi
	char v4; // al

	L = a1;
	v4 = *a2;
	if (*a2 == 88)                              // 'X'
	{
		a2 = "(binary)";
	}
	else if (v4 == 61 || v4 == 64)
	{
		goto LABEL_5;
	}
	while (1)
	{
		lj_strfmt_pushf((lua_State*)L, "%s: %s", a2, err2msg(a3));
		lj_err_throw((lua_State*)L, 3);
	LABEL_5:
		++a2;
	}
}

/* Refill buffer. */
static LJ_NOINLINE void bcread_fill(LexState *ls, MSize len, int need)
{
	lua_assert(len != 0);
	if (len > LJ_MAX_BUF || ls->c < 0)
		bcread_error(ls, LJ_ERR_BCBAD);
	do {
		const char *buf;
		size_t sz;
		char *p = sbufB(&ls->sb);
		MSize n = (MSize)(ls->pe - ls->p);
		if (n) {  /* Copy remainder to buffer. */
			if (sbuflen(&ls->sb))
			{  /* Move down in buffer. */
				lua_assert(ls->pe == sbufP(&ls->sb));
				if (ls->p != p) memmove(p, ls->p, n);
			}
			else
			{  /* Copy from buffer provided by reader. */
				p = lj_buf_need(&ls->sb, len);
				memcpy(p, ls->p, n);
			}
			ls->p = p;
			ls->pe = p + n;
		}
		setsbufP(&ls->sb, p + n);
		buf = ls->rfunc(ls->L, ls->rdata, &sz);  /* Get more data from reader. */
		if (buf == NULL || sz == 0) {  /* EOF? */
			if (need) bcread_error(ls, LJ_ERR_BCBAD);
			ls->c = -1;  /* Only bad if we get called again. */
			break;
		}
		if (n) {  /* Append to buffer. */
			n += (MSize)sz;
			p = lj_buf_need(&ls->sb, n < len ? len : n);
			memcpy(sbufP(&ls->sb), buf, sz);
			setsbufP(&ls->sb, p + n);
			ls->p = p;
			ls->pe = p + n;
		}
		else {  /* Return buffer provided by reader. */
			ls->p = buf;
			ls->pe = buf + sz;
		}
	} while (ls->p + len > ls->pe);
}

void bcread_fill_mod(_DWORD *a1, unsigned int a2, unsigned int a3, int a4)
{
	_DWORD *v4; // ebp
	char *v5; // ecx
	char *v6; // esi
	unsigned int v7; // edx
	int v8; // eax
	_WORD *v9; // edi
	char *buf; // eax
	char *p; // edx
	size_t v12; // ecx
	int v13; // eax
	char *v14; // ST24_4
	int v15; // edi
	unsigned int v16; // ecx
	_DWORD *v17; // edi
	unsigned int v18; // ecx
	int v19; // eax
	char v20; // al
	__int16 v21; // ax
	char *v22; // [esp-44h] [ebp-44h]
	unsigned int n; // [esp-44h] [ebp-44h]
	unsigned int len; // [esp-40h] [ebp-40h]
	int v25; // [esp-38h] [ebp-38h]
	_DWORD *v26; // [esp-30h] [ebp-30h]
	unsigned int sz; // [esp-20h] [ebp-20h]

	v4 = a1;                                      // ls
	len = a2;                                     // len
	if (a2 > 0x7FFFFF00 || a1[8] < 0)           // if (len > LJ_MAX_BUF || ls->c < 0)
		goto LABEL_41;
	v5 = (char *)a1[7];                           // v5 = ls->pe
	v6 = (char *)a1[6];                           // v6 = ls->p     // Current position in input buffer.
	v26 = a1 + 11;                                // v26 = ls->sb
	while (1)
	{
		p = (char *)v4[13];                         // char *p = sbufB(&ls->sb);  //  SBuf::b
		v12 = v5 - v6;                              // MSize n = (MSize)(ls->pe - ls->p);
		n = v12;                                    // v23 = n
		v13 = v4[13];                               // v13 = sbufB(&ls->sb);
		if (v12)                                  // if (n)
		{
			if (p == (char *)v4[11])                // if (p == &ls->sb)   // String buffer pointer.
			{
				if (len > v4[12] - (signed int)p)     // if ( len > sbufsz(&ls->sb) )
				{
					v19 = (int)lj_buf_need2((SBuf*)v26, len);
					v6 = (char *)v4[6];
					p = (char *)v19;
				}
				v15 = (int)p;                           // p
				v16 = n;                                // n
				if (n < 8)                            // if (n < 8)
					goto LABEL_42;
				if ((unsigned __int8)p & 1)
				{
					v20 = *v6;
					v15 = (int)(p + 1);
					++v6;
					*p = v20;
					v16 = n - 1;
				}
				if (v15 & 2)
				{
					v21 = *(_WORD *)v6;
					v15 += 2;
					v6 += 2;
					v16 -= 2;
					*(_WORD *)(v15 - 2) = v21;
				}
				if (v15 & 4)
				{
					v17 = (_DWORD *)(v15 + 4);
					*(v17 - 1) = *(_DWORD *)v6;
					qmemcpy(v17, v6 + 4, v16 - 4);        // qmemcpy 逐字节复制，要单独实现
				}
				else
				{
				LABEL_42:
					qmemcpy((void *)v15, v6, v16);
				}
			}
			else if (p != v6)
			{
				v14 = (char *)v4[13];                   // ls->sb->b
				memmove(p, v6, v12);
				p = v14;
			}
			v4[6] = (_DWORD)p;                                // ls->p = p
			v13 = (int)&p[n];                         // v13 = p + n
			v4[7] = (_DWORD)&p[n];                            // ls->pe = p + n
		}
		v4[11] = v13;                               // ls->sb = v13
		buf = (char *)((int(*)(_DWORD, _DWORD, unsigned int *))v4[15])(v4[1], v4[16], &sz);// buf = ls->rfunc(ls->L, ls->rdata, &sz);    //  Reader callback
		v6 = buf;
		if (!buf)
			break;
		v18 = sz;
		if (!sz)
			break;
		if (n)
		{
			v7 = len;
			if (sz + n >= len)
				v7 = sz + n;
			v25 = sz + n;
			v8 = v4[13];                              // ls->sb->b
			if (v7 > v4[12] - v8)                   // if (v7 > sbufsz(&ls->sb))
			{
				v8 = (int)lj_buf_need2((SBuf*)v26, v7);
				v18 = sz;
			}
			v9 = (_WORD *)v4[11];                     // ls->sb
			v22 = v6;
			if (v18 < 8)
				goto LABEL_9;
			if ((unsigned __int8)v9 & 1)
			{
				v9 = (_WORD *)((char *)v9 + 1);
				--v18;
				*((_BYTE *)v9 - 1) = *v6;
				v22 = v6 + 1;
				if (!((unsigned __int8)v9 & 2))
					goto LABEL_22;
			}
			else if (!((unsigned __int8)v9 & 2))
			{
				goto LABEL_22;
			}
			++v9;
			v18 -= 2;
			*(v9 - 1) = *(_WORD *)v22;
			v22 += 2;
		LABEL_22:
			if ((unsigned __int8)v9 & 4)
			{
				v9 += 2;
				v18 -= 4;
				*((_DWORD *)v9 - 1) = *(_DWORD *)v22;
				v22 += 4;
			}
		LABEL_9:
			qmemcpy(v9, v22, v18);
			v6 = (char *)v8;
			v4[6] = v8;                               // ls->p = v8
			v5 = (char *)(v8 + v25);                  // ls->sb->b + sz + n
			v4[11] = v8 + v25;                        // ls->sb = v8 + v25
			v4[7] = v8 + v25;                         // ls->pe = v8 + v25
			buf = (char *)(v8 + len);
			if (v5 >= buf)
				return;
		}
		else
		{
			v5 = &buf[sz];
			v4[6] = (uint32_t)buf;
			v4[7] = (uint32_t)v5;
			buf += len;
			if (v5 >= &v6[len])
				return;
		}
	}
	if (a4)
		LABEL_41:
	bcread_error_mod(v4[1], (const char *)v4[20], 2972);
	v4[8] = -1;                                   // ls->c = -1
	return;
}


/* Need a certain number of bytes. */
static LJ_AINLINE void bcread_need(LexState *ls, MSize len)
{
	if (LJ_UNLIKELY(ls->p + len > ls->pe))
		bcread_fill(ls, len, 1);
}

/* Want to read up to a certain number of bytes, but may need less. */
static LJ_AINLINE void bcread_want(LexState *ls, MSize len)
{
	if (LJ_UNLIKELY(ls->p + len > ls->pe))
		bcread_fill(ls, len, 0);
}

/* Return memory block from buffer. */
static LJ_AINLINE uint8_t *bcread_mem(LexState *ls, MSize len)
{
	uint8_t *p = (uint8_t *)ls->p;
	ls->p += len;
	lua_assert(ls->p <= ls->pe);
	return p;
}

/* Copy memory block from buffer. */
static void bcread_block(LexState *ls, void *q, MSize len)
{
	memcpy(q, bcread_mem(ls, len), len);
}

/* Read byte from buffer. */
static LJ_AINLINE uint32_t bcread_byte(LexState *ls)
{
	lua_assert(ls->p < ls->pe);
	return (uint32_t)(uint8_t)*ls->p++;
}

/* Read ULEB128 value from buffer. */
static LJ_AINLINE uint32_t bcread_uleb128(LexState *ls)
{
	uint32_t v = lj_buf_ruleb128(&ls->p);
	lua_assert(ls->p <= ls->pe);
	return v;
}

/* Read top 32 bits of 33 bit ULEB128 value from buffer. */
static uint32_t bcread_uleb128_33(LexState *ls)
{
	const uint8_t *p = (const uint8_t *)ls->p;
	uint32_t v = (*p++ >> 1);
	if (LJ_UNLIKELY(v >= 0x40)) {
		int sh = -1;
		v &= 0x3f;
		do {
			v |= ((*p & 0x7f) << (sh += 7));
		} while (*p++ >= 0x80);
	}
	ls->p = (char *)p;
	lua_assert(ls->p <= ls->pe);
	return v;
}

/* -- Bytecode reader ----------------------------------------------------- */

/* Read debug info of a prototype. */
static void bcread_dbg(LexState *ls, GCproto *pt, MSize sizedbg)
{
	void *lineinfo = (void *)proto_lineinfo(pt);
	bcread_block(ls, lineinfo, sizedbg);
	/* Swap lineinfo if the endianess differs. */
	if (bcread_swap(ls) && pt->numline >= 256) {
		MSize i, n = pt->sizebc - 1;
		if (pt->numline < 65536) {
			uint16_t *p = (uint16_t *)lineinfo;
			for (i = 0; i < n; i++) p[i] = (uint16_t)((p[i] >> 8) | (p[i] << 8));
		}
		else {
			uint32_t *p = (uint32_t *)lineinfo;
			for (i = 0; i < n; i++) p[i] = lj_bswap(p[i]);
		}
	}
}


void bcread_dbg_mod (unsigned int ls, int pt, unsigned int sizedbg)
{
	unsigned int v3; // ecx
	char *v4; // esi
	vec4i *v5; // ebp
	int v6; // edi
	int v7; // edx
	unsigned int v8; // ecx
	unsigned int v9; // esi
	const vec4i *v10; // edx
	unsigned int v11; // edi
	vec4i v12; // xmm4
	vec4i v13; // xmm3
	vec4i v14; // xmm0
	int v15; // edx
	_WORD *v16; // edx
	int v17; // edx
	int v18; // edx
	unsigned int v19; // eax
	char v20; // dl
	__int16 v21; // dx
	int v22; // [esp-20h] [ebp-20h]

	vec4i xmmword_9B0B0;
    xmmword_9B0B0.m128i = _mm_set_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x0D, 0x0C, 0x09, 0x08, 0x05, 0x04, 0x01, 0x00);
	vec4i xmmword_9B0C0;
    xmmword_9B0C0.m128i = _mm_set_epi8(0x0D, 0x0C, 0x09, 0x08, 0x05, 0x04, 0x01, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);

	v3 = sizedbg;
	v4 = *(char **)(ls + 24);
	v5 = *(vec4i **)(pt + 52);
	*(char **)(ls + 24) = &v4[sizedbg];
	v6 = (int)v5;
	v22 = pt;
	if (sizedbg >= 8)
	{
		if ((unsigned __int8)v5 & 1)
		{
			v20 = *v4;
			v6 = (int)v5->m128i_i32 + 1;
			++v4;
			v3 = sizedbg - 1;
			LOBYTE(v5->m128i_i32[0]) = v20;
		}
		if (v6 & 2)
		{
			v21 = *(_WORD *)v4;
			v6 += 2;
			v4 += 2;
			v3 -= 2;
			*(_WORD *)(v6 - 2) = v21;
		}
		if (v6 & 4)
		{
			v17 = *(_DWORD *)v4;
			v6 += 4;
			v4 += 4;
			v3 -= 4;
			*(_DWORD *)(v6 - 4) = v17;
		}
	}
	qmemcpy((void *)v6, v4, v3);
	if (*(_BYTE *)(ls + 108) & 1)
	{
		ls = *(_DWORD *)(v22 + 48);
		if (ls > 255)
		{
			v7 = *(_DWORD *)(v22 + 8);
			v8 = v7 - 1;
			if (ls > 0xFFFF)
			{
				v18 = (int)&v5->m128i_i32[v7 - 1];
				if (v8)
				{
					do
					{
						v19 = v5->m128i_i32[0];
						v5 = (vec4i *)((char *)v5 + 4);
						ls = _byteswap_ulong(v19);
						v5[-1].m128i_i32[3] = ls;
					} while (v5 != (vec4i *)v18);
				}
			}
			else
			{
				if (v7 == 1)
					return;
				v9 = ((unsigned int)(v7 - 9) >> 3) + 1;
				ls = 8 * v9;
				if ((unsigned int)(v7 - 2) <= 6)
				{
					ls = 0;
				}
				else
				{
					v10 = v5;
					v11 = 0;
					v12 = xmmword_9B0B0;
					v13 = xmmword_9B0C0;
					do
					{
						v14.m128i = _mm_loadu_si128(&(v10->m128i));
						++v11;
						++v10;
						_mm_storeu_si128(
							(vec4i *)&v10[-1],
							_mm_or_si128(
								_mm_srli_epi16(v14.m128i, 8u),
								_mm_or_si128(
									_mm_shuffle_epi8(_mm_slli_epi32(_mm_unpackhi_epi16(v14.m128i, _mm_setzero_si128()), 8u), v13.m128i),
									_mm_shuffle_epi8(_mm_slli_epi32(_mm_unpacklo_epi16(v14.m128i, _mm_setzero_si128()), 8u), v12.m128i))));
					} while (v9 > v11);
					if (v8 == ls)
						return;
				}
				*((_WORD *)v5->m128i_i32 + ls) = (*((_WORD *)v5->m128i_i32 + ls) << 8) | (*((_WORD *)v5->m128i_i32 + ls) >> 8);
				if (v8 > ls + 1)
				{
					*((_WORD *)v5->m128i_i32 + ls + 1) = (*((_WORD *)v5->m128i_i32 + ls + 1) << 8) | (*((_WORD *)v5->m128i_i32
						+ ls
						+ 1) >> 8);
					if (v8 > ls + 2)
					{
						*((_WORD *)v5->m128i_i32 + ls + 2) = (*((_WORD *)v5->m128i_i32 + ls + 2) << 8) | (*((_WORD *)v5->m128i_i32
							+ ls
							+ 2) >> 8);
						if (v8 > ls + 3)
						{
							*((_WORD *)v5->m128i_i32 + ls + 3) = (*((_WORD *)v5->m128i_i32 + ls + 3) << 8) | (*((_WORD *)v5->m128i_i32
								+ ls
								+ 3) >> 8);
							if (v8 > ls + 4)
							{
								*((_WORD *)v5->m128i_i32 + ls + 4) = (*((_WORD *)v5->m128i_i32 + ls + 4) << 8) | (*((_WORD *)v5->m128i_i32
									+ ls
									+ 4) >> 8);
								v15 = ls + 5;
								if (v8 > ls + 5)
								{
									ls += 6;
									*((_WORD *)v5->m128i_i32 + v15) = (*((_WORD *)v5->m128i_i32 + v15) << 8) | (*((_WORD *)v5->m128i_i32
										+ v15) >> 8);
									if (v8 > ls)
									{
										v16 = (_WORD *)((char *)v5->m128i_i32 + 2 * ls);
										ls = ((unsigned __int16)*v16 << 8) | ((unsigned int)(unsigned __int16)*v16 >> 8);
										*v16 = ls;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return;
}

/* Find pointer to varinfo. */
static const void *bcread_varinfo(GCproto *pt)
{
	const uint8_t *p = proto_uvinfo(pt);
	MSize n = pt->sizeuv;
	if (n) while (*p++ || --n);
	return p;
}

/* Read a single constant key/value of a template table. */
static void bcread_ktabk(LexState *ls, TValue *o)
{
	MSize tp = bcread_uleb128(ls);
	if (tp >= BCDUMP_KTAB_STR) {
		MSize len = tp - BCDUMP_KTAB_STR;
		const char *p = (const char *)bcread_mem(ls, len);
		setstrV(ls->L, o, lj_str_new(ls->L, p, len));
	}
	else if (tp == BCDUMP_KTAB_INT) {
		setintV(o, (int32_t)bcread_uleb128(ls));
	}
	else if (tp == BCDUMP_KTAB_NUM) {
		o->u32.lo = bcread_uleb128(ls);
		o->u32.hi = bcread_uleb128(ls);
	}
	else {
		lua_assert(tp <= BCDUMP_KTAB_TRUE);
		setpriV(o, ~tp);
	}
}

/* Read a template table. */
static GCtab *bcread_ktab(LexState *ls)
{
	MSize narray = bcread_uleb128(ls);
	MSize nhash = bcread_uleb128(ls);
	GCtab *t = lj_tab_new(ls->L, narray, hsize2hbits(nhash));
	if (narray) {  /* Read array entries. */
		MSize i;
		TValue *o = tvref(t->array);
		for (i = 0; i < narray; i++, o++)
			bcread_ktabk(ls, o);
	}
	if (nhash) {  /* Read hash entries. */
		MSize i;
		for (i = 0; i < nhash; i++) {
			TValue key;
			bcread_ktabk(ls, &key);
			lua_assert(!tvisnil(&key));
			bcread_ktabk(ls, lj_tab_set(ls->L, t, &key));
		}
	}
	return t;
}

/* Read GC constants of a prototype. */
static void bcread_kgc(LexState *ls, GCproto *pt, MSize sizekgc)
{
	MSize i;
	GCRef *kr = mref(pt->k, GCRef) - (ptrdiff_t)sizekgc;
	for (i = 0; i < sizekgc; i++, kr++) {
		MSize tp = bcread_uleb128(ls);
		if (tp >= BCDUMP_KGC_STR)
		{
			MSize len = tp - BCDUMP_KGC_STR;
			const char *p = (const char *)bcread_mem(ls, len);
			setgcref(*kr, obj2gco(lj_str_new(ls->L, p, len)));
		}
		else if (tp == BCDUMP_KGC_TAB)
		{
			setgcref(*kr, obj2gco(bcread_ktab(ls)));
#if LJ_HASFFI
		}
		else if (tp != BCDUMP_KGC_CHILD)
		{
			CTypeID id = tp == BCDUMP_KGC_COMPLEX ? CTID_COMPLEX_DOUBLE :
				tp == BCDUMP_KGC_I64 ? CTID_INT64 : CTID_UINT64;
			CTSize sz = tp == BCDUMP_KGC_COMPLEX ? 16 : 8;
			GCcdata *cd = lj_cdata_new_(ls->L, id, sz);
			TValue *p = (TValue *)cdataptr(cd);
			setgcref(*kr, obj2gco(cd));
			p[0].u32.lo = bcread_uleb128(ls);
			p[0].u32.hi = bcread_uleb128(ls);
			if (tp == BCDUMP_KGC_COMPLEX)
			{
				p[1].u32.lo = bcread_uleb128(ls);
				p[1].u32.hi = bcread_uleb128(ls);
			}
#endif
		}
		else
		{
			lua_State *L = ls->L;
			lua_assert(tp == BCDUMP_KGC_CHILD);
			if (L->top <= bcread_oldtop(L, ls))  /* Stack underflow? */
				bcread_error(ls, LJ_ERR_BCBAD);
			L->top--;
			setgcref(*kr, obj2gco(protoV(L->top)));
		}
	}
}

static vec4i xmmword_9B270;
static vec4i xmmword_9B290;
static vec4i xmmword_9B0C0;
static vec4i xmmword_9B0B0;
static vec4i xmmword_9B0D0;
static vec4i xmmword_9B030;
static vec4i xmmword_9B2A0;
static vec4i xmmword_9B280;
static vec4i xmmword_9B2B0;

int bcread_kgc_mod(int ls, int pt, int sizekgc)
{
	int ls_; // ebp
	unsigned int tp; // eax
	char *v5; // esi
	size_t len; // ecx
	unsigned int v7; // edi
	vec4i v8; // xmm2
	unsigned int v9; // eax
	unsigned int v10; // ecx
	vec4i v11; // xmm0
	vec4i v12; // xmm1
	unsigned int v13; // edx
	__int16 v14; // si
	int v15; // edi
	int L; // eax
	unsigned int top; // edx
	int v18; // eax
	unsigned int v19; // edx
	unsigned int nhash; // eax
	unsigned int v21; // eax
	unsigned int v22; // edi
	double *t_array; // esi
	double *o; // edx
	unsigned int v25; // edi
	double *v26; // eax
	int v27; // esi
	//int v28; // [esp-5Ch] [ebp-5Ch]
	//int v29; // [esp-58h] [ebp-58h]
	//int v30; // [esp-54h] [ebp-54h]
	//int v31; // [esp-50h] [ebp-50h]
	//int v32; // [esp-4Ch] [ebp-4Ch]
	int v33; // [esp-44h] [ebp-44h]
	char **v34; // [esp-40h] [ebp-40h]
	int v35; // [esp-3Ch] [ebp-3Ch]
	_DWORD *t; // [esp-38h] [ebp-38h]
	size_t v37; // [esp-34h] [ebp-34h]
	unsigned int nhash_; // [esp-34h] [ebp-34h]
	unsigned int narray; // [esp-30h] [ebp-30h]
	uint64_t v40; // [esp-24h] [ebp-24h]


	 xmmword_9B270.m128i = _mm_set_epi8(0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00);
	 xmmword_9B290.m128i = _mm_set_epi8(0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04);
	 xmmword_9B0C0.m128i = _mm_set_epi8(0x0D, 0x0C, 0x09, 0x08, 0x05, 0x04, 0x01, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	 xmmword_9B0B0.m128i = _mm_set_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x0D, 0x0C, 0x09, 0x08, 0x05, 0x04, 0x01, 0x00);
	 xmmword_9B0D0.m128i = _mm_set_epi8(0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF);
	 xmmword_9B030.m128i = _mm_set_epi8(0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x0C);
	 xmmword_9B2A0.m128i = _mm_set_epi8(0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08);
	 xmmword_9B280.m128i = _mm_set_epi8(0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10);
	 xmmword_9B2B0.m128i = _mm_set_epi8(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);

	v35 = pt - 4 * sizekgc;
	v34 = (char **)(ls + 24);
	v33 = 0;
	if (sizekgc)
	{
		ls_ = ls;
		while (1)
		{
			while (1)
			{
				tp = lj_buf_ruleb128(v34);
				if (tp <= 4)
					break;
				v5 = *(char **)(ls_ + 24);
				len = tp - 5;
				*(char **)(ls_ + 24) = &v5[tp - 5];
				if (tp != 5)
				{
					v7 = ((tp - 21) >> 4) + 1;
					if (tp - 6 <= 0xE)
					{
						v13 = 0;
					LABEL_20:
						v5[v13] = ~(v5[v13] ^ v13);
						if (len > v13 + 1)
						{
							v5[v13 + 1] = ~(v5[v13 + 1] ^ (v13 + 1));
							if (len > v13 + 2)
							{
								v5[v13 + 2] = ~(v5[v13 + 2] ^ (v13 + 2));
								if (len > v13 + 3)
								{
									v5[v13 + 3] = ~(v5[v13 + 3] ^ (v13 + 3));
									if (len > v13 + 4)
									{
										v5[v13 + 4] = ~(v5[v13 + 4] ^ (v13 + 4));
										if (len > v13 + 5)
										{
											v5[v13 + 5] = ~(v5[v13 + 5] ^ (v13 + 5));
											if (len > v13 + 6)
											{
												v5[v13 + 6] = ~(v5[v13 + 6] ^ (v13 + 6));
												if (len > v13 + 7)
												{
													v5[v13 + 7] = ~(v5[v13 + 7] ^ (v13 + 7));
													if (len > v13 + 8)
													{
														v5[v13 + 8] = ~(v5[v13 + 8] ^ (v13 + 8));
														if (len > v13 + 9)
														{
															v5[v13 + 9] = ~(v5[v13 + 9] ^ (v13 + 9));
															if (len > v13 + 10)
															{
																v5[v13 + 10] = ~(v5[v13 + 10] ^ (v13 + 10));
																if (len > v13 + 11)
																{
																	v5[v13 + 11] = ~(v5[v13 + 11] ^ (v13 + 11));
																	if (len > v13 + 12)
																	{
																		v5[v13 + 12] = ~(v5[v13 + 12] ^ (v13 + 12));
																		v18 = v13 + 13;
																		if (len > v13 + 13)
																		{
																			v19 = v13 + 14;
																			v5[v18] = ~(v5[v18] ^ v18);
																			if (len > v19)
																				v5[v19] = ~(v5[v19] ^ v19);
																		}
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
						goto LABEL_9;
					}
					v8 = xmmword_9B270;
					v9 = 0;
					v37 = len;
					do
					{
						v10 = v9++;
						v11.m128i = _mm_and_si128(
							_mm_or_si128(
								_mm_shuffle_epi8(
									_mm_add_epi32(xmmword_9B290.m128i, v8.m128i), xmmword_9B0C0.m128i),
								_mm_shuffle_epi8(v8.m128i, xmmword_9B0B0.m128i)),
							xmmword_9B0D0.m128i);
						v12.m128i = _mm_or_si128(
							_mm_shuffle_epi8(
								_mm_add_epi32(xmmword_9B030.m128i, v8.m128i), xmmword_9B0C0.m128i),
							_mm_shuffle_epi8(
								_mm_add_epi32(xmmword_9B2A0.m128i, v8.m128i), xmmword_9B0B0.m128i));
						v8.m128i = _mm_add_epi32(v8.m128i, xmmword_9B280.m128i);
						_mm_storeu_si128(
							(vec4i *)&v5[16 * v10],
							_mm_xor_si128(
								_mm_xor_si128(
									_mm_packus_epi16(v11.m128i, _mm_and_si128(v12.m128i, xmmword_9B0D0.m128i)),
									_mm_loadu_si128((const vec4i *)&v5[16 * v10])), xmmword_9B2B0.m128i));
					} while (v7 > v9);
					len = v37;
					v13 = 16 * v7;
					if (v37 != 16 * v7)
						goto LABEL_20;
				}
			LABEL_9:
				*(_DWORD *)(v35 + 4 * v33++) = lj_str_new_mod(*(_DWORD *)(ls_ + 4), v5, len);
				ls = v33;
				if (v33 == sizekgc)
					return ls;
			}
			if (tp != 1)
			{
				if (tp)
				{
					if (tp == 4)
					{
						v27 = (int)lj_mem_newgco((lua_State*)(ls_ + 4), 24);
						*(_BYTE *)(v27 + 5) = 10;
						*(_WORD *)(v27 + 6) = 16;
						*(_DWORD *)(v35 + 4 * v33) = v27;
						*(_DWORD *)(v27 + 8) = lj_buf_ruleb128(v34);
						*(_DWORD *)(v27 + 12) = lj_buf_ruleb128(v34);
						*(_DWORD *)(v27 + 16) = lj_buf_ruleb128(v34);
						*(_DWORD *)(v27 + 20) = lj_buf_ruleb128(v34);
					}
					else
					{
						v14 = (tp != 2) + 11;
						v15 = (int)lj_mem_newgco((lua_State*)(ls_ + 4), 16);
						*(_WORD *)(v15 + 6) = v14;
						*(_BYTE *)(v15 + 5) = 10;
						*(_DWORD *)(v35 + 4 * v33) = v15;
						*(_DWORD *)(v15 + 8) = lj_buf_ruleb128(v34);
						*(_DWORD *)(v15 + 12) = lj_buf_ruleb128(v34);
					}
				}
				else
				{
					L = *(_DWORD *)(ls_ + 4);
					top = *(_DWORD *)(L + 20);
					if (top <= *(_DWORD *)(ls_ + 72) + *(_DWORD *)(L + 28))
						bcread_error_mod(L, *(const char **)(ls_ + 80), 2972);
					*(_DWORD *)(L + 20) = top - 8;
					*(_DWORD *)(v35 + 4 * v33) = *(_DWORD *)(top - 8);
				}
				goto LABEL_15;
			}
			narray = lj_buf_ruleb128(v34);            // bcread_ktab()  begin
			nhash = lj_buf_ruleb128(v34);
			nhash_ = nhash;
			if (!nhash)
				break;
			if (nhash == 1)
			{
				v21 = 1;
			}
			else
			{
				_BitScanReverse(&nhash, nhash - 1);
				v21 = nhash + 1;
			}
			t = (_DWORD *)lj_tab_new(*(lua_State**)(ls_ + 4), narray, v21);// GCtab *t
			if (narray)
				goto LABEL_39;
		LABEL_42:                                       // /* Read hash entries. */
			v25 = 0;
			do
			{
				bcread_ktabk((LexState*)ls_, (TValue *)&v40);
				++v25;
				v26 = (double *)lj_tab_set((lua_State*)(ls_ + 4), (GCtab*)t, (cTValue*)&v40);
				bcread_ktabk((LexState*)ls_, (TValue*)v26);
			} while (nhash_ > v25);
		LABEL_44:
			*(_DWORD *)(v35 + 4 * v33) = (_DWORD)t;
		LABEL_15:
			ls = ++v33;
			if (v33 == sizekgc)
				return ls;
		}
		t = (uint32_t*)lj_tab_new(*(lua_State**)(ls_ + 4), narray, 0);
		if (!narray)
		{
			*(_DWORD *)(v35 + 4 * v33) = (_DWORD)t;
			goto LABEL_15;
		}
	LABEL_39:                                       // /* Read array entries. */
		v22 = 0;
		t_array = (double *)t[2];
		do
		{
			o = t_array;
			++v22;
			++t_array;
			bcread_ktabk((LexState*)ls_, (TValue*)o);
		} while (narray > v22);
		if (!nhash_)
			goto LABEL_44;
		goto LABEL_42;
	}
	return ls;
}


/* Read number constants of a prototype. */
static void bcread_knum(LexState *ls, GCproto *pt, MSize sizekn)
{
	MSize i;
	TValue *o = mref(pt->k, TValue);
	for (i = 0; i < sizekn; i++, o++) {
		int isnum = (ls->p[0] & 1);
		uint32_t lo = bcread_uleb128_33(ls);
		if (isnum) {
			o->u32.lo = lo;
			o->u32.hi = bcread_uleb128(ls);
		}
		else {
			setintV(o, lo);
		}
	}
}

unsigned int bcread_knum_mod(int ls, unsigned int pt , unsigned int sizekn)
{
	int v3; // esi
	unsigned int result; // eax
	int i; // edi
	unsigned __int8 *v6; // eax
	unsigned __int8 v7; // cl
	int v8; // eax
	unsigned __int8 v9; // dl
	unsigned __int8 v10; // cl
	signed int lo; // edx
	int v12; // ebp
	char *v13; // esi
	int v14; // edx
	signed int v15; // ecx
	int v16; // edi
	char v17; // al
	char v18; // [esp-19h] [ebp-19h]
	unsigned int v19; // [esp-18h] [ebp-18h]
	char **v20; // [esp-14h] [ebp-14h]
	int v21; // [esp-10h] [ebp-10h]

	v3 = ls;
	v20 = (char **)(ls + 24);
	result = sizekn;
	v19 = pt;
	if (sizekn)
	{
		i = 0;
		do
		{
			while (1)
			{
				v6 = *(unsigned __int8 **)(v3 + 24);
				v7 = *v6;
				v8 = (int)(v6 + 1);
				v9 = v7;
				v10 = v7 >> 1;
				v18 = v9 & 1;
				lo = v10;
				if (v10 > 0x3Fu)
				{
					v12 = v3;
					v13 = (char *)v8;
					v14 = v10 & 0x3F;
					v15 = -1;
					v21 = i;
					v16 = v14;
					do
					{
						v17 = *v13;
						v15 += 7;
						++v13;
						v16 |= (v17 & 0x7F) << v15;
					} while (v17 < 0);
					v8 = (int)v13;
					lo = v16;
					i = v21;
					v3 = v12;
				}
				*(_DWORD *)(v3 + 24) = v8;
				if (v18)
					break;                                // break while ( 1 )
				result = v19;
				*(double *)(v19 + 8 * i++) = (double)lo;// setintV(o, lo);
				if (i == sizekn)
					return result;
			}                                         // end of while ( 1 )
			*(_DWORD *)(v19 + 8 * i) = lo;
			result = lj_buf_ruleb128(v20);
			*(_DWORD *)(v19 + 8 * i++ + 4) = result;
		} while (i != sizekn);
	}
	return result;
}


/* Read bytecode instructions. */
static void bcread_bytecode(LexState *ls, GCproto *pt, MSize sizebc)
{
	BCIns *bc = proto_bc(pt);
	bc[0] = BCINS_AD((pt->flags & PROTO_VARARG) ? BC_FUNCV : BC_FUNCF,
		pt->framesize, 0);
	bcread_block(ls, bc + 1, (sizebc - 1)*(MSize)sizeof(BCIns));
	/* Swap bytecode instructions if the endianess differs. */
	if (bcread_swap(ls)) {
		MSize i;
		for (i = 1; i < sizebc; i++) bc[i] = lj_bswap(bc[i]);
	}
}

/* Read upvalue refs. */
static void bcread_uv(LexState *ls, GCproto *pt, MSize sizeuv)
{
	if (sizeuv) {
		uint16_t *uv = proto_uv(pt);
		bcread_block(ls, uv, sizeuv * 2);
		/* Swap upvalue refs if the endianess differs. */
		if (bcread_swap(ls)) {
			MSize i;
			for (i = 0; i < sizeuv; i++)
				uv[i] = (uint16_t)((uv[i] >> 8) | (uv[i] << 8));
		}
	}
}

void bcread_uv_mod(unsigned int ls, vec4i **pt, unsigned int sizeuv)
{
	char *v3; // esi
	vec4i *v4; // edx
	int v5; // edi
	unsigned int v6; // ecx
	const vec4i *v7; // esi
	unsigned int v8; // edi
	vec4i v9; // xmm4
	vec4i v10; // xmm3
	vec4i v11; // xmm0
	_WORD *v12; // esi
	_WORD *v13; // edx
	int v14; // ecx
	char v15; // cl
	__int16 v16; // cx
	unsigned int v17; // [esp-20h] [ebp-20h]

	v3 = *(char **)(ls + 24);
	v4 = *pt;
	*(char**)(ls + 24) = &v3[2 * sizeuv];		//	ls->p =   ls->p += len;
	v5 = (int)v4;
	v17 = 2 * sizeuv;
	if (2 * sizeuv >= 8)
	{
		if ((unsigned __int8)v4 & 1)
		{
			v15 = *v3;
			v5 = (int)v4->m128i_i32 + 1;
			++v3;
			LOBYTE(v4->m128i_i32[0]) = v15;
			--v17;
		}
		if (v5 & 2)
		{
			v16 = *(_WORD *)v3;
			v5 += 2;
			v3 += 2;
			*(_WORD *)(v5 - 2) = v16;
			v17 -= 2;
		}
		if (v5 & 4)
		{
			v14 = *(_DWORD *)v3;
			v5 += 4;
			v3 += 4;
			*(_DWORD *)(v5 - 4) = v14;
			v17 -= 4;
		}
	}
	qmemcpy((void *)v5, v3, v17);
	if (*(_BYTE *)(ls + 108) & 1 && sizeuv)
	{
		v6 = ((sizeuv - 8) >> 3) + 1;
		ls = 8 * v6;
		if (sizeuv - 1 <= 6)
		{
			ls = 0;
		}
		else
		{
			v7 = v4;
			v8 = 0;
			v9.m128i = _mm_set_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x0D, 0x0C, 0x09, 0x08, 0x05, 0x04, 0x01, 0x00);
			v10.m128i = _mm_set_epi8(0x0D, 0x0C, 0x09, 0x08, 0x05, 0x04, 0x01, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
			do
			{
				v11.m128i = _mm_loadu_si128(&(v7->m128i));
				++v8;
				++v7;
				_mm_storeu_si128(
					(vec4i *)&v7[-1],
					_mm_or_si128(
						_mm_srli_epi16(v11.m128i, 8u),
						_mm_or_si128(
							_mm_shuffle_epi8(_mm_slli_epi32(_mm_unpackhi_epi16(v11.m128i, _mm_setzero_si128()), 8u), v10.m128i),
							_mm_shuffle_epi8(_mm_slli_epi32(_mm_unpacklo_epi16(v11.m128i, _mm_setzero_si128()), 8u), v9.m128i))));
			} while (v6 > v8);
			if (sizeuv == ls)
				return;
		}
		*((_WORD *)v4->m128i_i32 + ls) = (*((_WORD *)v4->m128i_i32 + ls) << 8) | (*((_WORD *)v4->m128i_i32 + ls) >> 8);
		if (sizeuv > ls + 1)
		{
			*((_WORD *)v4->m128i_i32 + ls + 1) = (*((_WORD *)v4->m128i_i32 + ls + 1) << 8) | (*((_WORD *)v4->m128i_i32 + ls + 1) >> 8);
			if (sizeuv > ls + 2)
			{
				*((_WORD *)v4->m128i_i32 + ls + 2) = (*((_WORD *)v4->m128i_i32 + ls + 2) << 8) | (*((_WORD *)v4->m128i_i32
					+ ls
					+ 2) >> 8);
				if (sizeuv > ls + 3)
				{
					*((_WORD *)v4->m128i_i32 + ls + 3) = (*((_WORD *)v4->m128i_i32 + ls + 3) << 8) | (*((_WORD *)v4->m128i_i32
						+ ls
						+ 3) >> 8);
					if (sizeuv > ls + 4)
					{
						*((_WORD *)v4->m128i_i32 + ls + 4) = (*((_WORD *)v4->m128i_i32 + ls + 4) << 8) | (*((_WORD *)v4->m128i_i32
							+ ls
							+ 4) >> 8);
						if (sizeuv > ls + 5)
						{
							v12 = (_WORD *)((char *)v4->m128i_i32 + 2 * (ls + 5));
							ls += 6;
							*v12 = (*v12 << 8) | (*v12 >> 8);
							if (sizeuv > ls)
							{
								v13 = (_WORD *)((char *)v4->m128i_i32 + 2 * ls);
								ls = ((unsigned __int16)*v13 << 8) | ((unsigned int)(unsigned __int16)*v13 >> 8);
								*v13 = ls;
							}
						}
					}
				}
			}
		}
	}
	return;
}

/* Read a prototype. */
GCproto *lj_bcread_proto(LexState *ls)
{
	GCproto *pt;
	MSize framesize, numparams, flags, sizeuv, sizekgc, sizekn, sizebc, sizept;
	MSize ofsk, ofsuv, ofsdbg;
	MSize sizedbg = 0;
	BCLine firstline = 0, numline = 0;

	/* Read prototype header. */
	flags = bcread_byte(ls);
	numparams = bcread_byte(ls);
	framesize = bcread_byte(ls);
	sizeuv = bcread_byte(ls);
	sizekgc = bcread_uleb128(ls);
	sizekn = bcread_uleb128(ls);
	sizebc = bcread_uleb128(ls) + 1;
	if (!(bcread_flags(ls) & BCDUMP_F_STRIP)) {
		sizedbg = bcread_uleb128(ls);
		if (sizedbg) {
			firstline = bcread_uleb128(ls);
			numline = bcread_uleb128(ls);
		}
	}

	/* Calculate total size of prototype including all colocated arrays. */
	sizept = (MSize)sizeof(GCproto) +
		sizebc*(MSize)sizeof(BCIns) +
		sizekgc*(MSize)sizeof(GCRef);
	sizept = (sizept + (MSize)sizeof(TValue) - 1) & ~((MSize)sizeof(TValue) - 1);
	ofsk = sizept; sizept += sizekn*(MSize)sizeof(TValue);
	ofsuv = sizept; sizept += ((sizeuv + 1)&~1) * 2;
	ofsdbg = sizept; sizept += sizedbg;

	/* Allocate prototype object and initialize its fields. */
	pt = (GCproto *)lj_mem_newgco(ls->L, (MSize)sizept);
	pt->gct = ~LJ_TPROTO;
	pt->numparams = (uint8_t)numparams;
	pt->framesize = (uint8_t)framesize;
	pt->sizebc = sizebc;
	setmref(pt->k, (char *)pt + ofsk);
	setmref(pt->uv, (char *)pt + ofsuv);
	pt->sizekgc = 0;  /* Set to zero until fully initialized. */
	pt->sizekn = sizekn;
	pt->sizept = sizept;
	pt->sizeuv = (uint8_t)sizeuv;
	pt->flags = (uint8_t)flags;
	pt->trace = 0;
	setgcref(pt->chunkname, obj2gco(ls->chunkname));

	/* Close potentially uninitialized gap between bc and kgc. */
	*(uint32_t *)((char *)pt + ofsk - sizeof(GCRef)*(sizekgc + 1)) = 0;

	/* Read bytecode instructions and upvalue refs. */
	bcread_bytecode(ls, pt, sizebc);
	bcread_uv(ls, pt, sizeuv);

	/* Read constants. */
	bcread_kgc(ls, pt, sizekgc);
	pt->sizekgc = sizekgc;
	bcread_knum(ls, pt, sizekn);

	/* Read and initialize debug info. */
	pt->firstline = firstline;
	pt->numline = numline;
	if (sizedbg) {
		MSize sizeli = (sizebc - 1) << (numline < 256 ? 0 : numline < 65536 ? 1 : 2);
		setmref(pt->lineinfo, (char *)pt + ofsdbg);
		setmref(pt->uvinfo, (char *)pt + ofsdbg + sizeli);
		bcread_dbg(ls, pt, sizedbg);
		setmref(pt->varinfo, bcread_varinfo(pt));
	}
	else {
		setmref(pt->lineinfo, NULL);
		setmref(pt->uvinfo, NULL);
		setmref(pt->varinfo, NULL);
	}
	return pt;
}

/* Read and check header of bytecode dump. */
static int bcread_header(LexState *ls)
{
	uint32_t flags;
	bcread_want(ls, 3 + 5 + 5);
	if (bcread_byte(ls) != BCDUMP_HEAD2 ||
		bcread_byte(ls) != BCDUMP_HEAD3 ||
		bcread_byte(ls) != BCDUMP_VERSION) return 0;
	bcread_flags(ls) = flags = bcread_uleb128(ls);
	if ((flags & ~(BCDUMP_F_KNOWN)) != 0) return 0;
	if ((flags & BCDUMP_F_FR2) != LJ_FR2*BCDUMP_F_FR2) return 0;
	if ((flags & BCDUMP_F_FFI)) {
#if LJ_HASFFI
		lua_State *L = ls->L;
		if (!ctype_ctsG(G(L))) {
			ptrdiff_t oldtop = savestack(L, L->top);
			luaopen_ffi(L);  /* Load FFI library on-demand. */
			L->top = restorestack(L, oldtop);
	}
#else
		return 0;
#endif
}
	if ((flags & BCDUMP_F_STRIP)) {
		ls->chunkname = lj_str_newz(ls->L, ls->chunkarg);		//	新建字符串对象
	}
	else {
		MSize len = bcread_uleb128(ls);
		bcread_need(ls, len);
		ls->chunkname = lj_str_new(ls->L, (const char *)bcread_mem(ls, len), len);
	}
	return 1;  /* Ok. */
}

/* Read a bytecode dump. */
GCproto *lj_bcread(LexState *ls)
{
	lua_State *L = ls->L;
	lua_assert(ls->c == BCDUMP_HEAD1);
	bcread_savetop(L, ls, L->top);
	lj_buf_reset(&ls->sb);
	/* Check for a valid bytecode dump header. */
	if (!bcread_header(ls))
		bcread_error(ls, LJ_ERR_BCFMT);
	for (;;) {  /* Process all prototypes in the bytecode dump. */
		GCproto *pt;
		MSize len;
		const char *startp;
		/* Read length. */
		if (ls->p < ls->pe && ls->p[0] == 0) {  /* Shortcut EOF. */
			ls->p++;
			break;
		}
		bcread_want(ls, 5);
		len = bcread_uleb128(ls);
		if (!len) break;  /* EOF */
		bcread_need(ls, len);
		startp = ls->p;
		pt = lj_bcread_proto(ls);
		if (ls->p != startp + len)
			bcread_error(ls, LJ_ERR_BCBAD);
		setprotoV(L, L->top, pt);
		incr_top(L);
	}
	if ((int32_t)(2 * (uint32_t)(ls->pe - ls->p)) > 0 ||
		L->top - 1 != bcread_oldtop(L, ls))
		bcread_error(ls, LJ_ERR_BCBAD);
	/* Pop off last prototype. */
	L->top--;
	return protoV(L->top);
}


int lj_bcread_mod(int ls)
{
	int v1; // eax
	unsigned int v2; // ecx
	_BYTE *v3; // eax
	unsigned int h_flags; // ebp
	_DWORD *v5; // esi
	int v6; // edi
	unsigned int v7; // ecx
	unsigned int v8; // esi
	char *v9; // eax
	int v10; // edx
	int v11; // eax
	unsigned int v12; // ecx
	_BYTE *v13; // eax
	unsigned int v14; // edx
	unsigned int v15; // esi
	unsigned __int8 *v16; // eax
	unsigned __int8 v17; // cl
	int v18; // esi
	_BYTE *v19; // eax
	unsigned int ofsk; // esi
	int ofsuv; // ebp
	int sizept_; // edi
	int pt; // eax
	int pt__; // edi
	__int16 *v25; // edx
	int v26; // eax
	int v27; // edi
	unsigned int v28; // ecx
	__int16 *v29; // esi
	int v30; // esi
	int v31; // ebp
	int v32; // edi
	char v33; // ST18_1
	int v34; // ecx
	int v35; // ecx
	int v36; // ecx
	int v37; // ecx
	int v38; // ecx
	int v39; // ecx
	int v40; // ecx
	int v41; // ecx
	char v42; // cl
	int v43; // edx
	int v44; // eax
	int v45; // eax
	int *v46; // eax
	unsigned int v47; // eax
	bool v48; // cf
	int v49; // edx
	int v50; // eax
	unsigned int v51; // ecx
	int v52; // edx
	__int16 v53; // dx
	int v54; // edx
	int v56; // eax
	char *v57; // esi
	size_t v58; // ST08_4
	//int v59; // [esp-8Ch] [ebp-8Ch]
	//int v60; // [esp-88h] [ebp-88h]
	//int v61; // [esp-84h] [ebp-84h]
	//int v62; // [esp-80h] [ebp-80h]
	//int v63; // [esp-7Ch] [ebp-7Ch]
	unsigned int sizebc; // [esp-78h] [ebp-78h]
	char v65; // [esp-78h] [ebp-78h]
	unsigned __int8 framesize; // [esp-74h] [ebp-74h]
	unsigned int v67; // [esp-70h] [ebp-70h]
	int pt_; // [esp-6Ch] [ebp-6Ch]
	unsigned int sizekn; // [esp-68h] [ebp-68h]
	int sizekgc; // [esp-64h] [ebp-64h]
	unsigned int sizedbg; // [esp-60h] [ebp-60h]
	char **v72; // [esp-5Ch] [ebp-5Ch]
	signed int v73; // [esp-58h] [ebp-58h]
	int v74; // [esp-50h] [ebp-50h]
	char numparams; // [esp-4Bh] [ebp-4Bh]
	unsigned __int8 sizeuv; // [esp-4Ah] [ebp-4Ah]
	char flags; // [esp-49h] [ebp-49h]
	unsigned int v78; // [esp-48h] [ebp-48h]
	int v79; // [esp-44h] [ebp-44h]
	_DWORD *L_; // [esp-40h] [ebp-40h]

	v1 = *(_DWORD *)(ls + 4);                     // lua_State *L = ls->L;
	v2 = *(_DWORD *)(v1 + 28);                    // MRef stack = L->stack
	L_ = *(_DWORD **)(ls + 4);                    // lua_State *L_ = ls->L;
	*(_DWORD *)(ls + 72) = *(_DWORD *)(v1 + 20) - v2;// bcread_savetop(L, ls, L->top);
	*(_DWORD *)(ls + 44) = *(_DWORD *)(ls + 52);  // lj_buf_reset(&ls->sb);
	v3 = *(_BYTE **)(ls + 24);                    // v3 = ls->p
	if (*(_DWORD *)(ls + 28) < (unsigned int)(v3 + 13))// if (((ls->pe) - (ls->p)) < 13)
	{
		bcread_fill_mod((_DWORD *)ls, 0xDu, v2, 0); // bcread_fill_mod(ls,13,0)  // 第三个参数v2多余
		v3 = *(_BYTE **)(ls + 24);
	}
	*(_DWORD *)(ls + 24) = (_DWORD)(v3 + 1);                // ls->p++     // 文件头结构开始----
	if (*v3 != 65                                // v3[0] != 'A'
		|| (*(_DWORD *)(ls + 24) = (_DWORD)(v3 + 2), v3[1] != 78)// ls->p++ , v3[1] != 'N'
		|| (*(_DWORD *)(ls + 24) = (_DWORD)(v3 + 3), v3[2] != 65)// ls->p++ , v3[2] != 'A'
		|| (v72 = (char **)(ls                      // v75 = (char**)&(ls->p),
			+ 24),
			h_flags = lj_buf_ruleb128((char **)(ls + 24)),// flags = lj_buf_ruleb128(ls->p),
			*(_DWORD *)(ls                          // bcread_flags(ls) = flags,
				+ 108) = h_flags,
			h_flags & 0xFFFFFFF8))
	{
		bcread_error_mod(*(_DWORD *)(ls + 4), *(const char **)(ls + 80), 2938);// bcread_error_mod(lua_State* L,const char *chunkarg, ErrMsg em)   // LJ_ERR_BCFMT
	}
	if (h_flags & 4)                            // 这里一般不会被执行
	{
		v5 = *(_DWORD **)(ls + 4);                  // lua_State *L = ls->L;
		if (!*(_DWORD *)(v5[2] + 232))            // ls->L->glref + 232   // 这里和原版有区别 if (!ctype_ctsG(G(L)))
		{
			v6 = v5[5] - v5[7];
			luaopen_ffi((lua_State*)v5);
			v5[5] = v5[7] + v6;
		}
	}
	if (h_flags & 2)                            // 这里才是重点
	{
		v57 = *(char **)(ls + 80);
		v58 = strlen(*(const char **)(ls + 80));
		v11 = lj_str_new_mod(*(_DWORD *)(ls + 4), v57, v58);// 新建字符串对象
	}
	else
	{
		v8 = lj_buf_ruleb128(v72);
		v9 = *(char **)(ls + 24);
		v10 = (int)&v9[v8];
		if ((unsigned int)&v9[v8] > *(_DWORD *)(ls + 28))
		{
			v7 = 0;
			bcread_fill_mod((_DWORD *)ls, v8, v7, 1);
			v9 = *(char **)(ls + 24);
			v10 = (int)&v9[v8];
		}
		*(_DWORD *)(ls + 24) = v10;
		v11 = lj_str_new_mod(*(_DWORD *)(ls + 4), v9, v8);
	}
	*(_DWORD *)(ls + 76) = v11;                   // ls->chunkname = v11   //  文件头结构结束----
	while (1)
	{
		v13 = *(_BYTE **)(ls + 24);                 // v13 = ls->p
		v14 = *(_DWORD *)(ls + 28);                 // v14 = ls->pe
		if ((unsigned int)v13 < v14 && !*v13)
			break;
		if (v14 < (unsigned int)(v13 + 5))        // bcread_want  len = 5
		{
			v12 = 0;
			bcread_fill_mod((_DWORD *)ls, 5u, v12, 0);// bcread_fill    (魔改版)
		}	
		v15 = lj_buf_ruleb128(v72);
		if (!v15)
		{
			if (2 * (*(_DWORD *)(ls + 28) - *(_DWORD *)(ls + 24)) <= 0)
				goto LABEL_57;
			goto LABEL_60;
		}
		v16 = *(unsigned __int8 **)(ls + 24);
		v74 = (int)&v16[v15];
		if ((unsigned int)&v16[v15] > *(_DWORD *)(ls + 28))
		{
			bcread_fill_mod((_DWORD *)ls, v15, ls, 1);
			v16 = *(unsigned __int8 **)(ls + 24);
			v74 = (int)&v16[v15];
		}
		*(_DWORD *)(ls + 24) = (_DWORD)(v16 + 1);             // /* Read prototype header. */
		v17 = *v16;
		*(_DWORD *)(ls + 24) = (_DWORD)(v16 + 2);
		v17 ^= 0xE9u;
		v18 = v17;
		framesize = v17;
		flags = v16[1] ^ v17;
		*(_DWORD *)(ls + 24) = (_DWORD)(v16 + 3);
		numparams = v16[2] ^ v17;
		v67 = lj_buf_ruleb128(v72) - v17;
		sizebc = v67 + 1;
		sizekn = lj_buf_ruleb128(v72) - v18;
		v19 = *(_BYTE **)(ls + 24);
		*(_DWORD *)(ls + 24) = (_DWORD)(v19 + 1);
		sizeuv = *v19 ^ framesize;
		sizekgc = lj_buf_ruleb128(v72) - v18;
		if (*(_BYTE *)(ls + 108) & 2)             // if (!(bcread_flags(ls) & BCDUMP_F_STRIP))
		{
			v73 = 0;
			v78 = 0;
			sizedbg = 0;
		}
		else
		{
			sizedbg = lj_buf_ruleb128(v72);
			if (sizedbg)
			{
				v78 = lj_buf_ruleb128(v72);
				v73 = lj_buf_ruleb128(v72);
			}
			else
			{
				v73 = 0;
				v78 = 0;
			}
		}
		ofsk = (4 * (sizebc + sizekgc + 16) + 7) & 0xFFFFFFF8;// sizept = (sizept + (MSize)sizeof(TValue)-1) & ~((MSize)sizeof(TValue)-1);
		ofsuv = ofsk + 8 * sizekn;
		sizept_ = ofsuv + ((2 * sizeuv + 2) & 0x3FC) + sizedbg;
		v79 = ofsuv + ((2 * sizeuv + 2) & 0x3FC);
		pt = (int)lj_mem_newgco(*(lua_State**)(ls + 4), sizept_);
		*(_BYTE *)(pt + 5) = 7;                     // pt->gct = ~LJ_TPROTO;
		pt_ = pt;
		*(_DWORD *)(pt + 28) = sizekn;              // pt->sizekn = sizekn
		*(_DWORD *)(pt + 32) = sizept_;             // pt->sizept = sizept
		pt__ = pt;
		*(_DWORD *)(pt__ + 20) = pt__ + ofsuv;      // setmref(pt->uv, (char *)pt + ofsuv);
		*(_BYTE *)(pt + 6) = numparams;             // pt->numparams = (uint8_t)numparams
		*(_DWORD *)(pt + 24) = 0;                   // pt->sizekgc = 0
		*(_BYTE *)(pt + 7) = framesize;             // pt->framesize = (uint8_t)framesize
		*(_DWORD *)(pt + 8) = sizebc;               // pt->sizebc = sizebc
		*(_DWORD *)(pt__ + 16) = pt__ + ofsk;       // setmref(pt->k, (char *)pt + ofsk);
		*(_WORD *)(pt + 38) = 0;                    // pt->trace = 0
		*(_BYTE *)(pt + 36) = sizeuv;               // pt->sizeuv = sizeuv
		*(_BYTE *)(pt + 37) = flags;                // pt->flags = (uint8_t)flags;
		*(_DWORD *)(pt + 40) = *(_DWORD *)(ls + 76);// setgcref(pt->chunkname, obj2gco(ls->chunkname));
		*(_DWORD *)(pt + ofsk - (4 * sizekgc + 4)) = 0;// *(uint32_t *)((char *)pt + ofsk - sizeof(GCRef)*(sizekgc+1)) = 0;
		if (sizeuv)                               // bcread_uv()
			bcread_uv_mod(ls, (vec4i **)(pt + 20), sizeuv);
		v25 = *(__int16 **)(ls + 24);
		*(_DWORD *)(pt_ + 64) = ((*(_BYTE *)(pt_ + 37) & 2u) < 1 ? 89 : 92) | (*(unsigned __int8 *)(pt_ + 7) << 8);
		v26 = 4 * v67;
		*(_DWORD *)(ls + 24) = (_DWORD)&v25[2 * v67];
		v27 = pt_ + 68;
		v28 = 4 * v67;
		v29 = v25;
		if (4 * v67 >= 8)
		{
			if (v27 & 1)
			{
				v29 = (__int16 *)((char *)v25 + 1);
				v28 = v26 - 1;
				v27 = pt_ + 69;
				*(_BYTE *)(pt_ + 68) = *(_BYTE *)v25;
			}
			if (v27 & 2)
			{
				v53 = *v29;
				v27 += 2;
				++v29;
				v28 -= 2;
				*(_WORD *)(v27 - 2) = v53;
			}
			if (v27 & 4)
			{
				v52 = *(_DWORD *)v29;
				v27 += 4;
				v29 += 2;
				v28 -= 4;
				*(_DWORD *)(v27 - 4) = v52;
			}
		}
		qmemcpy((void *)v27, v29, v28);
		if (*(_BYTE *)(ls + 108) & 1 && sizebc > 1)
		{
			v49 = pt_;
			v50 = pt_ + v26;
			do
			{
				v51 = *(_DWORD *)(v49 + 68);
				v49 += 4;
				*(_DWORD *)(v49 + 64) = _byteswap_ulong(v51);
			} while (v49 != v50);
		}
		else if (!v67)
		{
			goto LABEL_32;
		}
		v30 = pt_ + 68;
		v31 = 0;
		v65 = 0;
		do
		{
			v32 = *(unsigned __int8 *)(v30 + 4 * v31);
			v33 = *(_BYTE *)(v30 + 4 * v31);
			v34 = v32
				- 253
				* ((signed int)(v32 + (-2122019415LL * (unsigned __int64)*(unsigned __int8 *)(v30 + 4 * v31) >> 32)) >> 7);
			*(_BYTE *)(v30 + 4 * v31 + 3) = ~*(_BYTE *)(v30 + 4 * v31 + 3);
			v35 = (unsigned __int8)(v34 * v34 % 253) * (unsigned __int8)(v34 * v34 % 253);
			v36 = (unsigned __int8)(v35 - -3 * ((signed int)(v35 + ((unsigned __int64)(-2122019415LL * v35) >> 32)) >> 7));
			v37 = (unsigned __int8)(v36 * v36
				- -3
				* ((signed int)(v36 * v36 + ((unsigned __int64)(-2122019415LL * v36 * v36) >> 32)) >> 7));
			v38 = v32
				* (v37 * v37 - 253 * ((signed int)(v37 * v37 + ((unsigned __int64)(-2122019415LL * v37 * v37) >> 32)) >> 7));
			v39 = (unsigned __int8)(v38 % 253) * (unsigned __int8)(v38 % 253);
			v40 = v32 * (v39 - 253 * ((signed int)(v39 + ((unsigned __int64)(-2122019415LL * v39) >> 32)) >> 7));
			v41 = (unsigned __int8)(v40 % 253) * (unsigned __int8)(v40 % 253);
			*(_BYTE *)(v30 + 4 * v31) = (v41 - 253
				* ((signed int)(v41 + ((unsigned __int64)(-2122019415LL * v41) >> 32)) >> 7))
				* v32
				% 253;
			*(_BYTE *)(v30 + 4 * v31 + 2) ^= v65;
			*(_BYTE *)(v30 + 4 * v31 + 1) ^= v65 ^ v33;
			++v31;
			++v65;
		} while (v31 != v67);
	LABEL_32:
		bcread_knum_mod(ls, *(_DWORD *)(pt_ + 16), sizekn);
		bcread_kgc_mod(ls, *(_DWORD *)(pt_ + 16), sizekgc);
		*(_DWORD *)(pt_ + 24) = sizekgc;
		*(_DWORD *)(pt_ + 44) = v78;
		*(_DWORD *)(pt_ + 48) = v73;
		if (sizedbg)
		{
			v42 = 0;
			if (v73 > 255)
				v42 = (v73 >= 0x10000) + 1;
			*(_DWORD *)(pt_ + 52) = v79 + pt_;
			*(_DWORD *)(pt_ + 56) = pt_ + (v67 << v42) + v79;
			bcread_dbg_mod(ls, pt_, sizedbg);
			v43 = *(unsigned __int8 *)(pt_ + 36);
			v44 = *(_DWORD *)(pt_ + 56);
			if (*(_BYTE *)(pt_ + 36))
			{
				do
				{
					do
						++v44;
					while (*(_BYTE *)(v44 - 1));
					--v43;
				} while (v43);
			}
			*(_DWORD *)(pt_ + 60) = v44;
			v45 = ls;
			if (*(_DWORD *)(ls + 24) != v74)
				goto LABEL_42;
		}
		else
		{
			*(_DWORD *)(pt_ + 52) = 0;
			*(_DWORD *)(pt_ + 56) = 0;
			*(_DWORD *)(pt_ + 60) = 0;
			v45 = ls;
			if (*(_DWORD *)(ls + 24) != v74)
				goto LABEL_42;
		}
		v46 = (int *)L_[5];
		*v46 = pt_;
		v46[1] = -8;
		v47 = L_[5] + 8;
		v48 = v47 < L_[6];
		L_[5] = v47;
		if (!v48)
			lj_state_growstack1((lua_State*)L_);
	}                                             // End of while ( 1 )
	v56 = (int)(v13 + 1);
	*(_DWORD *)(ls + 24) = v56;
	if ((signed int)(2 * (v14 - v56)) > 0)
		goto LABEL_60;
LABEL_57:
	v54 = L_[5];
	if (v54 - 8 != *(_DWORD *)(ls + 72) + L_[7])
	{
	LABEL_60:
		v45 = ls;
	LABEL_42:
		bcread_error_mod(*(_DWORD *)(v45 + 4), *(const char **)(v45 + 80), 2972);// bcread_error(ls, LJ_ERR_BCBAD);
	}
	L_[5] = v54 - 8;
	return *(_DWORD *)(v54 - 8);
}