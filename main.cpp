#include <array>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <print>
#include <thread>
#include <random>
#include <xieite/io/term.hpp>
#include <xieite/math/color3.hpp>
#include <xieite/math/vec2.hpp>

int main() {
	using namespace std::literals;
	
	xieite::term term;
	term.block(false);
	term.echo(false);
	term.canon(false);
	term.signal(false);
	term.proc(false);
	term.cursor_alt(true);
	term.screen_alt(true);
	term.cursor_invis(true);

	static constexpr xieite::vec2<int> game_size = { 20, 20 };

	const auto set_color_at = [&term](xieite::vec2<int> pos, const xieite::color3& color) noexcept -> void {
		term.set_cursor(pos.y + 1, pos.x * 2);
		term.bg(color);
		std::fputs("  ", term.out);
	};

	term.clear_screen();
	for (int x = 0; x < game_size.x; ++x) {
		for (int y = 0; y < game_size.y; ++y) {
			set_color_at({ x, y }, 0x007FFF);
		}
	}
	term.reset_style();
	term.set_cursor(game_size.y + 1, 0);
	std::fputs("Use arrow keys to move, press Q to quit", term.out);

	const bool win = ([&term, set_color_at] -> bool {
		const auto rand_pos = [] static noexcept -> xieite::vec2<int> {
			thread_local auto rng = std::mt19937(std::random_device()());
			return xieite::vec2<int>(
				std::uniform_int_distribution<int>(0, game_size.x - 1)(rng),
				std::uniform_int_distribution<int>(0, game_size.y - 1)(rng)
			);
		};

		xieite::vec2<int> apple = rand_pos();

		xieite::vec2<int> snake_head = rand_pos();
		xieite::vec2<int> snake_direction;
	
		std::array<xieite::vec2<int>, static_cast<std::size_t>(game_size.x * game_size.y)> snake_body = { snake_head };
		std::size_t snake_start = 0;
		std::size_t snake_end = 0;
		std::size_t score = 0;
	
		while (true) {
			term.reset_style();
			term.set_cursor(0, 0);
			std::print(term.out, "Score: {}", score);
			if (score >= snake_body.size()) {
				return true;
			}
	
			set_color_at(snake_head, 0x00FF00);
			snake_head = (snake_head + snake_direction + game_size) % game_size;
			++snake_start %= snake_body.size();
			snake_body[snake_start] = snake_head;
			if (snake_head == apple) {
				++score;
				apple = rand_pos();
			} else {
				set_color_at(snake_body[snake_end], 0x007FFF);
				++snake_end %= snake_body.size();
			}
			set_color_at(apple, 0xFF0000);
			set_color_at(snake_head, 0x00FF00);
	
			for (std::size_t i = 0; i < score; ++i) {
				if (snake_head == snake_body[(snake_end + i) % snake_body.size()]) {
					return false;
				}
			}
	
			std::fflush(stdout);
			std::this_thread::sleep_for(100ms);
			const bool can_move_x = !snake_direction.x || !score;
			const bool can_move_y = !snake_direction.y || !score;
			for (bool input = true; input;) {
				switch (term.read_key()) {
				case xieite::keys::Q:
				case xieite::keys::q:
					return false;
				case xieite::keys::right:
					if (can_move_x) {
						snake_direction = { 1, 0 };
					}
					break;
				case xieite::keys::left:
					if (can_move_x) {
						snake_direction = { -1, 0 };
					}
					break;
				case xieite::keys::down:
					if (can_move_y) {
						snake_direction = { 0, 1 };
					}
					break;
				case xieite::keys::up:
					if (can_move_y) {
						snake_direction = { 0, -1 };
					}
					break;
				default:
					input = false;
				}
			}
		}
	})();

	term.reset_style();
	term.set_cursor(game_size.y + 1, 0);
	term.clear_line();
	if (win) {
		std::fputs("You win! ", term.out);
	}
	std::fputs("Press any key to exit", term.out);
	std::fflush(term.out);
	std::this_thread::sleep_for(500ms);
	while (std::fgetc(term.in) > 0);
	term.block(true);
	std::fgetc(term.in);
}
