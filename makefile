codegen:
	@rm -rf tmp/
	@mkdir tmp/
	cellc-cs project/codegen.txt tmp/
	cp project/codegen.csproj tmp/
	dotnet build tmp/
	ln -s tmp/bin/Debug/netcoreapp3.1/codegen .

clean:
	@rm -rf tmp/ codegen