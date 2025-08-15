
ASM = util.c preAsm.c Assembler.c firstPass.c binRep.c secondPass.c
ASMTEST = util.c preAsm.c firstPass.c main.c binRep.c secindPass.c
assembler: $(ASM)

	@echo "Compiling assembler..."
	gcc $(ASM) -o assembler


test: $(ASMTEST)

	gcc $(ASMTEST) -o test


clean:

	rm -f assembler test
