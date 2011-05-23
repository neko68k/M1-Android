/*
    m1queue.c: simple command queue for M1

 */

#include "m1snd.h"
#include "m1ui.h"
#include "m1queue.h"

#define MAX_QUEUE_DEPTH (512)
#define INVALID_PREFIX (0xbeef)

static int init_cmd;
static int cmd_qptr;
static int cmd_qtop;
static int cmd_prefix;
static int boottime;
static volatile int cmd_qmutex;
static unsigned int cmd_queue[MAX_QUEUE_DEPTH];
static int *cmd_suffixstr, *cmd_prefixstr;
static int cmd_noprefixonstop;

void m1snd_addToCmdQueueRaw(int cmd)
{
	cmd_queue[cmd_qtop++] = cmd;
}

void m1snd_addToCmdQueue(int cmd)
{
	m1snd_resetSilenceCount();

	if ((cmd != games[curgame].stopcmd) || (!cmd_noprefixonstop))
	{
		// add prefix if one exists (deprecated)
		if (cmd_prefix != INVALID_PREFIX)
		{
			cmd_queue[cmd_qtop++] = cmd_prefix;
		}

		// add prefix string if one exists
		if (cmd_prefixstr != (int *)NULL)
		{
			int i = 0;

			while (cmd_prefixstr[i] != -1)
			{
				cmd_queue[cmd_qtop++] = cmd_prefixstr[i++];
			}
		}
	}

	cmd_queue[cmd_qtop++] = cmd;

	// add suffix string if one exists
	if (cmd_suffixstr != (int *)NULL)
	{
		int i = 0;

		while (cmd_suffixstr[i] != -1)
		{
			cmd_queue[cmd_qtop++] = cmd_suffixstr[i++];
		}
	}
}

void m1snd_setNoPrefixOnStop(void)
{
	cmd_noprefixonstop = 1;
}

void m1snd_setCmdPrefix(int cmd)
{
	cmd_prefix = cmd;
}

void m1snd_setCmdPrefixStr(int *prefix)
{
	cmd_prefixstr = prefix;
}

void m1snd_setCmdSuffixStr(int *suffix)
{
	cmd_suffixstr = suffix;
}

static void queue_timercb(int refcon)
{
	m1snd_getCurBoard()->SendCmd(refcon, 0);

	m1snd_resetSilenceCount();
	m1snd_initNormalizeState();

	cmd_qptr++;
	if (cmd_qptr == cmd_qtop)
	{
		m1snd_setPostVolume(1.0f);
		cmd_qptr = cmd_qtop = 0;
	}

	cmd_qmutex = 0;
}

static void queue_startcb(int refcon)
{
	timer_setturbo(1);
	m1ui_message(m1ui_this, M1_MSG_BOOTFINISHED, NULL, 0);
	cmd_qmutex = 0;
	boottime = 0;
	m1ui_message(m1ui_this, M1_MSG_STARTINGSONG, NULL, init_cmd);
}

void m1snd_processQueue(void)
{
	// if a timer is in progress, don't add more
	if (cmd_qmutex)
	{
		return;
	}

	if (cmd_qptr != cmd_qtop)
	{
		cmd_qmutex = 1;
		timer_set(TIME_IN_MSEC(m1snd_getCurBoard()->interCmdTime), cmd_queue[cmd_qptr], queue_timercb);
	}
}

extern void mac_start_timer(void);

void m1snd_startQueue(void)
{
	cmd_qmutex = 1;
	timer_setturbo(8);
	timer_set(TIME_IN_MSEC(m1snd_getCurBoard()->startupTime), 0, queue_startcb);
	m1ui_message(m1ui_this, M1_MSG_BOOTING, NULL, 0);

	boottime = 1;
}

int m1snd_getQueueBootTime(void)
{
	return boottime;
}

void m1snd_initQueue(void)
{
	cmd_qptr = cmd_qtop = 0;
	cmd_qmutex = 0;
	cmd_prefix = INVALID_PREFIX;
	cmd_noprefixonstop = 0;
	cmd_suffixstr = (int *)NULL;
}

int m1snd_isQueueBusy(void)
{
	if (cmd_qmutex)
	{
		return 1;
	}

	return 0;
}

void m1snd_setQueueInitCmd(int cmd)
{
	init_cmd = cmd;
}
