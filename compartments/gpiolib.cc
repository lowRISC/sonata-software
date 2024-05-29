// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include "gpiolib.hh"
#include <platform-gpio.hh>
#include <timeout.hh>
#include <token.h>

/// The number of LEDs available.
static constexpr uint8_t NumLeds = 8;
/// A mask of the LEDs that have been aquired.
static uint8_t ledTaken = 0x0;

/**
 * A handle showing ownership of the LED at the held index.
 */
struct LedHandle
{
	uint8_t index;
};

/**
 * Get a token key for use sealing LedHandles.
 */
static auto key()
{
	static auto key = token_key_new();
	return key;
}

/**
 * Get a pointer to the GPIO MMIO region.
 */
static auto gpio()
{
	static auto gpio = MMIO_CAPABILITY(SonataGPIO, gpio);
	return gpio;
}

/**
 * Aquire a handle to the LED at the given index.
 */
auto aquire_led(uint8_t index) -> std::optional<LedHandle *>
{
	if (NumLeds <= index)
		return {};

	// We should use some sort of lock here...
	const uint8_t LedBit = 1 << index;
	if (0 != (ledTaken & LedBit))
		return {};
	ledTaken |= LedBit;

	// Allocate an LedHandle to the heap and receive a sealed and unsealed key
	// pointing to this allocation
	auto [unsealed, sealed] =
	  blocking_forever<token_allocate<LedHandle>>(MALLOC_CAPABILITY, key());
	if (sealed == nullptr)
	{
		return {};
	}
	unsealed->index = index;
	return sealed.get();
}

/**
 * Unseal a handle with our led token key.
 */
static auto unseal_handle(LedHandle *handle) -> std::optional<LedHandle *>
{
	const auto unsealed = token_unseal(key(), Sealed<LedHandle>{handle});
	if (nullptr == unsealed)
	{
		return {};
	}
	return unsealed;
}

/**
 * Toggle the LED of the given handle.
 */
bool toggle_led(LedHandle *handle)
{
	if (auto unsealedHandle = unseal_handle(handle))
	{
		gpio()->led_toggle(unsealedHandle.value()->index);
		return true;
	}
	else
	{
		return false;
	}
};

/**
 * Reliquish ownership of the LED of the given handle.
 */
void release_led(LedHandle *handle)
{
	if (auto unsealedHandle = unseal_handle(handle))
	{
		// We should use some sort of lock here...
		const uint8_t LedBit = 1 << unsealedHandle.value()->index;
		ledTaken &= ~LedBit;
	}
	// The allocator checks validity before destroying so we don't have to.
	token_obj_destroy(MALLOC_CAPABILITY, key(), reinterpret_cast<SObj>(handle));
}
