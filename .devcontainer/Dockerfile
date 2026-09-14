FROM ubuntu:22.04

RUN yes | unminimize

RUN apt update && apt install -y \
    build-essential \
    curl \
    file \
    gdb \
    git \
    man \
    manpages-posix-dev \
    psmisc \
    python3 \
    python3-dev \
    python3-pip \
    python3-setuptools \
    valgrind \
    wget \
    xxd \
    zip

# Install Python packages
RUN pip install grpcio grpcio-tools msgpack requests

# Set container timezone and locale
RUN apt update && DEBIAN_FRONTEND=noninteractive apt install -y \
    locales \
    tzdata
ARG TZ=America/Chicago
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
RUN echo "en_US.UTF-8 UTF-8" > /etc/locale.gen && locale-gen

# Ubuntu 22.04 default clang-format package is too old
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -
RUN echo "deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-19 main" >> \
    /etc/apt/sources.list
RUN echo "deb-src http://apt.llvm.org/jammy/ llvm-toolchain-jammy-19 main" >> \
    /etc/apt/sources.list
RUN apt update && apt install -y clang-format-19
RUN ln -s /usr/bin/clang-format-19 /usr/bin/clang-format

# Install testius and socrates utilities
RUN wget -O /usr/local/bin/testius https://csci4061-fa26.s3.us-east-1.amazonaws.com/testius
RUN chmod 755 /usr/local/bin/testius
RUN wget -O /usr/local/bin/socrates https://csci4061-fa26.s3.us-east-1.amazonaws.com/socrates
RUN chmod 755 /usr/local/bin/socrates
