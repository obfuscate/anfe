#pragma once

#if defined(DISABLE_ASSERTS)
	#define ENGINE_ASSERT(expr, message)
#else
	#define ENGINE_ASSERT(expr, message) assert(expr && message)
#endif

#define ENGINE_FAIL(message) ENGINE_ASSERT(false, message)
