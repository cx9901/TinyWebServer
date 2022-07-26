#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>

#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../timer/lst_timer.h"
#include "../log/log.h"

class http_conn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0, //解析请求行
        CHECK_STATE_HEADER, //解析请求头
        CHECK_STATE_CONTENT //解析消息体，仅用于解析POST请求
    };
    enum HTTP_CODE
    {
        NO_REQUEST, //请求不完整，需要继续读取请求报文数据，跳转主线程继续监测读事件
        GET_REQUEST, //获得了完整的HTTP请求，调用do_request完成请求资源映射
        BAD_REQUEST, //HTTP请求报文有语法错误或请求资源为目录，跳转process_write完成响应报文
        NO_RESOURCE, //请求资源不存在，跳转process_write完成响应报文
        FORBIDDEN_REQUEST, //请求资源禁止访问，没有读取权限，跳转process_write完成响应报文
        FILE_REQUEST, //请求资源可以正常访问，跳转process_write完成响应报文
        INTERNAL_ERROR, //服务器内部错误，该结果在主状态机逻辑switch的default下，一般不会触发
        CLOSED_CONNECTION
    };
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    void init(int sockfd, const sockaddr_in &addr, char *, int, int, string user, string passwd, string sqlname);
    void close_conn(bool real_close = true);
    void process();
    bool read_once();
    bool write();
    sockaddr_in *get_address()
    {
        return &m_address;
    }
    void initmysql_result(connection_pool *connPool);
    int timer_flag;
    int improv; //Reactor模式中，将读写客户端数据交给线程池中的线程做，做完了将improv设置为1


private:
    void init();
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();

    //m_start_line是行在buffer中的起始位置，将该位置后面的数据赋给text
    //此时从状态机已提前将一行的末尾字符\r\n变为\0\0，所以text可以直接取出完整的行进行解析
    char *get_line() { return m_read_buf + m_start_line; };

    LINE_STATUS parse_line();
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content); //添加文本content
    bool add_status_line(int status, const char *title); //添加状态行：http/1.1 状态码 状态消息
    bool add_headers(int content_length); //添加消息报头，内部调用add_content_length和add_linger函数以及add_blank_line函数添加空行
    bool add_content_type(); //添加文本类型，这里是html
    bool add_content_length(int content_length); //记录响应报文长度，用于浏览器端判断服务器是否发送完数据
    bool add_linger(); //添加连接状态，通知浏览器端是保持连接还是关闭
    bool add_blank_line(); //添加空行

public:
    static int m_epollfd;
    static int m_user_count;
    MYSQL *mysql;
    int m_state;  //读为0, 写为1

private:
    int m_sockfd;
    sockaddr_in m_address;
    char m_read_buf[READ_BUFFER_SIZE];
    int m_read_idx; //指向缓冲区m_read_buf的数据末尾的下一个字节
    int m_checked_idx; //指向从状态机当前正在分析的字节
    int m_start_line;
    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx;
    CHECK_STATE m_check_state;
    METHOD m_method;
    char m_real_file[FILENAME_LEN];
    char *m_url;
    char *m_version;
    char *m_host;
    int m_content_length;
    bool m_linger;
    char *m_file_address;
    struct stat m_file_stat;
    struct iovec m_iv[2];
    int m_iv_count;
    int cgi;        //是否启用的POST
    char *m_string; //存储消息体数据
    int bytes_to_send;
    int bytes_have_send;
    char *doc_root;

    map<string, string> m_users;
    int m_TRIGMode;
    int m_close_log;

    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];
};

#endif
