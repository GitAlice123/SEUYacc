# 设置基础镜像
FROM ubuntu:latest

# 安装必要的软件包
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    graphviz

# 设置工作目录
WORKDIR /app

# 复制项目文件到工作目录
COPY . .

# 编译C++项目
RUN make

# 运行你的应用程序（这里假设可执行文件名为 "app"）
CMD ["./app"]
