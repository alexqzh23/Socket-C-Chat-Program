// Server.cpp : create a console application, and include the sources in the project
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
#include <windows.h>
#include "ClientLinkList.h"

#define DEFAULT_PORT	5019
#define BUFF_SIZE 4096

pClient head;
SOCKET sock;
struct sockaddr_in client_addr = { 0 };
int addr_len = sizeof(client_addr); // Length of client address
char manage[BUFF_SIZE] = { 0 };
char groupList[BUFF_SIZE];
char userList[BUFF_SIZE];

void InsertNode(Group** phead, char* x);

void group(char* string, char* groupname);

int printAllClients();

int printALLGroups();

char* attachAllGroups();

char* attachAllUsers();

void DeleteAllgroup(char* groupname);

int changeClientGroup(int clientID, char* groupname);

DWORD WINAPI threadManage(LPVOID param)
{
	char clientID[20];//clientID that administrator input
	int holder;//to catch the return value from RemoveClient() ('0' means remove successfully '1'means remove falied).
	int clientID1;//the int type of the clientID
	int state;//catch the return value from printAllClients() ('0' means there is at least one client connected with the server; '1' means no clients connected with the server)
	char groupmember[100];//the groupmember that administrator input
	char groupname[16];//the groupname that administrator input

	while (1) {
		char command[100] = { 0 };
		fprintf(stderr, "\nAs an administrator you can do some operations:\n");
		fprintf(stderr, "(Enter D): Delete a client or a group from the server\n(Enter G): Group some clients into a group\n(Enter L): List all the clients connecting with the server\n(Enter C): you can change a client to other groups\n\n");
		Sleep(200);
		gets(command);
		printf("\n");
		if (strcmp(command, "D") == 0) {//command "D"
			char select[10] = { 0 };
			fprintf(stderr, "\n(Enter 1): Delete a client\n(Enter 2): Delete a group\n");
			gets(select);
			if (strcmp(select, "1") == 0) {//enter the subcommand '1'
				fprintf(stderr, "\nHere are all the clients in the server:\n");
				state = printAllClients();
				if (state == 1) {
					continue;//if there is no clients connected with the server then directly bcak to main menu;
				}
				fprintf(stderr, "\nPlease input the client ID you want to delete: \n");
				gets(clientID);
				clientID1 = atoi(clientID);
				holder = RemoveClient(head, clientID1);//remove the client from the server.
				if (holder == 0) {
					fprintf(stderr, "\nThe client（ID = %s） has already been deleted from server!\n", clientID);
				}
				else {
					fprintf(stderr, "\nThe client does not exist in the server!\n");
				}
				continue;
			}
			else if (strcmp(select, "2") == 0) {//enter the subcommand '2'
				fprintf(stderr, "\nHere are all the groups in the server with their group name:\n");
				if (printALLGroups() == 0) {
					continue;
				};//show all the groups' name we have in the server now
				fprintf(stderr, "\nEnter the name of the group you want to delete!\n");
				gets(groupname);
				DeleteAllgroup(groupname);
			}
			else {
				printf("\nPlease input the valid command number!\n");
			}
		}
		else if (strcmp(command, "L") == 0) {//command "L"
			printAllClients();
		}
		else if (strcmp(command, "G") == 0) {//command "G"
			fprintf(stderr, "\nHere are all the clients in the server:\n");
			state = printAllClients();//show all clients in the server
			if (state == 1) {
				continue;//if there is no clients connected with the server then directly bcak to main menu;
			}
			fprintf(stderr, "\nPlease input the clients IDs you want to group(split the ID with the \"-\")\n");
			gets(groupmember);
			fprintf(stderr, "\nPlease input a group name\n");
			gets(groupname);
			group(groupmember, groupname);
			continue;
		}
		else if (strcmp(command, "C") == 0) {//command "C"
			state = printAllClients();
			if (state == 1) {
				continue;//if there is no clients connected with the server then directly bcak to main menu;
			}
			fprintf(stderr, "\nYou need to input the client ID that you want to operate: \n");
			gets(clientID);
			clientID1 = atoi(clientID);
			fprintf(stderr, "\nYou need to input the group name that you want to change: \n");
			gets(groupname);
			if (changeClientGroup(clientID1, groupname) == 0) {
				printf("\nclient does not exist!\n");
			}
			continue;
		}
		else {
			fprintf(stderr, "\nplease input the right the command!\n");
			continue;
		}
	}
}

