// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <cheri.hh>
#include <compartment.h>
#include <debug.hh>
#include <platform-entropy.hh>
#include <platform-gpio.hh>
#include <thread.h>
#include <vector>

#include "../../libraries/lcd.hh"
#include "cherry_bitmap.h"

using Debug = ConditionalDebug<true, "Flying_game">;
using namespace sonata::lcd;
using namespace CHERI;

// Control game speed
static constexpr uint32_t MillisecondsPerFrame = 400;
// Small wait between games to avoid accidentally starting the next
static constexpr uint32_t StartMenuWaitMilliseconds = 400;

// Snake speeds up as it gets longer if enabled
static constexpr bool SpeedScalingEnabled = true;
// If enabled, all joystick motions start the game (not just a press)
static constexpr bool StartOnAnyInput = true;
// If enabled, displays a cherry bitmap for the fruit instead of a
// green square at 10x10 and 5x5 tile sizes.
static constexpr bool UseCherryImage = true;

// Change colour of game elements
static constexpr Color BackgroundColor = Color::Black,
                       BorderColor     = Color::White,
                       ForegroundColor = Color::White, SnakeColor = Color::Red;
// Change the size of game elements (will automatically fit display)
static constexpr Size TileSize = {10, 10}, TileSpacing = {2, 2},
                      BorderSize = {4, 3};

typedef struct Position
{
	int32_t x;
	int32_t y;
} Position;

enum class Direction
{
	UP    = 0,
	DOWN  = 1,
};

// The allocator rounds heap allocations to a multiple of 8 bytes.
// Tile is a uint64_t so the allocated game space array is guaranteed to be a
// multiple of 8 bytes, as otherwise some out of bounds accesses will not be
// appropriately caught.
enum class Tile : uint64_t
{
    PLAYER,
	EMPTY,
	WALL
};

/**
 * @brief Converts a given size_t to an equivalent string representing its
 * unsigned base 10 representation in the given buffer.
 *
 * @param buffer The buffer/string to write the converted number to. Will
 * terminate at the end of the string.
 * @param num The size_t number to convert
 */
void size_t_to_str_base10(char *buffer, size_t num)
{
	// Parse the digits using repeated remainders mod 10
	ptrdiff_t endIdx = 0;
	if (num == 0)
	{
		buffer[endIdx++] = '0';
	}
	while (num != 0)
	{
		int remainder    = num % 10;
		buffer[endIdx++] = '0' + remainder;
		num /= 10;
	}
	buffer[endIdx--] = '\0';

	// Reverse the generated string
	ptrdiff_t startIdx = 0;
	while (startIdx < endIdx)
	{
		char swap          = buffer[startIdx];
		buffer[startIdx++] = buffer[endIdx];
		buffer[endIdx--]   = swap;
	}
}

/**
 * A game of snake for Sonata, using Cheri capability violations to detect when
 * the snake reaches the game's boundaries.
 */
class FlyingGame
{
	private:
	bool   isFirstGame = true;
	Tile **gameSpace   = nullptr;

	EntropySource prng{};

	Position heroPosition;
	Size                  gameSize, gamePadding;

	/**
	 * @brief Calculate game size and padding information from defined constants
	 * and display info.
	 *
	 * @param lcd The LCD that will be drawn to.
	 */
	void initialise_game_size(SonataLcd *lcd)
	{
		Rect screen =
		  Rect::from_point_and_size(Point::ORIGIN, lcd->resolution());
		Size displaySize = {screen.right - screen.left - BorderSize.width * 2,
		                    screen.bottom - screen.top - BorderSize.height * 2};
		Size spacedTileSize = {TileSize.width + TileSpacing.width,
		                       TileSize.height + TileSpacing.height};
		gameSize            = {displaySize.width / spacedTileSize.width,
                    displaySize.height / spacedTileSize.height};
		gamePadding         = {
          displaySize.width % spacedTileSize.width + TileSpacing.width,
          displaySize.height % spacedTileSize.height + TileSpacing.height};
		gamePadding = {
		  Point::ORIGIN.x + BorderSize.width + gamePadding.width / 2,
		  Point::ORIGIN.y + BorderSize.height + gamePadding.height / 2};
		Debug::log("Calculated game size based on settings: {}x{}",
		           static_cast<int>(gameSize.width),
		           static_cast<int>(gameSize.height));
	};

