#pragma once
#include <libassert/assert.hpp>

#if defined(DISABLE_ASSERTS)
	#define ENGINE_ASSERT(expr, message)
#else
	//-- ToDo: Show a message in the message box.
	#define ENGINE_ASSERT ASSERT
#endif

#define ENGINE_ASSERT_DEBUG DEBUG_ASSERT

#define ENGINE_FAIL(message) ENGINE_ASSERT(false, message)
