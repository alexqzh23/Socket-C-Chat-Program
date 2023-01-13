// Client.cpp : Defines the entry point for the console application.
//
// 1. open the *.c in the Visual C++, then "rebuild all".
// 2. click "yes" to create a project workspace.
// 3. You need to -add the library 'ws2_32.lib' to your project 
//    (Project -> Properties -> Linker -> Input -> Additional Dependencies) 
// 4. recompile the source.


// #include "stdafx.h"
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mswsock.h>
#include <limits.h>
#include <windows.h>
#include "curses.h"

#define DEFAULT_PORT	5019
#define BUFF_SIZE 4096
char userName[100];
char inputPrefix[128];
bool done = FALSE;
WINDOW* input, * output, * frecv, *prompt;
HANDLE  g_hMutex = NULL;
int flag = 0;

typedef struct node {
	char* data;
	struct node* next;
} Node;

// newly added number will be put at the head of list
void InsertNode(Node** phead, char* x) {
	Node* newNode = (Node*)malloc(sizeof(Node));
	newNode->data = (char*)malloc((strlen(x) + 1) * sizeof(char));
	newNode->next = NULL;
	strcpy(newNode->data, x);

	if (!*phead) {
		*phead = newNode;
	}
	else {
		Node* temp = *phead;
		while (temp->next) {
			temp = temp->next;
		}
		temp->next = newNode;
	}
}

char* transPath(char* path, char** fileName) {
	Node* head = NULL;
	int length = strlen(path);
	char* ptr = strtok(path, "\\"); // 实现字符串的分割
	while (ptr != NULL)
	{
		//printf("%s\n", ptr);
		InsertNode(&head, ptr);
		length += 2;
		ptr = strtok(NULL, "\\");
	}

	//length--;
	char* newPath = (char*)malloc(sizeof(char) * length);
	char* p = newPath;
	Node* temp = head;
	while (temp) {
		strcpy(p, temp->data);
		p += strlen(temp->data);
		if (temp->next) {
			strcpy(p, "\\\\");
		}
		else {
			(*fileName) = (char*)malloc(sizeof(char) * (strlen(temp->data) + 1));
			strcpy(*fileName, temp->data);
		}
		p += 2;
		temp = temp->next;
	}
	Node* nextNode = NULL;
	while (head) {
		nextNode = head->next; // save the address of next node
		free(head->data);
		free(head);
		head = nextNode;
	}
	return newPath;
}

