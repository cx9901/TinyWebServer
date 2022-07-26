#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "./threadpool/threadpool.h"
#include "./http/http_conn.h"

const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数
const int TIMESLOT = 5;             //最小超时单位

class WebServer
{
public:
    WebServer();
    ~WebServer();

    void init(int port , string user, string passWord, string databaseName,
              int log_write , int opt_linger, int trigmode, int sql_num,
              int thread_num, int close_log, int actor_model); //初始化

    void thread_pool(); //线程池
    void sql_pool(); //数据库
    void log_write(); //日志
    void trig_mode(); //触发模式
    void eventListen(); //监听
    void eventLoop(); //运行
    void init_connn_and_timer(int connfd, struct sockaddr_in client_address);
    void adjust_timer(util_timer *timer);
    void deal_timer(util_timer *timer, int sockfd);
    bool dealclinetdata();
    bool dealwithsignal(bool& timeout, bool& stop_server);
    void dealwithread(int sockfd);
    void dealwithwrite(int sockfd);

public:
    //基础
    int m_port; //端口
    char *m_root; //根目录文件夹路径
    int m_log_write; //日志写入方式：0，同步写入  1，异步写入
    int m_close_log; //是否关闭日志
    int m_actormodel; //并发模型选择，选择反应堆模型：0，Proactor模型  1，Reactor模型

    int m_pipefd[2];
    int m_epollfd;
    http_conn *users;

    //数据库相关
    connection_pool *m_connPool;
    string m_user;         //登陆数据库用户名
    string m_passWord;     //登陆数据库密码
    string m_databaseName; //使用数据库名
    int m_sql_num;         //数据库连接池数量

    //线程池相关
    threadpool<http_conn> *m_pool;
    int m_thread_num; //线程池内的线程数量

    //epoll_event相关
    epoll_event events[MAX_EVENT_NUMBER];

    int m_listenfd;
    int m_OPT_LINGER; //优雅关闭链接
    int m_TRIGMode; //触发组合模式 listenfd和connfd的模式组合：0，表示使用LT + LT 1，表示使用LT + ET 2，表示使用ET + LT 3，表示使用ET + ET
    int m_LISTENTrigmode; //listenfd触发模式
    int m_CONNTrigmode; //connfd触发模式

    //定时器相关
    client_data *users_timer;
    Utils utils;
};
#endif
