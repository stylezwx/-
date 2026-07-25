# DHT11 串口网页 + MySQL 入库

这个页面基于浏览器 `Web Serial API`，用于直接连接 STM32 的串口并显示输出内容。

现在目录里还带了一个本地 `Node.js` 服务，负责两件事：

1. 提供 `http://localhost:8080/` 网页
2. 接收网页上传的解析结果，并写入 MySQL

浏览器不会直接连接 MySQL。真正入库的是本地服务。

## 使用方法

1. 在 PowerShell 里进入本目录：

```powershell
cd .\serial-web
```

2. 启动本地服务：

```powershell
.\start-serial-web.ps1
```

3. 用 Chrome 或 Edge 打开：

```text
http://localhost:8080/
```

4. 在网页的 `MySQL 配置` 区填写：
   - 主机：例如 `127.0.0.1`
   - 端口：例如 `3306`
   - 用户名：例如 `root`
   - 密码：你的 MySQL 密码
   - 数据库名：例如 `dht11_demo`
   - 数据表名：例如 `telemetry_log`

5. 点击 `保存配置`，再点 `初始化数据表`

6. 点击 `授权并连接串口`，选择 STM32 对应的 `COM` 口，波特率填 `115200`

7. 如果串口输出是下面这种格式，页面会实时显示并自动写库：

```text
dht_1: temp=27 | dht_2: temp=26 | countdown=59s | alarm=35
```

## 默认建表结构

点击 `初始化数据表` 后，本地服务会自动创建：

```sql
CREATE TABLE telemetry_log (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    temp1 DECIMAL(6,2) NOT NULL,
    temp2 DECIMAL(6,2) NOT NULL,
    countdown_seconds INT NOT NULL,
    alarm_temp DECIMAL(6,2) NOT NULL,
    raw_line VARCHAR(255) NOT NULL,
    recorded_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);
```

## 可用命令

- `dog_small`
- `dog_medium`
- `dog_large`
- `cat_small`
- `cat_medium`
- `cat_large`
- `key_up`
- `key_down`
- `temp_up`
- `temp_down`

兼容说明：
- `dog` 等同于 `dog_medium`
- `cat` 等同于 `cat_medium`

其中四个按键等效命令含义如下：

- `key_up`: 倒计时 `+60s`
- `key_down`: 倒计时 `-60s`
- `temp_up`: 报警温度 `+1`
- `temp_down`: 报警温度 `-1`

## 注意

- `Web Serial` 只能在安全上下文里使用，所以必须通过 `http://localhost` 或 `https` 打开，不能直接双击 `index.html`
- 浏览器和 XCOM、串口助手、Keil 串口监视器等软件不能同时占用同一个串口
- 固件串口参数固定为 `115200 / 8-N-1 / 无流控`，对应 `USART1`
- 本机需要能执行 `node` 和 `mysql` 命令
- MySQL 密码只保存在服务进程内存中，不会写入 `mysql.config.json`，服务重启后需重新输入。
- 也可以在启动服务前设置 `SERIAL_WEB_MYSQL_PASSWORD` 环境变量提供密码。
