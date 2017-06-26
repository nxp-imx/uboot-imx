.section .firmware_imem,#alloc
        .incbin "lpddr4_pmu_train_imem.bin"
.section .firmware_dmem,#alloc
        .incbin "lpddr4_pmu_train_dmem.bin"
