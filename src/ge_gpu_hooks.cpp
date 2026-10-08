#include "ge_init.h"

#include <rex/ppc/stack.h>

#include <cstdint>
#include <cstring>

namespace {
constexpr uint32_t kMaxTextureDimension = 8192;

uint32_t Read32(const uint8_t* base, uint32_t address) {
  uint32_t value;
  std::memcpy(&value, base + address, sizeof(value));
  return __builtin_bswap32(value);
}
}  // namespace

// Scene-copy / fog-target initialization receives 0 x UINT32_MAX after its
// caller's stack-based dimensions have been overwritten by intervening calls.
// Query the title's current raster size again before the XDK calculates pitch
// and allocation. The SDK applies configured resolution_scale to these guest
// surfaces; multiplying the dimensions here would apply that scale twice.
// Otherwise the size-minus-one fields underflow to 8192x8191 with zero pitch.
extern "C" void sub_82099B40(PPCContext& ctx, uint8_t* base) {
  if (!ctx.r3.u32 || !ctx.r4.u32 || ctx.r3.u32 > kMaxTextureDimension ||
      ctx.r4.u32 > kMaxTextureDimension) {
    // Keep the caller's registers and stack pointer intact while querying.
    PPCContext query_ctx = ctx;
    const uint32_t width_out = rex::ppc::stack_push(query_ctx, base, uint32_t{0});
    const uint32_t height_out = rex::ppc::stack_push(query_ctx, base, uint32_t{0});
    query_ctx.r3.u64 = width_out;
    query_ctx.r4.u64 = height_out;
    __imp__sub_8238D718(query_ctx, base);
    const uint32_t width = Read32(base, width_out);
    const uint32_t height = Read32(base, height_out);
    if (width && height && width <= kMaxTextureDimension && height <= kMaxTextureDimension) {
      REXGPU_INFO("GE scene-copy dimensions repaired: {}x{} -> {}x{}",
                  ctx.r3.u32, ctx.r4.u32, width, height);
      ctx.r3.u32 = width;
      ctx.r4.u32 = height;
    }
  }
  __imp__sub_82099B40(ctx, base);
}
