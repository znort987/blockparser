FROM ubuntu:14.04
MAINTAINER Roman Shtylman <shtylman@gmail.com>

RUN apt-get update
RUN apt-get -y upgrade
RUN apt-get install -y make git clang-3.4 libsparsehash-dev libssl-dev libboost-graph-dev cmake

RUN useradd -s /bin/bash -m -d /home/bitcoin bitcoin
RUN chown bitcoin:bitcoin -R /home/bitcoin
ENV HOME /home/bitcoin

WORKDIR /home/bitcoin

# comment this in if testing using local code versus cloning github
#ADD . /home/bitcoin/blockparser
#RUN chown -R bitcoin:bitcoin /home/bitcoin/blockparser

USER bitcoin

# comment this out if using ADD directly
RUN git clone https://github.com/defunctzombie/blockparser.git blockparser

# build
RUN cd blockparser && mkdir build && cd build && cmake .. && make

# blockparser needs to access the block .dat files directly
# mount the folder where `blocks` file lives to this path
VOLUME ["/home/bitcoin/.bitcoin"]

CMD ["help"]

# this will make the container act as the parser executable
# docker run <container name> [parser args]
ENTRYPOINT ["/home/bitcoin/blockparser/build/parser"]
