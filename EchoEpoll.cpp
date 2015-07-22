//EchoEpoll.cpp

#include<iostream>
#include<sys/epoll.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdio.h>
##include<errno.h>

#define MAX_EVENTS 1000;

class myevent
{
public:
	int fd;
	void (*callBack)(int fd, int events, void* arg);
	int events;
	void* arg;
	bool status;
	char buffer[128];
	int len;
	int offset;
	long  lastActiveTime;
};
//Add event 
void SetEvent(myevent* ev, int fd, void(*callBack)(int, int , void*), void* arg)
{
	ev->fd=fd;
	ev->callBack=callBack;
	ev->events=0;
	ev->arg=arg;
	ev->status=false;
	bzero(ev->buffer,sizeof ev->buffer);
	ev->offset=0;
	ev->len=0;
	ev->lastActiveTime=time(NULL);
}

void AddEvent(int epfd, int events, myevent* ev)
{
	struct epoll_event epv={0,{0}};

	epv.data.prt=ev;
	epv.events=ev->events=events;

	int op;
	if(ev->status)// have been added
		op=EPOLL_CTL_MOD;
	else
	{
		op=EPOLL_CTL_ADD;
		ev->status=true;
	}
	if(epoll_ctl(epfd,op,ev->fd,&epv)<0)
		printf("Event add failed  FD=%d,events=%d\n",ev->fd,events);
	else
		printf("Event add successfully,FD=%d,op=%d,events=%0x\n",ev-fd,op,events);

}

void DelEvent(int epfd, myevent* ev)
{
	struct epoll_event epv={0,{0}};
	if(!ev->status)//have not been added
		return;
	epv.data.ptr=ev;
	ev->status=false;
	epoll_ctl(epfd,EPOLL_CTL_DEL,ev->fd,&epv);
}

int g_epollFd;
myevent g_Events[MAX_EVENTS+1];
void RecvData(int fd, int events, void* arg);
void SendData(int fd, int events, void* arg);
void AcceptConnection(int fd, int events, void arg)
{
	struct sockaddr_in sin;
	socket_t len=sizeof(struct sockaddr_in);

	int infd=accept(fd,(struct sockaddr_in*)&sin,&len) ;
	if(nfd==-1)
	{
		if(errno!= EAGAIN&&errno!+EINTR)
		{}
		printf("%s, accept, %d",__func__,errno);

	}
	int i;
	do
	{
			for( i=0; i<MAX_EVENTS;++i)//find the ffirst unused Event
		{
			if(g_Events[i].status==false)
				break;
		}
		if(i==MAX_EVENTS)
		{
			printf("%s:max connection limit %d\n",__func__,MAX_EVENTS );
			break;//break do-while

		}
		int iret=fcntl(nfd,F_SETFL, O_NONBLOCK);
		if(iret<0)
		{
			printf("%s, fctl nonblocking failed:%d",__func__, iret);
			break;//break do-while
		}
		SetEvent(&g_Events[i], nfd, RecvData,&g_Events[i]);
		AddEvent(g_epollFd,EPOLLIN,&g_Events[i]);
	}while(0);
	printf("new connection[%s:%d] [time:%d], pos[%d]\n",inet_ntoa(sin.si_addr),ntohs(sin.sin_prot),g_Events[i].lastActiveTime,i);

}

void RecvData(int fd, int events, void* arg)
{
	struct  myevent* ev=(struct myevent*)arg;
	int len=recv(fd,ev->buffer+ev->len,sizeof(ev->buffer)-1-ev->len,0);
	DelEvent(g_epollFd,ev);
	if(len>0)
	{
		ev->len+=len;
		ev->buffer[len]='\0';
		printf("C[%d]:%s\n",fd,ev->buffer);
		SetEvent(ev,fd,SendData,ev);
		AddEvent(g_epollFd,EPOLLOUT,ev);
	}
	else if(len==0)
	{
		close(ev->fd);
		printf("[fd=%d] pos[%d], closed gracefully.\n",fd, ev->events);
	}
	else
	{
		close(ev->fd);
		printf("recv[fd=%d] errno[%d]:%s\n",fd,errno,strerror(errno));
	}
}

void SendData(int fd, int events, void* arg)
{
	int len=send(fd, ev->buffer+ev->offset,ev->len-ev->offset,0);
	if(len>0)
	{
		printf("send [fd=%d],[%d<->%d]%s\n",fd,len,ev->len,ev->buffer);
		ev->offset+=len;
		if(ev->offset==ev->len)
		{
			DelEvent(g_epollFd,ev);
			SetEvent(ev,fd,RecvData,ev);
			AddEvent(g_epollFd,EPOLLIN,ev);
		}
	}
	else
	{
		close(ev->fd);
		DelEvent(g_epollFd,ev);
		printf("send[fd=%d] errno[%d]\n",fd,errno);
	}
}