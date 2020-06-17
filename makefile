cellc.net:
	@rm -rf tmp/cellc.net
	@mkdir -p tmp/cellc.net
	cellc-cs project/compiler-no-runtime.txt tmp/cellc.net/
	mv tmp/cellc.net/generated.cs tmp/
	../csharp/bin/apply-hacks < tmp/generated.cs > tmp/cellc.net/generated.cs
	cp project/cellc.csproj tmp/cellc.net
	dotnet build tmp/cellc.net
	@ln -s tmp/cellc.net/bin/Debug/netcoreapp3.1/cellc cellc.net

cellc: cellc.net
	@rm -rf tmp/cellc
	@mkdir -p tmp/cellc
	./cellc.net project/compiler-no-runtime.txt tmp/cellc/
	bin/apply-hacks < tmp/cellc/generated.cpp > tmp/cellc/cellc.cpp
	g++ -ggdb -Isrc/runtime/ tmp/cellc/cellc.cpp src/hacks.cpp src/runtime/*.cpp -o cellc

update-cellc:
	g++ -ggdb -Isrc/runtime/ tmp/cellc/cellc.cpp src/hacks.cpp src/runtime/*.cpp -o cellc

codegen.net:
	@rm -rf tmp/codegen.net/
	@mkdir -p tmp/codegen.net/
	cellc-cs project/codegen.txt tmp/codegen.net/
	cp project/codegen.csproj tmp/codegen.net/
	dotnet build tmp/codegen.net/
	@ln -s tmp/codegen.net/bin/Debug/netcoreapp3.1/codegen codegen.net

codegen:
	@rm -rf codegen tmp/codegen/
	@mkdir -p tmp/codegen/
	./cellc.net project/codegen.txt tmp/codegen/
	g++ -ggdb -Isrc/runtime/ tmp/codegen/generated.cpp src/runtime/*.cpp -o codegen

update-codegen:
	g++ -ggdb -Isrc/runtime/ tmp/codegen/generated.cpp src/runtime/*.cpp -o codegen

codegen-test-loop: codegen
	./codegen codegen-opt-code.txt
	mv generated.cpp codegen-1.cpp
	g++ -ggdb -Isrc/runtime/ codegen-1.cpp src/runtime/*.cpp -o codegen-1
	./codegen-1 codegen-opt-code.txt
	mv generated.cpp codegen-2.cpp
	g++ -ggdb -Isrc/runtime/ codegen-2.cpp src/runtime/*.cpp -o codegen-2
	./codegen-2 codegen-opt-code.txt
	mv generated.cpp codegen-3.cpp
	cmp codegen-2.cpp codegen-3.cpp

gen-html: cellc.net
	@rm -rf tmp/gen-html/
	@mkdir -p tmp/gen-html/
	./cellc.net project/gen-html.txt tmp/gen-html/
	g++ -ggdb -Isrc/runtime/ tmp/gen-html/generated.cpp src/runtime/*.cpp -o gen-html

update-gen-html:
	g++ -ggdb -Isrc/runtime/ tmp/gen-html/generated.cpp src/runtime/*.cpp -o gen-html

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
	@rm -rf automata.cs automata.txt runtime.cs typedefs.cs dump-*.txt *.o
	@mkdir tmp/
