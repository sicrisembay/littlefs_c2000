/*!
 * \file cli.h
 */
#ifndef COMM_CLI_H
#define COMM_CLI_H

#include "driver_def.h"

#if defined(CONFIG_USE_CLI)


/*!
 * \page page_cli Command Line Interface
 * \section section_cli_intf CLI Interface
 * \subsection subsect_cli_init CLI_init
 * <PRE>void CLI_init(void);</PRE>
 *
 * This function initializes the Command Line Interface.
 *
 * \param None
 *
 * \return void
 */
void CLI_init(void);


#endif /* #if defined(CONFIG_USE_CLI) */

#endif /* COMM_CLI_H */