int sendFile(SOCKET connect_sock, int fileState) {
	char sdfBuff[BUFF_SIZE];
	char temp[BUFF_SIZE];
	long msg_len;
	int flag = 0;
	char* fileName;
	char* fullpath;
	char* path;
	HANDLE hFile;
	werase(frecv);
	waddstr(frecv, "Please input file name or file path, input :Exit to exit");
	wrefresh(frecv);
	mvwprintw(input, 0, 0, inputPrefix);
	if (wgetnstr(input, sdfBuff, COLS - 4) != OK) {
		return 0;
	}
	werase(input);
	if (strcmp(sdfBuff, ":Exit") == 0) {
		werase(frecv);
		waddstr(frecv, "(Input :File to send a file | :Change to chat with someone new | :Quit to switch the chat mode)");
		wrefresh(frecv);
		return 0;
	}
	do {
		fileName = NULL;
		fullpath = NULL;
		path = NULL;
		fullpath = _fullpath(NULL, sdfBuff, strlen(sdfBuff));

		path = transPath(fullpath, &fileName);

		DWORD dwNum = MultiByteToWideChar(CP_ACP, 0, path, -1, NULL, 0);
		WCHAR* pwText;
		pwText = (WCHAR*)calloc(dwNum, sizeof(WCHAR));

		if (pwText == NULL)
		{
			free(pwText);
		}

		if (pwText != NULL) {
			memset(pwText, 0, dwNum * sizeof(WCHAR));
		}

		MultiByteToWideChar(CP_ACP, 0, path, -1, pwText, dwNum);

		hFile = CreateFile(pwText, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			werase(frecv);
			waddstr(frecv, "File open failed, Please try again! (Input :Exit to exit)");
			wrefresh(frecv);
			flag = 1;
		}
		else {
			flag = 0;
			free(path);
		}
		if (flag) {
			mvwprintw(input, 0, 0, inputPrefix);
			if (wgetnstr(input, sdfBuff, COLS - 4) != OK) {
				return -1;
			}
			werase(input);
			if (strcmp(sdfBuff, ":Exit") == 0) {
				werase(frecv);
				waddstr(frecv, "(Input :File to send a file | :Change to chat with someone new | :Quit to switch the chat mode)");
				wrefresh(frecv);
				return 0;
			}
		}
	} while (flag);

	LARGE_INTEGER liFileSize;
	if (GetFileSizeEx(hFile, &liFileSize) == FALSE) {
		printf("GetFileSizeEx function failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
	if (fileState == 1) { // group
		msg_len = send(connect_sock, "(group)", 7, 0);
	}

	sprintf(sdfBuff, "?FIN|%s|%ld", fileName, liFileSize);

	msg_len = send(connect_sock, sdfBuff, strlen(sdfBuff), 0);
	Sleep(100);
	if (TransmitFile(connect_sock, hFile, 0, 0, NULL, NULL, TF_USE_KERNEL_APC) == FALSE) {
		printf("TransmitFile function failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}

	CloseHandle(hFile);
	free(fileName);
	mvwprintw(input, 0, 0, inputPrefix);
	werase(frecv);
	waddstr(frecv, "(Input :File to send a file | :Change to chat with someone new | :Quit to switch the chat mode)");
	wrefresh(frecv);
	waddch(output, '\n');
	waddstr(output, "File has been send!");
	wrefresh(output);
	return 0;
}

int recvFile(SOCKET msg_sock, char* fileName, long fileSize) {
	char refBuff[BUFF_SIZE];
	char buffer[1024];
	long msg_len;
	long fileLen = fileSize;

	DWORD dwNum = MultiByteToWideChar(CP_ACP, 0, fileName, -1, NULL, 0);
	WCHAR* pwText;
	pwText = (WCHAR*)calloc(dwNum, sizeof(WCHAR));
	if (pwText == NULL)
	{
		free(pwText);
	}

	memset(pwText, 0, dwNum * sizeof(WCHAR));

	MultiByteToWideChar(CP_ACP, 0, fileName, -1, pwText, dwNum);

	HANDLE hFILE = CreateFile(pwText, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFILE == INVALID_HANDLE_VALUE) {
		printf("CreateFile function failed with error: %ld", WSAGetLastError());
		waddch(output, '\n');   /* result from wgetnstr has no newline */
		waddstr(output, buffer);
		wrefresh(output);
		WSACleanup();
		return -1;
	}

	if (SetFilePointer(hFILE, 0, NULL, FILE_BEGIN) == -1)
	{
		printf("SetFilePointer error\n");
		return 0;
	}
	memset(refBuff, 0, sizeof(refBuff));
	WaitForSingleObject(g_hMutex, INFINITE);
	do {
		
		
		msg_len = recv(msg_sock, refBuff, sizeof(refBuff), 0);

		if (msg_len <= 0) {
			printf("Client closed connection\n");
			closesocket(msg_sock);
			return -1;
		}

		DWORD dwWrite;
		if (!WriteFile(hFILE, &refBuff, msg_len, &dwWrite, NULL))
		{
			printf("WriteFile error\n");
			return 0;
		}
		/*waddch(output, '\n');
		sprintf(refBuff, "S: %d", fileLen);
		waddstr(output, refBuff);
		wrefresh(output);*/
		fileLen -= dwWrite;
		memset(refBuff, 0, sizeof(refBuff));
		Sleep(100);
	} while (fileLen > 0);
	ReleaseMutex(g_hMutex);
	CloseHandle(hFILE);
	free(pwText);
	return 0;
}

DWORD WINAPI runner1(LPVOID param) {
	DWORD connect_sock = *(DWORD *) param;
	char szBuff[BUFF_SIZE] = { 0 };
	char newName[BUFF_SIZE] = { 0 };
	char newGroup[BUFF_SIZE] = { 0 };
	char signal[BUFF_SIZE] = { 0 };
	int msg_len = 0;
	int inputKey = 0;
	char temp[BUFSIZ];

	while(1)
	{
		mvwprintw(input, 0, 0, inputPrefix);
		wrefresh(input);
		werase(frecv);
		waddstr(frecv, "(Press y for one-to-one chat, press n for group chat)");
		wrefresh(frecv);

		inputKey = _getch();
		werase(input);
		switch (inputKey){
			case 121: flag = 1; break; // y
			case 110: flag = 2; break; // n
		}

		if (flag == 1) // 私聊
		{
			msg_len = send(connect_sock, "(printName)", 11, 0);
			memset(szBuff, 0, sizeof(szBuff));
		  /* result from wgetnstr has no newline */
			werase(frecv);
			waddstr(frecv, "Please input a chat name");
			wrefresh(frecv);
			mvwprintw(input, 0, 0, inputPrefix);
			if (wgetnstr(input, szBuff, COLS - 4) != OK) {
				return -1;
			}
			werase(input);
			sprintf(newName, "#%s", szBuff);
			msg_len = send(connect_sock, newName ,sizeof(newName), 0);
			werase(frecv);
			waddstr(frecv, "(Input :File to send a file | :Change to chat with someone new | :Quit to switch the chat mode)");
			wrefresh(frecv);
			werase(prompt);
			sprintf(temp, "You are now chatting with %s", szBuff);
			waddstr(prompt, temp);
			wrefresh(prompt);
			memset(szBuff, 0, sizeof(temp));
			memset(szBuff, 0, sizeof(szBuff));
			while(1){
				memset(szBuff, 0, sizeof(szBuff));
				mvwprintw(input, 0, 0, inputPrefix);
				if (wgetnstr(input, szBuff, COLS - 4) != OK) {
					return -1;
				}
				werase(input);
				wrefresh(input);
				
				if(strcmp(szBuff, ":Quit") == 0){
					flag = 0;
					break;
				} else if(strcmp(szBuff, ":Change") == 0){
					msg_len = send(connect_sock, "(printName)", 11, 0);
					werase(frecv);
					memset(szBuff, 0, sizeof(szBuff));
					waddstr(frecv, "Please input a new chat name");
					wrefresh(frecv);
					mvwprintw(input, 0, 0, inputPrefix);
					if (wgetnstr(input, szBuff, COLS - 4) != OK) {
						return -1;
					}
					werase(input);
					mvwprintw(input, 0, 0, inputPrefix);
					sprintf(newName, "#%s", szBuff);
					msg_len = send(connect_sock, newName ,sizeof(newName), 0);
					werase(prompt);
					sprintf(temp, "You are now chatting with %s", szBuff);
					waddstr(prompt, temp);
					wrefresh(prompt);
					memset(temp, 0, sizeof(temp));
					memset(szBuff, 0, sizeof(szBuff));
					werase(frecv);
					waddstr(frecv, "(Input :File to send a file | :Change to chat with someone new | :Quit to switch the chat mode)");
					wrefresh(frecv);
					continue;
				} else if (strcmp(szBuff, ":File") == 0) {
					sendFile(connect_sock, 0);
					continue;
				}
				
				sprintf(temp, "[%s]: %s", userName, szBuff);
				waddch(output, '\n');   /* result from wgetnstr has no newline */
				waddstr(output, temp);
				wrefresh(output);

				msg_len = send(connect_sock, szBuff, sizeof(szBuff), 0);

				if (msg_len == SOCKET_ERROR){
					fprintf(stderr, "send() failed with error %d\n", WSAGetLastError());
					WSACleanup();
					return -1;
				}

				if (msg_len == 0){
					printf("server closed connection\n");
					closesocket(connect_sock);
					WSACleanup();
					return -1;
				}
			}
			flag = 0;
			continue;
		}

		if (flag == 2){ // 群聊
			msg_len = send(connect_sock, "(printGroup)", 12, 0);
			memset(szBuff, 0, sizeof(szBuff));
			werase(frecv);
			waddstr(frecv, "Please input a group name");
			wrefresh(frecv);
			mvwprintw(input, 0, 0, inputPrefix);
			if (wgetnstr(input, szBuff, COLS - 4) != OK) {
				return -1;
			}
			werase(input);
			memset(newGroup, 0, sizeof(newGroup));
			sprintf(newGroup, "$%s", szBuff);
			werase(prompt);
			sprintf(temp, "You are now chatting in group %s", szBuff);
			waddstr(prompt, temp);
			wrefresh(prompt);
			msg_len = send(connect_sock, newGroup ,sizeof(newGroup), 0);
			werase(frecv);
			waddstr(frecv, "(Input :File to send a file | :Change to chat in a new group | :Quit to switch the chat mode)");
			wrefresh(frecv);
			while(1){
				memset(szBuff, 0, sizeof(szBuff));
				mvwprintw(input, 0, 0, inputPrefix);
				if (wgetnstr(input, szBuff, COLS - 4) != OK) {
					return -1;
				}
				werase(input);
				if(strcmp(szBuff, ":Quit") == 0){
					flag = 0;
					break;
				}
				else if(strcmp(szBuff, ":Change") == 0){
					msg_len = send(connect_sock, "(printGroup)", 12, 0);
					werase(frecv);
					memset(szBuff, 0, sizeof(szBuff));
					waddstr(frecv, "Please input a new group name");
					wrefresh(frecv);
					mvwprintw(input, 0, 0, inputPrefix);
					if (wgetnstr(input, szBuff, COLS - 4) != OK) {
						return -1;
					}
					werase(input);
					mvwprintw(input, 0, 0, inputPrefix);
					memset(newGroup, 0, sizeof(newGroup));
					sprintf(newGroup, "$%s", szBuff);
					werase(prompt);
					sprintf(temp, "You are now chatting in group %s", szBuff);
					waddstr(prompt, temp);
					wrefresh(prompt);
					msg_len = send(connect_sock, newGroup, sizeof(newGroup), 0);
					memset(temp, 0, sizeof(temp));
					memset(szBuff, 0, sizeof(szBuff));
					werase(frecv);
					waddstr(frecv, "(Input :File to send a file | :Change to chat in a new group | :Quit to switch the chat mode)");
					wrefresh(frecv);
					continue;
				}
				else if (strcmp(szBuff, ":File") == 0) {
					sendFile(connect_sock, 1);
					continue;
				}
				sprintf(temp, "(G)[%s]: %s", userName, szBuff);
				waddch(output, '\n');   /* result from wgetnstr has no newline */
				waddstr(output, temp);
				wrefresh(output);

				msg_len = send(connect_sock, "(group)", 7, 0);
				
				if (msg_len == SOCKET_ERROR){
					fprintf(stderr, "send() failed with error %d\n", WSAGetLastError());
					Sleep(1500);
					WSACleanup();
					return -1;
				}

				if (msg_len == 0){
					printf("server closed connection\n");
					Sleep(1500);
					closesocket(connect_sock);
					WSACleanup();
					return -1;
				}

				msg_len = send(connect_sock, szBuff, sizeof(szBuff), 0);

				if (msg_len == SOCKET_ERROR){
					fprintf(stderr, "send() failed with error %d\n", WSAGetLastError());
					Sleep(1500);
					WSACleanup();
					return -1;
				}

				if (msg_len == 0){
					printf("server closed connection\n");
					Sleep(1500);
					closesocket(connect_sock);
					WSACleanup();
					return -1;
				}
				memset(szBuff, 0, sizeof(szBuff));
			}
			flag = 0;
			continue;
		}

		if(inputKey == 72 || inputKey == 0 || inputKey == 68){ //为了显示美观，加一个无回显的读取字符函数
			continue; //getch返回值如果是这几个值，则getch会自动跳过
		}
	}

	return 0;
}

DWORD WINAPI runner2(LPVOID param) {
	DWORD connect_sock = *(DWORD *) param;
	char szBuff[BUFF_SIZE] = { 0 };
	char temp[BUFF_SIZE];
	int msg_len;

	while(1)
	{
		Sleep(100);
		msg_len = recv(connect_sock, szBuff, sizeof(szBuff), 0);
	
		if (msg_len == SOCKET_ERROR){
			closesocket(connect_sock);
			WSACleanup();
			return -1;
		}

		if (msg_len == 0){
			printf("server closed connection\n");
			Sleep(1500);
			closesocket(connect_sock);
			WSACleanup();
			return -1;
		}

		strcpy(temp, szBuff);
		char* ptr = strtok(temp, "|");
		if (ptr && strcmp(ptr, "?FIN") == 0) {
				char * filename = strtok(NULL, "|");
				long fileSize = strtol(strtok(NULL, "|"), NULL, 10);
				recvFile(connect_sock, filename, fileSize);
				ptr = NULL;
				memset(szBuff, 0, sizeof(szBuff));
				continue;
		}
		if (szBuff[0] == '$' && szBuff[1] == '$' && szBuff[2] != '$') {
			memset(temp, 0, sizeof(temp));
			
			if (flag == 1) {
				memset(temp, 0, sizeof(temp));
				sprintf(temp, "\nYou are assigned to a new group %s", &szBuff[2]);
				waddstr(output, temp);
				wrefresh(output);
			}
			if (flag == 2) {
				werase(prompt);
				sprintf(temp, "You are now chatting in group %s", &szBuff[2]);
				waddstr(prompt, temp);
				wrefresh(prompt);
				memset(temp, 0, sizeof(temp));
				sprintf(temp, "\nYour new group is %s", &szBuff[2]);
				waddstr(output, temp);
				wrefresh(output);

			}

			continue;
		}

		if (strlen(szBuff) != 0){
			waddch(output, '\n');   /* result from wgetnstr has no newline */
			waddstr(output, szBuff);
			wrefresh(output);
			memset(szBuff, 0, sizeof(szBuff));
		}
	}

	return 0;
}

int main(int argc, char **argv){
	struct sockaddr_in server_addr;
	struct hostent *hp;
	SOCKET connect_sock;
	WSADATA wsaData;
	char tempBuff[BUFF_SIZE];
	DWORD ThreadId1;
	HANDLE ThreadHandle1;
	DWORD ThreadId2;
	HANDLE ThreadHandle2;
	g_hMutex = CreateMutex(NULL, FALSE, NULL);

	char			server_name[256];
	unsigned short	port = DEFAULT_PORT;
	unsigned int	addr;

	initscr();
	cbreak();
	echo();
	prompt = newwin(2, COLS, 0, 0);
	input = newwin(1, COLS, LINES - 2, 0);
	output = newwin(LINES - 4, COLS, 2, 0);
	frecv = newwin(1, COLS, LINES - 1, 0);
	wmove(prompt, 0, 0);
	/*wmove(frecv, LINES, 0);*/
	waddstr(frecv, "Welcome!");
	wrefresh(frecv);
	//wmove(frecv, LINES, 0);
	wmove(output, LINES - 5, 0);    /* start at the bottom */
	scrollok(output, TRUE);


	werase(frecv);
	waddstr(frecv, "Please input server address");
	wrefresh(frecv);
	mvwprintw(input, 0, 0, ">");
	if (wgetnstr(input, server_name, COLS - 4) != OK) {
		return -1;
	}
	werase(input);
	wrefresh(input);

	werase(frecv);
	waddstr(frecv, "Please input port number");
	wrefresh(frecv);
	mvwprintw(input, 0, 0, ">");
	if (wgetnstr(input, tempBuff, COLS - 4) != OK) {
		return -1;
	}
	port = atoi(tempBuff);
	werase(input);
	wrefresh(input);
	werase(frecv);
	waddstr(frecv, "Connecting server...");
	wrefresh(frecv);
	if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR){
		// stderr: standard error are printed to the screen.
		fprintf(stderr, "WSAStartup failed with error %d\n", WSAGetLastError());
		//WSACleanup function terminates use of the Windows Sockets DLL. 
		WSACleanup();
		return -1;
	}

	if (isalpha(server_name[0]))
		hp = gethostbyname(server_name);
	else{
		addr = inet_addr(server_name);
		hp = gethostbyaddr((char*)&addr, 4, AF_INET);
	}

	if (hp==NULL)
	{
		fprintf(stderr, "Cannot resolve address: %d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}

	//copy the resolved information into the sockaddr_in structure
	memset(&server_addr, 0, sizeof(server_addr));
	memcpy(&(server_addr.sin_addr), hp->h_addr, hp->h_length);
	server_addr.sin_family = hp->h_addrtype;
	server_addr.sin_port = htons(port);
	
	connect_sock = socket(AF_INET,SOCK_STREAM, 0);	//TCp socket


	if (connect_sock == INVALID_SOCKET){
		fprintf(stderr, "socket() failed with error %d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
	werase(prompt);
	sprintf(tempBuff, "Client connecting to: %s\n", hp->h_name);
	waddstr(prompt, tempBuff);
	wrefresh(prompt);
	//printf("Client connecting to: %s\n", hp->h_name);
	memset(tempBuff, 0, sizeof(tempBuff));
	if (connect(connect_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) 
		== SOCKET_ERROR){
		fprintf(stderr, "connect() failed with error %d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
	
	 /* result from wgetnstr has no newline */
	memset(userName, 0, sizeof(userName));
	werase(frecv);
	waddstr(frecv, "Please input your UserName");
	wrefresh(frecv);



	mvwprintw(input, 0, 0, ">");
	if (wgetnstr(input, userName, COLS - 4) != OK) {
		return -1;
	}
	werase(input);
	wrefresh(input);
	sprintf(inputPrefix, "[%s]>", userName);
	send(connect_sock, userName, sizeof(userName), 0);

	// create the threads
	ThreadHandle1 = CreateThread(NULL, 0, runner1, &connect_sock, 0, &ThreadId1);
	ThreadHandle2 = CreateThread(NULL, 0, runner2, &connect_sock, 0, &ThreadId2);
	WaitForSingleObject(ThreadHandle1, INFINITE);
	WaitForSingleObject(ThreadHandle2, INFINITE);
	CloseHandle(ThreadHandle1);
	CloseHandle(ThreadHandle2);
	CloseHandle(g_hMutex);
	closesocket(connect_sock);
	WSACleanup();
}