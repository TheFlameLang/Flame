/* Flame runtime for the C backend. Requires C11 + GCC/Clang (__typeof__, __auto_type). */
#ifndef FLAME_RT_H
#define FLAME_RT_H

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef const char *flame_str;

static inline void flame_panic(const char *msg)
{
    fprintf(stderr, "flame: %s\n", msg);
    abort();
}

/* ---------- print: replaces std::cout << ... ---------- */
static inline void flame_print_ll(long long v) { printf("%lld", v); }
static inline void flame_print_ull(unsigned long long v) { printf("%llu", v); }
static inline void flame_print_d(double v) { printf("%g", v); }
static inline void flame_print_b(bool v) { fputs(v ? "true" : "false", stdout); }
static inline void flame_print_c(char v) { putchar(v); }
static inline void flame_print_s(const char *v) { fputs(v ? v : "(null)", stdout); }
static inline void flame_print_p(const void *v) { printf("%p", v); }

#define flame_print(x) _Generic((x),                                              \
    _Bool: flame_print_b, char: flame_print_c,                                    \
    signed char: flame_print_ll, short: flame_print_ll, int: flame_print_ll,      \
    long: flame_print_ll, long long: flame_print_ll,                              \
    unsigned char: flame_print_ull, unsigned short: flame_print_ull,              \
    unsigned int: flame_print_ull, unsigned long: flame_print_ull,                \
    unsigned long long: flame_print_ull,                                          \
    float: flame_print_d, double: flame_print_d,                                  \
    char *: flame_print_s, const char *: flame_print_s,                           \
    default: flame_print_p)(x)

/* ---------- input: replaces std::cin >> x ---------- */
#define FLAME_IN(name, T, fmt) \
    static inline void name(T *p) { (void)scanf(fmt, p); }
FLAME_IN(flame_in_i8, int8_t, "%" SCNd8)
FLAME_IN(flame_in_i16, int16_t, "%" SCNd16)
FLAME_IN(flame_in_i32, int32_t, "%" SCNd32)
FLAME_IN(flame_in_i64, int64_t, "%" SCNd64)
FLAME_IN(flame_in_u8, uint8_t, "%" SCNu8)
FLAME_IN(flame_in_u16, uint16_t, "%" SCNu16)
FLAME_IN(flame_in_u32, uint32_t, "%" SCNu32)
FLAME_IN(flame_in_u64, uint64_t, "%" SCNu64)
FLAME_IN(flame_in_f, float, "%f")
FLAME_IN(flame_in_d, double, "%lf")
static inline void flame_in_b(bool *p)
{
    int t = 0;
    if (scanf("%d", &t) == 1) *p = t != 0;
}
static inline void flame_in_s(flame_str *p) /* reads one whitespace-delimited word */
{
    char buf[4096];
    if (scanf("%4095s", buf) == 1)
    {
        size_t n = strlen(buf) + 1;
        char *s = (char *)malloc(n);
        if (!s) flame_panic("out of memory");
        memcpy(s, buf, n);
        *p = s;
    }
}
#define flame_input(p) _Generic((p),                                   \
    int8_t *: flame_in_i8, int16_t *: flame_in_i16,                    \
    int32_t *: flame_in_i32, int64_t *: flame_in_i64,                  \
    uint8_t *: flame_in_u8, uint16_t *: flame_in_u16,                  \
    uint32_t *: flame_in_u32, uint64_t *: flame_in_u64,                \
    float *: flame_in_f, double *: flame_in_d,                         \
    bool *: flame_in_b, flame_str *: flame_in_s)(p)

/* ---------- strings ---------- */
static inline flame_str flame_strcat(const char *a, const char *b) /* result is malloc'd */
{
    size_t la = strlen(a), lb = strlen(b);
    char *r = (char *)malloc(la + lb + 1);
    if (!r) flame_panic("out of memory");
    memcpy(r, a, la);
    memcpy(r + la, b, lb + 1);
    return r;
}

/* ---------- vectors: `T *` with a hidden header in front of the data ----------
   v[i] works as usual, a NULL pointer is an empty vector. */
typedef struct
{
    _Alignas(max_align_t) size_t len;
    size_t cap;
} flame_vec_hdr;

#define flame_vec_hdr_of(v) (((flame_vec_hdr *)(v)) - 1)

static inline size_t flame_vec_len(const void *v) { return v ? ((const flame_vec_hdr *)v)[-1].len : 0; }

static inline void *flame_vec_reserve(void *v, size_t esz, size_t need)
{
    flame_vec_hdr *h = v ? flame_vec_hdr_of(v) : NULL;
    size_t cap = h ? h->cap : 0;
    if (need <= cap) return v;
    size_t ncap = cap ? cap * 2 : 8;
    if (ncap < need) ncap = need;
    flame_vec_hdr *n = (flame_vec_hdr *)realloc(h, sizeof *n + ncap * esz);
    if (!n) flame_panic("out of memory");
    if (!h) n->len = 0;
    n->cap = ncap;
    return n + 1;
}

static inline size_t flame_vec_bounds(size_t i, size_t len)
{
    if (i >= len) flame_panic("vector index out of range");
    return i;
}

static inline void *flame_vec_from_raw(const void *src, size_t n, size_t esz)
{
    void *v = flame_vec_reserve(NULL, esz, n);
    memcpy(v, src, n * esz);
    flame_vec_hdr_of(v)->len = n;
    return v;
}

#define flame_vec_push(v, x)                                                          \
    do                                                                                \
    {                                                                                 \
        (v) = flame_vec_reserve((v), sizeof(*(v)), flame_vec_len(v) + 1);             \
        (v)[flame_vec_hdr_of(v)->len++] = (x);                                        \
    } while (0)
#define flame_vec_pop(v) \
    (flame_vec_len(v) ? (void)(flame_vec_hdr_of(v)->len--) : flame_panic("pop from empty vector"))
#define flame_vec_at(v, i) ((v)[flame_vec_bounds((size_t)(i), flame_vec_len(v))])
#define flame_vec_from(T, n, ...) ((T *)flame_vec_from_raw((T[]){__VA_ARGS__}, (n), sizeof(T)))
#define flame_vec_free(v) ((void)((v) ? (free(flame_vec_hdr_of(v)), (v) = NULL) : NULL))

#endif /* FLAME_RT_H */