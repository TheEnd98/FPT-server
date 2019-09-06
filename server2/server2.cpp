#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <Windows.h>
#include <string.h>
#include <iostream>
#include <cstdlib>
#include "pch.h"
#include <iostream>
#include <fstream>
#include "pch.h"
#include <iostream>
#include <winsock2.h>
#include<process.h>
#pragma warning(disable: 4996)
#pragma comment(lib,"ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define server_addr "127.0.0.1"
#define welcome "220 welcome to ftp server on windown.\r\n"
#define PATH "C:\\server2\\server2\\"

using namespace std;

DWORD WINAPI ClientThread(LPVOID arg);

struct ftpClient {
	char verb[1024], curdic[1024], arg[1024], name[1024], pass[1024];
	SOCKET data_sk, ctrl_sk;
	int login;

	ftpClient(SOCKET c) {
		ctrl_sk = c;
		login = -1;
		memset(curdic, 0, 1024);// làm sạch mảng
		strcpy(curdic, PATH);
	}
};

void insert(char* source, int index) {
	char* tmp = new char[strlen(source) + 1];
	strcpy(tmp, source + index);
	source[index] = '"';
	strcpy(source + index + 1, tmp);
	free(tmp);
}

void sendResponse(ftpClient client, char* res) {
	send(client.ctrl_sk, res, strlen(res), 0);
}
void resUSER(ftpClient& client) {
	memset(client.name, 0, 1024);
	strcpy(client.name, client.arg);
	char s[] = "331 Please spcify the password.\n";
	sendResponse(client, s);
}

void resPASS(ftpClient& client) {
	memset(client.pass, 0, 1024);
	strcpy(client.pass, client.arg);

	if (strcmp(client.name, "anonymous") == 0) {
		// dang nhap an danh
		client.login = 1;
		char s[] = "230 Login succesfull.\r\n";
		sendResponse(client, s);
	}
	else {
		FILE* f = fopen("account.txt", "rt");
		char line[1024], user[1024], pass[1024];
		bool isValid = false;
		while (!feof(f)) {
			memset(line, 0, 1024); memset(user, 0, 1024); memset(pass, 0, 1024);
			fgets(line, 1023, f);
			sscanf(line, "%s%s", user, pass);
			if (strcmp(user, client.name) == 0 && strcmp(pass, client.pass) == 0) {
				// dang nhap voi tu cach la user
				client.login = 2;
				isValid = true;
				char s[] = "230 Login succesfull.\r\n";
				sendResponse(client, s);
				break;
			}
		}
		if (!isValid) {
			char s[] = "530 name or password incorrect.\r\n";
			sendResponse(client, s);
			client.login = 0;
			closesocket(client.ctrl_sk);
		}
	}

}

void resPWD(ftpClient client) {
	char dir[1024];
	memset(dir, 0, 1024);
	sprintf(dir, "%d \"%s\\\"\r\n", 257, client.curdic);
	sendResponse(client, dir);
}

void resCWD(ftpClient& client) {
	if (strstr(client.arg, "\\") == NULL) {
		char path[1024];
		memset(path, 0, 1024);
		sprintf(path, "%s%s\\", client.curdic, client.arg);
		strcpy(client.curdic, path);
	}
	else {
		memset(client.curdic, 0, sizeof(client.curdic));
		strcpy(client.curdic, client.arg);
	}
	char s[] = "250 Directory succesfully changed.\r\n";
	sendResponse(client, s);
}

void resSYST(ftpClient client) {
	char s[] = "215 Windown_NT.\r\n";
	sendResponse(client, s);
}

void resPASV(ftpClient& client) {
	//srand(time(NULL));
	int port = 1024 + rand() % (65536 - 1024 + 1);

	char response[1024];
	memset(response, 0, 1024);
	sprintf(response, "227 Entering Passive Mode (127,0,0,1,%d,%d).\r\n", port / 256, port % 256);
	sendResponse(client, response);

	SOCKET datasocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = inet_addr(server_addr);
	bind(datasocket, (sockaddr*)&saddr, sizeof(saddr));
	listen(datasocket, 10);
	SOCKADDR_IN caddr;
	int clen = sizeof(caddr);
	client.data_sk = accept(datasocket, (sockaddr*)&caddr, &clen);
	cout << "connect data succesfully" << endl;
}

