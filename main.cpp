#include <iostream>
#include <string>
#include <random>

#include <iostream>

//function to generate random API key
std::string genAPIK(size_t length = 28) {
    const std::string chars =
        "12334567890" "QWERTYIOPASDFGHJKLZXCVBNM";
    std::random_device ran;
    std::uniform_int_distribution<size_t> dist(0, chars.size() - 1);
    std::string key;
    key.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        key += chars[dist(ran)];
    }
    return key;
}

int main() {

	int choice;

	std::cout << "Welcome to the CLI CTRLai App!" << "\n" ;
	std::cout << "Generate API Key? Yes[1] No[0]";
	std::cin >> choice;
	if (choice == 1) {
		std::cout << "Your Generated API Key: " << genAPIK() << "\n";
	}
	else {
		std::cout << "Bye!";
	}
	return 0;
}

