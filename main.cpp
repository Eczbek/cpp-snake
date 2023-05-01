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

struct Position {
	int x;
	int y;
};

struct Color {
	std::uint8_t red;
	std::uint8_t green;
	std::uint8_t blue;
};

char readCharacter() noexcept {
	char input;
	return (read(STDIN_FILENO, &input, 1) > 0) ? input : 0;
}

int main() {
	const Position gameSize {
		20,
		20
	};

	std::default_random_engine randomEngine(std::random_device {}());
	std::uniform_int_distribution<int> randomDistributionX(0, gameSize.x - 1);
	std::uniform_int_distribution<int> randomDistributionY(0, gameSize.y - 1);

	Position apple {
		randomDistributionX(randomEngine),
		randomDistributionY(randomEngine)
	};

	std::deque<Position> body {
		{
			randomDistributionX(randomEngine),
			randomDistributionY(randomEngine)
		}
	};

	Position currentDirection {
		0,
		0
	};

	const Color red {
		255,
		0,
		0
	};
	const Color lime {
		127,
		255,
		0
	};
	const Color green {
		0,
		255,
		0
	};
	const Color azure {
		0,
		127,
		255
	};
	std::vector<std::vector<Color>> canvas(gameSize.x, std::vector<Color>(gameSize.y, azure));

	termios cooked;
	tcgetattr(STDIN_FILENO, &cooked);
	termios raw = cooked;
	raw.c_iflag &= ~(ICRNL | IXON);
	raw.c_lflag &= ~(ICANON | ECHO | IEXTEN | ISIG);
	raw.c_oflag &= ~(OPOST);
	tcsetattr(STDIN_FILENO, TCSANOW, &raw);
	const int blocking = fcntl(STDIN_FILENO, F_GETFL);
	fcntl(STDIN_FILENO, F_SETFL, blocking | O_NONBLOCK);
	std::cout << "\x1b[?47h\x1b[?25l";

	bool gameOver = false;
	while (!gameOver) {
		const Position head {
			(body[0].x + currentDirection.x + gameSize.x) % gameSize.x,
			(body[0].y + currentDirection.y + gameSize.y) % gameSize.y
		};

		if ((head.x == apple.x) && (head.y == apple.y)) {
			canvas[apple.x][apple.y] = azure;
			apple.x = randomDistributionX(randomEngine);
			apple.y = randomDistributionY(randomEngine);
		} else {
			canvas[body.back().x][body.back().y] = azure;
			body.pop_back();
		}
		for (const Position part : body) {
			if ((part.x == head.x) && (part.y == head.y)) {
				gameOver = true;
				break;
			}
			canvas[part.x][part.y] = lime;
		}
		if (gameOver) {
			break;
		}
		body.push_front(head);
		canvas[head.x][head.y] = green;
		canvas[apple.x][apple.y] = red;

		const std::size_t bodySize = body.size();
		std::cout << "\x1b[2J\x1b[HScore: " << (bodySize - 1) << "\n\r";
		for (int y = gameSize.y; y--;) {
			for (int x = 0; x < gameSize.x; ++x) {
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

		Position newDirection = currentDirection;
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
						if (!currentDirection.y || (bodySize < 2)) {
							newDirection.x = 0;
							newDirection.y = 1;
						}
						break;
					case 'B':
						if (!currentDirection.y || (bodySize < 2)) {
							newDirection.x = 0;
							newDirection.y = -1;
						}
						break;
					case 'C':
						if (!currentDirection.x || (bodySize < 2)) {
							newDirection.x = 1;
							newDirection.y = 0;
						}
						break;
					case 'D':
						if (!currentDirection.x || (bodySize < 2)) {
							newDirection.x = -1;
							newDirection.y = 0;
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
	std::cout << "\x1b[?25h\x1b[?47l";
}