void resCDUP(ftpClient& client) {
	char path[1024];
	strcpy(path, client.curdic);
	for (int i = strlen(path) - 2; i >= 0; i--) {
		if (path[i] == '\\') {
			path[i + 1] = 0;
			break;
		}
	}
	strcpy(client.curdic, path);
	char s[] = "200 Directory succesfully changed.\r\n";
	sendResponse(client, s);
}

void resDELE(ftpClient& client) {
	char cmd[256];
	sprintf(cmd, "del %s\\%s", client.curdic, client.arg);

	system(cmd);

	char s[] = "250 dele done !\n";
	sendResponse(client, s);
}

void resLIST(ftpClient client) {
	char sk[] = "150 Here comes the directory listing.\r\n";
	sendResponse(client, sk);
	char command[1024], path[1024];
	memset(path, 0, 1024);

	cout << client.curdic;
	strcpy(path, client.curdic);

	memset(command, 0, 1024);
	sprintf(command, "dir %s > 1.txt", path);
	system(command);// dùng lệnh hệ thống để thực hện hàm dir

	FILE* f = fopen("1.txt", "r+");
	char cmd[256];
	while (fgets(cmd, sizeof(cmd), f)) {// đọc từng dòng
		//xu ly output lenh dir
		if ((cmd[0] == '0') || cmd[0] == '1') {
			for (int i = 17; i < (int)(strlen(cmd) - 1); i++) {
				cmd[i] = cmd[i + 1];
			}
			send(client.data_sk, cmd, strlen(cmd) - 1, 0);
		}
	}

	fclose(f);


	closesocket(client.data_sk);
	char k[] = "226 Directory send OK.\r\n";
	sendResponse(client, k);
}

void resSTOR(ftpClient client) {//tải lên server
	if (client.login == 1) {
		char s[] = "550 Permission denied.\r\n";
		sendResponse(client, s);
		closesocket(client.data_sk);
	}
	else {
		char buf[1024], path[1024];
		memset(path, 0, 1024);
		sprintf(path, "%s\\%s", client.curdic, client.arg);
		char s[] = "150 Opening BINARY mode data connection.\r\n";
		sendResponse(client, s);

		FILE *f = fopen(path, "wb");
		while (1)
		{
			char buf[1024];
			int ret = recv(client.data_sk, buf, sizeof(buf) - 1, 0);
			if (ret > 0)
				fwrite(buf, 1, ret, f);
			else
				break;
		}
		fclose(f);
		char k1[] = "226 Transfer file successfully.\r\n";
		sendResponse(client, k1);
	}
}

void resOPTS(ftpClient client) {
	char s[] = "200 Always in UTF8 mode.\r\n";
	sendResponse(client, s);
}
void resTYPE(ftpClient client) {
	char s[] = "200 Switching to Binary mode.\r\n";
	sendResponse(client, s);
}

void resSIZE(ftpClient client) {
	char path[1024], size[1024];
	memset(path, 0, 1024);
	sprintf(path, "D:%s/%s", client.curdic, client.arg);
	FILE* f = fopen(path, "rb");
	if (f != NULL) {
		fseek(f, 0, SEEK_END);
		long filesize = ftell(f);
		char* data = (char*)calloc(filesize, 1);
		fseek(f, 0, SEEK_SET);
		fread(data, 1, filesize, f);
		fclose(f);

		memset(size, 0, 1024);
		sprintf(size, "213 %d\r\n", filesize);
		sendResponse(client, size);
	}
	else {
		char s[] = "550 Could not get file size.\r\n";
		sendResponse(client, s);
	}
}
void resRETR(ftpClient client) {// tải dữ liệu từ server về
	char path[1024], status[1024], sendok[1024];
	memset(path, 0, 1024); memset(status, 0, 1024); memset(sendok, 0, 1024);
	sprintf(path, "%s\\%s", client.curdic, client.arg);//đường dẫn đến file cần gửi

	FILE* f = fopen(path, "rb");//đọc theo bit
	if (f != NULL) {// nếu file có dữ liệu
		fseek(f, 0, SEEK_END);// trỏ đến vị trí cuối cùng
		long filesize = ftell(f);// độ dài file
		char* data = (char*)calloc(filesize, 1); //cấp phát bộ nhớ cho data
		fseek(f, 0, SEEK_SET);// trỏ đến vị trí đầu
		fread(data, 1, filesize, f);// đọc dữ liệu gửi đi
		fclose(f);

		sprintf(status, "150 Opening BINARY mode data connection for %s (%ld bytes).\r\n", client.arg, filesize);
		sendResponse(client, status);
		send(client.data_sk, data, filesize, 0);
		closesocket(client.data_sk);
		strcpy(sendok ,"226 Transfer complete.\r\n");
		sendResponse(client, sendok);
		free(data);
	}
	else {
		char s[] = "550 Failed to open file.\r\n";
		sendResponse(client, s);
	}
}
void resFEAT(ftpClient client) {
	char s1[] = "211 Features:\r\n";
	sendResponse(client, s1);
	char s2[] = "OPTS\r\n";
	sendResponse(client, s2);
	char s3[] = "DELE\r\n";
	sendResponse(client, s3);
	char s4[] = "STOR\r\n";
	sendResponse(client, s4);
	char s5[] = "RETR\r\n";
	sendResponse(client, s5);
	char s6[] = "TYPE\r\n";
	sendResponse(client, s6);
	char s7[] = "RETR\r\n";
	sendResponse(client, s7);
	char s8[] = "CDUP\r\n";
	sendResponse(client, s8);
	char s9[] = "STOR\r\n";
	sendResponse(client, s9);
	char s10[] = "211 End.\r\n";
	sendResponse(client, s10);
}


