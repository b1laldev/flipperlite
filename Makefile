.PHONY: test fmt

test:
	$(MAKE) -C firmware/host test
	cd server && go test ./...

fmt:
	cd server && gofmt -w .