DWORD WINAPI threadCheck(void* param)
{
	while (1)
	{
		CheckConnection(head);  //检查连接状况
		Sleep(2000);		//2s检查一次
	}

	return 0;
}

// The function executed by the new thread.
DWORD WINAPI runner(LPVOID param) {
	int msg_len = 0;
	char buf[BUFF_SIZE] = { 0 };
	char notice1[BUFF_SIZE] = { 0 };
	char notice2[BUFF_SIZE] = { 0 };
	char temp[BUFF_SIZE] = { 0 };
	int GFileflag = 0;
	long GFileLength = 0;
	int PFileflag = 0;
	long PFileLength = 0;
	pSend psend;
	pClient phead;
	pClient pclient = (pClient)param;
	memset(pclient->userName, 0, sizeof(pclient->userName));

	recv(pclient->msg_sock, pclient->userName, sizeof(pclient->userName), 0); //接收用户名

	memcpy(pclient->IP, inet_ntoa(client_addr.sin_addr), sizeof(pclient->IP)); //记录客户端IP
	pclient->flag = pclient->msg_sock; //不同的socket有不同UINT_PTR类型的数字来标识
	pclient->Port = htons(client_addr.sin_port);
	memset(pclient->group, 0, sizeof(pclient->group));

	if (strlen(pclient->userName) == 0) {
		return -1;
	}

	AddClient(head, pclient); //把新的客户端加入链表中

	printf("accepted connection from %s, port %d, user name: %s\n\n",
		pclient->IP,
		pclient->Port,
		pclient->userName);

	while (1) {
		if (!pclient) {
			return -1;
		}

		msg_len = recv(pclient->msg_sock, pclient->buf, sizeof(pclient->buf), 0); // Number of bytes for each recv call

		if (msg_len == SOCKET_ERROR) {
			//fprintf(stderr, "recv() failed with error %d\n", WSAGetLastError());
			//WSACleanup();
			return -1;
		}

		if (msg_len == 0) {
			printf("Client closed connection\n");
			closesocket(pclient->msg_sock);
			return -1;
		}


		if (pclient->buf[0] == '#' && pclient->buf[1] != '#') //#用户指定另一个用户进行聊天
		{
			SOCKET socket = FindClient1(head, &pclient->buf[1]); //验证客户是否存在
			if (socket != INVALID_SOCKET)
			{
				pClient c = (pClient)malloc(sizeof(Client));
				c = FindClient2(head, socket); //改变ChatName,发送消息的时候自动发给指定的用户
				memset(pclient->ChatName, 0, sizeof(pclient->ChatName));
				memcpy(pclient->ChatName, c->userName, sizeof(pclient->ChatName));
				sprintf(notice1, "Your new chat friend is %s", pclient->ChatName);
				send(pclient->msg_sock, notice1, 64, 0);
			}
			else {
				sprintf(notice1, "User %s is not on the server now!", &pclient->buf[1]);
				send(pclient->msg_sock, notice1, sizeof(notice1), 0);
				memcpy(pclient->ChatName, &pclient->buf[1], sizeof(pclient->ChatName));
			}
			continue;
		}
		else if (pclient->buf[0] == '$' && pclient->buf[1] != '$') {
			memset(pclient->group, 0, sizeof(pclient->group));
			memcpy(pclient->group, &pclient->buf[1], sizeof(pclient->group));
			memset(notice2, 0, sizeof(notice2));
			sprintf(notice2, "Your new group is %s", pclient->group);
			send(pclient->msg_sock, notice2, 64, 0);
		}
		else if (strcmp(pclient->buf, "(printName)") == 0) {
			memset(notice1, 0, sizeof(notice2));
			sprintf(notice1, "Online users: %s", attachAllUsers());
			send(pclient->msg_sock, notice1, BUFF_SIZE, 0);
		}
		else if (strcmp(pclient->buf, "(printGroup)") == 0) {
			memset(notice2, 0, sizeof(notice2));
			sprintf(notice2, "Existing groups: %s", attachAllGroups());
			send(pclient->msg_sock, notice2, BUFF_SIZE, 0);
		}
		else if (strcmp(pclient->buf, "(group)") == 0) { // group chat
			pclient = (pClient)param;
			int fileState = 0;
			memset(pclient->buf, 0, sizeof(pclient->buf));
			msg_len = recv(pclient->msg_sock, pclient->buf, sizeof(pclient->buf), 0); // Number of bytes for each recv call

			strcpy(temp, pclient->buf);
			char* ptr = strtok(temp, "|");
			if (ptr && strcmp(ptr, "?FIN") == 0) {
				char* filename = strtok(NULL, "|");
				long fileSize = strtol(strtok(NULL, "|"), NULL, 10);

				phead = head->next;
				while (phead != NULL) {
					//组内群发
					psend = (pSend)malloc(sizeof(Send));
					memcpy(psend->FromName, pclient->userName, sizeof(psend->FromName));
					memcpy(psend->ToName, phead->userName, sizeof(psend->ToName));
					memcpy(psend->data, pclient->buf, sizeof(psend->data));
					if (strcmp(psend->FromName, psend->ToName) != 0 && strcmp(pclient->group, phead->group) == 0) {
						SendData(head, psend->FromName, psend->ToName, psend->data);
					}
					memset(psend->data, 0, sizeof(psend->data));
					phead = phead->next;
				}
				memset(pclient->buf, 0, sizeof(pclient->buf));
				/*if (fileState != 0) {
					continue;
				}*/
				do {
					msg_len = recv(pclient->msg_sock, pclient->buf, sizeof(pclient->buf), 0);

					phead = head->next;
					while (phead != NULL) {
						//组内群发
						psend = (pSend)malloc(sizeof(Send));
						memcpy(psend->FromName, pclient->userName, sizeof(psend->FromName));
						memcpy(psend->ToName, phead->userName, sizeof(psend->ToName));
						memcpy(psend->data, pclient->buf, sizeof(psend->data));
						if (strcmp(psend->FromName, psend->ToName) != 0 && strcmp(pclient->group, phead->group) == 0) {
							SendData(head, psend->FromName, psend->ToName, psend->data); //发送数据
						}
						memset(psend->data, 0, sizeof(psend->data));
						phead = phead->next;
					}
					fileSize -= msg_len;
					memset(pclient->buf, 0, sizeof(pclient->buf));
				} while (fileSize > 0);

				ptr = NULL;
				sprintf(pclient->buf, "send a file \"%s\" to you", filename);
				Sleep(100);
			}

			sprintf(buf, "(G)[%s]: %s", pclient->userName, pclient->buf);

			phead = head->next;
			while (phead != NULL) {
				//组内群发
				psend = (pSend)malloc(sizeof(Send));
				memcpy(psend->FromName, pclient->userName, sizeof(psend->FromName));
				memcpy(psend->ToName, phead->userName, sizeof(psend->ToName));
				memcpy(psend->data, buf, sizeof(psend->data));
				if (strcmp(psend->FromName, psend->ToName) != 0 && strcmp(pclient->group, phead->group) == 0) {
					SendData(head, psend->FromName, psend->ToName, psend->data); //发送数据
				}
				memset(psend->data, 0, sizeof(psend->data));
				phead = phead->next;
			}
			memset(pclient->buf, 0, sizeof(pclient->buf));
			memset(buf, 0, sizeof(buf));
		}
		else if (strlen(pclient->buf) != 0) { // private chat
			/*puts(pclient->buf);*/
			strcpy(temp, pclient->buf);
			char* ptr = strtok(temp, "|");
			if (ptr && strcmp(ptr, "?FIN") == 0) {
				char* filename = strtok(NULL, "|");
				long fileSize = strtol(strtok(NULL, "|"), NULL, 10);

				psend = (pSend)malloc(sizeof(Send));
				memcpy(psend->FromName, pclient->userName, sizeof(psend->FromName));
				memcpy(psend->ToName, pclient->ChatName, sizeof(psend->ToName));
				memcpy(psend->data, pclient->buf, sizeof(psend->data));
				memset(pclient->buf, 0, sizeof(pclient->buf));

				SendData(head, psend->FromName, psend->ToName, psend->data);
				memset(psend->data, 0, sizeof(psend->data));

				do {
					memset(psend->data, 0, sizeof(psend->data));
					memset(pclient->buf, 0, sizeof(pclient->buf));
					msg_len = recv(pclient->msg_sock, pclient->buf, sizeof(pclient->buf), 0);

					psend = (pSend)malloc(sizeof(Send));
					memcpy(psend->FromName, pclient->userName, sizeof(psend->FromName));
					memcpy(psend->ToName, pclient->ChatName, sizeof(psend->ToName));
					memcpy(psend->data, pclient->buf, sizeof(psend->data));
					//strcpy(psend->data, pclient->buf);
					SendData(head, psend->FromName, psend->ToName, psend->data);
					//printf("S:%d\n", fileSize);
					fileSize -= msg_len;
					memset(psend->data, 0, sizeof(psend->data));
					memset(pclient->buf, 0, sizeof(pclient->buf));
					Sleep(100);
				} while (fileSize > 0);

				ptr = NULL;
				sprintf(pclient->buf, "send a file \"%s\" to you", filename);
				Sleep(100);
			}
			//把发送人的用户名和接收消息的用户和消息赋值给结构体，然后当作参数传进发送消息进程中
			psend = (pSend)malloc(sizeof(Send));
			memcpy(psend->FromName, pclient->userName, sizeof(psend->FromName));
			memcpy(psend->ToName, pclient->ChatName, sizeof(psend->ToName));
			sprintf(buf, "(P)[%s]: %s", pclient->userName, pclient->buf);
			memcpy(psend->data, buf, sizeof(psend->data));
			SendData(head, psend->FromName, psend->ToName, psend->data);
			memset(psend->data, 0, sizeof(psend->data));
			memset(buf, 0, sizeof(buf));
		}

		memset(pclient->buf, 0, sizeof(pclient->buf));
	}

	return 0; // New thread ends.
}

