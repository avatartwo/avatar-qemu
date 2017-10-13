
#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/error-report.h"
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

#include "hw/avatar/interrupts.h"


static QemuAvatarMessageQueue *rx_queue_ref = NULL;
static QemuAvatarMessageQueue *tx_queue_ref = NULL;
static uint64_t req_id;

static bool armv7m_exception_handling_enabled = false;


void qmp_avatar_armv7m_enable_irq(const char *rx_queue_name,
                                  const char *tx_queue_name, Error **errp)
{
    if(rx_queue_ref == NULL){
        rx_queue_ref = malloc(sizeof(QemuAvatarMessageQueue));
        qemu_avatar_mq_open_read(rx_queue_ref, rx_queue_name,
                sizeof(V7MInterruptResp));
    }
    if(tx_queue_ref == NULL){
        tx_queue_ref = malloc(sizeof(QemuAvatarMessageQueue));
        qemu_avatar_mq_open_write(tx_queue_ref, tx_queue_name,
                sizeof(V7MInterruptReq));
    }

    armv7m_exception_handling_enabled = true;
    qemu_log_mask(LOG_AVATAR, "armv7m interrupt injection enabled\n");
    qemu_log_flush();
}

void qmp_avatar_armv7m_disable_irq(Error **errp)
{
    qemu_log_mask(LOG_AVATAR, "armv7m interrupt injection disabled\n");
    armv7m_exception_handling_enabled = false;
    qemu_log_flush();
}

void qmp_avatar_armv7m_inject_irq(int64_t num_cpu,
                                  int64_t num_irq, Error **errp)
{
#ifdef TARGET_ARM
    if(!armv7m_exception_handling_enabled){
        qemu_log_mask(LOG_AVATAR, "invalid armv7m interrupt injection attempt");
        qemu_log_flush();
    }
    ARMCPU *armcpu = ARM_CPU(qemu_get_cpu(num_cpu));
    CPUARMState *env = &armcpu->env;
    armv7m_nvic_set_pending(env->nvic, num_irq);
#endif
}


void avatar_armv7m_exception_exit(int irq, uint32_t type)
{
    int ret;
    V7MInterruptResp resp;
    V7MInterruptReq request = {req_id++, irq, type};

    memset(&resp, 0, sizeof(resp));

    qemu_avatar_mq_send(tx_queue_ref, &request, sizeof(request));
    ret = qemu_avatar_mq_receive(rx_queue_ref, &resp, sizeof(resp));

    if(!resp.success || (resp.id != request.id)){
        error_report("ARMv7mInterruptRequest failed (%d)!\n", ret);
        exit(1);
    }
}