	/**
	 * @brief Displays the "start game" menu, waiting for an input and
	 * initialising a random seed based on the first user input.
	 *
	 * @param gpio The Sonata GPIO driver to use for I/O operations.
	 * @param lcd The LCD that will be drawn to.
	 */
	void wait_for_start(volatile SonataGpioBoard *gpio, SonataLcd *lcd)
	{
		Size  displaySize = lcd->resolution();
		Point centre      = {displaySize.width / 2, displaySize.height / 2};
		lcd->clean(BackgroundColor);

		// Text sizes are hard-coded for now as `draw_str` always uses 16pt font
		if (isFirstGame)
		{
			lcd->draw_str({centre.x - 60, centre.y},
			              StartOnAnyInput ? "Move the joystick to start"
			                              : "Press the joystick to start",
			              BackgroundColor,
			              ForegroundColor);
		}
		else
		{
			lcd->draw_str({centre.x - 25, centre.y - 15},
			              "Game over!",
			              BackgroundColor,
			              ForegroundColor);
			// Manually convert and concatenate score string due to no
			// implementation of existing utils
			lcd->draw_str({centre.x - 65, centre.y + 5},
			              StartOnAnyInput
			                ? "Move the joystick to play again..."
			                : "Press the joystick to play again...",
			              BackgroundColor,
			              ForegroundColor);
			// Wait for a short time to avoid instantly starting the next game
			// due to accidental user input
			thread_millisecond_wait(StartMenuWaitMilliseconds);
		}

		// Busy-wait for a valid joystick input
		SonataJoystick joystickInp, noInput = static_cast<SonataJoystick>(0x0);
		bool           waitingForInput = true;
		while (waitingForInput)
		{
			thread_millisecond_wait(50);
			joystickInp = gpio->read_joystick();
			if (!StartOnAnyInput && joystickInp == SonataJoystick::Pressed)
			{
				waitingForInput = false;
			}
			else if (StartOnAnyInput && joystickInp != noInput)
			{
				waitingForInput = false;
			}
		};
		Debug::log("Input detected. Game starting...");

		// Initialise Pseudo RNG based on cycle counter at time of first input
		prng.reseed();
	};

	/**
	 * @brief Compares the relevant bits of the input joystick state and the
	 * given direction to determine if the joystick is held in that direction or
	 * not.
	 *
	 * @param joystick The joystick GPIO input
	 * @param direction The joystick direction to test for
	 * @return true if the joystick is held in that direction, false otherwise.
	 */
	bool joystick_in_direction(SonataJoystick joystick,
	                           SonataJoystick direction)
	{
		return (static_cast<uint16_t>(joystick) &
		        static_cast<uint16_t>(direction)) > 0;
	};

	/**
	 * @brief Reads the GPIO output to find the current joystick output, and
	 * translates it into a relevant direction. Returns the previous direction
	 * if no current output.
	 *
	 * @param gpio The Sonata GPIO driver to use for I/O operations.
	 */
	Direction read_joystick(volatile SonataGpioBoard *gpio)
	{
		SonataJoystick joystickState = gpio->read_joystick();
		// The joystick can be in many possible directions - we check directions
		// in order relative to the current direction so that input prioritises
		// turning left/right over staying in the same direction. This avoids
		// the issue of input priority for diagonal joystick inputs, and feels
		// smoother to play.
		Direction directions[2] = {
		  Direction::UP, Direction::DOWN};
		SonataJoystick joystickStates[2] = {SonataJoystick::Up,
		                                    SonataJoystick::Down};

		uint8_t base;
		for (base = 0; base < 4; base++)
		{
			if (currentDirection == directions[base])
			{
				break;
			}
		}

		for (uint8_t offset = 1; offset <= 4; offset++)
		{
			if (offset == 2 && snakePositions.size() != 1)
			{
				continue; // Disallow moving in the opposite direction
			}
			uint8_t idx = (base + offset) % 4;
			if (joystick_in_direction(joystickState, joystickStates[idx]))
			{
				return directions[idx];
			}
		}
		return lastSeenDirection;
	};

	/**
	 * @brief Busy waits for a given amount of time, constantly polling for any
	 * joystick input and recording it to avoid inputs being eaten between
	 * frames.
	 *
	 * @param milliseconds The time to wait for in milliseconds.
	 * @param gpio The Sonata GPIO driver to use for I/O operations.
	 */
	void wait_with_input(uint32_t milliseconds, volatile SonataGpioBoard *gpio)
	{
		const uint32_t CyclesPerMillisecond = CPU_TIMER_HZ / 1000;
		const uint32_t Cycles  = milliseconds * CyclesPerMillisecond;
		const uint64_t Start   = rdcycle64();
		uint64_t       end     = Start + Cycles;
		uint64_t       current = Start;
		while (end > current)
		{
			lastSeenDirection = read_joystick(gpio);
			current           = rdcycle64();
		}
	};

