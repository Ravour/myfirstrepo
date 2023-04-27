#!/bin/bash

# 样例
# 1. 将v2ray端口改为443：./v2ray_change_port.sh 443
# 2. 将端口改为在原有端口上加1：./v2ray_change_port.sh auto

port=$1

if [ -z "$port" ]; then
    echo "请输入新的端口。 输入auto则为在原有端口上自动加1"
    exit 1
fi

# 复制修改端口脚本到云服务器
sshpass -p "password" scp -P 27538 ./change_port.sh root@ip:/root

# 登录云服务器后进入/root目录
# 设置脚本权限
# 运行脚本修改v2ray运行端口
sshpass -p "password" ssh root@ip -p 27538 "cd /root; chmod 755 change_port.sh; ./change_port.sh $port"


if [[ "$?" = "0" ]]; then
	if [[ "$port" = "auto" ]]; then
		caddy_config_path=./Caddyfile.txt
		# 获取修改后的配置文件
		sshpass -p "password" scp -P 27538 root@ip:/etc/caddy/Caddyfile $caddy_config_path
		# 查找端口的值
		cur_port=$(cat $caddy_config_path | grep -o "com:.* {")
		cur_port=${cur_port/com:/}
		cur_port=${cur_port/ {/}
		# 测试端口是否ok
		nc -vc ip $cur_port
	else
		# 测试端口是否ok
		nc -vc ip $port
	fi
	
	exit 0
else
	echo "修改端口失败"
	exit 1
fi