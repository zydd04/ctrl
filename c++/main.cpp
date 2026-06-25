#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <string>
#include <random>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "qrcodegen.hpp"

using qrcodegen::QrCode;

#pragma comment(lib, "Ws2_32.lib")

#define MOBILE_PORT "1935"
#define PYTHON_PORT "1936"
#define BUFFER_SIZE 4096

// Global Variables to pass data from mobile thread to python thread
SOCKET globalPythonSocket = INVALID_SOCKET;
std::mutex pythonSocketMutex;

// Function to safely broadcast a video frame chunk to Python
void forwardFrameToPython(const char* data, int length) {
    std::lock_guard<std::mutex> lock(pythonSocketMutex);
    if (globalPythonSocket != INVALID_SOCKET) {
        // First send 4 bytes indicating total length of the upcoming frame
        send(globalPythonSocket, reinterpret_cast<const char*>(&length), 4, 0);
        // Then send the frame payload bytes
        send(globalPythonSocket, data, length, 0);
    }
}

// Thread to accept Python AI consumer connections
void startPythonListener() {
    struct addrinfo* addrResult = NULL;
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, PYTHON_PORT, &hints, &addrResult);
    SOCKET listenSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);
    bind(listenSocket, addrResult->ai_addr, (int)addrResult->ai_addrlen);
    freeaddrinfo(addrResult);
    listen(listenSocket, SOMAXCONN);

    std::cout << "[Router] Python AI interface ready on port " << PYTHON_PORT << "...\n";

    while (true) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket != INVALID_SOCKET) {
            std::lock_guard<std::mutex> lock(pythonSocketMutex);
            if (globalPythonSocket != INVALID_SOCKET) {
                closesocket(globalPythonSocket);
            }
            globalPythonSocket = clientSocket;
            std::cout << "[Router] Python AI Engine connected and synchronized!\n";
        }
    }
    closesocket(listenSocket);
}

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

    std::vector<char> frameAccumulator;

    // Read loop to accumulate and parse individual frame streams
    while (true) {
        bytesReceived = recv(clientSocket, buffer.data(), BUFFER_SIZE, 0);
        if (bytesReceived <= 0) break;

        // Append packet chunks to frame accumulator
        frameAccumulator.insert(frameAccumulator.end(), buffer.begin(), buffer.begin() + bytesReceived);

        // Simple JPEG boundary detection parser
        // Mobile phone pushes images ending with 0xFF 0xD9 (JPEG EOF Marker)
        if (frameAccumulator.size() > 4) {
            size_t size = frameAccumulator.size();
            if ((unsigned char)frameAccumulator[size - 2] == 0xFF && (unsigned char)frameAccumulator[size - 1] == 0xD9) {
                // Forward the perfect reconstructed frame down the pipe to Python
                forwardFrameToPython(frameAccumulator.data(), (int)frameAccumulator.size());
                frameAccumulator.clear(); // Wipe buffer for next coming image frame
            }
        }
    }

    closesocket(clientSocket);
    std::cout << "[Server] Client disconnected.\n";
}

void startStreamingServer(std::string expectedKey) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // Spin up the Python engine listening sub-thread pipeline
    std::thread(startPythonListener).detach();

    struct addrinfo* addrResult = NULL;
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, MOBILE_PORT, &hints, &addrResult);
    SOCKET listenSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);
    bind(listenSocket, addrResult->ai_addr, (int)addrResult->ai_addrlen);
    freeaddrinfo(addrResult);
    listen(listenSocket, SOMAXCONN);

    std::cout << "[Server] Mobile pipeline listening on port " << MOBILE_PORT << "...\n";

    while (true) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) continue;
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
            imgBuffer[index + 0] = color;
            imgBuffer[index + 1] = color;
            imgBuffer[index + 2] = color;
        }
    }
    int stride = finalSize * 3; 
    stbi_write_png(filename.c_str(), finalSize, finalSize, 3, imgBuffer.data(), stride);
    std::cout << "QR Code saved to: " << filename << "\n";
}

std::string genAPIK(size_t length = 28) {
    const std::string chars = "12334567890QWERTYIOPASDFGHJKLZXCVBNM";
    std::random_device ran;
    std::uniform_int_distribution<size_t> dist(0, chars.size() - 1);
    std::string key;
    key.reserve(length);
    for (size_t i = 0; i < length; ++i) key += chars[dist(ran)];
    return key;
}

int main() {
	int serv = 0, png = 0, qr = 0, choice = 0;
	std::string Key = genAPIK(); 
	std::cout << "Welcome to the CLI CTRLai App!\n" ;
	std::cout << "Generate API Key? Yes[1] No[0]: "; std::cin >> choice;
	if (choice == 1) std::cout << "Your Generated API Key: " << Key << "\n";
	std::cout << "Generate Qr Code? Yes[1] No[0]: "; std::cin >> qr;
	if (qr == 1) genQr(Key);
	std::cout << "Save Qr Code As PNG? Yes[1] No[0]: "; std::cin >> png;
	if (png == 1) saveQrCodeToPng(Key, "qrcode.png", 8);
	std::cout << "Do you Wanna Start the Server? Yes[1] No[0]: "; std::cin >> serv;
	if (serv == 1) {
		std::cout << "Starting Server Engine...\n";
        std::thread serverThread(startStreamingServer, Key);
        serverThread.detach();
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
	}
	std::cout << "\nPress Enter to exit...";
    std::cin.ignore(); std::cin.get();
	return 0;
}
