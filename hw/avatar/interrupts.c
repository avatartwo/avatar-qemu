
#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qmp-commands.h"
#include "qapi/error.h"

#include "hw/sysbus.h"
#include "sysemu/sysemu.h"

#ifdef TARGET_ARM
#include "target/arm/cpu.h"
#elif TARGET_MIPS
#include "target/mips/cpu.h"
#endif

void qmp_avatar_arm_irq(int64_t num_cpu, int64_t num_irq, Error **errp)
{
#ifdef TARGET_ARM
    printf("In do_interrupt\n");
    CPUState *cpu = CPU(qemu_get_cpu(num_cpu));
    //cpu_interrupt(cpu, num_irq);
    ARMCPU *armcpu = ARM_CPU(qemu_get_cpu(num_cpu));
    //armcpu->pending |= 1<< irq
    CPUARMState *env = &armcpu->env;
    printf("got cpu");
    armv7m_nvic_set_pending(env->nvic, num_irq);
    printf("survived set pending");
#endif

}