	/**
	 * @brief Initialises information required for starting the game, including
	 * the snake and fruit positions.
	 */
	void initialise_game()
	{

	};

	/**
	 * @brief Draws the background (including the border) for the main game.
	 *
	 * @param lcd The LCD that will be drawn to.
	 */
	void draw_background(SonataLcd *lcd)
	{
		Size lcdSize = lcd->resolution();
		lcd->clean(BorderColor);
		lcd->fill_rect({BorderSize.width,
		                BorderSize.height,
		                lcdSize.width - BorderSize.width,
		                lcdSize.height - BorderSize.height},
		               BackgroundColor);
	}

	/**
	 * @brief Get the rectangle bounding box for the game tile at the given
	 * position, to use for drawing to the display.
	 *
	 * @param position The integer tile position (x, y) to draw at.
	 */
	Rect get_tile_rect(Position position)
	{
		Size spacedTileSize = {TileSize.width + TileSpacing.width,
		                       TileSize.height + TileSpacing.height};
		return Rect::from_point_and_size(
		  {gamePadding.width + position.x * spacedTileSize.width,
		   gamePadding.height + position.y * spacedTileSize.height},
		  TileSize);
	}

	/**
	 * @brief Draw a filled rectangle the size of one game tile at the specified
	 * position and colour.
	 *
	 * @param lcd The LCD that will be drawn to.
	 * @param position The integer tile position (x, y) to draw at.
	 * @param color The colour to fill the drawn tile.
	 */
	void draw_tile(SonataLcd *lcd, Position position, Color color)
	{
		Rect tileRect = get_tile_rect(position);
		lcd->fill_rect(tileRect, color);
	}

	/**
	 * @brief Checks whether the snake is colliding with anything (i.e. itself)
	 * in the game's space. Also responsible for causing the Out of Bounds
	 * accesses when hitting the game's boundary which trigger CHERI violations
	 * for scoring.
	 *
	 * @return true if the snake is colliding, false otherwise.
	 *
	 * @note The attribute nextPosition is used to pass along the snake's
	 * position instead of using a traditional argument to allow us to use the
	 * CHERI capability mechanisms for scoring. By keeping a simple function and
	 * ignoring caller/callee-saves responsibilities we can just change the PCC
	 * address to a function of the same kind that returns True to continue
	 * execution after an out of bounds access occurs.
	 */
	[[gnu::noinline]] bool check_if_colliding()
	{
		if (gameSpace[nextPosition.y][nextPosition.x] == Tile::WALL)
		{
			// Cause an out of bounds access on purpose when the snake collides
			// with itself so we can use CHERI violations for game scoring
			return gameSpace[gameSize.height][gameSize.width] == Tile::WALL;
		}
		return false;
	};

	/**
	 * @brief Updates the game's state by a frame, advancing the snake forward
	 * by 1 step in the input/previous direction, and handling collision and
	 * fruit-eating logic. Updates the display by drawing only relevant/new
	 * information, rather than drawing everything each frame.
	 *
	 * @param lcd The LCD that will be drawn to.
	 * @param position The integer tile position (x, y) to draw at.
	 * @return true if the game is still active, false if the game is over.
	 */
	bool update_game_state(volatile SonataGpioBoard *gpio, SonataLcd *lcd)
	{
		currentDirection = read_joystick(gpio);

		int8_t dx, dy;
		switch (currentDirection)
		{
			case Direction::UP:
				dx = -1;
				dy = 0;
				break;
			case Direction::DOWN:
				dx = 1;
				dy = 0;
				break;
		};

		Position currentPosition = snakePositions.back();
		nextPosition = {currentPosition.x + dx, currentPosition.y + dy};
		if (check_if_colliding())
		{
			Debug::log("Snake collided with something - game over.");
			return false;
		}
		snakePositions.push_back(nextPosition);
		gameSpace[nextPosition.y][nextPosition.x] = Tile::SNAKE;
		draw_tile(lcd, nextPosition, SnakeColor);
		return true;
	}

