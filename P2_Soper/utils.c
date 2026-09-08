#include "utils.h"

/** Variable global que indica si se ha recibido la alarma. 
 *  Se declara como volatile sig_atomic_t para garantizar operaciones atómicas y seguras en señales.
 */
volatile sig_atomic_t got_alarm = 0;

/** Variable global que indica si se ha recibido la señal SIGUSR1.
 *  Se declara como volatile sig_atomic_t para garantizar operaciones atómicas y seguras en señales.
 */
volatile sig_atomic_t got_usr1 = 0;

/** Variable global que indica si se ha recibido la señal SIGUSR2.
 *  Se declara como volatile sig_atomic_t para garantizar operaciones atómicas y seguras en señales.
 */
volatile sig_atomic_t got_usr2 = 0;

/** Variable global que indica si se ha iniciado la fase de terminación del proceso.
 *  Se declara como volatile sig_atomic_t para garantizar operaciones atómicas y seguras en señales.
 */
volatile sig_atomic_t terminating = 0;

