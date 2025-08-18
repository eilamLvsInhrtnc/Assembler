ASM = util.c preAsm.c Assembler.c firstPass.c binRep.c secondPass.c

assembler: $(ASM)

	@echo "Compiling assembler..."
	gcc $(ASM) -o assembler

clean:

	rm -f assembler
