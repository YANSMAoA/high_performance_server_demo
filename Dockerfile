# 使用特定版本的Ubuntu镜像
FROM ubuntu:latest

# 替换为清华大学的 Ubuntu 镜像源
RUN sed -i 's/http:\/\/ports.ubuntu.com/http:\/\/mirrors.tuna.tsinghua.edu.cn/g' /etc/apt/sources.list

# 更新软件包并安装所需的库
RUN apt-get update 
RUN apt-get update && apt-get install -y build-essential
RUN apt-get install -y python3 python3-pip
RUN apt-get install -y libsqlite3-dev nginx apache2-utils curl && \
rm -rf /var/lib/apt/lists/*
# 配置 Nginx (假设您已经创建了 nginx.conf 并放在与 Dockerfile 相同的目录下)

# 复制代码到容器中
COPY . /usr/src/myapp

# 设置工作目录
WORKDIR /usr/src/myapp

EXPOSE 80 8080 8081 8082
