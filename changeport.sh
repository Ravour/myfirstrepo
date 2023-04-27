#!/bin/bash

caddy_config_path=/etc/caddy/Caddyfile

port=$1

if [ -z "$port" ]; then
    echo "请输入新的端口。"
    exit 1
fi

# 查找老端口的值
old_port=$(cat $caddy_config_path | grep -o "com:.* {")
old_port=${old_port/com:/}
old_port=${old_port/ {/}

echo "老端口为：$old_port"

if [[ "$port" = "auto" ]]; then
    port=$((old_port + 1))
fi

echo "开始设置新的端口：$port"

# 在mac上应该为sed -i '' "s/com:.*/com:$port {/g" $caddy_config_path，
# 中间那个参数是在修改时添加备份文件的文件名。 这里设为空字符串
if sed -i "s/com:.*/com:$port {/g" $caddy_config_path; then
    service caddy restart
    v2ray restart
    echo "修改端口为${port}完成"
else 
    echo "修改端口失败！"
    exit 1
fi