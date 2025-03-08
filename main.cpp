#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <print>
#include <random>
#include <termios.h>
#include <thread>
#include <unistd.h>

using namespace std::literals;

struct pos_t {
	std::size_t x;
	std::size_t y;

	[[nodiscard]] friend bool operator==(const pos_t&, const pos_t&) = default;
};

constexpr auto game_size = pos_t(20, 20);
constexpr std::size_t snake_max = game_size.x * game_size.y;

pos_t rand_pos() noexcept {
	thread_local auto rng = std::mt19937(std::random_device()());
	return pos_t(
		std::uniform_int_distribution<std::size_t>(0, game_size.x - 1)(rng),
		std::uniform_int_distribution<std::size_t>(0, game_size.y - 1)(rng)
	);
}

struct color_t {
	std::uint8_t r;
	std::uint8_t g;
	std::uint8_t b;
};

constexpr auto red = color_t(255, 0, 0);
constexpr auto green = color_t(0, 255, 0);
constexpr auto azure = color_t(0, 127, 255);

void set_color_at(color_t color, pos_t pos) noexcept {
	std::print("\x1B[{};{}H\x1B[48;2;{};{};{}m  ", pos.y + 2, pos.x * 2 + 1, color.r, color.g, color.b);
}

int main() {
	::termios cooked_mode;
	::tcgetattr(STDIN_FILENO, &cooked_mode);
	::termios raw_mode = cooked_mode;
	raw_mode.c_iflag &= ~static_cast<::tcflag_t>(ICRNL | IXON);
	raw_mode.c_lflag &= ~static_cast<::tcflag_t>(ICANON | ECHO | IEXTEN | ISIG);
	raw_mode.c_oflag &= ~static_cast<::tcflag_t>(OPOST);
	::tcsetattr(STDIN_FILENO, TCSANOW, &raw_mode);
	const int block_mode = ::fcntl(STDIN_FILENO, F_GETFL);
	::fcntl(STDIN_FILENO, F_SETFL, block_mode | O_NONBLOCK);
	std::fputs("\x1B[s\x1B[?47h\x1B[?25l\x1B[2J", stdout);

	for (size_t x = 0; x < game_size.x; ++x) {
		for (size_t y = 0; y < game_size.y; ++y) {
			set_color_at(azure, pos_t(x, y));
		}
	}
	std::print("\x1B[0m\x1B[{}HUse arrow keys to move, press Q to quit", game_size.y + 2);

	const bool win = ([] static -> bool {
		pos_t apple = rand_pos();
	
		pos_t snake_body[snake_max];
		pos_t snake_head = rand_pos();
		std::size_t snake_start = 0;
		std::size_t snake_end = 0;
		std::size_t score = 0;
		auto snake_direction = pos_t(0, 0);
	
		while (true) {
			std::print("\x1B[0m\x1B[HScore: {}", score);
			if (score >= snake_max) {
				return true;
			}
	
			set_color_at(green, snake_head);
			snake_head = pos_t((snake_head.x + snake_direction.x) % game_size.x, (snake_head.y + snake_direction.y) % game_size.y);
			snake_body[snake_start = (snake_start + 1) % snake_max] = snake_head;
			if (snake_head == apple) {
				++score;
				apple = rand_pos();
			} else {
				const pos_t snake_tail = snake_body[snake_end++];
				snake_end %= snake_max;
				set_color_at(azure, snake_tail);
			}
			set_color_at(red, apple);
			set_color_at(green, snake_head);
	
			for (std::size_t i = 0; i < score; ++i) {
				const pos_t snake_part = snake_body[(snake_end + i) % snake_max];
				if (snake_head == snake_part) {
					return false;
				}
			}
	
			std::fflush(stdout);
			std::this_thread::sleep_for(100ms);
	
			const bool can_turn_x = !snake_direction.x || (score < 1);
			const bool can_turn_y = !snake_direction.y || (score < 1);
			while (true) {
				const int input = std::getchar();
				if (std::toupper(input) == 'Q') {
					return false;
				}
				if (input < 1) {
					break;
				}
				if ((input == '\x1B') && (std::getchar() == '[')) {
					switch (std::getchar()) {
					case 'A':
						if (can_turn_y) {
							snake_direction = pos_t(0, game_size.y - 1);
						}
						break;
					case 'B':
						if (can_turn_y) {
							snake_direction = pos_t(0, 1);
						}
						break;
					case 'C':
						if (can_turn_x) {
							snake_direction = pos_t(1, 0);
						}
						break;
					case 'D':
						if (can_turn_x) {
							snake_direction = pos_t(game_size.x - 1, 0);
						}
					}
				}
			}
		}
	})();

	std::print("\x1B[0m\x1B[{}H\x1B[2K", game_size.y + 2);
	if (win) {
		std::fputs("You win! ", stdout);
	}
	std::fputs("Press any key to exit", stdout);
	std::fflush(stdout);
	std::this_thread::sleep_for(500ms);
	while (std::getchar() > 0);
	::fcntl(STDIN_FILENO, F_SETFL, block_mode);
	std::getchar();
	::tcsetattr(STDIN_FILENO, TCSANOW, &cooked_mode);
	std::fputs("\x1B[?25h\x1B[?47l\x1B[u", stdout);
}
