#
# Copyright (c) 2018-19, Ian Cahoon <icahoon@gmail.com>
# All rights reserved.
#

repo_url         := icahoon
name             := tinymud
image_name       := $(repo_url)/$(name)
semantic_version := $(shell git describe --match "v[0-9]*" --tags)
image_version    := $(semantic_version:v%=%)

.PHONY: $(name)
$(name): Dockerfile
	sudo docker build --rm --force-rm \
		-t $(image_name):$(image_version) \
		-t $(image_name):latest \
		. 
	@sudo docker image prune -f --filter label=stage=intermediate

.PHONY: push
push:
	sudo docker login && \
	sudo docker push $(image_name)

.PHONY: clean
clean:
	-$(MAKE) -C src $@
	-sudo docker image rm -f $(image_name):$(image_version) 2>/dev/null
	-sudo docker image rm -f $(image_name):latest 2>/dev/null
	-sudo docker image prune -f --filter label=stage=intermediate

.PHONY: run
run:
	sudo docker run --rm -d -p 4201:4201 -v arcade.volume:/db --name arcade $(image_name)

.PHONY: stop
stop:
	sudo docker stop arcade

.PHONY: volume
volume:
	sudo docker volume create arcade.volume

.PHONY: clean_volume
clean_volume:
	-sudo docker volume rm arcade.volume

.PHONY: nuke
nuke: clean clean_volume
