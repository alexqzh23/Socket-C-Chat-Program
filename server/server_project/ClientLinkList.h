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

//�ͻ�����Ϣ�ṹ��
typedef struct _Client
{
	SOCKET msg_sock;		//�ͻ����׽���
	char buf[BUFF_SIZE];			//���ݻ�����
	char userName[16];		//�ͻ����û���
	char IP[20];			//�ͻ���IP
	unsigned short Port;	//�ͻ��˶˿�
	UINT_PTR flag;			//��ǿͻ��ˣ��������ֲ�ͬ�Ŀͻ���
	char ChatName[16];		//ָ��Ҫ���ĸ��ͻ�������
	char group[16];			//����
	struct _Client *next;	//ָ����һ�����
}Client, *pClient;

/*
* function  ��ʼ������
* return    �޷���ֵ
*/
void Init();

/*
* function  ��ȡͷ�ڵ�
* return    ����ͷ�ڵ�
*/
//pClient GetHeadNode(pClient head);

int PrintClient(pClient head);

/*
* function	���һ���ͻ���
* param     client��ʾһ���ͻ��˶���
* return    �޷���ֵ
*/
void AddClient(pClient head, pClient client);

/*
* function	ɾ��һ���ͻ���
* param     flag��ʶһ���ͻ��˶���
* return    ����0��ʾɾ���ɹ���-1��ʾʧ��
*/
int RemoveClient(pClient head, UINT_PTR flag);

/*
* function  ����name����ָ���ͻ���
* param     name��ָ���ͻ��˵��û���
* return    ����һ��client��ʾ���ҳɹ�������INVALID_SOCKET��ʾ�޴��û�
*/
SOCKET FindClient1(pClient head, char* name);

/*
* function  ����SOCKET����ָ���ͻ���
* param     client��ָ���ͻ��˵��׽���
* return    ����һ��pClient��ʾ���ҳɹ�������NULL��ʾ�޴��û�
*/
pClient FindClient2(pClient head, SOCKET client);

/*
* function  ����ͻ���������
* param     client��ʾһ���ͻ��˶���
* return    ����������
*/
int CountCon(pClient head);

/*
* function  �������
* return    �޷���ֵ
*/
void ClearClient(pClient head);

/*
* function  �������״̬���ر�һ������
* return ����ֵ
*/
void CheckConnection(pClient head);

/*
* function  ָ�����͸��ĸ��ͻ���
* param     FromName��������
* param     ToName,   ������
* param		data,	  ���͵���Ϣ
*/
int SendData(pClient head, char* FromName, char* ToName, char* data);

int sendFile(SOCKET connect_sock);

#endif //_CLIENT_LINK_LIST_H_