int main(int argc, char** argv) {
	struct sockaddr_in local;
	WSADATA wsaData;

	HANDLE ThreadHandle0;
	HANDLE ThreadHandle1;

	head = (pClient)malloc(sizeof(Client)); //创建一个头结点
	head->next = NULL;

	if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR) {
		// stderr: standard error are printed to the screen.
		fprintf(stderr, "WSAStartup failed with error %d\n", WSAGetLastError());
		//WSACleanup function terminates use of the Windows Sockets DLL. 
		WSACleanup();
		return -1;
	}

	memset(&local, 0, sizeof(local));

	// Fill in the address structure (Create local (server) socket address)
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY; // Default IP address
	local.sin_port = htons(DEFAULT_PORT); // Default port
	// Create listen socket (return listen socket descriptor)
	sock = socket(AF_INET, SOCK_STREAM, 0);	//TCp socket

	if (sock == INVALID_SOCKET) {
		fprintf(stderr, "socket() failed with error %d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
	// Bind listen socket to the local socket address
	if (bind(sock, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR) {
		fprintf(stderr, "bind() failed with error %d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}

	//waiting for the connections
	if (listen(sock, 5) == SOCKET_ERROR) {
		fprintf(stderr, "listen() failed with error %d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}

	printf("Waiting for connections ........\n");

	ThreadHandle0 = CreateThread(NULL, 0, threadManage, NULL, 0, NULL);
	CloseHandle(ThreadHandle0);

	ThreadHandle1 = CreateThread(NULL, 0, threadCheck, NULL, 0, NULL);
	CloseHandle(ThreadHandle1);

	while (1)
	{
		//创建一个新的客户端对象
		pClient pclient = (pClient)malloc(sizeof(Client));

		// Accept connections from client
		pclient->msg_sock = accept(sock, (struct sockaddr*)&client_addr, &addr_len);

		if (pclient->msg_sock == INVALID_SOCKET) {
			fprintf(stderr, "accept() failed with error %d\n", WSAGetLastError());
			closesocket(sock);
			WSACleanup();
			return -1;
		}

		CreateThread(NULL, 0, runner, pclient, 0, NULL);
	}

	ClearClient(head);
	closesocket(sock);
	WSACleanup();
	return 0;
}

// newly added number will be put at the head of list
void InsertNode(Group** phead, char* x) {
	Group* newGroup = (Group*)malloc(sizeof(Group));
	if (newGroup != NULL) {
		newGroup->data = (char*)malloc((strlen(x) + 1) * sizeof(char));
		newGroup->next = NULL;
		strcpy(newGroup->data, x);

		if (!*phead) {
			*phead = newGroup;
		}
		else {
			Group* temp = *phead;
			if (strcmp(temp->data, x) == 0) {
				free(newGroup);//free the memory
				return;
			}
			Group* prehead = NULL;
			while (temp) {
				if (strcmp(temp->data, x) == 0) {
					free(newGroup);//free the memory
					return;
				}
				prehead = temp;
				temp = temp->next;//point to the next node
			}
			prehead->next = newGroup;
		}
	}
}

void group(char* string, char* groupname) {
	char* splitor = "-";
	char* token;
	int i = 0;
	int flag = 0;
	char newGroup[100] = { 0 };
	token = strtok(string, splitor);//the first substring
	while (token != NULL) {
		pClient pCur = head->next;//pCur指向第一个结点
		while (pCur != NULL) {
			if (pCur->flag == atoi(token)) {
				flag = 1;
				strcpy(pCur->group, groupname);
				sprintf(newGroup, "$$%s", groupname);
				send(pCur->msg_sock, newGroup, 100, 0);
				break;
			}
			pCur = pCur->next;
		}
		if (flag == 0) {
			printf("Input ID: %s is not found!\n", token);
		}
		else {
			printf("Input ID: %s is grouped into %s!\n", token, groupname);
		}
		flag = 0;
		token = strtok(NULL, splitor);
	}
}

int printAllClients() {
	pClient pCur = head->next;//pCur指向第一个结点
	pClient pPre = head;      //pPre指向head 
	if (pCur == NULL) {//No any clients in the server! 
		fprintf(stderr, "No any clients in the server!\n");
		return 1;
	}
	while (pCur)//recursively print the clients info
	{
		printf("Client IP address: %s\nClient port %d\nClient user name: %s\nClient group: %s\nClient ID: %d\n",
			pCur->IP,
			pCur->Port,
			pCur->userName,
			pCur->group,
			pCur->flag);
		//point to next clients
		pPre = pCur;
		pCur = pCur->next;
		printf("\n");
	}
	return 0;
}

int printALLGroups() {
	pClient pCur = head->next;//pCur指向第一个结点
	pClient pPre = head;      //pPre指向head 
	int length = -1;
	Group* phead = NULL;
	int flag = 0;//defalut value is no groups
	if (pCur == NULL) {//No any groups in the server! 
		fprintf(stderr, "No any groups in the server!\n");
		return flag;
	}
	flag = 1;
	while (pCur)//recursively print the clients info
	{
		if (strlen(pCur->group) != 0) {
			InsertNode(&phead, pCur->group);
		}
		pCur = pCur->next;
	}
	if (phead == NULL) {//No any groups in the server! 
		fprintf(stderr, "No any groups in the server!\n");
		flag = 0;
		return flag;
	}
	printf("|");

	Group* nextGroup = NULL;
	while (phead) {
		nextGroup = phead->next; // save the address of next node
		printf(" %s |", phead->data);
		free(phead->data);
		free(phead);
		phead = nextGroup;
	}
	printf("\n");
	return flag;
}

char* attachAllGroups() {
	pClient pCur = head->next;//pCur指向第一个结点
	pClient pPre = head;      //pPre指向head 
	int length = -1;
	Group* head = NULL;
	int flag = 0;//defalut value is no groups

	memset(groupList, 0, sizeof(groupList));

	if (pCur == NULL) {//No any groups in the server! 
		strcpy(groupList, "No any groups on the server now!");
		return groupList;
	}
	flag = 1;
	while (pCur)//recursively print the clients info
	{
		InsertNode(&head, pCur->group);
		pCur = pCur->next;
	}
	Group* nextGroup = NULL;
	char* p = groupList;
	strcpy(p, "|");
	p += strlen("|");
	while (head) {
		nextGroup = head->next; // save the address of next node
		sprintf(p, " %s |", head->data);
		p += strlen(head->data) + 1;
		free(head->data);
		free(head);
		head = nextGroup;
	}
	return groupList;
}

char* attachAllUsers() {
	pClient pCur = head->next;//pCur指向第一个结点
	pClient pPre = head;      //pPre指向head 
	int length = -1;
	Group* head = NULL;;
	int flag = 0;//defalut value is no groups

	memset(groupList, 0, sizeof(groupList));

	if (pCur == NULL) {//No any groups in the server! 
		strcpy(userList, "No any users on the server now!");
		return userList;
	}
	flag = 1;
	while (pCur)//recursively print the clients info
	{
		InsertNode(&head, pCur->userName);
		pCur = pCur->next;
	}
	User* nextUser = NULL;
	char* p = userList;
	strcpy(p, "|");
	p += strlen("|");
	while (head) {
		nextUser = head->next; // save the address of next node
		sprintf(p, " %s |", head->data);
		p += strlen(head->data) + 1;
		free(head->data);
		free(head);
		head = nextUser;
	}
	return userList;
}

void DeleteAllgroup(char* groupname) {
	pClient pCur = head->next;//pCur指向第一个结点
	while (pCur) {
		printf("%s*", pCur->userName);
		if (strcmp(pCur->group, groupname) == 0) {
			pClient temp = pCur;// use a temp to point the same node as pCur had pointed.
			pCur = pCur->next;//move pCur to the next client.
			RemoveClient(head, temp->flag);
			continue;//after delete a node continue check next client.
		}
		pCur = pCur->next;
	}
	printf("successfuly delete the group: %s", groupname);
}

int changeClientGroup(int clientID, char* groupname) {
	pClient pCur = head->next;//pCur指向第一个结点
	int flag = 0;//indicate the client is not exits (defalut value)
	char newGroup[100] = { 0 };
	while (pCur) {
		if (pCur->flag == clientID) {
			strcpy(pCur->group, groupname);
			flag = 1;
			printf("Successfully change client: %s to group: %s \n", pCur->userName, groupname);//printf message to show it works
			sprintf(newGroup, "$$%s", groupname);
			send(pCur->msg_sock, newGroup, 100, 0);
			return flag;
		}
		pCur = pCur->next;//point to the next node;
	}
	return flag;
}