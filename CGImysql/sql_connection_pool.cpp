#include<mysql/mysql.h>
#include<stdio.h>
#include<string>
#include<string.h>
#include<stdlib.h>
#include<list>
#include<pthread.h>
#include<iostream>
#include"sql_connection_pool.h"

using namespace std;

connection_pool::connection_pool()
{
  m_curconn = 0;
  m_freeconn = 0;
}
connection_pool *connection_pool::GetInstance()
{
  static connection_pool connpool;
  return &connpool;
}

//构造初始化
void connection_pool::init(string url,string user,string password,string dbname,int port,int maxconn,int close_log)
{
  m_url = url;
  m_port = port;
  m_user = user;
  m_password = password;
  m_databasename = dbname;
  m_close_log = close_log;

  for(int i=0;i<maxconn;i++)
  {
    MYSQL *con=NULL;
    con=mysql_init(con);

    if(con == NULL)
    {
      LOG_ERROR("MYSQL Error");
      exit(1);
    }
    con = mysql_real_connect(con,url.c_str(),user.c_str(),password.c_str(),dbname.c_str(),port,NULL,0);

    if(con==NULL)
    {
        LOG_ERROR("MYSQL Error");
        exit(1);
    }
    connlist.push_back(con);
    ++m_freeconn;
  }

  reserve = sem(m_freeconn);
  m_maxconn = m_freeconn;
}

//当有请求时. 冲数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL *connection_pool::GetConnection()
{
  MYSQL *con = NULL;
  if(connlist.size() == 0)
    return NULL;
  reserve.wait();
  lock.lock();
  con = connlist.front();
  connlist.pop_front();

  --m_freeconn;
  ++m_curconn;

  lock.unlock();
  return con;
}

//释放当前使用的连接
bool connection_pool::ReleaseConnection(MYSQL *con)
{
  if(con==NULL)
    return false;
  lock.lock();
  connlist.push_back(con);
  ++m_freeconn;
  --m_curconn;

  lock.unlock();
  reserve.post();
  return true;
}

//销毁数据库连接池
void connection_pool::DestroyPool()
{
  lock.lock();
  if(connlist.size() > 0)
  {
    list<MYSQL *>::iterator it;
    for(it = connlist.begin();it!=connlist.end();it++)
    {
      MYSQL *con=*it;
      mysql_close(con);
    }
    m_curconn = 0;
    m_freeconn = 0;
    connlist.clear();
  }

  lock.unlock();
}

//当前空闲连接数
int connection_pool::GetFreeConn()
{
  return this->m_freeconn;
}
connection_pool::~connection_pool()
{
  DestroyPool();
}
connectionRAII::connectionRAII(MYSQL **SQL,connection_pool *connpool)
{
  *SQL = connpool ->GetConnection();
  conRAII = *SQL;
  poolRAII = connpool;
}

connectionRAII::~connectionRAII()
{
  poolRAII->ReleaseConnection(conRAII);
}
