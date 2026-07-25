PRESERVE8
THUMB

AREA |.text|, CODE, READONLY

xPortPendSVHandler
    mrs r0, psp
    isb

    ldmfd r0!, {r4-r11}
    stmfd sp!, {r0}

    ldr r0, =xCurrentTCB
    ldr r0, [r0]
    str r0, [r0]

    ldmfd sp!, {r0}
    str r0, [r0]

    ldmfd r0!, {r4-r11}
    msr psp, r0
    isb

    bx lr

xPortSysTickHandler
    push {r4, r5, lr}

    bl vTaskIncrementTick

    bl vTaskSwitchContext

    pop {r4, r5, lr}
    bx lr

    END
