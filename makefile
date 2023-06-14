cellcd:
	@rm -rf tmp/cellc/ && mkdir -p tmp/cellc/
	bin/cellc -d -nrt -t project/compiler-no-runtime.txt tmp/cellc/
	bin/apply-hacks < tmp/cellc/generated.cpp > tmp/cellc/cellc.cpp
	g++ -ggdb -DNEDEBUG -Isrc/runtime/ tmp/cellc/cellc.cpp src/hacks.cpp objs/dbg/*.o -o cellc

cellc:
	@rm -rf tmp/cellc/ && mkdir -p tmp/cellc/
	bin/cellc -t project/compiler.txt tmp/cellc/
	bin/apply-hacks < tmp/cellc/generated.cpp > tmp/cellc/cellc.cpp
	grep -v include src/hacks.cpp >> tmp/cellc/cellc.cpp
	g++ -ggdb -DNEDEBUG tmp/cellc/cellc.cpp -o cellc

cellcr:
	@rm -rf tmp/cellc/ && mkdir -p tmp/cellc/
	bin/cellc -t project/compiler.txt tmp/cellc/
	bin/apply-hacks < tmp/cellc/generated.cpp > tmp/cellc/cellc.cpp
	grep -v include src/hacks.cpp >> tmp/cellc/cellc.cpp
	g++ -O3 -flto -DNDEBUG tmp/cellc/cellc.cpp -o cellcr

update-cellcd:
	@rm -f cellc
	g++ -ggdb -DNEDEBUG -Isrc/runtime/ tmp/cellc/cellc.cpp src/hacks.cpp objs/dbg/*.o -o cellc

update-cellcr:
	@rm -f cellcr
	g++ -O3 -flto -DNDEBUG tmp/cellc/cellc.cpp -o cellcr

compiler-test-loop:
	@rm -rf tmp/cellc && mkdir -p tmp/cellc
	bin/cellc -t project/compiler.txt tmp/cellc/
	bin/apply-hacks < tmp/cellc/generated.cpp > tmp/cellc/cellc-1.cpp
	grep -v include src/hacks.cpp >> tmp/cellc/cellc-1.cpp
	# g++ -ggdb -Isrc/runtime/ tmp/cellc/cellc-1.cpp src/runtime/*.cpp src/hacks.cpp -o cellc-1
	g++ -ggdb tmp/cellc/cellc-1.cpp -o cellc-1
	./cellc-1 project/compiler.txt tmp/cellc/
	bin/apply-hacks < tmp/cellc/generated.cpp > tmp/cellc/cellc-2.cpp
	grep -v include src/hacks.cpp >> tmp/cellc/cellc-2.cpp
	g++ -ggdb tmp/cellc/cellc-2.cpp -o cellc-2
	./cellc-2 project/compiler.txt tmp/cellc/
	bin/apply-hacks < tmp/cellc/generated.cpp > tmp/cellc/cellc-3.cpp
	grep -v include src/hacks.cpp >> tmp/cellc/cellc-3.cpp
	# cmp tmp/cellc/cellc-2.cpp tmp/cellc/cellc-3.cpp
	g++ -ggdb tmp/cellc/cellc-3.cpp -o cellc-3
	./cellc-3 project/compiler.txt tmp/cellc/
	bin/apply-hacks < tmp/cellc/generated.cpp > tmp/cellc/cellc-4.cpp
	grep -v include src/hacks.cpp >> tmp/cellc/cellc-4.cpp
	cd tmp/cellc/ && ln -s cellc-4.cpp cellc.cpp
	# cmp tmp/cellc/cellc-3.cpp tmp/cellc/cellc-4.cpp
	diff tmp/cellc/cellc-3.cpp tmp/cellc/cellc-4.cpp

# compiler-test-loop: cellc.net
# 	@rm -f generated.cpp cellc-1.cpp cellc-2.cpp cellc-3.cpp
# 	@rm -rf tmp/cellc && mkdir -p tmp/cellc
# 	./cellc.net -t project/compiler-no-runtime.txt tmp/cellc/
# 	bin/apply-hacks < tmp/cellc/generated.cpp > tmp/cellc/cellc-1.cpp
# 	g++ -ggdb -Isrc/runtime/ tmp/cellc/cellc-1.cpp src/runtime/*.cpp src/hacks.cpp -o cellc-1
# 	./cellc-1 project/compiler-no-runtime.txt tmp/cellc/
# 	bin/apply-hacks < tmp/cellc/generated.cpp > tmp/cellc/cellc-2.cpp
# 	g++ -ggdb -Isrc/runtime/ tmp/cellc/cellc-2.cpp src/runtime/*.cpp src/hacks.cpp -o cellc-2
# 	./cellc-2 project/compiler-no-runtime.txt tmp/cellc/
# 	bin/apply-hacks < tmp/cellc/generated.cpp > tmp/cellc/cellc-3.cpp
# 	# cmp tmp/cellc/cellc-2.cpp tmp/cellc/cellc-3.cpp
# 	cd tmp/cellc/ && ln -s cellc-3.cpp cellc.cpp
# 	g++ -ggdb -Isrc/runtime/ tmp/cellc/cellc-3.cpp src/runtime/*.cpp src/hacks.cpp -o cellc-3
# 	./cellc-3 project/compiler-no-runtime.txt tmp/cellc/
# 	bin/apply-hacks < tmp/cellc/generated.cpp > tmp/cellc/cellc-4.cpp
# 	cmp tmp/cellc/cellc-3.cpp tmp/cellc/cellc-4.cpp

runtime-sources:
	@rm -f misc/runtime-sources.cell
	bin/build-runtime-src-file.py src/runtime/ misc/runtime-sources.cell

clean:
	@rm -rf tmp/ cellc.net cellc cellcr cellcp codegen codegen.net tests
	@rm -f cellc-[0-9] cellc-[0-9].cpp cellcr-*
	@rm -rf cellc-cs cellc.net generated.cpp cellc-cs.cpp
	@rm -rf automata.cs automata.txt runtime.cs typedefs.cs dump-*.txt *.o gmon.out
	@mkdir tmp/ tmp/null/

%: src/runtime/%.cpp
	rm -f objs/dbg/$@.o
	g++ -c -ggdb -DNEDEBUG -I src/runtime/ $< -o objs/dbg/$@.o
