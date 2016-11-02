################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
buttons.obj: ../buttons.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/Program Files (x86)/Texas Instruments/ccsv6/tools/compiler/arm_15.12.3.LTS/bin/armcl" -mv7M3 --code_state=16 -me --include_path="C:/Program Files (x86)/Texas Instruments/ccsv6/tools/compiler/arm_15.12.3.LTS/include" --include_path="C:/Program Files (x86)/Texas Instruments/StellarisWare" --include_path="C:/Program Files (x86)/Texas Instruments/StellarisWare/boards/ek-lm3s8962" -g --gcc --define="ccs" --define=PART_LM3S8962 --diag_wrap=off --diag_warning=225 --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="buttons.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

frame_graphics.obj: ../frame_graphics.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/Program Files (x86)/Texas Instruments/ccsv6/tools/compiler/arm_15.12.3.LTS/bin/armcl" -mv7M3 --code_state=16 -me --include_path="C:/Program Files (x86)/Texas Instruments/ccsv6/tools/compiler/arm_15.12.3.LTS/include" --include_path="C:/Program Files (x86)/Texas Instruments/StellarisWare" --include_path="C:/Program Files (x86)/Texas Instruments/StellarisWare/boards/ek-lm3s8962" -g --gcc --define="ccs" --define=PART_LM3S8962 --diag_wrap=off --diag_warning=225 --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="frame_graphics.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

lm3s8962_startup_ccs.obj: ../lm3s8962_startup_ccs.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/Program Files (x86)/Texas Instruments/ccsv6/tools/compiler/arm_15.12.3.LTS/bin/armcl" -mv7M3 --code_state=16 -me --include_path="C:/Program Files (x86)/Texas Instruments/ccsv6/tools/compiler/arm_15.12.3.LTS/include" --include_path="C:/Program Files (x86)/Texas Instruments/StellarisWare" --include_path="C:/Program Files (x86)/Texas Instruments/StellarisWare/boards/ek-lm3s8962" -g --gcc --define="ccs" --define=PART_LM3S8962 --diag_wrap=off --diag_warning=225 --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="lm3s8962_startup_ccs.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

main.obj: ../main.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/Program Files (x86)/Texas Instruments/ccsv6/tools/compiler/arm_15.12.3.LTS/bin/armcl" -mv7M3 --code_state=16 -me --include_path="C:/Program Files (x86)/Texas Instruments/ccsv6/tools/compiler/arm_15.12.3.LTS/include" --include_path="C:/Program Files (x86)/Texas Instruments/StellarisWare" --include_path="C:/Program Files (x86)/Texas Instruments/StellarisWare/boards/ek-lm3s8962" -g --gcc --define="ccs" --define=PART_LM3S8962 --diag_wrap=off --diag_warning=225 --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="main.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

rit128x96x4.obj: C:/Program\ Files\ (x86)/Texas\ Instruments/StellarisWare/boards/ek-lm3s8962/drivers/rit128x96x4.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/Program Files (x86)/Texas Instruments/ccsv6/tools/compiler/arm_15.12.3.LTS/bin/armcl" -mv7M3 --code_state=16 -me --include_path="C:/Program Files (x86)/Texas Instruments/ccsv6/tools/compiler/arm_15.12.3.LTS/include" --include_path="C:/Program Files (x86)/Texas Instruments/StellarisWare" --include_path="C:/Program Files (x86)/Texas Instruments/StellarisWare/boards/ek-lm3s8962" -g --gcc --define="ccs" --define=PART_LM3S8962 --diag_wrap=off --diag_warning=225 --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="rit128x96x4.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

ustdlib.obj: C:/Program\ Files\ (x86)/Texas\ Instruments/StellarisWare/utils/ustdlib.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/Program Files (x86)/Texas Instruments/ccsv6/tools/compiler/arm_15.12.3.LTS/bin/armcl" -mv7M3 --code_state=16 -me --include_path="C:/Program Files (x86)/Texas Instruments/ccsv6/tools/compiler/arm_15.12.3.LTS/include" --include_path="C:/Program Files (x86)/Texas Instruments/StellarisWare" --include_path="C:/Program Files (x86)/Texas Instruments/StellarisWare/boards/ek-lm3s8962" -g --gcc --define="ccs" --define=PART_LM3S8962 --diag_wrap=off --diag_warning=225 --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="ustdlib.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


