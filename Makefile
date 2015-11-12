.PHONY: clean All

All:
	@echo "----------Building project:[ RigolCsvToWav - Debug ]----------"
	@cd "RigolCsvToWav" && "$(MAKE)" -f  "RigolCsvToWav.mk"
clean:
	@echo "----------Cleaning project:[ RigolCsvToWav - Debug ]----------"
	@cd "RigolCsvToWav" && "$(MAKE)" -f  "RigolCsvToWav.mk" clean
