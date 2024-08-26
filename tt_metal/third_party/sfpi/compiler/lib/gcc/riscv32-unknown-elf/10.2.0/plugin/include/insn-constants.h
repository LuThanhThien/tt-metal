/* Generated automatically by the program `genconstants'
   from the machine description file `md'.  */

#ifndef GCC_INSN_CONSTANTS_H
#define GCC_INSN_CONSTANTS_H

#define S3_REGNUM 19
#define S11_REGNUM 27
#define T0_REGNUM 5
#define S7_REGNUM 23
#define RETURN_ADDR_REGNUM 1
#define SIBCALL_RETURN 1
#define EXCEPTION_RETURN 2
#define S5_REGNUM 21
#define S9_REGNUM 25
#define S1_REGNUM 9
#define GP_REGNUM 3
#define S4_REGNUM 20
#define S10_REGNUM 26
#define T1_REGNUM 6
#define S8_REGNUM 24
#define NORMAL_RETURN 0
#define S6_REGNUM 22
#define S0_REGNUM 8
#define S2_REGNUM 18

enum unspec {
  UNSPEC_EH_RETURN = 0,
  UNSPEC_ADDRESS_FIRST = 1,
  UNSPEC_PCREL = 2,
  UNSPEC_LOAD_GOT = 3,
  UNSPEC_TLS = 4,
  UNSPEC_TLS_LE = 5,
  UNSPEC_TLS_IE = 6,
  UNSPEC_TLS_GD = 7,
  UNSPEC_AUIPC = 8,
  UNSPEC_FLT_QUIET = 9,
  UNSPEC_FLE_QUIET = 10,
  UNSPEC_COPYSIGN = 11,
  UNSPEC_LRINT = 12,
  UNSPEC_LROUND = 13,
  UNSPEC_TIE = 14,
  UNSPEC_COMPARE_AND_SWAP = 15,
  UNSPEC_SYNC_OLD_OP = 16,
  UNSPEC_SYNC_EXCHANGE = 17,
  UNSPEC_ATOMIC_STORE = 18,
  UNSPEC_MEMORY_BARRIER = 19
};
#define NUM_UNSPEC_VALUES 20
extern const char *const unspec_strings[];

