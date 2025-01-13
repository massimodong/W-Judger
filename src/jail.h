#ifndef _JAIL_H_
#define _JAIL_H_

#include <unistd.h>

const int JUDGER_UID = 65534;

static void jail(){
	while(chroot("./") == -1);

	while(setgid(JUDGER_UID) == -1);
	while(setuid(JUDGER_UID) == -1);
	while(setresuid(JUDGER_UID, JUDGER_UID, JUDGER_UID) == -1);
}

#endif // _JAIL_H_
