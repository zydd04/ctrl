#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <string>
#include <random>
#include <vector>
#include "qrcodegen.hpp"

using qrcodegen::QrCode;

//function to generate qrcode. Credits: nayuki
void genQr(const std::string& text) {
	QrCode qr = QrCode::encodeText(text.c_str(), QrCode::Ecc::MEDIUM);
	int size = qr.getSize();
	int border = 1;
	for (int y = -border; y < size + border; y += 2) {
        for (int x = -border; x < size + border; x++) {
            bool top = (y >= 0 && y < size && x >= 0 && x < size) ? qr.getModule(x, y) : false;
            bool bottom = (y + 1 >= 0 && y + 1 < size && x >= 0 && x < size) ? qr.getModule(x, y + 1) : false;

            if (top && bottom)       std::cout << "█"; 
            else if (top && !bottom)  std::cout << "▀"; 
            else if (!top && bottom)  std::cout << "▄"; 
            else                      std::cout << " "; 
        }
        std::cout << "\n";
    }
}

//function to generate png image. Credits: Sean Barrett
void saveQrCodeToPng(const std::string& text, const std::string& filename, int scale = 8) {

    QrCode qr = QrCode::encodeText(text.c_str(), QrCode::Ecc::MEDIUM);
    int qrSize = qr.getSize();
    int border = 4; 
    int finalSize = (qrSize + (border * 2)) * scale;
    std::vector<unsigned char> imgBuffer(finalSize * finalSize * 3);

    for (int y = 0; y < finalSize; y++) {
        for (int x = 0; x < finalSize; x++) {

            int qrX = (x / scale) - border;
            int qrY = (y / scale) - border;

            bool isDark = false;
            if (qrX >= 0 && qrX < qrSize && qrY >= 0 && qrY < qrSize) {
                isDark = qr.getModule(qrX, qrY);
            }

            unsigned char color = isDark ? 0 : 255;
            int index = (y * finalSize + x) * 3;
            imgBuffer[index + 0] = color; // Red
            imgBuffer[index + 1] = color; // Green
            imgBuffer[index + 2] = color; // Blue
        }
    }

    int stride = finalSize * 3; 
    int success = stbi_write_png(filename.c_str(), finalSize, finalSize, 3, imgBuffer.data(), stride);

    if (success) {
        std::cout << "QR Code saved to: " << filename << "\n";
    } else {
        std::cerr << "Error! \n";
    }
}

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

	int png = 0;
	int qr = 0;
	int choice = 0;
	std::string Key = genAPIK(); 
	std::cout << "Welcome to the CLI CTRLai App!" << "\n" ;
	std::cout << "Generate API Key? Yes[1] No[0]: ";
	std::cin >> choice;
	if (choice == 1) {
		std::cout << "Your Generated API Key: " << Key << "\n";
	}
	else {
		std::cout << "Okay!" << "\n";
	}
	std::cout << "Generate Qr Code? Yes[1] No[0]: ";
	std::cin >> qr;
	if (qr == 1) {
		std::cout << "Your Generated Qr Code: " << "\n";
		genQr(Key);
	}

	else {
		std::cout << "Okay!";
	}
	std::cout << "Save Qr Code As PNG? Yes[1] No[0]: ";
	std::cin >> png;
	if (png == 1) {
		std::cout << "PNG image 'qrcode.png' Saved in working dir." << "\n";
		saveQrCodeToPng(Key, "qrcode.png", 8);
	}
	else {
		std::cout << "Okay!";
	}

	std::cout << "\nPress Enter to exit...";
    	std::cin.ignore(); 
    	std::cin.get();
	
	return 0;
}