enum unspecv {
  UNSPECV_GPR_SAVE = 0,
  UNSPECV_GPR_RESTORE = 1,
  UNSPECV_FRFLAGS = 2,
  UNSPECV_FSFLAGS = 3,
  UNSPECV_MRET = 4,
  UNSPECV_SRET = 5,
  UNSPECV_URET = 6,
  UNSPECV_BLOCKAGE = 7,
  UNSPECV_FENCE = 8,
  UNSPECV_FENCE_I = 9,
  UNSPECV_LOAD_IMMEDIATE = 10,
  UNSPECV_SFPASSIGNLREG = 11,
  UNSPECV_SFPASSIGNLREG_INT = 12,
  UNSPECV_SFPXFCMPV = 13,
  UNSPECV_SFPXICMPS = 14,
  UNSPECV_SFPXICMPV = 15,
  UNSPECV_SFPXVIF = 16,
  UNSPECV_SFPXBOOL = 17,
  UNSPECV_SFPXCONDB = 18,
  UNSPECV_SFPXCONDI = 19,
  UNSPECV_SFPINCRWC = 20,
  UNSPECV_SFPNONIMM_DST = 21,
  UNSPECV_SFPNONIMM_DST_SRC = 22,
  UNSPECV_SFPNONIMM_SRC = 23,
  UNSPECV_SFPNONIMM_STORE = 24,
  UNSPECV_GS_L1_LOAD_WAR = 25,
  UNSPECV_GS_SFPASSIGN_LV = 26,
  UNSPECV_GS_SFPPRESERVELREG = 27,
  UNSPECV_GS_SFPPRESERVELREG0_INT = 28,
  UNSPECV_GS_SFPPRESERVELREG1_INT = 29,
  UNSPECV_GS_SFPPRESERVELREG2_INT = 30,
  UNSPECV_GS_SFPPRESERVELREG3_INT = 31,
  UNSPECV_GS_SFPLOAD = 32,
  UNSPECV_GS_SFPLOAD_LV = 33,
  UNSPECV_GS_SFPLOAD_INT = 34,
  UNSPECV_GS_SFPXLOADI = 35,
  UNSPECV_GS_SFPXLOADI_LV = 36,
  UNSPECV_GS_SFPLOADI_INT = 37,
  UNSPECV_GS_SFPSTORE = 38,
  UNSPECV_GS_SFPSTORE_INT = 39,
  UNSPECV_GS_SFPMULI = 40,
  UNSPECV_GS_SFPMULI_INT = 41,
  UNSPECV_GS_SFPADDI = 42,
  UNSPECV_GS_SFPADDI_INT = 43,
  UNSPECV_GS_SFPMUL = 44,
  UNSPECV_GS_SFPMUL_LV = 45,
  UNSPECV_GS_SFPMUL_INT = 46,
  UNSPECV_GS_SFPADD = 47,
  UNSPECV_GS_SFPADD_LV = 48,
  UNSPECV_GS_SFPADD_INT = 49,
  UNSPECV_GS_SFPIADD_V_INT = 50,
  UNSPECV_GS_SFPXIADD_V = 51,
  UNSPECV_GS_SFPIADD_I = 52,
  UNSPECV_GS_SFPIADD_I_LV = 53,
  UNSPECV_GS_SFPXIADD_I = 54,
  UNSPECV_GS_SFPXIADD_I_LV = 55,
  UNSPECV_GS_SFPIADD_I_INT = 56,
  UNSPECV_GS_SFPSHFT_V = 57,
  UNSPECV_GS_SFPSHFT_I = 58,
  UNSPECV_GS_SFPSHFT_I_INT = 59,
  UNSPECV_GS_SFPABS = 60,
  UNSPECV_GS_SFPABS_LV = 61,
  UNSPECV_GS_SFPABS_INT = 62,
  UNSPECV_GS_SFPAND = 63,
  UNSPECV_GS_SFPOR = 64,
  UNSPECV_GS_SFPNOT = 65,
  UNSPECV_GS_SFPNOT_LV = 66,
  UNSPECV_GS_SFPNOT_INT = 67,
  UNSPECV_GS_SFPLZ = 68,
  UNSPECV_GS_SFPLZ_LV = 69,
  UNSPECV_GS_SFPLZ_INT = 70,
  UNSPECV_GS_SFPSETMAN_V = 71,
  UNSPECV_GS_SFPSETMAN_I = 72,
  UNSPECV_GS_SFPSETMAN_I_LV = 73,
  UNSPECV_GS_SFPSETMAN_I_INT = 74,
  UNSPECV_GS_SFPSETEXP_V = 75,
  UNSPECV_GS_SFPSETEXP_I = 76,
  UNSPECV_GS_SFPSETEXP_I_LV = 77,
  UNSPECV_GS_SFPSETEXP_I_INT = 78,
  UNSPECV_GS_SFPSETSGN_V = 79,
  UNSPECV_GS_SFPSETSGN_I = 80,
  UNSPECV_GS_SFPSETSGN_I_LV = 81,
  UNSPECV_GS_SFPSETSGN_I_INT = 82,
  UNSPECV_GS_SFPMAD = 83,
  UNSPECV_GS_SFPMAD_LV = 84,
  UNSPECV_GS_SFPMAD_INT = 85,
  UNSPECV_GS_SFPMOV = 86,
  UNSPECV_GS_SFPMOV_LV = 87,
  UNSPECV_GS_SFPMOV_INT = 88,
  UNSPECV_GS_SFPDIVP2 = 89,
  UNSPECV_GS_SFPDIVP2_LV = 90,
  UNSPECV_GS_SFPDIVP2_INT = 91,
  UNSPECV_GS_SFPEXEXP = 92,
  UNSPECV_GS_SFPEXEXP_LV = 93,
  UNSPECV_GS_SFPEXEXP_INT = 94,
  UNSPECV_GS_SFPEXMAN = 95,
  UNSPECV_GS_SFPEXMAN_LV = 96,
  UNSPECV_GS_SFPEXMAN_INT = 97,
  UNSPECV_GS_SFPSETCC_I = 98,
  UNSPECV_GS_SFPSETCC_V = 99,
  UNSPECV_GS_SFPXFCMPS = 100,
  UNSPECV_GS_SFPXFCMPV = 101,
  UNSPECV_GS_SFPENCC = 102,
  UNSPECV_GS_SFPCOMPC = 103,
  UNSPECV_GS_SFPPUSHC = 104,
  UNSPECV_GS_SFPPOPC = 105,
  UNSPECV_GS_SFPLUT = 106,
  UNSPECV_GS_SFPNOP = 107,
  UNSPECV_WH_SFPASSIGN_LV = 108,
  UNSPECV_WH_SFPPRESERVELREG = 109,
  UNSPECV_WH_SFPPRESERVELREG0_INT = 110,
  UNSPECV_WH_SFPPRESERVELREG1_INT = 111,
  UNSPECV_WH_SFPPRESERVELREG2_INT = 112,
  UNSPECV_WH_SFPPRESERVELREG3_INT = 113,
  UNSPECV_WH_SFPPRESERVELREG4_INT = 114,
  UNSPECV_WH_SFPPRESERVELREG5_INT = 115,
  UNSPECV_WH_SFPPRESERVELREG6_INT = 116,
  UNSPECV_WH_SFPPRESERVELREG7_INT = 117,
  UNSPECV_WH_SFPLOAD = 118,
  UNSPECV_WH_SFPLOAD_LV = 119,
  UNSPECV_WH_SFPLOAD_INT = 120,
  UNSPECV_WH_SFPXLOADI = 121,
  UNSPECV_WH_SFPXLOADI_LV = 122,
  UNSPECV_WH_SFPLOADI_INT = 123,
  UNSPECV_WH_SFPSTORE = 124,
  UNSPECV_WH_SFPSTORE_INT = 125,
  UNSPECV_WH_SFPMULI = 126,
  UNSPECV_WH_SFPMULI_INT = 127,
  UNSPECV_WH_SFPADDI = 128,
  UNSPECV_WH_SFPADDI_INT = 129,
  UNSPECV_WH_SFPMUL = 130,
  UNSPECV_WH_SFPMUL_LV = 131,
  UNSPECV_WH_SFPMUL_INT = 132,
  UNSPECV_WH_SFPADD = 133,
  UNSPECV_WH_SFPADD_LV = 134,
  UNSPECV_WH_SFPADD_INT = 135,
  UNSPECV_WH_SFPIADD_V_INT = 136,
  UNSPECV_WH_SFPXIADD_V = 137,
  UNSPECV_WH_SFPIADD_I = 138,
  UNSPECV_WH_SFPIADD_I_LV = 139,
  UNSPECV_WH_SFPXIADD_I = 140,
  UNSPECV_WH_SFPXIADD_I_LV = 141,
  UNSPECV_WH_SFPIADD_I_INT = 142,
  UNSPECV_WH_SFPSHFT_V = 143,
  UNSPECV_WH_SFPSHFT_I = 144,
  UNSPECV_WH_SFPSHFT_I_INT = 145,
  UNSPECV_WH_SFPABS = 146,
  UNSPECV_WH_SFPABS_LV = 147,
  UNSPECV_WH_SFPABS_INT = 148,
  UNSPECV_WH_SFPAND = 149,
  UNSPECV_WH_SFPOR = 150,
  UNSPECV_WH_SFPXOR = 151,
  UNSPECV_WH_SFPNOT = 152,
  UNSPECV_WH_SFPNOT_LV = 153,
  UNSPECV_WH_SFPNOT_INT = 154,
  UNSPECV_WH_SFPLZ = 155,
  UNSPECV_WH_SFPLZ_LV = 156,
  UNSPECV_WH_SFPLZ_INT = 157,
  UNSPECV_WH_SFPSETMAN_V = 158,
  UNSPECV_WH_SFPSETMAN_I = 159,
  UNSPECV_WH_SFPSETMAN_I_LV = 160,
  UNSPECV_WH_SFPSETMAN_I_INT = 161,
  UNSPECV_WH_SFPSETEXP_V = 162,
  UNSPECV_WH_SFPSETEXP_I = 163,
  UNSPECV_WH_SFPSETEXP_I_LV = 164,
  UNSPECV_WH_SFPSETEXP_I_INT = 165,
  UNSPECV_WH_SFPSETSGN_V = 166,
  UNSPECV_WH_SFPSETSGN_I = 167,
  UNSPECV_WH_SFPSETSGN_I_LV = 168,
  UNSPECV_WH_SFPSETSGN_I_INT = 169,
  UNSPECV_WH_SFPMAD = 170,
  UNSPECV_WH_SFPMAD_LV = 171,
  UNSPECV_WH_SFPMAD_INT = 172,
  UNSPECV_WH_SFPMOV = 173,
  UNSPECV_WH_SFPMOV_LV = 174,
  UNSPECV_WH_SFPMOV_INT = 175,
  UNSPECV_WH_SFPDIVP2 = 176,
  UNSPECV_WH_SFPDIVP2_LV = 177,
  UNSPECV_WH_SFPDIVP2_INT = 178,
  UNSPECV_WH_SFPEXEXP = 179,
  UNSPECV_WH_SFPEXEXP_LV = 180,
  UNSPECV_WH_SFPEXEXP_INT = 181,
  UNSPECV_WH_SFPEXMAN = 182,
  UNSPECV_WH_SFPEXMAN_LV = 183,
  UNSPECV_WH_SFPEXMAN_INT = 184,
  UNSPECV_WH_SFPSETCC_I = 185,
  UNSPECV_WH_SFPSETCC_V = 186,
  UNSPECV_WH_SFPXFCMPS = 187,
  UNSPECV_WH_SFPXFCMPV = 188,
  UNSPECV_WH_SFPENCC = 189,
  UNSPECV_WH_SFPCOMPC = 190,
  UNSPECV_WH_SFPPUSHC = 191,
  UNSPECV_WH_SFPPOPC = 192,
  UNSPECV_WH_SFPCAST = 193,
  UNSPECV_WH_SFPCAST_LV = 194,
  UNSPECV_WH_SFPCAST_INT = 195,
  UNSPECV_WH_SFPSHFT2_E = 196,
  UNSPECV_WH_SFPSHFT2_E_LV = 197,
  UNSPECV_WH_SFPSHFT2_E_INT = 198,
  UNSPECV_WH_SFPSTOCHRND_I = 199,
  UNSPECV_WH_SFPSTOCHRND_I_LV = 200,
  UNSPECV_WH_SFPSTOCHRND_I_INT = 201,
  UNSPECV_WH_SFPSTOCHRND_V = 202,
  UNSPECV_WH_SFPSTOCHRND_V_LV = 203,
  UNSPECV_WH_SFPSTOCHRND_V_INT = 204,
  UNSPECV_WH_SFPLUT = 205,
  UNSPECV_WH_SFPLUTFP32_3R = 206,
  UNSPECV_WH_SFPLUTFP32_6R = 207,
  UNSPECV_WH_SFPCONFIG_V = 208,
  UNSPECV_WH_SFPREPLAY = 209,
  UNSPECV_WH_SFPSWAP = 210,
  UNSPECV_WH_SFPSWAP_INT = 211,
  UNSPECV_WH_SFPTRANSP = 212,
  UNSPECV_WH_SFPSHFT2_G = 213,
  UNSPECV_WH_SFPSHFT2_GE = 214,
  UNSPECV_WH_SFPNOP = 215,
  UNSPECV_BH_SFPASSIGN_LV = 216,
  UNSPECV_BH_SFPPRESERVELREG = 217,
  UNSPECV_BH_SFPPRESERVELREG0_INT = 218,
  UNSPECV_BH_SFPPRESERVELREG1_INT = 219,
  UNSPECV_BH_SFPPRESERVELREG2_INT = 220,
  UNSPECV_BH_SFPPRESERVELREG3_INT = 221,
  UNSPECV_BH_SFPPRESERVELREG4_INT = 222,
  UNSPECV_BH_SFPPRESERVELREG5_INT = 223,
  UNSPECV_BH_SFPPRESERVELREG6_INT = 224,
  UNSPECV_BH_SFPPRESERVELREG7_INT = 225,
  UNSPECV_BH_SFPLOAD = 226,
  UNSPECV_BH_SFPLOAD_LV = 227,
  UNSPECV_BH_SFPLOAD_INT = 228,
  UNSPECV_BH_SFPXLOADI = 229,
  UNSPECV_BH_SFPXLOADI_LV = 230,
  UNSPECV_BH_SFPLOADI_INT = 231,
  UNSPECV_BH_SFPSTORE = 232,
  UNSPECV_BH_SFPSTORE_INT = 233,
  UNSPECV_BH_SFPMULI = 234,
  UNSPECV_BH_SFPMULI_INT = 235,
  UNSPECV_BH_SFPADDI = 236,
  UNSPECV_BH_SFPADDI_INT = 237,
  UNSPECV_BH_SFPMUL = 238,
  UNSPECV_BH_SFPMUL_LV = 239,
  UNSPECV_BH_SFPMUL_INT = 240,
  UNSPECV_BH_SFPADD = 241,
  UNSPECV_BH_SFPADD_LV = 242,
  UNSPECV_BH_SFPADD_INT = 243,
  UNSPECV_BH_SFPIADD_V_INT = 244,
  UNSPECV_BH_SFPXIADD_V = 245,
  UNSPECV_BH_SFPIADD_I = 246,
  UNSPECV_BH_SFPIADD_I_LV = 247,
  UNSPECV_BH_SFPXIADD_I = 248,
  UNSPECV_BH_SFPXIADD_I_LV = 249,
  UNSPECV_BH_SFPIADD_I_INT = 250,
  UNSPECV_BH_SFPSHFT_V = 251,
  UNSPECV_BH_SFPSHFT_I = 252,
  UNSPECV_BH_SFPSHFT_I_INT = 253,
  UNSPECV_BH_SFPABS = 254,
  UNSPECV_BH_SFPABS_LV = 255,
  UNSPECV_BH_SFPABS_INT = 256,
  UNSPECV_BH_SFPAND = 257,
  UNSPECV_BH_SFPOR = 258,
  UNSPECV_BH_SFPXOR = 259,
  UNSPECV_BH_SFPNOT = 260,
  UNSPECV_BH_SFPNOT_LV = 261,
  UNSPECV_BH_SFPNOT_INT = 262,
  UNSPECV_BH_SFPLZ = 263,
  UNSPECV_BH_SFPLZ_LV = 264,
  UNSPECV_BH_SFPLZ_INT = 265,
  UNSPECV_BH_SFPSETMAN_V = 266,
  UNSPECV_BH_SFPSETMAN_I = 267,
  UNSPECV_BH_SFPSETMAN_I_LV = 268,
  UNSPECV_BH_SFPSETMAN_I_INT = 269,
  UNSPECV_BH_SFPSETEXP_V = 270,
  UNSPECV_BH_SFPSETEXP_I = 271,
  UNSPECV_BH_SFPSETEXP_I_LV = 272,
  UNSPECV_BH_SFPSETEXP_I_INT = 273,
  UNSPECV_BH_SFPSETSGN_V = 274,
  UNSPECV_BH_SFPSETSGN_I = 275,
  UNSPECV_BH_SFPSETSGN_I_LV = 276,
  UNSPECV_BH_SFPSETSGN_I_INT = 277,
  UNSPECV_BH_SFPMAD = 278,
  UNSPECV_BH_SFPMAD_LV = 279,
  UNSPECV_BH_SFPMAD_INT = 280,
  UNSPECV_BH_SFPMOV = 281,
  UNSPECV_BH_SFPMOV_LV = 282,
  UNSPECV_BH_SFPMOV_INT = 283,
  UNSPECV_BH_SFPDIVP2 = 284,
  UNSPECV_BH_SFPDIVP2_LV = 285,
  UNSPECV_BH_SFPDIVP2_INT = 286,
  UNSPECV_BH_SFPEXEXP = 287,
  UNSPECV_BH_SFPEXEXP_LV = 288,
  UNSPECV_BH_SFPEXEXP_INT = 289,
  UNSPECV_BH_SFPEXMAN = 290,
  UNSPECV_BH_SFPEXMAN_LV = 291,
  UNSPECV_BH_SFPEXMAN_INT = 292,
  UNSPECV_BH_SFPSETCC_I = 293,
  UNSPECV_BH_SFPSETCC_V = 294,
  UNSPECV_BH_SFPXFCMPS = 295,
  UNSPECV_BH_SFPXFCMPV = 296,
  UNSPECV_BH_SFPENCC = 297,
  UNSPECV_BH_SFPCOMPC = 298,
  UNSPECV_BH_SFPPUSHC = 299,
  UNSPECV_BH_SFPPOPC = 300,
  UNSPECV_BH_SFPCAST = 301,
  UNSPECV_BH_SFPCAST_LV = 302,
  UNSPECV_BH_SFPCAST_INT = 303,
  UNSPECV_BH_SFPSHFT2_E = 304,
  UNSPECV_BH_SFPSHFT2_E_LV = 305,
  UNSPECV_BH_SFPSHFT2_E_INT = 306,
  UNSPECV_BH_SFPSTOCHRND_I = 307,
  UNSPECV_BH_SFPSTOCHRND_I_LV = 308,
  UNSPECV_BH_SFPSTOCHRND_I_INT = 309,
  UNSPECV_BH_SFPSTOCHRND_V = 310,
  UNSPECV_BH_SFPSTOCHRND_V_LV = 311,
  UNSPECV_BH_SFPSTOCHRND_V_INT = 312,
  UNSPECV_BH_SFPLUT = 313,
  UNSPECV_BH_SFPLUTFP32_3R = 314,
  UNSPECV_BH_SFPLUTFP32_6R = 315,
  UNSPECV_BH_SFPCONFIG_V = 316,
  UNSPECV_BH_SFPREPLAY = 317,
  UNSPECV_BH_SFPSWAP = 318,
  UNSPECV_BH_SFPSWAP_INT = 319,
  UNSPECV_BH_SFPTRANSP = 320,
  UNSPECV_BH_SFPSHFT2_G = 321,
  UNSPECV_BH_SFPSHFT2_GE = 322,
  UNSPECV_BH_SFPNOP = 323,
  UNSPECV_BH_SFPMUL24 = 324,
  UNSPECV_BH_SFPMUL24_LV = 325,
  UNSPECV_BH_SFPMUL24_INT = 326,
  UNSPECV_BH_SFPARECIP = 327,
  UNSPECV_BH_SFPARECIP_LV = 328,
  UNSPECV_BH_SFPARECIP_INT = 329,
  UNSPECV_BH_SFPGT = 330,
  UNSPECV_BH_SFPLE = 331
};
#define NUM_UNSPECV_VALUES 332
extern const char *const unspecv_strings[];

#endif /* GCC_INSN_CONSTANTS_H */
