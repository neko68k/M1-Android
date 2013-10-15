package com.neko68k.M1;

public class Game {
		int index;
		String title;
		String year;
		String romname;
		String mfg;
		String sys;
		String cpu;
		String sound1;
		String sound2;
		String sound3;
		String sound4;		
		Integer listavail;
		public Game(int index, String title, String year, String romname,
				String mfg, String sys, String cpu, String sound1,
				String sound2, String sound3, String sound4, String sound5,
				Integer listavail) {
			this.index = index;
			this.title = title;
			this.year = year;
			this.romname = romname;
			this.mfg = mfg;
			this.sys = sys;
			this.cpu = cpu;
			this.sound1 = sound1;
			this.sound2 = sound2;
			this.sound3 = sound3;
			this.sound4 = sound4;		
			this.listavail = listavail;
		}
		public Game() {
			
		}
		public int getIndex() {
			return index;
		}
		public void setIndex(int index) {
			this.index = index;
		}
		public String getTitle() {
			return title;
		}
		public void setTitle(String title) {
			this.title = title;
		}
		public String getYear() {
			return year;
		}
		public void setYear(String year) {
			this.year = year;
		}
		public String getRomname() {
			return romname;
		}
		public void setRomname(String romname) {
			this.romname = romname;
		}
		public String getMfg() {
			return mfg;
		}
		public void setMfg(String mfg) {
			this.mfg = mfg;
		}
		public String getSys() {
			return sys;
		}
		public void setSys(String sys) {
			this.sys = sys;
		}
		public String getCpu() {
			return cpu;
		}
		public void setCpu(String cpu) {
			this.cpu = cpu;
		}
		public String getSound1() {
			return sound1;
		}
		public void setSound1(String sound1) {
			this.sound1 = sound1;
		}
		public String getSound2() {
			return sound2;
		}
		public void setSound2(String sound2) {
			this.sound2 = sound2;
		}
		public String getSound3() {
			return sound3;
		}
		public void setSound3(String sound3) {
			this.sound3 = sound3;
		}
		public String getSound4() {
			return sound4;
		}
		public void setSound4(String sound4) {
			this.sound4 = sound4;
		}
		public Integer getListavail() {
			return listavail;
		}
		public void setListavail(Integer listavail) {
			this.listavail = listavail;
		}

}
