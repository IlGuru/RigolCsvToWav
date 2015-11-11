.PHONY: clean All

All:
	@echo "----------Building project:[ RigolCsvToWav - Release ]----------"
	@cd "RigolCsvToWav" && $(MAKE) -f  "RigolCsvToWav.mk"
clean:
	@echo "----------Cleaning project:[ RigolCsvToWav - Release ]----------"
	@cd "RigolCsvToWav" && $(MAKE) -f  "RigolCsvToWav.mk" clean
