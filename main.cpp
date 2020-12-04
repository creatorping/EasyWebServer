#include "config.h"

int main(int argc,char *argv[])
{
  //需要修改的数据库信息，登陆名，密码，库名
  string user = "root";
  string passwd = "284014";
  string databasename = "yourdb";

  //命令行解析
  Config config;
  config.parse_arg(argc,argv);

  webserver server;

  //初始化
  server.init(config.PORT, user, passwd, databasename, config.LOGWrite,
              config.OPT_LINGER, config.TRIGMode, config.sql_num, config.thread_num,
              config.close_log, config.actor_model);

  //启动日志系统
  server.log_write();

  //数据库连接池
  server.sql_pool();

  //开启线程池
  server.thread_pool();

  //设置触发模式
  server.trig_mode();

  //监听
  server.eventlisten();

  //运行
  server.eventloop();

  return 0;
}
