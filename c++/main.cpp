#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <string>
#include <random>
#include <vector>
#include <thread>
#include <chrono>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "qrcodegen.hpp"

using qrcodegen::QrCode;

#pragma comment(lib, "Ws2_32.lib")

#define PORT "1935"
#define BUFFER_SIZE 4096

// Function to handle incoming data from the mobile streaming app
void processMobileStream(SOCKET clientSocket, std::string expectedKey) {
    std::cout << "\n[Server] Client connected. Verifying API Key...\n";
    std::vector<char> buffer(BUFFER_SIZE);
    
    int bytesReceived = recv(clientSocket, buffer.data(), BUFFER_SIZE - 1, 0);
    if (bytesReceived <= 0) {
        std::cerr << "[Server] Failed to receive API Key.\n";
        closesocket(clientSocket);
        return;
    }

    buffer[bytesReceived] = '\0'; 
    std::string receivedData(buffer.data());
    
    // API KEY IMPLEMENTATION FOR SECURITY
    if (receivedData.find(expectedKey) == std::string::npos) {
        std::cerr << "[Server] AUTHENTICATION FAILED: Invalid API Key.\n";
        
        std::string errorMsg = "ERROR: Unauthorized API Key\n";
        send(clientSocket, errorMsg.c_str(), (int)errorMsg.length(), 0);
        
        closesocket(clientSocket);
        return;
    }

    std::cout << "[Server] AUTHENTICATION SUCCESSFUL! Stream authorized.\n";

    std::string successMsg = "AUTH_OK\n";
    send(clientSocket, successMsg.c_str(), (int)successMsg.length(), 0);

    // Loop to read incoming video/audio data streams
    do {
        bytesReceived = recv(clientSocket, buffer.data(), BUFFER_SIZE, 0);
        if (bytesReceived > 0) {
            std::cout << "[Server] Receiving " << bytesReceived << " bytes of authenticated stream packets...\n";
        } else if (bytesReceived == 0) {
            std::cout << "[Server] Connection closing...\n";
        } else {
            std::cerr << "[Server] recv failed with error: " << WSAGetLastError() << "\n";
            break;
        }
    } while (bytesReceived > 0);

    closesocket(clientSocket);
    std::cout << "[Server] Client disconnected.\n";
}

// FIXED: Added 'expectedKey' parameter so the function has access to the generated key
void startStreamingServer(std::string expectedKey) {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed with error: " << result << "\n";
        return;
    }

    struct addrinfo* addrResult = NULL;
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    result = getaddrinfo(NULL, PORT, &hints, &addrResult);
    if (result != 0) {
        std::cerr << "getaddrinfo failed with error: " << result << "\n";
        WSACleanup();
        return;
    }

    SOCKET listenSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed with error: " << WSAGetLastError() << "\n";
        freeaddrinfo(addrResult);
        WSACleanup();
        return;
    }

    result = bind(listenSocket, addrResult->ai_addr, (int)addrResult->ai_addrlen);
    if (result == SOCKET_ERROR) {
        std::cerr << "Bind failed with error: " << WSAGetLastError() << "\n";
        freeaddrinfo(addrResult);
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    freeaddrinfo(addrResult);

    result = listen(listenSocket, SOMAXCONN);
    if (result == SOCKET_ERROR) {
        std::cerr << "Listen failed with error: " << WSAGetLastError() << "\n";
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    std::cout << "\n[Server] Live streaming server listening on port " << PORT << "...\n";

    // Basic server loop to accept connections
    while (true) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed with error: " << WSAGetLastError() << "\n";
            continue;
        }

        // FIXED: Passed 'expectedKey' directly into the thread dispatcher loop
        std::thread(processMobileStream, clientSocket, expectedKey).detach();
    }

    closesocket(listenSocket);
    WSACleanup();
}

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
	int serv = 0;
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
	std::cout << "Generate Qr Code? Yes[1] No[0]: ";
	std::cin >> qr;
	if (qr == 1) {
		std::cout << "Your Generated Qr Code: " << "\n";
		genQr(Key);
	}
	std::cout << "Save Qr Code As PNG? Yes[1] No[0]: ";
	std::cin >> png;
	if (png == 1) {
		std::cout << "PNG image 'qrcode.png' Saved in working dir." << "\n";
		saveQrCodeToPng(Key, "qrcode.png", 8);
	}
	std::cout << "Do you Wanna Start the Server? Yes[1] No[0]: ";
	std::cin >> serv;
	if (serv == 1) {
		std::cout << "Starting Server..." << "\n";
        // FIXED: Passed the generated 'Key' into the server initialization routine
        std::thread serverThread(startStreamingServer, Key);
        serverThread.detach();
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
	}
	std::cout << "\nPress Enter to exit...";
    std::cin.ignore(); 
    std::cin.get();
	
	return 0;
}
