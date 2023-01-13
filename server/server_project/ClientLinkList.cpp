#include "ClientLinkList.h"
#define BUFF_SIZE 4096

HANDLE  g_hMutex = NULL;
char userName[100] = { 0 };
/*
* function  ��ʼ������
* return    �޷���ֵ
*/
void Init()
{
	pClient head = (pClient)malloc(sizeof(Client)); //����һ��ͷ���
	head->next = NULL;
}

/*
* function  ��ȡͷ�ڵ�
* return    ����ͷ�ڵ�
*/
//pClient GetHeadNode(pClient head)
//{
//	return head;
//}

int PrintClient(pClient head){
	pClient pCur = head;
	if(pCur->next == NULL){
		printf("No users now.\n");
		return 0;
	}
	while (pCur = pCur->next)
	{
		printf("IP address: %s, port %d, user name: %s, group: %s, flag: %d\n",
		pCur->IP,
		pCur->Port,
		pCur->userName,
		pCur->group,
		pCur->flag);
	}
	return 0;
}

/*
* function	���һ���ͻ���
* param     client��ʾһ���ͻ��˶���
* return    �޷���ֵ
*/
void AddClient(pClient head, pClient client)
{
	client->next = head->next;  //���磺head->1->2,Ȼ�����һ��3��������
	head->next = client;        //3->1->2,head->3->1->2
}

/*
* function	ɾ��һ���ͻ���
* param     flag��ʶһ���ͻ��˶���
* return    ����0��ʾɾ���ɹ���-1��ʾʧ��
*/
int RemoveClient(pClient head, UINT_PTR flag)
{
	//��ͷ������һ�����Ƚ�
	pClient pCur = head->next;//pCurָ���һ�����
	pClient pPre = head;      //pPreָ��head 
	while (pCur)
	{
		// head->1->2->3->4,Ҫɾ��2����ֱ����1->3
		if (pCur->flag == flag)
		{
			pPre->next = pCur->next;
			closesocket(pCur->msg_sock);  //�ر��׽���
			free(pCur);   //�ͷŸý��
			pCur = NULL;
			return 0;
		}
		pPre = pCur;
		pCur = pCur->next;
	}
	return -1;
}

/*
* function  ����ָ���ͻ���
* param     name��ָ���ͻ��˵��û���
* return    ����socket��ʾ���ҳɹ�������INVALID_SOCKET��ʾ�޴��û�
*/
SOCKET FindClient1(pClient head, char* name)
{
	//��ͷ������һ�����Ƚ�
	pClient pCur = head;
	while (pCur = pCur->next)
	{
		if (strcmp(pCur->userName, name) == 0)
			return pCur->msg_sock;
	}
	return INVALID_SOCKET;
}

/*
* function  ����SOCKET����ָ���ͻ���
* param     client��ָ���ͻ��˵��׽���
* return    ����һ��pClient��ʾ���ҳɹ�������NULL��ʾ�޴��û�
*/
pClient FindClient2(pClient head, SOCKET client)
{
	//��ͷ������һ�����Ƚ�
	pClient pCur = head;
	while (pCur = pCur->next)
	{
		if (pCur->msg_sock == client)
			return pCur;
	}
	return NULL;
}

/*
* function  ����ͻ���������
* param     client��ʾһ���ͻ��˶���
* return    ����������
*/
int CountCon(pClient head)
{
	int iCount = 0;
	pClient pCur = head;
	while (pCur = pCur->next)
		iCount++;
	return iCount;
}

/*
* function  �������
* return    �޷���ֵ
*/
void ClearClient(pClient head)
{
	pClient pCur = head->next;
	pClient pPre = head;
	while (pCur)
	{
		//head->1->2->3->4,��ɾ��1��head->2,Ȼ��free 1
		pClient p = pCur;
		pPre->next = p->next;
		free(p);
		pCur = pPre->next;
	}
}

/*
* function �������״̬���ر�һ������
* return ����ֵ
*/
void CheckConnection(pClient head)
{
	char error[128] = { 0 };   //����������Ϣ������Ϣ����
	pClient pclient = head;
	while (pclient = pclient->next)
	{
		if (send(pclient->msg_sock, "", sizeof(""), 0) == SOCKET_ERROR)
		{
			if (pclient->msg_sock != 0)
			{
				if (strlen(pclient->userName) != 0) {
					if (strlen(pclient->group) != 0) {
						printf("Disconnect from IP: %s, UserName: %s, Group: %s\n", pclient->IP, pclient->userName, pclient->group);
					}
					else {
						printf("Disconnect from IP: %s, UserName: %s\n", pclient->IP, pclient->userName);
					}
				}
				sprintf(error, "%s was offline", pclient->userName);
				send(FindClient1(head, pclient->ChatName), error, sizeof(error), 0);
				closesocket(pclient->msg_sock);   //����򵥵��жϣ���������Ϣʧ�ܣ�����Ϊ�����ж�(��ԭ���ж���)���رո��׽���
				pclient->msg_sock = 0;
				RemoveClient(head, pclient->flag);
				break;
			}
		}
	}
}

/*
* function  ָ�����͸��ĸ��ͻ���
* param     FromName��������
* param     ToName,   ������
* param		data,	  ���͵���Ϣ
*/
int SendData(pClient head,char* FromName, char* ToName, char* data)
{

	SOCKET client = FindClient1(head,ToName);   //�����Ƿ��д��û�
	char buf[128] = { 0 };
	char error[128] = { 0 };
	char temp[BUFF_SIZE] = { 0 };
	int ret = 0;
	if (client != INVALID_SOCKET){
		//��ӷ�����Ϣ���û���
		ret = send(client, data, BUFF_SIZE, 0);
	}else {//���ʹ�����Ϣ������Ϣ����
		sprintf(temp, "%s", data);
		char* ptr = strtok(temp, "|");
		if (ptr && strcmp(ptr, "?FIN") == 0) {
			if (client == INVALID_SOCKET)
				sprintf(error, "%s is not online!", ToName);
			else
				sprintf(error, "Message sent to %s is not allowed empty. Please try again!", ToName);

			send(FindClient1(head, FromName), error, sizeof(error), 0);
			return 1;
		}

		if (client == INVALID_SOCKET) {
			sprintf(error, "%s is not online!", ToName);
		}
		/*else {
			sprintf(error, "Message sent to %s is not allowed empty. Please try again!\n", ToName);
		}*/
		send(FindClient1(head, FromName), error, sizeof(error), 0);
	}
	if (ret == SOCKET_ERROR)//����������Ϣ������Ϣ����
	{
		sprintf(temp, "%s", data);
		char* ptr = strtok(temp, "|");
		if (ptr && strcmp(ptr, "?FIN") == 0) {
			sprintf(error, "%s is offline!", ToName);
			send(FindClient1(head, FromName), error, sizeof(error), 0);
			return 1;
		}
	}
	return 0;
}