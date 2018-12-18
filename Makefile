repo_url := icahoon

name           := tinymud
version        := 1.0

alpine_version := 3.8
arch           := amd64

image_name    := $(repo_url)/$(name)
image_version := $(version)


.PHONY: $(name)
$(name): Dockerfile
	docker build \
		--rm --force-rm \
		--build-arg alpine_version=$(alpine_version) \
		--build-arg arch=$(arch) \
		-t $(image_name):$(image_version) \
		-t $(image_name):latest \
		.

.PHONY: push
push:
	docker login && \
	docker push $(repo_url)/$(name)

.PHONY: clean
clean:
	$(MAKE) -C tinymud $@
	docker image rm -f $(image_name):$(image_version) 2>/dev/null
	docker image rm -f $(image_name):latest 2>/dev/null
