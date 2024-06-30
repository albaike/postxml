FROM albaike/pstk:latest

RUN apt update && \
    apt install -y git dpkg-dev build-essential debhelper

USER postgres
WORKDIR /postxml
RUN git config --global --add safe.directory /tmp/postxml
COPY . /tmp/postxml
RUN cd /tmp/postxml && git archive --format=tar HEAD | tar -x -C /postxml
USER root
RUN sh -c "apt install -y $(dpkg-checkbuilddeps 2>&1 | sed 's/.*Unmet build dependencies: //')"
USER postgres
RUN ./build_debian.sh

USER root
RUN rm -rf /tmp/postxml
RUN apt install -y ./postgresql-16-postxml_1.0-1_amd64.deb