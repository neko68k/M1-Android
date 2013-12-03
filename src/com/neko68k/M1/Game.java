package com.neko68k.M1;

import android.database.Cursor;
import android.os.Parcel;
import android.os.Parcelable;

public class Game implements Parcelable{
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
		Integer romavail;

		public Game() {
			this.index = 0;
			this.title="";
			this.year="";
			this.romname="";
			this.mfg="";
			this.sys="";
			this.cpu="";
			this.sound1="";
			this.sound2="";
			this.sound3="";
			this.sound4="";
			//this.sound5="";
			//this.listavail=0;
		}
		public Game(Cursor cursor){
			int tblYear = cursor.getColumnIndexOrThrow(GameListOpenHelper.KEY_YEAR);
            int tblMfg = cursor.getColumnIndexOrThrow(GameListOpenHelper.KEY_MFG);
            int tblBoard = cursor.getColumnIndexOrThrow(GameListOpenHelper.KEY_SYS);
            int tblHardware = cursor.getColumnIndexOrThrow(GameListOpenHelper.KEY_CPU);
            int tblSound1 = cursor.getColumnIndexOrThrow(GameListOpenHelper.KEY_SOUND1);
            int tblSound2 = cursor.getColumnIndexOrThrow(GameListOpenHelper.KEY_SOUND2);
            int tblSound3 = cursor.getColumnIndexOrThrow(GameListOpenHelper.KEY_SOUND3);
            int tblSound4 = cursor.getColumnIndexOrThrow(GameListOpenHelper.KEY_SOUND4);
            int tblRomname = cursor.getColumnIndexOrThrow(GameListOpenHelper.KEY_ROMNAME);
            int tblId = cursor.getColumnIndexOrThrow(GameListOpenHelper.KEY_ID);
			index = cursor.getInt(tblId);
			year = cursor.getString(tblYear);
			mfg = cursor.getString(tblMfg);
			sys = cursor.getString(tblBoard);
			cpu = cursor.getString(tblHardware);
			sound1 = cursor.getString(tblSound1);
			romname = cursor.getString(tblRomname);
					
					
			return;		
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
		public Integer getromavail() {
			return romavail;
		}
		public void setromavail(Integer romavail) {
			this.romavail = romavail;
		}
		public int describeContents() {
			// TODO Auto-generated method stub
			return 0;
		}
		public void writeToParcel(Parcel out, int flags) {
			// TODO Auto-generated method stub
			
			out.writeInt(index);
			out.writeString(title);
			out.writeString(year);
			out.writeString(romname);
			out.writeString(mfg);
			out.writeString(sys);
			out.writeString(cpu);
			out.writeString(sound1);
			out.writeString(sound2);
			out.writeString(sound3);
			out.writeString(sound4);		
			//out.writeInt(romavail);
			
		}
		public Game(Parcel in){
			index = in.readInt();
			title = in.readString();
			year = in.readString();
			romname = in.readString();
			mfg = in.readString();
			sys = in.readString();
			cpu = in.readString();
			sound1 = in.readString();
			sound2 = in.readString();
			sound3 = in.readString();
			sound4 = in.readString();
			//romavail = in.readInt();			
		}
		public static final Parcelable.Creator<Game>CREATOR = new
		        Parcelable.Creator<Game>(){
		               // @Override
		                public Game createFromParcel(Parcel source){                                
		                        return new Game(source);
		                }
		               // @Override
		                public Game[] newArray(int size){
		                        return new Game[size];
		                }
		        };
}
