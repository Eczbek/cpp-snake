#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <fcntl.h>
#include <iostream>
#include <limits>
#include <random>
#include <string>
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

	std::vector<std::vector<Color>> canvas;
	for (int x = 0; x < gameSize.x; ++x) {
		std::vector<Color>& column = canvas.emplace_back();
		for (int y = 0; y < gameSize.y; ++y)
			column.emplace_back();
	}

	bool running = true;

	termios cooked;
	tcgetattr(STDIN_FILENO, &cooked);
	termios raw = cooked;
	cfmakeraw(&raw);
	raw.c_lflag &= ~(ICANON);
	tcsetattr(STDIN_FILENO, TCSANOW, &raw);
	const int blocking = fcntl(STDIN_FILENO, F_GETFL);
	fcntl(STDIN_FILENO, F_SETFL, blocking | O_NONBLOCK);
	std::cout << "\x1b[?47h\x1b[?25l";

	while (running) {
		const Position head {
			(body[0].x + currentDirection.x + gameSize.x) % gameSize.x,
			(body[0].y + currentDirection.y + gameSize.y) % gameSize.y
		};

		if ((head.x == apple.x) && (head.y == apple.y)) {
			apple.x = randomDistributionX(randomEngine);
			apple.y = randomDistributionY(randomEngine);
		} else
			body.pop_back();
		
		for (const Position part : body)
			if ((head.x == part.x) && (head.y == part.y)) {
				running = false;
				break;
			}

		if (!running)
			break;

		body.push_front(head);

		for (int x = 0; x < gameSize.x; ++x)
			for (int y = 0; y < gameSize.y; ++y)
				canvas[x][y] = { 0, 127, 255 };
		for (const Position part : body)
			canvas[part.x][part.y] = { 127, 255, 0 };
		canvas[head.x][head.y] = { 0, 255, 0 };
		canvas[apple.x][apple.y] = { 255, 0, 0 };

		const std::size_t bodySize = body.size();
		std::cout << "\x1b[2J\x1b[HScore: " << (bodySize - 1) << "\n\r";
		for (int y = gameSize.y; y--;) {
			for (int x = 0; x < gameSize.x; ++x)
				std::cout << "\x1b[48;2;"
					<< static_cast<int>(canvas[x][y].red) << ';'
					<< static_cast<int>(canvas[x][y].green) << ';'
					<< static_cast<int>(canvas[x][y].blue) << "m  ";
			std::cout << "\x1b[0m\n\r";
		}
		std::cout << "Use arrow keys to move, press q to quit";
		std::cout.flush();
		if (bodySize == (gameSize.x * gameSize.y))
			break;

		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		Position newDirection = currentDirection;
		std::string input;
		while (true) {
			char character = 0;
			read(STDIN_FILENO, &character, 1);
			if (!character)
				break;
			input += character;
		}
		const std::size_t inputSize = input.size();
		for (std::size_t i = 0; i < inputSize; ++i) {
			if (static_cast<char>(std::tolower(static_cast<unsigned char>(input[i]))) == 'q')
				running = false;
			else if ((i < inputSize - 2) && (input[++i] == '['))
				switch (input[++i]) {
					case 'A':
						if (!currentDirection.y || (bodySize == 1)) {
							newDirection.x = 0;
							newDirection.y = 1;
						}
						break;
					case 'B':
						if (!currentDirection.y || (bodySize == 1)) {
							newDirection.x = 0;
							newDirection.y = -1;
						}
						break;
					case 'C':
						if (!currentDirection.x || (bodySize == 1)) {
							newDirection.x = 1;
							newDirection.y = 0;
						}
						break;
					case 'D':
						if (!currentDirection.x || (bodySize == 1)) {
							newDirection.x = -1;
							newDirection.y = 0;
						}
						break;
				}
		}
		currentDirection = newDirection;
	}

	std::cout << "\x1b[2K\x1b[0G";
	if (running)
		std::cout << "You win! ";
	std::cout << "Press any key to exit";
	std::cout.flush();
	std::this_thread::sleep_for(std::chrono::seconds(1));
	while (true) {
		char character = 0;
		read(STDIN_FILENO, &character, 1);
		if (!character)
			break;
	}
	fcntl(STDIN_FILENO, F_SETFL, blocking);
	std::cin.get();

	tcsetattr(STDIN_FILENO, TCSANOW, &cooked);
	std::cout << "\x1b[?25h\x1b[?47l";

	return 0;
}
