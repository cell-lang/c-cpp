cellc.net:
	@rm -rf tmp/cellc.net
	@mkdir -p tmp/cellc.net
	cellc-cs project/compiler-no-runtime.txt tmp/cellc.net
	mv tmp/cellc.net/generated.cs tmp/
	../csharp/bin/apply-hacks < tmp/generated.cs > tmp/cellc.net/generated.cs
	cp project/cellc.csproj tmp/cellc.net
	dotnet build tmp/cellc.net
	@ln -s tmp/cellc.net/bin/Debug/netcoreapp3.1/cellc cellc.net

codegen.net:
	@rm -rf tmp/
	@mkdir tmp/
	cellc-cs project/codegen.txt tmp/
	cp project/codegen.csproj tmp/
	dotnet build tmp/
	@ln -s tmp/bin/Debug/netcoreapp3.1/codegen codegen.net
	@rm -f generated.cpp

codegen:
	@rm -rf codegen tmp/codegen/
	@mkdir -p tmp/codegen/
	./cellc.net project/codegen.txt tmp/codegen/
	g++ -ggdb -Isrc/runtime/ tmp/codegen/generated.cpp src/runtime/*.cpp -o codegen

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
	@rm -rf tmp/ cellc.net codegen codegen.net
	@rm -rf cellc-cs cellc.net generated.cpp cellc-cs.cpp
	@rm -rf automata.cs automata.txt runtime.cs typedefs.cs dump-*.txt
	@mkdir tmp/
