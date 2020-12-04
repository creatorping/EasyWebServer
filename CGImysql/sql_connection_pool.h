#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include<stdio.h>
#include<list>
#include<mysql/mysql.h>
#include<error.h>
#include<string.h>
#include<iostream>
#include<string>
#include"../lock/locker.h"
#include"../log/log.h"

using namespace std;
class connection_pool
{
public:
  MYSQL *GetConnection();   //获取数据库连接
  bool ReleaseConnection(MYSQL *conn); //释放连接
  int GetFreeConn(); //获取连接
  void DestroyPool(); //销毁所有连接

  //单例模式
  static connection_pool *GetInstance();

  void init(string url,string user,string password,string databasename,int port,int maxconn,int close_log);

private:
  connection_pool();
  ~connection_pool();

  int m_maxconn; //最大连接数
  int m_curconn; //当前已使用的连接数
  int m_freeconn; //当前空闲连接数
  locker lock;
  list <MYSQL *> connlist; //连接池
  sem reserve;

public:
  string m_url; //主机地址
  string m_port; //数据库端口号
  string m_user;  //登录数据库使用的用户名
  string m_password; //登录数据库的密码
  string m_databasename; //使用数据库名
  int m_close_log; //日志开关
};

class connectionRAII
{
public:
  connectionRAII(MYSQL **con,connection_pool *connpool);
  ~connectionRAII();

private:
  MYSQL *conRAII;
  connection_pool *poolRAII;
};

#endif
