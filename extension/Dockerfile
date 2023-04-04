FROM postgres:15
LABEL authors="mgioioso"

RUN apt-get update &&  apt-get install cmake llvm-11 llvm-11-dev postgresql-server-dev-15 libpq-dev -y

COPY .  /borealisx2
WORKDIR /borealisx2

RUN make install