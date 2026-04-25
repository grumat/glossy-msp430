#pragma once

#include <type_traits>


/**
 * Returns the underlying integer type of an enum class.
 *
 * @tparam E The enum class type.
 * @param e The enum value.
 * @return The underlying integer type of the enum.
 */
template <typename E>
constexpr auto E2I(E e) noexcept
{
	return static_cast<std::underlying_type_t<E>>(e);
}


/**
 * Returns a reference to the underlying integer type of an enum class.
 * This allows for writable forwarding of enum values.
 *
 * @tparam E The enum class type.
 * @param e The enum value.
 * @return A reference to the underlying integer type of the enum.
 */
template <typename E>
constexpr auto &E2I_r(E &e) noexcept
{
	return reinterpret_cast<std::underlying_type_t<E> &>(e);
}


/**
 * Checks if any of the specified bits are set in the given value.
 *
 * @tparam E The enum class type.
 * @param value The value to check.
 * @param bits The bits to check for.
 * @return true if any of the specified bits are set, false otherwise.
 */
template <typename E>
constexpr bool IsSet(E value, E bits) noexcept
{
	return (E2I(value) & E2I(bits)) != 0;
}

/**
 * Checks if any of the specified bits are set in the given value.
 *
 * @tparam E The enum class type.
 * @param value The value to check.
 * @param bits The bits to check for.
 * @return true if all of the specified bits are set, false otherwise.
 */
template <typename E>
constexpr bool IsSet(E value, E mask, E bits) noexcept
{
	return (E2I(value) & E2I(mask)) == E2I(bits);
}


/**
 * Checks if all of the specified bits are reset in the given value.
 *
 * @tparam E The enum class type.
 * @param value The value to check.
 * @param bits The bits to check for.
 * @return true if all of the specified bits are reset, false otherwise.
 */
template <typename E>
constexpr bool IsReset(E value, E bits) noexcept {
    return (E2I(value) & E2I(bits)) == 0;
}

/**
 * Checks if all of the specified bits are reset in the given value.
 *
 * @tparam E The enum class type.
 * @param value The value to check.
 * @param mask The mask to apply before checking.
 * @param bits The bits to check for.
 * @return true if all of the specified bits are reset, false otherwise.
 */
template <typename E>
constexpr bool IsReset(E value, E mask, E bits) noexcept
{
	return (E2I(value) & E2I(mask)) != E2I(bits);
}


template <typename E>
constexpr bool IsAllSet(E value)
{
	using UnderlyingType = std::underlying_type_t<E>;
	constexpr UnderlyingType all_set = ~UnderlyingType(0);
	return E2I(value) == all_set;
}

