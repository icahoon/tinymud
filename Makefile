repo_url         := icahoon
name             := tinymud
image_name       := $(repo_url)/$(name)
semantic_version := $(shell git describe --match "v[0-9]*" --tags)
image_version    := $(semantic_version:v%=%)


.PHONY: $(name)
$(name): Dockerfile
	sudo docker build \
		--rm --force-rm \
		-t $(image_name):$(image_version) \
		-t $(image_name):latest \
		.

.PHONY: push
push:
	sudo docker login && \
	sudo docker push $(image_name)

.PHONY: clean
clean:
	-$(MAKE) -C src $@
	-sudo docker image rm -f $(image_name):$(image_version) 2>/dev/null
	-sudo docker image rm -f $(image_name):latest 2>/dev/null
