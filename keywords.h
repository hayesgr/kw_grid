#ifndef KEYWORDS_H
#define KEYWORDS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <smmintrin.h>
#define MAX_KW_LEN 16
#define INVALID_TOKEN_ID 0xFFFFFFFFLU

// 16-byte payload aligned for XMM direct vector operations
typedef union {
    char bytes[16];
    uint64_t chunk[2];
    __uint128_t raw128;
    __m128i vec;
} KW_Data,kw_data;

typedef struct {
    KW_Data data;
    uint32_t id;
    uint32_t padding[3];
}__attribute__((aligned(32))) KeywordEntry;

typedef struct {
    uint16_t start; // Changed to uint16_t to support larger tables safely
    uint8_t count;
} Slice;

// Default static list of keywords (Max 16 chars each)
extern const char *keywords[];
extern const uint32_t DEFAULT_KW_COUNT;

// Core API Functions
int init_table(void);
int print_tables(void);
uint32_t is_Keyword(const KW_Data* kw, uint8_t length);

#endif // KEYWORDS_H