void Handle_client(ftpClient& client) {
	if (strcmp(client.verb, "USER") == 0) resUSER(client);
	else
		if (strcmp(client.verb, "PASS") == 0) resPASS(client);
		else
			if (client.login > 0) {
				if (strcmp(client.verb, "OPTS") == 0) resOPTS(client);
				else if (strcmp(client.verb, "PWD") == 0) resPWD(client);
				else if (strcmp(client.verb, "CWD") == 0) resCWD(client);
				else if (strcmp(client.verb, "SYST") == 0) resSYST(client);
				else if (strcmp(client.verb, "PASV") == 0) resPASV(client);
				else if (strcmp(client.verb, "LIST") == 0) resLIST(client);
				else if (strcmp(client.verb, "TYPE") == 0) resTYPE(client);
				else if (strcmp(client.verb, "FEAT") == 0) resTYPE(client);
				else if (strcmp(client.verb, "SIZE") == 0) resSIZE(client);
				else if (strcmp(client.verb, "RETR") == 0) resRETR(client);
				else if (strcmp(client.verb, "STOR") == 0) resSTOR(client);
				else if (strcmp(client.verb, "CDUP") == 0) resCDUP(client);
				else if (strcmp(client.verb, "DELE") == 0) resDELE(client);
			}
			else {
				char s[] = "500 Unknown command.\r\n";
				sendResponse(client, s);
			}
}

int main()
{
	WSADATA DATA;
	WSAStartup(MAKEWORD(2, 2), &DATA);

	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(21);
	saddr.sin_addr.s_addr = inet_addr(server_addr);

	bind(s, (sockaddr*)&saddr, sizeof(saddr));
	listen(s, 10);

	while (1) {
		SOCKADDR_IN caddr;
		int clen = sizeof(caddr);
		SOCKET c = accept(s, (sockaddr*)&caddr, &clen);
		DWORD ID = 0;
		CreateThread(NULL, 0, ClientThread, (LPVOID)c, 0, &ID);
	}
}

DWORD WINAPI ClientThread(LPVOID arg) {
	SOCKET c = (SOCKET)arg;
	ftpClient client = ftpClient(c);
	send(client.ctrl_sk, welcome, strlen(welcome), 0);

	char buffer[1024], verb[1024];

	while (1) {

		if (client.login == 0) break;

		memset(buffer, 0, 1024);
		memset(client.verb, 0, 1024);
		memset(client.arg, 0, 1024);
		recv(client.ctrl_sk, buffer, 1023, 0);
		if (strlen(buffer) == 0) break;

		char* firstspace = strstr(buffer, " ");
		if (firstspace != NULL) {
			strncpy(client.verb, buffer, firstspace - buffer);
			strcpy(client.arg, firstspace + 1);
			client.arg[strlen(client.arg) - 1] = 0;
			client.arg[strlen(client.arg) - 1] = 0;
			cout << client.verb << " " << client.arg << endl;
		}
		else {
			strcpy(client.verb, buffer);
			client.verb[strlen(client.verb) - 1] = 0;
			client.verb[strlen(client.verb) - 1] = 0;
			cout << client.verb << endl;
		}

		if (strcmp(verb, "QUIT") == 0) {
			char s[] = "221 Goodbye.\r\n";
			sendResponse(client, s);
			break;
		}
		else {
			Handle_client(client);
		}
	}
	cout << "Client disconnect" << endl;
	return 0;
}



