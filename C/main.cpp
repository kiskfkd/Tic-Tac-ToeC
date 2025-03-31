#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include "DxLib.h"
#include <ws2tcpip.h>
#include <iostream>
#include <cstdio>

#pragma comment(lib, "ws2_32.lib")

const int CELL_SIZE = 100;
const int BOARD_SIZE = 3;
const char* SERVER_IP = "127.0.0.1";
const int PORT = 8080;

int board[BOARD_SIZE][BOARD_SIZE] = { 0 };
int turn;
int clientID;
SOCKET clientSocket;

void DrawBoard() {
    for (int i = 1; i < BOARD_SIZE; i++) {
        DrawLine(i * CELL_SIZE, 0, i * CELL_SIZE, BOARD_SIZE * CELL_SIZE, GetColor(255, 255, 255));
        DrawLine(0, i * CELL_SIZE, BOARD_SIZE * CELL_SIZE, i * CELL_SIZE, GetColor(255, 255, 255));
    }
}

void DrawMarks() {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            int centerX = x * CELL_SIZE + CELL_SIZE / 2;
            int centerY = y * CELL_SIZE + CELL_SIZE / 2;
            if (board[y][x] == 1) {
                DrawCircle(centerX, centerY, 40, GetColor(0, 255, 0), FALSE);
            }
            else if (board[y][x] == 2) {
                DrawLine(centerX - 30, centerY - 30, centerX + 30, centerY + 30, GetColor(255, 0, 0));
                DrawLine(centerX - 30, centerY + 30, centerX + 30, centerY - 30, GetColor(255, 0, 0));
            }
        }
    }
}

void SendMove(int x, int y) {
    char buffer[32];
    sprintf(buffer, "%d %d %d\n", x, y, clientID + 1);
    send(clientSocket, buffer, strlen(buffer), 0);
}

void ReceiveBoard() {
    fd_set readfds;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    FD_ZERO(&readfds);
    FD_SET(clientSocket, &readfds);

    int ret = select(0, &readfds, NULL, NULL, &timeout);

    if (ret > 0 && FD_ISSET(clientSocket, &readfds)) {
        char buffer[64];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';

            // 勝利メッセージの確認
            if (strncmp(buffer, "Win:", 4) == 0) {
                int winner;
                sscanf(buffer + 5, "%d", &winner);
                printf("プレイヤー%dの勝利！\n", winner);
                DrawString(10, BOARD_SIZE * CELL_SIZE + 30, "ゲーム終了", GetColor(255, 255, 0));
                return;
            }
            if (strncmp(buffer, "Draw", 4) == 0) {
                printf("引き分けです！\n");
                DrawString(10, BOARD_SIZE * CELL_SIZE + 30, "引き分け", GetColor(255, 255, 0));
                return;
            }

            // ボード情報を更新
            int index = 0;
            for (int y = 0; y < BOARD_SIZE; y++) {
                for (int x = 0; x < BOARD_SIZE; x++) {
                    sscanf(buffer + index, "%d", &board[y][x]);
                    while (buffer[index] != ' ' && buffer[index] != '\n' && buffer[index] != '\0') index++;
                    index++;
                }
            }
            sscanf(buffer + index, "%d", &turn);
        }
    }
}

void HandleInput() {
    if (turn % 2 != clientID) return;

    int mouseX, mouseY;
    int button = GetMouseInput();
    if (button & MOUSE_INPUT_LEFT) {
        GetMousePoint(&mouseX, &mouseY);
        int x = mouseX / CELL_SIZE;
        int y = mouseY / CELL_SIZE;
        if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE && board[y][x] == 0) {
            SendMove(x, y);
        }
    }
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    ChangeWindowMode(TRUE), DxLib_Init(), SetDrawScreen(DX_SCREEN_BACK);

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);
    connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

    char buffer[64];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        sscanf(buffer, "ClientID: %d", &clientID);
    }

    while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0) {
        ClearDrawScreen();
        DrawBoard();
        DrawMarks();
        HandleInput();
        ReceiveBoard();
        ScreenFlip();
    }

    closesocket(clientSocket);
    WSACleanup();
    DxLib_End();
    return 0;
}
