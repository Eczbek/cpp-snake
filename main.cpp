#include <chrono> // std::chrono::milliseconds, std::chrono::seconds
#include <deque> // std::deque
#include <iostream> // std::cout
#include <random> // std::mt19937, std::random_device, std::uniform_int_distribution
#include <thread> // std::this_thread
#include <xieite/console/NonBlockLock.hpp>
#include <xieite/console/RawLock.hpp>
#include <xieite/console/erasors.hpp>
#include <xieite/console/modes.hpp>
#include <xieite/console/nextKeyPress.hpp>
#include <xieite/console/readBuffer.hpp>
#include <xieite/console/resets.hpp>
#include <xieite/console/setBackground.hpp>
#include <xieite/console/setCursorPosition.hpp>
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
		<< xieite::console::modes::saveScreen
		<< xieite::console::modes::hideCursor;
	xieite::console::NonBlockLock nonBlockLock;
	xieite::console::RawLock rawLock;

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

		std::vector<std::vector<xieite::graphics::Color>> displayMap;
		for (int x = 0; x < size.x; ++x) {
			displayMap.emplace_back();
			for (int y = 0; y < size.y; ++y)
				displayMap[x].push_back(xieite::graphics::colors::azure);
		}
		for (const Position part : body)
			displayMap[part.x][part.y] = xieite::graphics::colors::lime;
		displayMap[body[0].x][body[0].y] = xieite::graphics::colors::green;
		displayMap[apple.x][apple.y] = xieite::graphics::colors::red;
	
		std::string displayString;
		for (int y = size.y; y--;) {
			for (int x = 0; x < size.x; ++x)
				displayString += xieite::console::setBackground(displayMap[x][y]) + "  " + std::string(xieite::console::resets::background);
			displayString += "\n\r";
		}
		std::cout
			<< xieite::console::erasors::screen
			<< xieite::console::setCursorPosition({ 0, 0 })
			<< "Score: " << score << "\n\r"
			<< displayString;
		std::cout.flush();

		std::this_thread::sleep_for(std::chrono::milliseconds(100));

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

	std::cout << "Press any key to exit";
	std::cout.flush();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	nonBlockLock.~NonBlockLock();
	rawLock.~RawLock();
	xieite::console::nextKeyPress();

	std::cout
		<< xieite::console::modes::showCursor
		<< xieite::console::modes::restoreScreen;
}
