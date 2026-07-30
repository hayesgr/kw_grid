#include "keywords.h"
const char *keywords[]={
    "auto",
    "atomic",
    "alignas",
    "alignof",
    "bool",
    "break",
    "c8",
    "c16",
    "c32",
    "case",
    "const",
    "char8_t",
    "char8_t*",
    "char16_t",
    "char16_t*",
    "char32_t",
    "char32_t*",
    "constexpr",
    "continue",
    "do",
    "default",
    "else",
    "enum",
    "extern",
    "f32",
    "f64",
    "f80",
    "f128",
    "for",
    "false",
    "float32_t",
    "float32_t*",
    "float64_t",
    "float64_t*",
    "float80_t",
    "float80_t*",
    "float128_t",
    "float128_t*",
    "goto",
    "generic",
    "i8",
    "if",
    "i16",
    "i32",
    "i64",
    "i128",
    "inline",
    "int8_t",
    "int8_t*",
    "int16_t",
    "int16_t*",
    "int32_t",
    "int32_t*",
    "int64_t",
    "int64_t*",
    "int128_t",
    "int128_t*",
    "noreturn",
    "null",
    "nullptr",
    "pub",
    "priv",
    "public",
    "private",
    "return",
    "retrict",
    "register",
    "sizeof",
    "static",
    "struct",
    "switch",
    "static_assert",
    "true",
    "typedef",
    "thread_local",
    "u8",
    "u16",
    "u32",
    "u64",
    "u128",
    "union",
    "uint8_t",
    "uint8_t*",
    "uint16_t",
    "uint16_t*",
    "uint32_t",
    "uint32_t*",
    "uint64_t",
    "uint64_t*",
    "uint128_t",
    "uint128_t*",
    "void",
    "void*",
    "volatile",
    "while"
};

typedef uint32_t bool_t;
const uint32_t DEFAULT_KW_COUNT = sizeof(keywords) / sizeof(keywords[0]);

static Slice KEYWORD_GRID[256][16];
static KeywordEntry *KEYWORDS = NULL;
static uint32_t KW_COUNT = 0;

uint32_t str_len(const char* str) {
    uint32_t r = 0;
    while (str[r] != 0) r++;
    return r;
}

// Helper to sort keywords by first character, then length
int compare_keywords(const void* a, const void* b) {
    const char* strA = *(const char**)a;
    const char* strB = *(const char**)b;

    if (strA[0] != strB[0])
        return (unsigned char)strA[0] - (unsigned char)strB[0];

    uint32_t lenA = str_len(strA);
    uint32_t lenB = str_len(strB);

    return (int)lenA - (int)lenB;
}

