#
# Copyright (c) 2018-19, Ian Cahoon <icahoon@gmail.com>
# All rights reserved.
#

#___ builder _________________________________________________________________

FROM    icahoon/cbuild AS builder
LABEL   stage=intermediate
COPY    --chown=arcade:arcade . ${HOME}
RUN     cd src && make clean && make && cd - && \
        tar -czf tinymud.tar.gz tinymudctl bin/tinymud tinybase.db help.txt

#___ tinymud _________________________________________________________________

FROM       icahoon/base
LABEL      maintainer="Ian Cahoon <icahoon@gmail.com>"
COPY       --from=builder ${HOME}/tinymud.tar.gz ${HOME}/tinymud.tar.gz
RUN        tar -xzf tinymud.tar.gz && \
           rm -f tinymud.tar.gz
VOLUME     ["/db"]
EXPOSE     4201
ENTRYPOINT ["./tinymudctl", "start"]
