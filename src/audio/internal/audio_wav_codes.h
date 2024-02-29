#pragma once

#include "core/types.h"

namespace Audio
{
#ifdef _XBOX //Big-Endian
constexpr u32 char_code_RIFF = 'RIFF';
constexpr u32 char_code_DATA = 'data';
constexpr u32 char_code_FMT  = 'fmt ';
constexpr u32 char_code_WAVE = 'WAVE';
constexpr u32 char_code_XWMA = 'XWMA';
constexpr u32 char_code_DPDS = 'dpds';
#endif

#ifndef _XBOX //Little-Endian
constexpr u32 char_code_RIFF = 'FFIR';
constexpr u32 char_code_DATA = 'atad';
constexpr u32 char_code_FMT  = ' tmf';
constexpr u32 char_code_WAVE = 'EVAW';
constexpr u32 char_code_XWMA = 'AMWX';
constexpr u32 char_code_DPDS = 'sdpd';
#endif

} // namespace Audio
