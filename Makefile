.PHONY: clean All

All:
	@echo "----------Building project:[ RigolCsvToWav - Release64 ]----------"
	@cd "RigolCsvToWav" && $(MAKE) -f  "RigolCsvToWav.mk"
clean:
	@echo "----------Cleaning project:[ RigolCsvToWav - Release64 ]----------"
	@cd "RigolCsvToWav" && $(MAKE) -f  "RigolCsvToWav.mk" clean
