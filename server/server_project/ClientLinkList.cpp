#include "ClientLinkList.h"
#define BUFF_SIZE 4096

HANDLE  g_hMutex = NULL;
char userName[100] = { 0 };
/*
* function  初始化链表
* return    无返回值
*/
void Init()
{
	pClient head = (pClient)malloc(sizeof(Client)); //创建一个头结点
	head->next = NULL;
}

/*
* function  获取头节点
* return    返回头节点
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
* function	添加一个客户端
* param     client表示一个客户端对象
* return    无返回值
*/
void AddClient(pClient head, pClient client)
{
	client->next = head->next;  //比如：head->1->2,然后添加一个3进来后是
	head->next = client;        //3->1->2,head->3->1->2
}

/*
* function	删除一个客户端
* param     flag标识一个客户端对象
* return    返回0表示删除成功，-1表示失败
*/
int RemoveClient(pClient head, UINT_PTR flag)
{
	//从头遍历，一个个比较
	pClient pCur = head->next;//pCur指向第一个结点
	pClient pPre = head;      //pPre指向head 
	while (pCur)
	{
		// head->1->2->3->4,要删除2，则直接让1->3
		if (pCur->flag == flag)
		{
			pPre->next = pCur->next;
			closesocket(pCur->msg_sock);  //关闭套接字
			free(pCur);   //释放该结点
			pCur = NULL;
			return 0;
		}
		pPre = pCur;
		pCur = pCur->next;
	}
	return -1;
}

/*
* function  查找指定客户端
* param     name是指定客户端的用户名
* return    返回socket表示查找成功，返回INVALID_SOCKET表示无此用户
*/
SOCKET FindClient1(pClient head, char* name)
{
	//从头遍历，一个个比较
	pClient pCur = head;
	while (pCur = pCur->next)
	{
		if (strcmp(pCur->userName, name) == 0)
			return pCur->msg_sock;
	}
	return INVALID_SOCKET;
}

/*
* function  根据SOCKET查找指定客户端
* param     client是指定客户端的套接字
* return    返回一个pClient表示查找成功，返回NULL表示无此用户
*/
pClient FindClient2(pClient head, SOCKET client)
{
	//从头遍历，一个个比较
	pClient pCur = head;
	while (pCur = pCur->next)
	{
		if (pCur->msg_sock == client)
			return pCur;
	}
	return NULL;
}

/*
* function  计算客户端连接数
* param     client表示一个客户端对象
* return    返回连接数
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
* function  清空链表
* return    无返回值
*/
void ClearClient(pClient head)
{
	pClient pCur = head->next;
	pClient pPre = head;
	while (pCur)
	{
		//head->1->2->3->4,先删除1，head->2,然后free 1
		pClient p = pCur;
		pPre->next = p->next;
		free(p);
		pCur = pPre->next;
	}
}

/*
* function 检查连接状态并关闭一个连接
* return 返回值
*/
void CheckConnection(pClient head)
{
	char error[128] = { 0 };   //发送下线消息给发消息的人
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
				closesocket(pclient->msg_sock);   //这里简单的判断：若发送消息失败，则认为连接中断(其原因有多种)，关闭该套接字
				pclient->msg_sock = 0;
				RemoveClient(head, pclient->flag);
				break;
			}
		}
	}
}

/*
* function  指定发送给哪个客户端
* param     FromName，发信人
* param     ToName,   收信人
* param		data,	  发送的消息
*/
int SendData(pClient head,char* FromName, char* ToName, char* data)
{

	SOCKET client = FindClient1(head,ToName);   //查找是否有此用户
	char buf[128] = { 0 };
	char error[128] = { 0 };
	char temp[BUFF_SIZE] = { 0 };
	int ret = 0;
	if (client != INVALID_SOCKET){
		//添加发送消息的用户名
		ret = send(client, data, BUFF_SIZE, 0);
	}else {//发送错误消息给发消息的人
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
	if (ret == SOCKET_ERROR)//发送下线消息给发消息的人
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