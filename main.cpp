#include <chrono> // std::chrono::milliseconds, std::chrono::seconds
#include <deque> // std::deque
#include <iostream> // std::cout
#include <random> // std::mt19937, std::random_device, std::uniform_int_distribution
#include <thread> // std::this_thread
#include <vector> // std::vector
#include <xieite/console/Canvas.hpp>
#include <xieite/console/RawLock.hpp>
#include <xieite/console/codes.hpp>
#include <xieite/console/getKeyPress.hpp>
#include <xieite/console/readBuffer.hpp>
#include <xieite/graphics/Color.hpp>
#include <xieite/graphics/colors.hpp>

struct Position {
	int x;
	int y;
};

int main() {
	const Position size { 20, 20 };

	std::mt19937 rng(std::random_device {}());
	std::uniform_int_distribution distX(0, size.x - 1);
	std::uniform_int_distribution distY(0, size.y - 1);

	Position apple { distX(rng), distY(rng) };

	std::deque<Position> body {
		{ distX(rng), distY(rng) }
	};
	Position direction { 0, 0 };

	int score = 0;

	std::cout
		<< xieite::console::saveScreen
		<< xieite::console::hideCursor;

	bool running = true;
	while (running) {
		const Position head {
			(body[0].x + direction.x + size.x) % size.x,
			(body[0].y + direction.y + size.y) % size.y
		};

		if (head.x == apple.x && head.y == apple.y) {
			apple = { distX(rng), distY(rng) };
			++score;
		} else
			body.pop_back();

		for (const Position part : body)
			if (part.x == head.x && part.y == head.y) {
				running = false;
				break;
			}
		if (running)
			body.push_front(head);

		xieite::console::Canvas canvas(size.y, size.x, xieite::graphics::colors::azure);
		for (const Position part : body)
			canvas.draw(part.x, part.y, xieite::graphics::colors::lime);
		canvas.draw(body[0].x, body[0].y, xieite::graphics::colors::green);
		canvas.draw(apple.x, apple.y, xieite::graphics::colors::red);
		
		std::cout
			<< xieite::console::eraseScreen
			<< xieite::console::setCursorPosition({ 0, 0 })
			<< "Score: " << score << "\n\r"
			<< canvas.string(1, 0);
		std::cout.flush();
		{
			xieite::console::RawLock rawLock(false);
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		const std::string input = xieite::console::readBuffer();
		if (input.size())
			switch (input.back()) {
			case 'q':
				running = false;
				break;
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
			}
	}

	std::cout << "\n\rPress any key to exit";
	std::cout.flush();
	std::this_thread::sleep_for(std::chrono::seconds(1));
	xieite::console::getKeyPress();

	std::cout
		<< xieite::console::showCursor
		<< xieite::console::restoreScreen;
}
