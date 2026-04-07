FROM gcc:latest

RUN apt-get update && apt-get install -y \
    cmake \
    python3 \
    dos2unix \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/citadel

COPY . .

RUN mkdir build && cd build && \
    cmake .. && \
    cmake --build .

RUN dos2unix entrypoint.sh && chmod +x entrypoint.sh

ENTRYPOINT ["./entrypoint.sh"]