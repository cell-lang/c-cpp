cellc.net:
	@rm -rf tmp/cellc.net && mkdir -p tmp/cellc.net
	cellc-cs project/compiler-no-runtime.txt tmp/cellc.net/
	../csharp/bin/apply-hacks < tmp/cellc.net/generated.cs > tmp/cellc.net/cellc.cs
	@rm tmp/cellc.net/generated.cs
	@cp project/cellc.csproj tmp/cellc.net/
	dotnet build -c Release tmp/cellc.net/
	@ln -s tmp/cellc.net/bin/Release/netcoreapp3.1/cellc cellc.net

cellc: # cellc.net
	@rm -rf tmp/cellc/ && mkdir -p tmp/cellc/
	bin/cellc -t project/compiler-no-runtime.txt tmp/cellc/
	#misc/cellc -t project/compiler-no-runtime.txt tmp/cellc/
	# ./cellc.net -t project/compiler-no-runtime.txt tmp/cellc/
	bin/apply-hacks < tmp/cellc/generated.cpp > tmp/cellc/cellc.cpp
	g++ -ggdb -Isrc/runtime/ tmp/cellc/cellc.cpp src/hacks.cpp src/runtime/*.cpp -o cellc

cellc2:
	@rm -rf tmp/cellc/ && mkdir -p tmp/cellc/
	./cellc -t project/compiler-no-runtime.txt tmp/cellc/
	bin/apply-hacks < tmp/cellc/generated.cpp > tmp/cellc/cellc.cpp
	g++ -ggdb -Isrc/runtime/ tmp/cellc/cellc.cpp src/hacks.cpp src/runtime/*.cpp -o cellc2

update-cellc:
	@rm -f cellc
	g++ -ggdb -Isrc/runtime/ tmp/cellc/cellc.cpp src/hacks.cpp src/runtime/*.cpp -o cellc

cellcr:
	@rm -rf tmp/cellc/ && mkdir -p tmp/cellc/
	misc/cellc -t project/compiler-no-runtime.txt tmp/cellc/
	bin/apply-hacks < tmp/cellc/generated.cpp > tmp/cellc/cellc.cpp
	g++ -O3 -flto -DNDEBUG -Isrc/runtime/ tmp/cellc/cellc.cpp src/hacks.cpp src/runtime/*.cpp -o cellcr

update-cellcr:
	@rm -f cellcr
	g++ -O3 -flto -DNDEBUG -Isrc/runtime/ tmp/cellc/cellc.cpp src/hacks.cpp src/runtime/*.cpp -o cellcr

update-cellcp:
	@rm -f cellcp
	g++ -pg -O -flto -DNDEBUG -Isrc/runtime/ tmp/cellc/cellc.cpp src/hacks.cpp src/runtime/*.cpp -o cellcp

update-cellc-inline:
	@rm -f cellc
	g++ -ggdb -Isrc/runtime/ tmp/cellc/cellc.cpp src/hacks.cpp -o cellc

update-cellcr-inline:
	@rm -f cellcr
	g++ -O3 -DNDEBUG -Isrc/runtime/ tmp/cellc/cellc.cpp src/hacks.cpp -o cellcr

codegen.net:
	@rm -rf tmp/codegen.net/ && mkdir -p tmp/codegen.net/
	cellc-cs project/codegen.txt tmp/codegen.net/
	cp project/codegen.csproj tmp/codegen.net/
	dotnet build tmp/codegen.net/
	@ln -s tmp/codegen.net/bin/Debug/netcoreapp3.1/codegen codegen.net

codegen:
	@rm -rf codegen tmp/codegen/ && mkdir -p tmp/codegen/
	./cellc project/codegen.txt tmp/codegen/
	g++ -ggdb -Isrc/runtime/ tmp/codegen/generated.cpp src/runtime/*.cpp -o codegen

update-codegen:
	g++ -ggdb -Isrc/runtime/ tmp/codegen/generated.cpp src/runtime/*.cpp -o codegen

compiler-test-loop: cellc.net
	@rm -f generated.cpp cellc-1.cpp cellc-2.cpp cellc-3.cpp
	@rm -rf tmp/cellc && mkdir -p tmp/cellc
	./cellc.net -t project/compiler-no-runtime.txt tmp/cellc/
	bin/apply-hacks < tmp/cellc/generated.cpp > tmp/cellc/cellc-1.cpp
	g++ -ggdb -Isrc/runtime/ tmp/cellc/cellc-1.cpp src/runtime/*.cpp src/hacks.cpp -o cellc-1
	./cellc-1 project/compiler-no-runtime.txt tmp/cellc/
	bin/apply-hacks < tmp/cellc/generated.cpp > tmp/cellc/cellc-2.cpp
	g++ -ggdb -Isrc/runtime/ tmp/cellc/cellc-2.cpp src/runtime/*.cpp src/hacks.cpp -o cellc-2
	./cellc-2 project/compiler-no-runtime.txt tmp/cellc/
	bin/apply-hacks < tmp/cellc/generated.cpp > tmp/cellc/cellc-3.cpp
	cmp tmp/cellc/cellc-2.cpp tmp/cellc/cellc-3.cpp
	cd tmp/cellc/ && ln -s cellc-3.cpp cellc.cpp
	# g++ -ggdb -Isrc/runtime/ tmp/cellc/cellc-3.cpp src/runtime/*.cpp src/hacks.cpp -o cellc-3
	# ./cellc-3 project/compiler-no-runtime.txt tmp/cellc/
	# bin/apply-hacks < tmp/cellc/generated.cpp > tmp/cellc/cellc-4.cpp
	# cmp tmp/cellc/cellc-3.cpp tmp/cellc/cellc-4.cpp

codegen-test-loop: codegen
	./codegen misc/codegen-opt-code.txt
	mv generated.cpp codegen-1.cpp
	g++ -ggdb -Isrc/runtime/ codegen-1.cpp src/runtime/*.cpp -o codegen-1
	./codegen-1 misc/codegen-opt-code.txt
	mv generated.cpp codegen-2.cpp
	g++ -ggdb -Isrc/runtime/ codegen-2.cpp src/runtime/*.cpp -o codegen-2
	./codegen-2 misc/codegen-opt-code.txt
	mv generated.cpp codegen-3.cpp
	cmp codegen-2.cpp codegen-3.cpp

gen-html: cellc.net
	@rm -rf tmp/gen-html/ && mkdir -p tmp/gen-html/
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
	@rm -rf tmp/tests/ && mkdir tmp/tests/
	./cellc project/tests.txt tmp/tests/
	g++ -ggdb -Isrc/runtime/ tmp/tests/generated.cpp src/runtime/*.cpp -o tests

update-tests:
	g++ -ggdb -Isrc/runtime/ tmp/tests/generated.cpp src/runtime/*.cpp -o tests

runtime.h:
	rm -f src/runtime/runtime.h

	echo '#include "iolib.h"' >> src/runtime/runtime.h
	echo '#include "extern.h"' >> src/runtime/runtime.h
	echo '#include "os-interface.h"' >> src/runtime/runtime.h

	grep -v '#include' src/runtime/mem-utils.cpp         		>> src/runtime/runtime.h
	grep -v '#include' src/runtime/mem.cpp               		>> src/runtime/runtime.h
	grep -v '#include' src/runtime/mem-alloc.cpp         		>> src/runtime/runtime.h
	grep -v '#include' src/runtime/mem-copying.cpp       		>> src/runtime/runtime.h
	grep -v '#include' src/runtime/mem-core.cpp          		>> src/runtime/runtime.h
	grep -v '#include' src/runtime/new-code.cpp          		>> src/runtime/runtime.h
	grep -v '#include' src/runtime/algs.cpp              		>> src/runtime/runtime.h
	grep -v '#include' src/runtime/basic-ops.cpp         		>> src/runtime/runtime.h
	grep -v '#include' src/runtime/bin-rel-obj.cpp       		>> src/runtime/runtime.h
	grep -v '#include' src/runtime/instrs.cpp            		>> src/runtime/runtime.h
	grep -v '#include' src/runtime/inter-utils.cpp       		>> src/runtime/runtime.h
	grep -v '#include' src/runtime/os-interface-linux.cpp		>> src/runtime/runtime.h
	grep -v '#include' src/runtime/iolib.cpp             		>> src/runtime/runtime.h
	grep -v '#include' src/runtime/parsing.cpp           		>> src/runtime/runtime.h
	grep -v '#include' src/runtime/printing.cpp          		>> src/runtime/runtime.h
	grep -v '#include' src/runtime/sorting.cpp           		>> src/runtime/runtime.h
	grep -v '#include' src/runtime/tern-rel-obj.cpp      		>> src/runtime/runtime.h
	grep -v '#include' src/runtime/utils.cpp             		>> src/runtime/runtime.h
	grep -v '#include' src/runtime/debug.cpp             		>> src/runtime/runtime.h
# 	grep -v '#include' src/hacks.cpp                    		>> src/runtime/runtime.h

clean:
	@rm -rf tmp/ cellc.net cellc cellcr cellcp codegen codegen.net tests
	@rm -f cellc-[0-9] cellc-[0-9].cpp cellcr-*
	@rm -rf cellc-cs cellc.net generated.cpp cellc-cs.cpp
	@rm -rf automata.cs automata.txt runtime.cs typedefs.cs dump-*.txt *.o gmon.out
	@mkdir tmp/ tmp/null/
