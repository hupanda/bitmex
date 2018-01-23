#include "stdafx.h"
#include "bitmexwebsocket.cpp"

int main(int argc, char* argv[]) {
	
	try {
		cache c;
		c.set_symbols({ "XBTUSD" });
		bitmexwebsocket endpoint;
		endpoint.init(&c);
		endpoint.start();
		int i = 0;
		while (true)
		{
			i = i + 1;
		}
	}
	catch (const std::exception & e) {
		std::cout << e.what() << std::endl;
	}
	catch (websocketpp::lib::error_code e) {
		std::cout << e.message() << std::endl;
	}
	catch (...) {
		std::cout << "other exception" << std::endl;
	}
}