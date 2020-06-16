codegen:
	@rm -rf tmp/
	@mkdir tmp/
	cellc-cs project/codegen.txt tmp/
	cp project/codegen.csproj tmp/
	dotnet build tmp/
	@ln -s tmp/bin/Debug/netcoreapp3.1/codegen .
	@rm -f generated.cpp

cellc-cs.cpp: codegen
	./codegen code.txt
	bin/apply-hacks < generated.cpp > cellc-cs.cpp

cellc-cs: cellc-cs.cpp
	g++ -ggdb -Isrc/runtime/ cellc-cs.cpp src/hacks.cpp src/runtime/*.cpp -o cellc-cs

tiny-test: codegen
	./codegen tiny-code.txt
	g++ -ggdb -Isrc/runtime/ generated.cpp src/runtime/*.cpp -o tiny-test

update-tiny-test:
	g++ -ggdb -Isrc/runtime/ generated.cpp src/runtime/*.cpp -o tiny-test

tests:
	@rm -rf tmp/
	@mkdir tmp/
	./cellc-cs project/tests.txt tmp/
	cp tmp/generated.cs ../work/dotnet-tests
	dotnet build ../work/dotnet-tests
	@echo
	../work/dotnet-tests/bin/Debug/netcoreapp3.1/dotnet-tests

clean:
	@rm -rf tmp/ codegen generated.cpp cellc-cs.cpp automata.cs automata.txt runtime.cs typedefs.cs dump-*.txt