	/**
	 * @brief Runs the main game loop, updating the snake's movement and drawing
	 * new information to the display, and regulates update/frame timing.
	 *
	 * @param gpio The Sonata GPIO driver to use for I/O operations
	 * @param lcd The LCD that will be drawn to.
	 */
	void main_game_loop(volatile SonataGpioBoard *gpio, SonataLcd *lcd)
	{
		const uint32_t CyclesPerMillisecond = CPU_TIMER_HZ / 1000;
		uint64_t       currentTime          = rdcycle64();

		// Draw initial information (to be drawn on top of, rather than
		// re-drawing each frame)
		draw_background(lcd);
		draw_tile(lcd, snakePositions.front(), SnakeColor);
		draw_cherry(lcd, fruitPosition);

		bool gameStillActive = true;
		while (gameStillActive)
		{
			uint64_t nextTime = rdcycle64();
			uint64_t elapsedTimeMilliseconds =
			  (nextTime - currentTime) / CyclesPerMillisecond;
			uint64_t frameTime = MillisecondsPerFrame;
			if (SpeedScalingEnabled)
			{
				// Scale the game's speed in an inverse relationship between
				// MILLISECONDS_PER_FRAME and MILLISECONDS_PER_FRAME / 2
				frameTime /= 2;
				frameTime += (frameTime / snakePositions.size());
			}
			if (elapsedTimeMilliseconds < frameTime)
			{
				uint64_t remainingTime = frameTime - elapsedTimeMilliseconds;
				wait_with_input(remainingTime, gpio);
			}
			currentTime = rdcycle64();

			gameStillActive = update_game_state(gpio, lcd);
		}
	};

	/**
	 * @brief Cleans up the non-contiguous 2D game space array used for
	 * collision checking.
	 */
	void free_game_space()
	{
		for (size_t y = 0; y < gameSize.height; y++)
		{
			delete[] gameSpace[y];
		}
		delete[] gameSpace;
	}

	public:
	/**
	 * @brief Plays a game of snake using the stored state as settings.
	 *
	 * @param gpio The Sonata GPIO driver to use for I/O operations.
	 * @param lcd The LCD that will be drawn to.
	 */
	void run_game(volatile SonataGpioBoard *gpio, SonataLcd *lcd)
	{
		wait_for_start(gpio, lcd);
		initialise_game();
		main_game_loop(gpio, lcd);
		free_game_space();
		isFirstGame = false;
	};

	/**
	 * @brief Constructor for a SnakeGame.
	 *
	 * @param lcd The LCD that the game will be drawn to.
	 */
	SnakeGame(SonataLcd *lcd)
	{
		initialise_game_size(lcd);
	};

	~SnakeGame()
	{
		if (gameSpace != nullptr)
		{
			free_game_space();
		}
	};
};

/**
 * @brief A minimal function used to replace SnakeGame::check_if_colliding for
 * use in error recovery, letting us utilise RISC-V's capability violations with
 * a single compartment as a scoring mechanism.
 *
 * @return True, always.
 */
[[gnu::noinline]] bool return_from_handled_error()
{
	return true;
}

/**
 * @brief Handles any CHERI Capability Violation errors. If the error was a
 * Bounds or Tag violation it assumes it is because of the incorrect memory
 * access in SnakeGame::check_if_colliding and therefore it recovers the program
 * and ends the game. Otherwise, this force unwinds and ends the program.
 */
extern "C" ErrorRecoveryBehaviour
compartment_error_handler(ErrorState *frame, size_t mcause, size_t mtval)
{
	auto [exceptionCode, registerNumber] = extract_cheri_mtval(mtval);
	if (exceptionCode == CauseCode::BoundsViolation ||
	    exceptionCode == CauseCode::TagViolation)
	{
		// If an explicit out of bounds access occurs, or bounds are made
		// invalid by some negative array access, we **assume** that this was
		// caused by the SnakeGame::check_if_colliding function and that the
		// snake has hit the boundary of the game and so the game should end.
		frame->pcc = (void *)(&return_from_handled_error);
		return ErrorRecoveryBehaviour::InstallContext;
	}

	Debug::log(
	  "Unexpected CHERI Capability violation encountered. Stopping...");
	return ErrorRecoveryBehaviour::ForceUnwind;
}

// Thread entry point.
void __cheri_compartment("flying_game") flying_game()
{
	auto gpio = MMIO_CAPABILITY(SonataGpioBoard, gpio_board);
	auto lcd  = SonataLcd();
	Debug::log("Detected display resolution: {} {}",
	           static_cast<int>(lcd.resolution().width),
	           static_cast<int>(lcd.resolution().height));
	SnakeGame game = SnakeGame(&lcd);
	while (true)
	{
		game.run_game(gpio, &lcd);
	}
}
