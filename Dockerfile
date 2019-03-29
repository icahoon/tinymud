#
# Copyright (c) 2018, Ian Cahoon <icahoon@gmail.com>
# All rights reserved.
#

#___ builder _________________________________________________________________

FROM    icahoon/cbuild AS builder
COPY    --chown=arcade:arcade . ${HOME}
RUN     cd tinymud && make

#___ tinymud _________________________________________________________________

FROM       icahoon/base
LABEL      maintainer="Ian Cahoon <icahoon@gmail.com>"

ENV        port=4201

COPY       --from=builder --chown=arcade:arcade ${HOME}/bin/tinymud ${HOME}/bin/tinymud
COPY       --from=builder --chown=arcade:arcade ${HOME}/tinybase.db ${HOME}/tinybase.db

ENTRYPOINT [ "tinymud", "tinybase.db", "tinybase.db.dump", "${port}" ]
EXPOSE     ${port}
