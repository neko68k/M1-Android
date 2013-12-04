package com.neko68k.M1;

import java.util.Arrays;

public class M1UpdateThread extends Thread {
	final int MAX_BUFFERS = 32;
	// final int MAX_BUFFERS = 4;
	volatile private byte buffers[][] = new byte[MAX_BUFFERS][]; // two
																	// channels,
																	// 16-bit
	volatile int count;
	int localCountIn;
	int localCountOut;
	int inIndex;
	int outIndex;
	volatile Semaphore notFull = new Semaphore("notFull", 0, false);
	volatile Semaphore notEmpty = new Semaphore("notEmpty", 0, false);
	private boolean playing = true;
	private boolean paused = false;
	private boolean stopped = true;
	private boolean quit = false;

	public M1UpdateThread(String threadName) {
		setName(threadName);
		count = 0;
		inIndex = 0;
		outIndex = 0;
		localCountIn = 0;
		localCountOut = 0;
	}

	public void run() {
		byte buffer[];
		while (playing == true) {
			if (paused == false) {
				buffer = NDKBridge.m1sdrGenerationCallback();
				make_product(buffer);

			}
		}

	}

	public void PlayStart() {
		playing = true;
		paused = false;
		stopped = false;
		if (this.getState() == Thread.State.NEW) {
			this.start();
		}
	}

	public void PlayQuit() {
		paused = true;
		stopped = true;
		quit = true;
		while (playing == true) {

		}
	}

	public void PlayStop() {
		// retrigger song on play if(stopped)
		paused = true;
		stopped = true;
	}

	public void PlayPause() {
		paused = true;
	}

	public void PlayUnPause() {
		paused = false;
	}

	private void clearBuffers() {
		int i = 0;
		for (i = 0; i < MAX_BUFFERS; i++) {
			Arrays.fill(buffers[i], (byte) 0);
		}
	}

	public void make_product(byte[] value) {
		if (localCountIn == MAX_BUFFERS)
			notFull.sem_wait();

		buffers[inIndex] = value;
		synchronized (this) {
			count = count + 1;
			localCountIn = count;
		}

		if (localCountIn == 1)
			notEmpty.sem_signal();

		inIndex = (inIndex + 1);
		if (inIndex == MAX_BUFFERS) {

			inIndex = 0;
		}

		if (stopped == true) {
			NDKBridge.stop();
			clearBuffers();
			if (quit == true) {
				playing = false;
			}
		}
	}

	public byte[] take_product() {
		byte buffer[];

		if (localCountOut == 0)
			notEmpty.sem_wait(currentThread().getName());

		buffer = buffers[outIndex];
		synchronized (this) {
			count = count - 1;
			localCountOut = count;
		}

		if (localCountOut == MAX_BUFFERS - 1)
			notFull.sem_signal(currentThread().getName());

		outIndex = outIndex + 1;
		if (outIndex == MAX_BUFFERS)
			outIndex = 0;

		return (buffer);
	}
}
