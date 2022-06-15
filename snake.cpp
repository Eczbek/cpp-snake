
#include <algorithm>
#include <chrono>
#include <deque>
#include <fcntl.h>
#include <iostream>
#include <random>
#include <termios.h>
#include <thread>
#include <unistd.h>


struct Position {
	int x;
	int y;
};

constexpr Position size { 20, 20 };

std::mt19937 rng(std::random_device {}());
std::uniform_int_distribution distX(0, size.x - 1);
std::uniform_int_distribution distY(0, size.y - 1);

Position apple { distX(rng), distY(rng) };

std::deque<Position> body { { distX(rng), distY(rng) } };
Position direction { 0, 0 };

int score = 0;
bool running = true;

int main() {
	termios cooked;
	tcgetattr(STDIN_FILENO, &cooked);
	termios raw = cooked;
	cfmakeraw(&raw);
	tcsetattr(STDIN_FILENO, TCSANOW, &raw);
	const int blocking = fcntl(STDIN_FILENO, F_GETFL);
	fcntl(STDIN_FILENO, F_SETFL, blocking | O_NONBLOCK);
	std::cout << "\033[?25l";

	while (running) {
		const Position head { (body[0].x + direction.x + size.x) % size.x, (body[0].y + direction.y + size.y) % size.y };

		if (head.x == apple.x && head.y == apple.y) {
			apple = { distX(rng), distY(rng) };
			++score;
		} else
			body.pop_back();
		if (std::find_if(body.begin() + 1, body.end(), [&](const Position& part) {
			return part.x == head.x && part.y == head.y;
		}) != body.end())
			break;
		body.push_front(head);

		std::string display;
		for (int y = 0; y < size.y; ++y) {
			for (int x = 0; x < size.x; ++x)
				display += ". ";
			display += "\r\n";
		}
		const std::size_t max = display.size() - 2;
		for (const Position& part: body)
			display[max - (part.y + 1) * size.x * 2 + part.x * 2 - part.y * 2] = '#';
		display[max - (apple.y + 1) * size.x * 2 + apple.x * 2 - apple.y * 2] = '@';

		std::cout << "\033[2J\033[HScore: " << score << "\r\n" << display;
		std::cout.flush();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		char input = 0;
		while (read(STDIN_FILENO, &input, 1) == 1);

		switch (input) {
			case 'd':
				if (!direction.x)
					direction = { 1, 0 };
				break;
			case 'a':
				if (!direction.x)
					direction = { -1, 0 };
				break;
			case 'w':
				if (!direction.y)
					direction = { 0, 1 };
				break;
			case 's':
				if (!direction.y)
					direction = { 0, -1 };
				break;
			case 'q':
				running = false;
		}
	}

	tcsetattr(STDIN_FILENO, TCSANOW, &cooked);
	fcntl(STDIN_FILENO, F_SETFL, blocking);
	std::cout << "\033[?25h";
}
