
#include <power/reboot.h>
#include <logging/log_ctrl.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(os);

__weak void k_sys_fatal_error_handler(unsigned int reason,
                                      const z_arch_esf_t *esf)
{
        ARG_UNUSED(esf);

        LOG_PANIC();
        LOG_ERR("Restarting system");
        sys_reboot(0);
        CODE_UNREACHABLE;
}
