#include <WinSock2.h>
#include <stdio.h>
#include <mswsock.h>
#include <limits.h>
#include <windows.h>

#ifndef _CLIENT_LINK_LIST_H_
#define _CLIENT_LINK_LIST_H_
#define BUFF_SIZE 4096

typedef struct node {
	char* data;
	struct node* next;
} Node;

typedef struct group {
	char* data;
	struct node* next;
} Group, User;

typedef struct _Send
{
	char FromName[16];
	char ToName[16];
	char data[BUFF_SIZE];
}Send, * pSend;

//客户端信息结构体
typedef struct _Client
{
	SOCKET msg_sock;		//客户端套接字
	char buf[BUFF_SIZE];			//数据缓冲区
	char userName[16];		//客户端用户名
	char IP[20];			//客户端IP
	unsigned short Port;	//客户端端口
	UINT_PTR flag;			//标记客户端，用来区分不同的客户端
	char ChatName[16];		//指定要和哪个客户端聊天
	char group[16];			//组名
	struct _Client *next;	//指向下一个结点
}Client, *pClient;

/*
* function  初始化链表
* return    无返回值
*/
void Init();

/*
* function  获取头节点
* return    返回头节点
*/
//pClient GetHeadNode(pClient head);

int PrintClient(pClient head);

/*
* function	添加一个客户端
* param     client表示一个客户端对象
* return    无返回值
*/
void AddClient(pClient head, pClient client);

/*
* function	删除一个客户端
* param     flag标识一个客户端对象
* return    返回0表示删除成功，-1表示失败
*/
int RemoveClient(pClient head, UINT_PTR flag);

/*
* function  根据name查找指定客户端
* param     name是指定客户端的用户名
* return    返回一个client表示查找成功，返回INVALID_SOCKET表示无此用户
*/
SOCKET FindClient1(pClient head, char* name);

/*
* function  根据SOCKET查找指定客户端
* param     client是指定客户端的套接字
* return    返回一个pClient表示查找成功，返回NULL表示无此用户
*/
pClient FindClient2(pClient head, SOCKET client);

/*
* function  计算客户端连接数
* param     client表示一个客户端对象
* return    返回连接数
*/
int CountCon(pClient head);

/*
* function  清空链表
* return    无返回值
*/
void ClearClient(pClient head);

/*
* function  检查连接状态并关闭一个连接
* return 返回值
*/
void CheckConnection(pClient head);

/*
* function  指定发送给哪个客户端
* param     FromName，发信人
* param     ToName,   收信人
* param		data,	  发送的消息
*/
int SendData(pClient head, char* FromName, char* ToName, char* data);

int sendFile(SOCKET connect_sock);

#endif //_CLIENT_LINK_LIST_H_