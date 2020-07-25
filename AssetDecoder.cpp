
//
#include "AssetDecoder.h"
#include "defs.h"
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

const unsigned char key_bytes[76] =
{
	0xBC, 0x4B, 0x77, 0x1D, 0xE8, 0xC0, 0x96, 0x00, 0x8C, 0xA2, 0x69, 0x00, 0xB9, 0xCB, 0xDA, 0x35,
	0xC2, 0xCE, 0x63, 0x00, 0xA5, 0x6B, 0x5A, 0x00, 0x2E, 0x71, 0x92, 0x14, 0x19, 0xDE, 0x92, 0x00,
	0x93, 0x62, 0x0B, 0x00, 0x46, 0x56, 0xDF, 0x03, 0xC2, 0x63, 0x50, 0x00, 0xD5, 0x93, 0x72, 0x00,
	0xA3, 0x5B, 0x5E, 0x00, 0xCB, 0xAF, 0xC3, 0x30, 0x76, 0xA6, 0x1C, 0x01, 0xAD, 0x4E, 0x78, 0x00,
	0x02, 0xB9, 0x4B, 0x00, 0x8C, 0xF4, 0x49, 0x00, 0x54, 0x39, 0x81, 0x05
};

Il2CppArray* NewSpecific(size_t element_size, size_t n)
{
	size_t ByteLen = n * element_size;
	size_t ByteLenAlloc = ByteLen + 16;
	Il2CppArray* arr = (Il2CppArray*)malloc(ByteLenAlloc);
	arr->klass = 0;
	arr->monitor = 0;
	arr->bounds = 0;
	memset((char*)arr + 8, 0, ByteLen + 8);
	arr->max_length = n;
	return arr;
}

