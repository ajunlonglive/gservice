# 配置文件详解
**配置文件名称必须为conf.json,并且和可执行文件文件在同一目录.**<br />
配置文件格式如下:

```json
{
  "name": "gsoap server",
  "port": 7654,
  "pool": 48,
  "queue_max": 1024,
  "send_timeout": 3,
  "recv_timeout": 6,
  "double_format": "%.6lf",
  "redirect_url": "/login.html",
  "database_max_use_count": 256,
  "databases": [
    {
      "server": "192.168.1.252",
      "port": 5432,
      "dbname": "postgres",
      "user": "postgres",
      "password": "/0YVARorrHWADtCV0/PClQ==",
      "client_encoding": "UTF8",
      "connect_timeout": 10,
      "statement_timeout": 10,
      "max_connection": 48,
      "weight": 1,
      "iswriter": true
    }  
  ]
}
```
## 根节点
- name:gsaop服务程序名称.<br />
- port:gsaop服务侦听的端口号.范围1-65535.<br />
- pool:gsaop处理请求的线程数量,范围1-512.值应为CPU内核数的2,4,6,8倍,建议使用CPU内核数*4,.注意这个值和数据库最大连接数无关.<br />
- queue_max:socket请求队列大小,范围为1024-8192
- send_timeout:发送数据的超时时间,范围为-999999-120,正值时单位为秒,负值是单位为微秒.
- recv_timeout:接收数据的超时时间,范围为-999999-120,正值时单位为秒,负值是单位为微秒.
- double_format:指明输出浮点数时小数的精度,不一定有效,取决于小数的值.
- redirect_url:重新向时的url
- database_max_use_count:PostgreSQL每个连接使用的最大次数,范围为0-32767,0表示不重连.达到这个值时将关闭PostgreSQL连接并重新打开.目的是为防止一些质量不高的插件使用pmalloc分配内存后未手动释放,这部份内存将一直占用PostgreSQL的共享内存,只有关闭连接后这部份内存才会释放.

## databases节点
databases节点是数组,至少需要包含一台可读写的PostgreSQL.同时可配置多台只读主机.使用时由函数 get_next_dbserver(arg, false)第二个参数指明是读取还是只读.同时还需要配合事务等级共同使用.<br />
- server:PostgreSQL服务器主机名或IP.<br />
- port:PostgreSQL服务器端口号.<br />
- dbname:PostgreSQL服务器数据库名称.<br />
- user:PostgreSQL服务器用户名称.<br />
- password:PostgreSQL服务器用户密码.密码采用AES 128位加密,可使用工具aescrypt计算密码值.<br />
- client_encoding:PostgreSQL客户端使用的字符编码类型.<br />
- connect_timeout:PostgreSQL服务器连接超时时间,单位为秒,范围为1-180.<br />
- statement_timeout:PostgreSQL服务器执行命令允许的最大时间,单位为秒,范围为1-180.如果执行命令的时间超过这个值,将会取消命令执行并发送异常信息.<br />
- max_connection:PostgreSQL连接数.范围1-1024.值应为PostgreSQL服务器CPU内核数的2,4,6,8倍,建议使用CPU内核数*2,.注意这个值和gsaop处理请求的线程数量无关.<br />
- weight:当前PostgreSQL服务器使用频率系数,只有在多台只读PostgreSQL服务器时才会生效,默认设置为1.例如需要某台PostgreSQL服务器具有较高的使用频率时,可将其它只读PostgreSQL服务器设置为1,具有较高的使用频率PostgreSQL服务器设置为2或更高的值,当调用get_next_dbserver(arg, false)获取连接时,连接到具有较高的使用频率PostgreSQL服务器将是其它PostgreSQL服务器的2倍或更高.
- iswriter:指定当前PostgreSQL服务器是否可读写,true为可读写,false为只读.<br />
## 示例
### 只有一台PostgreSQL服务器
```json
{
  "name": "gsoap server",
  "port": 7654,
  "pool": 48,
  "send_timeout": 3,
  "recv_timeout": 6,
  "double_format": "%.6lf",
  "database_max_use_count": 256,
  "databases": [
    {
      "server": "192.168.1.1",
      "port": 5432,
      "dbname": "postgres",
      "user": "postgres",
      "password": "/0YVARorrHWADtCV0/PClQ==",
      "client_encoding": "UTF8",
      "connect_timeout": 10,
      "statement_timeout": 10,
      "max_connection": 48,
      "weight": 1,
      "iswriter": true
    }
  ]
}
```
### 两台PostgreSQL服务器,192.168.1.1可读写,192.168.1.2只读
```json
{
  "name": "gsoap server",
  "port": 7654,
  "pool": 48,
  "send_timeout": 3,
  "recv_timeout": 6,
  "double_format": "%.6lf",
  "database_max_use_count": 256,
  "databases": [
    {
      "server": "192.168.1.1",
      "port": 5432,
      "dbname": "postgres",
      "user": "postgres",
      "password": "/0YVARorrHWADtCV0/PClQ==",
      "client_encoding": "UTF8",
      "connect_timeout": 10,
      "statement_timeout": 10,
      "max_connection": 48,
      "weight": 1,
      "iswriter": true
    },
    {
      "server": "192.168.1.2",
      "port": 5432,
      "dbname": "postgres",
      "user": "postgres",
      "password": "/0YVARorrHWADtCV0/PClQ==",
      "client_encoding": "UTF8",
      "connect_timeout": 10,
      "statement_timeout": 10,
      "max_connection": 48,
      "weight": 1,
      "iswriter": false
    }
  ]
}
```
### 多台PostgreSQL服务器,192.168.1.1可读写,其它只读.其中192.168.1.3的使用频率将是192.168.1.2的4倍
```json
{
  "name": "gsoap server",
  "port": 7654,
  "pool": 48,
  "send_timeout": 3,
  "recv_timeout": 6,
  "double_format": "%.6lf",
  "database_max_use_count": 256,
  "databases": [
    {
      "server": "192.168.1.1",
      "port": 5432,
      "dbname": "postgres",
      "user": "postgres",
      "password": "/0YVARorrHWADtCV0/PClQ==",
      "client_encoding": "UTF8",
      "connect_timeout": 10,
      "statement_timeout": 10,
      "max_connection": 48,
      "weight": 1,
      "iswriter": true
    },
    {
      "server": "192.168.1.2",
      "port": 5432,
      "dbname": "postgres",
      "user": "postgres",
      "password": "/0YVARorrHWADtCV0/PClQ==",
      "client_encoding": "UTF8",
      "connect_timeout": 10,
      "statement_timeout": 10,
      "max_connection": 48,
      "weight": 1,
      "iswriter": false
    },
    {
      "server": "192.168.1.3",
      "port": 5432,
      "dbname": "postgres",
      "user": "postgres",
      "password": "/0YVARorrHWADtCV0/PClQ==",
      "client_encoding": "UTF8",
      "connect_timeout": 10,
      "statement_timeout": 10,
      "max_connection": 48,
      "weight": 4,
      "iswriter": false
    }
  ]
}
```
