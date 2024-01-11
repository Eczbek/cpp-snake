#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <fcntl.h>
#include <iostream>
#include <limits>
#include <random>
#include <termios.h>
#include <thread>
#include <unistd.h>

enum struct Direction {
	none,
	right,
	left,
	up,
	down
};

struct Position {
	std::size_t x;
	std::size_t y;

	constexpr Position(const std::size_t x, const std::size_t y) noexcept
	: x(x), y(y) {}
};

struct Color {
	std::uint8_t red;
	std::uint8_t green;
	std::uint8_t blue;

	constexpr Color(const std::uint8_t red, const std::uint8_t green, const std::uint8_t blue) noexcept
	: red(red), green(green), blue(blue) {}
};

inline char readCharacter() noexcept {
	char input;
	return (read(STDIN_FILENO, &input, 1) > 0) ? input : '\0';
}

inline Position randomPosition(const Position size) noexcept {
	static std::mt19937 engine = std::mt19937(std::random_device()());
	return Position(std::uniform_int_distribution<std::size_t>(0, size.x - 1)(engine), std::uniform_int_distribution<std::size_t>(0, size.y - 1)(engine));
}

int main() {
	static constexpr Color red = Color(255, 0, 0);
	static constexpr Color lime = Color(127, 255, 0);
	static constexpr Color green = Color(0, 255, 0);
	static constexpr Color azure = Color(0, 127, 255);

	static constexpr Position gameSize  = Position(20, 20);
	Position apple = randomPosition(gameSize);
	std::deque<Position> body = {
		randomPosition(gameSize)
	};
	Direction currentDirection = Direction::none;

	auto canvas = std::vector<std::vector<Color>>(gameSize.x, std::vector<Color>(gameSize.y, azure));

	termios cooked;
	tcgetattr(STDIN_FILENO, &cooked);
	termios raw = cooked;
	raw.c_iflag &= ~static_cast<unsigned int>(ICRNL | IXON);
	raw.c_lflag &= ~static_cast<unsigned int>(ICANON | ECHO | IEXTEN | ISIG);
	raw.c_oflag &= ~static_cast<unsigned int>(OPOST);
	tcsetattr(STDIN_FILENO, TCSANOW, &raw);
	const int blocking = fcntl(STDIN_FILENO, F_GETFL);
	fcntl(STDIN_FILENO, F_SETFL, blocking | O_NONBLOCK);
	std::cout << "\x1b[s\x1b[?47h\x1b[?25l";

	bool gameOver = false;
	while (!gameOver) {
		Position head = body[0];
		switch (currentDirection) {
		case Direction::right:
			head.x = (head.x + 1) % gameSize.x;
			break;
		case Direction::left:
			head.x = (head.x + gameSize.x - 1) % gameSize.x;
			break;
		case Direction::up:
			head.y = (head.y + 1) % gameSize.y;
			break;
		case Direction::down:
			head.y = (head.y + gameSize.y - 1) % gameSize.y;
			break;
		default:
			break;
		}

		if ((head.x == apple.x) && (head.y == apple.y)) {
			canvas[apple.x][apple.y] = azure;
			apple = randomPosition(gameSize);
		} else {
			canvas[body.back().x][body.back().y] = azure;
			body.pop_back();
		}
		bool primary = false;
		for (const Position part : body) {
			if ((part.x == head.x) && (part.y == head.y)) {
				gameOver = true;
				break;
			}
			canvas[part.x][part.y] = primary ? green : lime;
			primary = !primary;
		}
		if (gameOver) {
			break;
		}
		body.push_front(head);
		canvas[head.x][head.y] = green;
		canvas[apple.x][apple.y] = red;

		const std::size_t bodySize = body.size();
		std::cout << "\x1b[HScore: " << (bodySize - 1) << "\n\r";
		for (std::size_t y = gameSize.y; y--;) {
			for (std::size_t x = 0; x < gameSize.x; ++x) {
				std::cout << "\x1b[48;2;"
					<< static_cast<int>(canvas[x][y].red) << ';'
					<< static_cast<int>(canvas[x][y].green) << ';'
					<< static_cast<int>(canvas[x][y].blue) << "m  ";
			}
			std::cout << "\x1b[0m\n\r";
		}
		std::cout << "Use arrow keys to move, press q to quit";
		std::cout.flush();
		if (bodySize == (gameSize.x * gameSize.y)) {
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		Direction newDirection = currentDirection;
		while (true) {
			const char input = readCharacter();
			if (static_cast<char>(std::tolower(static_cast<unsigned char>(input))) == 'q') {
				gameOver = true;
			}
			if (!input || gameOver) {
				break;
			}
			if ((input == '\x1b') && (readCharacter() == '[')) {
				switch (readCharacter()) {
					case 'A':
						if ((currentDirection != Direction::down) || (bodySize < 2)) {
							newDirection = Direction::up;
						}
						break;
					case 'B':
						if ((currentDirection != Direction::up) || (bodySize < 2)) {
							newDirection = Direction::down;
						}
						break;
					case 'C':
						if ((currentDirection != Direction::left) || (bodySize < 2)) {
							newDirection = Direction::right;
						}
						break;
					case 'D':
						if ((currentDirection != Direction::right) || (bodySize < 2)) {
							newDirection = Direction::left;
						}
				}
			}
		}
		currentDirection = newDirection;
	}

	std::cout << "\x1b[2K\x1b[0G";
	if (!gameOver) {
		std::cout << "You win! ";
	}
	std::cout << "Press any key to exit";
	std::cout.flush();
	std::this_thread::sleep_for(std::chrono::seconds(1));
	while (readCharacter());
	fcntl(STDIN_FILENO, F_SETFL, blocking);
	std::cin.get();

	tcsetattr(STDIN_FILENO, TCSANOW, &cooked);
	std::cout << "\x1b[?25h\x1b[?47l\x1b[u";
}
