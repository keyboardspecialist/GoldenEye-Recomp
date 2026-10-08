#pragma once

// ReXGlue 0.10 emits the alias attributes before extern "C", where GCC ignores
// them. Keep them on the declaration so the public guest entry point is a weak
// alias of its implementation. This is included before the generated PCH and
// therefore survives codegen without modifying generated files or the SDK.
#if defined(__GNUC__) && !defined(__clang__) && !defined(__APPLE__)
#ifndef DEFINE_REX_FUNC
#define DEFINE_REX_FUNC(name)                                              \
  extern "C" REX_FUNC(name)                                                \
      __attribute__((weak, noinline, alias("__imp__" #name)));             \
  REX_EXTERN(__imp__##name)
#endif
#endif
