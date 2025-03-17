#pragma once

//-- Define bitwise operators for an enum class, allowing usage as bitmasks.
#define DEFINE_ENUM_CLASS_BITWISE_OPERATORS(Enum)                   \
	inline constexpr Enum operator|(Enum lhs, Enum rhs) {           \
		static_assert(std::is_enum_v<Enum>, "Your type isn't enum!"); \
		return static_cast<Enum>(                                   \
			static_cast<std::underlying_type_t<Enum>>(lhs) |        \
			static_cast<std::underlying_type_t<Enum>>(rhs));        \
	}                                                               \
	inline constexpr Enum operator&(Enum lhs, Enum rhs) {           \
		static_assert(std::is_enum_v<Enum>, "Your type isn't enum!"); \
		return static_cast<Enum>(                                   \
			static_cast<std::underlying_type_t<Enum>>(lhs) &        \
			static_cast<std::underlying_type_t<Enum>>(rhs));        \
	}                                                               \
	inline constexpr Enum operator^(Enum lhs, Enum rhs) {           \
		static_assert(std::is_enum_v<Enum>, "Your type isn't enum!"); \
		return static_cast<Enum>(                                   \
			static_cast<std::underlying_type_t<Enum>>(lhs) ^        \
			static_cast<std::underlying_type_t<Enum>>(rhs));        \
	}                                                               \
	inline constexpr Enum operator~(Enum E) {                       \
		static_assert(std::is_enum_v<Enum>, "Your type isn't enum!"); \
		return static_cast<Enum>(                                   \
			~static_cast<std::underlying_type_t<Enum>>(E));         \
	}                                                               \
	inline Enum& operator|=(Enum& lhs, Enum rhs) {                  \
		static_assert(std::is_enum_v<Enum>, "Your type isn't enum!"); \
		return lhs = static_cast<Enum>(                             \
				static_cast<std::underlying_type_t<Enum>>(lhs) | \
				static_cast<std::underlying_type_t<Enum>>(rhs)); \
	}                                                               \
	inline Enum& operator&=(Enum& lhs, Enum rhs) {                  \
		static_assert(std::is_enum_v<Enum>, "Your type isn't enum!"); \
		return lhs = static_cast<Enum>(                             \
					static_cast<std::underlying_type_t<Enum>>(lhs) & \
					static_cast<std::underlying_type_t<Enum>>(rhs)); \
	}                                                               \
	inline Enum& operator^=(Enum& lhs, Enum rhs) {                  \
		static_assert(std::is_enum_v<Enum>, "Your type isn't enum!"); \
		return lhs = static_cast<Enum>(                             \
					static_cast<std::underlying_type_t<Enum>>(lhs) ^ \
					static_cast<std::underlying_type_t<Enum>>(rhs)); \
	}                                                                \
	inline bool hasFlag(Enum lhs, Enum rhs) {                            \
		return static_cast<std::underlying_type_t<Enum>>(lhs & rhs) != 0; \
	}
