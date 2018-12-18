#
# Copyright (c) 2018, Ian Cahoon <icahoon@gmail.com>
# All rights reserved.
#
#  Run: docker run --volume $PWD:/workspace example
#

ARG     arch=amd64
ARG     alpine_version=3.8

#___ builder _________________________________________________________________

FROM    icahoon/cbuild AS builder
WORKDIR /tinymud
COPY    . /tinymud
RUN     make -C tinymud

#___ tinymud _________________________________________________________________

FROM       icahoon/base:${alpine_version}-${arch}
LABEL      maintainer="Ian Cahoon <icahoon@gmail.com>"

ENV        port 4201

USER       root
RUN        apk add --no-cache bash && \
           mkdir /tinymud && \
           chown arcade:arcade /tinymud

USER       arcade
WORKDIR    /tinymud
COPY       --from=builder --chown=arcade:arcade /tinymud/bin/tinymud /tinymud/tinybase.db /tinymud/
EXPOSE     ${port}/tcp

ENTRYPOINT [ "./tinymud", "tinybase.db", "tinybase.db.dump", "${port}" ]
