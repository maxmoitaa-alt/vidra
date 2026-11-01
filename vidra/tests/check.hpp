#pragma once
#include <cstdio>
inline int g_fail = 0, g_pass = 0;
#define CHECK(c) do { if (c) ++g_pass; else { ++g_fail; std::fprintf(stderr, "FALHOU %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)
#define CHECK_EQ(a, b) CHECK((a) == (b))
inline int report(const char* n) { std::printf("%s: %d ok, %d falhas\n", n, g_pass, g_fail); return g_fail == 0 ? 0 : 1; }