FByteArray* DigitalSea_Scipio(FByteArray* _AssertArrayIn)
{
	unsigned int max_length_v1;
	uint32_t max_length_v2;
	int buf_2ndLast_byte;
	int buf_3rdLast_byte;
	unsigned int v8;
	int wrapper_size;
	int dec_buf_size;
	int *dec_buf;
	FByteArray *dec_buf_1;
	int ptr_bytes_array_1;
	uintptr_t v21;
	uint32_t enc_data_size;
	FByteArray *Scipio_GcBuffer;
	uint8_t v26;
	int v29;
	uint32_t i;
	FByteArray *Scipio_GcBuffer_2;
	int gcbuf_i;
	int dec_i;
	int v38;
	FByteArray *Scipio_GcBuffer_3;
	int v41;
	unsigned int dec_buf_size_1;
	unsigned int v45;
	int ptr_bytes_array_2;
	int v47;
	int v51;
	int v53;
	int v54;
	int v59;
	int v61;
	int *v62;
	int v63;
	unsigned int v65;
	uint32_t *v66;
	unsigned int v68;
	unsigned int v70;
	unsigned int v71;
	int v75;
	signed int v77;
	unsigned int v78;
	int v81;
	unsigned int v83;
	int v84;
	unsigned int v85;
	unsigned int v86;
	unsigned int v87;
	unsigned int v89;
	uint32_t *v91;
	int v93;
	unsigned int v94;
	unsigned int v96;
	uint32_t *v98;
	int v100;
	uint32_t *v101;
	unsigned int v104;
	unsigned int v105;
	uint32_t v106;
	unsigned int v107;
	unsigned int v108;
	unsigned int v112;
	int v116;
	unsigned int v118;
	int v121;
	int v123;
	int v125;
	int v128;
	int v132;
	unsigned int v136;
	unsigned int v138;
	unsigned int v141;
	unsigned int v143;
	int v146;
	int v150;
	unsigned int v154;
	unsigned int v156;
	unsigned int v159;
	unsigned int v161;
	uint8_t v164;
	unsigned int dec_buf_size_minus8;
	unsigned int v169;
	unsigned int v170;
	unsigned int v171;
	unsigned int v172;
	unsigned int v173;
	unsigned int v174;
	unsigned int v175;
	FByteArray *dec_buf_2;
	unsigned int v177;
	unsigned int v178;
	unsigned int *p_max_length;
	int v180;
	int v181;
	int v182;
	unsigned int v183;
	int *v184;
	int v185;
	unsigned int v186;
	int v187;
	int v188;
	unsigned int v189;
	unsigned int v190;
	int v191;
	unsigned int v193;
	int v194;
	int v195;
	int v196;
	unsigned int v197;
	unsigned int v198;
	int v199;
	int v200;
	int v201;
	unsigned int v202;
	unsigned int v203;
	int buf_last_byte;
	uint32_t *dec_key;

	int ptr_bytes_array = (int)_AssertArrayIn;
	FByteArray* pGcBuffer = nullptr;

	if (ptr_bytes_array)
	{
		p_max_length = (unsigned int *)(ptr_bytes_array + 12);
		max_length_v1 = *(_DWORD *)(ptr_bytes_array + 12);
		max_length_v2 = *(_DWORD *)(ptr_bytes_array + 12);
	}
	else
	{
		assert(false);
	}

	if (*p_max_length <= max_length_v2 - 1)
	{
		assert(false);
	}
	buf_last_byte = *(unsigned __int8 *)(ptr_bytes_array + max_length_v2 - 1 + 16);

	if (*p_max_length <= max_length_v2 - 2)
	{
		assert(false);
	}
	buf_2ndLast_byte = *(unsigned __int8 *)(ptr_bytes_array + max_length_v2 - 2 + 16);

	if (*p_max_length <= max_length_v2 - 3)
	{
		assert(false);
	}
	buf_3rdLast_byte = *(unsigned __int8 *)(ptr_bytes_array + max_length_v2 - 3 + 16);

	v8 = max_length_v2 - 4;
	if (*p_max_length <= v8)
	{
		assert(false);
	}

	wrapper_size = (buf_last_byte + (buf_2ndLast_byte << 8)) | (buf_3rdLast_byte << 16) | (*(unsigned __int8 *)(ptr_bytes_array + v8 + 16) << 24);
	dec_buf_size = *p_max_length - 4 - wrapper_size;

	dec_buf = (int*)NewSpecific(sizeof(char), dec_buf_size);
	assert(dec_buf);
	dec_buf_1 = (FByteArray *)dec_buf;
	dec_buf_2 = (FByteArray *)dec_buf;

	dec_key = (uint32_t*)NewSpecific(sizeof(uint32_t), 19);
	assert(dec_key);

	FByteArray* key_array = (FByteArray*)dec_key;
	memcpy(key_array->m_Items, key_bytes, 76);

	if (wrapper_size <= 0)
	{
		pGcBuffer = 0;
	}
	else
	{
		dec_buf_2 = dec_buf_1;
		pGcBuffer = (FByteArray*)(NewSpecific(sizeof(char), wrapper_size));
		ptr_bytes_array_1 = ptr_bytes_array;

		v187 = *p_max_length - 4 - wrapper_size;
		v185 = ptr_bytes_array + *p_max_length + 12 - wrapper_size;
		v21 = 0;
		while (1)
		{
			enc_data_size = v187 + v21;
			if (enc_data_size >= (signed int)(*p_max_length - 4))
				break;

			Scipio_GcBuffer = pGcBuffer;

			if (*p_max_length <= enc_data_size)
			{
				assert(false);
			}
			v26 = *(_BYTE *)(v185 + v21);

			if ((uint32_t)Scipio_GcBuffer->max_length <= v21)
			{
				assert(false);
			}

			Scipio_GcBuffer->m_Items[v21++] = v26;
			ptr_bytes_array_1 = ptr_bytes_array;
		}
		v29 = 235;
		for (i = 0; ; ++i)
		{
			v193 = v29;
			Scipio_GcBuffer_2 = pGcBuffer;
			if ((signed int)i >= (signed int)Scipio_GcBuffer_2->max_length)
				break;

			if ((uint32_t)pGcBuffer->max_length <= i)
			{
				assert(false);
			}
			gcbuf_i = pGcBuffer->m_Items[i];
			dec_i = gcbuf_i ^ (v193 >> 8);
			v38 = v193 + gcbuf_i;
			Scipio_GcBuffer_3 = pGcBuffer;

			v41 = 205 * v38;
			if ((uint32_t)Scipio_GcBuffer_3->max_length <= i)
			{
				assert(false);
			}
			v29 = v41 + 207;
			Scipio_GcBuffer_3->m_Items[i] = dec_i;
		}
	}

	v184 = (int*)NewSpecific(sizeof(uint32_t), 2);
	assert(v184);
	dec_buf_size_1 = dec_buf_1->max_length;
	dec_buf_size_minus8 = dec_buf_size_1 - 8;
	v45 = 0;
	ptr_bytes_array_2 = ptr_bytes_array;
	if (dec_buf_size_1 != 8)
	{
		v47 = 0;
		v45 = 0;
		do
		{
			v177 = v47;

			if (*p_max_length <= v45)
			{
				assert(false);
			}
			v199 = *(unsigned __int8 *)(ptr_bytes_array_2 + v45 + 16);

			if (*p_max_length <= (v45 | 1))
			{
				assert(false);
			}
			v194 = *(unsigned __int8 *)(ptr_bytes_array_2 + (v45 | 1) + 16);
			v169 = v45 | 1;

			v178 = v45;
			if (*p_max_length <= (v45 | 2))
			{
				assert(false);
			}
			v170 = v45 | 2;
			v51 = *(unsigned __int8 *)(ptr_bytes_array_2 + (v45 | 2) + 16);

			if (*p_max_length <= (v45 | 3))
			{
				assert(false);
			}
			v53 = *(unsigned __int8 *)(ptr_bytes_array_2 + (v45 | 3) + 16);
			v172 = v45 | 3;

			v54 = (v199 + (v194 << 8)) | (v51 << 16) | (v53 << 24);
			if (!v184[3])
			{
				assert(false);
			}
			v184[4] = v54;

			if (*p_max_length <= (v45 | 4))
			{
				assert(false);
			}
			v200 = *(unsigned __int8 *)(ptr_bytes_array + (v45 | 4) + 16);
			v171 = v45 | 4;

			if (*p_max_length <= (v45 | 5))
			{
				assert(false);
			}
			v195 = *(unsigned __int8 *)(ptr_bytes_array + (v45 | 5) + 16);
			v173 = v45 | 5;

			if (*p_max_length <= (v45 | 6))
			{
				assert(false);
			}
			v59 = *(unsigned __int8 *)(ptr_bytes_array + (v45 | 6) + 16);
			v174 = v45 | 6;

			if (*p_max_length <= (v45 | 7))
			{
				assert(false);
			}
			v61 = *(unsigned __int8 *)(ptr_bytes_array + (v45 | 7) + 16);
			v175 = v45 | 7;
			v62 = v184;

			v63 = (v200 + (v195 << 8)) | (v59 << 16) | (v61 << 24);
			if ((unsigned int)v184[3] <= 1)
			{
				assert(false);
			}
			v184[5] = v63;
			if (v177)
			{
				if (v177 > 1)
				{
					v101 = dec_key;
					if (!v184[3])
					{
						assert(false);
					}
					if (v177 == 3)
					{
						v190 = v184[4];
						LOBYTE(v177) = 3;
						if ((unsigned int)v184[3] <= 1)
						{
							assert(false);
						}
						v104 = v184[5];
						v105 = 234846730;
						v106 = 5;
						do
						{
							v198 = v104;
							if (!v101)
							{
								assert(false);
							}
							v107 = (v105 >> 11) & 3;
							v108 = v101[3];
							v203 = v105;
							if (v108 <= v107)
							{
								assert(false);
							}
							v104 = v198 - ((v190 + (16 * v190 ^ (v190 >> 5))) ^ (v105 + dec_key[v107 + 4]));
							if (v108 <= v106 - 4)
							{
								assert(false);
							}
							v190 -= (v104 + (16 * v104 ^ (v104 >> 5))) ^ (v105 + dec_key[v106--] - 117423365);
							v105 -= 117423365;
							v101 = dec_key;
						} while (v203 != 117423365);
						v62 = v184;

						v112 = v184[3];
						if (!v112)
						{
							assert(false);
						}
						v184[4] = v190;
						if (v112 <= 1)
						{
							assert(false);
						}
						v184[5] = v104;
					}
					else
					{
						v191 = v184[4];

						if ((unsigned int)v101[3] <= 0xD)
						{
							assert(false);
						}
						v116 = v101[17];

						v118 = v62[3];
						if (!v118)
						{
							assert(false);
						}
						v62[4] = v116 ^ v178 ^ v191;
						if (v118 <= 1)
						{
							assert(false);
						}
						v121 = v62[5];

						if ((unsigned int)v101[3] <= 2)
						{
							assert(false);
						}
						v123 = v101[6];
						v62 = v184;

						v125 = v123 ^ v178 ^ v121;
						if ((unsigned int)v184[3] <= 1)
						{
							assert(false);
						}
						v184[5] = v125;
					}
				}
				else
				{
					v65 = v184[3];
					v66 = dec_key;
					if (!v65)
					{
						assert(false);
					}
					v68 = v184[4];
					if (v65 <= 1)
					{
						assert(false);
					}
					v70 = v184[5];

					v71 = v66[3];
					if (!v71)
					{
						assert(false);
					}
					v201 = v66[4];
					if (v71 <= 1)
					{
						assert(false);
					}
					v196 = v66[5];
					if (v71 <= 2)
					{
						assert(false);
					}
					v75 = v66[6];
					if (v71 <= 3)
					{
						assert(false);
					}
					v188 = v66[7];
					v77 = 234846730;
					do
					{
						v70 -= (v75 + 16 * v68) ^ (v77 + v68) ^ (v188 + (v68 >> 5));
						v68 -= (v201 + 16 * v70) ^ (v77 + v70) ^ (v196 + (v70 >> 5));
						v77 -= 117423365;
					} while (v77);

					v78 = v184[3];
					if (!v78)
					{
						assert(false);
					}
					v184[4] = v68;
					if (v78 <= 1)
					{
						assert(false);
					}
					v184[5] = v70;
					v62 = v184;
				}
			}
			else
			{
				v81 = v184[3];
				if (!v81)
				{
					assert(false);
				}
				v202 = v184[4];
				v183 = v81 - 1;
				v180 = v81 - 2;
				v181 = v184[3];
				v83 = 234846730;
				v84 = 2;
				do
				{
					v182 = v84;
					v186 = v83;
					v189 = (v83 >> 2) & 3;
					v85 = v180;
					if (v183)
					{
						do
						{
							v86 = v85 + 1;
							v87 = v62[3];
							if (v87 <= v85)
							{
								assert(false);
							}
							v89 = v62[v85 + 4];
							if (v87 <= v86)
							{
								assert(false);
							}
							v91 = dec_key;

							v197 = v189 ^ v86 & 3;
							if ((uint32_t)v91[3] <= v197)
							{
								assert(false);
							}
							v93 = v184[v85 + 5]
								- ((((v89 >> 5) ^ 4 * v202) + ((v202 >> 3) ^ 16 * v89)) ^ ((v186 ^ v202)
									+ (dec_key[v197 + 4] ^ v89)));
							v62 = v184;
							v184[v85-- + 5] = v93;
							v202 = v93;
						} while (v85 != -1);
					}
					v94 = v62[3];
					if (v94 <= v183)
					{
						assert(false);
					}
					v96 = v62[v181 + 3];
					if (!v94)
					{
						assert(false);
					}
					v98 = dec_key;

					if (v98[3] <= v189)
					{
						assert(false);
					}
					v100 = v62[4]
						- ((((v96 >> 5) ^ 4 * v202) + ((v202 >> 3) ^ 16 * v96)) ^ ((v186 ^ v202)
							+ (dec_key[v189 + 4] ^ v96)));
					v62[4] = v100;
					v83 = v186 - 117423365;
					v84 = v182 - 1;
					v202 = v100;
				} while (v182 != 1);
			}
			if (!v62[3])
			{
				assert(false);
			}
			v128 = v62[4];
			dec_buf_1 = dec_buf_2;

			if ((uint32_t)dec_buf_2->max_length <= v178)
			{
				assert(false);
			}
			dec_buf_2->m_Items[v178] = v128;

			if (!v184[3])
			{
				assert(false);
			}
			v132 = v184[4];

			if ((uint32_t)dec_buf_2->max_length <= v169)
			{
				assert(false);
			}
			dec_buf_2->m_Items[v169] = BYTE1(v132);

			if (!v184[3])
			{
				assert(false);
			}
			v136 = v184[4];

			v138 = v136 >> 16;
			if ((uint32_t)dec_buf_2->max_length <= v170)
			{
				assert(false);
			}
			dec_buf_2->m_Items[v170] = v138;

			if (!v184[3])
			{
				assert(false);
			}
			v141 = v184[4];

			v143 = v141 >> 24;
			if ((uint32_t)dec_buf_2->max_length <= v172)
			{
				assert(false);
			}
			dec_buf_2->m_Items[v172] = v143;

			if ((unsigned int)v184[3] <= 1)
			{
				assert(false);
			}
			v146 = v184[5];

			if ((uint32_t)dec_buf_2->max_length <= v171)
			{
				assert(false);
			}
			dec_buf_2->m_Items[v171] = v146;

			if ((unsigned int)v184[3] <= 1)
			{
				assert(false);
			}
			v150 = v184[5];

			if ((uint32_t)dec_buf_2->max_length <= v173)
			{
				assert(false);
			}
			dec_buf_2->m_Items[v173] = BYTE1(v150);

			if ((unsigned int)v184[3] <= 1)
			{
				assert(false);
			}
			v154 = v184[5];

			v156 = v154 >> 16;
			if ((uint32_t)dec_buf_2->max_length <= v174)
			{
				assert(false);
			}
			dec_buf_2->m_Items[v174] = v156;

			if ((unsigned int)v184[3] <= 1)
			{
				assert(false);
			}
			v159 = v184[5];

			v161 = v159 >> 24;
			if ((uint32_t)dec_buf_2->max_length <= v175)
			{
				assert(false);
			}
			dec_buf_2->m_Items[v175] = v161;
			v47 = ((_BYTE)v177 + 1) & 3;
			v45 = v178 + 8;
			ptr_bytes_array_2 = ptr_bytes_array;
		} while (v178 + 8 < dec_buf_size_minus8);
		dec_buf_size_1 = dec_buf_2->max_length;
	}
	if ((v45 < dec_buf_size_1) + ((signed int)dec_buf_size_1 >> 31) > 0)
	{
		do
		{
			if (*p_max_length <= v45)
			{
				assert(false);
			}
			v164 = *(_BYTE *)(ptr_bytes_array_2 + v45 + 16);

			if ((uint32_t)dec_buf_1->max_length <= v45)
			{
				assert(false);
			}
			dec_buf_1->m_Items[v45++] = v164;
		} while ((v45 < (uint32_t)dec_buf_1->max_length) + ((uint32_t)dec_buf_1->max_length >> 31) > 0);
	}
	return dec_buf_1;
}