int init_table(void) {
    KW_COUNT = DEFAULT_KW_COUNT;

    // Allocate local array of string pointers for sorting
    const char** sorted_kw = malloc(KW_COUNT * sizeof(char*));
    memcpy(sorted_kw, keywords, KW_COUNT * sizeof(char*));

    // 1. Sort keywords so same (first_letter, length) entries group together
    qsort(sorted_kw, KW_COUNT, sizeof(char*), compare_keywords);

    // 2. Setup arrays
    KEYWORDS = calloc(KW_COUNT, sizeof(KeywordEntry));
    memset(KEYWORD_GRID, 0xFF, sizeof(KEYWORD_GRID)); // 0xFFFF sets start to 65535 sentinel

    // 3. Build Grid Ranges
    for (uint32_t i = 0; i < KW_COUNT; i++) {
        uint32_t len = str_len(sorted_kw[i]);
        uint8_t letter_idx = (uint8_t)sorted_kw[i][0];

        if (len > MAX_KW_LEN) continue; // Safety guard

        if (KEYWORD_GRID[letter_idx][len-1].start == 0xFFFF) {
            KEYWORD_GRID[letter_idx][len-1].start = (uint16_t)i;
            KEYWORD_GRID[letter_idx][len-1].count = 1;
        } else {
            KEYWORD_GRID[letter_idx][len-1].count++;
        }
    }

    // 4. Build 128-bit SIMD Chunks
    for (uint32_t i = 0; i < KW_COUNT; i++) {
        uint32_t len = str_len(sorted_kw[i]);
        KW_Data TB;
        TB.raw128 = 0; // Clear all 16 bytes

        for (uint32_t j = 0; j < len && j < MAX_KW_LEN; j++) {
            TB.bytes[j] = sorted_kw[i][j];
        }

        KEYWORDS[i].data.chunk[0] = TB.chunk[0];
        KEYWORDS[i].data.chunk[1] = TB.chunk[1];
        KEYWORDS[i].id = i;
    }

    free(sorted_kw);
    return 1;
}
int print_tables(void) {
    printf("const Slice KW_GRID[256][16] = {\n");
    for (uint32_t letter = 0; letter < 256; letter++) {
        // Label display for printable ASCII, raw hex for others
        if (letter >= 32 && letter <= 126) {
            printf("   /* '%c' */ {", (char)letter);
        } else {
            printf("   /* 0x%02X */ {", letter);
        }

        for (uint32_t len = 0; len < 16; len++) {
            printf("{%5u, %3u}", KEYWORD_GRID[letter][len].start, KEYWORD_GRID[letter][len].count);
            if (len != 15) printf(",");
        }
        printf("}");
        if (letter != 255) printf(",");
        printf("\n");
    }
    printf("};\n\n");

    printf("const KeywordEntry KEYWORDS[%u] = {\n\t", KW_COUNT);
    for (uint32_t i = 0; i < KW_COUNT; i++) {
        printf("{{{0x%016llXULL, 0x%016llXULL}}, %2u}",
               (unsigned long long)KEYWORDS[i].data.chunk[0],
               (unsigned long long)KEYWORDS[i].data.chunk[1],
               KEYWORDS[i].id);
        if (i != KW_COUNT - 1) printf(", ");
        if (i % 2 == 1) printf("\n\t"); // 2 entries per line for clean layout
    }
    printf("\n};\n");

    return 1;
}

// (__x86_64__) or (_M_X64)
// --- x86_64 SSE4.1 Path ---
static inline bool_t check_16byte_match(const kw_data* token, const kw_data* kw) {
    __m128i diff = _mm_xor_si128(token->vec, kw->vec);
    return _mm_testz_si128(diff, diff) != 0;
}
// (__aarch64__)
// --- ARM64 NEON Path ---
// 128-bit vector load + XOR + reduction max to zero check
/*static inline bool_t check_16byte_match(const kw_data* token, const kw_data* kw) {
    uint8x16_t a = vld1q_u8((const uint8_t*)token_ptr);
    uint8x16_t b = vld1q_u8((const uint8_t*)kw_ptr);
    uint8x16_t diff = veorq_u8(a, b);
    return vmaxvq_u8(diff) == 0;
}*/

//Fallback method
// --- Portable 64-bit SWAR Fallback (RISC-V, WebAssembly, 32-bit x86) ---
/*static inline bool_t check_16byte_match(const kw_data* token, const kw_data* kw) {
    const uint64_t* t = (const uint64_t*)token_ptr;
    const uint64_t* k = (const uint64_t*)kw_ptr;
    return !((t[0] ^ k[0]) | (t[1] ^ k[1]));
}*/

/*
copy what you want to test into a KW_Data union then pass the pointer
If it is correct it will return an index else it return 0xFFFFFFFFLU
you should aquired length when copying the word to be tested into the union.
16 characters max length
*/
uint32_t is_Keyword(const KW_Data* kw, uint8_t length) {
    uint8_t first_letter = (uint8_t)kw->bytes[0];
    uint16_t start = KEYWORD_GRID[first_letter][length-1].start;
    uint8_t num    = KEYWORD_GRID[first_letter][length-1].count;
    if (start == 0xFFFF) return INVALID_TOKEN_ID;
    for (uint16_t k = start; k < start + num; k++) {
        if (check_16byte_match(kw, &KEYWORDS[k].data)) {
            return KEYWORDS[k].id;
        }
    }
    return INVALID_TOKEN_ID;
}
