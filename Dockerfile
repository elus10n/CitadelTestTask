FROM gcc:latest

RUN apt-get update && apt-get install -y cmake

WORKDIR /usr/src/citadel

COPY . .

RUN mkdir build && cd build && \
    cmake .. && \
    cmake --build .

CMD cd build && ./unit_tests && ./main_app && cat example/report.txt